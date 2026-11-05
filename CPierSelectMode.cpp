#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPierSelectMode.h"

/*
 *	コンストラクタ
 */
CPierSelectMode::CPierSelectMode():
	C3DPluginMode(lang(PierPlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CPierSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CPierSelectMode::GetPluginList(){
	return g_PierPluginList;
}

/*
 *	モードを有効化
 */
void CPierSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectPier);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Pier);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CPierSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CPierSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CPierSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
