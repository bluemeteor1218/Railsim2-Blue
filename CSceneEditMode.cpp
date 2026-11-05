#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CSurfacePlugin.h"
#include "CSkinPlugin.h"
#include "CSceneEditMode.h"
#include "CConfigMode.h"

/*
 *	リネーム終了処理
 */
void CSceneListView::EndRename(
	CListElement *item	//	アイテム
){
	CScene *sc = (CScene *)item->GetData();
	sc->SetName(item->GetString(0));
}

/*
 *	ダブルクリック
 */
void CSceneListView::DoubleClick(){
	((CScene *)m_FocusItem->GetData())->Enter(false);
	g_Skin->MouseUp();
	CGameMode::SetNeutral();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CSceneEditMode::CSceneEditMode():
	CModelPluginMode(lang(SurfacePlugin))	//	基本クラス
{
	m_SelectScene = NULL;
	int ww = TILE_UNIT*16, wh = TILE_UNIT*10;
	m_MyCamera.Init(2000.0f, 100.0f, 5000.0f, false);
	m_Camera = &m_MyCamera;
	if(!g_RSPV) m_AddButton.Init(0, 0, TILE_UNIT*4, TILE_UNIT, lang(Add), &m_InfoWindow);
	m_InfoWindow.SetSize(m_InfoWindow.GetWidth(), m_InfoWindow.GetHeight());
	if(!g_RSPV){
		m_SceneWindow.Init(g_DispWidth-ww-TILE_UNIT*5, g_DispHeight-wh-TILE_UNIT*3,
			ww, wh, lang(SceneList), &m_Interface, true);
		m_SceneWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
		char *lvcol[2] = {lang(Name), lang(Surface)};
		m_SceneListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
			&m_SceneWindow, 2, lvcol, DRAG_INSERT, LISTVIEW_RENAMABLE|LISTVIEW_INSERTABLE, this, CMD_SCENE);
	}
	InitSwitchWindow();
	m_OptionListView.GiveFocus(false);
	m_PluginMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(AddScene), m_PluginMenu);
	m_SceneMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(SelectScene), m_SceneMenu);
	new CPopMenu(lang(DeleteScene), m_SceneMenu);
	new CPopMenu(lang(ChangeName), m_SceneMenu);
}

/*
 *	デストラクタ
 */
CSceneEditMode::~CSceneEditMode(){
	DELETE_V(m_SceneMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CSceneEditMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_SceneWindow){
		m_SceneListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}else if(wnd==&m_InfoWindow){
		m_AddButton.SetPos(w-TILE_UNIT*4-TILE_HALF, h+TILE_QUAD);
		CModelPluginMode::WindowResized(w, h, wnd);
	}else{
		CModelPluginMode::WindowResized(w, h, wnd);
	}
}

/*
 *	メニュー発行
 */
CPopMenu *CSceneEditMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_PITVELEM:
		class CSceneAdder: public CMenuCommand{
		private:
			CSurfacePlugin *m_SurfacePlugin;	//	シーン
		public:
			CSceneAdder(CSurfacePlugin *sp){ m_SurfacePlugin = sp; }
			void Exec(){ g_SceneEditMode->AddScene(m_SurfacePlugin); }
		};
		m_PluginMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
		m_PluginMenu->GetMenu(0)->SetCommand(new CSceneAdder((CSurfacePlugin *)data));
		return m_PluginMenu;
	case CMD_PILVELEM: {
		CListElement *le = (CListElement *)data;
		CTreeElement *te = (CTreeElement *)le->GetData();
		CTreeFileElement *fe = te->IsFile();
		if(fe){
			m_PluginMenu->GetMenu(0)->SetCommand(
				new CSceneAdder((CSurfacePlugin *)fe->GetPlugin()));
			return m_PluginMenu;
		}
		break; }
	case CMD_SCENE: {
		class CSceneSelector: public CMenuCommand{
		private:
			CScene *m_Scene;	//	シーン
		public:
			CSceneSelector(CScene *sc){ m_Scene = sc; }
			void Exec(){ m_Scene->Enter(false); CGameMode::SetNeutral(); }
		};
		class CSceneDeleter: public CMenuCommand{
		private:
			bool m_Confirm;		//	確認
			CScene *m_Scene;	//	シーン
		public:
			CSceneDeleter(bool c, CScene *sc){ m_Confirm = c; m_Scene = sc; }
			void Exec(){
				if(m_Confirm){
					g_SceneEditMode->DeleteScene(m_Scene);
				}else{
					CYesNoDialog *dlg = new CYesNoDialog(
						lang(DeleteSceneCfmMessage), m_Scene->GetName(), false);
					dlg->SetYesCommand(new CSceneDeleter(true, m_Scene));
					EnqueueCommonDialog(dlg);
				}
			}
		};
		class CSceneRenamer: public CMenuCommand{
		private:
			CScene *m_Scene;	//	シーン
		public:
			CSceneRenamer(CScene *sc){ m_Scene = sc; }
			void Exec(){ m_Scene->GetListElement()->BeginRename(); }
		};
		if(data){
			m_SceneMenu->GetMenu(0)->Enable(true);
			m_SceneMenu->GetMenu(0)->SetCommand(new CSceneSelector(
				(CScene *)((CListElement *)data)->GetData()));
			m_SceneMenu->GetMenu(1)->Enable(!g_NetworkInitialized);
			m_SceneMenu->GetMenu(1)->SetCommand(new CSceneDeleter(
				false, (CScene *)((CListElement *)data)->GetData()));
			m_SceneMenu->GetMenu(2)->Enable(!g_NetworkInitialized);
			m_SceneMenu->GetMenu(2)->SetCommand(new CSceneRenamer(
				(CScene *)((CListElement *)data)->GetData()));
		}else{
			m_SceneMenu->GetMenu(0)->Enable(false);
			m_SceneMenu->GetMenu(1)->Enable(false);
			m_SceneMenu->GetMenu(2)->Enable(false);
		}
		return m_SceneMenu; }
	}
	return CPluginMode::Dispatch(type, data);
}

/*
 *	ダブルクリック処理
 */
void CSceneEditMode::DoubleClick(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_PITVELEM:
		AddScene((CSurfacePlugin *)data);
		break;
	}
}

/*
 *	現在のモデルプラグインを取得
 */
CModelPlugin *CSceneEditMode::GetModelPlugin(){
	return g_Surface;
}

/*
 *	シーン追加
 */
void CSceneEditMode::AddScene(
	CSurfacePlugin *spi	//	地形プラグイン
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	g_SaveFile->AddScene(spi);
	g_SaveFile->ListScene(&m_SceneListView);
	int id = m_SceneListView.GetItemNum()-1;
	m_SceneListView.SetSelectionMark(id, 0);
	m_SceneListView.EnsureVisible(id);
	m_SceneListView.GetFocusItem()->BeginRename();
}

/*
 *	シーン削除
 */
void CSceneEditMode::DeleteScene(
	CScene *sc	//	シーン
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	g_SaveFile->DeleteScene(sc);
	g_SaveFile->ListScene(&m_SceneListView);
}

/*
 *	プラグインリスト取得
 */
CPluginList *CSceneEditMode::GetPluginList(){
	return g_SurfacePluginList;
}

/*
 *	モードを有効化
 */
void CSceneEditMode::EnterModelPlugin(){
	ms_ModeLabel = lang(EditScene);
	m_PluginTree.SelectPlugin(g_Surface = Surface());
	g_SaveFile->ListScene(&m_SceneListView);
	m_SceneListView.GiveFocus(false);
}

/*
 *	入力チェック
 */
CModelInst *CSceneEditMode::ScanInputModelPlugin(){
	if(g_RSPV) return NULL;
	if(m_SceneWindow.CheckClose()){
		SetNeutral();
		return NULL;
	}
	if(m_AddButton.IsPushed()) AddScene(g_Surface);
	if(m_SceneListView.GetInsertRow()>=0){
		int move_from = m_SceneListView.GetSelectionMark();
		int move_to = m_SceneListView.GetInsertRow();
		if(move_from<0) ErrorDialog("invalid value: move_from");
		vector<CScene *> scene_list = g_SaveFile->GetSceneByVector();
		scene_list.insert(scene_list.begin()+move_to, scene_list[move_from]);
		scene_list.erase(scene_list.begin()+(move_to<=move_from ? move_from+1 : move_from));
		g_SaveFile->SetSceneByVector(scene_list);
		g_SaveFile->ListScene(&m_SceneListView);
	}
	CListElement *sle = m_SceneListView.GetFocusItem();
	if(sle && sle->IsSelected()){
		CScene *sc = (CScene *)sle->GetData();
		if(sc!=m_SelectScene){
			m_SelectScene = sc;
			m_PluginTree.SelectPlugin(m_SelectScene->m_SurfacePlugin);
			m_SelectScene->m_SurfacePlugin->SetSwitch(m_SelectScene);
			m_PreviewModel = NULL;
			return m_SelectScene;
		}else if(!g_Surface || !CPlugin::IsPreview()
			|| g_Surface->GetLinkInst()!=m_SelectScene){
			m_SceneListView.SetSelectionMark(-1, 0);
		}
	}else if(m_SelectScene){
		m_SelectScene = NULL;
		if(g_Surface) g_Surface->FreeInst();
	}
	return NULL;
}

/*
 *	レンダリング
 */
void CSceneEditMode::RenderModelPlugin(){
	if(CPlugin::IsPreview()){
		CSurfacePlugin::RenderPreview();
		devSetZRead(FALSE);
		devSetZWrite(FALSE);
		RenderCompass();
	}
}
