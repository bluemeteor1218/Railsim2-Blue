#ifndef CDIADIALOG_H_INCLUDED
#define CDIADIALOG_H_INCLUDED

#include "CWindowCtrl.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CStaticCtrl.h"
#include "CGroupBox.h"
#include "CEditCtrl.h"
#include "CTrainListView.h"

class CRailConnector;
class CStation;
class CDiaElementBase;
class CDiaListBase;
class CDiaInstBase;
class CPointElement;
class CDiaElement;

/*
 *	ダイヤ関連ダイアログ基本クラス
 */
class CDiaDialogBase: public CWindowCtrl, public CWindowResizer, public CMenuCommander{
protected:
	int m_ExtHeight;				//	拡張領域高さ
	CStaticCtrl	m_NameLabel;		//	名称ラベル
	CEditCtrl m_NameEdit;			//	名称エディット
	CGroupListView m_GroupListView;	//	編成リスト
	CPopMenu *m_GroupMenu;			//	編成メニュー
	CCheckBox m_UseDefault;			//	デフォルト設定を使用
	CStaticCtrl m_DefaultLabel;		//	デフォルト選択時ラベル
	CListView m_RoutineListView;	//	ルーチンリスト
	CPopMenu *m_RoutineMenu;		//	ルーチンメニュー
	CPushButton m_RoutineAdd;		//	追加
	CPushButton m_RoutineDelete;	//	削除
	CPushButton m_RoutinePrev;		//	ひとつ戻す
	CPushButton m_RoutineNext;		//	ひとつ送る
	CGroupBox m_DiaGroup;			//	設定グループ
	CPushButton m_ApplyButton;		//	適用ボタン
	CDiaInstBase *m_DiaInst;		//	ダイヤインスタンス
	CDiaListBase *m_DiaList;		//	ルーチン
	CDiaElementBase *m_DiaElement;	//	編集中ダイヤ要素
public:
	CDiaDialogBase();
	~CDiaDialogBase();
	void Init(int, int, int, int, char *,  CInterface *, char *, int);
	void InitFoot();
	void WindowResized(int, int, CWindowCtrl *);
	virtual void ResizeDiaDialogBase(int, int) = 0;
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	void Enter(CDiaInstBase *);
	void SetTrainGroup(CTrainGroup *);
	void SetDiaElementBase(CDiaElementBase *);
	void ScanInputDia();
	void AddRoutine();
	void DeleteRoutine();
	void RotateRoutine(bool);
	virtual void ScanInputDiaDialogBase() = 0;
	void RenderWindow(){ RenderDiaDialogBase(); }
	virtual void RenderDiaDialogBase(){}
	virtual void SetElementValue(CDiaElementBase *) = 0;
};

#define CDIADIALOGBASE_CASTFUNC(el) \
	el *GetElement(){ return m_DiaElement ? (el *)m_DiaElement : NULL; }

/*
 *	ポイント切替ダイアログ
 */
class CPointDialog: public CDiaDialogBase{
private:
	CRadioButton m_Point[3];	//	ポイントラジオ
public:
	void Init(CInterface *);
	void ResizeDiaDialogBase(int, int);
	void ScanInputDiaDialogBase();
	void SetElementValue(CDiaElementBase *);
	CDIADIALOGBASE_CASTFUNC(CPointElement);
};

/*
 *	ダイヤ設定ダイアログ
 */
class CDiaDialog: public CDiaDialogBase{
private:
	int m_OffsetSlide;				//	オフセットスライダ状態
	CRadioButton m_Action[3];		//	動作ラジオ
	CRadioButton m_TimeType[2];		//	時間ラジオ
	CEditCtrl m_HourEdit;			//	時ラベル
	CStaticCtrl m_HourLabel;		//	時エディット
	CEditCtrl m_MinuteEdit;			//	分エディット
	CStaticCtrl m_MinuteLabel;		//	分ラベル
	CEditCtrl m_SecondEdit;			//	秒エディット
	CStaticCtrl m_SecondLabel;		//	秒ラベル
	CEditCtrl m_StopPosEdit;		//	停車位置エディット
	CPushButton m_StopPosButton;	//	停車位置反映ボタン
//	CCheckBox m_JointCheck;			//	連結チェック
public:
	void Init(CInterface *);
	void ResizeDiaDialogBase(int, int);
	void ScanInputDiaDialogBase();
	void RenderDiaDialogBase();
	void SetElementValue(CDiaElementBase *);
	CDIADIALOGBASE_CASTFUNC(CDiaElement);
};

#endif
