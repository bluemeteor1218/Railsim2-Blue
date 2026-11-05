#include "stdafx.h"
#include "CPopMenu.h"
#include "CSimpleDialog.h"
#include "CPier.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailDetectCurve.h"
#include "CRailEditMode.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CSkinPlugin.h"

//	関数宣言
void PushUndoStack();

//	外部定数
extern const int CSR_MOVE_TSD;

//	外部グローバル
extern CLinePlugin *g_Line;

//	内部グローバル
bool g_ShowRailSelect = false;
bool g_ShowRailWaySelect = false;
bool g_ShowRailBlockSelect = false;
bool g_ShowSpeedLimitSelect = false;
bool g_ShowPierSelect = false;
bool g_ShowPoleSelect = false;
bool g_ShowLineSelect = false;
bool g_ShowWarpSelect = false;
int g_RailEditModalState;					//	モーダル状態
string g_RailEditTempString;				//	テンポラリ文字列
CYesNoDialog *g_RailEditYesNoDialog = NULL;	//	問合せダイアログ
CInputDialog *g_RailEditInputDialog = NULL;	//	入力ダイアログ

/*
 *	レール選択表示状態リセット
 */
void ResetShowRailSelect(){
	g_ShowRailSelect = false;
	g_ShowRailWaySelect = false;
	g_ShowRailBlockSelect = false;
	g_ShowSpeedLimitSelect = false;
	g_ShowPierSelect = false;
	g_ShowPoleSelect = false;
	g_ShowLineSelect = false;
	g_ShowWarpSelect = false;
}

/*
 *	コンストラクタ
 */
CRailEditMode::CRailEditMode(){
	int i, gww = TILE_UNIT*14, gwh = TILE_UNIT*(3+6);
	m_EditWindow.Init(TILE_UNIT, g_DispHeight-gwh-TILE_UNIT,
		gww, gwh, lang(EditOption), &m_Interface, true);
	m_ModeLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT,
		gww-TILE_UNIT, TILE_UNIT, lang(EditMode_UpDn), &m_EditWindow, 0, 1);
	char *modelabel[RAIL_EDIT_MODES] = {
		lang(SelectEditRail), lang(SelectAddPier), lang(SelectEditPier),
		lang(SelectAddPole), lang(SelectEditPole), lang(SelectEditLine), lang(ConnectLine),
		lang(SelectWarp), lang(ConnectWarp), lang(RailBlock), lang(SpeedLimit)};
	for(i = 0; i<RAIL_EDIT_MODES; i++) m_Mode[i].Init(
		TILE_HALF+(i/6)*((gww-TILE_UNIT-TILE_QUAD)/2+TILE_QUAD),
		TILE_QUAD+TILE_UNIT*(2+i%6), (gww-TILE_UNIT-TILE_QUAD)/2, TILE_UNIT,
		modelabel[i], &m_EditWindow, i ? &m_Mode[i-1] : NULL);
	m_EditMode = EM_EDIT_RAIL;
	m_Mode[m_EditMode].SetCheck();

	class CRailBlockSetter: public CMenuCommand{
	public:
		void Exec(){
			g_RailEditModalState = 10;
			g_ModalDialog = g_RailEditInputDialog = new CInputDialog(
				lang(RailBlockNameInquireMessage), lang(SetRailBlock), "", 64);
		}
	};
	class CRailBlockRemover: public CMenuCommand{
	public:
		void Exec(){
			PushUndoStack();
			g_Scene->SetRailBlock("");
			g_Scene->Dump();
		}
	};
	m_RailBlockMenu = new CPopMenu("", NULL);
	(new CPopMenu(lang(SetRailBlock), m_RailBlockMenu))->SetCommand(new CRailBlockSetter);
	(new CPopMenu(lang(DeleteRailBlock), m_RailBlockMenu))->SetCommand(new CRailBlockRemover);

	class CSpeedLimitSetter: public CMenuCommand{
	public:
		void Exec(){
			g_RailEditModalState = 20;
			g_ModalDialog = g_RailEditInputDialog = new CInputDialog(
				lang(SpeedLimitInquireMessage), lang(SetSpeedLimit), "", 64);
		}
	};
	class CSpeedLimitRemover: public CMenuCommand{
	public:
		void Exec(){
			PushUndoStack();
			g_Scene->SetSpeedLimit(-1);
			g_Scene->Dump();
		}
	};
	m_SpeedLimitMenu = new CPopMenu("", NULL);
	(new CPopMenu(lang(SetSpeedLimit), m_SpeedLimitMenu))->SetCommand(new CSpeedLimitSetter);
	(new CPopMenu(lang(DeleteSpeedLimit), m_SpeedLimitMenu))->SetCommand(new CSpeedLimitRemover);
}

/*
 *	デストラクタ
 */
CRailEditMode::~CRailEditMode(){
	DELETE_V(m_RailBlockMenu);
	DELETE_V(m_SpeedLimitMenu);
}

/*
 *	モードを有効化
 */
void CRailEditMode::EnterCursorScenery(){
	ms_ModeLabel = lang(EditRail);
	g_Scene->ScanInputRailWay(4, V3ZERO, V3ZERO, true);
	g_Scene->ScanInputPier(4, V3ZERO, V3ZERO);
	g_Scene->ScanInputLine(4, V3ZERO, V3ZERO);
	g_Scene->ScanInputPole(4, V3ZERO, V3ZERO);
	m_LineLinkFrom.m_Link = NULL;
	m_WarpLinkFrom.m_Link = NULL;
	m_DragState = 0;
}

/*
 *	モーダル処理
 */
void CRailEditMode::ModalFuncCursorScenery(){
	bool changed = false;
	if(g_RailEditInputDialog){
		if(g_RailEditInputDialog->CheckOK()){
			g_RailEditTempString = g_RailEditInputDialog->GetInputText();
			DELETE_V(g_ModalDialog);
			g_RailEditInputDialog = NULL;
			switch(g_RailEditModalState){
			case 10:	//	set rail block
				PushUndoStack();
				changed |= g_Scene->SetRailBlock((char *)g_RailEditTempString.c_str());
				break;
			case 20:	//	set rail block
				PushUndoStack();
				int speed_limit = -1;
				sscanf((char *)g_RailEditTempString.c_str(), "%d", &speed_limit);
				if(speed_limit<0) speed_limit = -1;
				changed |= g_Scene->SetSpeedLimit(speed_limit);
				break;
			}
		}else if(g_RailEditInputDialog->CheckCancel()){
			DELETE_V(g_ModalDialog);
			g_RailEditInputDialog = NULL;
		}
	}
	if(changed) g_Scene->Dump();
}

/*
 *	入力チェック
 */
void CRailEditMode::ScanInputCursorScenery(){
	POINT pos = g_Cursor.GetPos();
	m_EditWindow.SetAutoTransparent();
	if(m_Interface.ScanInput()) return;
	if(m_EditWindow.CheckClose()){
		SetNeutral();
		return;
	}
	if(GetKey(DIK_UP)==S_PUSH){
		int tmp = m_Mode->GetNumber()-1;
		if(tmp<0) tmp = RAIL_EDIT_MODES-1;
		m_Mode[tmp].SetCheck();
	}
	if(GetKey(DIK_DOWN)==S_PUSH){
		int tmp = m_Mode->GetNumber()+1;
		if(tmp>=RAIL_EDIT_MODES) tmp = 0;
		m_Mode[tmp].SetCheck();
	}
	EditMode editmode = (EditMode)m_Mode->GetNumber();
	ResetShowRailSelect();
	switch(m_EditMode){
	case EM_ADD_PIER:
	case EM_ADD_POLE:
		g_ShowRailSelect = true;
		g_ShowRailWaySelect = true;
		break;
	case EM_EDIT_RAIL:
	case EM_EDIT_RAIL_BLOCK:
	case EM_EDIT_SPEED_LIMIT:
		if(m_EditMode==EM_EDIT_RAIL_BLOCK) g_ShowRailBlockSelect = !CheckAlt();
		if(m_EditMode==EM_EDIT_SPEED_LIMIT) g_ShowSpeedLimitSelect = !CheckAlt();
		g_ShowRailSelect = true;
		g_ShowRailWaySelect = CheckAlt();
		break;
	case EM_EDIT_PIER: g_ShowPierSelect = true; break;
	case EM_EDIT_POLE: g_ShowPoleSelect = true; break;
	case EM_EDIT_LINE: g_ShowLineSelect = true; break;
	case EM_EDIT_WARP: g_ShowWarpSelect = true; break;
	}
	if(g_NetworkInitialized){
		GetCamera()->ScanInput(1);
		return;
	}
	if(editmode!=m_EditMode){
		m_EditMode = editmode;
		g_Scene->ScanInputRailWay(4, V3ZERO, V3ZERO, true);
		g_Scene->ScanInputPier(4, V3ZERO, V3ZERO);
		g_Scene->ScanInputLine(4, V3ZERO, V3ZERO);
		g_Scene->ScanInputPole(4, V3ZERO, V3ZERO);
	}
	if(CheckAlt()){
		switch(m_EditMode){
		case EM_EDIT_RAIL:
		case EM_EDIT_RAIL_BLOCK:
		case EM_EDIT_SPEED_LIMIT:
			g_Scene->ScanInputRailWay(1, g_Cursor.GetVEC3(), V3ZERO, m_EditMode==EM_EDIT_RAIL);
			switch(GetCamera()->ScanInput(1)){
			case 12:
				if(CRailDetectCurve2D::IsDetected()){
					PushUndoStack();
					CRailDetectCurve2D::GetDetect().SplitLink(NULL);
					g_Scene->Dump();
				}
				break;
			}
			break;
		}
	}else if(m_EditMode==EM_ADD_PIER || m_EditMode==EM_ADD_POLE){
		g_Scene->ScanInputRailWay(1, g_Cursor.GetVEC3(), V3ZERO, true);
		switch(GetCamera()->ScanInput(1)){
		case 12:
			if(CRailDetectCurve2D::IsDetected()){
				PushUndoStack();
				CRailLinkTemp &detect = CRailDetectCurve2D::GetDetect();
				VEC3 tpos = detect.m_Pos, tright = detect.m_Right, tup = detect.m_Up, tdir = detect.m_Dir;
				if(!detect.m_Side) tright=-tright;
				float ofs = detect.m_Side ? detect.m_Link->GetSegLen()-detect.m_SumLen : detect.m_SumLen;
				if(m_EditMode==EM_ADD_PIER && g_Pier){
					float pickalt = tpos.y;
					if(g_Rail) g_Rail->CalcPierPos(&tpos, &tright, &tup, &tdir);
					if(g_Tie) g_Tie->CalcPierPos(&tpos, &tright, &tup, &tdir);
					if(g_Girder) g_Girder->CalcPierPos(&tpos, &tright, &tup, &tdir);
					CPier *pier = new CPier(tpos, tdir, tup, pickalt, g_Pier);
					if(pier->Confirm()) detect.m_Link->InsertPier(ofs, pier);
				}else if(m_EditMode==EM_ADD_POLE && g_Pole){
					CPole *pole = new CPole(tpos+g_Line->GetTrolleyAlt()*tup
						+g_Line->GetHeight()*V3UP, tdir, g_Pole);
					detect.m_Link->InsertPole(ofs, pole, 0, false);
				}
				g_Scene->Dump();
			}
			break;
		}
	}else{
		switch(m_EditMode){
		case EM_CONNECT_LINE:
			g_Scene->ScanInputPole(1, g_Cursor.GetVEC3(), V3ZERO);
			break;
		case EM_CONNECT_WARP:
			g_Scene->ScanInputRailWay(5, g_Cursor.GetVEC3(), V3ZERO, false);
			break;
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
				switch(m_EditMode){
				case EM_EDIT_RAIL:
				case EM_EDIT_RAIL_BLOCK:
				case EM_EDIT_SPEED_LIMIT:
					g_Scene->ScanInputRailWay(2, m_DragBegin, m_DragEnd, m_EditMode==EM_EDIT_RAIL);
					break;
				case EM_EDIT_PIER:
					g_Scene->ScanInputPier(2, m_DragBegin, m_DragEnd);
					break;
				case EM_EDIT_POLE:
					g_Scene->ScanInputPole(2, m_DragBegin, m_DragEnd);
					break;
				case EM_EDIT_LINE:
					g_Scene->ScanInputLine(2, m_DragBegin, m_DragEnd);
					break;
				case EM_EDIT_WARP:
					g_SaveFile->ScanInputWarp(2, m_DragBegin, m_DragEnd);
					break;
				}
				break;
			}
			break;
		case 32:
			switch(m_EditMode){
			case EM_CONNECT_LINE:
				m_LineLinkFrom.m_Link = NULL;
				break;
			case EM_CONNECT_WARP:
				m_WarpLinkFrom.m_Link = NULL;
				break;
			case EM_EDIT_RAIL_BLOCK:
				m_RailBlockMenu->Popup(pos.x, pos.y);
				break;
			case EM_EDIT_SPEED_LIMIT:
				m_SpeedLimitMenu->Popup(pos.x, pos.y);
				break;
			}
			break;
		default:
			switch(m_DragState){
			case 0:
				switch(m_EditMode){
				case EM_EDIT_RAIL:
				case EM_EDIT_RAIL_BLOCK:
				case EM_EDIT_SPEED_LIMIT:
					g_Scene->ScanInputRailWay(4, g_Cursor.GetVEC3(), V3ZERO, m_EditMode==EM_EDIT_RAIL);
					if(CRailDetectCurve2D::IsDetected())
						CRailDetectCurve2D::GetDetect().m_SpliceItr->m_Selected |= 1;
					break;
				case EM_EDIT_PIER:
					g_Scene->ScanInputPier(4, g_Cursor.GetVEC3(), V3ZERO);
					if(CPier::IsDetected()) CPier::GetDetect()->AddSelectFlag(1);
					break;
				case EM_EDIT_POLE:
					g_Scene->ScanInputPole(4, g_Cursor.GetVEC3(), V3ZERO);
					if(CPole::IsDetected()) CPole::GetDetect().m_Link->AddSelectFlag(1);
					break;
				case EM_EDIT_LINE:
					g_Scene->ScanInputLine(4, g_Cursor.GetVEC3(), V3ZERO);
					if(CLine::IsDetected()) CLine::GetDetect()->AddSelectFlag(1);
					break;
				case EM_EDIT_WARP:
					g_SaveFile->ScanInputWarp(4, g_Cursor.GetVEC3(), V3ZERO);
					if(CRailWay::IsDetected())
						CRailWay::GetDetect().m_Link->AddSelectFlag(1);
					break;
				}
				break;
			case 1:
				switch(m_EditMode){
				case EM_EDIT_RAIL:
				case EM_EDIT_RAIL_BLOCK:
				case EM_EDIT_SPEED_LIMIT:
					g_Scene->ScanInputRailWay(4, m_DragBegin, V3ZERO, m_EditMode==EM_EDIT_RAIL);
					if(CRailDetectCurve2D::IsDetected())
						CRailDetectCurve2D::GetDetect().m_SpliceItr->m_Selected |= 1;
					break;
				case EM_EDIT_PIER:
					g_Scene->ScanInputPier(4, m_DragBegin, V3ZERO);
					if(CPier::IsDetected()) CPier::GetDetect()->AddSelectFlag(1);
					break;
				case EM_EDIT_POLE:
					g_Scene->ScanInputPole(4, m_DragBegin, V3ZERO);
					if(CPole::IsDetected()) CPole::GetDetect().m_Link->AddSelectFlag(1);
					break;
				case EM_EDIT_LINE:
					g_Scene->ScanInputLine(4, m_DragBegin, V3ZERO);
					if(CLine::IsDetected()) CLine::GetDetect()->AddSelectFlag(1);
					break;
				case EM_CONNECT_LINE:
					if(CPole::IsDetected()){
						if(m_LineLinkFrom.m_Link && g_Line){
							CPoleLink linkto = CPole::GetDetect();
							VEC3 dir = linkto.GetPos()-m_LineLinkFrom.GetPos();
							if(m_LineLinkFrom.m_Link==linkto.m_Link
								&& m_LineLinkFrom.m_Track==linkto.m_Track){
								EnqueueCommonDialog(new CSimpleDialog(
									lang(SameSrcAndDest), lang(Error)));
								g_Skin->Error();
							}else if(g_Scene->FindLine(&m_LineLinkFrom, &linkto)){
								EnqueueCommonDialog(new CSimpleDialog(
									lang(LineAlreadyExists), lang(Error)));
								g_Skin->Error();
							}else{
								PushUndoStack();
								m_LineLinkFrom.m_Side =
									V3Dot(&dir, &m_LineLinkFrom.GetDir())>0.0f;
								linkto.m_Side = V3Dot(&dir, &linkto.GetDir())<0.0f;
								new CLine(m_LineLinkFrom, linkto, g_Line);
								m_LineLinkFrom.m_Link = NULL;
								g_Scene->Dump();
							}
						}else{
							m_LineLinkFrom = CPole::GetDetect();
						}
					}
					break;
				case EM_EDIT_WARP:
					g_SaveFile->ScanInputWarp(4, m_DragBegin, V3ZERO);
					if(CRailWay::IsDetected())
						CRailWay::GetDetect().m_Link->AddSelectFlag(1);
					break;
				case EM_CONNECT_WARP:
					if(CRailDetectCurve2D::IsDetected()){
						if(m_WarpLinkFrom.m_Link){
							CRailLinkTemp linkto = CRailDetectCurve2D::GetDetect();
							if(m_WarpLinkFrom.m_Link==linkto.m_Link
								&& m_WarpLinkFrom.m_Side==linkto.m_Side){
								EnqueueCommonDialog(new CSimpleDialog(
									lang(SameSrcAndDest), lang(Error)));
								g_Skin->Error();
							}else{
								PushUndoStack();
								g_SaveFile->SetWarpRoot();
								CRailWay *warp = new CRailWay(
									R2L(m_WarpLinkFrom.SplitLink(NULL)),
									R2L(linkto.SplitLink(NULL)), NULL, NULL, NULL);
								warp->SetWarpDummy();
								m_WarpLinkFrom.m_Link = NULL;
								g_Scene->ResetRailWayRoot();
							}
						}else{
							m_WarpLinkFrom = CRailDetectCurve2D::GetDetect();
						}
					}
					break;
				}
			case 2:
				switch(m_EditMode){
				case EM_EDIT_RAIL:
				case EM_EDIT_RAIL_BLOCK:
				case EM_EDIT_SPEED_LIMIT:
					g_Scene->ScanInputRailWay(3, V3ZERO, V3ZERO, m_EditMode==EM_EDIT_RAIL);
					break;
				case EM_EDIT_PIER:
					g_Scene->ScanInputPier(3, V3ZERO, V3ZERO);
					break;
				case EM_EDIT_POLE:
					g_Scene->ScanInputPole(3, V3ZERO, V3ZERO);
					break;
				case EM_EDIT_LINE:
					g_Scene->ScanInputLine(3, V3ZERO, V3ZERO);
					break;
				case EM_EDIT_WARP:
					g_SaveFile->ScanInputWarp(3, V3ZERO, V3ZERO);
					break;
				}
				break;
			}
			m_DragState = 0;
			if(GetKey(DIK_DELETE)==S_PUSH){
				PushUndoStack();
				bool deleted = false;
				switch(m_EditMode){
				case EM_EDIT_RAIL: deleted |= g_Scene->DeleteRailWay(); break;
				case EM_EDIT_PIER: deleted |= g_Scene->DeletePier(NULL); break;
				case EM_EDIT_POLE: deleted |= g_Scene->DeletePole(NULL); break;
				case EM_EDIT_LINE: deleted |= g_Scene->DeleteLine(NULL); break;
				case EM_EDIT_WARP: deleted |= g_SaveFile->DeleteWarp(); break;
				}
				if(deleted) g_Scene->Dump();
			}
			break;
		}
	}
}

/*
 *	レンダリング
 */
void CRailEditMode::RenderCursorScenery(){
	if(ms_PhotoMode) return;
	if(g_NetworkInitialized){
		devResetMatrix();
		devSetLighting(FALSE);
		g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight/3,
			ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
			lang(CannotEditInNetMode));
	}else{
		switch(m_EditMode){
		case EM_EDIT_RAIL:
			if(CheckAlt()) CRailDetectCurve2D::RenderLink();
			break;
		case EM_ADD_PIER:
		case EM_ADD_POLE:
			CRailDetectCurve2D::RenderLink();
			break;
		case EM_CONNECT_LINE:
			CPole::RenderLink();
			if(m_LineLinkFrom.m_Link){
				m_LineLinkFrom.Render(0xffffff00);
				if(CPole::IsDetected()) Draw3DLineWithShadow(
					m_LineLinkFrom.GetPos(), CPole::GetDetect().GetPos(), 0xffffff00);
			}
			break;
		case EM_CONNECT_WARP:
			if(CRailDetectCurve2D::IsDetected()){
				CRailDetectCurve2D::RenderLink();
				devResetMatrix();
				devSetLighting(FALSE);
				devSetTexture(0, NULL);
	 			Draw3DPointAs2DRect(CRailDetectCurve2D::GetDetect().m_Pos, 0xff00ffff, 5);
			}
			if(m_WarpLinkFrom.m_Link && m_WarpLinkFrom.m_Link->GetScene()==g_Scene){
				devSetLighting(TRUE);
				g_LinkObject.SetPos(m_WarpLinkFrom.m_Pos);
				g_LinkObject.SetDir(m_WarpLinkFrom.m_Dir, m_WarpLinkFrom.m_Up);
				g_LinkObject.Render();
				devResetMatrix();
				devSetLighting(FALSE);
				devSetTexture(0, NULL);
				Draw3DPointAs2DRect(m_WarpLinkFrom.m_Pos, 0xffffff00, 5);
				if(CRailDetectCurve2D::IsDetected()){
					CRailLinkTemp linkto = CRailDetectCurve2D::GetDetect();
					if(m_WarpLinkFrom.m_Link->GetScene()==linkto.m_Link->GetScene()
						&& m_WarpLinkFrom.m_Link->GetScene()==g_Scene){
						devSetTexture(0, NULL);
						Draw3DLineWithShadow(m_WarpLinkFrom.m_Pos, linkto.m_Pos, 0xffffff00);
					}
				}
			}
			devSetLighting(FALSE);
			break;
		case EM_EDIT_RAIL_BLOCK:
			if(CheckAlt()) CRailDetectCurve2D::RenderLink();
			if(CRailDetectCurve2D::IsDetected()){
				CRailWay *blockway = CRailDetectCurve2D::GetDetect().m_Link;
				if(blockway->IsRailBlock()) g_StrTex->RenderLeft(
					TILE_QUAD, TILE_UNIT*2+TILE_QUAD, 0xffffffff, 0xff000000,
					FlashIn("%s: %s", lang(RailBlock), blockway->GetRailBlock().c_str()));
			}
			break;
		case EM_EDIT_SPEED_LIMIT:
			if(CheckAlt()) CRailDetectCurve2D::RenderLink();
			if(CRailDetectCurve2D::IsDetected()){
				CRailWay *limitway = CRailDetectCurve2D::GetDetect().m_Link;
				if(limitway->IsSpeedLimit()) g_StrTex->RenderLeft(
					TILE_QUAD, TILE_UNIT*2+TILE_QUAD, 0xffffffff, 0xff000000,
					FlashIn("%s: %d [km/h]", lang(SpeedLimit), limitway->GetSpeedLimit()));
			}
			break;
		}
		if(m_DragState==2) Draw2DRect(
			m_DragBegin.x, m_DragBegin.y, m_DragEnd.x, m_DragEnd.y, 0xffff0000);
	}
}
