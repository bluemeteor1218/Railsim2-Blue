#include "stdafx.h"
#include "RailMap.h"
#include "CCamera.h"
#include "CRailPlanCurve.h"
#include "CRailConnector.h"
#include "CRailWay.h"
#include "CStringTexture.h"
#include "CRailPlugin.h"

//	static メンバ
float CRailPlanCurve::ms_RadiusDrawPos;

/*
 *	曲線処理
 */
void CRailPlanCurve::Curve(
	VEC3 &pos1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &dir2,	//	終点
	bool comp2,				//	補完制御点フラグ
	bool top				//	再帰トップ
){
	if(CalcSplit(pos1, dir1, pos2, dir2)){
		m_Dump->Add(pos1, 0xc0ff00ff, pos2, 0xc0ff00ff);
		RailMapLine(pos1, 0xc0ff00ff, pos2, 0xc0ff00ff, false);
		return;
	}
	if(top && 0.0f<=m_Radius && m_Radius<10000.0f){
		float fscale = 0.003f*CCamera::GetCurrentCamera()->GetDist();
		VEC3 tup(m_SplitDir.x, 0.0f, m_SplitDir.z);
		V3Norm(&tup, &tup);
		VEC3 tdir(tup.z, 0.0f, -tup.x);
		void (CStringTexture::*func1)(
			VEC3, VEC3, VEC3, D3DCOLOR, D3DCOLOR, const char *, float);
		void (CStringTexture::*func2)(
			VEC3, VEC3, VEC3, D3DCOLOR, D3DCOLOR, const char *, float);
		VEC3 cdir = GetVDir();
		if(V3Dot(&GetVDir(), &tup)<0.0f){
			tdir = -tdir;
			tup = -tup;
			func1 = &CStringTexture::RenderRight3D;
			func2 = &CStringTexture::RenderLeft3D;
		}else{
			func1 = &CStringTexture::RenderRight3D;
			func2 = &CStringTexture::RenderLeft3D;
		}
		if(cdir.y>0.0f) tup = -tup;
		if(V3Dot(&tdir, &dir2)<0.0f) g_StrTex->RenderRight3D(
			0.5f*(pos1+pos2)+FONT_HEIGHT*fscale*ms_RadiusDrawPos*tup,
			tdir, tup, 0xffffffff, 0xff000000, FlashIn("R = %.1f", m_Radius), fscale);
		else g_StrTex->RenderLeft3D(
			0.5f*(pos1+pos2)+FONT_HEIGHT*fscale*ms_RadiusDrawPos*tup,
			tdir, tup, 0xffffffff, 0xff000000, FlashIn("R = %.1f", m_Radius), fscale);
		m_Dump->Add(pos1, 0xc0008000, pos2, 0xc0008000);
		//RailMapLine(pos1, 0xc0008000, pos2, 0xc0008000, false);
	}
	VEC3 right;
	if(m_CompSplit){
		V3Norm(&right, V3Cross(&right, &V3UP, &m_SplitDir));
		VEC3 pp1 = m_SplitPos-2.0f*right, pp2 = m_SplitPos+2.0f*right;
		m_Dump->Add(pp1, 0xc00000ff, pp2, 0xc00000ff);
		RailMapLine(pp1, 0xc00000ff, pp2, 0xc00000ff, false);
	}
	if(comp2){
		V3Norm(&right, V3Cross(&right, &V3UP, &dir2));
		VEC3 pp1 = pos2-2.0f*right, pp2 = pos2+2.0f*right;
		m_Dump->Add(pp1, 0xc00000ff, pp2, 0xc00000ff);
		RailMapLine(pp1, 0xc00000ff, pp2, 0xc00000ff, false);
	}
	CRailPlanCurve curve(m_Dump);
	curve.Curve(pos1, dir1, m_SplitPos, m_SplitDir, false, m_CompSplit);
	curve.Curve(m_SplitPos, m_SplitDir, pos2, dir2, false, m_CompSplit);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CRailBuildCurve::CRailBuildCurve(
	CRailConnectorLink &begin,	//	開始リンク
	CRailConnectorLink &end,	//	終了リンク
	CRailPlugin *rpi,			//	レールプラグイン
	CTiePlugin *tpi,			//	枕木プラグイン
	CGirderPlugin *gpi			//	橋桁プラグイン
){
	m_BeginLink = begin;
	m_EndLink = end;
	m_RailPlugin = rpi;
	m_TiePlugin = tpi;
	m_GirderPlugin = gpi;
}

/*
 *	曲線処理
 */
void CRailBuildCurve::Curve(
	VEC3 &pos1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &dir2,	//	終点
	bool comp2,				//	補完制御点フラグ
	bool top				//	再帰トップ
){
	CalcSplit(pos1, dir1, pos2, dir2);
	if(!m_BeginLink.m_Link){
		CRailConnector *con1 = new CRailConnector(pos1, dir1);
		m_BeginLink = con1->CreateLink(1, 0);
	}
	float cant = 0.0f;
	if(m_Radius>=0.0f){
		cant = m_RailPlugin ? m_RailPlugin->CantFunc(m_Radius) : 0.0f;
		VEC3 right, up = V3UP;
		V3NormAxis(&right, &up, &dir1);
		if(V3Dot(&right, &m_SplitDir)<0.0f) cant = -cant;
	}
	m_BeginLink.m_Cant = cant;
	if(m_CompSplit){
		CRailBuildCurve curve1(m_BeginLink, R2L(CRailConnectorLink()),
			m_RailPlugin, m_TiePlugin, m_GirderPlugin);
		curve1.Curve(pos1, dir1, m_SplitPos, m_SplitDir, false, true);
		CRailBuildCurve curve2(curve1.m_BeginLink, m_EndLink,
			m_RailPlugin, m_TiePlugin, m_GirderPlugin);
		curve2.Curve(m_SplitPos, m_SplitDir, pos2, dir2, false, true);
		m_BeginLink = curve2.m_BeginLink;
		return;
	}
	if(m_EndLink.m_Link){
		m_EndLink.m_Cant = cant;
		CRailWay *way = new CRailWay(m_BeginLink, m_EndLink,
			m_RailPlugin, m_TiePlugin, m_GirderPlugin);
		g_SingleTrackSegment->push_back(way);
		m_BeginLink.Connect(R2L(way->CreateLink(0)));
		m_EndLink.Connect(R2L(way->CreateLink(1)));
	}else{
		CRailConnector *con2 = new CRailConnector(pos2, dir2);
		CRailConnectorLink nextlink = con2->CreateLink(0, 0);
		nextlink.m_Cant = cant;
		CRailWay *way = new CRailWay(m_BeginLink, nextlink,
			m_RailPlugin, m_TiePlugin, m_GirderPlugin);
		g_SingleTrackSegment->push_back(way);
		m_BeginLink.Connect(R2L(way->CreateLink(0)));
		nextlink.Connect(R2L(way->CreateLink(1)));
		m_BeginLink = con2->CreateLink(1, 0);
	}
}
