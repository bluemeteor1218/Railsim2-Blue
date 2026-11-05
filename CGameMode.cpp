#include "stdafx.h"
#include "HighTimer.h"
#include "Capture.h"
#include "RSPV.h"
#include "CJobTimer.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CToggleIcon.h"
#include "CPopMenu.h"
#include "CDragContainer.h"
#include "CModelSwitch.h"
#include "CSkinPlugin.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CTrainPlugin.h"
#include "CStationPlugin.h"
#include "CSurfacePlugin.h"
#include "CEnvPlugin.h"
#include "CNeutralMode.h"
#include "CRailSelectMode.h"
#include "CTieSelectMode.h"
#include "CGirderSelectMode.h"
#include "CPierSelectMode.h"
#include "CLineSelectMode.h"
#include "CPoleSelectMode.h"
#include "CRailBuildMode.h"
#include "CRailEditMode.h"
#include "CTrainEditMode.h"
#include "CDiaEditMode.h"
#include "CTrainSetMode.h"
#include "CTrainViewMode.h"
#include "CStationSelectMode.h"
#include "CStationBuildMode.h"
#include "CStationEditMode.h"
#include "CSceneEditMode.h"
#include "CEnvEditMode.h"
#include "CSimulationMode.h"
#include "CFileMode.h"
#include "CConfigMode.h"
#include "CSkinSelectMode.h"
#include "CVideoMode.h"
#include "CExitMode.h"

//	関数宣言
void ModelPluginFreeInst();
namespace LanguageResource{
	bool InitLanguage();
}

//	内部定数
const int MODE_LABEL_WIDTH = TILE_UNIT*6;			//	モード表示部幅
const int PANEL_HIDE_FRAME = MAXFPS;				//	パネル表示時間
const int MODE_SUB[MODE_NUM] = {8, 4, 3, 3, 3, 5};	//	サブモード数
const float PANEL_SHOW_RATIO = 0.5f;				//	パネル表示速度
const float PANEL_HIDE_RATIO = 0.1f;				//	パネル隠蔽速度

//	外部グローバル
extern bool g_IgnoreAcceleration;

//	内部グローバル
int g_BlinkCounter = 0;			//	汎用点滅カウンタ (0..MAXFPS-1)
float g_BlinkAlpha = 0.0f;		//	汎用点滅アルファ (0.0..1.0)
bool g_RenderBlink = false;		//	レンダリング点滅フラグ
LONGLONG g_SoundSync = 0;		//	サウンド同期用タイマ
float g_FrameDelta = 0.0f;		//	フレーム時間 [ms]
bool g_CursorLockable = false;	//	カーソルロック開始

CNeutralMode *g_NeutralMode;				//	ニュートラルモード
CRailSelectMode *g_RailSelectMode;			//	レール選択モード
CTieSelectMode *g_TieSelectMode;			//	枕木選択モード
CGirderSelectMode *g_GirderSelectMode;		//	橋桁選択モード
CPierSelectMode *g_PierSelectMode;			//	橋脚選択モード
CLineSelectMode *g_LineSelectMode;			//	架線選択モード
CPoleSelectMode *g_PoleSelectMode;			//	架線柱選択モード
CRailBuildMode *g_RailBuildMode;			//	線路設置モード
CRailEditMode *g_RailEditMode;				//	線路編集モード
CTrainEditMode *g_TrainEditMode;			//	車輌編成モード
CDiaEditMode *g_DiaEditMode;				//	運行設定モード
CTrainSetMode *g_TrainSetMode;				//	車輌配置モード
CTrainViewMode *g_TrainViewMode;			//	車輌視点モード
CStructSelectMode *g_StructSelectMode;		//	施設選択モード
CStructBuildMode *g_StructBuildMode;		//	施設設置モード
CStructEditMode *g_StructEditMode;			//	施設編集モード
CStationSelectMode *g_StationSelectMode;	//	施設選択モード
CStationBuildMode *g_StationBuildMode;		//	施設設置モード
CStationEditMode *g_StationEditMode;		//	施設編集モード
CSceneEditMode *g_SceneEditMode;			//	シーン編集モード
CEnvEditMode *g_EnvEditMode;				//	環境編集モード
CSimulationMode *g_SimulationMode;			//	シミュレーション設定モード
CFileMode *g_FileMode;						//	ファイルモード
CConfigMode *g_ConfigMode;					//	環境設定モード
CSkinSelectMode *g_SkinSelectMode;			//	スキン選択モード
CVideoMode *g_VideoMode;					//	ビデオモード
CExitMode *g_ExitMode;						//	終了モード
CWindowCtrl *g_ModalDialog = NULL;			//	モーダルウィンドウ

//	static メンバ
int CGameMode::ms_TopPanelTime;
int CGameMode::ms_RightPanelTime;
float CGameMode::ms_TopPanelShow;
float CGameMode::ms_RightPanelShow;
float CGameMode::ms_WindDirTemp;
string CGameMode::ms_ModeLabel;
CGameMode *CGameMode::ms_ActiveMode;
CToggleIcon *CGameMode::ms_MenuIcon[];

/*
 *	[static]
 *	初期化の初期化
 */
void CGameMode::WakeUp(){
	LanguageResource::InitLanguage();
	g_ConfigMode = new CConfigMode;
	g_ConfigMode->Load();
}

/*
 *	[static]
 *	各モード初期化
 */
void CGameMode::InitMenu(){
	CPluginMode::InitMenu();
	int i, j;
	ms_TopPanelTime = ms_RightPanelTime = PANEL_HIDE_FRAME*3;
	ms_TopPanelShow = ms_RightPanelShow = 1.0f;
	string name[MODE_NUM][9] = {{
		FlashIn("%s (F1)", lang(RailOperation)),
		FlashIn("%s (1)", lang(SelectRail)),
		FlashIn("%s (2)", lang(SelectTie)),
		FlashIn("%s (3)", lang(SelectGirder)),
		FlashIn("%s (4)", lang(SelectPier)),
		FlashIn("%s (5)", lang(SelectLine)),
		FlashIn("%s (6)", lang(SelectPole)),
		FlashIn("%s (7)", lang(BuildRail)),
		FlashIn("%s (8)", lang(EditRail))
	}, {
		FlashIn("%s (F2)", lang(TrainOperation)),
		FlashIn("%s (1)", lang(EditTrain)),
		FlashIn("%s (2)", lang(TravelSetting)),
		FlashIn("%s (3)", lang(SetTrain)),
		FlashIn("%s (4)", lang(TrainView))
	}, {
		FlashIn("%s (F3)", lang(StationOperation)),
		FlashIn("%s (1)", lang(SelectStation)),
		FlashIn("%s (2)", lang(BuildStation)),
		FlashIn("%s (3)", lang(EditStation))
	}, {
		FlashIn("%s (F4)", lang(StructOperation)),
		FlashIn("%s (1)", lang(SelectStruct)),
		FlashIn("%s (2)", lang(BuildStruct)),
		FlashIn("%s (3)", lang(EditStruct))
	}, {
		FlashIn("%s (F5)", lang(SceneOperation)),
		FlashIn("%s (1)", lang(EditScene)),
		FlashIn("%s (2)", lang(SelectEnv)),
		FlashIn("%s (3)", lang(SimulationSetting))
	}, {
		FlashIn("%s (F6)", lang(System)),
		FlashIn("%s (1)", lang(File)),
		FlashIn("%s (2)", lang(Configuration)),
		FlashIn("%s (3)", lang(SelectSkin)),
		FlashIn("%s (4)", lang(Video)),
		FlashIn("%s (5)", lang(Quit))
	}};
	char *expr[MODE_NUM][9] = {{
		lang(RailOperationModeExp),
		lang(SelectRailModeExp),
		lang(SelectTieModeExp),
		lang(SelectGirderModeExp),
		lang(SelectPierModeExp),
		lang(SelectLineModeExp),
		lang(SelectPoleModeExp),
		lang(BuildRailModeExp),
		lang(EditRailModeExp)
	}, {
		lang(TrainOperationModeExp),
		lang(EditTrainModeExp),
		lang(TravelSettingModeExp),
		lang(SetTrainModeExp),
		lang(TrainViewModeExp)
	}, {
		lang(StationOperationModeExp),
		lang(SelectStationModeExp),
		lang(BuildStationModeExp),
		lang(EditStationModeExp)
	}, {
		lang(StructOperationModeExp),
		lang(SelectStructModeExp),
		lang(BuildStructModeExp),
		lang(EditStructModeExp)
	}, {
		lang(SceneOperationModeExp),
		lang(EditSceneModeExp),
		lang(SelectEnvModeExp),
		lang(SimulationSettingModeExp)
	}, {
		lang(SystemModeExp),
		lang(FileModeExp),
		lang(ConfigurationModeExp),
		lang(SelectSkinModeExp),
		lang(VideoModeExp),
		lang(QuitModeExp)
	}};
	CGameMode *mode[MODE_NUM][9] = {
		{NULL, g_RailSelectMode, g_TieSelectMode, g_GirderSelectMode, g_PierSelectMode,
			g_LineSelectMode, g_PoleSelectMode, g_RailBuildMode, g_RailEditMode},
		{NULL, g_TrainEditMode, g_DiaEditMode, g_TrainSetMode, g_TrainViewMode},
		{NULL, g_StationSelectMode, g_StationBuildMode, g_StationEditMode},
		{NULL, g_StructSelectMode, g_StructBuildMode, g_StructEditMode},
		{NULL, g_SceneEditMode, g_EnvEditMode, g_SimulationMode},
		{NULL, g_FileMode, g_ConfigMode, g_SkinSelectMode, g_VideoMode, g_ExitMode}};
	for(i = 0; i<MODE_NUM; i++){
		int sub = MODE_SUB[i];
		ms_MenuIcon[i] = new CToggleIcon[sub+1];
		ms_MenuIcon[i][0].Init(
			0, 0, TILE_UNIT*2, TILE_UNIT*2, (char *)name[i][0].c_str(), expr[i][0], NULL,
			i ? &ms_MenuIcon[i-1][0] : NULL, NULL, DIK_F1+i,
			0.875f, 0.25f, 0.875f, 0.125f, 0.0f, 0.0f, i);
		ms_MenuIcon[i][0].SetSlidePos(
			g_DispWidth-TILE_UNIT*2, g_DispHeight*(i+2), true);
		ms_MenuIcon[i][1].Init(
			0, 0, TILE_UNIT*2, TILE_UNIT*2, (char *)name[i][1].c_str(), expr[i][1], NULL,
			i ? &ms_MenuIcon[i-1][MODE_SUB[i-1]] : NULL, mode[i][1], DIK_1,
			0.625f, 0.125f, 0.75f, 0.125f, 0.25f, 0.0, i);
		for(j = 2; j<sub; j++) ms_MenuIcon[i][j].Init(
			0, 0, TILE_UNIT*2, TILE_UNIT*2, (char *)name[i][j].c_str(), expr[i][j], NULL,
			&ms_MenuIcon[i][j-1], mode[i][j], DIK_1+j-1,
			0.625f, 0.25f, 0.75f, 0.25f, (j%4)*0.25f, (j/4)*0.25f, i);
		ms_MenuIcon[i][sub].Init(
			0, 0, TILE_UNIT*2, TILE_UNIT*2, (char *)name[i][sub].c_str(), expr[i][sub], NULL,
			&ms_MenuIcon[i][j-1], mode[i][sub], DIK_1+j-1, 0.625f, 0.375f,
			0.75f, 0.375f, (sub%4)*0.25f, (sub/4)*0.25f, i);
	}
	ms_MenuIcon[0][0].ClearGroupCheck();
	ms_MenuIcon[0][1].ClearGroupCheck();
	if(g_RSPV==RSPV_SKIN){
		ms_MenuIcon[5][0].SetCheck(false);
		ms_MenuIcon[5][3].SetCheck(false);
	}
	void InitNetworkInterface();
	InitNetworkInterface();
}

/*
 *	[static]
 *	メインループ
 */
void CGameMode::MainLoop(){
	CArrowSceneryMode::InitInterface();
	CRailBuildMode::InitRailwayInterface();
	CStructBuildMode::InitInterface();
	g_NeutralMode = new CNeutralMode;
	g_RailSelectMode = new CRailSelectMode;
	g_TieSelectMode = new CTieSelectMode;
	g_GirderSelectMode = new CGirderSelectMode;
	g_PierSelectMode = new CPierSelectMode;
	g_LineSelectMode = new CLineSelectMode;
	g_PoleSelectMode = new CPoleSelectMode;
	g_RailBuildMode = new CRailBuildMode;
	g_RailEditMode = new CRailEditMode;
	g_TrainEditMode = new CTrainEditMode;
	g_DiaEditMode = new CDiaEditMode;
	g_TrainSetMode = new CTrainSetMode;
	g_TrainViewMode = new CTrainViewMode;
	g_StructSelectMode = new CStructSelectMode;
	g_StructBuildMode = new CStructBuildMode;
	g_StructEditMode = new CStructEditMode;
	g_StationSelectMode = new CStationSelectMode;
	g_StationBuildMode = new CStationBuildMode;
	g_StationEditMode = new CStationEditMode;
	g_SceneEditMode = new CSceneEditMode;
	g_EnvEditMode = new CEnvEditMode;
	g_SimulationMode = new CSimulationMode;
	g_FileMode = new CFileMode;
	//g_ConfigMode = new CConfigMode;
	g_SkinSelectMode = new CSkinSelectMode;
	g_VideoMode = new CVideoMode;
	g_ExitMode = new CExitMode;
	if(!g_RSPV){
		LoadModeSettings();
	}else{
		DELETE_A(g_ConfigBuffer);
	}
	InitCapture();
	g_ConfigMode->CheckHardware();
	switch(g_RSPV){
	case RSPV_RAIL:
		g_RailPluginList->FindAvailable()->SetPreview();
		g_RailSelectMode->Enter();
		break;
	case RSPV_TIE:
		g_TiePluginList->FindAvailable()->SetPreview();
		g_TieSelectMode->Enter();
		break;
	case RSPV_GIRDER:
		g_GirderPluginList->FindAvailable()->SetPreview();
		g_GirderSelectMode->Enter();
		break;
	case RSPV_PIER:
		g_PierPluginList->FindAvailable()->SetPreview();
		g_PierSelectMode->Enter();
		break;
	case RSPV_LINE:
		g_LinePluginList->FindAvailable()->SetPreview();
		g_LineSelectMode->Enter();
		break;
	case RSPV_POLE:
		g_PolePluginList->FindAvailable()->SetPreview();
		g_PoleSelectMode->Enter();
		break;
	case RSPV_TRAIN:
		g_TrainPluginList->FindAvailable()->SetPreview();
		g_TrainEditMode->Enter();
		break;
	case RSPV_STATION:
		g_StationPluginList->FindAvailable()->SetPreview();
		g_StationSelectMode->Enter();
		break;
	case RSPV_STRUCT:
		g_StructPluginList->FindAvailable()->SetPreview();
		g_StructSelectMode->Enter();
		break;
	case RSPV_SURFACE:
		g_SurfacePluginList->FindAvailable()->SetPreview();
		g_SceneEditMode->Enter();
		break;
	case RSPV_ENV:
		g_EnvPluginList->FindAvailable()->SetPreview();
		g_EnvEditMode->Enter();
		break;
	case RSPV_SKIN:
		g_SkinPluginList->FindAvailable()->SetPreview();
		g_SkinSelectMode->Enter();
		break;
	default:
		g_NeutralMode->Enter();
		break;
	}
	InitMenu();
	while(ms_ActiveMode) ms_ActiveMode->Spin();
	bool RSNCloseSession();
	RSNCloseSession();
	int i;
	for(i = 0; i<MODE_NUM; i++) DELETE_A(ms_MenuIcon[i]);
	if(!g_RSPV) g_ConfigMode->Save();
	ReleaseCaptureRS();
	CPluginMode::FreeMenu();
	DELETE_V(g_NeutralMode);
	DELETE_V(g_RailSelectMode);
	DELETE_V(g_TieSelectMode);
	DELETE_V(g_GirderSelectMode);
	DELETE_V(g_PierSelectMode);
	DELETE_V(g_LineSelectMode);
	DELETE_V(g_PoleSelectMode);
	DELETE_V(g_RailBuildMode);
	DELETE_V(g_RailEditMode);
	DELETE_V(g_TrainEditMode);
	DELETE_V(g_DiaEditMode);
	DELETE_V(g_TrainSetMode);
	DELETE_V(g_TrainViewMode);
	DELETE_V(g_StructSelectMode);
	DELETE_V(g_StructBuildMode);
	DELETE_V(g_StructEditMode);
	DELETE_V(g_StationSelectMode);
	DELETE_V(g_StationBuildMode);
	DELETE_V(g_StationEditMode);
	DELETE_V(g_SceneEditMode);
	DELETE_V(g_EnvEditMode);
	DELETE_V(g_SimulationMode);
	DELETE_V(g_FileMode);
	DELETE_V(g_ConfigMode);
	DELETE_V(g_SkinSelectMode);
	DELETE_V(g_VideoMode);
	DELETE_V(g_ExitMode);
}

/*
 *	[static]
 *	設定の読込
 */
void CGameMode::LoadModeSettings(){
	char *str = g_ConfigScript, *eee;
	try{
		if(!(str = g_NeutralMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_RailSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_TieSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_GirderSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_PierSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_LineSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_PoleSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_RailBuildMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_RailEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_TrainEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_TrainSetMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_TrainViewMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StructSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StructBuildMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StructEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StationSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StationBuildMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_StationEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_SceneEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_EnvEditMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_SimulationMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_FileMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_ConfigMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_SkinSelectMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_VideoMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(!(str = g_ExitMode->LoadSetting(eee = str)) && g_ConfigBuffer) throw CSynErr(eee);
		if(g_ConfigBuffer && *(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		err.Handle("\"Config.txt\"", g_ConfigBuffer);
	}
	DELETE_A(g_ConfigBuffer);
}

/*
 *	[static]
 *	設定の保存
 */
void CGameMode::SaveModeSettings(
	FILE *file	//	ファイル
){
	g_NeutralMode->SaveSetting(file);
	g_RailSelectMode->SaveSetting(file);
	g_TieSelectMode->SaveSetting(file);
	g_GirderSelectMode->SaveSetting(file);
	g_PierSelectMode->SaveSetting(file);
	g_LineSelectMode->SaveSetting(file);
	g_PoleSelectMode->SaveSetting(file);
	g_RailBuildMode->SaveSetting(file);
	g_RailEditMode->SaveSetting(file);
	g_TrainEditMode->SaveSetting(file);
	g_TrainSetMode->SaveSetting(file);
	g_TrainViewMode->SaveSetting(file);
	g_StructSelectMode->SaveSetting(file);
	g_StructBuildMode->SaveSetting(file);
	g_StructEditMode->SaveSetting(file);
	g_StationSelectMode->SaveSetting(file);
	g_StationBuildMode->SaveSetting(file);
	g_StationEditMode->SaveSetting(file);
	g_SceneEditMode->SaveSetting(file);
	g_EnvEditMode->SaveSetting(file);
	g_SimulationMode->SaveSetting(file);
	g_FileMode->SaveSetting(file);
	g_ConfigMode->SaveSetting(file);
	g_SkinSelectMode->SaveSetting(file);
	g_VideoMode->SaveSetting(file);
	g_ExitMode->SaveSetting(file);
}

/*
 *	[static]
 *	ニュートラルモードへ移行
 */
void CGameMode::SetNeutral(){
	int i, j;
	bool sw = false;
	for(i = 0; i<MODE_NUM; i++){
		for(j = 1; j<=MODE_SUB[i]; j++){
			if(ms_MenuIcon[i][j].GetCheck()){
				sw = true;
				ms_MenuIcon[i][j].SetCheck(true);
			}
		}
	}
	if(!sw) g_NeutralMode->Enter();
	ms_TopPanelTime = PANEL_HIDE_FRAME*3;
	ms_RightPanelTime = PANEL_HIDE_FRAME*3;
}

/*
 *	フレーム入力チェック
 */
bool CGameMode::ScanInputFrame(
	int option	//	2: photomode
){
	if(g_RSPV) return false;
	bool enmouse = option&2 ? false : true;
	int i, j;
	POINT pos = g_Cursor.GetPos();
	if(!g_ConfigMode->GetHideTopPanel()
		|| ms_TopPanelTime<PANEL_HIDE_FRAME
		&& pos.y<TILE_UNIT*(ms_TopPanelTime ? 3 : 2))
		ms_TopPanelTime = PANEL_HIDE_FRAME;
	if(!g_ConfigMode->GetHideRightPanel()
		|| ms_RightPanelTime<PANEL_HIDE_FRAME
		&& pos.x>=g_DispWidth-TILE_UNIT*(ms_RightPanelTime ? 5 : 4))
		ms_RightPanelTime = PANEL_HIDE_FRAME;
	for(i = 0; i<MODE_NUM; i++){
		int sub = MODE_SUB[i];
		if(ms_MenuIcon[i][0].ScanInput(!CEditBox::IsActive(), enmouse)){
			ms_RightPanelTime = PANEL_HIDE_FRAME*3;
			return true;
		}
		if(ms_MenuIcon[i][0].GetCheck()){
			for(j = 1; j<=sub; j++){
				if(ms_MenuIcon[i][j].ScanInput(!CEditBox::IsActive(), enmouse)){
					ms_RightPanelTime = PANEL_HIDE_FRAME*3;
					return true;
				}
			}
		}else{
			for(j = 1; j<=sub; j++){
				if(ms_MenuIcon[i][j].GetCheck()
					&& ms_MenuIcon[i][j].ScanInput(false, enmouse)){
					ms_RightPanelTime = PANEL_HIDE_FRAME*3;
					return true;
				}
			}
		}
	}
	return false;
}

/*
 *	フレーム描画
 */
void CGameMode::RenderFrame(
	int option	//	1: speedinfo, 2: photomode
){
	TIMER_RAII("CGameMode::RenderFrame");
	int i, j, top1, top2, right1, right2;
	if(g_RSPV && g_RSPV!=RSPV_SKIN) option = 2;
	if(option&2){
		top1 = 0;
		g_StrTex->RenderLeft(TILE_QUAD, FontY(TILE_UNIT),
			0xffffffff, 0xff000000, FlashIn("%.1f FPS", GetFPS()));
		if(!g_RSPV){
			g_StrTex->RenderCenter(g_DispWidth/2, FontY(TILE_UNIT),
				0xffffffff, 0xff000000, g_Scene->GetNumberedName());
			g_StrTex->RenderRight(g_DispWidth-TILE_QUAD, FontY(TILE_UNIT),
				0xffffffff, 0xff000000, g_SaveFile->GetTimeText());
		}
	}else{
		if(g_ModalDialog || g_RSPV){
			ms_TopPanelTime = PANEL_HIDE_FRAME;
			ms_RightPanelTime = PANEL_HIDE_FRAME;
		}
		if(ms_TopPanelTime){
			ms_TopPanelShow = PANEL_SHOW_RATIO
				+ms_TopPanelShow*(1.0f-PANEL_SHOW_RATIO);
			ms_TopPanelTime--;
		}else{
			ms_TopPanelShow = -0.1f*PANEL_HIDE_RATIO
				+ms_TopPanelShow*(1.0f-PANEL_HIDE_RATIO);
		}
		top2 = Round(ms_TopPanelShow*TILE_UNIT*2);
		top1 = top2-TILE_UNIT*2;
		if(ms_RightPanelTime){
			ms_RightPanelShow = PANEL_SHOW_RATIO
				+ms_RightPanelShow*(1.0f-PANEL_SHOW_RATIO);
			ms_RightPanelTime--;
		}else{
			ms_RightPanelShow = -1.1f*PANEL_HIDE_RATIO
				+ms_RightPanelShow*(1.0f-PANEL_HIDE_RATIO);
		}
		right1 = g_DispWidth-Round(ms_RightPanelShow*TILE_UNIT*2);
		right2 = right1+TILE_UNIT*2;
		g_Skin->SetFrameTexture();
		SetUVMap(0.0f, 0.0f, 0.375f, 0.125f);
		TexMap2DRect(0, top1, TILE_UNIT*6, top2, 0xffffffff);
		SetUVMap(0.375f, 0.0f, 0.5f, 0.125f);
		TexMap2DRect(TILE_UNIT*6, top1,
			TILE_UNIT*6+MODE_LABEL_WIDTH, top2, 0xffffffff);
		SetUVMap(0.5f, 0.0f, 0.75f, 0.125f);
		TexMap2DRect(TILE_UNIT*6+MODE_LABEL_WIDTH, top1,
			TILE_UNIT*10+MODE_LABEL_WIDTH, top2, 0xffffffff);
		SetUVMap(0.75f, 0.0f, 0.875f, 0.125f);
		TexMap2DRect(TILE_UNIT*10+MODE_LABEL_WIDTH, top1, right1, top2, 0xffffffff);
		if(right1<g_DispWidth){
			SetUVMap(0.875f, 0.0f, 1.0f, 0.125f);
			TexMap2DRect(right1, top1, right2, top2, 0xffffffff);
			SetUVMap(0.875f, 0.375f, 1.0f, 0.5f);
			TexMap2DRect(right1, top2, right2, TILE_UNIT*2, 0xffffffff);
		}
		int ty = TILE_UNIT*2;
		bool ishow = right1<g_DispWidth+TILE_UNIT*2;
		for(i = 0; i<MODE_NUM; i++){
			int sub = MODE_SUB[i];
			ms_MenuIcon[i][0].SetSlidePos(right1, ty);
			int sy = ms_MenuIcon[i][0].GetPosY();
			if(!ms_MenuIcon[i][0].GetCheck()){
				if(ishow){
					int tf = 0;
					for(j = 1; j<=sub; j++) if(ms_MenuIcon[i][j].GetCheck()) tf = j;
					if(tf){
						ms_MenuIcon[i][0].Render(0.875f, 0.5f);
						ms_MenuIcon[i][tf].SetPos(right1-TILE_UNIT*2, sy);
						ms_MenuIcon[i][tf].Render(0.75f, 0.5f);
					}else{
						ms_MenuIcon[i][0].Render();
					}
				}
				ty += TILE_UNIT*2;
				continue;
			}
			if(ishow){
				ms_MenuIcon[i][0].Render();
				for(j = 1; j<=sub; j++){
					ms_MenuIcon[i][j].SetPos(right1-TILE_UNIT*2, sy+(j-1)*TILE_UNIT*2);
					ms_MenuIcon[i][j].Render();
				}
			}
			ty += sub*TILE_UNIT*2;
		}
		g_Skin->SetFrameTexture();
		for(i = 0; i<MODE_NUM; i++){
			int y1 = (i ? ms_MenuIcon[i-1][0].GetPosY() : 0)+TILE_UNIT*2;
			int y2 = ms_MenuIcon[i][0].GetPosY();
			if(y2-y1<=0) continue;
			SetUVMap(0.875f, 0.375f, 1.0f, 0.5f);
			TexMap2DRect(right1, y1, right2, y2, 0xffffffff);
		}
		ty = ms_MenuIcon[MODE_NUM-1][0].GetPosY()+TILE_UNIT*2;
		SetUVMap(0.875f, 0.375f, 1.0f, 0.5f);
		TexMap2DRect(right1, ty, right2, g_DispHeight, 0xffffffff);
		CToggleIcon::RenderPopupText();
		if(top2<=0) return;
		g_StrTex->RenderCenter(TILE_UNIT*6+MODE_LABEL_WIDTH/2, top1+TILE_UNIT/4+FontY(TILE_UNIT),
			g_Skin->m_FrameData.m_LabelFontColor, 0, ms_ModeLabel.c_str());
		g_StrTex->RenderLeft(TILE_UNIT*10+MODE_LABEL_WIDTH, top1+FontY(TILE_UNIT),
			g_Skin->m_FrameData.m_InfoFontColor, 0, FlashIn("%.1f FPS", GetFPS()));
		if(!g_RSPV){
			g_StrTex->RenderCenter(TILE_UNIT*8+TILE_HALF+MODE_LABEL_WIDTH
				+(g_DispWidth-TILE_UNIT*12-TILE_HALF-MODE_LABEL_WIDTH)/2, top1+FontY(TILE_UNIT),
				g_Skin->m_FrameData.m_InfoFontColor, 0, g_Scene->GetNumberedName());
			g_StrTex->RenderRight(g_DispWidth-TILE_UNIT*2, top1+FontY(TILE_UNIT),
				g_Skin->m_FrameData.m_InfoFontColor, 0, g_SaveFile->GetTimeText());
		}
	}
	if(option&1){
		int speed = GetEffectSpeed();
		if(speed!=1) g_StrTex->RenderCenter(g_DispWidth/2, top1+TILE_UNIT*2+TILE_QUAD,
			ScaleColor(0xffffffff, g_BlinkAlpha), ScaleColor(0xff000000, g_BlinkAlpha),
			speed ? FlashIn("%d %s", speed, lang(XSpeed)) : lang(Paused));
	}
}

/*
 *	コンパス描画
 */
void CGameMode::RenderCompass(){
	if(!g_ConfigMode->GetCompass()) return;
	VEC3 dir = GetVDir(), cpos = GetVPos()+14.5f*dir-4.5f*GetVUp()*g_FovRatio;
	VEC3 cdir(dir.x, 0.0f, dir.z);
	g_Skin->ScaleCompass(g_FovRatio);
	g_CompassObject[0].SetPos(cpos);
	g_CompassObject[0].SetDir(cdir, V3UP);
	g_CompassObject[0].Render();
	g_CompassObject[1].SetPos(cpos);
	g_CompassObject[1].Render();
	if(g_RSPV || !g_ConfigMode->GetWindMeter() || !g_ConfigMode->GetWind()) return;
	const float itv = 0.6f, range = 0.9f, windmax = SYMMETRIC_ROTATION_MAX*itv;
	float windspeed = V3Len(&g_WindDir);
	if(!g_ModalDialog) ms_WindDirTemp +=
		windmax*(1.0f-expf(-0.2f*GetEffectSpeed()*windspeed/windmax));
	ValueCircular(&ms_WindDirTemp, -0.5f*itv, 0.5f*itv);
	float z, tz;
	for(z = -range; z<range+0.1f; z += itv){
		tz = z+ms_WindDirTemp;
		float alpha = (range-fabsf(tz))/range;
		if(alpha<=0.0f) continue;
		g_WindDirObject.SetPos(g_CompassObject[1].GetPos()+g_FovRatio*tz*g_WindDirNorm);
		g_WindDirObject.SetDir(g_WindDirNorm, V3UP);
		g_WindDirObject.RenderA(alpha);
	}
	devResetMatrix();
	if(ms_ActiveMode!=g_NeutralMode || !g_ConfigMode->IsWindowDiv()){
		int vp_w = g_DispWidth, vp_h = g_DispHeight;
		g_StrTex->RenderRight(vp_w*45/100, vp_h-TILE_UNIT,
			0xffffffff, 0xff000000, FlashIn("%s: %.1f [m/s]", lang(WindSpeed), MAXFPS*windspeed));
	}
}

/*
 *	[static]
 *	ダイアログ描画
 */
bool CGameMode::RenderDialog(){
	if(!g_ModalDialog) return false;
	devSetTexture(0, NULL);
	Fill2DRect(0, 0, g_DispWidth, g_DispHeight, 0x80808080);
	g_ModalDialog->Render();
	return true;
}

/*
 *	コンストラクタ
 */
CGameMode::CGameMode(){
	CInterface::ResetTabHead();
	m_Interface.Init(0, 0, 0, 0, "", NULL);
}

/*
 *	モードを有効化
 */
void CGameMode::Enter(){
	SetViewport(0, 0, sv3.width, sv3.height);
	ms_TopPanelTime = PANEL_HIDE_FRAME*3;
	ModelPluginFreeInst();
	CPopMenu::ResetCurrentMenu();
	CEditBox::Disable();
	Randomize();
	g_SystemSwitch[SYS_SW_SHADOW].SetValue(g_ConfigMode->GetShadow());
	g_SystemSwitch[SYS_SW_ENVMAP].SetValue(g_ConfigMode->GetEnvMap());
	void ResetShowRailSelect();
	ResetShowRailSelect();
	EnterGame();
	ms_ActiveMode = this;
}

/*
 *	モードループ
 */
void CGameMode::Spin(){
	SetMasterVolume();
	while(PeekAllMessage()){
		if(IsActive()){
			g_JobTimer.Start();
			{
				TIMER_RAII("All");
				if(g_CursorLockable) g_Cursor.Clip();
				//カーソル描画直前に
				//ScanInputDevice();
				//g_Cursor.FixCursor();
				g_BlinkCounter = (g_BlinkCounter+1)%MAXFPS;
				g_BlinkAlpha = 0.5f*(sinf(2.0f*D3DX_PI*g_BlinkCounter/MAXFPS)+1.0f);
				g_ConfigMode->SetSpecularLight();
				g_ManualControl = !!g_SimulationMode->GetManualControl() || g_NetworkInitialized;
				g_IgnoreAcceleration = !!g_SimulationMode->GetIgnoreAcceleration();
				void CheckNetworkState();
				CheckNetworkState();
				SpinGame();
				if(CheckAlt() && GetKey(DIK_F4)==S_PUSH){
					Exit();
					break;
				}
				if(GetButton(DIM_LEFT)<=S_PULL){
					CDragContainer::EndDrag();
				}
			}
			{
				TIMER_RAII("SyncFrame");
				SyncFrame();
			}
			g_JobTimer.Stop();
		}else{
			SetMasterVolume();
			ClipCursor(NULL);
			if(g_NetworkInitialized){
				g_SaveFile->Simulate(-1);
				SyncFrame();
			}else{
				WaitMessage();
			}
		}
		if(ms_ActiveMode!=this) return;
	}
	Exit();
}

/*
 *	サウンド定常処理
 */
void CGameMode::SpinSound(){
	LONGLONG t = HighTimer();
	g_FrameDelta = (float)(t-g_SoundSync);
	g_SoundSync = t;
	SetListenerPos(GetVPos());
	SetListenerDir(GetVDir(), GetVUp());
}

/*
 *	実効シミュレーション速度取得
 */
int CGameMode::GetEffectSpeed(){
	return IsPaused() ? 0 : g_SimulationMode->GetSimSpeed();
}
