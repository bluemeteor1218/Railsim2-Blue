#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CPoleSelectMode.h"

/*
 *	コンストラクタ
 */
CPoleSelectMode::CPoleSelectMode():
	C3DPluginMode(lang(PolePlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CPoleSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CPoleSelectMode::GetPluginList(){
	return g_PolePluginList;
}

/*
 *	モードを有効化
 */
void CPoleSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectPole);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Pole);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CPoleSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CPoleSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CPoleSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
