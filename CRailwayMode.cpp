#include "stdafx.h"
#include "CPopMenu.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CRailwayPluginSet.h"
#include "CRailwayMode.h"

//	外部定数
extern const int ARROW_INTERFACE_HEIGHT;

//	static メンバ
int CRailwayMode::ms_CurrentType = 0;
int CRailwayMode::ms_WindowPos[2][2];
int CRailwayMode::ms_TrackNum = 1;
float CRailwayMode::ms_TrackInterval = 4.0;
CWindowCtrl CRailwayMode::ms_RailWindow;
CPushButton CRailwayMode::ms_PluginSetButton;
CStaticCtrl CRailwayMode::ms_TypeLabel;
CCheckBox CRailwayMode::ms_Type[6];
CCheckBox CRailwayMode::ms_EnableCant;
CCheckBox CRailwayMode::ms_LiftRailSurface;
CStaticCtrl CRailwayMode::ms_MultiTrackLabel;
CCheckBox CRailwayMode::ms_MultiTrackCheck;
CStaticCtrl CRailwayMode::ms_TrackNumLabel;
CEditCtrl CRailwayMode::ms_TrackNumEdit;
CStaticCtrl CRailwayMode::ms_TrackIntLabel;
CEditCtrl CRailwayMode::ms_TrackIntEdit;
CCamera CRailwayMode::ms_RailwayModeCamera;

/*
 *	[static]
 *	共通インターフェイスの初期化
 */
void CRailwayMode::InitRailwayInterface(){
	CInterface::ResetTabHead();
	int i, sww = TILE_UNIT*14, swh = TILE_UNIT*8, gwh = ARROW_INTERFACE_HEIGHT;
	ms_WindowPos[0][0] = g_DispWidth-sww-TILE_UNIT*3;
	ms_WindowPos[0][1] = g_DispHeight-swh-TILE_UNIT;
	ms_WindowPos[1][0] = TILE_UNIT;
	ms_WindowPos[1][1] = g_DispHeight-gwh-swh-TILE_UNIT*2;
	ms_RailWindow.Init(ms_WindowPos[0][0], ms_WindowPos[0][1],
		sww, swh, lang(RailwayOption), NULL, true);
	ms_PluginSetButton.Init(sww-TILE_UNIT*6-TILE_HALF, TILE_QUAD/2+TILE_UNIT,
		TILE_UNIT*6, TILE_UNIT, lang(LoadSaveSettings), &ms_RailWindow);
	ms_TypeLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT,
		sww-TILE_UNIT, TILE_UNIT, lang(BuildElement), &ms_RailWindow, 0, 1);
	char *typelabel[6] = {
		lang(Rail), lang(Tie), lang(Girder), lang(Pier), lang(Line), lang(Pole)};
	bool setdefault[6] = {true, true, false, true, true, true};
	for(i = 0; i<6; i++){
		ms_Type[i].Init(TILE_HALF+(i%3)*(sww-TILE_QUAD*3)/3, TILE_QUAD+TILE_UNIT*(i/3+2),
			(i+1)*(sww-TILE_QUAD*3)/3-i*(sww-TILE_QUAD*3)/3-TILE_QUAD, TILE_UNIT,
			typelabel[i], &ms_RailWindow);
		if(setdefault[i]) ms_Type[i].SetCheck(1);
	}
	int tw = sww-TILE_UNIT-TILE_HALF;
	ms_EnableCant.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*4,
		tw/2, TILE_UNIT, lang(Cant), &ms_RailWindow);
	ms_LiftRailSurface.Init(tw/2+TILE_HALF, TILE_QUAD+TILE_UNIT*4,
		tw-tw/2, TILE_UNIT, lang(AltOffset), &ms_RailWindow);
	ms_EnableCant.SetCheck(1);
	ms_LiftRailSurface.SetCheck(1);
	ms_MultiTrackLabel.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*5,
		sww-TILE_UNIT, TILE_UNIT, lang(MultiTrackOption), &ms_RailWindow, 0, 1);
	ms_MultiTrackCheck.Init(TILE_HALF, TILE_QUAD+TILE_UNIT*6,
		TILE_UNIT*4, TILE_UNIT, lang(Enabled), &ms_RailWindow);
	ms_TrackNumLabel.Init(TILE_HALF*2+TILE_UNIT*4, TILE_QUAD+TILE_UNIT*6,
		TILE_UNIT*2, TILE_UNIT, lang(TrNum), &ms_RailWindow, 0, 1);
	ms_TrackNumEdit.Init(TILE_HALF*2+TILE_UNIT*6, TILE_QUAD+TILE_UNIT*6,
		TILE_UNIT*2, TILE_UNIT, "2", &ms_RailWindow, 3);
	ms_TrackIntLabel.Init(TILE_HALF+TILE_UNIT*9, TILE_QUAD+TILE_UNIT*6,
		TILE_UNIT*2, TILE_UNIT, lang(Itv), &ms_RailWindow, 0, 1);
	ms_TrackIntEdit.Init(TILE_HALF+TILE_UNIT*11, TILE_QUAD+TILE_UNIT*6,
		TILE_UNIT*2, TILE_UNIT, "4.00", &ms_RailWindow, 6);
	ms_RailwayModeCamera.Init(20.0f, 2.0f, 200.0f, false);
}

/*
 *	[static]
 *	各種プラグイン ON/OFF 取得
 */
void CRailwayMode::GetPlugin(
	CRailPlugin **b_rail,		//	レールプラグイン
	CTiePlugin **b_tie,			//	枕木プラグイン
	CGirderPlugin **b_girder,	//	橋桁プラグイン
	CPierPlugin **b_pier,		//	橋脚プラグイン
	CLinePlugin **b_line,		//	架線プラグイン
	CPolePlugin **b_pole		//	架線柱プラグイン
){
	*b_rail = ms_Type[0].GetCheck() ? g_Rail : NULL;
	*b_tie = ms_Type[1].GetCheck() ? g_Tie : NULL;
	*b_girder = ms_Type[2].GetCheck() ? g_Girder : NULL;
	*b_pier = ms_Type[3].GetCheck() ? g_Pier : NULL;
	*b_line = ms_Type[4].GetCheck() ? g_Line : NULL;
	*b_pole = ms_Type[5].GetCheck() ? g_Pole : NULL;
}

/*
 *	[static]
 *	レール関連オプション設定
 */
void CRailwayMode::SetRailwayOption(
	bool rail, bool tie, bool girder,	//	on/off
	bool pier, bool line, bool pole,	//	on/off
	bool cant,	//	カントフラグ
	bool lift,	//	複線フラグ
	bool multi,	//	複線フラグ
	int tnum,	//	軌道数
	float tint	//	軌道間隔
){
	ms_Type[0].SetCheck(rail); ms_Type[1].SetCheck(tie); ms_Type[2].SetCheck(girder);
	ms_Type[3].SetCheck(pier); ms_Type[4].SetCheck(line); ms_Type[5].SetCheck(pole);
	ms_EnableCant.SetCheck(cant);
	ms_LiftRailSurface.SetCheck(lift);
	ms_MultiTrackCheck.SetCheck(multi);
	ms_TrackNumEdit.SetText(FlashIn("%d", tnum));
	ms_TrackIntEdit.SetText(FlashIn("%.2f", tint));
}

/*
 *	[static]
 *	設定読込
 */
char *CRailwayMode::LoadRailwaySetting(
	char *str	//	対象文字列
){
	char *eee;
	int tnum;
	float tint;
	bool type[6], cant, lift, multi;
	if(!(str = BeginBlock(eee = str, "RailwayMode"))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "RailCheck", &type[0]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "TieCheck", &type[1]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "GirderCheck", &type[2]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "PierCheck", &type[3]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "LineCheck", &type[4]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "PoleCheck", &type[5]))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "EnableCant", &cant))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "LiftRailSurface", &lift))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "MultiTrack", &multi))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "TrackNum", &tnum))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "TrackInterval", &tint))) throw CSynErr(eee);
	SetRailwayOption(type[0], type[1], type[2], type[3], type[4], type[5],
		cant, lift, multi, tnum, tint);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	[static]
 *	設定保存
 */
void CRailwayMode::SaveRailwaySetting(
	FILE *file	//	ファイル
){
	fprintf(file, "RailwayMode{\n");
	fprintf(file, "\tRailCheck = %s;\n", YESNO[ms_Type[0].GetCheck()]);
	fprintf(file, "\tTieCheck = %s;\n", YESNO[ms_Type[1].GetCheck()]);
	fprintf(file, "\tGirderCheck = %s;\n", YESNO[ms_Type[2].GetCheck()]);
	fprintf(file, "\tPierCheck = %s;\n", YESNO[ms_Type[3].GetCheck()]);
	fprintf(file, "\tLineCheck = %s;\n", YESNO[ms_Type[4].GetCheck()]);
	fprintf(file, "\tPoleCheck = %s;\n", YESNO[ms_Type[5].GetCheck()]);
	fprintf(file, "\tEnableCant = %s;\n", YESNO[ms_EnableCant.GetCheck()]);
	fprintf(file, "\tLiftRailSurface = %s;\n", YESNO[ms_LiftRailSurface.GetCheck()]);
	fprintf(file, "\tMultiTrack = %s;\n", YESNO[ms_MultiTrackCheck.GetCheck()]);
	fprintf(file, "\tTrackNum = %d;\n", GetTrackNum());
	fprintf(file, "\tTrackInterval = %f;\n", GetTrackInterval());
	fprintf(file, "}\n\n");
}

/*
 *	[static]
 *	モードを有効化
 */
void CRailwayMode::EnterRailway(
	int type	//	タイプ (0: interface, 1: scenery)
){
	ms_WindowPos[ms_CurrentType][0] = ms_RailWindow.GetPosX();
	ms_WindowPos[ms_CurrentType][1] = ms_RailWindow.GetPosY();
	ms_CurrentType = type;
	ms_RailWindow.SetPos(ms_WindowPos[ms_CurrentType][0], ms_WindowPos[ms_CurrentType][1]);
	ms_RailWindow.SetColor(0xffffffff);
}

/*
 *	[static]
 *	入力チェック
 */
void CRailwayMode::ScanInputRailway(){
	if(ms_PluginSetButton.IsPushed()){
		ms_PluginSetButton.SetPush(false);
		POINT pos = g_Cursor.GetPos();
		g_RailwayPluginSetMenu->Popup(pos.x, pos.y);
	}
}

/*
 *	[static]
 *	入力チェック
 */
void CRailwayMode::ReformEditValue(){
	ms_TrackNum = GetTrackNum();
	ms_TrackNumEdit.SetText(FlashIn("%d", ms_TrackNum));
	ms_TrackInterval = GetTrackInterval();
	ms_TrackIntEdit.SetText(FlashIn("%.2f", ms_TrackInterval));
	if(!ms_MultiTrackCheck.GetCheck()) ms_TrackNum = 1;
}
