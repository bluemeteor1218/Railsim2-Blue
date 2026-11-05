#ifndef CGIRDERSELECTMODE_H_INCLUDED
#define CGIRDERSELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"
#include "CRailwayMode.h"

/*
 *	橋桁選択モード
 */
class CGirderSelectMode: public C3DPluginMode, public CRailwayMode{
private:
public:
	CGirderSelectMode();
	~CGirderSelectMode(){}
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Girder"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ModalFunc3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CGirderSelectMode *g_GirderSelectMode;

#endif
