#ifndef CNEUTRALMODE_H_INCLUDED
#define CNEUTRALMODE_H_INCLUDED

#include "CSceneryMode.h"
#include "CWindowCtrl.h"
#include "CListView.h"

/*
 *	ニュートラルモード
 */
class CNeutralMode: public CCursorSceneryMode, public CMenuCommander, public CWindowResizer{
private:
	bool m_PointMode;			//	ポイント制御モード
	CDetectInfo m_FocusInfo;	//	フォーカス情報
	CWindowCtrl m_SwitchWindow;	//	スイッチ窓
	CListView m_SwitchListView;	//	スイッチリスト
	CListView m_OptionListView;	//	オプションリスト
public:
	CNeutralMode();
	~CNeutralMode(){}
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD){ return NULL; }
	void DoubleClick(CMDTYPE, DWORD){}
	CModelInst *GetFocusInst(){ return m_FocusInfo.GetModelInst(); }
	bool CameraCtrlExp(){ return true; }
	void DeleteModelInst(CModelInst *);
	void SetFocusInfo(CDetectInfo &d){ m_FocusInfo = d; }
	void EnterCursorScenery();
	void ScanInputCursorScenery();
	void RenderCursorScenery();
	void RenderCursorSceneryFull();
	bool IsWindowDivisible(){ return true; }
};

//	外部グローバル
extern CNeutralMode *g_NeutralMode;

#endif
