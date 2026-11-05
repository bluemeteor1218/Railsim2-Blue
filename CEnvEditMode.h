#ifndef CENVEDITMODE_H_INCLUDED
#define CENVEDITMODE_H_INCLUDED

#include "C3DPluginMode.h"

/*
 *	シーン編集モード
 */
class CEnvEditMode: public C3DPluginMode{
private:
	CCamera m_MyCamera;	//	カメラ
public:
	CEnvEditMode();
	~CEnvEditMode(){}
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	void DoubleClick(CMDTYPE, DWORD);
	bool DrawBackground();
	char *PluginDirName(){ return "Env"; }
	CPluginList *GetPluginList();
	void Enter3DPlugin();
	void ScanInput3DPlugin();
	void Render3DPlugin();
};

//	外部グローバル
extern CEnvEditMode *g_EnvEditMode;

#endif
