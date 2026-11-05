#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CPopMenu.h"

//	内部定数
const int SIMPLE_DIALOG_WIDTH = TILE_UNIT*12;	//	ダイアログ幅
const int SIMPLE_DIALOG_HEIGHT = 96;			//	ダイアログ高

//	外部グローバル
list<CWindowCtrl *> g_DialogQueue;

/*
 *	コンストラクタ
 */
CSimpleDialog::CSimpleDialog(
	char *label,	//	ラベル
	char *title		//	ウィンドウタイトル
){
	ResetTabHead();
	int tw = g_StrTex->DrawString(label, 0)->GetWidth()+TILE_UNIT*3, tw2;
	if(tw<(tw2 = g_StrTex->DrawString(title, 0)->GetWidth()+TILE_UNIT*2)) tw = tw2;
	if(tw<SIMPLE_DIALOG_WIDTH) tw = SIMPLE_DIALOG_WIDTH;
	Init((g_DispWidth-tw)/2, (g_DispHeight-SIMPLE_DIALOG_HEIGHT)/2,
		tw, SIMPLE_DIALOG_HEIGHT, title, NULL, false);
	m_Label.Init(TILE_UNIT, TILE_UNIT*2,
		m_Width-TILE_UNIT*2, TILE_UNIT, label, this, 1, 1);
	m_OKButton.Init((m_Width-TILE_UNIT*4)/2, TILE_UNIT*4,
		TILE_UNIT*4, TILE_UNIT, "OK", this);
	m_OKButton.GiveFocus(false);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CYesNoDialog::CYesNoDialog(
	char *label,	//	ラベル
	char *title,	//	ウィンドウタイトル
	bool def		//	デフォルト (true: yes)
){
	ResetTabHead();
	int tw = g_StrTex->DrawString(label, 0)->GetWidth()+TILE_UNIT*3, tw2;
	if(tw<(tw2 = g_StrTex->DrawString(title, 0)->GetWidth()+TILE_UNIT*2)) tw = tw2;
	if(tw<SIMPLE_DIALOG_WIDTH) tw = SIMPLE_DIALOG_WIDTH;
	Init((g_DispWidth-tw)/2, (g_DispHeight-SIMPLE_DIALOG_HEIGHT)/2,
		tw, SIMPLE_DIALOG_HEIGHT, title, NULL, false);
	m_Label.Init(TILE_UNIT, TILE_UNIT*2,
		m_Width-TILE_UNIT*2, TILE_UNIT, label, this, 1, 1);
	m_YesButton.Init(m_Width/2-TILE_UNIT*5, TILE_UNIT*4,
		TILE_UNIT*4, TILE_UNIT, FlashIn("%s (Y)", lang(Yes)), this);
	m_NoButton.Init(m_Width/2+TILE_UNIT, TILE_UNIT*4,
		TILE_UNIT*4, TILE_UNIT, FlashIn("%s (N)", lang(No)), this);
	if(def) m_YesButton.GiveFocus(false);
	else m_NoButton.GiveFocus(false);
	m_YesCommand = m_NoCommand = NULL;
}

/*
 *	デストラクタ
 */
CYesNoDialog::~CYesNoDialog(){
	DELETE_V(m_YesCommand);
	DELETE_V(m_NoCommand);
}

/*
 *	入力チェック
 */
bool CYesNoDialog::ScanInputWindow(){
	if(CheckShift() || CheckCtrl() || CheckAlt()) return false;
	if(GetKey(DIK_Y)==S_PULL) m_YesButton.SetPush(true);
	else if(GetKey(DIK_N)==S_PULL) m_NoButton.SetPush(true);
	if(m_YesButton.IsPushed() && m_YesCommand){
		m_YesCommand->Exec();
		DELETE_V(m_YesCommand);
	}else if(m_NoButton.IsPushed() && m_NoCommand){
		m_NoCommand->Exec();
		DELETE_V(m_NoCommand);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CInputDialog::CInputDialog(
	char *label,	//	ラベル
	char *title,	//	ウィンドウタイトル
	char *def,		//	デフォルト
	int maxlen		//	最大文字数
){
	ResetTabHead();
	int tw = g_StrTex->DrawString(label, 0)->GetWidth()+TILE_UNIT*3, tw2;
	if(tw<(tw2 = g_StrTex->DrawString(title, 0)->GetWidth()+TILE_UNIT*2)) tw = tw2;
	if(tw<(tw2 = maxlen*FONT_WIDTH+TILE_UNIT*3)) tw = tw2;
	if(tw<SIMPLE_DIALOG_WIDTH) tw = SIMPLE_DIALOG_WIDTH;
	Init((g_DispWidth-tw)/2, (g_DispHeight-SIMPLE_DIALOG_HEIGHT)/2,
		tw, SIMPLE_DIALOG_HEIGHT+TILE_UNIT, title, NULL, false);
	m_Label.Init(TILE_UNIT, TILE_UNIT*2,
		m_Width-TILE_UNIT*2, TILE_UNIT, label, this, 0, 0);
	m_Edit.Init(TILE_UNIT, TILE_UNIT*3, m_Width-TILE_UNIT*2, TILE_UNIT, def, this, maxlen);
	m_OKButton.Init(m_Width-TILE_UNIT*10, TILE_UNIT*5,
		TILE_UNIT*4, TILE_UNIT, "OK", this);
	m_CancelButton.Init(m_Width-TILE_UNIT*5, TILE_UNIT*5,
		TILE_UNIT*4, TILE_UNIT, lang(Cancel), this);
	m_Edit.GiveFocus(false);
}

/*
 *	入力チェック
 */
bool CInputDialog::ScanInputWindow(){
	if(m_Edit.IsFocus() && !m_Edit.IsComp()
		&& (GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH){
		m_OKButton.SetPush(true);
		return true;
	}else if(!m_Edit.IsComp() && GetKey(DIK_ESCAPE)==S_PUSH){
		m_CancelButton.SetPush(true);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CMultiInputDialog::CMultiInputDialog(
	char *title,	//	ウィンドウタイトル
	int itemnum,	//	アイテム数
	char **label,	//	ラベルリスト
	char **def,		//	デフォルトリスト
	int *maxlen		//	最大文字数リスト
){
	ResetTabHead();
	m_ItemNumber = itemnum;
	int tw = g_StrTex->DrawString(title, 0)->GetWidth()+TILE_UNIT*3, tw2;
	int i;
	for(i = 0; i<m_ItemNumber; i++){
		if(tw<(tw2 = g_StrTex->DrawString(label[i], 0)->GetWidth()+TILE_UNIT*2)) tw = tw2;
		if(tw<(tw2 = maxlen[i]*FONT_WIDTH+TILE_UNIT*3)) tw = tw2;
	}
	if(tw<SIMPLE_DIALOG_WIDTH) tw = SIMPLE_DIALOG_WIDTH;
	int item_itv = TILE_UNIT*2+TILE_HALF;
	int th = SIMPLE_DIALOG_HEIGHT+item_itv*(m_ItemNumber-1)+TILE_UNIT;
	Init((g_DispWidth-tw)/2, (g_DispHeight-th)/2, tw, th, title, NULL, false);
	m_Label = new CStaticCtrl[m_ItemNumber];
	m_Edit = new CEditCtrl[m_ItemNumber];
	for(i = 0; i<m_ItemNumber; i++){
		m_Label[i].Init(TILE_UNIT, TILE_UNIT*2+item_itv*i,
			m_Width-TILE_UNIT*2, TILE_UNIT, label[i], this, 0, 0);
		m_Edit[i].Init(TILE_UNIT, TILE_UNIT*3+item_itv*i,
			m_Width-TILE_UNIT*2, TILE_UNIT, def[i], this, maxlen[i]);
	}
	int ty = TILE_UNIT*5+item_itv*(m_ItemNumber-1);
	m_OKButton.Init(m_Width-TILE_UNIT*10, ty,
		TILE_UNIT*4, TILE_UNIT, "OK", this);
	m_CancelButton.Init(m_Width-TILE_UNIT*5, ty,
		TILE_UNIT*4, TILE_UNIT, lang(Cancel), this);
	m_Edit[0].GiveFocus(false);
}

/*
 *	デストラクタ
 */
CMultiInputDialog::~CMultiInputDialog(){
	DELETE_A(m_Label);
	DELETE_A(m_Edit);
}

/*
 *	入力チェック
 */
bool CMultiInputDialog::ScanInputWindow(){
	int i;
	bool is_comp = false;
	for(i = 0; i<m_ItemNumber; i++){
		if(m_Edit[i].IsFocus() && !m_Edit[i].IsComp()
			&& (GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH){
			m_OKButton.SetPush(true);
			return true;
		}else if(m_Edit[i].IsComp()){
			is_comp = true;
		}
	}
	if(!is_comp && GetKey(DIK_ESCAPE)==S_PUSH){
		m_CancelButton.SetPush(true);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	共通ダイアログにエンキュー
 */
void EnqueueCommonDialog(
	CWindowCtrl *dlg	//	ダイアログ
){
	g_DialogQueue.push_back(dlg);
	g_ModalDialog = *g_DialogQueue.begin();
}

/*
 *	共通ダイアログ処理
 */
void ProcessCommonDialog(){
	if(g_DialogQueue.size() && g_ModalDialog==*g_DialogQueue.begin()
		&& (*g_DialogQueue.begin())->GetWindowState()){
		DELETE_V(g_ModalDialog);
		g_DialogQueue.pop_front();
		if(g_DialogQueue.size()) g_ModalDialog = *g_DialogQueue.begin();
	}
}
