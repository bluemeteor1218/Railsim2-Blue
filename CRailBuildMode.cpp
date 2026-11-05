#include "stdafx.h"
#include "CPopMenu.h"
#include "CScene.h"
#include "CRailPlanCurve.h"
#include "CRailDetectCurve.h"
#include "CRailBuilder.h"
#include "CLine.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CRailwayPluginSet.h"
#include "CSurfacePlugin.h"
#include "CRailBuildMode.h"

//	外部定数
extern const float CORNER_MIN_DIST;

//	内部グローバル
bool g_EnableCant;

/*
 *	コンストラクタ
 */
CRailBuildMode::CRailBuildMode(){
	m_Builder = m_LastPos = NULL;
	ListRailwayPluginSet();
}

/*
 *	デストラクタ
 */
CRailBuildMode::~CRailBuildMode(){
	DELETE_V(m_Builder);
	DELETE_V(g_RailwayPluginSetMenu);
}

/*
 *	設定読込
 */
char *CRailBuildMode::LoadArrowScenerySetting(
	char *str	//	対象文字列
){
	if(!str) return NULL;
	return LoadRailwaySetting(str);
}

/*
 *	設定保存
 */
void CRailBuildMode::SaveArrowScenerySetting(
	FILE *file	//	ファイル
){
	SaveRailwaySetting(file);
}

/*
 *	モードを有効化
 */
void CRailBuildMode::EnterArrowScenery(){
	ms_ModeLabel = lang(BuildRail);
	m_Interface.SetChild(&ms_RailWindow);
	m_Interface.SetChild(&ms_OptionWindow);
	CRailDetectCurve3D::ResetDetect();
	EnterRailway(1);
}

/*
 *	モーダル処理
 */
void CRailBuildMode::ModalFuncArrowScenery(){
	ModalFuncRailwayPluginSet();
}

/*
 *	入力チェック
 */
void CRailBuildMode::ScanInputArrowScenery(){
	ScanInputRailway();
	if(m_ArrowMode){
		if(!m_Builder){
			m_ArrowMode = false;
			return;
		}
		g_Scene->ClipAlt(&m_SnapPos, NULL, NULL, GetAltMask());
		if(GetKey(DIK_SPACE)>=S_PUSH) CRailDetectCurve3D::ResetDetect();
		else g_Scene->ScanInputRailWay(0, m_SnapPos, V3ZERO, true);
		if(CRailDetectCurve3D::IsDetected()){
			if(!m_LastPos->SetLink(R2L(CRailLinkTemp(CRailDetectCurve3D::GetDetect())))){
				CRailDetectCurve3D::ResetDetect();
				//Surface()->ClipAlt(&m_SnapPos, NULL, NULL, GetAltMask());
				m_SnapPos = m_LastPos->SetPos(m_SnapPos, 0);
			}
		}else{
			m_LastPos->SetLink(R2L(CRailLinkTemp()));
			m_SnapPos = m_LastPos->SetPos(m_SnapPos, GetAltMask());
		}
		//if(m_Interface.ScanInput()) break;
		switch(GetCamera()->ScanInput(2)){
		case 10: {
ADD:
			CRailBuilder *prev = m_LastPos->GetPrev(), *fin = prev;
			if(prev && m_LastPos->IsLinkFilled()){
				fin = m_LastPos;
				goto BUILD;
			}
			if(prev && prev->IsLast()){
				if(prev!=m_Builder){
BUILD:
					void PushUndoStack();
					PushUndoStack();
					int i, pinum = 0, ponum = 0;
					float pipos = 0.0f, pisum = 0.0f, popos = 0.0f, posum = 0.0f;
					CRailPlugin *b_rail;
					CTiePlugin *b_tie;
					CGirderPlugin *b_girder;
					CPierPlugin *b_pier;
					CLinePlugin *b_line;
					CPolePlugin *b_pole;
					GetPlugin(&b_rail, &b_tie, &b_girder, &b_pier, &b_line, &b_pole);
					g_EnableCant = IsCantEnabled();
					CRailBuilder::ResetDirSum();
					g_LastPole.resize(ms_TrackNum);
					g_FinishPole.resize(ms_TrackNum);
					g_MultiTrackDummy = false;
					for(i = 0; i<ms_TrackNum; i++)
						g_LastPole[i] = g_FinishPole[i] = CPoleLink();
					for(i = 0; i<=ms_TrackNum; i++){
						if(i==ms_TrackNum){
							if(!(b_girder && b_girder->IsMultiTrack())
								&& !(b_pier && b_pier->IsMultiTrack())
								&& !(b_pole && b_pole->IsMultiTrack())) break;
							g_MultiTrackDummy = true;
							g_DummyTrackNum = ms_TrackNum;
							g_DummyTrackInterval = ms_TrackInterval;
						}
						CRailBuilder::SetTrack(i, ms_TrackNum, ms_TrackInterval, IsLiftRail());
						CRailConnectorLink con1, con2;
						CRailLinkTemp link1 = m_Builder->CheckLink()
							? m_Builder->GetLink() : CRailLinkTemp();
						CRailLinkTemp link2 = fin->CheckLink()
							? fin->GetLink() : CRailLinkTemp();
						CRailWay *lway1, *lway2;
						if(link1.m_Link) con1 = link1.SplitLink(&lway1);
						if(link2.m_Link) con2 = link2.SplitLink(&lway2);
						if(b_rail) b_rail->ResetMapTemp();
						if(b_tie) b_tie->ResetMapTemp();
						if(b_girder) b_girder->ResetMapTemp();
						if(b_pier) b_pier->ResetPierPos();
						if(b_line){
							b_line->ResetMapTemp();
							b_line->ResetPolePos();
						}
						if(i<ms_TrackNum){
							if(link1.m_Link){
								lway1->CopyMapTemp(link1.m_Side);
								lway1->CopyMapTempMulti(link1.m_Side,
									&pipos, &pisum, &pinum, &popos, &posum, &ponum);
								g_LastPole[i] = lway1->GetPoleLink(link1.m_Side);
							}
							if(link2.m_Link)
								g_FinishPole[i] = lway2->GetPoleLink(link2.m_Side);
						}else{
							if(pinum){
								pipos /= pinum;
								pisum /= pinum;
							}
							if(ponum){
								popos /= ponum;
								posum /= ponum;
							}
							if(b_pier){
								b_pier->SetPierPos(pipos);
								if(pinum) b_pier->AddPierPos(pisum);
							}
							if(b_line){
								b_line->SetPolePos(popos);
								if(ponum) b_line->AddPolePos(posum);
							}
						}
						m_Builder->BuildRail(con1, con2, b_rail, b_tie, b_girder);
						g_Scene->BuildLine(b_pier, b_line, b_pole);
						g_MultiTrackDummy = false;
					}
					g_Scene->Dump();
					DELETE_V(m_Builder);
					m_Builder = m_LastPos = new CRailBuilder(m_SnapPos, NULL);
					//	dump g_MultiTrackRailList
					/*chdir(g_BaseDir);
					FILE *file = fopen("MultiTrackRailList.txt", "wt");
					if(file){
						ILLPRailWay illpr = g_MultiTrackRailList.begin();
						for(; illpr!=g_MultiTrackRailList.end(); illpr++){
							fprintf(file, "list<list<CRailWay *> >{\n");
							ILPRailWay ilpr = illpr->begin();
							for(; ilpr!=illpr->end(); ilpr++){
								fprintf(file, "\tlist<CRailWay *>{\n");
								IPRailWay ipr = ilpr->begin();
								for(; ipr!=ilpr->end(); ipr++){
									fprintf(file, "\t\t%p\n\t\t", *ipr);
									IPolePos ippo = (*ipr)->m_PoleList.begin();
									for(; ippo!=(*ipr)->m_PoleList.end(); ippo++)
										fprintf(file, "%.1f ", ippo->m_Pos);
									fprintf(file, "\n");
								}
								fprintf(file, "\t}\n");
							}
							fprintf(file, "}\n");
						}
						fclose(file);
					}*/
				}
			}else{
				if(m_LastPos->IsLinkEmpty() || m_LastPos->IsLinkFilled())
					m_LastPos = new CRailBuilder(m_SnapPos, m_LastPos);
				else m_LastPos->PushLink();
			}
			break; }
		case 32:
POP:
			if(!(m_LastPos = m_LastPos->Pop())) m_Builder = NULL;
			break;
		case 0:
			if(GetKey(DIK_ESCAPE)==S_PUSH){
				m_ArrowMode = false;
			}else if(GetKey(DIK_RETURN)==S_PUSH){
				goto ADD;
			}else if(GetKey(DIK_BACK)==S_PUSH){
				goto POP;
			}else if(GetKey(DIK_DELETE)==S_PUSH){
				if(m_Builder!=m_LastPos){
					DELETE_V(m_Builder);
					m_Builder = m_LastPos = new CRailBuilder(m_SnapPos, NULL);
				}else{
					DELETE_V(m_Builder);
					m_LastPos = NULL;
				}
			}
			break;
		}
	}else{
		CRailDetectCurve3D::ResetDetect();
		if(m_Interface.ScanInput()) return;
		if(ms_RailWindow.CheckClose()){
			SetNeutral();
			return;
		}
		switch(GetCamera()->ScanInput(1)){
		case 12:
			if(!g_NetworkInitialized){
				m_ArrowMode = true;
				g_Cursor.Center();
				if(!m_Builder) m_Builder = m_LastPos = new CRailBuilder(m_SnapPos, NULL);
				ReformEditValue();
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
 *	レンダリング
 */
void CRailBuildMode::RenderArrowScenery(){
	if(ms_PhotoMode) return;
	ms_RailWindow.SetColor(m_ArrowMode ? 0x80ffffff : 0xffffffff);
	if(m_ArrowMode) CRailDetectCurve3D::RenderLink();
	DrawGrid();
	if(m_Builder){
		CRailPlugin *b_rail = ms_Type[0].GetCheck() ? g_Rail : NULL;
		CTiePlugin *b_tie = ms_Type[1].GetCheck() ? g_Tie : NULL;
		CGirderPlugin *b_girder = ms_Type[2].GetCheck() ? g_Girder : NULL;
		CLineDumpL dump(256);
		int i;
		CRailBuilder::ResetDirSum();
		for(i = 0; i<ms_TrackNum; i++){
			CRailBuilder::SetTrack(i, ms_TrackNum, ms_TrackInterval, IsLiftRail());
			m_Builder->Render(&dump, b_rail, b_tie, b_girder, i==ms_TrackNum-1);
		}
	}
	if(m_ArrowMode){
		char *err, *errs[3] = {
			NULL, lang(MismatchedTrackNum), lang(MismatchedTrackInterval)};
		CRailPlugin *b_rail;
		CTiePlugin *b_tie;
		CGirderPlugin *b_girder;
		CPierPlugin *b_pier;
		CLinePlugin *b_line;
		CPolePlugin *b_pole;
		GetPlugin(&b_rail, &b_tie, &b_girder, &b_pier, &b_line, &b_pole);
		if(b_girder && (err = errs[b_girder->ConfirmMultiTrack(ms_TrackNum, ms_TrackInterval)]))
			g_StrTex->RenderLeft(TILE_QUAD, TILE_UNIT*5+TILE_QUAD, 0xffffff00, 0xff000000,
				FlashIn("%s %s", lang(Caution_Girder), err));
		if(b_pier && (err = errs[b_pier->ConfirmMultiTrack(ms_TrackNum, ms_TrackInterval)]))
			g_StrTex->RenderLeft(TILE_QUAD, TILE_UNIT*6+TILE_QUAD, 0xffffff00, 0xff000000,
				FlashIn("%s %s", lang(Caution_Pier), err));
		if(b_pole && (err = errs[b_pole->ConfirmMultiTrack(ms_TrackNum, ms_TrackInterval)]))
			g_StrTex->RenderLeft(TILE_QUAD, TILE_UNIT*7+TILE_QUAD, 0xffffff00, 0xff000000,
				FlashIn("%s %s", lang(Caution_Pole), err));
	}
}

/*
 *	ビルダーリセット
 */
void CRailBuildMode::ResetBuilder(){
	DELETE_V(m_Builder);
}
