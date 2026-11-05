#ifndef CRAILSELECTMODE_H_INCLUDED
#define CRAILSELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"
#include "CRailwayMode.h"

/*
 *	レール選択モード
 */
class CRailSelectMode: public C3DPluginMode, public CRailwayMode{
private:
public:
	CRailSelectMode();
	~CRailSelectMode(){}
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Rail"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ModalFunc3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CRailSelectMode *g_RailSelectMode;

#endif
