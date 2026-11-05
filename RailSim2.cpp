#include "stdafx.h"
#include "RSPV.h"
#include "CPluginTree.h"
#include "CScene.h"
#include "CSaveFile.h"
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
#include "CGameMode.h"

//	内部定数
extern const float RAILSIM_VERSION = 2.15f;	//	本体バージョン
extern const char* VERSION_STRING = "2.15";	//	表示バージョン文字列 (2.00a 等のバグ修正コードを含む) 

//	外部グローバル
extern char *g_PluginViewArg;
extern string g_SelectedSkin;
extern bool g_CursorLockable;

//	内部グローバル
int g_RSPV = RSPV_NONE;						//	RSPV モード
HFONT g_TempFont = NULL;					//	初期フォント
CCursor g_Cursor;							//	カーソル
CStringTexture *g_StrTex = NULL;			//	文字テクスチャ	
CRailPlugin *g_Rail = NULL;					//	カレントレール
CTiePlugin *g_Tie = NULL;					//	カレント枕木
CGirderPlugin *g_Girder = NULL;				//	カレント橋桁
CPierPlugin *g_Pier = NULL;					//	カレント橋脚
CLinePlugin *g_Line = NULL;					//	カレント架線
CPolePlugin *g_Pole = NULL;					//	カレント架線柱
CSurfacePlugin *g_DefaultSurface = NULL;	//	デフォルトサーフェイス
CEnvPlugin *g_DefaultEnv = NULL;			//	デフォルト環境
CSaveFile *g_SaveFile = NULL;				//	カレントファイル

string g_SelectedRailID;	//	選択レール ID
string g_SelectedTieID;		//	選択枕木 ID
string g_SelectedGirderID;	//	選択橋桁 ID
string g_SelectedPierID;	//	選択橋脚 ID
string g_SelectedLineID;	//	選択架線 ID
string g_SelectedPoleID;	//	選択架線柱 ID
string g_SelectedTrainID;	//	選択車輌 ID
string g_SelectedStationID;	//	選択駅舎 ID
string g_SelectedStructID;	//	選択施設 ID
string g_SelectedSurfaceID;	//	選択地形 ID
string g_SelectedEnvID;		//	選択地形 ID
string g_SelectedSkinID;	//	選択スキン ID

CRailPluginList *g_RailPluginList;			//	レールリスト
CTiePluginList *g_TiePluginList;			//	枕木リスト
CGirderPluginList *g_GirderPluginList;		//	橋桁リスト
CPierPluginList *g_PierPluginList;			//	橋脚リスト
CLinePluginList *g_LinePluginList;			//	架線リスト
CPolePluginList *g_PolePluginList;			//	架線柱リスト
CTrainPluginList *g_TrainPluginList;		//	車輌リスト
CStationPluginList *g_StationPluginList;	//	駅舎リスト
CStructPluginList *g_StructPluginList;		//	施設リスト
CSurfacePluginList *g_SurfacePluginList;	//	地形リスト
CEnvPluginList *g_EnvPluginList;			//	地形リスト
CSkinPluginList *g_SkinPluginList;			//	スキンリスト

CTexture g_OpeningTexture;	//	オープニング用画像

/*
 *	オープニングアニメーション
 */
void Opening(){
	static int cnt = 0, loading = 0;
	while(PeekAllMessage()){
		ScanInputDevice();
		const int anim = 30;
		const int letters = 10, letl = 10;
		const int letsize = 32, letsp = 24;
		BeginScene();
		int i;
		devTEX_LINEAR(0);
		devBLEND_ADD2();
		devSetTexture(0, g_OpeningTexture.GetObject());
		for(i = 0; i<letters; i++){
			int begin = (anim-letl)*i/(letters-1), end = begin+letl;
			if(cnt<begin) continue;
			float u = (i%4)*0.25f, v = (i/4)*0.25f;
			SetUVMap(u, v, u+0.25f, v+0.25f);
			int x = (g_DispWidth-letters*(letsize+letsp)+letsp)/2+i*(letsize+letsp);
			int y = (g_DispHeight-letsize)/2;
			int yofs = cnt<end ? (end-cnt)*128/letl : 0;
			int alpha = cnt<end ? (cnt-begin)*255/letl : 255;
			D3DCOLOR col = i<7
				? MAKE_AC(alpha, 0, i*255/6, 255)
				: MAKE_AC(alpha, 0, 128+(i-7)*127/2, 0);
			D3DCOLOR sdw = MAKE_AC(alpha/4, 255, 255, 255);
			int x1 = x, y1 = y-yofs, x2 = x+letsize, y2 = y+letsize+yofs;
			{
				int rel = cnt<end ? end-cnt : 0;
				int sfty = 256/(rel+4);
				int sft1 = (g_DispWidth/2-x1)*2/(rel+4);
				int sft2 = (g_DispWidth/2-x2)*2/(rel+4);
				VTX_TLX vt[] = {
					x1+sft1-0.5f, y1-sfty-0.5f, 0.0f, 1.0f, sdw&0xff000000, sv3.u[0], sv3.v[0],
					x2+sft2-0.5f, y1-sfty-0.5f, 0.0f, 1.0f, sdw&0xff000000, sv3.u[1], sv3.v[0],
					x2-0.5f, y2-0.5f, 0.0f, 1.0f, sdw, sv3.u[1], sv3.v[1],
					x1-0.5f, y2-0.5f, 0.0f, 1.0f, sdw, sv3.u[0], sv3.v[1]};
				sv3.pDev->SetVertexShader(FVF_TLX);
				sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vt, sizeof(VTX_TLX));
			}
			TexMap2DRect(x1, y1, x2, y2, col);
		//	Dialog("%d %d %p", x, y, col);
		}
		devTEX_POINT(0);
		devBLEND_ALPHA();
		if(cnt>=anim){
			const int pitypes = 15;
			static char *pitype[pitypes] = {
				"Rail", "Tie", "Girder", "Pier", "Line", "Pole",
				"RailwayPluginSet", "Train", "TrainGroupTemplate",
				"Station", "Struct", "Surface", "Env", "Skin", "Layout"};
			const int grphwid = TILE_UNIT*8;
			int gx1 = (g_DispWidth-grphwid)/2, gy1 = g_DispHeight-TILE_UNIT*3;
			int gx2 = (g_DispWidth+grphwid)/2, gy2 = g_DispHeight-TILE_UNIT*3+FONT_HEIGHT;
			D3DCOLOR fcol[4] = {0xffc0c0c0, 0xffc0c0c0, 0xff808080, 0xff808080};
			D3DCOLOR gcol[4] = {0xc080c0ff, 0xc080c0ff, 0xc0406080, 0xc0406080};
			devSetTexture(0, NULL);
			Grad2DRect(gx1, gy1, gx2, gy2, fcol);
			Fill2DRect(gx1+1, gy1+1, gx2-1, gy2-1, 0x80000000);
			int tw = loading<pitypes ? loading*(grphwid-4)/(pitypes-1) : grphwid-4;
			Grad2DRect(gx1+2, gy1+2, gx1+2+tw, gy2-2, gcol);
			g_StrTex->RenderCenter(g_DispWidth/2, g_DispHeight-TILE_UNIT*2,
				0xffc0c0c0, 0, FlashIn("Loading %s...", pitype[loading]));
			loading++;
		}
		EndScene();
		if(CheckAlt() && GetKey(DIK_F4)==S_PUSH){
			ExitProcess(0);
			break;
		}
		SyncFrame();
		if(cnt>=anim) return;
		cnt++;
	}
}

/*
 *	エントリーポイント
 */
void Main(){
	SetListenerSens(10.0f);
	SetMasterVolume(/*DSBVOLUME_MAX*/);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	Randomize();
	InitSystemObject();
	InitSystemSwitch();
	g_OpeningTexture.LoadResource("OPENING");
	g_TempFont = CreateFont(FONT_HEIGHT, 0, 0, 0,
		FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH|FF_MODERN, NULL/*"ＭＳ Ｐゴシック"*/);
	g_StrTex = new CStringTexture(FONT_HEIGHT, 1);
	g_StrTex->SetFont(g_TempFont);
	CSkinPlugin *skin = NULL;
	g_RailPluginList = new CRailPluginList;
	g_TiePluginList = new CTiePluginList;
	g_GirderPluginList = new CGirderPluginList;
	g_PierPluginList = new CPierPluginList;
	g_LinePluginList = new CLinePluginList;
	g_PolePluginList = new CPolePluginList;
	g_TrainPluginList = new CTrainPluginList;
	g_StationPluginList = new CStationPluginList;
	g_StructPluginList = new CStructPluginList;
	g_SurfacePluginList = new CSurfacePluginList;
	g_EnvPluginList = new CEnvPluginList;
	g_SkinPluginList = new CSkinPluginList;

	if(g_PluginViewArg){
		char *cparam = g_PluginViewArg;
		int cplen = strlen(cparam);
		char tname[256], dname[256];
		char *ptr = cparam+cplen-5;
		while(*ptr!='\\') ptr--;
		strcpy(tname, ptr+1);
		*ptr = 0;
		strcpy(dname, cparam);
		*ptr = '\\';
		if(!strcmpi(tname, "Rail2.txt")){
			if(g_RailPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_RAIL;
		}else if(!strcmpi(tname, "Tie2.txt")){
			if(g_TiePluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_TIE;
		}else if(!strcmpi(tname, "Girder2.txt")){
			if(g_GirderPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_GIRDER;
		}else if(!strcmpi(tname, "Pier2.txt")){
			if(g_PierPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_PIER;
		}else if(!strcmpi(tname, "Line2.txt")){
			if(g_LinePluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_LINE;
		}else if(!strcmpi(tname, "Pole2.txt")){
			if(g_PolePluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_POLE;
		}else if(!strcmpi(tname, "Train2.txt")){
			if(g_TrainPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_TRAIN;
		}else if(!strcmpi(tname, "Station2.txt")){
			if(g_StationPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_STATION;
		}else if(!strcmpi(tname, "Struct2.txt")){
			if(g_StructPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_STRUCT;
		}else if(!strcmpi(tname, "Surface2.txt")){
			if(g_SurfacePluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_SURFACE;
		}else if(!strcmpi(tname, "Env2.txt")){
			if(g_EnvPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_ENV;
		}else if(!strcmpi(tname, "Skin2.txt")){
			if(g_SkinPluginList->LoadOne(cparam, dname, false)) g_RSPV = RSPV_SKIN;
		}else if(!strcmpi(tname, "Train.txt")){
			if(g_TrainPluginList->LoadOne(cparam, dname, true)) g_RSPV = RSPV_TRAIN;
		}else if(!strcmpi(tname, "Station.txt")){
			if(g_StationPluginList->LoadOne(cparam, dname, true)) g_RSPV = RSPV_STATION;
		}else if(!strcmpi(tname, "Struct.txt")){
			if(g_StructPluginList->LoadOne(cparam, dname, true)) g_RSPV = RSPV_STRUCT;
		}else if(!strcmpi(tname, "Surface.txt")){
			if(g_SurfacePluginList->LoadOne(cparam, dname, true)) g_RSPV = RSPV_SURFACE;
		}
		if(!g_RSPV){
			ErrorDialog("%s: %s", lang(InvalidPluginType), tname);
		}
	}
	if(g_RSPV){
		SetWindowText(svw.hWnd, "RailSim II - Plugin Viewer Mode");
		if(!(g_Env = g_EnvPluginList->FindAvailable())){
			if(!g_EnvPluginList->LoadOne("Env\\Default\\Env2.txt", "Default", false)
				|| !(g_Env = g_EnvPluginList->FindAvailable())){
				ErrorDialog(lang(NoEnvAvailable));
				return;
			}
		}
		if(!(skin = g_SkinPluginList->FindAvailable())){
			if(!g_SkinPluginList->LoadOne("Skin\\Default_Blue\\Skin2.txt", "Default_Blue", false)
				|| !(skin = g_SkinPluginList->FindAvailable())){
				ErrorDialog(lang(NoSkinAvailable));
				return;
			}
		}
	}else{
		Sleep(500);
		Opening();
		g_RailPluginList->List();
		if(!(g_Rail = g_RailPluginList->FindPlugin((char *)g_SelectedRailID.c_str())))
			g_Rail = g_RailPluginList->FindAvailable();
		Opening();
		g_TiePluginList->List();
		if(!(g_Tie = g_TiePluginList->FindPlugin((char *)g_SelectedTieID.c_str())))
			g_Tie = g_TiePluginList->FindAvailable();
		Opening();
		g_GirderPluginList->List();
		if(!(g_Girder = g_GirderPluginList->FindPlugin((char *)g_SelectedGirderID.c_str())))
			g_Girder = g_GirderPluginList->FindAvailable();
		Opening();
		g_PierPluginList->List();
		if(!(g_Pier = g_PierPluginList->FindPlugin((char *)g_SelectedPierID.c_str())))
			g_Pier = g_PierPluginList->FindAvailable();
		Opening();
		g_LinePluginList->List();
		if(!(g_Line = g_LinePluginList->FindPlugin((char *)g_SelectedLineID.c_str())))
			g_Line = g_LinePluginList->FindAvailable();
		Opening();
		g_PolePluginList->List();
		if(!(g_Pole = g_PolePluginList->FindPlugin((char *)g_SelectedPoleID.c_str())))
			g_Pole = g_PolePluginList->FindAvailable();
		Opening();
		void LoadRailwayPluginSetList();
		LoadRailwayPluginSetList();
		Opening();
		g_TrainPluginList->List();
		if(!(g_Train = g_TrainPluginList->FindPlugin((char *)g_SelectedTrainID.c_str())))
			g_Train = g_TrainPluginList->FindAvailable();
		Opening();
		void LoadTrainGroupTemplateList();
		LoadTrainGroupTemplateList();
		Opening();
		g_StationPluginList->List();
		if(!(g_Station = g_StationPluginList->FindPlugin((char *)g_SelectedStationID.c_str())))
			g_Station = g_StationPluginList->FindAvailable();
		Opening();
		g_StructPluginList->List();
		if(!(g_Struct = g_StructPluginList->FindPlugin((char *)g_SelectedStructID.c_str())))
			g_Struct = g_StructPluginList->FindAvailable();
		Opening();
		g_SurfacePluginList->List();
		if(!(g_DefaultSurface = g_SurfacePluginList->FindPlugin("Default"))
			&& !(g_DefaultSurface = g_SurfacePluginList->FindPlugin((char *)g_SelectedSurfaceID.c_str()))
			&& !(g_DefaultSurface = g_SurfacePluginList->FindAvailable())){
			ErrorDialog(lang(NoSurfaceAvailable));
			return;
		}
		Opening();
		g_EnvPluginList->List();
		if(!(g_DefaultEnv = g_EnvPluginList->FindPlugin("Default"))
			&& !(g_DefaultEnv = g_EnvPluginList->FindPlugin((char *)g_SelectedEnvID.c_str()))
			&& !(g_DefaultEnv = g_EnvPluginList->FindAvailable())){
			ErrorDialog(lang(NoEnvAvailable));
			return;
		}
		Opening();
		g_SkinPluginList->List();
		if(!(skin = g_SkinPluginList->FindPlugin((char *)g_SelectedSkinID.c_str()))
			&& !(skin = g_SkinPluginList->FindAvailable())){
			ErrorDialog(lang(NoSkinAvailable));
			return;
		}
		Opening();
	}
	DeleteObject(g_TempFont);
	skin->SetPreview();
	CTreeDirElement::InitMenu();
	g_Cursor.Init();
	g_CursorLockable = true;
	InitGrid();
	CGameMode::MainLoop();
	DELETE_V(g_SaveFile);
	ClipCursor(NULL);
	CTreeDirElement::FreeMenu();
	DELETE_V(g_StrTex);
	DELETE_V(g_RailPluginList);
	DELETE_V(g_TiePluginList);
	DELETE_V(g_GirderPluginList);
	DELETE_V(g_PierPluginList);
	DELETE_V(g_LinePluginList);
	DELETE_V(g_PolePluginList);
	DELETE_V(g_TrainPluginList);
	DELETE_V(g_StationPluginList);
	DELETE_V(g_StructPluginList);
	DELETE_V(g_SurfacePluginList);
	DELETE_V(g_EnvPluginList);
	DELETE_V(g_SkinPluginList);
}

/*
 *	モデルプラグインの関連インスタンスを解放
 */
void ModelPluginFreeInst(){
	if(g_Train) g_Train->FreeInst();
	if(g_Struct) g_Struct->FreeInst();
	if(g_Station) g_Station->FreeInst();
	if(g_Surface) g_Surface->FreeInst();
}

/*
 *	現在選択されているプラグインの ID をセット
 */
void SetCurrentPluginID(){
	g_SelectedRailID = g_Rail ? g_Rail->GetID() : "";
	g_SelectedTieID = g_Tie ? g_Tie->GetID() : "";
	g_SelectedGirderID = g_Girder ? g_Girder->GetID() : "";
	g_SelectedPierID = g_Pier ? g_Pier->GetID() : "";
	g_SelectedLineID = g_Line ? g_Line->GetID() : "";
	g_SelectedPoleID = g_Pole ? g_Pole->GetID() : "";
	g_SelectedTrainID = g_Train ? g_Train->GetID() : "";
	g_SelectedStationID = g_Station ? g_Station->GetID() : "";
	g_SelectedStructID = g_Struct ? g_Struct->GetID() : "";
	g_SelectedSurfaceID = g_Surface ? g_Surface->GetID() : "";
	g_SelectedEnvID = g_Env ? g_Env->GetID() : "";
	g_SelectedSkinID = g_Skin ? g_Skin->GetID() : "";
}
