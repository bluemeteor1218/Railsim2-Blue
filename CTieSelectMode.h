#ifndef CTIESELECTMODE_H_INCLUDED
#define CTIESELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"
#include "CRailwayMode.h"

/*
 *	枕木選択モード
 */
class CTieSelectMode: public C3DPluginMode, public CRailwayMode{
private:
public:
	CTieSelectMode();
	~CTieSelectMode(){}
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Tie"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ModalFunc3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CTieSelectMode *g_TieSelectMode;

#endif
