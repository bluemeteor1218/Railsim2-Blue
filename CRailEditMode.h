#ifndef CRAILEDITMODE_H_INCLUDED
#define CRAILEDITMODE_H_INCLUDED

#include "CRailWay.h"
#include "CSceneryMode.h"

const int RAIL_EDIT_MODES = 11;	//	モード数

/*
 *	レール編集モード
 */
class CRailEditMode: public CCursorSceneryMode{
private:
	int m_DragState;				//	範囲選択状態
	enum EditMode{
		EM_EDIT_RAIL,
		EM_ADD_PIER,
		EM_EDIT_PIER,
		EM_ADD_POLE,
		EM_EDIT_POLE,
		EM_EDIT_LINE,
		EM_CONNECT_LINE,
		EM_EDIT_WARP,
		EM_CONNECT_WARP,
		EM_EDIT_RAIL_BLOCK,
		EM_EDIT_SPEED_LIMIT,
	};
	EditMode m_EditMode;			//	編集モード
	CPopMenu *m_RailBlockMenu;		//	閉塞区間メニュー
	CPopMenu *m_SpeedLimitMenu;		//	制限速度メニュー
	VEC3 m_DragBegin;				//	範囲選択開始座標
	VEC3 m_DragEnd;					//	範囲選択終了座標
	CPoleLink m_LineLinkFrom;		//	架線接続元
	CRailLinkTemp m_WarpLinkFrom;	//	ワープ接続元
	CWindowCtrl m_EditWindow;		//	編集設定窓
	CStaticCtrl m_ModeLabel;		//	モードラベル
	CRadioButton m_Mode[RAIL_EDIT_MODES];	//	モードラジオ
public:
	CRailEditMode();
	~CRailEditMode();
	void EnterCursorScenery();
	void ModalFuncCursorScenery();
	void ScanInputCursorScenery();
	void RenderCursorScenery();
};

//	外部グローバル
extern CRailEditMode *g_RailEditMode;

#endif
