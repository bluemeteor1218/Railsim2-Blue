//	Copyright (c) 2002 Midikyou

//#include "headers.h"
//#include "debug.h"
//#include "graphic.h"
//#include "view.h"
//#include "texture.h"
//#include "mesh.h"
//#include "object.h"
#include "../stdafx.h"

//	外部グローバル
extern int g_DispWidth;
extern int g_DispHeight;

//	内部グローバル
MAT8 CObject::matShadow = {{0, 0, 0, 0.5f}};

/*
 *	コンストラクタ
 */
CObject::CObject(){
	m_pMesh = NULL;
	m_pParent = NULL;
	m_mtx = MTX4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	m_scale = 1.0f;
}

/*
 *	デストラクタ
 */
CObject::~CObject(){
}

/*
 *	メッシュのアタッチ
 *
 *	pM	: メッシュ
 *	p	: 位置
 *	s	: スケール
 */
void CObject::SetMesh(CMesh *pM, VEC3 p, float s){
	m_pMesh = pM;
	m_mtx = MTX4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1);
	m_scale = s;
	SetPos(p);
	if(m_pMesh) SetBoxMinMax(&m_box, pM->m_min, pM->m_max);
}

/*
 *	影の色設定（全オブジェクト共通）
 *
 *	cv	: 色
 */
void CObject::SetShadowColor(D3DCOLORVALUE cv){
	matShadow.Diffuse = cv;
}

/*
 *	スケールの設定（絶対）
 */
void CObject::SetScale(float s){
	float sr = s/m_scale;

	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixScaling(&mtxtmp, sr, sr, sr);
	m_mtx = mtxtmp*mtxold;
	m_scale = s;
}

/*
 *	スケールの設定（絶対）
 */
void CObject::SetScale(float sx, float sy, float sz){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixScaling(&mtxtmp, sx/m_scale, sy/m_scale, sz/m_scale);
	m_mtx = mtxtmp*mtxold;
	m_scale = 1.0f;
}

/*
 *	スケールの設定（相対）
 */
void CObject::Scale(float s){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixScaling(&mtxtmp, s, s, s);
	m_mtx = mtxtmp*mtxold;
	m_scale *= s;
}

/*
 *	位置指定（親スケールを無視）
 *
 *	v		: 3D座標
 */
void CObject::SetPosS(VEC3 v){
	if(m_pParent) v /= m_pParent->GetWScale();
	SetPos(v);
}

/*
 *	移動
 *
 *	v		: 移動量
 */
void CObject::Move(VEC3 v){
	v /= m_scale;	//	スケーリングの影響を回避

	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixTranslation(&mtxtmp, v.x, v.y, v.z);
	m_mtx = mtxtmp*mtxold;
}
//	原点中心
void CObject::Move2(VEC3 v){
	v /= m_scale;	//	スケーリングの影響を回避

	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixTranslation(&mtxtmp, v.x, v.y, v.z);
	m_mtx = mtxold*mtxtmp;
}

/*
 *	移動（親スケールを無視）
 *
 *	v		: 移動量
 */
void CObject::MoveS(VEC3 v){
	v /= GetWScale();	//	スケーリングの影響を回避

	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixTranslation(&mtxtmp, v.x, v.y, v.z);
	m_mtx = mtxtmp*mtxold;
}

/*
 *	X軸周りの回転
 *
 *	v		: 回転角（rad）
 *
 *	※回転の方向＝軸の＋方向に左手親指を向けた時の人差し指の方向
 */
void CObject::RotX(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationX(&mtxtmp, v);
	m_mtx = mtxtmp*mtxold;
}
//	原点中心
void CObject::RotX2(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationX(&mtxtmp, v);
	m_mtx = mtxold*mtxtmp;
}

/*
 *	Y軸周りの回転
 *
 *	v		: 回転角（rad）
 */
void CObject::RotY(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationY(&mtxtmp, v);
	m_mtx = mtxtmp*mtxold;
}
//	原点中心
void CObject::RotY2(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationY(&mtxtmp, v);
	m_mtx = mtxold*mtxtmp;
}

/*
 *	Z軸周りの回転
 *
 *	v		: 回転角（rad）
 */
void CObject::RotZ(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationZ(&mtxtmp, v);
	m_mtx = mtxtmp*mtxold;
}
//	原点中心
void CObject::RotZ2(float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationZ(&mtxtmp, v);
	m_mtx = mtxold*mtxtmp;
}

/*
 *	任意軸周りの回転
 *
 *	ax	: 軸ベクトル
 *	v		: 回転角（rad）
 */
void CObject::RotAxis(VEC3 ax, float v){
	MTX4 mtxold = m_mtx, mtxtmp;
	D3DXMatrixRotationAxis(&mtxtmp, &ax, v);
	m_mtx = mtxtmp*mtxold;
}

/*
 *	ビルボード回転（X, Z軸回転を固定）
 */
void CObject::Billboard(){
	//	オブジェクトからカメラへのベクトルを得る
	VEC3 v = GetVPos()-GetWPos();

	MTX4 mtxold = m_mtx;

	//	回転行列を得る
	if(v.x>0.0f)
		D3DXMatrixRotationY(&m_mtx, -atanf(v.z/v.x)+D3DX_PI/2);
	else
		D3DXMatrixRotationY(&m_mtx, -atanf(v.z/v.x)-D3DX_PI/2);

	m_mtx._41 = mtxold._41;
	m_mtx._42 = mtxold._42;
	m_mtx._43 = mtxold._43;
	m_mtx._44 = 1;

	float s = m_scale; m_scale = 1;
	SetScale(s);
}

/*
 *	ビルボード回転（X, Y軸回転を固定）
 */
void CObject::Billboard2(){
	//	オブジェクトからカメラへのベクトルを得る
	VEC3 v = GetVPos()-GetWPos();

	MTX4 mtxold = m_mtx;

	//	回転行列を得る
	if(v.x>0.0f)
		D3DXMatrixRotationZ(&m_mtx, atanf(v.y/v.x)-D3DX_PI/2);
	else
		D3DXMatrixRotationZ(&m_mtx, atanf(v.y/v.x)+D3DX_PI/2);

	m_mtx._41 = mtxold._41;
	m_mtx._42 = mtxold._42;
	m_mtx._43 = mtxold._43;
	m_mtx._44 = 1;

	float s = m_scale; m_scale = 1;
	SetScale(s);
}

/*
 *	指定点の方を向く
 *
 *	at	: 位置
 */
void CObject::LookAt(VEC3 at){
	VEC3 dir, up(0, 1, 0), right;

	dir = at-GetWPos();
	D3DXVec3Normalize(&dir, &dir);
	D3DXVec3Cross(&right, &up, &dir);
	D3DXVec3Normalize(&right, &right);
	D3DXVec3Cross(&up, &dir, &right);
	D3DXVec3Normalize(&up, &up);

	m_mtx._11 = right.x; m_mtx._12 = right.y; m_mtx._13 = right.z;
	m_mtx._21 = up.x;	m_mtx._22 = up.y;	m_mtx._23 = up.z;
	m_mtx._31 = dir.x; m_mtx._32 = dir.y; m_mtx._33 = dir.z;
	m_mtx._44 = 1;

	float s = m_scale; m_scale = 1;
	SetScale(s);
}

/*
 *	指定点の方を向く（X, Z軸回転を固定）
 *
 *	at	: 位置
 */
void CObject::LookAt2(VEC3 at){
	//	オブジェクトから指定点へのベクトルを得る
	VEC3 v = at-GetWPos();
	float rx = 0, ry, rz = 0;

	//	回転角を得る
	if	(v.x> 0.0f)	ry = -atanf(v.z/v.x)+D3DX_PI/2;
	else if(v.x< 0.0f)	ry = -atanf(v.z/v.x)-D3DX_PI/2;
	else if(v.z>=0.0f)	ry = 0;
	else if(v.z< 0.0f)	ry = -D3DX_PI;

	//	回転行列を計算
	MTX4 mtxold = m_mtx;

	D3DXMatrixRotationYawPitchRoll(&m_mtx, ry, rx, rz);

	m_mtx._41 = mtxold._41;
	m_mtx._42 = mtxold._42;
	m_mtx._43 = mtxold._43;
	m_mtx._44 = 1;

	float s = m_scale; m_scale = 1;
	SetScale(s);
}

/*
 *	向きを指定
 *
 *	dir	: 向き
 */
void CObject::SetDir(VEC3 dir, VEC3 up){
	VEC3 right;

	D3DXVec3Normalize(&dir, &dir);
	D3DXVec3Cross(&right, &up, &dir);
	D3DXVec3Normalize(&right, &right);
	D3DXVec3Cross(&up, &dir, &right);
	D3DXVec3Normalize(&up, &up);

	m_mtx._11 = right.x; m_mtx._12 = right.y; m_mtx._13 = right.z;
	m_mtx._21 = up.x;	m_mtx._22 = up.y;	m_mtx._23 = up.z;
	m_mtx._31 = dir.x; m_mtx._32 = dir.y; m_mtx._33 = dir.z;
	m_mtx._44 = 1;

	float s = m_scale; m_scale = 1;
	SetScale(s);
}

/*
 *	位置取得(親スケールを考慮する)
 */
VEC3 CObject::GetPosS(){
	if(m_pParent)
		return m_pParent->GetWScale()*GetPos();
	else
		return GetPos();
}

/*
 *	位置取得(ワールド座標系)
 */
VEC3 CObject::GetWPos(){
	if(m_pParent){
		MTX4 mtxtmp = GetWMatrix();
		return VEC3(mtxtmp._41, mtxtmp._42, mtxtmp._43);
	}else
		return GetPos();
}

/*
 *	Rightベクトル取得(ワールド座標系)
 */
VEC3 CObject::GetWRight(){
	if(m_pParent){
		MTX4 mtxtmp = GetWMatrix();
		return VEC3(mtxtmp._11, mtxtmp._12, mtxtmp._13);
	}else
		return GetRight();
}

/*
 *	Upベクトル取得(ワールド座標系)
 */
VEC3 CObject::GetWUp(){
	if(m_pParent){
		MTX4 mtxtmp = GetWMatrix();
		return VEC3(mtxtmp._21, mtxtmp._22, mtxtmp._23);
	}else
		return GetUp();
}

/*
 *	Dirベクトル取得(ワールド座標系)
 */
VEC3 CObject::GetWDir(){
	if(m_pParent){
		MTX4 mtxtmp = GetWMatrix();
		return VEC3(mtxtmp._31, mtxtmp._32, mtxtmp._33);
	}else
		return GetDir();
}

/*
 *	変換行列の取得(ワールド座標系)
 */
MTX4 CObject::GetWMatrix(){
	if(m_pParent)
		return m_mtx*m_pParent->GetWMatrix();
	else
		return m_mtx;
}

/*
 *	スケールの取得(親スケール考慮、ワールド座標系のスケールとも考えられる)
 */
float CObject::GetWScale(){
	return m_pParent ? m_scale*m_pParent->GetWScale() : m_scale;
}

/*
 *	レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderCustom(CNamedObject *nobj){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderCustom(&GetWMatrix(), nobj);
		else m_pMesh->RenderCustom(&m_mtx, nobj);
	}
}

/*
 *	レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::Render(){
	if(m_pMesh){
		if(m_pParent) m_pMesh->Render(&GetWMatrix());
		else m_pMesh->Render(&m_mtx);
	}
}

/*
 *	アンビエントのみでレンダリング
 */
void CObject::RenderAmb(){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderAmb(&GetWMatrix());
		else m_pMesh->RenderAmb(&m_mtx);
	}
}

/*
 *	テクスチャ指定レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderT(LPTEX8 texalt){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderT(&GetWMatrix(), texalt);
		else m_pMesh->RenderT(&m_mtx, texalt);
	}
}

/*
 *	α値指定レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderA(float alphaalt){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderA(&GetWMatrix(), alphaalt);
		else m_pMesh->RenderA(&m_mtx, alphaalt);
	}
}

/*
 *	α加算レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderAP(float alphaplus){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderAP(&GetWMatrix(), alphaplus);
		else m_pMesh->RenderAP(&m_mtx, alphaplus);
	}
}

/*
 *	マテリアル指定レンダリング
 *
 *	pMat	: 代替マテリアル
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderSC(MAT8 *pMat){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderSC(&GetWMatrix(), pMat);
		else m_pMesh->RenderSC(&m_mtx, pMat);
	}
}

/*
 *	影のレンダリング
 *
 *	pMtx	: 投影行列（GetShadowMtx()で取得）
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CObject::RenderShadow(MTX4 *pMtx){
	if(m_pMesh){
		if(m_pParent) m_pMesh->RenderSC(&(GetWMatrix()*(*pMtx)), &matShadow);
		else m_pMesh->RenderSC(&(m_mtx*(*pMtx)), &matShadow);
	}
}

/*
 *	境界ボックスの取得
 */
float CObject::GetRadius(){
	return m_pMesh ? m_pMesh->m_radius*GetWScale() : 0.0f;
}

/*
 *	境界ボックスの取得
 */
VEC3 CObject::GetCenter(){
	if(!m_pMesh) return GetPos();

	MTX4 mtxtmp = m_pParent ? GetWMatrix() : m_mtx;
	VEC3 vTmp;
	D3DXVec3TransformCoord(&vTmp, &m_pMesh->m_center, &mtxtmp);
	return vTmp;
}

/*
 *	境界ボックスの取得
 */
BOX8 CObject::GetBox(){
	MTX4 mtxtmp = m_pParent ? GetWMatrix() : m_mtx;

	BOX8 box;
	VEC3 vTmp;

	for(int i = 0; i<8; i++){
		D3DXVec3TransformCoord(&vTmp, &m_box.v[i], &mtxtmp);
		box.v[i] = VEC3(vTmp.x, vTmp.y, vTmp.z);
	}
	return box;
}

/*
 *	境界球の取得
 *
 *	c		: 中心
 *	r		: 半径
 */
void CObject::GetSphere(VEC3 *c, float *r){
	*c = m_pMesh->m_center+GetWPos();
	*r = m_pMesh->m_radius *m_scale;
}

/*
 *	線分交差判定（境界ボックスと）
 *
 *	pos	: 線分の始点
 *	dir	: 線分の方向
 *
 *	※開発中
 */
BOOL CObject::IntersectB(VEC3 pos, VEC3 dir){
	VEC3 min = m_pMesh->m_min*m_scale;
	VEC3 max = m_pMesh->m_max*m_scale;

	//	{ 線分の座標系をオブジェクトに合わせる
	pos -= GetWPos();
	dir -= GetWPos();

	VEC3 vx = GetWRight();
	VEC3 vy = GetWUp();
	VEC3 vz = GetWDir();

	VEC3 pos2;
	pos2.x = D3DXVec3Dot(&pos, &vx);
	pos2.y = D3DXVec3Dot(&pos, &vy);
	pos2.z = D3DXVec3Dot(&pos, &vz);

	VEC3 dir2;
	dir2.x = D3DXVec3Dot(&dir, &vx);
	dir2.y = D3DXVec3Dot(&dir, &vy);
	dir2.z = D3DXVec3Dot(&dir, &vz);
	//	}
	//	TextF(0, 16, "%.1f %.1f %.1f\n", pos2.x, pos2.y, pos2.z);
	//	TextF(0, 32, "%.1f %.1f %.1f\n", dir2.x, dir2.y, dir2.z);
	return D3DXBoxBoundProbe(&min, &max, &pos2, &dir2);
}

/*
 *	線分交差判定（境界球と）
 *
 *	pos	: 線分の始点
 *	dir	: 線分の方向
 */
BOOL CObject::IntersectS(VEC3 pos, VEC3 dir){
	VEC3 center = m_pMesh->m_center+GetWPos();

	return D3DXSphereBoundProbe(
		&center,
		m_pMesh->m_radius *m_scale,
		&pos,
		&dir);
}

/*
 *	線分交差判定（メッシュとの詳細な判定）
 *
 *	pos	: 線分の始点
 *	dir	: 線分の方向
 *	hit	: 最初の交差点
 *	tri	: 交差した三角形(ローカル座標系)の頂点の格納先
 *	inv	: 裏表検出フラグ 1: front, 2: back
 *
 *	※沢山の物体を判定する場合はまず IntersectS() で粗く判定する方が良いです。
 */
BOOL CObject::Pick(VEC3 pos, VEC3 dir, VEC3 *hit, VEC3 tri[3], int inv){
	if(!m_pMesh) return FALSE;

	//	pos, dirをローカル座標系に変換
	MTX4 mw, mi;
	VEC3 lpos, pos2, dir2;

	mw = GetWMatrix();
	lpos = GetWPos();
	D3DXMatrixInverse(&mi, NULL, &mw);
	D3DXVec3TransformNormal(&pos2, &pos, &mi); 
	D3DXVec3TransformNormal(&dir2, &dir, &mi);
	D3DXVec3TransformNormal(&pos, &lpos, &mi);
	pos2 -= pos;

	//	メッシュをロックしてポリゴン単位で判定
	LPD3DXMESH pMesh = m_pMesh->GetObject();

	LPDIRECT3DVERTEXBUFFER8 pVB;
	LPDIRECT3DINDEXBUFFER8 pIB;
	pMesh->GetVertexBuffer(&pVB);
	pMesh->GetIndexBuffer(&pIB);

	BYTE* pIndTmp;
	BYTE* pVtx;
	pIB->Lock(0, 0, (BYTE **)&pIndTmp, D3DLOCK_READONLY);
	pVB->Lock(0, 0, (BYTE **)&pVtx, D3DLOCK_READONLY);

	UINT fvfSize = D3DXGetFVFVertexSize(pMesh->GetFVF());
//	Dialog("fvf = %d\nfvfSize = %d", pMesh->GetFVF(), fvfSize);
	DWORD dwNumFaces = pMesh->GetNumFaces();
	DWORD i, dwCount = 0;

	D3DINDEXBUFFER_DESC ib_desc;
	pIB->GetDesc(&ib_desc);
	int prim_size = ib_desc.Size/(dwNumFaces*3);

	VEC3 v0, v1, v2, tmp;
	float t, u, v;

	#define PROC_FACES( ) \
		do { \
			for(i = 0; i<dwNumFaces; i++){ \
				v0 = *(VEC3 *)(pVtx+fvfSize*pInd[3*i+0]); \
				v1 = *(VEC3 *)(pVtx+fvfSize*pInd[3*i+1]); \
				v2 = *(VEC3 *)(pVtx+fvfSize*pInd[3*i+2]); \
				if((inv&1) && IntersectTriangle(pos2, dir2, v0, v1, v2, &t, &u, &v) \
					|| (inv&2) && IntersectTriangle(pos2, dir2, v0, v2, v1, &t, &v, &u)){ \
					tmp = (v1-v0)*u+(v2-v0)*v+v0; \
					/* 進行方向の最近点を選択 */ \
					if(D3DXVec3Dot(&dir2, &(tmp-pos2))>=0.0f && (!dwCount || \
						D3DXVec3Length(&(tmp-pos2))<D3DXVec3Length(&(*hit-pos2)))){ \
						*hit = tmp; \
						if(tri){ tri[0] = v0; tri[1] = v1; tri[2] = v2; } \
						dwCount++; \
					} \
				} \
			} \
		} while(false)
	if(prim_size==2){
		WORD* pInd = (WORD*)pIndTmp;
		PROC_FACES();
/*	}else if(prim_size==4){
		DWORD* pInd = (DWORD*)pIndTmp;
	//	ErrorDialog("%d %d %d\n%d %d %d\n%d %d %d\n%d %d %d\n%d %d %d\n%d %d %d\n...",
	//		pInd[0+0],  pInd[0+1],  pInd[0+2],  pInd[3+0],  pInd[3+1],  pInd[3+2],
	//		pInd[6+0],  pInd[6+1],  pInd[6+2],  pInd[9+0],  pInd[9+1],  pInd[9+2],
	//		pInd[12+0], pInd[12+1], pInd[12+2], pInd[15+0], pInd[15+1], pInd[15+2]);
		PROC_FACES();*/
	}else{
		ErrorDialog("UNEXPECTED INDEX BUFFER FORMAT: %d", prim_size);
	}
	pVB->Unlock();
	pIB->Unlock();
	pVB->Release();
	pIB->Release();

	if(dwCount>0){
		D3DXVec3TransformNormal(&tmp, hit, &mw);
		*hit = tmp+lpos;
		if(tri){
			for(i = 0; i<3; i++){
				D3DXVec3TransformNormal(&tmp, &tri[i], &mw);
				tri[i] = tmp+lpos;
			}
		}
		return TRUE;
	}else{
		*hit = VEC3(0, 0, 0);
		return FALSE;
	}
}

/*
 *	可視判定
 *
 *	戻り値	: TRUE = 可視、FALSE = 不可視
 *
 *	※不可視ポリゴンのクリップはDirect3Dが勝手にやってくれる設定にしてあるので、
 *	あまり出番はないかも・・・
 *	(それでも不可視時に描画スキップすると若干のパフォーマンスアップあり)
 */
BOOL CObject::IsVisible(){
	//	中心座標から判断
	VEC3 pos = WorldToScreen(GetWPos());

	if(pos.z>=0
		&& 0<=pos.y && pos.y<=g_DispHeight
		&& 0<=pos.x && pos.x<=g_DispWidth) return TRUE;

	//	境界ボックスから判断
	BOX8 box = GetBox();

	for(int i = 0; i<8; i++){
		pos = WorldToScreen(box.v[i]);

		if(pos.z>=0
			&& 0<=pos.y && pos.y<=g_DispHeight
			&& 0<=pos.x && pos.x<=g_DispWidth) return TRUE;
	}
	return FALSE;
}

/*
 *	境界ボックステスト
 *
 *	pDst	: 境界ボックスA
 *	pSrc	: 境界ボックスB
 *
 *	※SphereTestよりも判定は厳密ですが、速度が若干落ちます。
 *	※辺と辺だけで接触している場合は検出できません。
 */
BOOL BoxTest(BOX8 *pDst, BOX8 *pSrc){
	//	Aの各面を作成
	D3DXPLANE p[6];

	D3DXPlaneFromPoints(&p[0], &pDst->v[0], &pDst->v[1], &pDst->v[2]);	//	right
	D3DXPlaneFromPoints(&p[1], &pDst->v[2], &pDst->v[3], &pDst->v[7]);	//	bottom
	D3DXPlaneFromPoints(&p[2], &pDst->v[4], &pDst->v[6], &pDst->v[7]);	//	left
	D3DXPlaneFromPoints(&p[3], &pDst->v[0], &pDst->v[4], &pDst->v[5]);	//	top
	D3DXPlaneFromPoints(&p[4], &pDst->v[0], &pDst->v[2], &pDst->v[6]);	//	back
	D3DXPlaneFromPoints(&p[5], &pDst->v[1], &pDst->v[5], &pDst->v[7]);	//	front

	//	Bの頂点が１つでも
	for(int j = 0; j<8; j++){
		BOOL f = TRUE;

		//	Aの全ての面の内側にあるか？
		for(int i = 0; i<6; i++){
			//	外側にあったら別の頂点を調べる
			if(D3DXPlaneDotCoord(&p[i], &pSrc->v[j])<0){
				f = FALSE;
				break;
			}
		}
		if(f) return TRUE;
	}
	return FALSE;
}

/*
 *	三角形と線分の交差判定
 */
BOOL IntersectTriangle(
	const VEC3& orig, const VEC3& dir,
	VEC3& v0, VEC3& v1, VEC3& v2,
	FLOAT *t, FLOAT *u, FLOAT *v){
	//	※SDKサンプルそのまま(^^;

	//	 Find vectors for two edges sharing vert0
	D3DXVECTOR3 edge1 = v1-v0;
	D3DXVECTOR3 edge2 = v2-v0;

	//	 Begin calculating determinant-also used to calculate U parameter
	D3DXVECTOR3 pvec;
	D3DXVec3Cross(&pvec, &dir, &edge2);

	//	 If determinant is near zero, ray lies in plane of triangle
	FLOAT det = D3DXVec3Dot(&edge1, &pvec);
//	if(det<0.0001f) return FALSE;
	if(det<=0.0f) return FALSE;

	//	 Calculate distance from vert0 to ray origin
	D3DXVECTOR3 tvec = orig-v0;

	//	 Calculate U parameter and test bounds
	*u = D3DXVec3Dot(&tvec, &pvec);
	if(*u<0.0f || *u>det) return FALSE;

	//	 Prepare to test V parameter
	D3DXVECTOR3 qvec;
	D3DXVec3Cross(&qvec, &tvec, &edge1);

	//	 Calculate V parameter and test bounds
	*v = D3DXVec3Dot(&dir, &qvec);
	if(*v<0.0f || *u+*v>det) return FALSE;

	//	 Calculate t, scale parameters, ray intersects triangle
	*t = D3DXVec3Dot(&edge2, &qvec);
	FLOAT fInvDet = 1.0f/det;
	*t *= fInvDet;
	*u *= fInvDet;
	*v *= fInvDet;

	return TRUE;
}
