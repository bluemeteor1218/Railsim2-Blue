//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "vertex.h"
#include "draw.h"

//	※実行前にライティングをOFFにし、行列やマテリアルをリセットすること。

/*
 *	2D直線を描画
 *
 *	x1, y1	: 始点座標
 *	x2, y2	: 終点座標
 *	c1		: 始点色
 *	c2		: 終点色
 */
void Draw2DLine(int x1, int y1, int x2, int y2, D3DCOLOR c1, D3DCOLOR c2){
	if(c2==0) c2 = c1;
	VTX_TL vt[] = {x1, y1, 0, 1, c1, x2, y2, 0, 1, c2};
	//CVertex v;
	//v.Create(vt, FVF_TL, sizeof(vt));
	//v.RenderLL();
	sv3.pDev->SetVertexShader(FVF_TL);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, vt, sizeof(VTX_TL));
}

/*
 *	3D直線を描画
 *
 *	x1, y1	: 始点座標
 *	x2, y2	: 終点座標
 *	c1		: 始点色
 *	c2		: 終点色
 */
void Draw3DLine(VEC3 p1, VEC3 p2, D3DCOLOR c1, D3DCOLOR c2){
	if(c2==0) c2 = c1;
	VTX_L vt[] = {p1.x, p1.y, p1.z, c1, p2.x, p2.y, p2.z, c2};
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderLL();
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, vt, sizeof(VTX_L));
}

/*
 *	2D矩形を描画
 *
 *	x1, y1	: 左上座標
 *	x2, y2	: 右下座標
 *	c		: 色
 */
void Draw2DRect(int x1, int y1, int x2, int y2, D3DCOLOR c){
	VTX_TL vt[] = {
		x1,		y1,		0.0f, 1.0f, c,
		x1,		y2-1,	0.0f, 1.0f, c,
		x2-1,	y2-1,	0.0f, 1.0f, c,
		x2-1,	y1,		0.0f, 1.0f, c,
		x1,		y1,		0.0f, 1.0f, c};
	//CVertex v;
	//v.Create(vt, FVF_TL, sizeof(vt));
	//v.RenderLS(4);
	sv3.pDev->SetVertexShader(FVF_TL);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, vt, sizeof(VTX_TL));
}
//	塗りつぶし
void Fill2DRect(int x1, int y1, int x2, int y2, D3DCOLOR c){
	VTX_TL vt[] = {
		x1-0.5f, y2-0.5f, 0.0f, 1.0f, c,
		x1-0.5f, y1-0.5f, 0.0f, 1.0f, c,
		x2-0.5f, y1-0.5f, 0.0f, 1.0f, c,
		x2-0.5f, y2-0.5f, 0.0f, 1.0f, c};
	//CVertex v;
	//v.Create(vt, FVF_TL, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_TL);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TL));
}
//	塗りつぶし
void Grad2DRect(int x1, int y1, int x2, int y2, D3DCOLOR *c){
#if 1
	VTX_TLX vt[] = {
		x1-0.5f, y1-0.5f, 0.0f, 1.0f, c[0], 0.0f, 0.0f,
		x2-0.5f, y1-0.5f, 0.0f, 1.0f, c[1], 0.0f, 0.0f,
		x2-0.5f, y2-0.5f, 0.0f, 1.0f, c[2], 0.0f, 0.0f,
		x1-0.5f, y2-0.5f, 0.0f, 1.0f, c[3], 0.0f, 0.0f};
	sv3.pDev->SetVertexShader(FVF_TLX);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TLX));
#else
	VTX_TL vt[] = {
		x1-0.5f, y1-0.5f, 0.0f, 1.0f, c[0],
		x2-0.5f, y1-0.5f, 0.0f, 1.0f, c[1],
		x2-0.5f, y2-0.5f, 0.0f, 1.0f, c[2],
		x1-0.5f, y2-0.5f, 0.0f, 1.0f, c[3]};
	//CVertex v;
	//v.Create(vt, FVF_TL, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_TL);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TL));
#endif
}
//	テクスチャマッピング
void TexMap2DRect(int x1, int y1, int x2, int y2, D3DCOLOR c){
	VTX_TLX vt[] = {
		x1-0.5f, y1-0.5f, 0.0f, 1.0f, c, sv3.u[0], sv3.v[0],
		x2-0.5f, y1-0.5f, 0.0f, 1.0f, c, sv3.u[1], sv3.v[0],
		x2-0.5f, y2-0.5f, 0.0f, 1.0f, c, sv3.u[1], sv3.v[1],
		x1-0.5f, y2-0.5f, 0.0f, 1.0f, c, sv3.u[0], sv3.v[1]};
	//CVertex v;
	//v.Create(vt, FVF_TLX, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_TLX);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TLX));
}
//	テクスチャマッピング (右 90°回転)
void TexMap2DRect90(int x1, int y1, int x2, int y2, D3DCOLOR c){
	VTX_TLX vt[] = {
		x1-0.5f, y1-0.5f, 0.0f, 1.0f, c, sv3.u[0], sv3.v[1],
		x2-0.5f, y1-0.5f, 0.0f, 1.0f, c, sv3.u[0], sv3.v[0],
		x2-0.5f, y2-0.5f, 0.0f, 1.0f, c, sv3.u[1], sv3.v[0],
		x1-0.5f, y2-0.5f, 0.0f, 1.0f, c, sv3.u[1], sv3.v[1]};
	//CVertex v;
	//v.Create(vt, FVF_TLX, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_TLX);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TLX));
}

/*
 *	3D矩形を描画
 *
 *	c	: 中心
 *	w	: 横幅
 *	h	: 縦幅
 *	c	: 色
 */
void Draw3DRect(VEC3 p1, VEC3 p2, VEC3 p3, VEC3 p4, D3DCOLOR c){
	VTX_L vt[] = {
		p1.x, p1.y, p1.z, c,
		p2.x, p2.y, p2.z, c,
		p3.x, p3.y, p3.z, c,
		p4.x, p4.y, p4.z, c,
		p1.x, p1.y, p1.z, c};
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderLS(4);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, vt, sizeof(VTX_L));
}
//	塗りつぶし
void Fill3DRect(VEC3 p1, VEC3 p2, VEC3 p3, VEC3 p4, D3DCOLOR c){
	VTX_L vt[] = {
		p1.x, p1.y, p1.z, c,
		p2.x, p2.y, p2.z, c,
		p3.x, p3.y, p3.z, c,
		p4.x, p4.y, p4.z, c};
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_L));
}
//	テクスチャーマッピング
void TexMap3DRect(VEC3 p1, VEC3 p2, VEC3 p3, VEC3 p4, D3DCOLOR c){
	VTX_LX vt[] = {
		p1.x, p1.y, p1.z, c, sv3.u[0], sv3.v[0],
		p2.x, p2.y, p2.z, c, sv3.u[1], sv3.v[0],
		p3.x, p3.y, p3.z, c, sv3.u[1], sv3.v[1],
		p4.x, p4.y, p4.z, c, sv3.u[0], sv3.v[1]};
	//CVertex v;
	//v.Create(vt, FVF_LX, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_LX);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_LX));
}
//	テクスチャーマッピング×2 (XY平面)
void Tex2Map3DRect(VEC3 p1, VEC3 p2, VEC3 p3, VEC3 p4, D3DCOLOR c){
	VTX_LX2 vt[] = {
		p1.x, p1.y, p1.z, c, sv3.u[0], sv3.v[0], 0.0f, 0.0f,
		p2.x, p2.y, p2.z, c, sv3.u[1], sv3.v[0], 1.0f, 0.0f,
		p3.x, p3.y, p3.z, c, sv3.u[1], sv3.v[1], 1.0f, 1.0f,
		p4.x, p4.y, p4.z, c, sv3.u[0], sv3.v[1], 0.0f, 1.0f};
	//CVertex v;
	//v.Create(vt, FVF_LX2, sizeof(vt));
	//v.RenderTF(2);
	sv3.pDev->SetVertexShader(FVF_LX2);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_LX2));
}

/*
 *	6角形を描画(XY平面)
 *
 *	pos	: 中心
 *	r	: 半径
 *	c	: 色
 */
//	塗りつぶし
void Fill3DHex(VEC3 pos, float r, D3DCOLOR cc, D3DCOLOR ca){
	VTX_L vt[6+2];
	float dr = D3DX_PI/3;
	vt[0].x = pos.x;
	vt[0].y = pos.y;
	vt[0].z = pos.z;
	vt[0].d = cc;
	for(int i = 0; i<6; i++){
		vt[i+1].x = pos.x+r*cos(dr*(6-i));
		vt[i+1].y = pos.y+r*sin(dr*(6-i));
		vt[i+1].z = pos.z;
		vt[i+1].d = ca;
	}
	vt[7] = vt[1];
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderTF(6);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 6, vt, sizeof(VTX_L));
}

/*
 *	円を描画(XY平面)
 *
 *	pos	: 中心
 *	r	: 半径
 *	c	: 色
 */
void Draw3DCircle(VEC3 pos, float r, D3DCOLOR c){
	VTX_L vt[36+1];
	float dr = D3DX_PI/18;
	for(int i = 0; i<36; i++){
		vt[i].x = pos.x+r*cos(dr*i);
		vt[i].y = pos.y+r*sin(dr*i);
		vt[i].z = pos.z;
		vt[i].d = c;
	}
	vt[36] = vt[0];
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderLS(36);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINESTRIP, 36, vt, sizeof(VTX_L));
}
//	塗りつぶし
void Fill3DCircle(VEC3 pos, float r, D3DCOLOR cc, D3DCOLOR ca){
	VTX_L vt[36+2];
	float dr = D3DX_PI/18;
	vt[0].x = pos.x;
	vt[0].y = pos.y;
	vt[0].z = pos.z;
	vt[0].d = cc;
	for(int i = 0; i<36; i++){
		vt[i+1].x = pos.x+r*cos(dr*(36-i));
		vt[i+1].y = pos.y+r*sin(dr*(36-i));
		vt[i+1].z = pos.z;
		vt[i+1].d = ca;
	}
	vt[37] = vt[1];
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderTF(36);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 36, vt, sizeof(VTX_L));
}

/*
 *	六面体の描画
 *
 *	※境界ボックス表示用(トランスフォームのリセット忘れずに)
 */
void DrawBox(BOX8 *pB, D3DCOLOR c){
	VTX_L vt[10] = {
		pB->v[2].x, pB->v[2].y, pB->v[2].z, c,
		pB->v[0].x, pB->v[0].y, pB->v[0].z, c,
		pB->v[1].x, pB->v[1].y, pB->v[1].z, c,
		pB->v[3].x, pB->v[3].y, pB->v[3].z, c,
		pB->v[2].x, pB->v[2].y, pB->v[2].z, c,
		pB->v[6].x, pB->v[6].y, pB->v[6].z, c,
		pB->v[4].x, pB->v[4].y, pB->v[4].z, c,
		pB->v[5].x, pB->v[5].y, pB->v[5].z, c,
		pB->v[7].x, pB->v[7].y, pB->v[7].z, c,
		pB->v[6].x, pB->v[6].y, pB->v[6].z, c,
	};
	//CVertex v;
	//v.Create(vt, FVF_L, sizeof(vt));
	//v.RenderLS(9);
	Draw3DLine(pB->v[0], pB->v[4], c);
	Draw3DLine(pB->v[1], pB->v[5], c);
	Draw3DLine(pB->v[3], pB->v[7], c);
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINESTRIP, 9, vt, sizeof(VTX_L));
}
