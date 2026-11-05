#include "stdafx.h"
#include "HighTimer.h"
#include "CPixelbit.h"
#include "Capture.h"
#include "RailMap.h"
#include "CJobTimer.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CWindowDivInfo.h"
#include "CSurfacePlugin.h"
#include "CSimulationMode.h"
#include "CFileMode.h"
#include "CConfigMode.h"
#include "CNeutralMode.h"

//	関数宣言
void ProcessCommonDialog();
bool ScanInputNetworkInterface();
void RenderNetworkInterface();

//	外部定数
extern const float CAM_PAN;

//	内部定数
extern const int ARROW_INTERFACE_HEIGHT = TILE_UNIT*8;
const float ARROW_FOLLOW = 0.5f;	//	矢印追跡速度

//	外部グローバル
extern float g_RailMapScale;
extern float g_StereoInterval;
extern bool g_RotateMap;
extern VEC2 g_RailMapMove;

//	static メンバ
int CSceneryMode::ms_PhotoMode = 0;
bool CSceneryMode::ms_NeedResetViewport = false;

/*
 *	シーンカメラ取得
 */
CCamera *CSceneryMode::GetCamera(){
	if(IsWindowDivisible() && g_ConfigMode->IsWindowDiv()){
		CWindowInfo *wnd = g_ConfigMode->GetActiveWindow();
		if(wnd) return wnd->GetCamera();
	}
	return g_Scene ? g_Scene->GetCamera() : NULL;
}

/*
 *	モードを有効化
 */
void CSceneryMode::EnterGame(){
	CInterface::SetFocus(NULL);
	if(this!=g_NeutralMode) g_NeutralMode->DeleteModelInst(NULL);
	GetCamera()->Select();
	EnterScenery();
}

/*
 *	モードループ
 */
void CSceneryMode::SpinGame(){
	devSetState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	g_ConfigMode->SetTexFilter();
	if(!g_ModalDialog && !IsPausedScenery()) g_SaveFile->Simulate(-1);
	ms_NeedResetViewport = false;
	CWindowInfo* active_wnd = NULL;
	if(g_ConfigMode->GetStereo()){
		ApplyCamera();
		SpinSound();
		float interval = g_ConfigMode->GetStereoInterval()
			*(g_ConfigMode->GetStereoMethod() ? -1 : 1);
		// left
		SetViewport(0, 0, sv3.width/2, sv3.height);
		g_StereoInterval = -0.5f*interval;
		ApplyCamera();
		g_SaveFile->RenderScene(!ms_PhotoMode);
		sv3.pDev->EndScene();
		// right
		SetViewport(sv3.width/2, 0, sv3.width/2, sv3.height);
		g_StereoInterval = 0.5f*interval;
		ApplyCamera();
		g_SaveFile->RenderScene(!ms_PhotoMode);
		sv3.pDev->EndScene();
		// reset
		//SetViewport(0, 0, sv3.width, sv3.height);
		//g_StereoInterval = 0.0f;
		//ApplyCamera();
		BeginScene(0);
		ms_NeedResetViewport = true;
	}else if(IsWindowDivisible() && g_ConfigMode->IsWindowDiv()){
		ApplyCamera();
		SpinSound();
		g_ConfigMode->GetRootWindow()->RenderScene(0, 0, g_DispWidth, g_DispHeight, this, !ms_PhotoMode);
		SetViewport(0, 0, sv3.width, sv3.height);
		active_wnd = g_ConfigMode->GetActiveWindow();
		BeginScene(0);
		g_ConfigMode->RenderWindowDiv();
		if(active_wnd){
			sv3.pDev->EndScene();
			g_Scene = active_wnd->GetScene();
			*g_Scene->GetCamera() = *active_wnd->GetCamera();
			active_wnd->ApplyViewportAndCamera();
			BeginScene(0);
		}
		SpinSound();
		ms_NeedResetViewport = true;
	}else{
		ApplyCamera();
		SpinSound();
		g_SaveFile->RenderScene(!ms_PhotoMode);
	}
	const bool divisible = IsWindowDivisible() && !g_ConfigMode->GetStereo();
	bool enable_scenery_input = true;
	#define SCAN_INPUT_WINDOW_DIV() \
		if(divisible){ \
			switch(g_ConfigMode->ScanInputWindowDiv()){ \
			case 2: \
				active_wnd = NULL; \
			case 1: \
				enable_scenery_input = false; \
				break; \
			default:; \
			} \
		}
	switch(ms_PhotoMode){
	case 0:
		RenderScenery();
		SCAN_INPUT_WINDOW_DIV();
		break;
	case 1:
		RenderScenery();
		g_Cursor.ScanInput(!g_ModalDialog && IsArrowMode());
		SCAN_INPUT_WINDOW_DIV();
		if(ms_NeedResetViewport) ResetViewport();
		GetCamera()->PrintInfo(CameraCtrlExp());
		RenderFrame(3);
		RenderNetworkInterface();
		RenderDialog();
		g_Cursor.Render();
		break;
	case 2:
		g_Cursor.ScanInput(!g_ModalDialog && IsArrowMode());
		if(ms_NeedResetViewport) ResetViewport();
		if(RenderDialog()) g_Cursor.Render();
		break;
	}

#if ENABLE_JOB_TIMER
	g_JobTimer.DrawResult();
#endif

	if(active_wnd){
		sv3.pDev->EndScene();
		g_Scene = active_wnd->GetScene();
		active_wnd->ApplyViewportAndCamera();
		BeginScene(0);
	}
	if(enable_scenery_input) ScanInputScenery();
	{
		TIMER_RAII("EndScene");
		EndScene();
	}
	VideoCapture(1|(ms_PhotoMode==2 ? 4 : 0)
		|(!g_SimulationMode->GetSimSpeed() || IsPaused() ? 2 : 0), this);
}

/*
 *	ビューポート etc. リセット
 */
void CSceneryMode::ResetViewport(){
	SetViewport(0, 0, sv3.width, sv3.height);
	if(g_ConfigMode->GetStereo()){
		g_StereoInterval = 0.0f;
		ApplyCamera();
		BeginScene(0);
	}else if(IsWindowDivisible() && g_ConfigMode->IsWindowDiv()){
	}
}

/*
 *	カメラ適用
 */
void CSceneryMode::ApplyCamera(){
	GetCamera()->Apply(false);
}

/*
 *	ショートカットキーの処理
 */
bool CSceneryMode::ProcessShortcutKey(){
	static int drag_map = 0;
	static POINT map_drag_csr;
	static VEC2 map_drag_mov;
	if(g_ConfigMode->GetShowMap() && !CEditBox::IsActive() && GetKey(DIK_SLASH)>=S_PUSH){
		POINT cp = g_Cursor.GetPos();
		if(GetButton(DIM_LEFT)==S_PUSH){
			drag_map = 1;
			map_drag_csr = cp;
			map_drag_mov = g_RailMapMove;
		}else if(drag_map){
			if(GetButton(DIM_LEFT)==S_HOLD){
				g_RailMapMove = map_drag_mov+VEC2(cp.x-map_drag_csr.x, cp.y-map_drag_csr.y);
			}else{
				drag_map = 0;
			}
		}else{
			int wh = CInterface::GetFocus() ? 0 : GetWheel();
			if(!wh){
				if(GetKey(DIK_PRIOR)>=S_PUSH) wh += 50;
				if(GetKey(DIK_NEXT)>=S_PUSH) wh -= 50;
			}
			float ratio = CheckFast();
			if(wh){
				g_RailMapScale *= 1.0f+wh*ratio*0.0002f;
				ValueArea(&g_RailMapScale, 0.05f, 10.0f);
			}
		}
		return true;
	}
	drag_map = 0;
	if(CheckCtrl() && GetKey(DIK_S)==S_PUSH)
		g_ConfigMode->SetShadow(!g_ConfigMode->GetShadow());
	else if(CheckCtrl() && GetKey(DIK_D)==S_PUSH)
		g_ConfigMode->SetStereo(!g_ConfigMode->GetStereo());
	else if(CheckCtrl() && GetKey(DIK_M)==S_PUSH)
		g_ConfigMode->SetShowMap(!g_ConfigMode->GetShowMap());
	else if(CheckCtrl() && GetKey(DIK_N)==S_PUSH) g_RotateMap = !g_RotateMap;
	else if(CheckCtrl() && GetKey(DIK_Z)==S_PUSH) g_FileMode->LoadUndo();
	else if(CheckCtrl() && GetKey(DIK_Y)==S_PUSH) g_FileMode->LoadRedo();
	else if(CheckCtrl() && GetKey(DIK_UP)==S_PUSH) g_SaveFile->NextScene(true);
	else if(CheckCtrl() && GetKey(DIK_DOWN)==S_PUSH) g_SaveFile->NextScene(false);
	else if(CheckCtrl() && GetKey(DIK_0)==S_PUSH
		|| !CEditBox::IsActive() && GetKey(DIK_NUMPAD0)==S_PUSH)
		g_SimulationMode->SetSimSpeed(0);
	else if(CheckCtrl() && GetKey(DIK_1)==S_PUSH
		|| !CEditBox::IsActive() && GetKey(DIK_NUMPAD1)==S_PUSH)
		g_SimulationMode->SetSimSpeed(1);
	else if(CheckCtrl() && GetKey(DIK_2)==S_PUSH
		|| !CEditBox::IsActive() && GetKey(DIK_NUMPAD2)==S_PUSH)
		g_SimulationMode->SetSimSpeed(2);
	else if(CheckCtrl() && GetKey(DIK_3)==S_PUSH
		|| !CEditBox::IsActive() && GetKey(DIK_NUMPAD3)==S_PUSH)
		g_SimulationMode->SetSimSpeed(3);
	else if(CheckCtrl() && GetKey(DIK_4)==S_PUSH
		|| !CEditBox::IsActive() && GetKey(DIK_NUMPAD4)==S_PUSH)
		g_SimulationMode->SetSimSpeed(4);
	else if(GetKey(DIK_F11)==S_PUSH)
		ms_PhotoMode = (ms_PhotoMode+(CheckShift() ? 2 : 1))%3;
	else return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
CWindowCtrl CArrowSceneryMode::ms_OptionWindow;
CStaticCtrl CArrowSceneryMode::ms_SnapLabel;
CCheckBox CArrowSceneryMode::ms_Grid;
CRadioButton CArrowSceneryMode::ms_Snap[4];
CCheckBox CArrowSceneryMode::ms_Axle[3];
CStaticCtrl CArrowSceneryMode::ms_BuildLabel;
CCheckBox CArrowSceneryMode::ms_Build[2];

/*
 *	[static]
 *	共通インターフェイスの初期化
 */
void CArrowSceneryMode::InitInterface(){
	CInterface::ResetTabHead();
	int i, gww = TILE_UNIT*14, gwh = ARROW_INTERFACE_HEIGHT;
	ms_OptionWindow.Init(TILE_UNIT, g_DispHeight-gwh-TILE_UNIT,
		gww, gwh, lang(CoordOption), NULL, false);
	ms_SnapLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT,
		gww-TILE_UNIT, TILE_UNIT, lang(SnapOption), &ms_OptionWindow, 0, 1);
	ms_Grid.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*2,
		gww-TILE_UNIT, TILE_UNIT, lang(ShowGrid), &ms_OptionWindow);
	ms_Grid.SetCheck(1);
	char *snaplabel[4] = {"10", "1", ".1", ".01"};
	for(i = 0; i<4; i++) ms_Snap[i].Init(
		TILE_HALF+i*(gww-TILE_QUAD*3)/4, TILE_QUAD+TILE_UNIT*3,
		(i+1)*(gww-TILE_QUAD*3)/4-i*(gww-TILE_QUAD*3)/4-TILE_QUAD, TILE_UNIT,
		snaplabel[i], &ms_OptionWindow, i ? &ms_Snap[i-1] : NULL);
	ms_Snap[3].SetCheck();
	char *axlelabel[3] = {"X", "Y", "Z"};
	for(i = 0; i<3; i++){
		ms_Axle[i].Init( TILE_HALF+(i+1)*(gww-TILE_QUAD*3)/4, TILE_QUAD+TILE_UNIT*4,
			(i+2)*(gww-TILE_QUAD*3)/4-(i+1)*(gww-TILE_QUAD*3)/4-TILE_QUAD, TILE_UNIT,
			axlelabel[i], &ms_OptionWindow);
		ms_Axle[i].SetCheck(1);
	}
	char *buildlabel[2] = {lang(UG), lang(Elev)};
	ms_BuildLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*5,
		gww-TILE_UNIT, TILE_UNIT, lang(AltOption), &ms_OptionWindow, 0, 1);
	for(i = 0; i<2; i++) ms_Build[i].Init(
		TILE_HALF+i*(gww-TILE_QUAD*3)/2, TILE_QUAD+TILE_UNIT*6,
		(i+1)*(gww-TILE_QUAD*3)/2-i*(gww-TILE_QUAD*3)/2-TILE_QUAD, TILE_UNIT,
		buildlabel[i], &ms_OptionWindow);
}

/*
 *	[static]
 *	高度クリップマスク値取得
 */
int CArrowSceneryMode::GetAltMask(){
	if(CheckAlt()) return 0;
	int i, ret = 0;
	for(i = 0; i<2; i++) ret |= (!ms_Build[i].GetCheck())<<i;
	return ret;
}

/*
 *	[static]
 *	高度クリップマスク値取得
 */
float CArrowSceneryMode::GetSnapScale(){
	static float snap[4] = {10.0f, 1.0f, 0.1f, 0.01f};
	return snap[ms_Snap->GetNumber()];
}

/*
 *	コンストラクタ
 */
CArrowSceneryMode::CArrowSceneryMode(){
	m_ArrowMode = false;
}

/*
 *	編集座標取得
 */
VEC3 *CArrowSceneryMode::ArrowPos(){
	return g_Scene->GetArrowPos();
}

/*
 *	スナップ座標取得
 */
VEC3 CArrowSceneryMode::GetSnapPos(){
	VEC3 arrowpos = *ArrowPos();
	if(CheckAlt()) return arrowpos;
	float s = GetSnapScale();
	return VEC3(
		ms_Axle[0].GetCheck() ? s*Round(arrowpos.x/s) : arrowpos.x,
		ms_Axle[1].GetCheck() ? s*Round(arrowpos.y/s) : arrowpos.y,
		ms_Axle[2].GetCheck() ? s*Round(arrowpos.z/s) : arrowpos.z);
}

/*
 *	グリッド描画
 */
void CArrowSceneryMode::DrawGrid(){
	if(ms_Grid.GetCheck()){
		::DrawGrid(m_SnapPos);
	}else{
		devSetZRead(FALSE);
		devSetZWrite(FALSE);
		devSetLighting(FALSE);
		devTEX_POINT(0);
		devTEX_POINT(1);
	}
}

/*
 *	モードを有効化
 */
void CArrowSceneryMode::EnterScenery(){
	*ArrowPos() = GetCamera()->GetFocus();
	m_SnapPos = GetSnapPos();
	m_ArrowMode = false;
	EnterArrowScenery();
}

/*
 *	入力チェック
 */
void CArrowSceneryMode::ScanInputScenery(){
	bool setcam = false;
	if(g_ModalDialog){
		//g_Cursor.ScanInput();
		CInterface::ProcessKey();
		g_ModalDialog->ScanInput();
		ProcessCommonDialog();
		ModalFuncArrowScenery();
	}else{
		if(m_ArrowMode && !g_Cursor.IsLock()){
			setcam = true;
			POINT delta = g_Cursor.GetDelta();
			if(CheckShift()){
				*ArrowPos() -= delta.y*V3UP*GetCamera()->GetDist()*CAM_PAN;
			}else{
				VEC3 fow = GetVFow();
				*ArrowPos() += (GetVRight()*delta.x-fow*delta.y)
					*GetCamera()->GetDist()*CAM_PAN*CheckFast();
			}
			m_SnapPos = GetSnapPos();
			Surface()->ClipRect(ArrowPos());
			Surface()->ClipRect(&m_SnapPos);
		}
		//g_Cursor.ScanInput(m_ArrowMode);
		CInterface::ProcessKey();
		CScene *oldscene = g_Scene;
		if(ScanInputFrame(ms_PhotoMode ? 2 : 0)){
		}else if(ProcessShortcutKey()){
			if(g_Scene!=oldscene) m_SnapPos = *ArrowPos();
		}else if(CPopMenu::ScanInputAll()){
		}else if(ms_PhotoMode<2 && ScanInputNetworkInterface()){
		}else{
			ScanInputArrowScenery();
		}
		float s = 0.5f*GetSnapScale();
		ValueArea(&ArrowPos()->y, m_SnapPos.y-s, m_SnapPos.y+s);
	}
	if(setcam) GetCamera()->SetFocus(
		(1.0f-ARROW_FOLLOW)*GetCamera()->GetFocus()+ARROW_FOLLOW*m_SnapPos);
}

/*
 *	レンダリング
 */
void CArrowSceneryMode::RenderScenery(){
	ms_OptionWindow.SetColor(m_ArrowMode ? 0x80ffffff : 0xffffffff);
	RenderArrowScenery();
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	RenderRailMap();
	devSetLighting(TRUE);
	RenderCompass();
	devSetLighting(FALSE);
	devTEX_POINT(0);
	devTEX_POINT(1);
	if(ms_PhotoMode) return;
	m_Interface.Render();
	GetCamera()->PrintInfo();
	g_StrTex->RenderLeft(TILE_QUAD, TILE_UNIT*2+TILE_QUAD, 0xffffffff, 0xff000000,
		FlashIn("%s: (%.2f, %.2f, %.2f)", lang(Coord), m_SnapPos.x, m_SnapPos.y, m_SnapPos.z));
	if(!m_ArrowMode && !g_ModalDialog){
		if(g_NetworkInitialized){
			g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight/3,
				ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
				lang(CannotEditInNetMode));
		}else{
			g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight/3,
				ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
				lang(ClickToBuild));
			g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight/3+TILE_UNIT,
				ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
				lang(EscToAbort));
		}
	}
	RenderFrame(1);
	RenderNetworkInterface();
	RenderDialog();
//	CDragContainer::Render();
	CPopMenu::RenderAll();
	g_Cursor.ScanInput(!g_ModalDialog && m_ArrowMode);
	if(!m_ArrowMode || g_ModalDialog) g_Cursor.Render();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	モードを有効化
 */
void CCursorSceneryMode::EnterScenery(){
	EnterCursorScenery();
}

/*
 *	入力チェック
 */
void CCursorSceneryMode::ScanInputScenery(){
	//g_Cursor.ScanInput();
	CInterface::ProcessKey();
	if(g_ModalDialog){
		g_ModalDialog->ScanInput();
		ProcessCommonDialog();
		ModalFuncCursorScenery();
	}else if(GetKey(DIK_ESCAPE)==S_PUSH){
		SetNeutral();
	}else{
		if(ScanInputFrame(ms_PhotoMode ? 2 : 0)){
		}else if(ProcessShortcutKey()){
		}else if(CPopMenu::ScanInputAll()){
		}else if(ms_PhotoMode<2 && ScanInputNetworkInterface()){
		}else{
			ScanInputCursorScenery();
		}
	}
}

/*
 *	レンダリング
 */
void CCursorSceneryMode::RenderScenery(){
	RenderCursorScenery();
	if(!ms_PhotoMode && !CameraCtrlLock() &&
		(GetButton(DIM_LEFT)|GetButton(DIM_MIDDLE)|GetButton(DIM_RIGHT))>=S_PUSH){
		VEC3 focus = GetCamera()->GetFocus(), hit, tri[3];
		DrawFocus(focus);
		devSetTexture(0, NULL);
		devResetMatrix();
		if(g_Scene->PickScene(focus, -V3UP, &hit, tri, 1)){
			DrawTangent(hit, CalcPlaneNormal(tri[0], tri[1], tri[2]), 0xffff0000, NULL);
			Draw3DLine(hit, focus, 0xffff0000, 0x80ff0000);
		}
		if(g_Scene->PickScene(focus, V3UP, &hit, tri, 2)){
			DrawTangent(hit, CalcPlaneNormal(tri[0], tri[1], tri[2]), 0xffff0000, NULL);
			Draw3DLine(hit, focus, 0xffff0000, 0x80ff0000);
		}
	}
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	RenderRailMap();
	devSetLighting(TRUE);
	RenderCompass();
	devSetLighting(FALSE);
	devTEX_POINT(0);
	devTEX_POINT(1);
	if(ms_PhotoMode) return;
	if(ms_NeedResetViewport) ResetViewport();
	RenderCursorSceneryFull();
	m_Interface.Render();
	GetCamera()->PrintInfo(CameraCtrlExp());
	RenderFrame(1);
	RenderNetworkInterface();
	RenderDialog();
//	CDragContainer::Render();
	CPopMenu::RenderAll();
	g_Cursor.ScanInput();
	g_Cursor.Render();
}
