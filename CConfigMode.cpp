#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CSkinPlugin.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CFileMode.h"
#include "CSceneryMode.h"
#include "CConfigMode.h"

//	内部定数
const int WW = 512, WH = 384;	//	窓サイズ
const int RES_MODE_WIDTH[RES_MODE_NUM] = {640, 800, 1024, 1280};	//	解像度幅
const int RES_MODE_HEIGHT[RES_MODE_NUM] = {480, 600, 768, 960};		//	解像度高

//	外部グローバル
extern char *g_PluginViewArg;
extern bool g_StencilEnabled;

extern string g_SelectedRailID;
extern string g_SelectedTieID;
extern string g_SelectedGirderID;
extern string g_SelectedPierID;
extern string g_SelectedLineID;
extern string g_SelectedPoleID;
extern string g_SelectedTrainID;
extern string g_SelectedStationID;
extern string g_SelectedStructID;
extern string g_SelectedSurfaceID;
extern string g_SelectedEnvID;
extern string g_SelectedSkinID;

//	内部グローバル
extern char *YESNO[2] = {"no", "yes"};
char *g_ConfigBuffer = NULL;	//	コンフィグファイルバッファ
char *g_ConfigScript = NULL;	//	コンフィグファイル解析位置
float g_ConfigVersion;			//	コンフィグファイルバージョン
bool g_FullScreen = true;		//	フルスクリーン
bool g_RailMipMap = true;		//	レール等ミップマップ
bool g_TrainMipMap = false;		//	車輌ミップマップ
bool g_StructMipMap = false;	//	施設等ミップマップ
bool g_SurfaceMipMap = true;	//	地形等ミップマップ
bool g_NamedObjectMipMap;		//	名前付きオブジェクトミップマップ

/*
 *	コンストラクタ
 */
CConfigMode::CConfigMode(){
	int i, half = WW/2-TILE_HALF*3, win = half-TILE_UNIT*2;

	m_ConfigWindow1.Init(0, 0,
		WW, WH, FlashIn("%s [1/2]", lang(Configuration)), &m_Interface, true);

	m_InterfaceGroup.Init(TILE_UNIT, TILE_UNIT*2,
		half, TILE_UNIT*5, lang(Interface), &m_ConfigWindow1);
	m_HideTopPanel.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(HideTopPanel), &m_InterfaceGroup);
	m_HideRightPanel.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(HideSidePanel), &m_InterfaceGroup);
	m_WindowShadow.Init(TILE_UNIT, TILE_UNIT*3+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(WindowShadow), &m_InterfaceGroup);

	m_DeviceGroup.Init(TILE_UNIT, TILE_UNIT*8,
		half, TILE_UNIT*10, lang(Device), &m_ConfigWindow1);
	m_NeedRestart.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(NeedRestart), &m_DeviceGroup, 0, 1);
	m_FullScreen.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(FullScreen), &m_DeviceGroup);
	m_ResLabel.Init(TILE_UNIT, TILE_UNIT*3+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(Resolution), &m_DeviceGroup, 0, 1);
	for(i = 0; i<RES_MODE_NUM; i++) m_Resolution[i].Init(
		TILE_UNIT+(i%2)*(half-TILE_UNIT*1)/2, TILE_UNIT*(i/2+4)+TILE_QUAD,
		(half-TILE_UNIT*3)/2, TILE_UNIT,
		FlashIn("%d * %d", RES_MODE_WIDTH[i], RES_MODE_HEIGHT[i]),
		&m_DeviceGroup, i ? &m_Resolution[i-1] : NULL);
	string miplabel[MIPMAP_NUM] = {
		lang(RailEtc), lang(Train), lang(StationAndStruct), lang(Surface)};
	m_MipMapLabel.Init(TILE_UNIT, TILE_UNIT*6+TILE_QUAD,
		half-TILE_UNIT*2, TILE_UNIT, lang(MipMap), &m_DeviceGroup, 0, 1);
	for(i = 0; i<MIPMAP_NUM; i++) m_MipMap[i].Init(
		TILE_UNIT+(i%2)*(half-TILE_UNIT*1)/2, TILE_UNIT*(i/2+7)+TILE_QUAD,
		(half-TILE_UNIT*3)/2, TILE_UNIT, (char *)miplabel[i].c_str(), &m_DeviceGroup);

	m_AccessoryGroup.Init(TILE_UNIT, TILE_UNIT*19,
		half, TILE_UNIT*4, lang(Accesories), &m_ConfigWindow1);
	m_Compass.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD,
		(half-TILE_UNIT*3)/2, TILE_UNIT,
		lang(Compass), &m_AccessoryGroup);
	m_WindMeter.Init(TILE_UNIT+(half-TILE_UNIT*1)/2, TILE_UNIT+TILE_QUAD,
		(half-TILE_UNIT*3)/2, TILE_UNIT,
		lang(WindMeter), &m_AccessoryGroup);
	m_ShowMap.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		FlashIn("%s (Ctrl + M)", lang(Map)), &m_AccessoryGroup);

	m_EffectGroup.Init(WW/2+TILE_HALF, TILE_UNIT*2,
		half, TILE_UNIT*11, lang(VisualEffect), &m_ConfigWindow1);
	m_Shadow.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		FlashIn("%s (Ctrl + S)", lang(Shadow)), &m_EffectGroup);
	m_LinearFilter.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(TextureFiltering), &m_EffectGroup);
	m_EnvMap.Init(TILE_UNIT, TILE_UNIT*3+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(EnvMap), &m_EffectGroup);
	m_SpecularLight.Init(TILE_UNIT, TILE_UNIT*4+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(SpecularLight), &m_EffectGroup);
	m_SunLensFlare.Init(TILE_UNIT, TILE_UNIT*5+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(LensflareOfSun), &m_EffectGroup);
	m_SunWhiteout.Init(TILE_UNIT, TILE_UNIT*6+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(WhiteoutOfSun), &m_EffectGroup);
	m_MiscLensFlare.Init(TILE_UNIT, TILE_UNIT*7+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(LensflareOfOthers), &m_EffectGroup);
	m_MiscParticle.Init(TILE_UNIT, TILE_UNIT*8+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(Particle), &m_EffectGroup);
	m_Wind.Init(TILE_UNIT, TILE_UNIT*9+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(Wind), &m_EffectGroup);
	m_SoundGroup.Init(WW/2+TILE_HALF, TILE_UNIT*14,
		half, TILE_UNIT*5, lang(Sound), &m_ConfigWindow1);
	m_InterfaceSound.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(Interface), &m_SoundGroup);
	string sndlabel[MIPMAP_NUM] = {
		lang(RailEtc), lang(Train), lang(StationAndStruct), lang(Surface)};
	for(i = 0; i<PISND_NUM; i++) m_PluginSound[i].Init(
		TILE_UNIT+(i%2)*(half-TILE_UNIT*1)/2, TILE_UNIT*(i/2+2)+TILE_QUAD,
		(half-TILE_UNIT*3)/2, TILE_UNIT, (char *)sndlabel[i].c_str(), &m_SoundGroup);

	m_MiscGroup.Init(WW/2+TILE_HALF, TILE_UNIT*20,
		half, TILE_UNIT*3, lang(Miscellaneous), &m_ConfigWindow1);
	m_UseUndo.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		lang(UseUndo), &m_MiscGroup);

	m_ConfigWindow2.Init(0, 0,
		WW, WH, FlashIn("%s [2/2]", lang(Configuration)), &m_Interface, true);

	m_StereoGroup.Init(TILE_UNIT, TILE_UNIT*2,
		half, TILE_UNIT*7, lang(Stereoscopy), &m_ConfigWindow2);
	m_StereoEnabled.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, half-TILE_UNIT*2, TILE_UNIT,
		FlashIn("%s (Ctrl + D)", lang(StereoscopyEnabled)), &m_StereoGroup);
	m_StereoMethodLabel.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD,
		half-TILE_UNIT*2, TILE_UNIT, lang(StereoscopyMethod), &m_StereoGroup, 0, 1);
	string stereolabel[STEREO_METHOD_NUM] = {
		lang(ParallelView), lang(CrossEyedView)};
	for(i = 0; i<STEREO_METHOD_NUM; i++) m_StereoMethod[i].Init(
		TILE_UNIT, TILE_UNIT*(i+3)+TILE_QUAD,
		half-TILE_UNIT*2, TILE_UNIT, (char *)stereolabel[i].c_str(),
		&m_StereoGroup, i ? &m_StereoMethod[i-1] : NULL);
	m_StereoMethod[0].SetCheck();
	m_StereoIntervalLabel.Init(TILE_UNIT, TILE_UNIT*5+TILE_QUAD,
		half-TILE_UNIT*6, TILE_UNIT, FlashIn("%s [m]", lang(StereoscopyInterval)), &m_StereoGroup, 0, 1);
	m_StereoIntervalEdit.Init(half-TILE_UNIT*5, TILE_UNIT*5+TILE_QUAD,
		TILE_UNIT*4, TILE_UNIT, "1.00", &m_StereoGroup, 8);

	m_ActiveWindow = NULL;
}

/*
 *	デストラクタ
 */
CConfigMode::~CConfigMode(){
}

/*
 *	モードを有効化
 */
void CConfigMode::EnterInterface(){
	ms_ModeLabel = lang(Configuration);
	m_StereoIntervalEdit.FinishInput();
	if(!*m_StereoIntervalEdit.GetText()) m_StereoIntervalEdit.SetText("1.00");
	else m_StereoIntervalEdit.SetText(FlashIn("%.2f", GetStereoInterval()));
}

/*
 *	入力チェック
 */
void CConfigMode::ScanInputInterface(){
	m_Interface.ScanInput();
	if(m_ConfigWindow1.CheckClose() || m_ConfigWindow2.CheckClose()){
		SetNeutral();
		return;
	}
	if(!m_UseUndo.GetCheck()) g_FileMode->ResetUndo();
}

/*
 *	入力チェック
 */
int CConfigMode::ScanInputWindowDiv(){
	devSetTexture(0, NULL);
	devBLEND_ALPHA();
	if(IsWindowDiv() && GetButton(DIM_LEFT)==S_FREE && GetButton(DIM_MIDDLE)==S_FREE && GetButton(DIM_RIGHT)==S_FREE){
		m_ActiveWindow = m_RootWindow.GetPointWindow(0, 0, g_DispWidth, g_DispHeight, g_Cursor.GetPos());
	}
	int ret = CWindowDivInfo::ScanInput(m_RootWindow.GetDivAdr(), 0, 0, g_DispWidth, g_DispHeight, g_Scene->GetCamera(), &g_Scene);
	CWindowDivInfo::RenderInterface();
	return ret;
}

/*
 *	レンダリング
 */
void CConfigMode::RenderInterface(){
	int ix = g_DispWidth*60/100, iy = g_DispHeight;
	g_StrTex->RenderLeft(TILE_QUAD, g_DispHeight-TILE_UNIT, 0xffffffff, 0xff000000,
		FlashIn("Version %s / LANG = %s", VERSION_STRING, g_LanguageName.c_str()));
}

/*
 *	レンダリング
 */
void CConfigMode::RenderWindowDiv(){
	devSetTexture(0, NULL);
	devBLEND_ALPHA();
	if(m_RootWindow.GetDiv()){
		m_RootWindow.GetDiv()->RenderInterfaceRecursive(0, 0, g_DispWidth, g_DispHeight);
	}
}

/*
 *	設定読込
 */
bool CConfigMode::Load(){
	int i;
	bool hidetoppanel = false, hiderightpanel = false, windowshadow = true;
	bool fullscreen = true;
	int res[2] = {640, 480};
	bool compass = true, windmeter = true, showmap = false;
	bool shadow = false, linearfilter = true, envmap = true;
	bool specularlight = true, sunlensflare = true, sunwhiteout = true;
	bool misclensflare = true, miscparticle = true, wind = true;
	bool interfacesound = true, railsound = true, trainsound = true;
	bool structsound = true, surfacesound = true;
	bool useundo = true;
	bool stereoenabled = false;
	int stereomethod = 0;
	float stereointerval = 1.0f;
	char *str, *eee;
	g_SelectedSkinID = "Default_Blue";
	g_SelectedRailID = "Default_JR_Narrow";
	g_SelectedTieID = "Default_JRN_BallastPC";
	g_SelectedGirderID = "Default_JRN_SinglePC";
	g_SelectedPierID = "Default_SinglePC";
	g_SelectedLineID = "Default_SimpleCatenary";
	g_SelectedPoleID = "Default_JRN_Single";
	g_SelectedTrainID = "Aizentranza01";
	g_SelectedStationID = "MM02";
	g_SelectedStructID = "Ship";
	g_SelectedSurfaceID = "Default";
	g_SelectedEnvID = "Default";
	if(chdir(g_BaseDir)) goto SET;
	str = g_ConfigBuffer = LoadBinaryText("Config.txt");
	if(!g_ConfigBuffer) goto SET;
	try{
		if(!(str = Space(eee = str))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "DatafileHeader"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "RailSimVersion", &g_ConfigVersion))) throw CSynErr(eee);
		if(g_ConfigVersion>RAILSIM_VERSION)
			throw CSynErr(eee, "%s: %.2f", lang(UnsupportedVersion), g_ConfigVersion);
		string datafiletype;
		if(!(str = AsgnIdentifier(eee = str, "DatafileType", &datafiletype))) throw CSynErr(eee);
		if(datafiletype!="Config") throw CSynErr(eee, lang(InvalidDatafileType));
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "ConfigMode"))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "Interface"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "HideTopPanel", &hidetoppanel))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "HideRightPanel", &hiderightpanel))) throw CSynErr(eee);
		if(g_ConfigVersion>=2.03f) if(!(str = AsgnYesNo(eee = str, "WindowShadow", &windowshadow))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Device"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "FullScreen", &fullscreen))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "Resolution", res, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "RailMipMap", &g_RailMipMap))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "TrainMipMap", &g_TrainMipMap))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "StructMipMap", &g_StructMipMap))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "SurfaceMipMap", &g_SurfaceMipMap))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Accessory"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "Compass", &compass))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "WindMeter", &windmeter))) throw CSynErr(eee);
		if(g_ConfigVersion>=2.03f) if(!(str = AsgnYesNo(eee = str, "ShowMap", &showmap))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "Effect"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "Shadow", &shadow))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "LinearFilter", &linearfilter))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "EnvMap", &envmap))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "SpecularLight", &specularlight))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "SunLensFlare", &sunlensflare))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "SunWhiteout", &sunwhiteout))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "MiscLensFlare", &misclensflare))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "MiscParticle", &miscparticle))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "Wind", &wind))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Sound"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "InterfaceSound", &interfacesound))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "RailSound", &railsound))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "TrainSound", &trainsound))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "StructSound", &structsound))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "SurfaceSound", &surfacesound))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Misc"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "UseUndo", &useundo))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(g_ConfigVersion>=2.12f){
			if(!(str = BeginBlock(eee = str, "Stereoscopy"))) throw CSynErr(eee);
			if(!(str = AsgnYesNo(eee = str, "Enabled", &stereoenabled))) throw CSynErr(eee);
			if(!(str = AsgnInteger(eee = str, "Method", &stereomethod))) throw CSynErr(eee);
			ValueArea(&stereomethod, 0, 1);
			if(!(str = AsgnFloat(eee = str, "Interval", &stereointerval))) throw CSynErr(eee);
			ValueArea(&stereointerval, 0.0f, 1000.0f);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}

		if(!(str = BeginBlock(eee = str, "SelectedPlugin"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Rail", &g_SelectedRailID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Tie", &g_SelectedTieID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Girder", &g_SelectedGirderID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Pier", &g_SelectedPierID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Line", &g_SelectedLineID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Pole", &g_SelectedPoleID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Train", &g_SelectedTrainID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Station", &g_SelectedStationID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Struct", &g_SelectedStructID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Surface", &g_SelectedSurfaceID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Env", &g_SelectedEnvID))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Skin", &g_SelectedSkinID))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		
		g_ConfigScript = str;
	}
	catch(CSynErr err){
		err.Handle("\"Config.txt\"", g_ConfigBuffer);
		return false;
	}
	//DELETE_A(g_ConfigBuffer);
SET:
	m_HideTopPanel.SetCheck(hidetoppanel);
	m_HideRightPanel.SetCheck(hiderightpanel);
	m_WindowShadow.SetCheck(windowshadow);
	m_FullScreen.SetCheck(fullscreen);
	m_Resolution[0].ClearGroupCheck();
	if(g_PluginViewArg){
		g_DispWidth = 640;
		g_DispHeight = 480;
	}else{
		g_DispWidth = res[0];
		g_DispHeight = res[1];
	}
	for(i = 0; i<RES_MODE_NUM; i++) if(g_DispWidth==RES_MODE_WIDTH[i]
		&& g_DispHeight==RES_MODE_HEIGHT[i]) m_Resolution[i].SetCheck();
	g_FullScreen = !!fullscreen;
	m_MipMap[0].SetCheck(g_RailMipMap);
	m_MipMap[1].SetCheck(g_TrainMipMap);
	m_MipMap[2].SetCheck(g_StructMipMap);
	m_MipMap[3].SetCheck(g_SurfaceMipMap);
	m_Compass.SetCheck(compass);
	m_WindMeter.SetCheck(windmeter);
	m_ShowMap.SetCheck(showmap);
	m_Shadow.SetCheck(shadow);
	m_LinearFilter.SetCheck(linearfilter);
	m_EnvMap.SetCheck(envmap);
	m_SpecularLight.SetCheck(specularlight);
	m_SunLensFlare.SetCheck(sunlensflare);
	m_SunWhiteout.SetCheck(sunwhiteout);
	m_MiscLensFlare.SetCheck(misclensflare);
	m_MiscParticle.SetCheck(miscparticle);
	m_Wind.SetCheck(wind);
	m_InterfaceSound.SetCheck(interfacesound);
	m_PluginSound[0].SetCheck(railsound);
	m_PluginSound[1].SetCheck(trainsound);
	m_PluginSound[2].SetCheck(structsound);
	m_PluginSound[3].SetCheck(surfacesound);
	m_UseUndo.SetCheck(useundo);
	m_StereoEnabled.SetCheck(stereoenabled);
	m_StereoMethod[stereomethod].SetCheck();
	m_StereoIntervalEdit.SetText(FlashIn("%.2f", stereointerval));
	m_ConfigWindow1.SetPos((g_DispWidth-WW)/2-TILE_UNIT, (g_DispHeight-WH)/2);
	m_ConfigWindow2.SetPos((g_DispWidth-WW)/2-TILE_UNIT*2, (g_DispHeight-WH)/2+TILE_UNIT);
	return true;
}

/*
 *	設定保存
 */
bool CConfigMode::Save(){
	int tmp;
	if((tmp = m_Resolution->GetNumber())>=0){
		g_DispWidth = RES_MODE_WIDTH[tmp];
		g_DispHeight = RES_MODE_HEIGHT[tmp];
	}
	void SetCurrentPluginID();
	SetCurrentPluginID();
	if(chdir(g_BaseDir)) return false;
	FILE *file = fopen("Config.txt", "wt");
	if(!file) return false;
	fprintf(file, "/*\n *\tRailSim II Configuration Datafile\n */\n\n");
	fprintf(file, "DatafileHeader{\n");
	fprintf(file, "\tRailSimVersion = %.2f;\n", RAILSIM_VERSION);
	fprintf(file, "\tDatafileType = Config;\n");
	fprintf(file, "}\n\n");
	fprintf(file, "ConfigMode{\n");
	fprintf(file, "\tInterface{\n");
	fprintf(file, "\t\tHideTopPanel = %s;\n", YESNO[m_HideTopPanel.GetCheck()]);
	fprintf(file, "\t\tHideRightPanel = %s;\n", YESNO[m_HideRightPanel.GetCheck()]);
	fprintf(file, "\t\tWindowShadow = %s;\n", YESNO[m_WindowShadow.GetCheck()]);
	fprintf(file, "\t}\n");
	fprintf(file, "\tDevice{\n");
	fprintf(file, "\t\tFullScreen = %s;\n", YESNO[m_FullScreen.GetCheck()]);
	fprintf(file, "\t\tResolution = %d, %d;\n", g_DispWidth, g_DispHeight);
	fprintf(file, "\t\tRailMipMap = %s;\n", YESNO[m_MipMap[0].GetCheck()]);
	fprintf(file, "\t\tTrainMipMap = %s;\n", YESNO[m_MipMap[1].GetCheck()]);
	fprintf(file, "\t\tStructMipMap = %s;\n", YESNO[m_MipMap[2].GetCheck()]);
	fprintf(file, "\t\tSurfaceMipMap = %s;\n", YESNO[m_MipMap[3].GetCheck()]);
	fprintf(file, "\t}\n");
	fprintf(file, "\tAccessory{\n");
	fprintf(file, "\t\tCompass = %s;\n", YESNO[m_Compass.GetCheck()]);
	fprintf(file, "\t\tWindMeter = %s;\n", YESNO[m_WindMeter.GetCheck()]);
	fprintf(file, "\t\tShowMap = %s;\n", YESNO[m_ShowMap.GetCheck()]);
	fprintf(file, "\t}\n");
	fprintf(file, "\tEffect{\n");
	fprintf(file, "\t\tShadow = %s;\n", YESNO[m_Shadow.GetCheck()]);
	fprintf(file, "\t\tLinearFilter = %s;\n", YESNO[m_LinearFilter.GetCheck()]);
	fprintf(file, "\t\tEnvMap = %s;\n", YESNO[m_EnvMap.GetCheck()]);
	fprintf(file, "\t\tSpecularLight = %s;\n", YESNO[m_SpecularLight.GetCheck()]);
	fprintf(file, "\t\tSunLensFlare = %s;\n", YESNO[m_SunLensFlare.GetCheck()]);
	fprintf(file, "\t\tSunWhiteout = %s;\n", YESNO[m_SunWhiteout.GetCheck()]);
	fprintf(file, "\t\tMiscLensFlare = %s;\n", YESNO[m_MiscLensFlare.GetCheck()]);
	fprintf(file, "\t\tMiscParticle = %s;\n", YESNO[m_MiscParticle.GetCheck()]);
	fprintf(file, "\t\tWind = %s;\n", YESNO[m_Wind.GetCheck()]);
	fprintf(file, "\t}\n");
	fprintf(file, "\tSound{\n");
	fprintf(file, "\t\tInterfaceSound = %s;\n", YESNO[m_InterfaceSound.GetCheck()]);
	fprintf(file, "\t\tRailSound = %s;\n", YESNO[m_PluginSound[0].GetCheck()]);
	fprintf(file, "\t\tTrainSound = %s;\n", YESNO[m_PluginSound[1].GetCheck()]);
	fprintf(file, "\t\tStructSound = %s;\n", YESNO[m_PluginSound[2].GetCheck()]);
	fprintf(file, "\t\tSurfaceSound = %s;\n", YESNO[m_PluginSound[3].GetCheck()]);
	fprintf(file, "\t}\n", YESNO[m_Shadow.GetCheck()]);
	fprintf(file, "\tMisc{\n");
	fprintf(file, "\t\tUseUndo = %s;\n", YESNO[m_UseUndo.GetCheck()]);
	fprintf(file, "\t}\n", YESNO[m_Shadow.GetCheck()]);
	fprintf(file, "\tStereoscopy{\n");
	fprintf(file, "\t\tEnabled = %s;\n", YESNO[m_StereoEnabled.GetCheck()]);
	fprintf(file, "\t\tMethod = %d;\n", m_StereoMethod->GetNumber());
	fprintf(file, "\t\tInterval = %.2f;\n", GetStereoInterval());
	fprintf(file, "\t}\n", YESNO[m_Shadow.GetCheck()]);
	fprintf(file, "\tSelectedPlugin{\n");
	fprintf(file, "\t\tRail = \"%s\";\n", (char *)g_SelectedRailID.c_str());
	fprintf(file, "\t\tTie = \"%s\";\n", (char *)g_SelectedTieID.c_str());
	fprintf(file, "\t\tGirder = \"%s\";\n", (char *)g_SelectedGirderID.c_str());
	fprintf(file, "\t\tPier = \"%s\";\n", (char *)g_SelectedPierID.c_str());
	fprintf(file, "\t\tLine = \"%s\";\n", (char *)g_SelectedLineID.c_str());
	fprintf(file, "\t\tPole = \"%s\";\n", (char *)g_SelectedPoleID.c_str());
	fprintf(file, "\t\tTrain = \"%s\";\n", (char *)g_SelectedTrainID.c_str());
	fprintf(file, "\t\tStation = \"%s\";\n", (char *)g_SelectedStationID.c_str());
	fprintf(file, "\t\tStruct = \"%s\";\n", (char *)g_SelectedStructID.c_str());
	fprintf(file, "\t\tSurface = \"%s\";\n", (char *)g_SelectedSurfaceID.c_str());
	fprintf(file, "\t\tEnv = \"%s\";\n", (char *)g_SelectedEnvID.c_str());
	fprintf(file, "\t\tSkin = \"%s\";\n", (char *)g_SelectedSkinID.c_str());
	fprintf(file, "\t}\n");
	fprintf(file, "}\n\n");
	SaveModeSettings(file);
	fclose(file);
	return true;
}

/*
 *	テクスチャフィルタ設定
 */
void CConfigMode::SetTexFilter(){
	if(m_LinearFilter.GetCheck()){
		devTEX_LINEAR(0);
		devTEX_LINEAR(1);
	}else{
		devTEX_POINT(0);
		devTEX_POINT(1);
	}
}

/*
 *	ステレオ間隔取得
 */
float CConfigMode::GetStereoInterval(){
	float interval = 0.0f;
	m_StereoIntervalEdit.FinishInput();
	sscanf(m_StereoIntervalEdit.GetText(), "%f", &interval);
	ValueArea(&interval, 0.0f, 1000.0f);
	return interval;
}

/*
 *	ハードウェア互換性のチェック
 */
void CConfigMode::CheckHardware(){
	int i;
	if(!g_StencilEnabled && GetShadow()){
		EnqueueCommonDialog(new CSimpleDialog(
			lang(StencilBufferProblem), lang(CompatibilityProblem)));
		m_Shadow.SetCheck(0);
	}
	if(!svs.pDS && (GetInterfaceSound() || GetRailSound()
		|| GetTrainSound() || GetStructSound() || GetSurfaceSound())){
		EnqueueCommonDialog(new CSimpleDialog(
			lang(DirectSoundProblem), lang(CompatibilityProblem)));
		m_InterfaceSound.SetCheck(0);
		for(i = 0; i<4; i++) m_PluginSound[i].SetCheck(0);
	}
}

/*
 *	画面分割関連初期化
 */
void CConfigMode::FreeWindowDiv(){
	m_RootWindow.Free();
	CWindowDivInfo::InitState();
	m_ActiveWindow = NULL;
}
