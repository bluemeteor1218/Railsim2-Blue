#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CLineSelectMode.h"

/*
 *	コンストラクタ
 */
CLineSelectMode::CLineSelectMode():
	C3DPluginMode(lang(LinePlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CLineSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CLineSelectMode::GetPluginList(){
	return g_LinePluginList;
}

/*
 *	モードを有効化
 */
void CLineSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectLine);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Line);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CLineSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CLineSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CLineSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
