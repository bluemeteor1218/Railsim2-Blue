#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CRailSelectMode.h"

/*
 *	コンストラクタ
 */
CRailSelectMode::CRailSelectMode():
	C3DPluginMode(lang(RailPlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CRailSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CRailSelectMode::GetPluginList(){
	return g_RailPluginList;
}

/*
 *	モードを有効化
 */
void CRailSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectRail);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Rail);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CRailSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CRailSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CRailSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
