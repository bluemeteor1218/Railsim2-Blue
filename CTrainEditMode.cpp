#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CTrain.h"
#include "CTrainGroup.h"
#include "CSaveFile.h"
#include "CTrainPlugin.h"
#include "CTrainGroupTemplate.h"
#include "CSkinPlugin.h"
#include "CTrainEditMode.h"

//	外部定数
extern const float GROUP_PREVIEW_SPEED;

//	外部グローバル
extern bool g_PreviewAnimation;
extern bool g_UpdateTrainGroupList;

/*
 *	コンストラクタ
 */
CTrainEditMode::CTrainEditMode():
	CModelPluginMode(lang(TrainPlugin))	//	基本クラス
{
	m_MyCamera.Init(40.0f, 2.0f, 200.0f, false);
	int ww = TILE_UNIT*16, wh = TILE_UNIT*10;
	if(!g_RSPV) m_AddButton.Init(0, 0, TILE_UNIT*4, TILE_UNIT, lang(Add), &m_InfoWindow);
	m_InfoWindow.SetSize(m_InfoWindow.GetWidth(), m_InfoWindow.GetHeight());
	if(!g_RSPV){
		m_TemplateWindow.Init(g_DispWidth-ww-TILE_UNIT*5, TILE_UNIT*3,
			ww, wh, lang(ConsistTemplate), &m_Interface, false);
		m_TemplateWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
		char *traincol[2] = {lang(Train), lang(Invert)};
		m_TemplateListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
			&m_TemplateWindow, 2, traincol, DRAG_NONE, 0, this, CMD_NONE);
		m_TemplateWindow.Show(false);
		m_GroupWindow.Init(g_DispWidth-ww-TILE_UNIT*3, g_DispHeight-wh-TILE_UNIT*5,
			ww, wh, lang(ConsistList), &m_Interface, true);
		m_GroupWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
		char *groupcol[3] = {lang(Name), lang(CarNum), lang(State)};
		m_GroupListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
			&m_GroupWindow, 3, groupcol, DRAG_INSERT, LISTVIEW_RENAMABLE|LISTVIEW_INSERTABLE, this, CMD_GROUP);
		m_GroupListView.GiveFocus(false);
		m_TrainWindow.Init(g_DispWidth-ww-TILE_UNIT*5, g_DispHeight-wh-TILE_UNIT*3,
			ww, wh, lang(CarList), &m_Interface, false);
		m_TrainWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
		m_TrainListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
			&m_TrainWindow, 2, traincol, DRAG_INSERT, LISTVIEW_INSERTABLE, this, CMD_TRAIN);
	}
	InitSwitchWindow();
	m_TrainListView.GiveFocus(false);
	m_OptionListView.GiveFocus(false);
	class CGroupPreviewSwitcher: public CMenuCommand{
	public:
		void Exec(){ g_TrainEditMode->SwitchPreviewMode(); }
	};
	if(g_RSPV){
		m_GroupMenu = NULL;
		m_TrainMenu = NULL;
		m_TemplateMenu = NULL;
	}else{
		new CPopMenu( "-", m_PreviewMenu);
		(new CPopMenu( lang(SingleView), m_PreviewMenu))->SetCommand(new CGroupPreviewSwitcher);
		m_PluginMenu = new CPopMenu( "", NULL);
		new CPopMenu(lang(AddCar), m_PluginMenu);
		m_GroupMenu = new CPopMenu("", NULL);
		new CPopMenu(lang(AddConsist), m_GroupMenu);
		new CPopMenu(lang(DeleteConsist), m_GroupMenu);
		new CPopMenu(lang(ChangeName), m_GroupMenu);
		new CPopMenu(lang(SaveToTemplate), m_GroupMenu);
		m_TrainMenu = new CPopMenu("", NULL);
		new CPopMenu(lang(InvertCar), m_TrainMenu);
		new CPopMenu(lang(DeleteCar), m_TrainMenu);
		m_TemplateMenu = new CPopMenu("", NULL);
		new CPopMenu(lang(AddToNewConsist), m_TemplateMenu);
		new CPopMenu(lang(AddToCurrentConsist), m_TemplateMenu);
		new CPopMenu(lang(DeleteTemplate), m_TemplateMenu);
		new CPopMenu(lang(ChangeName), m_TemplateMenu);
		m_GroupPreview = true;
	}
}

/*
 *	デストラクタ
 */
CTrainEditMode::~CTrainEditMode(){
	DELETE_V(m_GroupMenu);
	DELETE_V(m_TrainMenu);
	DELETE_V(m_TemplateMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CTrainEditMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_TemplateWindow){
		m_TemplateListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}else if(wnd==&m_GroupWindow){
		m_GroupListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}else if(wnd==&m_TrainWindow){
		m_TrainListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
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
CPopMenu *CTrainEditMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	CListElement *renametarget = NULL;
RETRY:
	switch(type){
	case CMD_PITVELEM:
		class CTrainAdder: public CMenuCommand{
		private:
			CTrainPlugin *m_TrainPlugin;	//	車輌プラグイン
		public:
			CTrainAdder(CTrainPlugin *tp){ m_TrainPlugin = tp; }
			void Exec(){ g_TrainEditMode->AddTrain(m_TrainPlugin); }
		};
		m_PluginMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
		m_PluginMenu->GetMenu(0)->SetCommand(new CTrainAdder((CTrainPlugin *)data));
		return m_PluginMenu;
	case CMD_PILVELEM:
		if(data){
			CListElement *le = (CListElement *)data;
			CTreeElement *te = (CTreeElement *)le->GetData();
			CTreeFileElement *fe = te->IsFile();
			if(fe){
				renametarget = le;
				type = fe->GetCommandType();
				data = (DWORD)fe->GetPlugin();
				goto RETRY;
			}
		}else{
			return CPluginMode::Dispatch(type, data);
		}
		break;
	case CMD_GROUPTMP:
		SetTemplateMenu(m_TemplateMenu, (CTrainGroupTemplate *)data, renametarget);
		return m_TemplateMenu;
	case CMD_GROUP: {
		class CGroupAdder: public CMenuCommand{
		public:
			void Exec(){ g_TrainEditMode->AddGroup(); }
		};
		class CGroupDeleter: public CMenuCommand{
		private:
			CTrainGroup *m_Group;	//	編成
		public:
			CGroupDeleter(CTrainGroup *gr){ m_Group = gr; }
			void Exec(){ g_TrainEditMode->DeleteGroup(m_Group); }
		};
		m_GroupMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
		m_GroupMenu->GetMenu(0)->SetCommand(new CGroupAdder);
		if(data){
			CTrainGroup *group = (CTrainGroup *)((CListElement *)data)->GetData();
			m_GroupMenu->GetMenu(1)->Enable(!g_NetworkInitialized);
			m_GroupMenu->GetMenu(1)->SetCommand(new CGroupDeleter(group));
			m_GroupMenu->GetMenu(2)->Enable(!g_NetworkInitialized);
			m_GroupMenu->GetMenu(2)->SetCommand(new CGroupRenamer(group));
			if(group->GetTrainNum()){
				m_GroupMenu->GetMenu(3)->Enable(true);
				m_GroupMenu->GetMenu(3)->SetCommand(MakeGroupSaver(group));
			}else{
				m_GroupMenu->GetMenu(3)->Enable(false);
			}
		}else{
			m_GroupMenu->GetMenu(1)->Enable(false);
			m_GroupMenu->GetMenu(2)->Enable(false);
			m_GroupMenu->GetMenu(3)->Enable(false);
		}
		return m_GroupMenu; }
	case CMD_TRAIN: {
		if(!data) return NULL;
		class CTrainReverser: public CMenuCommand{
		private:
			CTrain *m_Train;	//	車輌
		public:
			CTrainReverser(CTrain *tr){ m_Train = tr; }
			void Exec(){
				if(g_NetworkInitialized){
					g_Skin->Error();
					return;
				}
				m_Train->Reverse();
			}
		};
		class CTrainDeleter: public CMenuCommand{
		private:
			CTrain *m_Train;	//	車輌
		public:
			CTrainDeleter(CTrain *tr){ m_Train = tr; }
			void Exec(){ g_TrainEditMode->DeleteTrain(m_Train); }
		};
		m_TrainMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
		m_TrainMenu->GetMenu(0)->SetCommand(new CTrainReverser(
			(CTrain *)((CListElement *)data)->GetData()));
		m_TrainMenu->GetMenu(1)->Enable(!g_NetworkInitialized);
		m_TrainMenu->GetMenu(1)->SetCommand(new CTrainDeleter(
			(CTrain *)((CListElement *)data)->GetData()));
		return m_TrainMenu; }
	}
	return CPluginMode::Dispatch(type, data);
}

/*
 *	ダブルクリック処理
 */
void CTrainEditMode::DoubleClick(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_PITVELEM:
		AddTrain((CTrainPlugin *)data);
		break;
	case CMD_GROUPTMP:
		NewFromTemplate((CTrainGroupTemplate *)data);
		break;
	}
}

/*
 *	プレビューモード変更
 */
void CTrainEditMode::SwitchPreviewMode(){
	m_GroupPreview = !m_GroupPreview;
	m_PreviewMenu->GetMenu(7)->SetString(
		m_GroupPreview ? lang(SingleView) : lang(ConsistView));
}

/*
 *	編成追加
 */
void CTrainEditMode::AddGroup(){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	g_SaveFile->AddGroup();
	g_SaveFile->ListGroup(&m_GroupListView);
	int id = m_GroupListView.GetItemNum()-1;
	m_GroupListView.SetSelectionMark(id, 0);
	m_GroupListView.EnsureVisible(id);
	m_GroupListView.GetFocusItem()->BeginRename();
}

/*
 *	編成削除
 */
void CTrainEditMode::DeleteGroup(
	CTrainGroup *gr	//	編成
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	if(gr->IsSet()){
		EnqueueCommonDialog(new CSimpleDialog(lang(CannotChangeWhileTrainSet), lang(Error)));
		g_Skin->Error();
		return;
	}
	if(g_Train) g_Train->FreeInst();
	m_TrainListView.DeleteAllItems();
	g_SaveFile->DeleteGroup(gr);
	if(g_TrainGroup) g_TrainGroup->ListTrain(&m_TrainListView);
	g_SaveFile->ListGroup(&m_GroupListView);
}

/*
 *	テンプレートから編成追加
 */
void CTrainEditMode::NewFromTemplate(
	CTrainGroupTemplate *tmp	//	テンプレート
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	g_SaveFile->AddGroup();
	g_SaveFile->ListGroup(&m_GroupListView);
	int id = m_GroupListView.GetItemNum()-1;
	m_GroupListView.SetSelectionMark(id, 0);
	m_GroupListView.EnsureVisible(id);
	CListElement *gle = m_GroupListView.GetFocusItem();
	g_TrainGroup = (CTrainGroup *)gle->GetData();
	tmp->AddToGroup(g_TrainGroup);
	g_TrainGroup->ListTrain(&m_TrainListView);
	m_GroupListView.GetFocusItem()->BeginRename();
}

/*
 *	テンプレートから車輌追加
 */
void CTrainEditMode::AddFromTemplate(
	CTrainGroupTemplate *tmp	//	テンプレート
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	if(!g_TrainGroup) return;
	tmp->AddToGroup(g_TrainGroup);
	g_TrainGroup->ListTrain(&m_TrainListView);
	int id = m_TrainListView.GetItemNum()-1;
	m_TrainListView.SetSelectionMark(id, 0);
	m_TrainListView.EnsureVisible(id);
	m_TrainListView.GiveFocus(false);
}

/*
 *	車輌追加
 */
void CTrainEditMode::AddTrain(
	CTrainPlugin *tpi	//	車輌プラグイン
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	if(!g_TrainGroup){
		EnqueueCommonDialog(new CSimpleDialog(lang(NoConsistSelected), lang(Error)));
		g_Skin->Error();
		return;
	}
	g_TrainGroup->AddTrain(tpi, CheckCtrl());
	g_TrainGroup->ListTrain(&m_TrainListView);
	int id = m_TrainListView.GetItemNum()-1;
	m_TrainListView.SetSelectionMark(id, 0);
	m_TrainListView.EnsureVisible(id);
	m_TrainListView.GiveFocus(false);
}

/*
 *	車輌削除
 */
void CTrainEditMode::DeleteTrain(
	CTrain *tr	//	車輌
){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	if(!g_TrainGroup){
		EnqueueCommonDialog(new CSimpleDialog(lang(NoConsistSelected), lang(Error)));
		g_Skin->Error();
		return;
	}
	if(g_Train) g_Train->FreeInst();
	g_TrainGroup->DeleteTrain(tr);
	g_TrainGroup->ListTrain(&m_TrainListView);
}

/*
 *	現在のモデルプラグインを取得
 */
CModelPlugin *CTrainEditMode::GetModelPlugin(){
	return g_Train;
}

/*
 *	プラグインリスト取得
 */
CPluginList *CTrainEditMode::GetPluginList(){
	return g_TrainPluginList;
}

/*
 *	プラグイン検索
 */
CPlugin *CTrainEditMode::FindModelPlugin(
	char *type,	//	タイプ
	char *id	//	識別子
){
	if(string(type)=="TrainGroupTemplate"){
		ITrainGroupTemplate itgt = FindTrainGroupTemplate(id);
		return itgt==g_TrainGroupTemplateList.end() ? NULL : &*itgt;
	}
	return NULL;
}

/*
 *	設定読込
 */
char *CTrainEditMode::LoadModelPluginSetting(
	char *str	//	対象文字列
){
	ListTrainGroupTemplate();
	return str;
}

/*
 *	モードを有効化
 */
void CTrainEditMode::EnterModelPlugin(){
	ms_ModeLabel = lang(EditTrain);
	if(g_Train) m_PluginTree.SelectPlugin(g_Train);
	else m_PluginTree.SelectPlugin(g_TrainGroupTemplate);
	g_SaveFile->ListGroup(&m_GroupListView);
	if(g_TrainGroup) g_TrainGroup->ListTrain(&m_TrainListView);
	else m_TrainListView.DeleteAllItems();
	//m_TrainListView.GiveFocus();
}

/*
 *	モーダル処理
 */
void CTrainEditMode::ModalFuncModelPlugin(){
	ModalFuncTrainGroupTemplate();
}

/*
 *	入力チェック
 */
CModelInst *CTrainEditMode::ScanInputModelPlugin(){
	if(g_RSPV) return NULL;
	if(m_GroupWindow.CheckClose()){
		SetNeutral();
		return NULL;
	}
	if(m_AddButton.IsPushed()) AddTrain(g_Train);
	if(m_GroupListView.GetInsertRow()>=0){
		int move_from = m_GroupListView.GetSelectionMark();
		int move_to = m_GroupListView.GetInsertRow();
		if(move_from<0) ErrorDialog("invalid value: move_from");
		vector<CTrainGroup *> group_list = g_SaveFile->GetTrainGroupByVector();
		group_list.insert(group_list.begin()+move_to, group_list[move_from]);
		group_list.erase(group_list.begin()+(move_to<=move_from ? move_from+1 : move_from));
		g_SaveFile->SetTrainGroupByVector(group_list);
		g_SaveFile->ListGroup(&m_GroupListView);
	}
	if(m_TrainListView.GetInsertRow()>=0){
		if(g_TrainGroup->IsSet()){
			EnqueueCommonDialog(new CSimpleDialog(lang(CannotChangeWhileTrainSet), lang(Error)));
			g_Skin->Error();
		}else{
			int move_from = m_TrainListView.GetSelectionMark();
			int move_to = m_TrainListView.GetInsertRow();
			if(move_from<0) ErrorDialog("invalid value: move_from");
			vector<CTrain *> train_list = g_TrainGroup->GetTrainByVector();
			train_list.insert(train_list.begin()+move_to, train_list[move_from]);
			train_list.erase(train_list.begin()+(move_to<=move_from ? move_from+1 : move_from));
			g_TrainGroup->SetTrainByVector(train_list);
			g_TrainGroup->ListTrain(&m_TrainListView);
			m_TrainListView.SetSelectionMark(move_from<move_to ? move_to-1 : move_to, 0);
		}
	}
	CTrain *linktrain = NULL;
	CListElement *gle = m_GroupListView.GetFocusItem();
	if(gle && gle->IsSelected()){
		CTrainGroup *tg = (CTrainGroup *)gle->GetData();
		if(tg!=g_TrainGroup){
			g_TrainGroup = tg;
			tg->ListTrain(&m_TrainListView);
			m_TrainWindow.Show(true);
			if(g_Train) g_Train->FreeInst();
		}
	}else if(g_TrainGroup){
		g_TrainGroup = NULL;
		m_TrainListView.DeleteAllItems();
		m_TrainWindow.Show(false);
		if(g_Train) g_Train->FreeInst();
	}
	if(g_TrainGroup){
		CTrain *train = g_TrainGroup->GetSelectedTrain();
		if(linktrain = g_TrainGroup->EditTrain(&m_TrainListView, &m_PluginTree))
			m_PreviewModel = NULL;
		if(m_GroupPreview && g_TrainGroup && g_TrainGroup->GetSelectedTrain()
			&& train!=g_TrainGroup->GetSelectedTrain())
			m_Camera->SetFocusTarget(V3ZERO, GROUP_PREVIEW_SPEED);
		if(GetKey(DIK_DELETE)==S_PUSH && !CEditBox::IsActive()){
			if(m_TrainListView.IsFocus()){
				int selm = m_TrainListView.GetSelectionMark();
				DeleteTrain(train);
				if(selm>=m_TrainListView.GetItemNum()) selm = m_TrainListView.GetItemNum()-1;
				m_TrainListView.SetSelectionMark(selm, 0);
			}else if(m_GroupListView.IsFocus()){
				int selm = m_GroupListView.GetSelectionMark();
				DeleteGroup(g_TrainGroup);
				if(selm>=m_GroupListView.GetItemNum()) selm = m_GroupListView.GetItemNum()-1;
				m_GroupListView.SetSelectionMark(selm, 0);
			}
		}
	}
	CTreeElement *el = m_PluginTree.GetFocusItem();
	if(el){
		CTreeFileElement *fe = el->IsFile();
		if(fe){
			if(fe->GetCommandType()==CMD_GROUPTMP){
				m_InfoWindow.Show(false);
				m_TemplateWindow.Show(true);
			}else{
				m_TemplateWindow.Show(false);
			}
		}else{
			m_TemplateWindow.Show(false);
		}
	}
	return linktrain;
}

/*
 *	レンダリング
 */
void CTrainEditMode::RenderModelPlugin(){
	if(!g_Train) return;
	if(m_GroupPreview && g_TrainGroup && g_TrainGroup->GetSelectedTrain()){
		g_PreviewAnimation = false;
		g_SaveFile->ResetSwitch();
		g_TrainGroup->Preview();
		g_PreviewAnimation = true;
	}else{
		CTrainPlugin::RenderPreview();
	}
}
