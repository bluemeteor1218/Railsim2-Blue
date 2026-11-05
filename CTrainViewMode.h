#ifndef CTRAINVIEWMODE_H_INCLUDED
#define CTRAINVIEWMODE_H_INCLUDED

#include "CTrainListView.h"
#include "CSceneryMode.h"

/*
 *	ニュートラルモード
 */
class CTrainViewMode: public CCursorSceneryMode, public CWindowResizer, public CMenuCommander{
private:
	int m_ViewMode;					//	視点モード
	CObject m_GroupLocal;			//	編成ローカル系
	CWindowCtrl m_GroupWindow;		//	編成窓
	CGroupListView m_GroupListView;	//	編成リスト
	CPopMenu *m_GroupMenu;			//	編成メニュー
public:
	CTrainViewMode();
	~CTrainViewMode();
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	bool CameraCtrlExp(){ return true; }
	bool CameraCtrlLock(){ return !!m_ViewMode; }
	void EnterCursorScenery();
	void ApplyCamera();
	void ScanInputCursorScenery();
	void RenderCursorScenery();
};

//	外部グローバル
extern CTrainViewMode *g_TrainViewMode;

#endif
