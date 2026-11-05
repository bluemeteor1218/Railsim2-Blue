#ifndef CTRAINSETMODE_H_INCLUDED
#define CTRAINSETMODE_H_INCLUDED

#include "CTrainListView.h"
#include "CSceneryMode.h"

/*
 *	ニュートラルモード
 */
class CTrainSetMode: public CCursorSceneryMode, public CWindowResizer, public CMenuCommander{
private:
	CWindowCtrl m_GroupWindow;		//	編成窓
	CGroupListView m_GroupListView;	//	編成リスト
	CPopMenu *m_GroupMenu;			//	編成メニュー
public:
	CTrainSetMode();
	~CTrainSetMode();
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	void EnterCursorScenery();
	void ScanInputCursorScenery();
	void RenderCursorScenery();
	//bool IsWindowDivisible(){ return true; }
};

//	外部グローバル
extern CTrainSetMode *g_TrainSetMode;

#endif
