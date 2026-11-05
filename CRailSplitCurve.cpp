#include "stdafx.h"
#include "CRailSplitCurve.h"
#include "CRailWay.h"
#include "CRailPlugin.h"

//	内部定数
const float CANT_GRAD_DIST = 5.0f;	//	カント消滅距離

/*
 *	コンストラクタ
 */
CRailSplitCurve::CRailSplitCurve(
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi,	//	橋桁プラグイン
	CRailWay *way		//	レールインスタンス
){
	m_GradMode = 0;
	m_RailPlugin = rpi;
	m_TiePlugin = tpi;
	m_GirderPlugin = gpi;
	m_RailWay = way;
}

/*
 *	カント計算
 */
float CRailSplitCurve::CalcCant(
	VEC3 &pos,		//	座標
	VEC3 &bpos1,	//	基準制御点
	float bcant1,	//	基準カント量
	VEC3 &bpos2,	//	基準制御点
	float bcant2,	//	基準カント量
	float cant		//	絶対カント量
){
	if(pos==bpos1) return bcant1;
	if(pos==bpos2) return bcant2;
	float dist1 = V3Len(&(pos-bpos1)), dist2 = V3Len(&(bpos2-pos));
	float ratio1 = 1.0f-dist1/CANT_GRAD_DIST, ratio2 = 1.0f-dist2/CANT_GRAD_DIST;
	if(ratio1<0.0f) ratio1 = 0.0f;
	if(ratio2<0.0f) ratio2 = 0.0f;
	return ratio1+ratio2>=1.0f ? bcant1*ratio1+bcant2*ratio2
		: bcant1*ratio1+bcant2*ratio2+cant*(1.0f-ratio1-ratio2);
}

/*
 *	詳細トレース
 */
void CRailSplitCurve::Trace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	VEC3 &bpos1, float bcant1,	//	始点側制御点
	VEC3 &bpos2, float bcant2	//	終点側制御点
){
	if(CalcSplit(pos1, dir1, pos2, dir2)){
		bool fcant1 = fabsf(bcant1)>0.01f, fcant2 = fabsf(bcant2)>0.01f;
		if(V3Len(&(dir2-dir1))<0.001f && m_SegLen>0.2f*CANT_GRAD_DIST && (fcant1 || fcant2)){
			switch(m_GradMode){
			case 1: {
				//	カント終了
				if(V3Len(&(up2-up1))<0.1f) goto FIN;
				VEC3 sright, sup;
				float tcant = bcant1*(1.0f-sinf(0.5f*D3DX_PI
					*V3Len(&(bpos1-m_SplitPos))/V3Len(&(bpos2-bpos1))));
				CalcCantAxis(&sright, &sup, &m_SplitDir, tcant);
				CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
				curve.SetGradMode(1);
				curve.Trace(pos1, right1, up1, dir1,
					m_SplitPos, sright, sup, m_SplitDir, bpos1, bcant1, bpos2, bcant2);
				curve.Trace(m_SplitPos, sright, sup, m_SplitDir,
					pos2, right2, up2, dir2, bpos1, bcant1, bpos2, bcant2);
				return; }
			case 2: {
				//	カント開始
				if(V3Len(&(up2-up1))<0.1f) goto FIN;
				VEC3 sright, sup;
				float tcant = bcant2*(1.0f-sinf(0.5f*D3DX_PI
					*V3Len(&(bpos2-m_SplitPos))/V3Len(&(bpos2-bpos1))));
				CalcCantAxis(&sright, &sup, &m_SplitDir, tcant);
				CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
				curve.SetGradMode(2);
				curve.Trace(pos1, right1, up1, dir1,
					m_SplitPos, sright, sup, m_SplitDir, bpos1, bcant1, bpos2, bcant2);
				curve.Trace(m_SplitPos, sright, sup, m_SplitDir,
					pos2, right2, up2, dir2, bpos1, bcant1, bpos2, bcant2);
				return; }
			}
			VEC3 rpos1 = pos1+0.5f*D3DX_PI*CANT_GRAD_DIST*dir1;
			VEC3 rpos2 = pos2-0.5f*D3DX_PI*CANT_GRAD_DIST*dir2;
			VEC3 rright, rup = V3UP;
			V3NormAxis(&rright, &rup, &m_SplitDir);
			if(fcant1 && fcant2){
				if(m_SegLen>1.03f*D3DX_PI*CANT_GRAD_DIST){
					//	両側カント
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(1);
					curve.Trace(pos1, right1, up1, dir1,
						rpos1, rright, rup, m_SplitDir, pos1, bcant1, rpos1, 0.0f);
					curve.SetGradMode(0);
					curve.Trace(rpos1, rright, rup, m_SplitDir,
						rpos2, rright, rup, m_SplitDir, rpos1, 0.0f, rpos2, 0.0f);
					curve.SetGradMode(2);
					curve.Trace(rpos2, rright, rup, m_SplitDir,
						pos2, right2, up2, dir2, rpos2, 0.0f, pos2, bcant2);
					return;
				}else if(bcant1*bcant2<0.0f){
					//	両側逆カント
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(1);
					curve.Trace(pos1, right1, up1, dir1,
						m_SplitPos, rright, rup, m_SplitDir, pos1,
						bcant1, m_SplitPos, 0.0f);
					curve.SetGradMode(2);
					curve.Trace(m_SplitPos, rright, rup, m_SplitDir,
						pos2, right2, up2, dir2, m_SplitPos, 0.0f, pos2, bcant2);
					return;
				}
			}else if(fcant1){
				if(m_SegLen>0.53f*D3DX_PI*CANT_GRAD_DIST){
					//	カント開始
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(1);
					curve.Trace(pos1, right1, up1, dir1,
						rpos1, rright, rup, m_SplitDir, pos1, bcant1, rpos1, 0.0f);
					curve.SetGradMode(0);
					curve.Trace(rpos1, rright, rup, m_SplitDir,
						pos2, right2, up2, dir2, rpos1, 0.0f, pos2, bcant2);
					return;
				}else if(V3Len(&(dir1-dir2))<0.01f){
					//	カント開始 (短)
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(1);
					curve.Trace(pos1, right1, up1, dir1,
						pos2, right2, up2, dir2, pos1, bcant1, pos2, bcant2);
					return;
				}
			}else{
				if(m_SegLen>0.53f*D3DX_PI*CANT_GRAD_DIST){
					//	カント終了
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(0);
					curve.Trace(pos1, right1, up1, dir1,
						rpos2, rright, rup, m_SplitDir, pos1, bcant1, rpos2, 0.0f);
					curve.SetGradMode(2);
					curve.Trace(rpos2, rright, rup, m_SplitDir,
						pos2, right2, up2, dir2, rpos2, 0.0f, pos2, bcant2);
					return;
				}else if(V3Len(&(dir1-dir2))<0.01f){
					//	カント終了 (短)
					CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
					curve.SetGradMode(2);
					curve.Trace(pos1, right1, up1, dir1,
						pos2, right2, up2, dir2, pos1, bcant1, pos2, bcant2);
					return;
				}
			}
		}
FIN:
		m_RailWay->AddSplitter(CRailSplitter(pos2, right2, up2, dir2));
		return;
	}
	float cant = 0.0f;
	if(m_Radius>=0.0f){
		cant = m_RailPlugin ? m_RailPlugin->CantFunc(m_Radius) : 0.0f;
		VEC3 right, up = V3UP;
		V3NormAxis(&right, &up, &dir1);
		if(V3Dot(&right, &m_SplitDir)<0.0f) cant = -cant;
	}
	cant = CalcCant(m_SplitPos, bpos1, bcant1, bpos2, bcant2, cant);
	VEC3 sright, sup;
	CalcCantAxis(&sright, &sup, &m_SplitDir, cant);
	CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, m_RailWay);
	curve.Trace(pos1, right1, up1, dir1,
		m_SplitPos, sright, sup, m_SplitDir, bpos1, bcant1, bpos2, bcant2);
	curve.Trace(m_SplitPos, sright, sup, m_SplitDir,
		pos2, right2, up2, dir2, bpos1, bcant1, bpos2, bcant2);
}
