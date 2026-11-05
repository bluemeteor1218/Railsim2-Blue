#ifndef CPOLESELECTMODE_H_INCLUDED
#define CPOLESELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"
#include "CRailwayMode.h"

/*
 *	架線柱選択モード
 */
class CPoleSelectMode: public C3DPluginMode, public CRailwayMode{
private:
public:
	CPoleSelectMode();
	~CPoleSelectMode(){}
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Pole"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ModalFunc3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CPoleSelectMode *g_PoleSelectMode;

#endif
