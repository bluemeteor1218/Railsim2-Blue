#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CTiePlugin.h"
#include "CLinePlugin.h"
#include "CTieSelectMode.h"

/*
 *	コンストラクタ
 */
CTieSelectMode::CTieSelectMode():
	C3DPluginMode(lang(TiePlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CTieSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CTieSelectMode::GetPluginList(){
	return g_TiePluginList;
}

/*
 *	モードを有効化
 */
void CTieSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectTie);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Tie);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CTieSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CTieSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CTieSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
