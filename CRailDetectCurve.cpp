#include "stdafx.h"
#include "CScene.h"
#include "CRailDetectCurve.h"
#include "CRailPlugin.h"
#include "CSkinPlugin.h"

//	内部定数
const float DETECT_3D_MAX = 4.0f;	//	3D 検出最大距離

//	外部グローバル
extern float g_FovRatio;

//	内部グローバル
bool g_SelectMultiTrackDummy = true;

//	static メンバ
float CRailDetectCurve2D::ms_MinDist;
CRailLinkTemp CRailDetectCurve2D::ms_Detect;

/*
 *	[static]
 *	接続点を表示
 */
bool CRailDetectCurve2D::RenderLink(){
	if(ms_MinDist<0.0f || ms_Detect.m_Link->GetScene()!=g_Scene) return false;
	devSetLighting(TRUE);
	g_LinkObject.Render();
	ms_Detect.m_Link->ShowSegment();
	return true;
}

/*
 *	コンストラクタ
 */
CRailDetectCurve2D::CRailDetectCurve2D(
	CRailWay *way,		//	作業レール
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi,	//	橋桁プラグイン
	int mode,			//	検出モード
	VEC3 &cursor1,		//	カーソル座標 1
	VEC3 &cursor2		//	カーソル座標 2
):
	CRailTraceCurve(rpi, tpi, gpi, way)	//	基本クラス
{
	m_DetectMode = mode;
	m_CursorPos1 = cursor1;
	m_CursorPos2 = cursor2;
}

/*
 *	トレース完了
 */
void CRailDetectCurve2D::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	int flag;
	bool select_master = g_SelectMultiTrackDummy || !m_RailWay->IsMultiTrackDummy();
	switch(m_DetectMode){
	case 5:
		flag = Round(sumlen);
		sumlen = seglen = 0.0f;
	case 1: {	//	scan rail point link
		VEC3 sp1 = WorldToScreen(pos1+0.25f*seglen*dir1);
		VEC3 sp2 = WorldToScreen(pos2-0.25f*seglen*dir2);
		float dist[2] = {sp1.z<0.0f ? 1000.0f : V3Len(&(sp1-m_CursorPos1)),
			sp2.z<0.0f ? 1000.0f : V3Len(&(sp2-m_CursorPos1))};
		if(m_DetectMode==5){
			if(!(flag&1)) dist[0] = 1000.0f;
			if(!(flag&2)) dist[1] = 1000.0f;
			seglen = m_RailWay->GetSegLen();
		}
		VEC3 tpos[2] = {pos1, pos2}, tright[2] = {right1, right2};
		VEC3 tup[2] = {up1, up2}, tdir[2] = {-dir1, dir2};
		int i = dist[0]<dist[1] ? 0 : 1;
		if(!m_RailWay->IsLinkable(i, tpos[i])){
			i = !i;
			if(!m_RailWay->IsLinkable(i, tpos[i])) i = -1;
		}
		if(i>=0 && dist[i]<DETECT_2D_MAX && (ms_MinDist<0.0f || dist[i]<ms_MinDist) && select_master){
			ms_MinDist = dist[i];
			ms_Detect = CRailLinkTemp(
				i, i ? m_RailWay->GetSegLen()-sumlen-seglen : sumlen,
				tpos[i], tright[i], tup[i], tdir[i], m_RailWay, ms_SpliceItr);
			if(m_RailPlugin) m_RailPlugin->BeforeDump(
				tpos[0], tright[0], tup[0], tpos[1], tright[1], tup[1]);
			g_LinkObject.SetPos(tpos[i]);
			g_LinkObject.SetDir(tdir[i], tup[i]);
		}
		splitter.m_Selected &= 2;
		break; }
	case 2: {	//	scan rectangle selection
		VEC3 spc = WorldToScreen(0.5f*(pos1+pos2));
		if(spc.z>=0.0f && m_CursorPos1.x<=spc.x && spc.x<=m_CursorPos2.x
			&& m_CursorPos1.y<=spc.y && spc.y<=m_CursorPos2.y && select_master)
			splitter.m_Selected |= 1; else splitter.m_Selected &= 2;
		break; }
	case 3:	//	finish rectangle selection
		if(CheckCtrl()){
			splitter.m_Selected &= ~splitter.m_Selected<<1;
		}else{
			if(CheckShift()) splitter.m_Selected |= splitter.m_Selected<<1;
			else splitter.m_Selected = splitter.m_Selected<<1;
		}
		splitter.m_Selected &= 2;
		break;
	case 4: {	//	nearest point selection
		VEC3 center = 0.5f*(pos1+pos2), spc = WorldToScreen(center);
		float dist = V3Len(&(spc-m_CursorPos1));
		float th = 250.0f*seglen/(V3Len(&(GetVPos()-center))*g_FovRatio);
		if(th<DETECT_2D_MIN) th = DETECT_2D_MIN;
		if(spc.z>=0.0f && dist<th && (ms_MinDist<0.0f || dist<ms_MinDist) && select_master){
			ms_MinDist = dist;
			ms_Detect = CRailLinkTemp(
				1, 0.0f, pos1, right1, up1, dir1, m_RailWay, ms_SpliceItr);
		}
		splitter.m_Selected &= 2;
		break; }
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
float CRailDetectCurve3D::ms_MinDist;
CRailLinkTemp CRailDetectCurve3D::ms_Detect;

/*
 *	[static]
 *	接続点を表示
 */
bool CRailDetectCurve3D::RenderLink(){
	if(ms_MinDist<0.0f) return false;
	devSetLighting(TRUE);
	g_LinkObject.Render();
	ms_Detect.m_Link->ShowSegment();
	return true;
}

/*
 *	コンストラクタ
 */
CRailDetectCurve3D::CRailDetectCurve3D(
	CRailWay *way,		//	作業レール
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi,	//	橋桁プラグイン
	VEC3 &arrow			//	矢印座標
):
	CRailTraceCurve(rpi, tpi, gpi, way)	//	基本クラス
{
	m_ArrowPos = arrow;
}

/*
 *	トレース完了
 */
void CRailDetectCurve3D::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	float dist[2] = {V3Len(&(pos1+0.1f*dir1-m_ArrowPos)),
		V3Len(&(pos2-0.1f*dir2-m_ArrowPos))};
	VEC3 tpos[2] = {pos1, pos2}, tright[2] = {right1, right2};
	VEC3 tup[2] = {up1, up2}, tdir[2] = {-dir1, dir2};
	int i = dist[0]<dist[1] ? 0 : 1;
	if(!m_RailWay->IsLinkable(i, tpos[i])){
		i = !i;
		if(!m_RailWay->IsLinkable(i, tpos[i])) i = -1;
	}
	if(i>=0 && dist[i]<DETECT_3D_MAX && (ms_MinDist<0.0f || dist[i]<ms_MinDist)){
		ms_MinDist = dist[i];
		ms_Detect = CRailLinkTemp(
			i, 0.0f, tpos[i], tright[i], tup[i], tdir[i], m_RailWay, ms_SpliceItr);
		if(m_RailPlugin) m_RailPlugin->BeforeDump(
			tpos[0], tright[0], tup[0], tpos[1], tright[1], tup[1]);
		g_LinkObject.SetPos(tpos[i]);
		g_LinkObject.SetDir(tdir[i], tup[i]);
		return;
	}
}
