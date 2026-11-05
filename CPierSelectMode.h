#ifndef CPIERSELECTMODE_H_INCLUDED
#define CPIERSELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"
#include "CRailwayMode.h"

/*
 *	橋脚選択モード
 */
class CPierSelectMode: public C3DPluginMode, public CRailwayMode{
private:
public:
	CPierSelectMode();
	~CPierSelectMode(){}
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Pier"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ModalFunc3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CPierSelectMode *g_PierSelectMode;

#endif
