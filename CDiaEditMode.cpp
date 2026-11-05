#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailWay.h"
#include "CRailConnector.h"
#include "CStation.h"
#include "CTrainGroup.h"
#include "CDiaEditMode.h"
#include "CSkinPlugin.h"

/*
 *	コンストラクタ
 */
CDiaEditMode::CDiaEditMode(){
	m_PointDialog.Init(&m_Interface);
	m_DiaDialog.Init(&m_Interface);
	m_EditConnector = NULL;
	m_EditStation = NULL;
}

/*
 *	デストラクタ
 */
CDiaEditMode::~CDiaEditMode(){
}

/*
 *	モードを有効化
 */
void CDiaEditMode::EnterCursorScenery(){
	ms_ModeLabel = lang(DiaSetting);
	m_PointDialog.Show(false);
	m_DiaDialog.Show(false);
	m_EditConnector = NULL;
	m_EditStation = NULL;
	g_Scene->ScanInputRailConnector(5, g_Cursor.GetVEC3(), V3ZERO);
	g_Scene->ScanInputStation(5, g_Cursor.GetVEC3(), V3ZERO, true);
}

/*
 *	入力チェック
 */
void CDiaEditMode::ScanInputCursorScenery(){
	if(!m_Interface.ScanInput()){
		g_Scene->ScanInputRailConnector(4, g_Cursor.GetVEC3(), V3ZERO);
		g_Scene->ScanInputStation(4, g_Cursor.GetVEC3(), V3ZERO, true);
		if(CRailConnector::IsDetected() && CModelInst::IsDetected()){
			if(CRailConnector::GetMinDist()<CModelInst::GetMinDist())
				CModelInst::ResetDetect(0, V3ZERO, V3ZERO);
			else CRailConnector::ResetDetect();
		}
		if(CRailConnector::IsDetected()) CRailConnector::GetDetect()->AddSelectFlag(1);
		else if(CModelInst::IsDetected())
			CModelInst::GetDetectInfo().GetModelInst()->AddSelectFlag(1);
		switch(GetCamera()->ScanInput(1)){
		case 12:
			g_SaveFile->ScanInputWarp(4, g_Cursor.GetVEC3(), V3ZERO);
			if(CRailWay::IsDetected()
				&& (!CRailConnector::IsDetected() ||
					CRailWay::GetMinDist()<CRailConnector::GetMinDist())
				&& (!CModelInst::IsDetected() ||
					CRailWay::GetMinDist()<CModelInst::GetMinDist())){
				VEC3 pos = CRailWay::GetDetect().GetPos();
				if(V3Len(&(pos-GetCamera()->GetFocus()))<1.0f){
					CRailWay::GetDetect().WarpToOppositeEnd();
				}else{
					GetCamera()->SetFocus(CRailWay::GetDetect().GetPos());
				}
			}else if(CRailConnector::IsDetected()){
				if(g_ManualControl){
					CRailConnector::GetDetect()->SwitchNetPoint();
				}else{
					m_EditConnector = CRailConnector::GetDetect();
					m_EditStation = NULL;
					g_Scene->ScanInputRailConnector(6, g_Cursor.GetVEC3(), V3ZERO);
					g_Scene->ScanInputStation(5, g_Cursor.GetVEC3(), V3ZERO, true);
					m_DiaDialog.Show(false);
					m_PointDialog.Enter(m_EditConnector->GetPointInst());
					GetCamera()->SetFocus(m_EditConnector->GetPos());
				}
			}else if(CModelInst::IsDetected()){
				if(g_ManualControl){
					g_Skin->Error();
				}else{
					CStation *station = (CStation *)CModelInst::GetDetectInfo().GetModelInst();
					if(station->IsStoppable()){
						m_EditStation = (CStation *)CModelInst::GetDetectInfo().GetModelInst();
						m_EditConnector = NULL;
						g_Scene->ScanInputStation(6, g_Cursor.GetVEC3(), V3ZERO, true);
						g_Scene->ScanInputRailConnector(5, g_Cursor.GetVEC3(), V3ZERO);
						m_PointDialog.Show(false);
						m_DiaDialog.Enter(m_EditStation->GetDiaInst());
						GetCamera()->SetFocus(m_EditStation->GetPos());
					}else{
						EnqueueCommonDialog(new CSimpleDialog(
							lang(NoPlatform), lang(Error)));
						g_Skin->Error();
					}
				}
			}
			break;
		}
	}
	m_PointDialog.ScanInputDia();
	m_DiaDialog.ScanInputDia();
	if(m_PointDialog.CheckClose()){
		m_PointDialog.Show(false);
		m_EditConnector = NULL;
		g_Scene->ScanInputRailConnector(5, g_Cursor.GetVEC3(), V3ZERO);
	}
	if(m_DiaDialog.CheckClose()){
		m_DiaDialog.Show(false);
		m_EditStation = NULL;
		g_Scene->ScanInputStation(5, g_Cursor.GetVEC3(), V3ZERO, true);
	}
}

/*
 *	レンダリング
 */
void CDiaEditMode::RenderCursorScenery(){
	if(ms_PhotoMode) return;
	devResetMatrix();
	devSetLighting(FALSE);
	devSetTexture(0, NULL);
	if(CRailConnector::IsDetected()) CRailConnector::GetDetect()->Render(0, g_ManualControl);
	if(m_EditConnector) m_EditConnector->Render(0, false);
	devSetLighting(FALSE);
}
