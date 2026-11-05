#include "stdafx.h"
#include "RailMap.h"
#include "CRailDetectCurve.h"
#include "CRailDumpCurve.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CCamera.h"

//	外部グローバル
extern bool g_ShowRailSelect;
extern bool g_ShowRailWaySelect;
extern bool g_ShowRailBlockSelect;
extern bool g_ShowSpeedLimitSelect;
extern bool g_MultiTrackDummy;
extern int g_DummyTrackNum;
extern float g_DummyTrackInterval;
extern VEC3 g_RailMapCenter;
extern VEC2 g_RailMapOffset;

//	内部グローバル
// diffuse, ambient, specular, emissive
MAT8 g_MatSelect[5] = {												//	選択マテリアル (不透明)
	{{0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {1,1,1,1}, 1.0f},				//	非選択
	{{0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {1,0,0,1}, 1.0f},				//	ハイライト
	{{0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,1,0,1}, 1.0f},				//	選択
	{{0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {1,1,0,1}, 1.0f},				//	選択+ハイライト
	{{0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {1,0.5f,0,1}, 1.0f}};			//	薄いハイライト
MAT8 g_MatSelectA[5] = {											//	選択マテリアル (半透明)
	{{0,0,0,0.75f}, {0,0,0,1}, {0,0,0,1}, {1,1,1,1}, 1.0f},			//	非選択
	{{0,0,0,0.75f}, {0,0,0,1}, {0,0,0,1}, {1,0,0,1}, 1.0f},			//	ハイライト
	{{0,0,0,0.75f}, {0,0,0,1}, {0,0,0,1}, {0,1,0,1}, 1.0f},			//	選択
	{{0,0,0,0.75f}, {0,0,0,1}, {0,0,0,1}, {1,1,0,1}, 1.0f},			//	選択+ハイライト
	{{0,0,0,0.75f}, {0,0,0,1}, {0,0,0,1}, {1,0.5f,0,1}, 1.0f}};		//	薄いハイライト
D3DCOLOR g_ColorSelect[5] = {
	0xffffffff, 0xffff0000, 0xff00ff00, 0xffffff00, 0xff800000};	//	D3DCOLOR 版

/*
 *	トレース完了
 */
void CRailDumpCurve::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	VEC3 tpos1 = pos1, ipos1 = pos1, tpos2 = pos2, ipos2 = pos2;
	VEC3 icright1, icup1 = V3UP, icright2, icup2 = V3UP;
	V3NormAxis(&icright1, &icup1, &dir1);
	V3NormAxis(&icright2, &icup2, &dir2);
	if(m_RailPlugin){
		if(g_MultiTrackDummy){
			m_RailPlugin->BeforeDump(
				tpos1, right1, R2L(VEC3(up1)),
				tpos2, right2, R2L(VEC3(up2)));
			m_RailPlugin->AfterDump(
				tpos1, R2L(VEC3(up1)), ipos1, icup1,
				tpos2, R2L(VEC3(up2)), ipos2, icup2);
		}else{
			m_RailPlugin->Dump(
				tpos1, right1, R2L(VEC3(up1)), ipos1, icright1, icup1,
				tpos2, right2, R2L(VEC3(up2)), ipos2, icright2, icup2, seglen, 4);
		}
	}
	if(m_TiePlugin){
		if(g_MultiTrackDummy){
			m_TiePlugin->AfterDump(
				tpos1, R2L(VEC3(up1)), ipos1, icup1,
				tpos2, R2L(VEC3(up2)), ipos2, icup2);
		}else{
			m_TiePlugin->Dump(
				tpos1, right1, R2L(VEC3(up1)), ipos1, icright1, icup1,
				tpos2, right2, R2L(VEC3(up2)), ipos2, icright2, icup2, seglen, 4);
		}
	}
	if(m_GirderPlugin && g_MultiTrackDummy==m_GirderPlugin->IsMultiTrack())
		m_GirderPlugin->Dump(
			tpos1, right1, R2L(VEC3(up1)), ipos1, icright1, icup1,
			tpos2, right2, R2L(VEC3(up2)), ipos2, icright2, icup2, seglen, 4);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	トレース完了
 */
void CRailRenderCurve::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	if(!g_MultiTrackDummy) RailMapLine(pos1, 0xffffffff, pos2, 0xffffffff);
	VEC3 tpos1 = pos1, ipos1 = pos1, tpos2 = pos2, ipos2 = pos2;
	VEC3 icright1, icup1 = V3UP, icright2, icup2 = V3UP;
	V3NormAxis(&icright1, &icup1, &dir1);
	V3NormAxis(&icright2, &icup2, &dir2);
	int terminate = (ms_Terminate1 ? 1 : 0)|(ms_Terminate2 ? 2 : 0);
	int tmp_selected = splitter.m_Selected;
	if(!tmp_selected && CRailDetectCurve2D::IsDetected() &&
		(g_ShowRailWaySelect && CRailDetectCurve2D::GetDetect().m_Link==m_RailWay
		|| g_ShowRailBlockSelect && m_RailWay->IsRailBlock()
			&& m_RailWay->GetRailBlock()==CRailDetectCurve2D::GetDetect().m_Link->GetRailBlock()
		|| g_ShowSpeedLimitSelect && m_RailWay->IsSpeedLimit()
			&& m_RailWay->GetSpeedLimit()==CRailDetectCurve2D::GetDetect().m_Link->GetSpeedLimit())){
		tmp_selected = 4;
	}
	bool use_altmat = g_ShowRailSelect && tmp_selected;
	MAT8 *altmat = use_altmat ? &g_MatSelect[tmp_selected] : NULL;
	MAT8 *altmat2 = use_altmat ? &g_MatSelectA[tmp_selected] : NULL;
	int render_mode;
	if(m_RailPlugin){
		if(g_MultiTrackDummy){
			m_RailPlugin->BeforeDump(
				tpos1, right1, R2L(VEC3(up1)),
				tpos2, right2, R2L(VEC3(up2)));
			m_RailPlugin->AfterDump(
				tpos1, R2L(VEC3(up1)), ipos1, icup1,
				tpos2, R2L(VEC3(up2)), ipos2, icup2);
		}else{
			if(use_altmat || m_RailWay->GetParent()){
				devSetTexture(0, NULL);
				if(use_altmat){
					devSetMaterial(altmat2);
					render_mode = 1;
				}else{
					render_mode = 7;
				}
				devResetMatrix();
				m_RailPlugin->Dump(
					R2L(VEC3(tpos1)), R2L(VEC3(right1)), R2L(VEC3(up1)),
					R2L(VEC3(ipos1)), R2L(VEC3(icright1)), R2L(VEC3(icup1)),
					R2L(VEC3(tpos2)), R2L(VEC3(right2)), R2L(VEC3(up2)),
					R2L(VEC3(ipos2)), R2L(VEC3(icright2)), R2L(VEC3(icup2)), seglen, render_mode);
			}
			m_RailPlugin->Render(
				tpos1, right1, R2L(VEC3(up1)), dir1, ipos1, icright1, icup1,
				tpos2, right2, R2L(VEC3(up2)), dir2, ipos2, icright2, icup2,
				terminate, seglen, altmat);
		}
	}
	if(m_TiePlugin){
		if(g_MultiTrackDummy){
			m_TiePlugin->AfterDump(
				tpos1, R2L(VEC3(up1)), ipos1, icup1,
				tpos2, R2L(VEC3(up2)), ipos2, icup2);
		}else{
			if(use_altmat || m_RailWay->GetParent()){
				devSetTexture(0, NULL);
				if(use_altmat){
					devSetMaterial(altmat2);
					render_mode = 1;
				}else{
					render_mode = 7;
				}
				devResetMatrix();
				m_TiePlugin->Dump(
					R2L(VEC3(tpos1)), R2L(VEC3(right1)), R2L(VEC3(up1)),
					R2L(VEC3(ipos1)), R2L(VEC3(icright1)), R2L(VEC3(icup1)),
					R2L(VEC3(tpos2)), R2L(VEC3(right2)), R2L(VEC3(up2)),
					R2L(VEC3(ipos2)), R2L(VEC3(icright2)), R2L(VEC3(icup2)), seglen, render_mode);
			}
			m_TiePlugin->Render(
				tpos1, right1, R2L(VEC3(up1)), dir1, ipos1, icright1, icup1,
				tpos2, right2, R2L(VEC3(up2)), dir2, ipos2, icright2, icup2,
				terminate, seglen, altmat);
		}
	}
	if(g_MultiTrackDummy){
		right1.y = right2.y = 0.0f;
		V3Norm(&up1, V3Cross(&up1, &dir1, V3Norm(&right1, &right1)));
		V3Norm(&up2, V3Cross(&up2, &dir2, V3Norm(&right2, &right2)));
	}
	if(m_GirderPlugin && g_MultiTrackDummy==m_GirderPlugin->IsMultiTrack()){
		if(use_altmat || m_RailWay->GetParent()){
			devSetTexture(0, NULL);
			if(use_altmat){
				devSetMaterial(altmat2);
				render_mode = 1;
			}else{
				render_mode = 7;
			}
			devResetMatrix();
			m_GirderPlugin->Dump(
				R2L(VEC3(tpos1)), R2L(VEC3(right1)), R2L(VEC3(up1)),
				R2L(VEC3(ipos1)), R2L(VEC3(icright1)), R2L(VEC3(icup1)),
				R2L(VEC3(tpos2)), R2L(VEC3(right2)), R2L(VEC3(up2)),
				R2L(VEC3(ipos2)), R2L(VEC3(icright2)), R2L(VEC3(icup2)), seglen, render_mode);
		}
		m_GirderPlugin->Render(
			tpos1, right1, R2L(VEC3(up1)), dir1, ipos1, icright1, icup1,
			tpos2, right2, R2L(VEC3(up2)), dir2, ipos2, icright2, icup2, terminate, seglen, altmat);
	}else if(g_MultiTrackDummy){
		if(g_ShowRailSelect && tmp_selected){
			devSetTexture(0, NULL);
			devSetMaterial(altmat);
			devResetMatrix();
			devSetZRead(FALSE);
			int i;
			for(i = 0; i<g_DummyTrackNum; i++){
				VEC3 p11 = tpos1+right1*((i-0.5f*g_DummyTrackNum)*g_DummyTrackInterval);
				VEC3 p12 = tpos1+right1*((i+1-0.5f*g_DummyTrackNum)*g_DummyTrackInterval);
				VEC3 p21 = tpos2+right2*((i-0.5f*g_DummyTrackNum)*g_DummyTrackInterval);
				VEC3 p22 = tpos2+right2*((i+1-0.5f*g_DummyTrackNum)*g_DummyTrackInterval);
				if(!i) Draw3DLine(p11, p21, 0xffffffff, 0xffffffff);
				Draw3DLine(p11, p22, 0xffffffff, 0xffffffff);
				Draw3DLine(p12, p21, 0xffffffff, 0xffffffff);
				Draw3DLine(p12, p22, 0xffffffff, 0xffffffff);
				Draw3DLine(p11, p12, 0xffffffff, 0xffffffff);
				Draw3DLine(p21, p22, 0xffffffff, 0xffffffff);
			}
			devSetZRead(TRUE);
		}
	}
}
