#ifndef CSTATIONBUILDMODE_H_INCLUDED
#define CSTATIONBUILDMODE_H_INCLUDED

#include "CStructBuildMode.h"

/*
 *	駅舎設置モード
 */
class CStationBuildMode: public CStructBuildMode{
protected:
public:
	CStationBuildMode(){}
	~CStationBuildMode(){}
	void EnterStructBuild();
	void Build();
	void RenderStructBuild();
};

//	外部グローバル
extern CStationBuildMode *g_StationBuildMode;

#endif
