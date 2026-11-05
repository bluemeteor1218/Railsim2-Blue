#ifndef CSTRUCTSELECTMODE_H_INCLUDED
#define CSTRUCTSELECTMODE_H_INCLUDED

#include "C3DPluginMode.h"

class CStruct;
class CStructPlugin;

/*
 *	施設選択モード
 */
class CStructSelectMode: public CModelPluginMode{
private:
public:
	CStructSelectMode(char *str = NULL);
	~CStructSelectMode(){}
	virtual CPopMenu *Dispatch(CMDTYPE, DWORD);
	virtual CModelPlugin *GetModelPlugin();
	virtual char *PluginDirName(){ return "Struct"; }
	virtual CPluginList *GetPluginList();
	virtual void EnterModelPlugin();
	virtual CModelInst *ScanInputModelPlugin();
	virtual void RenderModelPlugin();
};

//	外部グローバル
extern CStructSelectMode *g_StructSelectMode;

#endif
