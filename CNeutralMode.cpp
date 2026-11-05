#include "stdafx.h"
#include "CRailConnector.h"
#include "CRailWay.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CModelInst.h"
#include "CNeutralMode.h"

//	外部グローバル
extern CScene *g_Scene;

/*
 *	コンストラクタ
 */
CNeutralMode::CNeutralMode(){
	int ww = TILE_UNIT*16, wh = TILE_UNIT*10;
	m_SwitchWindow.Init(TILE_UNIT, g_DispHeight-wh-TILE_UNIT*2,
		ww, wh, lang(AppearanceSetting), &m_Interface, true);
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
void CNeutralMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_SwitchWindow){
		int h0 = h-TILE_UNIT*5/2, h1 = h0/2;
		m_SwitchListView.SetSize(w-TILE_UNIT, h1);
		m_OptionListView.SetSize(w-TILE_UNIT, h0-h1);
		m_OptionListView.SetPos(TILE_HALF, TILE_QUAD*3+TILE_UNIT+h1);
	}
}

/*
 *	インスタンス削除の確認
 */
void CNeutralMode::DeleteModelInst(
	CModelInst *minst	//	インスタンス
){
	if(GetCamera() && (!minst || minst==GetCamera()->GetFocusInst()))
		GetCamera()->SetFocusInfo(R2L(CDetectInfo()));
	if(!minst || minst==GetFocusInst()){
		if(GetFocusInst()) GetFocusInst()->GetModelPlugin()->FreeInst();
		m_FocusInfo = CDetectInfo();
		m_SwitchWindow.Show(false);
	}
}

/*
 *	モードを有効化
 */
void CNeutralMode::EnterCursorScenery(){
	if(IsModeActive()) DeleteModelInst(NULL);
	ms_ModeLabel = lang(Neutral);
	m_SwitchWindow.Show(false);
	g_Scene->ScanInputRailConnector(5, g_Cursor.GetVEC3(), V3ZERO);
	m_PointMode = false;
}

/*
 *	入力チェック
 */
void CNeutralMode::ScanInputCursorScenery(){
	m_PointMode = false;
	if(!ms_PhotoMode){
		if(m_Interface.ScanInput()) return;
		if(m_SwitchWindow.CheckClose()) m_SwitchWindow.Show(false);
	}
	if(g_ManualControl && CheckAlt()){
		m_PointMode = true;
		g_Scene->ScanInputRailConnector(4, g_Cursor.GetVEC3(), V3ZERO);
		if(CRailConnector::IsDetected()) CRailConnector::GetDetect()->AddSelectFlag(1);
		switch(GetCamera()->ScanInput(1)){
		case 12:
			if(CRailConnector::IsDetected()) CRailConnector::GetDetect()->SwitchNetPoint();
			break;
		}
	}else{
		int tmp_sw = GetCamera()->ScanInput(1);
		if(CheckShift() && CheckCtrl()) tmp_sw = 0; // splitting
		switch(tmp_sw){
		case 12:
			g_SaveFile->ScanInputInst(0, g_Cursor.GetVEC3(), V3ZERO);
			g_SaveFile->ScanInputWarp(4, g_Cursor.GetVEC3(), V3ZERO);
			if(CModelInst::IsDetected() && (!CRailWay::IsDetected()
				|| CModelInst::GetMinDist()<CRailWay::GetMinDist())){
				m_FocusInfo = CModelInst::GetDetectInfo();
				GetCamera()->SetFocusInfo(m_FocusInfo);
				CModelPlugin *mpi = GetFocusInst()->GetModelPlugin();
				mpi->SetSwitch(GetFocusInst());
				mpi->ListSwitch(&m_SwitchListView, &m_OptionListView, GetFocusInst());
				m_SwitchWindow.Show(true);
			}else if(CRailWay::IsDetected()){
				VEC3 pos = CRailWay::GetDetect().GetPos();
				if(!GetFocusInst() && V3Len(&(pos-GetCamera()->GetFocus()))<1.0f){
					CRailWay::GetDetect().WarpToOppositeEnd();
				}else{
					DeleteModelInst(NULL);
					GetCamera()->SetFocus(CRailWay::GetDetect().GetPos());
				}
			}
			break;
		case 32:
			DeleteModelInst(NULL);
			break;
		case 0:
			GetCamera()->ControlLocal();
			if(GetFocusInst()){
				CModelInst *tmpinst = GetFocusInst()->Control();
				if(tmpinst!=GetFocusInst()){
					m_FocusInfo = CDetectInfo();
					GetCamera()->SetFocusInfo(R2L(CDetectInfo()));
					m_SwitchWindow.Show(false);
				}
			}
			break;
		}
	}
	if(GetFocusInst()){
		CModelPlugin *mpi = GetFocusInst()->GetModelPlugin();
		mpi->SetSwitch(GetFocusInst());
		mpi->EditSwitch(&m_SwitchListView, &m_OptionListView);
	}
}

/*
 *	レンダリング
 */
void CNeutralMode::RenderCursorScenery(){
	if(ms_PhotoMode<2){
		if(m_PointMode){
			devResetMatrix();
			devSetLighting(FALSE);
			devSetTexture(0, NULL);
			if(CRailConnector::IsDetected()) CRailConnector::GetDetect()->Render(0, true);
			//if(m_EditConnector) m_EditConnector->Render(0);
			devSetLighting(FALSE);
		}
		if(GetFocusInst() && GetFocusInst()->GetScene()==g_Scene){
			if(ms_PhotoMode<1) m_FocusInfo.GetPartsInst()->DrawBox();
			//GetFocusInst()->PrintInfo(); // Full
		}
	}
}

/*
 *	レンダリング (画面分割無視)
 */
void CNeutralMode::RenderCursorSceneryFull(){
	if(ms_PhotoMode<2){
		if(GetFocusInst() && GetFocusInst()->GetScene()==g_Scene){
			GetFocusInst()->PrintInfo();
		}
	}
}
