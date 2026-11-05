#include "stdafx.h"
#include "HighTimer.h"
#include "Capture.h"
#include "Network.h"
#include "CModelPlugin.h"
#include "CEnvPlugin.h"
#include "CSkinPlugin.h"
#include "CConfigMode.h"
#include "CSaveFile.h"

//	関数宣言
bool ScanInputNetworkInterface();
void RenderNetworkInterface();

//	外部グローバル
extern int g_AncientNightFlag;

/*
 *	リネーム可能か
 */
bool CPluginListView::IsRenamable(
	CListElement *item	//	アイテム
){
	CTreeElement *tel = (CTreeElement *)item->GetData();
	CTreeFileElement *fe = tel->IsFile();
	if(!fe) return true;
	return fe->GetPlugin()->IsRenamable();
}

/*
 *	ドロップ可能か
 */
bool CPluginListView::IsDroppable(
	CListElement *item	//	アイテム
){
	return !!((CTreeElement *)item->GetData())->IsDirectory();
}

/*
 *	リネーム確認
 */
bool CPluginListView::ConfirmRename(
	CListElement *item,	//	アイテム
	string &newname		//	新規名
){
	CTreeElement *tel = (CTreeElement *)item->GetData();
	CTreeFileElement *fe = tel->IsFile();
	if(!fe) return true;
	return fe->ConfirmRename(newname);
}

/*
 *	リネーム終了処理
 */
void CPluginListView::EndRename(
	CListElement *item	//	アイテム
){
	CTreeElement *te = (CTreeElement *)item->GetData();
	te->SetString(item->GetString(0));
	te->GetParent()->Sort(false);
	te->GetParent()->ListDirectory();
}

/*
 *	ドロップ
 */
void CPluginListView::Drop(){
	((CTreeDirElement *)m_DropItem->GetData())->Drop();
	m_ShowDir->ListDirectory();
}

/*
 *	ダブルクリック
 */
void CPluginListView::DoubleClick(){
	CTreeElement *el = (CTreeElement *)m_FocusItem->GetData();
	el->Open();
	g_Skin->MouseUp();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CInterfaceMode::CInterfaceMode(){
	m_Camera = NULL;
}

/*
 *	モードを有効化
 */
void CInterfaceMode::EnterGame(){
	CSoundEffector::InitPlayList();
	if(m_Camera){
		m_Camera->Select();
	//	m_Camera->ResetCamera();
	//	m_Camera->ResetLight();
	}
	EnterInterface();
}

/*
 *	モードループ
 */
void CInterfaceMode::SpinGame(){
//	static double a = 0.0, s = 0.95;
//	double b = HighTimer();
	if(g_NetworkInitialized && !g_ModalDialog) g_SaveFile->Simulate(-1);
	if(m_Camera){
		m_Camera->Apply(true);
		g_AncientNightFlag = !m_Camera->IsLightOn();
		g_SystemSwitch[SYS_SW_NIGHT].SetValue(g_AncientNightFlag);
		g_SystemSwitch[SYS_SW_FRONT].SetValue(0);
		g_SystemSwitch[SYS_SW_CONNECT1].SetValue(0);
		g_SystemSwitch[SYS_SW_CONNECT2].SetValue(0);
		g_DayAlpha = g_AncientNightFlag ? 0.0f : 1.0f;
		g_NightAlpha = 1.0f-g_DayAlpha;
	}
	if(!DrawBackground()){
		devResetMatrix();
		devResetMaterial();
		devSetZRead(FALSE);
		devSetZWrite(FALSE);
		devSetLighting(FALSE);
		devTEX_POINT(0);
		devTEX_POINT(1);
		g_Skin->DrawBackground(m_Camera ? m_Camera->IsLightOn() : true);
	}
	SpinSound();
	RenderInterface();
	m_Interface.Render();
	if(m_Camera) m_Camera->PrintInfo();
	RenderFrame(0);
	RenderNetworkInterface();
	RenderDialog();
	CDragContainer::Render();
	CPopMenu::RenderAll();
	g_Cursor.ScanInput();
	g_Cursor.Render();
//	a = HighTimer()-b+s*a;
//	g_StrTex->RenderLeft(TILE_UNIT*16, TILE_UNIT+FontY(TILE_UNIT),
//		0xffff8000, 0xff000000, FlashIn("Render %.1f[ms]", (1.0-s)*a));
	EndScene();
	VideoCapture(0, NULL);
	//g_Cursor.ScanInput();
	CInterface::ProcessKey();
	if(g_ModalDialog){
		g_ModalDialog->ScanInput();
		void ProcessCommonDialog();
		ProcessCommonDialog();
		ModalFuncInterface();
	}else if(GetKey(DIK_ESCAPE)==S_PUSH){
		if(g_RSPV) Exit(); else SetNeutral();
	}else if(ScanInputFrame(0)){
	}else if(CPopMenu::ScanInputAll()){
	}else if(ScanInputNetworkInterface()){
	}else{
		ScanInputInterface();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
bool CPluginMode::ms_ShowProperty = true;
CPopMenu *CPluginMode::ms_DirMenu = NULL;
CWindowCtrl *CPluginMode::ms_LastInfoWindow = NULL;

/*
 *	メニュー初期化
 */
void CPluginMode::InitMenu(){
	ms_DirMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(CreateDir), ms_DirMenu);
	new CPopMenu(lang(DeleteDir), ms_DirMenu);
	new CPopMenu(lang(ChangeName), ms_DirMenu);
}

/*
 *	メニュー解放
 */
void CPluginMode::FreeMenu(){
	DELETE_V(ms_DirMenu);
}

/*
 *	コンストラクタ
 */
CPluginMode::CPluginMode(
	char *tname //	プラグインタイプ名
){
	if(!g_RSPV){
		m_PluginTree.Init(0, TILE_UNIT*2, g_DispWidth, g_DispHeight-TILE_UNIT*2,
			tname, &m_Interface, &m_PluginListView, this);
		m_PluginTree.GiveFocus(false);
		int ww = g_DispWidth*6/10, wh = g_DispHeight-TILE_UNIT*4;
		m_ListWindow.Init(g_DispWidth*4/10-TILE_UNIT*5, TILE_UNIT*3,
			ww, wh, lang(DirectoryList), &m_Interface, true);
		m_ListWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
		char *lvcol[3] = {lang(Name), "ID", lang(Author)};
		m_PluginListView.Init(TILE_UNIT/2, TILE_UNIT*3/2,
			ww-TILE_UNIT, wh-TILE_UNIT*2, &m_ListWindow, 3, lvcol, DRAG_PLUGIN,
			LISTVIEW_MULTISELECTABLE|LISTVIEW_RENAMABLE, this, CMD_PILVELEM);
	}
	int iww = TILE_UNIT*16, iwh = TILE_UNIT*6;
	m_InfoWindow.Init(g_DispWidth-iww-TILE_UNIT*5, TILE_UNIT*3,
		iww, iwh, lang(PluginInfo), &m_Interface, true);
	m_InfoWindow.SetResize(TILE_UNIT*4, TILE_UNIT*4, g_DispWidth, g_DispHeight, this);
	char *swcol[2] = {lang(Switch), lang(SetValue)};
	m_InfoStatic.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT,
		iww-TILE_UNIT, iwh-TILE_UNIT*2, &m_InfoWindow, FONT_HEIGHT);
	m_PluginMenu = NULL;
	class CPropertyViewer: public CMenuCommand{
	private:
		CPluginMode *m_Owner;	//	プラグインモード
	public:
		CPropertyViewer(CPluginMode *o){ m_Owner = o; }
		void Exec(){ m_Owner->ViewProperty(); }
	};
	m_PreviewMenu = new CPopMenu("", NULL);
	(new CPopMenu(lang(Properties), m_PreviewMenu))->SetCommand(new CPropertyViewer(this));
}

/*
 *	デストラクタ
 */
CPluginMode::~CPluginMode(){
	DELETE_V(m_PluginMenu);
	DELETE_V(m_PreviewMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CPluginMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_ListWindow){
		m_PluginListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}else if(wnd==&m_InfoWindow){
		m_InfoStatic.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}
}

/*
 *	メニュー発行
 */
CPopMenu *CPluginMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	if(type==CMD_PILVELEM){
		class CListDirCreator: public CMenuCommand{
		private:
			CTreeDirElement *m_TreeElement;	//	ツリー要素
		public:
			CListDirCreator(CTreeDirElement *p){ m_TreeElement = p; }
			void Exec(){
				m_TreeElement->CreateNewDir();
			}
		};
		class CListDirDeleter: public CMenuCommand{
		private:
			CTreeDirElement *m_TreeElement;	//	ツリー要素
		public:
			CListDirDeleter(CTreeDirElement *p){ m_TreeElement = p; }
			void Exec(){
				CTreeDirElement *pp = m_TreeElement->GetParent();
				m_TreeElement->Delete();
				pp->ListDirectory();
			}
		};
		class CListDirRenamer: public CMenuCommand{
		private:
			CListElement *m_ListElement;	//	リスト要素
		public:
			CListDirRenamer(CListElement *p){ m_ListElement = p; }
			void Exec(){ m_ListElement->BeginRename(); }
		};
		ms_DirMenu->GetMenu(0)->SetCommand(
			new CListDirCreator(m_PluginListView.GetShowDir()));
		CListElement *le = (CListElement *)data;
		if(!le){
			ms_DirMenu->GetMenu(1)->Enable(false);
			ms_DirMenu->GetMenu(2)->Enable(false);
			return ms_DirMenu;
		}
		CTreeElement *te = (CTreeElement *)le->GetData();
		CTreeDirElement *de = te->IsDirectory();
		if(de){
			ms_DirMenu->GetMenu(1)->SetCommand(new CListDirDeleter(de));
			ms_DirMenu->GetMenu(2)->SetCommand(new CListDirRenamer(le));
			return ms_DirMenu;
		}
	}
	return NULL;
}

/*
 *	プロパティ表示
 */
void CPluginMode::ViewProperty(){
	if(!m_ListWindow.IsVisible()){
		ms_ShowProperty = true;
		m_InfoWindow.Show(true);
		m_InfoStatic.GiveFocus(false);
	}
}

/*
 *	プラグイン検索
 */
CPlugin *CPluginMode::FindPluginBase(
	char *type,	//	タイプ
	char *id	//	識別子
){
	if(string(type)==PluginDirName()) return GetPluginList()->FindPlugin(id, false);
	return FindPlugin(type, id);
}

/*
 *	設定読込
 */
char *CPluginMode::LoadInterfaceSetting(
	char *str	//	対象文字列
){
	if(str) str = m_PluginTree.Load(str, PluginDirName(), this);
	GetPluginList()->BuildTree(&m_PluginTree);
	return LoadPluginSetting(str);
}

/*
 *	設定保存
 */
void CPluginMode::SaveInterfaceSetting(
	FILE *file	//	ファイル
){
	m_PluginTree.Save(file, PluginDirName());
}

/*
 *	モードを有効化
 */
void CPluginMode::EnterInterface(){
	if(g_RSPV){
		m_ListWindow.Show(false);
	}else{
		CTreeElement *tmp = m_PluginTree.GetFocusItem();
		if(tmp){
			CTreeDirElement *de = tmp->IsDirectory();
			if(de) de->ListDirectory();
			m_PluginTree.GiveFocus(false);
		}
	}
	CWindowCtrl *w = ms_LastInfoWindow;
	if(w && w!=&m_InfoWindow){
		m_InfoWindow.SetPos(w->GetPosX(), w->GetPosY());
		m_InfoWindow.SetSize(w->GetWidth(), w->GetHeight());
	}
	ms_LastInfoWindow = &m_InfoWindow;
	if(!ms_ShowProperty) m_InfoWindow.Show(false);
	EnterPlugin();
}

/*
 *	入力チェック
 */
void CPluginMode::ScanInputInterface(){
	if(CheckCtrl() && GetKey(DIK_S)==S_PUSH)
		g_ConfigMode->SetShadow(!g_ConfigMode->GetShadow());
	else ScanInputPlugin();
}

/*
 *	レンダリング
 */
void CPluginMode::RenderInterface(){
	if(!g_RSPV && m_ListWindow.IsVisible()) m_InfoWindow.Show(false);
	else if(CPlugin::IsPreview() && ms_ShowProperty) m_InfoWindow.Show(true);
	if(m_InfoWindow.CheckClose()){
		ms_ShowProperty = false;
		m_InfoWindow.Show(false);
	}
	RenderPlugin();
}
