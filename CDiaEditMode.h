#ifndef CDIAEDITMODE_H_INCLUDED
#define CDIAEDITMODE_H_INCLUDED

#include "CDiaDialog.h"
#include "CSceneryMode.h"

class CRailConnector;
class CStation;

/*
 *	ニュートラルモード
 */
class CDiaEditMode: public CCursorSceneryMode{
private:
	CRailConnector *m_EditConnector;	//	編集中のポイント
	CStation *m_EditStation;			//	編集中の駅舎
	CPointDialog m_PointDialog;			//	ポイント切替ダイアログ
	CDiaDialog m_DiaDialog;				//	ダイヤ設定ダイアログ
public:
	CDiaEditMode();
	~CDiaEditMode();
	void EnterCursorScenery();
	void ScanInputCursorScenery();
	void RenderCursorScenery();
	bool IsPausedCursorScenery(){
		return m_PointDialog.IsVisible() || m_DiaDialog.IsVisible();
	}
	//bool IsWindowDivisible(){ return true; }
};

//	外部グローバル
extern CDiaEditMode *g_DiaEditMode;

#endif
