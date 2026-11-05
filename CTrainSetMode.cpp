#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailDetectCurve.h"
#include "CTrainGroup.h"
#include "CTrainSetCurve.h"
#include "CTrainSetMode.h"
#include "CSkinPlugin.h"

//	外部グローバル
extern bool g_UpdateTrainGroupList;

/*
 *	リネーム終了処理
 */
void CGroupListView::EndRename(
	CListElement *item	//	アイテム
){
	CTrainGroup *sc = (CTrainGroup *)item->GetData();
	sc->SetName(item->GetString(0));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ダブルクリック
 */
void CTrainListView::DoubleClick(){
	CTrain *train = (CTrain *)m_FocusItem->GetData();
	g_Skin->MouseUp();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	車輌名変更
 */
void CGroupRenamer::Exec(){
	m_Group->GetListElement()->BeginRename();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CTrainSetMode::CTrainSetMode(){
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
}

/*
 *	デストラクタ
 */
CTrainSetMode::~CTrainSetMode(){
	DELETE_V(m_GroupMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CTrainSetMode::WindowResized(
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
CPopMenu *CTrainSetMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_GROUP: {
		if(data){
			CTrainGroup *group = (CTrainGroup *)((CListElement *)data)->GetData();
			m_GroupMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
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
void CTrainSetMode::EnterCursorScenery(){
	ms_ModeLabel = lang(SetTrain);
	CRailDetectCurve2D::ResetDetect();
	g_SaveFile->ListGroup(&m_GroupListView);
	g_UpdateTrainGroupList = false;
}

/*
 *	入力チェック
 */
void CTrainSetMode::ScanInputCursorScenery(){
	if(g_UpdateTrainGroupList){
		g_SaveFile->ListGroup(&m_GroupListView);
		g_UpdateTrainGroupList = false;
	}
	if(ms_PhotoMode || !m_Interface.ScanInput()){
		if(!CheckAlt() && GetKey(DIK_TAB)==S_PUSH){
			g_SaveFile->NextGroup(CheckShift());
			if(g_TrainGroup) m_GroupListView.SetSelectionMark(g_TrainGroup->GetSerial(), 0);
		}
		bool splitting = false;
		if(g_TrainGroup){
			g_TrainGroup->Control(NULL);
			if(g_TrainGroup->IsSet() && CheckShift() && CheckCtrl()) splitting = true;
		}
		if(splitting) CRailDetectCurve2D::ResetDetect();
		else g_Scene->ScanInputRailWay(1, g_Cursor.GetVEC3(), V3ZERO, false);
		switch(GetCamera()->ScanInput(1)){
		case 12:
			if(CRailDetectCurve2D::IsDetected() && !splitting){
				if(g_TrainGroup){
					if(g_NetworkInitialized){
						CRailLinkTemp &link = CRailDetectCurve2D::GetDetect();
						void EnqueueSetTrainControl(void *, void *, int, float, int);
						EnqueueSetTrainControl(g_TrainGroup->OldAdr(), link.m_Link->OldAdr(),
							link.m_Side, link.m_SumLen, CheckCtrl() ? (CheckShift() ? 2 : 1) : 0);
					}else{
						g_TrainGroup->Set(&CRailDetectCurve2D::GetDetect(), -1);
					}
				}else{
					EnqueueCommonDialog(
						new CSimpleDialog(lang(NoConsistSelected), lang(Error)));
					g_Skin->Error();
				}
			}
			break;
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
void CTrainSetMode::RenderCursorScenery(){
	if(!ms_PhotoMode) CRailDetectCurve2D::RenderLink();
	if(g_TrainGroup) g_TrainGroup->PrintInfo();
}
