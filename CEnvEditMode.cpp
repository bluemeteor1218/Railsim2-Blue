#include "stdafx.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CEnvPlugin.h"
#include "CSkinPlugin.h"
#include "CEnvEditMode.h"
#include "CConfigMode.h"

/*
 *	コンストラクタ
 */
CEnvEditMode::CEnvEditMode():
	C3DPluginMode(lang(EnvPlugin))	//	基本クラス
{
	m_MyCamera.Init(2000.0f, 2000.0f, 2000.0f, false);
	m_Camera = &m_MyCamera;
}

/*
 *	ウィンドウリサイズ
 */
void CEnvEditMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	C3DPluginMode::WindowResized(w, h, wnd);
}

/*
 *	メニュー発行
 */
CPopMenu *CEnvEditMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	ダブルクリック処理
 */
void CEnvEditMode::DoubleClick(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
}

/*
 *	背景を描画
 */
bool CEnvEditMode::DrawBackground(){
	if(!g_Env) return NULL;
	g_ConfigMode->SetTexFilter();
	g_Env->Render((int)(365/4)+(m_MyCamera.IsLightOn() ? 0.5 : 0.0f));
	return true;
}

/*
 *	プラグインリスト取得
 */
CPluginList *CEnvEditMode::GetPluginList(){
	return g_EnvPluginList;
}

/*
 *	モードを有効化
 */
void CEnvEditMode::Enter3DPlugin(){
	ms_ModeLabel = lang(SelectEnv);
	m_PluginTree.SelectPlugin(g_Env = g_Scene->GetEnv());
}

/*
 *	入力チェック
 */
void CEnvEditMode::ScanInput3DPlugin(){
}

/*
 *	レンダリング
 */
void CEnvEditMode::Render3DPlugin(){
	if(CPlugin::IsPreview()){
		if(g_Env){
			if(g_Scene) g_Scene->SetEnv(g_Env);
			g_Env->RenderAfter();
		}
		RenderCompass();
	}
}
