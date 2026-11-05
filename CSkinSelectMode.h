#ifndef CSKINSELECTMODE_H_INCLUDED
#define CSKINSELECTMODE_H_INCLUDED

#include "CStaticCtrl.h"
#include "CCheckBox.h"
#include "CGroupBox.h"
#include "CRadioButton.h"
#include "CEditCtrl.h"
#include "CPopMenu.h"
#include "CPluginTree.h"
#include "CInterfaceMode.h"

/*
 *	スキン選択モード
 */
class CSkinSelectMode: public CPluginMode{
private:
	CWindowCtrl m_TestWindow;			//	テスト窓
	CStaticCtrl m_PropertyLabel;		//	プロパティ
	CListView m_PropertyList;			//	プロパティ
	CStaticCtrl m_TestLabel;			//	テスト
	CCheckBox m_CheckTest[2];			//	チェックボックス
	CGroupBox m_GroupTest;				//	グループボックス
	CRadioButton m_RadioTest[2];		//	ラジオボタン
	CEditCtrl m_EditTest;				//	エディットボックス
	CPushButton m_PushTest;				//	プッシュボタン
public:
	CSkinSelectMode();
	~CSkinSelectMode(){}
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *PluginDirName(){ return "Skin"; }
	CPluginList *GetPluginList();
	void EnterPlugin();
	void ScanInputPlugin();
};

//	外部グローバル
extern CSkinSelectMode *g_SkinSelectMode;

#endif
