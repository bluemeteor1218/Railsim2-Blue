#include "stdafx.h"
#include "CExitMode.h"
#include "CSimpleDialog.h"

//	内部定数
const int WW = 256, WH = TILE_UNIT*6;	//	窓サイズ

/*
 *	コンストラクタ
 */
CExitMode::CExitMode(){
	m_Dialog = NULL;
}

/*
 *	モードを有効化
 */
void CExitMode::EnterInterface(){
	ms_ModeLabel = lang(Quit);
	g_ModalDialog = m_Dialog = new CYesNoDialog(
		lang(QuitCfmMessage), lang(QuitCfm), false);
}

/*
 *	モーダル処理
 */
void CExitMode::ModalFuncInterface(){
	if(m_Dialog->CheckYes()){
		DELETE_V(g_ModalDialog);
		m_Dialog = NULL;
		Sleep(200);
		Exit();
	}else if(m_Dialog->CheckNo()){
		DELETE_V(g_ModalDialog);
		m_Dialog = NULL;
		SetNeutral();
	}
}
