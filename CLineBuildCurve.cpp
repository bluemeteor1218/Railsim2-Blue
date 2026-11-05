#include "stdafx.h"
#include "CLineBuildCurve.h"
#include "CRailBuilder.h"
#include "CRailWay.h"
#include "CPier.h"
#include "CLine.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"

//	内部グローバル
vector<CPoleLink> g_LastPole;	//	最終架線柱
vector<CPoleLink> g_FinishPole;	//	最終架線柱

/*
 *	コンストラクタ
 */
CLineBuildCurve::CLineBuildCurve(
	CRailWay *way,		//	作業レール
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi,	//	橋桁プラグイン
	CPierPlugin *ipi,	//	橋脚プラグイン
	CLinePlugin *lpi,	//	架線プラグイン
	CPolePlugin *ppi	//	架線柱プラグイン
):
	CRailTraceCurve(rpi, tpi, gpi, way)	//	基本クラス
{
	m_PierPlugin = ipi;
	m_LinePlugin = lpi;
	m_PolePlugin = ppi;
}

/*
 *	トレース完了
 */
void CLineBuildCurve::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	VEC3 cpos1 = pos1, cpos2 = pos2;
	if(m_RailPlugin) m_RailPlugin->BeforeDump(cpos1, right1, up1, cpos2, right2, up2);
	if(m_PierPlugin && g_MultiTrackDummy==m_PierPlugin->IsMultiTrack()){
		float tmp = m_PierPlugin->GetPierPos();
		while(tmp<=seglen){
			float q2 = tmp/seglen, q1 = 1.0f-q2;
			VEC3 tpos = q1*cpos1+q2*cpos2, tright;
			VEC3 tup = q1*up1+q2*up2, tdir = q1*dir1+q2*dir2;
			float pickalt = tpos.y;
			V3NormAxis(&tright, &tup, &tdir);
			if(m_RailPlugin) m_RailPlugin->CalcPierPos(&tpos, &tright, &tup, &tdir);
			if(m_TiePlugin) m_TiePlugin->CalcPierPos(&tpos, &tright, &tup, &tdir);
			if(m_GirderPlugin) m_GirderPlugin->CalcPierPos(&tpos, &tright, &tup, &tdir);
			CPier *pier = new CPier(tpos, tdir, tup, pickalt, m_PierPlugin);
			if(pier->Confirm()) m_RailWay->AddPier(tmp, pier);
			tmp += m_PierPlugin->m_Interval;
		}
		m_PierPlugin->AddPierPos(seglen);
		m_RailWay->AddPierPos(seglen);
	}
	if(m_LinePlugin && g_MultiTrackDummy==
		(m_PolePlugin ? m_PolePlugin->IsMultiTrack() : false)){
		float tmp = m_LinePlugin->GetPolePos();
		int i, n, pts = m_PolePlugin ? m_PolePlugin->GetTrackNum() : 1;
		if(pts>g_LastPole.size()) pts = g_LastPole.size();
		int ct = g_MultiTrackDummy ? 0 : CRailBuilder::GetCurrentTrack();
		if(g_LastPole[ct].IsValid()){
			VEC3 lp1 = cpos1+m_LinePlugin->m_TrolleyAlt*up1+m_LinePlugin->m_Height*V3UP;
			VEC3 lp2 = cpos2+m_LinePlugin->m_TrolleyAlt*up2+m_LinePlugin->m_Height*V3UP;
			VEC3 lpos, ldir;
			if(g_MultiTrackDummy){
				if(m_PolePlugin){
					int tnum = m_PolePlugin->GetTrackNum();
					if(tnum>g_DummyTrackNum){
						float tint = m_PolePlugin->GetTrackInterval();
						float center = tint*(g_DummyTrackNum-tnum)*0.5f;
						lp1 += right1*center;
						lp2 += right2*center;
					}
				}
				lpos = ldir = V3ZERO;
				for(i = n = 0; i<pts; i++){
					if(g_LastPole[i].IsValid()){
						n++;
						lpos += g_LastPole[i].GetPos();
						ldir -= g_LastPole[i].GetOrigDir();
					}
				}
				lpos *= 1.0f/n;
				V3Norm(&ldir, &ldir);
			}else{
				lpos = g_LastPole[ct].GetPos();
				ldir = -g_LastPole[ct].GetOrigDir();
			}
			const float yev = 3.0;	//	Y axis evaluation weight
			lpos.y *= yev; ldir.y *= yev; lp1.y *= yev; lp2.y *= yev;
			V3Norm(&ldir ,&ldir);
			float dist1 = V3Len(&(lp1-lpos));
			float dist2 = V3Len(&(lp2-lpos));
			float def1 = V3Dot(&ldir, &(lp1-lpos));
			float def2 = V3Dot(&ldir, &(lp2-lpos));
			def1 = 0.3f*sqrtf(dist1*dist1-def1*def1);
			def2 = 0.3f*sqrtf(dist2*dist2-def2*def2);
			if(def2>m_LinePlugin->m_MaxDeflection){
				if(def1>m_LinePlugin->m_MaxDeflection){
					m_LinePlugin->SetPolePos(tmp = 0.0f);
				}else{
					float dlim = seglen*(m_LinePlugin->m_MaxDeflection-def1)/(def2-def1);
					if(dlim<tmp) m_LinePlugin->SetPolePos(tmp = dlim);
				}
			}
		}
		float mastersegsum = 0.0f, mastersegofs = 0.0f;
		if(g_MultiTrackDummy){
			bool before = true;
			ILPRailWay ilpr = --g_MultiTrackSegment->end();
			IPRailWay ipr = ilpr->begin();
			float deadsum = 0.0f;
			for(; ipr!=ilpr->end(); ipr++){
				mastersegsum += (*ipr)->GetSegLen();
				if(before){
					if(*ipr==m_RailWay) before = false;
					else mastersegofs += (*ipr)->GetSegLen();
				}
			}
		}
		while(tmp<=seglen){
			float q2 = tmp/seglen, q1 = 1.0f-q2;
			VEC3 tpos = q1*cpos1+q2*cpos2, tup = q1*up1+q2*up2, tdir = q1*dir1+q2*dir2;
			CPole *pole = new CPole(tpos+m_LinePlugin->m_TrolleyAlt*tup
				+m_LinePlugin->m_Height*V3UP, tdir, m_PolePlugin);
			if(g_MultiTrackDummy){
				for(i = 0; i<pts; i++){
					if(g_LastPole[i].IsValid())
						new CLine(g_LastPole[i], pole->CreateLink(0, i), m_LinePlugin);
					g_LastPole[i] = pole->CreateLink(1, i);
				}
				float masterposnorm = (mastersegofs+sumlen+tmp)/mastersegsum;
			//	if(masterposnorm>1.0f)
			//		Dialog("masterposnorm err\nmastersegofs %f\nmastersegsum %f\nsumlen %f\ntmp %f\nmasterposnorm %f",
			//		mastersegofs, mastersegsum, sumlen, tmp, masterposnorm);
				ILPRailWay ilpr = g_MultiTrackSegment->begin();
				int track = 0;
				for(; ilpr!=--g_MultiTrackSegment->end(); ilpr++, track++){
					float slavesegsum = 0.0f, slavesegofs = 0.0f;
					IPRailWay ipr = ilpr->begin();
					for(; ipr!=ilpr->end(); ipr++) slavesegsum += (*ipr)->GetSegLen();
					for(ipr = ilpr->begin(); ipr!=ilpr->end(); ipr++){
						float slaveposnorm1 = slavesegofs/slavesegsum;
						slavesegofs += (*ipr)->GetSegLen();
						float slaveposnorm2 = slavesegofs/slavesegsum;
						if(slaveposnorm1<=masterposnorm && masterposnorm<slaveposnorm2)
							(*ipr)->AddPole((*ipr)->GetSegLen()
								*(masterposnorm-slaveposnorm1)/(slaveposnorm2-slaveposnorm1),
								pole, track, true);
					}
				}
			}else{
				if(g_LastPole[ct].IsValid())
					new CLine(g_LastPole[ct], pole->CreateLink(0, 0), m_LinePlugin);
				g_LastPole[ct] = pole->CreateLink(1, 0);
			}
			m_RailWay->AddPole(tmp, pole, 0, false);
			tmp += m_LinePlugin->m_MaxInterval;
		}
		m_LinePlugin->AddPolePos(seglen);
		m_RailWay->AddPolePos(seglen);
	}
}
