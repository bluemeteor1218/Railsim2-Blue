#ifndef CRAILPLANCURVE_H_INCLUDED
#define CRAILPLANCURVE_H_INCLUDED

#include "CRailLink.h"
#include "CRailCurve.h"

class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;

/*
 *	レール計画カーブ
 */
class CRailPlanCurve: public CRailCurve{
private:
	static float ms_RadiusDrawPos;	//	半径表示位置
	CLineDumpL *m_Dump;	//	ダンパ
public:
	static void SetRadiusDrawPos(float p){ ms_RadiusDrawPos = p; }
	CRailPlanCurve(CLineDumpL *dump){ m_Dump = dump; }
	void Curve(VEC3 &, VEC3 &, VEC3 &, VEC3 &, bool, bool);
};

/*
 *	レール設置カーブ
 */
class CRailBuildCurve: public CRailCurve{
private:
	CRailConnectorLink m_BeginLink;	//	開始リンク
	CRailConnectorLink m_EndLink;	//	終了リンク
	CRailPlugin *m_RailPlugin;		//	レールプラグイン
	CTiePlugin *m_TiePlugin;		//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;	//	橋桁プラグイン
public:
	CRailBuildCurve(CRailConnectorLink &, CRailConnectorLink &,
		CRailPlugin *, CTiePlugin *, CGirderPlugin *);
	CRailConnectorLink &GetNext(){ return m_BeginLink; }
	void Curve(VEC3 &, VEC3 &, VEC3 &, VEC3 &, bool, bool);
};

#endif
