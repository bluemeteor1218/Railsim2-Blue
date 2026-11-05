#ifndef CSTATIONSELECTMODE_H_INCLUDED
#define CSTATIONSELECTMODE_H_INCLUDED

#include "CStructSelectMode.h"

/*
 *	駅舎選択モード
 */
class CStationSelectMode: public CStructSelectMode{
private:
public:
	CStationSelectMode();
	~CStationSelectMode(){}
	CModelPlugin *GetModelPlugin();
	char *PluginDirName(){ return "Station"; }
	CPluginList *GetPluginList();
	void EnterModelPlugin();
	void RenderModelPlugin();
};

//	外部グローバル
extern CStationSelectMode *g_StationSelectMode;

#endif
