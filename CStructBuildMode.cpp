#include "stdafx.h"
#include "CScene.h"
#include "CStruct.h"
#include "CStructPlugin.h"
#include "CSurfacePlugin.h"
#include "CStructBuildMode.h"

//	外部定数
extern const float CORNER_MIN_DIST;
extern const int ARROW_INTERFACE_HEIGHT;

//	static メンバ
CWindowCtrl CStructBuildMode::ms_StructWindow;
CStaticCtrl CStructBuildMode::ms_DirLabel;
CRadioButton CStructBuildMode::ms_Dir[4];
CCheckBox CStructBuildMode::ms_FitNormal;

/*
 *	[static]
 *	共通インターフェイスの初期化
 */
void CStructBuildMode::InitInterface(){
	CInterface::ResetTabHead();
	int i, sww = TILE_UNIT*14, swh = TILE_UNIT*5, gwh = ARROW_INTERFACE_HEIGHT;
	ms_StructWindow.Init(TILE_UNIT, g_DispHeight-gwh-swh-TILE_UNIT*2,
		sww, swh, lang(StructOption), NULL, true);
	ms_DirLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT,
		sww-TILE_UNIT, TILE_UNIT, lang(AngleSplit), &ms_StructWindow, 0, 1);
	char *dirlabel[4] = {"32", "72", "256", "360"};
	for(i = 0; i<4; i++) ms_Dir[i].Init(
		TILE_HALF+i*(sww-TILE_QUAD*3)/4, TILE_QUAD+TILE_UNIT*2,
		(i+1)*(sww-TILE_QUAD*3)/4-i*(sww-TILE_QUAD*3)/4-TILE_QUAD, TILE_UNIT,
		dirlabel[i], &ms_StructWindow, i ? &ms_Dir[i-1] : NULL);
	ms_Dir[3].SetCheck();
	ms_FitNormal.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*3,
		sww-TILE_UNIT, TILE_UNIT, lang(FitGradient), &ms_StructWindow);
}

/*
 *	[static]
 *	角度分割数取得
 */
int CStructBuildMode::GetAngleSplit(){
	static int anglesplit[4] = {32, 72, 256, 360};
	return anglesplit[ms_Dir->GetNumber()];
}

/*
 *	コンストラクタ
 */
CStructBuildMode::CStructBuildMode(){
	m_HitFlag = false;
	m_BuildAngle = 0;
}

/*
 *	デストラクタ
 */
CStructBuildMode::~CStructBuildMode(){
}

/*
 *	設置方向ベクトル計算
 */
void CStructBuildMode::GetBuildDir(
	VEC3 *dir, VEC3 *up	//	格納先
){
	float theta = 2.0f*D3DX_PI*m_BuildAngle/GetAngleSplit();
	*dir = VEC3(sinf(theta), 0.0f, cosf(theta));
	if(!m_HitFlag || !ms_FitNormal.GetCheck()){
		*up = V3UP;
	}else{
		*up = m_HitNorm;
		VEC3 dummy;
		V3NormAxis(&dummy, dir, up);
	}
}

/*
 *	モードを有効化
 */
void CStructBuildMode::EnterArrowScenery(){
	ms_ModeLabel = lang(BuildStruct);
	m_Interface.SetChild(&ms_StructWindow);
	m_Interface.SetChild(&ms_OptionWindow);
	EnterStructBuild();
}

/*
 *	モードを有効化
 */
void CStructBuildMode::EnterStructBuild(){
	if(g_Struct) g_Struct->SetPreview();
}

/*
 *	入力チェック
 */
void CStructBuildMode::ScanInputArrowScenery(){
	static int rep;
	int s, split = GetAngleSplit(), delta = CheckCtrl() ? split/8 : 1;
	if((s = GetKey(DIK_LEFT))>=S_PUSH){
		if(PickRepeat(&rep, s)) m_BuildAngle -= delta;
		m_BuildAngle = m_BuildAngle/delta*delta;
	}
	if((s = GetKey(DIK_RIGHT))>=S_PUSH){
		if(PickRepeat(&rep, s)) m_BuildAngle += delta;
		m_BuildAngle = m_BuildAngle/delta*delta;
	}
	ValueCircular(&m_BuildAngle, 0, split-1);
	m_HitFlag = g_Scene->ClipAlt(&m_SnapPos, &m_HitPos, &m_HitNorm, GetAltMask());
//	goto BUILD;	//	for debug immediate termination
	if(m_ArrowMode){
		//if(m_Interface.ScanInput()) return;
		switch(GetCamera()->ScanInput(2)){
		case 10:
BUILD:
			void PushUndoStack();
			PushUndoStack();
			Build();
		//	ms_ActiveMode = NULL;	//	for debug immediate termination
			break;
		case 32:
			m_ArrowMode = false;
			break;
		case 0:
			if(GetKey(DIK_ESCAPE)==S_PUSH){
				m_ArrowMode = false;
			}else if(GetKey(DIK_RETURN)==S_PUSH){
				goto BUILD;
			}
			break;
		}
	}else{
		if(m_Interface.ScanInput()) return;
		if(ms_StructWindow.CheckClose()){
			SetNeutral();
			return;
		}
		switch(GetCamera()->ScanInput(1)){
		case 12:
			if(!g_NetworkInitialized){
				m_ArrowMode = true;
				g_Cursor.Center();
			}
			break;
		case 0:
			if(GetKey(DIK_ESCAPE)==S_PUSH){
				SetNeutral();
				break;
			}
			break;
		}
	}
}

/*
 *	建設
 */
void CStructBuildMode::Build(){
	if(g_Struct){
		VEC3 tup, tdir;
		GetBuildDir(&tdir, &tup);
		if(g_Struct) g_Struct->SetTempSwitch();
		new CStruct(g_Struct, m_SnapPos, tdir, tup, true);
	}
}

/*
 *	レンダリング
 */
void CStructBuildMode::RenderArrowScenery(){
	ms_StructWindow.SetColor(m_ArrowMode ? 0x80ffffff : 0xffffffff);
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
	g_RenderBlink = true;
	CNamedObject::InitAfterRenderList();
	CHeadlight::InitRenderList();
	RenderStructBuild();
	CNamedObject::AfterRenderAll();
	CHeadlight::RenderAll();
	g_RenderBlink = false;
	DrawGrid();
	devSetTexture(0, NULL);
	devResetMatrix();
	if(m_HitFlag){
		DrawTangent(m_HitPos, m_HitNorm, 0xffff0000, NULL);
		Draw3DLine(m_SnapPos, m_HitPos, 0xffff0000, 0x80ff0000);
	}
	int split = GetAngleSplit();
	g_StrTex->RenderLeft(
		TILE_QUAD, TILE_UNIT*3+TILE_QUAD, 0xffffffff, 0xff000000,
		split%3 ? FlashIn("%s: %d/%d [rad/2pi]", lang(Angle), m_BuildAngle, split)
		: FlashIn("%s: %d [deg]", lang(Angle), m_BuildAngle*360/split));
}

/*
 *	レンダリング
 */
void CStructBuildMode::RenderStructBuild(){
	if(ms_PhotoMode) return;
	VEC3 tdir, tup;
	GetBuildDir(&tdir, &tup);
	if(g_Struct) g_Struct->SetTempSwitch();
	CStructPlugin::RenderPreview(m_SnapPos, tdir, tup);
}
