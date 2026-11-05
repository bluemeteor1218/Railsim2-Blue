#include "stdafx.h"
#include "CCamera.h"
#include "CShadowVolume.h"
#include "CScene.h"
#include "CEnvPlugin.h"
#include "CSurfacePlugin.h"
#include "CSceneryMode.h"
#include "CConfigMode.h"

//	外部定数
extern const float CLIP_PLANE_NEAR;

//	内部定数
const int GRID_SIZE = 32;		//	グリッドサイズ
const float GRID_ZOOM = 2.0f;	//	グリッド拡大率
const float TANGENT_LEN = 4.0f;	//	接線表示長さ

//	外部グローバル
extern bool g_StencilEnabled;

//	内部グローバル
bool g_ShadowNeeded;				//	影が必要かどうか
CVertex g_DetailGridVertex;			//	グリッド頂点バッファ
CShadowVolume g_ShadowVolume;		//	シャドウボリューム

/*
 *	直線と 1 点の距離を求める
 *
 *	戻り値: 距離
 */
float LinePointDistance(
	VEC3 *p1,	//	直線が通る 1 点
	VEC3 *n1,	//	直線の方向ベクトル
	VEC3 *p2	//	点
){
	return V3Len(&(*p2-(*p1+*n1*V3Dot(n1, &(*p2-*p1)))));
}

/*
 *	行列正規化
 */
void NormalizeMatrix(MTX4 *mtx){
	V3Norm((VEC3 *)&mtx->_11, (VEC3 *)&mtx->_11);
	V3Norm((VEC3 *)&mtx->_21, (VEC3 *)&mtx->_21);
	V3Norm((VEC3 *)&mtx->_31, (VEC3 *)&mtx->_31);
}

/*
 *	2 直線の最近点を求める
 *
 *	戻り値: 平行なら true を返す
 */
bool LineLineNearest(
	VEC3 *m1,	//	直線 1 上の最近点
	VEC3 *m2,	//	直線 2 上の最近点
	VEC3 *p1,	//	直線 1 を通る 1 点
	VEC3 *n1,	//	直線 1 の方向ベクトル
	VEC3 *p2,	//	直線 2 を通る 1 点
	VEC3 *n2	//	直線 2 の方向ベクトル
){
	VEC3 r = *p2-*p1;
	float d = V3Len(&r);
	float n12 = V3Dot(n1, n2);
	float t = 1.0f-n12*n12;
	if(t<0.000001f) return true;
	*m1 = *p1+*n1*(V3Dot(n1, &(r-*n2*V3Dot(n2, &r)))/t);
	*m2 = *p2+*n2*(V3Dot(n2, &(*n1*V3Dot(n1, &r)-r))/t);
	return false;
}

/*
 *	2 直線の距離を求める
 *
 *	戻り値: 距離
 */
float LineLineDistance(
	VEC3 *p1,	//	直線 1 を通る 1 点
	VEC3 *n1,	//	直線 1 の方向ベクトル
	VEC3 *p2,	//	直線 2 を通る 1 点
	VEC3 *n2	//	直線 2 の方向ベクトル
){
	VEC3 m1, m2;
	return LineLineNearest(&m1, &m2, p1, n1, p2, n2)
		? V3Len(&(m1-m2)) : LinePointDistance(p1, n1, p2);
}

/*
 *	ある点から一定距離にある直線上の点 (2 個) を求める
 */
void LinePointPosition(
	VEC3 *d1,	//	結果 1
	VEC3 *d2,	//	結果 2
	VEC3 *p1,	//	直線が通る 1 点
	VEC3 *n1,	//	直線の方向ベクトル
	VEC3 *p2,	//	点
	float x		//	距離
){
	float r, beta, gamma, tx = p1->x-p2->x, ty = p1->y-p2->y, tz = p1->z-p2->z;
	beta = n1->x*tx+n1->y*ty+n1->z*tz;
	gamma = tx*tx+ty*ty+tz*tz-x*x;
	r = sqrt(beta*beta-4*gamma);
	*d1 = *p1+*n1*(-beta+r);
	*d2 = *p1+*n1*(-beta-r);
}

/*
 *	グリッド作成
 */
void InitGrid(){
	int i, cnt = 0;
	VTX_L grid[(GRID_SIZE-1)*16];	//	両端・両側・±・XZ・
	for(i = -GRID_SIZE/2+1; i<GRID_SIZE/2; i++){
		D3DCOLOR color = i%5 ? 0x00c0c0ff : 0x000000ff;
		D3DCOLOR alpha = (255*(GRID_SIZE/2-1-abs(i))/(GRID_SIZE/2-1))<<24;
		grid[cnt].x = i; grid[cnt].y = 0.0f; grid[cnt].z = -GRID_SIZE/2;
		grid[cnt].d = color; cnt++;
		grid[cnt].x = i; grid[cnt].y = 0.0f; grid[cnt].z = 0.0f;
		grid[cnt].d = color|alpha; cnt++;
		grid[cnt].x = i; grid[cnt].y = 0.0f; grid[cnt].z = GRID_SIZE/2;
		grid[cnt].d = color; cnt++;
		grid[cnt].x = i; grid[cnt].y = 0.0f; grid[cnt].z = 0.0f;
		grid[cnt].d = color|alpha; cnt++;
		grid[cnt].x = -GRID_SIZE/2; grid[cnt].y = 0.0f; grid[cnt].z = i;
		grid[cnt].d = color; cnt++;
		grid[cnt].x = 0.0f; grid[cnt].y = 0.0f; grid[cnt].z = i;
		grid[cnt].d = color|alpha; cnt++;
		grid[cnt].x = GRID_SIZE/2; grid[cnt].y = 0.0f; grid[cnt].z = i;
		grid[cnt].d = color; cnt++;
		grid[cnt].x = 0.0f; grid[cnt].y = 0.0f; grid[cnt].z = i;
		grid[cnt].d = color|alpha; cnt++;
	}
	g_DetailGridVertex.Create(grid, FVF_L, sizeof(VTX_L)*(GRID_SIZE-1)*8);
}

/*
 *	グリッド描画
 */
void DrawGrid(
	VEC3 pos	//	中心座標
){
	int i;
	float cdist = CCamera::GetCurrentCamera()->GetDist(), gscale;
	cdist *= g_FovRatio;
	if(cdist>GRID_ZOOM*2000.0f) gscale = 100.0f;
	else if(cdist>GRID_ZOOM*1000.0f) gscale = 50.0f;
	else if(cdist>GRID_ZOOM*400.0f) gscale = 20.0f;
	else if(cdist>GRID_ZOOM*200.0f) gscale = 10.0f;
	else if(cdist>GRID_ZOOM*100.0f) gscale = 5.0f;
	else if(cdist>GRID_ZOOM*40.0f) gscale = 2.0f;
	else if(cdist>GRID_ZOOM*20.0f) gscale = 1.0f;
	else if(cdist>GRID_ZOOM*10.0f) gscale = 0.5f;
	else if(cdist>GRID_ZOOM*4.0f) gscale = 0.2f;
	else gscale = 0.1f;
	float fscale = 0.003f*cdist/gscale, gstep = 5.0f*gscale;
	MTX4 move;
	VEC3 tpos(Round(pos.x/gstep)*gstep, pos.y, Round(pos.z/gstep)*gstep);
	D3DXMatrixTranslation(&move, tpos.x, tpos.y, tpos.z);
	move._11 *= gscale; move._22 *= gscale; move._33 *= gscale;
	pos = (pos-tpos)/gscale;
	devSetTexture(0, NULL);
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	devTransform(&move);
	devResetMaterial();
	devTEX_POINT(0);
	devTEX_POINT(1);
	g_DetailGridVertex.RenderLL();
	Draw3DLine(pos, VEC3(pos.x, 0.0f, GRID_SIZE), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, VEC3(pos.x, 0.0f, -GRID_SIZE), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, VEC3(GRID_SIZE, 0.0f, pos.z), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, VEC3(-GRID_SIZE, 0.0f, pos.z), 0xffff8000, 0x00ff8000);
	int step = 100.0f<=cdist/gscale && cdist/gscale<200.0f ? 2 : 1;
	int lim = 20000.0f<=cdist ? 0 : 2;
	VEC3 vdir = GetVDir();
	VEC3 xofs = 0.5f*GRID_SIZE*V3RIGHT, zofs = 0.5f*GRID_SIZE*V3DIR;
	float xs = vdir.x<0.0f ? -1.0f : 1.0f, zs = vdir.z<0.0f ? -1.0f : 1.0f;
	float uinv = vdir.y<0.0f ? 1.0f : -1.0f;
	for(i = -lim; i<=lim; i += step){
		char *fmt = (char *)(gscale<1.0f ? "%.1f" : "%.0f");
		D3DCOLOR col = ((255*(3-abs(i))/3)<<24)|0x00ffffff;
		if(vdir.x<0.0f) g_StrTex->RenderRight3D(-zofs+5.0f*i*V3RIGHT, -V3DIR*xs, uinv*xs*V3RIGHT,
			col, 0xff000000, FlashIn(fmt, tpos.x+i*gstep), fscale);
		else g_StrTex->RenderLeft3D(-zofs+5.0f*i*V3RIGHT, -V3DIR*xs, uinv*xs*V3RIGHT,
			col, 0xff000000, FlashIn(fmt, tpos.x+i*gstep), fscale);
		if(vdir.z<0.0f) g_StrTex->RenderLeft3D(-xofs+5.0f*i*V3DIR, V3RIGHT*zs, uinv*zs*V3DIR,
			col, 0xff000000, FlashIn(fmt, tpos.z+i*gstep), fscale);
		else g_StrTex->RenderRight3D(-xofs+5.0f*i*V3DIR, V3RIGHT*zs, uinv*zs*V3DIR,
			col, 0xff000000, FlashIn(fmt, tpos.z+i*gstep), fscale);
	}
	float midfix = 0.5f*FONT_HEIGHT*fscale;
	g_StrTex->RenderCenter3D(zofs+midfix*V3DIR, V3RIGHT*zs, uinv*zs*V3DIR,
		0xffffffff, 0xff000000, "Z", fscale);
	g_StrTex->RenderCenter3D(xofs+midfix*V3RIGHT, -V3DIR*xs, uinv*xs*V3RIGHT,
		0xffffffff, 0xff000000, "X", fscale);
}

/*
 *	XZ 接線描画
 */
void DrawTangent(
	VEC3 pos,			//	中心座標
	VEC3 norm,			//	法線
	D3DCOLOR col,		//	色
	CLineDumpL *dump	//	ダンパ
){
	VEC3 xn(norm.y, -norm.x, 0.0f), zn(0.0f, -norm.z, norm.y);
	D3DCOLOR col2 = col&0x00ffffff;
	V3Norm(&xn, &xn);
	V3Norm(&zn, &zn);
	xn *= TANGENT_LEN; zn *= TANGENT_LEN;
	if(dump){
		dump->Add(pos, col, pos+xn, col2);
		dump->Add(pos, col, pos-xn, col2);
		dump->Add(pos, col, pos+zn, col2);
		dump->Add(pos, col, pos-zn, col2);
	}else{
		Draw3DLine(pos, pos+xn, col, col2);
		Draw3DLine(pos, pos-xn, col, col2);
		Draw3DLine(pos, pos+zn, col, col2);
		Draw3DLine(pos, pos-zn, col, col2);
	}
}

/*
 *	フォーカス描画
 */
void DrawFocus(
	VEC3 pos	//	中心座標
){
	devResetMatrix();
	devSetTexture(0, NULL);
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	devResetMaterial();
	Draw3DLine(pos, pos+VEC3(0.0f, 0.0f, TANGENT_LEN), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, pos+VEC3(0.0f, 0.0f, -TANGENT_LEN), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, pos+VEC3(TANGENT_LEN, 0.0f, 0.0f), 0xffff8000, 0x00ff8000);
	Draw3DLine(pos, pos+VEC3(-TANGENT_LEN, 0.0f, 0.0f), 0xffff8000, 0x00ff8000);
}

/*
 *	影付き 3D 直線を描画
 */
void Draw3DLineWithShadow(
	VEC3 pos1, VEC3 pos2,		//	座標
	D3DCOLOR c1, D3DCOLOR c2	//	色
){
	VEC3 p1 = WorldToScreen(pos1), p2 = WorldToScreen(pos2);
	if(p1.z<0.0f && p2.z<0.0f) return;
	if(p1.z<0.0f || p2.z<0.0f){
		VEC3 vp = GetVPos(), vd = GetVDir();
		float q1 = V3Dot(&(vp-pos1), &vd)+CLIP_PLANE_NEAR/g_FovRatio;
		float q2 = V3Dot(&(pos2-vp), &vd);
		if(p1.z<0.0f) p1 = WorldToScreen((q2*pos1+q1*pos2)/(q1+q2));
		else p2 = WorldToScreen((q2*pos1+q1*pos2)/(q1+q2));
	}
	int p1x = Round(p1.x), p1y = Round(p1.y);
	int p2x = Round(p2.x), p2y = Round(p2.y);
	D3DCOLOR s2 = (c2 ? c2 : c1)&0xff000000;
	Draw2DLine(p1x+1, p1y+1, p2x+1, p2y+1, c1&0xff000000, s2 ? s2 : 0x01000000);
	Draw2DLine(p1x, p1y, p2x, p2y, c1, c2);
}

/*
 *	3D 点を 2D 矩形で描画
 */
void Draw3DPointAs2DRect(
	VEC3 pos,		//	座標
	D3DCOLOR color,	//	色
	int r			//	半径
){
	VEC3 p = WorldToScreen(pos);
	if(p.z<0.0f) return;
	int px = Round(p.x), py = Round(p.y);
	Draw2DRect(px-r+1, py-r+1, px+r+1, py+r+1, 0xff000000);
	Draw2DRect(px-r, py-r, px+r, py+r, color);
}

/*
 *	影を初期化
 */
void InitShadow(){
	CEnvPlugin *env = g_RSPV ? g_Env : g_Scene->GetEnv();
	if(g_ConfigMode->GetShadow() && env->GetShadowColor()
		&& !g_SystemSwitch[SYS_SW_NIGHT].GetValue() && g_StencilEnabled){
		g_ShadowNeeded = true;
		g_ShadowVolume.Reset();
	}else{
		g_ShadowNeeded = false;
	}
}

/*
 *	影を落とす
 */
void CastShadow(
	CObject *obj	//	オブジェクト
){
	if(!g_ShadowNeeded || g_RenderBlink) return;
	g_ShadowVolume.BuildFromMesh(obj, VEC3(svl.dir.Direction));
}

/*
 *	影のレンダリング
 */
void RenderShadow(){
	if(!g_ShadowNeeded) return;
	CEnvPlugin *env = g_RSPV ? g_Env : g_Scene->GetEnv();
	devSetTexture(0, NULL);
	g_ShadowVolume.Render();
	g_ShadowVolume.Draw(ScaleColor(env->GetShadowColor(), g_DayAlpha));
}
