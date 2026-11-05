//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "vertex.h"
#include "texture.h"
#include "height_field.h"

/*
 *	面法線の計算
 */
VEC3 CalcPlaneNormal(VEC3 v0, VEC3 v1, VEC3 v2){
	VEC3 v3, v4, tmp, n;

	v3 = v1-v0;
	v4 = v2-v1;
	D3DXVec3Cross(&tmp, &v3, &v4);
	D3DXVec3Normalize(&n, &tmp);
	return n;
}

/*
 *	コンストラクタ
 */
CHeightField::CHeightField(){
	m_fRender = FALSE;
	m_map = NULL;
}

/*
 *	デストラクタ
 */
CHeightField::~CHeightField(){
	Free();
}

/*
 *	地形生成
 *
 *	strFile	: 凹凸情報テクスチャー（グレイスケール）
 *	pos		: 位置（中心）
 *	s		: スケール
 *	h		: 高さの最大値
 *	c		: 色
 *
 *	※一度に生成（レンダリング）できるポリゴンの数はビデオカードにより制限
 *	されます。そのため使用するテクスチャーは64x64以下にした方が無難です。
 *	大きなマップを作成する場合は、いくつかに分割して下さい。
 */
BOOL CHeightField::Generate(LPCSTR strFile, VEC3 pos, float s, float h, D3DCOLOR c){
	Free();	//	既存なら解放

	//	テクスチャーの読込み
	HRESULT hr;
	LPTEX8 pBump;

	hr = LOAD_TEXTURE_SYS(&pBump, strFile);

	if(FAILED(hr)){
		Debug("LOAD_TEXTURE_SYS:%s\n", strFile);
		return FALSE;
	}
	//	フォーマットを取得
	D3DSURFACE_DESC desc;

	pBump->GetLevelDesc(0, &desc);

	//	頂点テーブルを作成
	int w = desc.Width;
	int d = desc.Height;
	DWORD num = d*w*6;

	//	同時にレンダリングできるプリミティブ数（頂点／三角？）を超えている
	if(num>sv3.capsMaxPrim){
		Debug("Over max primitive count.\n");
		RELEASE(pBump);
		return FALSE;
	}

	VTX_NX *vt = new VTX_NX[num]; 

	//	テクスチャー(LV0)全体を読込み専用でロック
	D3DLOCKED_RECT rect;

	hr = pBump->LockRect(0, &rect, NULL, D3DLOCK_READONLY);

	LPBYTE pSrc = (LPBYTE)rect.pBits;	//	テクスチャの先頭アドレス
	int incSrc = rect.Pitch;	//	テクスチャ１ライン分のバイト数
	int incDst = w*6;			//	頂点テーブルの１ライン分のデータ数

	int x, z, L03, L11, L24, L55;	//	ピクセル座標, 頂点と対応する輝度
	DWORD off;	//	頂点オフセット
	VEC3 n1, n2;	//	面法線

	pos.x -= w*s/2;	//	フィールド中央を原点に
	pos.z -= d*s/2;

	//	輝度から高さを計算
	for(z = 0; z<d; z++){
		for(x = 0; x<w; x++){
			//	Ｒ値を代表して輝度とする
			L03 = *(pSrc+x*4 +1);

			if(z==d-1 && x==w-1){
				L11 = L55 = L24 = L03;
			}else if(z==d-1){
				L11 = L03;
				L55 = *(pSrc+(x+1)*4 +1);	//	ARGB中のRを指定するので+1
				L24 = L55;
			}else if(x==w-1){
				L11 = *(pSrc+incSrc+x*4 +1);
				L55 = L03;
				L24 = L11;
			}else{
				L11 = *(pSrc+incSrc+x*4 +1);
				L55 = *(pSrc+(x+1)*4 +1);
				L24 = *(pSrc+incSrc+(x+1)*4 +1);
			}

			off = z*incDst+x*6;	//	オフセット値の計算

			//	頂点座標の計算
			vt[off].x = pos.x+x*s;
			vt[off].z = pos.z+z*s;
			vt[off].y = pos.y+h*L03/256;
			vt[off].d = c;
			vt[off].u = (float)x/w;
			vt[off].v = (float)z/d;

			vt[off+1] = vt[off];
			vt[off+1].y = pos.y+h*L11/256;
			vt[off+1].z += s;
			vt[off+1].v += (float)1/d;

			vt[off+2] = vt[off];
			vt[off+2].x += s;
			vt[off+2].z += s;
			vt[off+2].y = pos.y+h*L24/256;
			vt[off+2].u += (float)1/w;
			vt[off+2].v += (float)1/d;

			vt[off+3] = vt[off];
			vt[off+4] = vt[off+2];

			vt[off+5] = vt[off];
			vt[off+5].x += s;
			vt[off+5].y = pos.y+h*L55/256;
			vt[off+5].u += (float)1/w;

			//	面法線を計算
			n1 = CalcPlaneNormal(
				VEC3(vt[off ].x, vt[off ].y, vt[off ].z),
				VEC3(vt[off+1].x, vt[off+1].y, vt[off+1].z),
				VEC3(vt[off+2].x, vt[off+2].y, vt[off+2].z));
			n2 = CalcPlaneNormal(
				VEC3(vt[off+3].x, vt[off+3].y, vt[off+3].z),
				VEC3(vt[off+4].x, vt[off+4].y, vt[off+4].z),
				VEC3(vt[off+5].x, vt[off+5].y, vt[off+5].z));

			//	頂点に分配
			vt[off ].n = vt[off+1].n = vt[off+2].n = n1;
			vt[off+3].n = vt[off+4].n = vt[off+5].n = n2;
		}
		pSrc += incSrc;
	}
	//	ロック解除
	pBump->UnlockRect(0);
	RELEASE(pBump);

	//	法線の補間
	int idx[6];
	off = 0;

	for(z = 0; z<d; z++){
		for(x = 0; x<w; x++){
			idx[0] = off+2;
			idx[1] = off+4;
			idx[2] = (x==w-1) ? off+1 : off+6+1;
			idx[3] = (z==d-1) ? off+5 : off+incDst+5;

			idx[4] = (z==d-1) ? ((x==w-1) ? off : off+6)
				: ((x==w-1) ? off+incDst : off+incDst+6);

			idx[5] = (z==d-1) ? ((x==w-1) ? off+3 : off+6+3)
				: ((x==w-1) ? off+incDst+3 : off+incDst+6+3);

			vt[idx[0]].n = vt[idx[1]].n = 
			vt[idx[2]].n = vt[idx[3]].n = 
			vt[idx[4]].n = vt[idx[5]].n = 
				(vt[idx[0]].n+vt[idx[1]].n +
				vt[idx[2]].n+vt[idx[3]].n +
				vt[idx[4]].n+vt[idx[5]].n)/6;

			off += 6;
		}
	}

	//	頂点バッファの作成
	m_vtx.Create(vt, FVF_NX, sizeof(VTX_NX)*num);
	m_fRender = TRUE;

	//	高度マップの保存
	m_map = new float[d*w];

	off = 0;

	for(z = 0; z<d; z++){
		for(x = 0; x<w; x++){
			m_map[off/6] = vt[off].y;
			off += 6;
		}
	}
	delete [] vt;

	m_scale = s;
	m_width = w;
	m_depth = d;

	return TRUE;
}

/*
 *	解放
 */
void CHeightField::Free(){
	if(m_map){
		delete [] m_map;
		m_map = NULL;
	}
}

/*
 *	レンダリング
 *
 *	※事前にテクスチャーを指定しておくこと。
 */
void CHeightField::Render(){
	if(m_fRender) m_vtx.RenderTL();
}

/*
 *	指定位置の高度（Y座標）を返す
 */
float CHeightField::GetHeight(float x, float z){
	if(!m_map) return 0;

	//	フィールドの座標系に変換
	x = min(m_width-1, max(0, x/m_scale+m_width/2));
	z = min(m_depth-1, max(0, z/m_scale+m_depth/2));

	int ix0 = floor(x);
	int iz0 = floor(z);
	int ix1 = (ix0+1)&(m_width-1);
	int iz1 = (iz0+1)&(m_depth-1);

	//	面上での座標系に変換
	x -= ix0;
	z -= iz0;

	//	4点の高さを取得
	float h0 = m_map[iz0*m_width+ix0];
	float h1 = m_map[iz0*m_width+ix1];
	float h2 = m_map[iz1*m_width+ix0];
	float h3 = m_map[iz1*m_width+ix1];

	return (h0+(h1-h0)*x)*(1-z)+(h2+(h3-h2)*x)*z;
}
