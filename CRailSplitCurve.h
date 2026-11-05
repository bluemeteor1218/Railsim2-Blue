#ifndef CRAILSPLITCURVE_H_INCLUDED
#define CRAILSPLITCURVE_H_INCLUDED

#include "CRailCurve.h"

class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CRailWay;

/*
 *	レール設置カーブ
 */
class CRailSplitCurve: public CRailCurve{
protected:
	int m_GradMode;					//	カント傾斜モード
	CRailPlugin *m_RailPlugin;		//	レールプラグイン
	CTiePlugin *m_TiePlugin;		//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;	//	橋桁プラグイン
	CRailWay *m_RailWay;			//	レールインスタンス
public:
	CRailSplitCurve(CRailPlugin *, CTiePlugin *, CGirderPlugin *, CRailWay *);
	void SetGradMode(int gm){ m_GradMode = gm; }
	float CalcCant(VEC3 &, VEC3 &, float, VEC3 &, float, float);
	void Trace(VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, float, VEC3 &, float);
	void FinishTrace(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &);
};

#endif
