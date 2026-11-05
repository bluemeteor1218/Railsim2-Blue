#ifndef CRAILDUMPCURVE_H_INCLUDED
#define CRAILDUMPCURVE_H_INCLUDED

#include "CRailTraceCurve.h"

/*
 *	レールダンプカーブ
 */
class CRailDumpCurve: public CRailTraceCurve{
public:
	CRailDumpCurve(CRailPlugin *rpi, CTiePlugin *tpi, CGirderPlugin *gpi):
		CRailTraceCurve(rpi, tpi, gpi, NULL){}
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

/*
 *	レールレンダリングカーブ
 */
class CRailRenderCurve: public CRailTraceCurve{
public:
	CRailRenderCurve(CRailPlugin *rpi, CTiePlugin *tpi, CGirderPlugin *gpi, CRailWay *way):
		CRailTraceCurve(rpi, tpi, gpi, way){}
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

//	外部グローバル
//extern bool g_ShowRailSelect;

#endif
