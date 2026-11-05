#ifndef CINTERFACEMODE_H_INCLUDED
#define CINTERFACEMODE_H_INCLUDED

#include "CGameMode.h"
#include "CWindowCtrl.h"
#include "CMultiStatic.h"
#include "CListView.h"
#include "CPopMenu.h"
#include "CPluginTree.h"

/*
 *	プラグインリストビュー
 */
class CPluginListView: public CIconListView{
	friend class CTreeDirElement;
private:
	CTreeDirElement *m_ShowDir;	//	表示ディレクトリ
public:
	bool IsRenamable(CListElement *);
	bool IsDroppable(CListElement *);
	bool ConfirmRename(CListElement *, string &);
	void EndRename(CListElement *);
	void Drop();
	void DoubleClick();
	CTreeDirElement *GetShowDir(){ return m_ShowDir; }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	インターフェイスモード
 */
class CInterfaceMode: public CGameMode{
protected:
	CCamera *m_Camera;	//	カメラ
public:
	CInterfaceMode();
	virtual ~CInterfaceMode(){}
	virtual bool DrawBackground(){ return false; }
	char *LoadSetting(char *str){ return LoadInterfaceSetting(str); }
	virtual char *LoadInterfaceSetting(char *str){ return str; }
	void SaveSetting(FILE *file){ SaveInterfaceSetting(file); }
	virtual void SaveInterfaceSetting(FILE *){}
	void EnterGame();
	virtual void EnterInterface() = 0;
	void SpinGame();
	virtual void ModalFuncInterface(){}
	virtual void ScanInputInterface() = 0;
	virtual void RenderInterface(){}
};

/*
 *	プラグインモード
 */
class CPluginMode: public CInterfaceMode, public CWindowResizer, public CMenuCommander{
protected:
	static bool ms_ShowProperty;			//	プロパティ表示フラグ
	static CPopMenu *ms_DirMenu;			//	ディレクトリメニュー
	static CWindowCtrl *ms_LastInfoWindow;	//	最後に使用した情報窓
	CWindowCtrl m_ListWindow;				//	リスト窓
	CPluginListView m_PluginListView;		//	リスト
	CPluginTree m_PluginTree;				//	ツリー
	CWindowCtrl m_InfoWindow;				//	プラグイン情報窓
	CMultiStatic m_InfoStatic;				//	プラグイン情報表示
	CPopMenu *m_PluginMenu;					//	ツリーメニュー
	CPopMenu *m_PreviewMenu;				//	プレビューメニュー
public:
	static void InitMenu();
	static void FreeMenu();
	CPluginMode(char *);
	virtual ~CPluginMode();
	virtual void WindowResized(int, int, CWindowCtrl *);
	virtual CPopMenu *Dispatch(CMDTYPE, DWORD) = 0;
	virtual void DoubleClick(CMDTYPE, DWORD){}
	virtual bool DrawBackground(){ return false; }
	CPluginTree *GetTree(){ return &m_PluginTree; }
	void ViewProperty();
	void SetProperty(char *text){ m_InfoStatic.SetText(text); }
	virtual char *PluginDirName() = 0;
	virtual CPluginList *GetPluginList() = 0;
	CPlugin *FindPluginBase(char *, char *);
	virtual CPlugin *FindPlugin(char *, char *){ return NULL; }
	char *LoadInterfaceSetting(char *);
	virtual char *LoadPluginSetting(char *str){ return str; }
	void SaveInterfaceSetting(FILE *);
	void SetPreviewMenuPlugin();
	void EnterInterface();
	virtual void EnterPlugin() = 0;
	void ModalFuncInterface(){ ModalFuncPlugin(); }
	virtual void ModalFuncPlugin(){}
	void ScanInputInterface();
	virtual void ScanInputPlugin() = 0;
	void RenderInterface();
	virtual void RenderPlugin(){}
};

#endif
