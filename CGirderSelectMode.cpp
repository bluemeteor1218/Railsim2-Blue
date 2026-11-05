#include "stdafx.h"
#include "CRailPlugin.h"
#include "CPierPlugin.h"
#include "CGirderPlugin.h"
#include "CLinePlugin.h"
#include "CGirderSelectMode.h"

/*
 *	コンストラクタ
 */
CGirderSelectMode::CGirderSelectMode():
	C3DPluginMode(lang(GirderPlugin))	//	基本クラス
{
	m_Camera = &ms_RailwayModeCamera;
}

/*
 *	メニュー発行
 */
CPopMenu *CGirderSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CGirderSelectMode::GetPluginList(){
	return g_GirderPluginList;
}

/*
 *	モードを有効化
 */
void CGirderSelectMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectGirder);
	m_Interface.SetChild(&ms_RailWindow);
	m_PluginTree.SelectPlugin(g_Girder);
	EnterRailway(0);
}

/*
 *	モーダル処理
 */
void CGirderSelectMode::ModalFunc3DPlugin(){
	void ModalFuncRailwayPluginSet();
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CGirderSelectMode::ScanInput3DPlugin(){
	ScanInputRailway();
}

/*
 *	レンダリング
 */
void CGirderSelectMode::Render3DPlugin(){
	CRailPlugin::RenderPreview();
	CPierPlugin::RenderPreview();
	CLinePlugin::RenderPreview();
}
