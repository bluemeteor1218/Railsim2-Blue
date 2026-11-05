#include "stdafx.h"
#include "CScene.h"
#include "CStructEditMode.h"

//	外部定数
extern const int CSR_MOVE_TSD;

/*
 *	コンストラクタ
 */
CStructEditMode::CStructEditMode(){
}

/*
 *	デストラクタ
 */
CStructEditMode::~CStructEditMode(){
}

/*
 *	モードを有効化
 */
void CStructEditMode::EnterCursorScenery(){
	ms_ModeLabel = lang(EditStruct);
	m_DragState = 0;
	EnterStructEdit();
}

/*
 *	入力チェック
 */
void CStructEditMode::ScanInputCursorScenery(){
	if(m_Interface.ScanInput()) return;
	if(g_NetworkInitialized){
		GetCamera()->ScanInput(1);
		return;
	}
	switch(GetCamera()->ScanInput(3)){
	case 10:
		m_DragState = 1;
		m_DragBegin = g_Cursor.GetVEC3();
		break;
	case 11:
		switch(m_DragState){
		case 1:
			if(V3Len(&(g_Cursor.GetVEC3()-m_DragBegin))<CSR_MOVE_TSD) break;
			m_DragState = 2;
		case 2:
			m_DragEnd = g_Cursor.GetVEC3();
			ScanInputStructEdit(2, m_DragBegin, m_DragEnd);
			break;
		}
		break;
	default:
		switch(m_DragState){
		case 0:
			ScanInputStructEdit(4, g_Cursor.GetVEC3(), V3ZERO);
			if(CModelInst::IsDetected())
				CModelInst::GetDetectInfo().GetModelInst()->AddSelectFlag(1);
			break;
		case 1:
			ScanInputStructEdit(4, m_DragBegin, V3ZERO);
			if(CModelInst::IsDetected())
				CModelInst::GetDetectInfo().GetModelInst()->AddSelectFlag(1);
		case 2:
			ScanInputStructEdit(3, V3ZERO, V3ZERO);
			break;
		}
		m_DragState = 0;
		if(GetKey(DIK_DELETE)==S_PUSH){
			void PushUndoStack();
			PushUndoStack();
			Delete();
		}
		break;
	}
}

/*
 *	入力チェック
 */
void CStructEditMode::ScanInputStructEdit(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	g_Scene->ScanInputStruct(mode, rect1, rect2, true);
}

/*
 *	撤去
 */
void CStructEditMode::Delete(){
	g_Scene->DeleteStruct(NULL);
}

/*
 *	レンダリング
 */
void CStructEditMode::RenderCursorScenery(){
	if(ms_PhotoMode) return;
	if(g_NetworkInitialized){
		devResetMatrix();
		devSetLighting(FALSE);
		g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight/3,
			ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
			lang(CannotEditInNetMode));
	}else{
		if(m_DragState==2) Draw2DRect(
			m_DragBegin.x, m_DragBegin.y, m_DragEnd.x, m_DragEnd.y, 0xffff0000);
	}
}
