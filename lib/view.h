//	Copyright (c) 2002 Midikyou

void RotVX(float v);
void RotVY(float v);
void RotVZ(float v);
void RotVUp(float v);
void MoveV(VEC3 v);
void MoveVW(VEC3 v);
void SetVPos(VEC3 v);
void SetView(VEC3 pos, VEC3 dir, VEC3 up = VEC3(0.0f, 1.0f, 0.0f));
void SetViewport(DWORD x, DWORD y, DWORD w, DWORD h, float znear = 0.0f, float zfar = 1.0f);
void LookAtV(VEC3 at);
VEC3 WorldToScreen(VEC3 pos);
VEC3 ScreenToWorld(VEC3 pos);

/*
 *	ビュー変換行列の要素
 */
#define VMTX_11 sv3.mtxView._11
#define VMTX_12 sv3.mtxView._12
#define VMTX_13 sv3.mtxView._13
#define VMTX_14 sv3.mtxView._14
#define VMTX_21 sv3.mtxView._21
#define VMTX_22 sv3.mtxView._22
#define VMTX_23 sv3.mtxView._23
#define VMTX_24 sv3.mtxView._24
#define VMTX_31 sv3.mtxView._31
#define VMTX_32 sv3.mtxView._32
#define VMTX_33 sv3.mtxView._33
#define VMTX_34 sv3.mtxView._34
#define VMTX_41 sv3.mtxView._41
#define VMTX_42 sv3.mtxView._42
#define VMTX_43 sv3.mtxView._43
#define VMTX_44 sv3.mtxView._44

/*
 *	ビューの回転
 */
#define VMTX_ROT(func, v) { \
	sv3.mtxOld = sv3.mtxView; \
	func(&sv3.mtxTmp, v); \
	sv3.mtxView = sv3.mtxOld*sv3.mtxTmp; \
}

/*
 *	ビューの回転（任意の軸）
 */
#define VMTX_ROTA(axis, v) { \
	sv3.mtxOld = sv3.mtxView; \
	D3DXMatrixRotationAxis(&sv3.mtxTmp, &axis, v); \
	sv3.mtxView = sv3.mtxOld*sv3.mtxTmp; \
}

/*
 *	ビューの移動
 */
#define VMTX_MOVE(func, v) { \
	sv3.mtxOld = sv3.mtxView; \
	func(&sv3.mtxTmp, (v).x, (v).y, (v).z); \
	sv3.mtxView = sv3.mtxOld*sv3.mtxTmp; \
}

/*
 *	ビューの移動（ワールド座標）
 */
#define VMTX_MOVEW(func, v) { \
	func(&sv3.mtxTmp, (v).x, (v).y, (v).z); \
	sv3.mtxView = sv3.mtxTmp*sv3.mtxView; \
}

/*
 *	位置取得
 */
inline VEC3 GetVPos(){
	return -VEC3(
		VMTX_41*VMTX_11+VMTX_42*VMTX_12+VMTX_43*VMTX_13,
		VMTX_41*VMTX_21+VMTX_42*VMTX_22+VMTX_43*VMTX_23,
		VMTX_41*VMTX_31+VMTX_42*VMTX_32+VMTX_43*VMTX_33);
}

/*
 *	Rightベクトルの取得
 */
inline VEC3 GetVRight(){
	return VEC3(VMTX_11, VMTX_21, VMTX_31);
}

/*
 *	Upベクトルの取得
 */
inline VEC3 GetVUp(){
	return VEC3(VMTX_12, VMTX_22, VMTX_32);
}

/*
 *	Dirベクトルの取得
 */
inline VEC3 GetVDir(){
	return VEC3(VMTX_13, VMTX_23, VMTX_33);
}

/*
 *	ビュー変換行列の指定
 *
 *	※逆行列は更新しません。
 */
inline void SetVMatrix(MTX4 *pMtx){
	sv3.mtxView = *pMtx;
}

/*
 *	ビュー変換行列の取得
 */
inline MTX4 GetVMatrix(){
	return sv3.mtxView;
}

/*
 *	ビュー逆変換行列の取得
 *
 *	※BeginScene()で更新されます。
 */
inline MTX4 GetVInvMatrix(){
	return sv3.mtxViewInv;
}

/*
 *	面法線の計算
 */
inline VEC3 CalcPlaneNormal(VEC3 v0, VEC3 v1, VEC3 v2){
	VEC3 v3, v4, tmp, n;

	v3 = v1-v0;
	v4 = v2-v1;
	D3DXVec3Cross(&tmp, &v3, &v4);
	D3DXVec3Normalize(&n, &tmp);
	return n;
}
