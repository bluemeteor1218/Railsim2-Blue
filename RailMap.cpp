#include "stdafx.h"
#include "RailMap.h"
#include "CVertexDump.h"
#include "CConfigMode.h"
#include "CSceneryMode.h"
#include "CSurfacePlugin.h"
#include "CScene.h"

//	内部グローバル
bool g_MapDrawNeeded;				//	地図描画が必要かどうか
float g_RailMapScale = 0.5f;		//	2D マップスケール
VEC3 g_RailMapCenter = V3ZERO;		//	2D マップ 3D 基準位置
VEC2 g_RailMapOffset;				//	2D マップ描画中心位置
VEC2 g_RailMapMove;					//	2D マップ描画移動距離
bool g_RotateMap = false;			//	2D マップ回転
VEC2 g_MapRotateRight;				//	2D マップ回転 right
VEC2 g_MapRotateDir;				//	2D マップ回転 dir
CLineDumpTL *g_LineDumpTL = NULL;	//	2D マップダンパ
CLineDumpTL *g_LineDumpTL2 = NULL;	//	2D マップダンパ (影用)

struct RailMapTextData{
	string m_Text;
	int m_PosX, m_PosY;
	int m_Width;
	D3DCOLOR m_Color;
	RailMapTextData(char *text, VEC2 pos, D3DCOLOR color){
		m_Text = text;
		m_PosX = Round(pos.x);
		m_PosY = Round(pos.y);
		m_Width = g_StrTex->DrawString(text, 0xff000000)->GetWidth();
		m_Color = color;
	}
	void Draw(){
		g_StrTex->RenderLeft(m_PosX, m_PosY, m_Color, 0xff000000, m_Text.c_str());
	}
	int Right(){ return m_PosX+m_Width; }
	int Bottom(){ return m_PosY+FONT_HEIGHT; }
	bool HitTest(RailMapTextData *rhs){
		return m_PosX<rhs->Right() && rhs->m_PosX<Right()
			&& m_PosY<rhs->Bottom() && rhs->m_PosY<Bottom();
	}
};
typedef list<RailMapTextData> RailMapTextDataList;
RailMapTextDataList g_RailMapTextData;

/*
 *	2D マップ初期化
 */
void InitRailMap(){
	if(g_ConfigMode->GetShowMap() && CSceneryMode::GetPhotoMode()!=2){
		g_MapDrawNeeded = true;
		DELETE_V(g_LineDumpTL);
		DELETE_V(g_LineDumpTL2);
		g_LineDumpTL = new CLineDumpTL(1024);
		g_LineDumpTL2 = new CLineDumpTL(1024);
		g_RailMapTextData.clear();
		if(g_RotateMap){
			g_RailMapCenter = CCamera::GetCurrentCamera()->GetFocus();
			VEC3 vdir = GetVDir();
			V2Norm(&g_MapRotateDir, &VEC2(vdir.x, -vdir.z));
			g_MapRotateRight = VEC2(-g_MapRotateDir.y, g_MapRotateDir.x);
		}else{
			g_RailMapCenter = V3ZERO;
		}
		g_RailMapOffset = VEC2(g_DispWidth, g_DispHeight)*0.5f+g_RailMapMove;
		CSurfacePlugin *surface = g_Scene->GetSurface();
		float size_x = surface->GetSizeX(), size_z = surface->GetSizeZ();
		VEC3 border1(0.5f*size_x, 0.0f, 0.5f*size_z), border2(0.5f*size_x, 0.0f, -0.5f*size_z);
		RailMapLine(border1, 0x80ffffff, border2, 0x80ffffff);
		RailMapLine(border2, 0x80ffffff, -border1, 0x80ffffff);
		RailMapLine(-border1, 0x80ffffff, -border2, 0x80ffffff);
		RailMapLine(-border2, 0x80ffffff, border1, 0x80ffffff);
	}else{
		g_MapDrawNeeded = false;
	}
}

/*
 *	2D マップ直線ダンプ
 */
void DumpMapLine(VEC2 p1, D3DCOLOR c1, VEC2 p2, D3DCOLOR c2, bool shadow){
	if(!g_MapDrawNeeded) return;
	g_LineDumpTL->Add(p1, c1, p2, c2);
	if(shadow){
	//	g_LineDumpTL2->Add(p1+VEC2(-1, -1), c1&0xff000000, p2+VEC2(-1, -1), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(0, -1), c1&0xff000000, p2+VEC2(0, -1), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(1, -1), c1&0xff000000, p2+VEC2(1, -1), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(-1, 0), c1&0xff000000, p2+VEC2(-1, 0), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(1, 0), c1&0xff000000, p2+VEC2(1, 0), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(-1, 1), c1&0xff000000, p2+VEC2(-1, 1), c2&0xff000000);
	//	g_LineDumpTL2->Add(p1+VEC2(0, 1), c1&0xff000000, p2+VEC2(0, 1), c2&0xff000000);
		g_LineDumpTL2->Add(p1+VEC2(1, 1), c1&0xff000000, p2+VEC2(1, 1), c2&0xff000000);
	}
}

/*
 *	2D マップ座標変換
 */
VEC2 TransformMapPos(VEC3 p){
	VEC3 t = (p-g_RailMapCenter)*g_RailMapScale;
	VEC2 t2(t.x, -t.z);
	if(g_RotateMap) t2 = VEC2(V2Dot(&t2, &g_MapRotateRight), -V2Dot(&t2, &g_MapRotateDir));
	return t2+g_RailMapOffset;
}

/*
 *	3D マップ直線ダンプ
 */
void RailMapLine(VEC3 p1, D3DCOLOR c1, VEC3 p2, D3DCOLOR c2, bool shadow, bool bold){
	if(!g_MapDrawNeeded) return;
	VEC2 sc1 = TransformMapPos(p1);
	VEC2 sc2 = TransformMapPos(p2);
	if(bold){
		VEC2 dir = sc2-sc1;
		V2Norm(&dir, &dir);
		VEC2 p = VEC2(-dir.y, dir.x)*0.5f;
		DumpMapLine(sc1+p, c1, sc2+p, c2, shadow);
		DumpMapLine(sc1, c1, sc2, c2, shadow);
		DumpMapLine(sc1-p, c1, sc2-p, c2, shadow);
	}else{
		DumpMapLine(sc1, c1, sc2, c2, shadow);
	}
}

void RailMapText(VEC3 pos, char *text, D3DCOLOR color){
	if(!g_MapDrawNeeded) return;
	RailMapTextData data(text, TransformMapPos(pos), color);
	int iy = data.m_PosY;
	while(true){
		bool hit = false;
		RailMapTextDataList::iterator mtl = g_RailMapTextData.begin();
		for(; mtl!=g_RailMapTextData.end(); ++mtl){
			if(mtl->HitTest(&data)){
				hit = true;
				if(iy<data.m_PosY || iy==data.m_PosY && iy>mtl->m_PosY)
					data.m_PosY = mtl->Bottom();
				else data.m_PosY = mtl->m_PosY-FONT_HEIGHT;
				break;
			}
		}
		if(!hit) break;
	}
	g_RailMapTextData.push_back(data);
	if(iy<data.m_PosY) DumpMapLine(VEC2(data.m_PosX, iy), 0xffff0000,
		VEC2(data.m_PosX, data.m_PosY), 0xffff0000, false);
	else if(data.Bottom()<iy) DumpMapLine(VEC2(data.m_PosX, iy), 0xffff0000,
		VEC2(data.m_PosX, data.Bottom()), 0xffff0000, false);
}

/*
 *	2D マップレンダリング
 */
void RenderRailMap(){
	if(!g_MapDrawNeeded) return;
	devSetLighting(FALSE);
	CCamera *camera = CCamera::GetCurrentCamera();
	float fov = camera->GetFieldOfView();
	VEC3 vpos = GetVPos();
	VEC3 vdir = GetVDir(), vright = GetVRight();
	V3Norm(&vdir, &vdir);
	V3Norm(&vright, &vright);
	float sinfov = sinf(fov*0.5f), cosfov = cos(fov*0.5f);
	const float VLLEN = 100.0f, VPCRS = 10.0f;
	VEC2 fp = TransformMapPos(camera->GetFocus());
	DumpMapLine(fp, 0xff80c0ff, fp-VEC2(VPCRS, 0), 0x0080c0ff);
	DumpMapLine(fp, 0xff80c0ff, fp-VEC2(0, VPCRS), 0x0080c0ff);
	DumpMapLine(fp, 0xff80c0ff, fp+VEC2(VPCRS, 0), 0x0080c0ff);
	DumpMapLine(fp, 0xff80c0ff, fp+VEC2(0, VPCRS), 0x0080c0ff);
	RailMapLine(vpos, 0xff80c0ff, vpos+VLLEN*(cosfov*vdir+sinfov*vright), 0x0080c0ff);
	RailMapLine(vpos, 0xff80c0ff, vpos+VLLEN*(cosfov*vdir-sinfov*vright), 0x0080c0ff);
	g_LineDumpTL2->Render(true);
	g_LineDumpTL->Render(true);
	RailMapTextDataList::iterator mtl = g_RailMapTextData.begin();
	for(; mtl!=g_RailMapTextData.end(); ++mtl) mtl->Draw();
	devSetTexture(0, NULL);
}
