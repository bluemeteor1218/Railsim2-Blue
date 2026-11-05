//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "graphic.h"
#include "view.h"

//	外部グローバル
extern int g_DispWidth;
extern int g_DispHeight;

/*
 *	X 軸周りの回転
 *
 *	v	: 回転角（rad）
 *
 *	※回転の方向＝軸の＋方向に左手親指を向けた時の人差し指の方向
 */
void RotVX(float v){
	VMTX_ROT(D3DXMatrixRotationX, -v);
}

/*
 *	Y 軸周りの回転
 *
 *	v	: 回転角（rad）
 */
void RotVY(float v){
	VMTX_ROT(D3DXMatrixRotationY, -v);
}

/*
 *	Ｚ軸周りの回転
 *
 *	v	: 回転角（rad）
 */
void RotVZ(float v){
	VMTX_ROT(D3DXMatrixRotationZ, -v);
}

/*
 *	Upベクトル周りの回転
 *
 *	v	: 回転角（rad）
 */
void RotVUp(float v){
	VMTX_ROTA(D3DXVECTOR3(VMTX_21, VMTX_22, VMTX_23), v);
}

/*
 *	移動
 *
 *	v	: 移動ベクトル
 */
void MoveV(VEC3 v){
	VMTX_MOVE(D3DXMatrixTranslation, -v);
}

/*
 *	移動（ワールド座標系）
 *
 *	v	: 移動ベクトル
 */
void MoveVW(VEC3 v){
	VMTX_MOVEW(D3DXMatrixTranslation, -v);
}

/*
 *	位置設定
 *
 *	v	: 位置ベクトル
 */
void SetVPos(VEC3 v){
	VMTX_MOVEW(D3DXMatrixTranslation, GetVPos()-v);
}

/*
 *	位置、姿勢の設定
 *
 *	pos：位置
 *	dir：向き
 *	up ：上方向ベクトル
 */
void SetView(VEC3 pos, VEC3 dir, VEC3 up){
	D3DXMatrixLookAtLH(&sv3.mtxView, &pos, &dir, &up);
}

/*
 *	ビューポート設定
 */
void SetViewport(DWORD x, DWORD y, DWORD w, DWORD h, float znear, float zfar){
	D3DVIEWPORT8 vp;
	vp.X = x;
	vp.Y = y;
	vp.Width = w;
	vp.Height = h;
	vp.MinZ = znear;
	vp.MaxZ = zfar;
	sv3.pDev->SetViewport(&vp);

	//ビューポート行列の作成(ワールド→スクリーン座標変換用)
	sv3.mtxVPort = MTX4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	sv3.mtxVPort._11 =  (w*0.5f);
	sv3.mtxVPort._22 = -(h*0.5f);
	sv3.mtxVPort._41 =  (w*0.5f)+x;
	sv3.mtxVPort._42 =  (h*0.5f)+y;
}

/*
 *	向きの設定
 *
 *	at	：注視する点
 */
void LookAtV(VEC3 at){
	VEC3 up(0, 1, 0);
	VEC3 pos = GetVPos();
	D3DXMatrixLookAtLH(&sv3.mtxView, &pos, &at, &up);
}

/*
 *	ワールド座標からスクリーン座標への変換
 *
 *	※カメラより後ろにある場合はZ値が-1
 */
VEC3 WorldToScreen(VEC3 pos){
	VEC3 out;

	D3DXVec3TransformCoord(&out, &pos, &sv3.mtxWtoS);
	if(D3DXVec3Dot(&(pos-GetVPos()), &GetVDir())>=0){
		out.z = 0.0f;
	}else{
		out.z = -1.0f;
		out.x = g_DispWidth-out.x;
		out.y = g_DispHeight-out.y;
	}
	return out;
}

/*
 *	スクリーン座標からワールド座標への変換
 *
 *	※pos.zは0で渡すこと
 */
VEC3 ScreenToWorld(VEC3 pos){
	VEC3 v3;
	D3DXVec3TransformCoord(&v3, &pos, &sv3.mtxStoW);
	return VEC3(v3.x, v3.y, v3.z);
}
