//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "vertex.h"

/*
 *	３角形の法線ベクトルを計算
 */
void CalcNormal(VEC3 t[3], VEC3 *n){
	VEC3 u = t[1]-t[0];
	VEC3 v = t[2]-t[0];
	VEC3 tmp;

	D3DXVec3Cross(&tmp, &u, &v); 
	D3DXVec3Normalize(n, &tmp);
}

/*
 *	VTX_LXに矩形情報トライアングルリストでセットする
 *
 *	vt		: 頂点テーブルのアドレス
 *	x1, y1	: 始点
 *	x2, y2	: 終点
 */
void SetRectTL_LX(VTX_LX *vt, int x1, int y1, int x2, int y2, int z, D3DCOLOR c){
	SetVTX_LX(vt , x1, y1, z, c, 0, 1);
	SetVTX_LX(vt+1, x1, y2, z, c, 0, 0);
	SetVTX_LX(vt+2, x2, y1, z, c, 1, 1);
	SetVTX_LX(vt+3, x2, y2, z, c, 1, 0);
	SetVTX_LX(vt+4, x2, y1, z, c, 1, 1);
	SetVTX_LX(vt+5, x1, y2, z, c, 0, 0);
}

/*
 *	コンストラクタ
 */
CVertex::CVertex(){
	m_pVB = NULL;
	m_fvf = 0;
	m_stride = 0;
	m_num = 0;
}

/*
 *	デストラクタ
 */
CVertex::~CVertex(){
	Free();
}

/*
 *	頂点バッファ作成
 *
 *	pSrc	: 頂点が格納された配列
 *	fvf		: 頂点フォーマット
 *	size	: 配列のサイズ
 */
BOOL CVertex::Create(LPVOID pSrc, DWORD fvf, UINT size){
	HRESULT hr;

	Free();	//	既存ならバッファを解放
	m_fvf = fvf;

	switch(m_fvf){
	case FVF_TL:	m_stride = sizeof(VTX_TL);	m_num = size/sizeof(VTX_TL);	break;
	case FVF_TLX:	m_stride = sizeof(VTX_TLX);	m_num = size/sizeof(VTX_TLX);	break;
	case FVF_L:		m_stride = sizeof(VTX_L);	m_num = size/sizeof(VTX_L);		break;
	case FVF_LX:	m_stride = sizeof(VTX_LX);	m_num = size/sizeof(VTX_LX);	break;
	case FVF_LX2:	m_stride = sizeof(VTX_LX2);	m_num = size/sizeof(VTX_LX2);	break;
	case FVF_N:		m_stride = sizeof(VTX_N);	m_num = size/sizeof(VTX_N);		break;
	case FVF_NX:	m_stride = sizeof(VTX_NX);	m_num = size/sizeof(VTX_NX);	break;
	case FVF_NX2:	m_stride = sizeof(VTX_NX2);	m_num = size/sizeof(VTX_NX2);	break;
	default:		return FALSE;
	}	//	D3DXGetFVFVertexSize()

	//	バッファの作成
	hr = sv3.pDev->CreateVertexBuffer(
		size, 0, fvf, D3DPOOL_MANAGED, &m_pVB);
	if(FAILED(hr)) return FALSE;

	//	ロックして書込み
	LPVOID pDst;

	hr = m_pVB->Lock(0, size, (BYTE **)&pDst, 0);

	if(FAILED(hr)) return FALSE;

	memcpy(pDst, pSrc, size);
	m_pVB->Unlock();

	return TRUE;
}

/*
 *	頂点バッファ解放
 */
void CVertex::Free(){
	RELEASE(m_pVB);
}

/*
 *	頂点バッファをロック
 *
 *	※ロック後は必ずアンロックすること。
 */
BOOL CVertex::Lock(LPVOID *ppBuf){
	if(FAILED(m_pVB->Lock(0, 0/*全体*/, (BYTE **)ppBuf, 0)))
		return FALSE;
	else
		return TRUE;
}

/*
 *	頂点バッファをアンロック
 */
void CVertex::Unlock(){
	m_pVB->Unlock();
}

/*
 *	※以下のメソッドはライト、マテリアル、テクスチャ、ワールドマトリクスの
 *	影響を受けます。
 *	
 *	ラィティング済み頂点を使用する場合はライティングを無効にしてから、
 *	テクスチャなし頂点を使用する場合はテクスチャをNULLにしてから行ってください。
 */
/*
 *	ポイントリスト
 */
void CVertex::RenderPL(){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_POINTLIST, 0, m_num);
}

/*
 *	ラインリスト
 */
void CVertex::RenderLL(){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_LINELIST, 0, m_num/2);
}

/*
 *	ラインストリップ
 *
 *	count	: 線の数
 */
void CVertex::RenderLS(UINT count){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_LINESTRIP, 0, count);
}

/*
 *	トライアングルリスト
 */
void CVertex::RenderTL(){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_num/3);
}

/*
 *	トライアングルストリップ
 *
 *	count	: 3角形の数
 */
void CVertex::RenderTS(UINT count){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, count);
}

/*
 *	トライアングルファン
 *
 *	count	: 3角形の数
 */
void CVertex::RenderTF(UINT count){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, count);
}

/*
 *	タイプ指定
 *
 *	type	: プリミティブタイプ
 *	count	: 3角形の数
 */
void CVertex::Render(PRIMTYPE type, UINT count){
	sv3.pDev->SetStreamSource(0, m_pVB, m_stride);
	sv3.pDev->SetVertexShader(m_fvf);
	sv3.pDev->DrawPrimitive(type, 0, count);
}
