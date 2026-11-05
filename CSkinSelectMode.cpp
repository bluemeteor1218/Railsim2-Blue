#include "stdafx.h"
#include "CSkinPlugin.h"
#include "CSkinSelectMode.h"

//	内部定数
const int WW = TILE_UNIT*20, WH = TILE_UNIT*17+TILE_HALF*5;	//	窓サイズ

/*
 *	コンストラクタ
 */
CSkinSelectMode::CSkinSelectMode():
	CPluginMode(lang(SkinPlugin))	//	基本クラス
{
	int i;
	m_TestWindow.Init((g_DispWidth-WW)*2/3, (g_DispHeight-WH)/2,
		WW, WH, lang(InterfaceTest), &m_Interface, true);
	m_TestWindow.SetResize(WW-TILE_UNIT*2, WH-TILE_UNIT*2, g_DispWidth, g_DispHeight, this);
	m_PropertyLabel.Init(0, 0, 1, 1, lang(PluginInfo), &m_TestWindow, 0, 1);
	char *plcol[2] = {lang(Properties), lang(Value)};
	m_PropertyList.Init(
		0, 0, WW-TILE_UNIT*2, 1, &m_TestWindow, 2, plcol, DRAG_NONE, 0);
	m_TestLabel.Init(0, 0, 1, 1, lang(Test), &m_TestWindow, 0, 1);
	for(i = 0; i<2; i++) m_CheckTest[i].Init(
		0, 0, 1, 1, FlashIn("CheckBox%d", i), &m_TestWindow);
	m_CheckTest[0].SetCheck(1);
	m_GroupTest.Init(0, 0, 1, 1, "GroupBox", &m_TestWindow);
	for(i = 0; i<2; i++) m_RadioTest[i].Init(0, 0, 1, 1, FlashIn("RadioButton%d", i),
		&m_GroupTest, i ? &m_RadioTest[i-1] : NULL);
	m_EditTest.Init(0, 0, 1, 1, "EditCtrl", &m_TestWindow, 64);
	m_PushTest.Init(0, 0, 1, 1, "PushButton", &m_TestWindow);
	WindowResized(WW, WH, &m_TestWindow);
	m_TestWindow.Show(false);
	CListElement *property;
	property = m_PropertyList.InsertItem(0, "ID");
	if(g_Skin) property->SetString(1, g_Skin->GetID());
	property = m_PropertyList.InsertItem(1, lang(Name));
	if(g_Skin) property->SetString(1, g_Skin->GetName());
	property = m_PropertyList.InsertItem(2, lang(Author));
	if(g_Skin) property->SetString(1, g_Skin->GetAuthor());
}

/*
 *	ウィンドウリサイズ
 */
void CSkinSelectMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_TestWindow){
		int i, ty = h-WH;
		m_PropertyLabel.SetPos(TILE_UNIT, TILE_UNIT*2);
		m_PropertyLabel.SetSize(w-TILE_UNIT*2, TILE_UNIT);
		m_PropertyList.SetPos(TILE_UNIT, TILE_UNIT*3+TILE_HALF);
		m_PropertyList.SetSize(w-TILE_UNIT*2, TILE_UNIT*5+ty);
		m_TestLabel.SetPos(TILE_UNIT, TILE_UNIT*9+TILE_HALF+ty);
		m_TestLabel.SetSize(w-TILE_UNIT*2, TILE_UNIT);
		for(i = 0; i<2; i++){
			m_CheckTest[i].SetPos(
				TILE_UNIT+i*((w-TILE_UNIT*3)/2+TILE_UNIT), TILE_UNIT*10+TILE_HALF*2+ty);
			m_CheckTest[i].SetSize((w-TILE_UNIT*3)/2, TILE_UNIT);
		}
		m_GroupTest.SetPos(TILE_UNIT, TILE_UNIT*11+TILE_HALF*3+ty);
		m_GroupTest.SetSize(w-TILE_UNIT*2, TILE_UNIT*3);
		for(i = 0; i<2; i++){
			m_RadioTest[i].SetPos(TILE_UNIT+i*((w-TILE_UNIT*5)/2+TILE_UNIT), TILE_UNIT+4);
			m_RadioTest[i].SetSize((w-TILE_UNIT*5)/2, TILE_UNIT);
		}
		m_EditTest.SetPos(TILE_UNIT, TILE_UNIT*14+TILE_HALF*4+ty);
		m_EditTest.SetSize(w-TILE_UNIT*2, TILE_UNIT);
		m_PushTest.SetPos(w-TILE_UNIT*9, TILE_UNIT*15+TILE_HALF*5+ty);
		m_PushTest.SetSize(TILE_UNIT*8, TILE_UNIT);
	}else{
		CPluginMode::WindowResized(w, h, wnd);
	}
}

/*
 *	メニュー発行
 */
CPopMenu *CSkinSelectMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	return CPluginMode::Dispatch(type, data);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CSkinSelectMode::GetPluginList(){
	return g_SkinPluginList;
}

/*
 *	モードを有効化
 */
void CSkinSelectMode::EnterPlugin(){
	ms_ModeLabel = lang(SelectSkin);
	if(!g_RSPV) m_PluginTree.SelectPlugin(g_Skin);
	m_TestWindow.Show(true);
	if(!*m_InfoStatic.GetText() && g_Skin) g_Skin->SetPreview();
}

/*
 *	入力チェック
 */
void CSkinSelectMode::ScanInputPlugin(){
	bool f = m_Interface.ScanInput();
	if(m_ListWindow.CheckClose()){
		m_ListWindow.Show(false);
		return;
	}
	if(m_TestWindow.CheckClose()){
		SetNeutral();
		return;
	}
	CTreeElement *el = m_PluginTree.GetPushedItem();
	if(el){
		CTreeFileElement *fe = el->IsFile();
		if(fe){
			m_TestWindow.Show(true);
			CPlugin *pi = fe->GetPlugin();
			m_PropertyList.GetElement(0)->SetString(1, pi->GetID());
			m_PropertyList.GetElement(1)->SetString(1, pi->GetName());
			m_PropertyList.GetElement(2)->SetString(1, pi->GetAuthor());
			ms_TopPanelTime = ms_RightPanelTime = MAXFPS*3;
		}else{
			m_TestWindow.Show(false);
		}
	}
	if(f || !CPlugin::IsPreview()) return;
	if(GetButton(DIM_RIGHT)==S_PUSH){
		POINT pos = g_Cursor.GetPos();
		m_PreviewMenu->Popup(pos.x, pos.y);
	}
}
