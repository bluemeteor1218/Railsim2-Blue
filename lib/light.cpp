//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "render.h"
#include "light.h"

/*
 *	ディレクショナルライトの設定
 *
 *	dir	: 方向
 *	cv	: 色
 */
void SetDirLight(VEC3 dir, D3DCOLORVALUE cv){
	ZeroMemory(&svl.dir, sizeof(D3DLIGHT8));
	svl.dir.Type = D3DLIGHT_DIRECTIONAL;
	svl.dir.Diffuse = cv;
	svl.dir.Specular = cv;
	svl.dir.Range = 1000.0f;
	D3DXVec3Normalize((VEC3 *)&svl.dir.Direction, &dir);

	sv3.pDev->SetLight(0, &svl.dir);
	sv3.pDev->LightEnable(0, TRUE);
	svl.fDir = TRUE;
}

/*
 *	ディレクショナルライトのON/OFF
 *
 *	f		: TRUE＝有効、FALSE＝無効
 */
void EnableDirLight(BOOL f){
	devSetLight(0, f);
	svl.fDir = f;
}

/*
 *	投影行列の取得
 *
 *	point		: 投影面に含まれる点
 *	normal	: 法線ベクトル
 *
 *	※ディレクショナルライトの向きが影響する。
 */
MTX4 GetShadowMtx(VEC3 point, VEC3 normal){
	MTX4 mtx;
	D3DXPLANE plane;
	VEC3 v = svl.dir.Direction;
	VEC4 light(-v.x, -v.y, -v.z, 0.0f);

	D3DXPlaneFromPointNormal(&plane, &point, &normal);
	D3DXMatrixShadow(&mtx, &light, &plane);
	return mtx;
}
