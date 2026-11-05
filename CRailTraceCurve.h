#ifndef CRAILTRACECURVE_H_INCLUDED
#define CRAILTRACECURVE_H_INCLUDED

#include "CRailCurve.h"
#include "CRailLink.h"

class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CRailWay;

/*
 *	レールトレースカーブ
 */
class CRailTraceCurve{
protected:
	static bool ms_Terminate1, ms_Terminate2;	//	終端フラグ
	static IRailSplitter ms_SpliceItr;			//	splice 位置
	CRailWay *m_RailWay;			//	作業レール
	CRailPlugin *m_RailPlugin;		//	レールプラグイン
	CTiePlugin *m_TiePlugin;		//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;	//	橋桁プラグイン
public:
	static void SetTerminate(bool t1, bool t2){ ms_Terminate1 = t1; ms_Terminate2 = t2; }
	static void SetSplitItr(IRailSplitter sit){ ms_SpliceItr = sit; }
	static void SetSplitItr(list<CRailSplitter>::reverse_iterator){}
	CRailTraceCurve::CRailTraceCurve(
		CRailPlugin *rpi, CTiePlugin *tpi, CGirderPlugin *gpi, CRailWay *way){
		m_RailPlugin = rpi; m_TiePlugin = tpi; m_GirderPlugin = gpi; m_RailWay = way;
	}
	virtual bool Confirm(VEC3 &, VEC3 &){ return true; }
	virtual void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &) = 0;
};

#endif
