#include "stdafx.h"
#include "CStation.h"
#include "CSaveFile.h"
#include "CStationPlugin.h"
#include "CSkinPlugin.h"
#include "CStationSelectMode.h"

/*
 *	コンストラクタ
 */
CStationSelectMode::CStationSelectMode():
	CStructSelectMode(lang(StationPlugin))	//	基本クラス
{
}

/*
 *	現在のモデルプラグインを取得
 */
CModelPlugin *CStationSelectMode::GetModelPlugin(){
	return g_Station;
}

/*
 *	プラグインリスト取得
 */
CPluginList *CStationSelectMode::GetPluginList(){
	return g_StationPluginList;
}

/*
 *	モードを有効化
 */
void CStationSelectMode::EnterModelPlugin(){
	ms_ModeLabel = lang(SelectStation);
	m_PluginTree.SelectPlugin(g_Station);
}

/*
 *	レンダリング
 */
void CStationSelectMode::RenderModelPlugin(){
	CStationPlugin::RenderPreview(V3ZERO, V3DIR, V3UP);
}
