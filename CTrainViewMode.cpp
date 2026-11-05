#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CTrainGroup.h"
#include "CTrainSetCurve.h"
#include "CTrainViewMode.h"
#include "CSkinPlugin.h"

//	外部グローバル
extern bool g_UpdateTrainGroupList;

/*
 *	コンストラクタ
 */
CTrainViewMode::CTrainViewMode(){
	int ww = TILE_UNIT*16, wh = TILE_UNIT*10;
	char *groupcol[3] = {lang(Name), lang(CarNum), lang(State)};
	m_GroupWindow.Init(g_DispWidth-ww-TILE_UNIT*3, g_DispHeight-wh-TILE_UNIT,
		ww, wh, lang(ConsistList), &m_Interface, true);
	m_GroupWindow.SetResize(TILE_UNIT*8, TILE_UNIT*8, g_DispWidth, g_DispHeight, this);
	m_GroupListView.Init(TILE_UNIT/2, TILE_QUAD+TILE_UNIT, ww-TILE_UNIT, wh-TILE_UNIT*2,
		&m_GroupWindow, 3, groupcol, DRAG_NONE, LISTVIEW_RENAMABLE, this, CMD_GROUP);
	m_GroupListView.GiveFocus(false);
	m_GroupMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(ChangeName), m_GroupMenu);
	m_ViewMode = 0;
}

/*
 *	デストラクタ
 */
CTrainViewMode::~CTrainViewMode(){
	DELETE_V(m_GroupMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CTrainViewMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_GroupWindow){
		m_GroupListView.SetSize(w-TILE_UNIT, h-TILE_UNIT*2);
	}
}

/*
 *	メニュー発行
 */
CPopMenu *CTrainViewMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_GROUP: {
		if(data){
			CTrainGroup *group = (CTrainGroup *)((CListElement *)data)->GetData();
			m_GroupMenu->GetMenu(0)->Enable(true);
			m_GroupMenu->GetMenu(0)->SetCommand(new CGroupRenamer(group));
		}else{
			m_GroupMenu->GetMenu(0)->Enable(false);
		}
		return m_GroupMenu; }
	}
	return NULL;
}

/*
 *	モードを有効化
 */
void CTrainViewMode::EnterCursorScenery(){
	ms_ModeLabel = lang(TrainView);
	g_SaveFile->ListGroup(&m_GroupListView);
	m_GroupLocal.SetPos(V3ZERO);
	m_GroupLocal.SetDir(V3DIR, V3UP);
}

/*
 *	カメラ適用
 */
void CTrainViewMode::ApplyCamera(){
	if(g_TrainGroup && g_TrainGroup->IsSet()){
		if(m_ViewMode){
			g_TrainGroup->SetCabinView();
			GetCamera()->SetFocus(GetVPos());
			GetCamera()->ApplyProjection(0.2f, &m_GroupLocal);
		}else{
			g_TrainGroup->CalcViewAxis(&m_GroupLocal);
			GetCamera()->Apply(false, &m_GroupLocal);
		}
	}else{
		CSceneryMode::ApplyCamera();
	}
}

/*
 *	入力チェック
 */
void CTrainViewMode::ScanInputCursorScenery(){
	if(g_UpdateTrainGroupList){
		g_SaveFile->ListGroup(&m_GroupListView);
		g_UpdateTrainGroupList = false;
	}
	if(ms_PhotoMode || !m_Interface.ScanInput()){
		if(GetKey(DIK_SPACE)==S_PUSH) m_ViewMode = !m_ViewMode;
		if(!CheckAlt() && GetKey(DIK_TAB)==S_PUSH){
			CTrainGroup *orig = g_TrainGroup;
			while(true){
				g_SaveFile->NextGroup(CheckShift());
				if(g_TrainGroup->IsSet()) break;
				if(g_TrainGroup==orig){
					EnqueueCommonDialog(
						new CSimpleDialog(lang(NoConsistSet), lang(Error)));
					g_Skin->Error();
					break;
				}
			}
			if(g_TrainGroup) m_GroupListView.SetSelectionMark(g_TrainGroup->GetSerial(), 0);
		}
		if(g_TrainGroup) g_TrainGroup->Control(NULL);
		if(m_ViewMode && g_TrainGroup && g_TrainGroup->IsSet()){
			GetCamera()->ScanInput(4);
		}else{
			switch(GetCamera()->ScanInput(1, &m_GroupLocal)){
			case 12:
				break;
			case 0:
				GetCamera()->ControlLocal(&m_GroupLocal);
				break;
			}
		}
	}
	if(m_GroupWindow.CheckClose()){
		SetNeutral();
		return;
	}
	CListElement *gle = m_GroupListView.GetFocusItem();
	if(gle && gle->IsSelected()){
		CTrainGroup *tg = (CTrainGroup *)gle->GetData();
		if(tg!=g_TrainGroup && !g_UpdateTrainGroupList) g_TrainGroup = tg;
	}else if(g_TrainGroup){
		g_TrainGroup = NULL;
	}
}

/*
 *	レンダリング
 */
void CTrainViewMode::RenderCursorScenery(){
	if(g_TrainGroup) g_TrainGroup->PrintInfo();
}
