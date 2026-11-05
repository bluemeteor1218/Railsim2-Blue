#include "stdafx.h"
#include "CStruct.h"
#include "CSaveFile.h"
#include "CStructPlugin.h"
#include "CSkinPlugin.h"
#include "CStructSelectMode.h"

/*
 *	コンストラクタ
 */
CStructSelectMode::CStructSelectMode(
	char *type	//	タイプ名
):
	CModelPluginMode(type ? type : lang(StructPlugin))	//	基本クラス
{
	m_MyCamera.Init(80.0f, 2.0f, 2000.0f, false);
	InitSwitchWindow();
	m_OptionListView.GiveFocus(false);
}

/*
 *	メニュー発行
 */
CPopMenu *CStructSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	現在のモデルプラグインを取得
 */
CModelPlugin *CStructSelectMode::GetModelPlugin(){
	return g_Struct;
}

/*
 *	プラグインリスト取得
 */
CPluginList *CStructSelectMode::GetPluginList(){
	return g_StructPluginList;
}

/*
 *	モードを有効化
 */
void CStructSelectMode::EnterModelPlugin(){
	ms_ModeLabel = lang(SelectStruct);
	m_PluginTree.SelectPlugin(g_Struct);
}

/*
 *	入力チェック
 */
CModelInst *CStructSelectMode::ScanInputModelPlugin(){
	return NULL;
}

/*
 *	レンダリング
 */
void CStructSelectMode::RenderModelPlugin(){
	CStructPlugin::RenderPreview(V3ZERO, V3DIR, V3UP);
}
