#ifndef CLINEBUILDCURVE_H_INCLUDED
#define CLINEBUILDCURVE_H_INCLUDED

#include "CRailTraceCurve.h"

class CRailWay;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;

/*
 *	架線設置カーブ
 */
class CLineBuildCurve: public CRailTraceCurve{
private:
	CPierPlugin *m_PierPlugin;	//	橋脚プラグイン
	CLinePlugin *m_LinePlugin;	//	架線プラグイン
	CPolePlugin *m_PolePlugin;	//	架線柱プラグイン
public:
	CLineBuildCurve(CRailWay *, CRailPlugin *, CTiePlugin *,
		CGirderPlugin *, CPierPlugin *, CLinePlugin *, CPolePlugin *);
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

#endif
