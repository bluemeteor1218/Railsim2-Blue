#ifndef C3DPLUGINMODE_H_INCLUDED
#define C3DPLUGINMODE_H_INCLUDED

#include "CInterfaceMode.h"

class CModelPlugin;

/*
 *	レンダリングプラグインモード
 */
class C3DPluginMode: public CPluginMode{
protected:
public:
	C3DPluginMode(char *);
	virtual ~C3DPluginMode(){}
	virtual void WindowResized(int w, int h, CWindowCtrl * wnd){
		CPluginMode::WindowResized(w, h, wnd);
	}
	virtual CPopMenu *Dispatch(CMDTYPE, DWORD) = 0;
	virtual bool DrawBackground(){ return false; }
	virtual char *PluginDirName() = 0;
	virtual CPluginList *GetPluginList() = 0;
	CPlugin *FindPlugin(char *type, char *id){ return Find3DPlugin(type, id); }
	virtual CPlugin *Find3DPlugin(char *, char *){ return NULL; }
	char *LoadPluginSetting(char *str){ return Load3DPluginSetting(str); }
	virtual char *Load3DPluginSetting(char *str){ return str; }
	void EnterPlugin();
	virtual void Enter3DPlugin() = 0;
	void ModalFuncPlugin(){ ModalFunc3DPlugin(); }
	virtual void ModalFunc3DPlugin(){}
	void ScanInputPlugin();
	virtual void ScanInput3DPlugin() = 0;
	void RenderPlugin();
	virtual void Render3DPlugin() = 0;
};

/*
 *	モデルプラグインモード
 */
class CModelPluginMode: public C3DPluginMode{
protected:
	CCamera m_MyCamera;				//	カメラ
	CWindowCtrl m_SwitchWindow;		//	スイッチ窓
	CListView m_SwitchListView;		//	スイッチリスト
	CListView m_OptionListView;		//	オプションリスト
	CModelPlugin *m_PreviewModel;	//	表示中プラグイン
public:
	CModelPluginMode(char *);
	virtual ~CModelPluginMode(){}
	void InitSwitchWindow();
	virtual void WindowResized(int, int, CWindowCtrl *);
	virtual CPopMenu *Dispatch(CMDTYPE, DWORD) = 0;
	virtual CModelPlugin *GetModelPlugin() = 0;
	virtual char *PluginDirName() = 0;
	virtual CPluginList *GetPluginList() = 0;
	CPlugin *Find3DPlugin(char *type, char *id){ return FindModelPlugin(type, id); }
	virtual CPlugin *FindModelPlugin(char *, char *){ return NULL; }
	char *Load3DPluginSetting(char *str){ return LoadModelPluginSetting(str); }
	virtual char *LoadModelPluginSetting(char *str){ return str; }
	void Enter3DPlugin();
	virtual void EnterModelPlugin() = 0;
	void ModalFunc3DPlugin(){ ModalFuncModelPlugin(); }
	virtual void ModalFuncModelPlugin(){}
	void ScanInput3DPlugin();
	virtual CModelInst *ScanInputModelPlugin() = 0;
	void Render3DPlugin();
	virtual void RenderModelPlugin() = 0;
};

#endif
