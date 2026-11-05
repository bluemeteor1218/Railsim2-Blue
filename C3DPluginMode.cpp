#include "stdafx.h"
#include "HighTimer.h"
#include "CModelPlugin.h"
#include "CEnvPlugin.h"
#include "CSkinPlugin.h"
#include "CConfigMode.h"
#include "C3DPluginMode.h"

/*
 *	コンストラクタ
 */
C3DPluginMode::C3DPluginMode(
	char *tname	//	プラグインタイプ名
):
	CPluginMode(tname)	//	基本クラス
{
	new CPopMenu("-", m_PreviewMenu);
	new CPopMenu(lang(ResetCamera), m_PreviewMenu);
	new CPopMenu(lang(ResetLight), m_PreviewMenu);
	new CPopMenu("", m_PreviewMenu);
	new CPopMenu("", m_PreviewMenu);
}

/*
 *	モードを有効化
 */
void C3DPluginMode::EnterPlugin(){
	CTreeElement *tmp = m_PluginTree.GetFocusItem();
	if(!tmp) return;
	CTreeDirElement *de = tmp->IsDirectory();
	if(de) de->ListDirectory();
	m_PluginTree.GiveFocus(false);
	Enter3DPlugin();
}

/*
 *	入力チェック
 */
void C3DPluginMode::ScanInputPlugin(){
	POINT pos = g_Cursor.GetPos();
	switch(0){
	case 0:
		if(m_Interface.ScanInput()) break;
		if(m_ListWindow.CheckClose()){
			m_ListWindow.Show(false);
			return;
		}
		if(!CPlugin::IsPreview() && !g_Cursor.IsLock()) break;
		switch(m_Camera->ScanInput(0)){
		case 32:
			class CCameraResetter: public CMenuCommand{
			private:
				CCamera *m_Camera;	//	カメラ
			public:
				CCameraResetter(CCamera *c){ m_Camera = c; }
				void Exec(){ m_Camera->ResetCamera(); }
			};
			class CLightResetter: public CMenuCommand{
			private:
				CCamera *m_Camera;	//	カメラ
			public:
				CLightResetter(CCamera *c){ m_Camera = c; }
				void Exec(){ m_Camera->ResetLight(); }
			};
			class CLightLinker: public CMenuCommand{
			private:
				CCamera *m_Camera;	//	カメラ
			public:
				CLightLinker(CCamera *c){ m_Camera = c; }
				void Exec(){ m_Camera->LinkLight(!m_Camera->IsLightLinked()); }
			};
			class CLightSwitcher: public CMenuCommand{
			private:
				CCamera *m_Camera;	//	カメラ
			public:
				CLightSwitcher(CCamera *c){ m_Camera = c; }
				void Exec(){ m_Camera->SwitchLight(!m_Camera->IsLightOn()); }
			};
			(m_PreviewMenu->GetMenu(2))->SetCommand(new CCameraResetter(m_Camera));
			(m_PreviewMenu->GetMenu(3))->SetCommand(new CLightResetter(m_Camera));
			(m_PreviewMenu->GetMenu(4))->SetCommand(new CLightLinker(m_Camera));
			(m_PreviewMenu->GetMenu(5))->SetCommand(new CLightSwitcher(m_Camera));
			m_PreviewMenu->GetMenu(4)->SetString(
				m_Camera->IsLightLinked() ? lang(FixLight) : lang(InterlockLight));
			m_PreviewMenu->GetMenu(5)->SetString(
				m_Camera->IsLightOn() ? lang(LightOff) : lang(LightOn));
			m_PreviewMenu->Popup(pos.x, pos.y);
			return;
		}
		break;
	}
	ScanInput3DPlugin();
}

/*
 *	レンダリング
 */
void C3DPluginMode::RenderPlugin(){
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
	InitShadow();
	CNamedObject::InitAfterRenderList();
	CHeadlight::InitRenderList();
	g_ConfigMode->SetTexFilter();
	Render3DPlugin();
	RenderShadow();
	CNamedObject::AfterRenderAll();
	CHeadlight::RenderAll();
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	devTEX_POINT(0);
	devTEX_POINT(1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CModelPluginMode::CModelPluginMode(
	char *tname	//	プラグインタイプ名
):
	C3DPluginMode(tname)	//	基本クラス
{
	m_Camera = &m_MyCamera;
	m_PreviewModel = NULL;
}

/*
 *	スイッチウィンドウ初期化
 */
void CModelPluginMode::InitSwitchWindow(){
	int ww = TILE_UNIT*16, wh = TILE_UNIT*10;
	m_SwitchWindow.Init(g_DispWidth-ww-TILE_UNIT*7, g_DispHeight-wh-TILE_UNIT,
		ww, wh, lang(AppearanceSetting), &m_Interface, false);
	m_SwitchWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
	char *swcol[2] = {lang(Switch), lang(SetValue)};
	m_SwitchListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
		&m_SwitchWindow, 2, swcol, DRAG_NONE, 0, this, CMD_NONE);
	char *optcol[1] = {lang(Option)};
	m_OptionListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
		&m_SwitchWindow, 1, optcol, DRAG_NONE, 0, this, CMD_NONE);
	m_OptionListView.GiveFocus(false);
	WindowResized(ww, wh, &m_SwitchWindow);
}

/*
 *	ウィンドウリサイズ
 */
void CModelPluginMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_SwitchWindow){
		int h0 = h-TILE_UNIT*5/2, h1 = h0/2;
		m_SwitchListView.SetSize(w-TILE_UNIT, h1);
		m_OptionListView.SetSize(w-TILE_UNIT, h0-h1);
		m_OptionListView.SetPos(TILE_HALF, TILE_QUAD*3+TILE_UNIT+h1);
	}else{
		C3DPluginMode::WindowResized(w, h, wnd);
	}
}

/*
 *	モードを有効化
 */
void CModelPluginMode::Enter3DPlugin(){
	EnterModelPlugin();
}

/*
 *	入力チェック
 */
void CModelPluginMode::ScanInput3DPlugin(){
	CModelInst *inst = ScanInputModelPlugin();
	if(m_PreviewModel!=GetModelPlugin()){
		m_PreviewModel = GetModelPlugin();
		if(m_PreviewModel){
			m_PreviewModel->ListSwitch(&m_SwitchListView, &m_OptionListView, inst);
		}else{
			m_SwitchListView.DeleteAllItems();
			m_OptionListView.DeleteAllItems();
		}
	}
	if(m_PreviewModel && CPlugin::IsPreview()){
		m_PreviewModel->EditSwitch(&m_SwitchListView, &m_OptionListView);
		m_PreviewModel->StoreTempSwitch();
	}else if(m_PreviewModel){
		m_PreviewModel = NULL;
		m_SwitchListView.DeleteAllItems();
		m_OptionListView.DeleteAllItems();
	}
}

/*
 *	レンダリング
 */
void CModelPluginMode::Render3DPlugin(){
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	RenderModelPlugin();
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
}
