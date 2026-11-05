#include "stdafx.h"
#include "CCursor.h"
#include "CSkinPlugin.h"
#include "CSkinSelectMode.h"
#include "CConfigMode.h"

//	内部定数
const int INTERFACE_SOUND_VOLUME = -1500;	//	操作音量

//	内部グローバル
CObject g_ArrowObject;		//	矢印オブジェクト
CObject g_LinkObject;		//	矢印オブジェクト
CObject g_SegmentObject;	//	セグメントオブジェクト
CObject g_CompassObject[2];	//	コンパスオブジェクト
CObject g_WindDirObject;	//	風方向オブジェクト
CSkinPlugin *g_Skin = NULL;	//	カレントスキン

/*
 *	コンストラクタ
 */
CSkinPlugin::CSkinPlugin(
	char *id	//	ID
):
	CPlugin(id)	//	基本クラス
{
	m_NormalCursorData.m_Cursor2DAnimFrame = NULL;
	m_InterfaceData.m_hFont = NULL;
	int i;
	for(i = 0; i<4; i++) m_ResizeCursorData[i].m_Cursor2DAnimFrame = NULL;
}

/*
 *	デストラクタ
 */
CSkinPlugin::~CSkinPlugin(){
	DELETE_A(m_NormalCursorData.m_Cursor2DAnimFrame);
	int i;
	for(i = 0; i<4; i++) DELETE_A(m_ResizeCursorData[i].m_Cursor2DAnimFrame);
	DeleteObject(m_InterfaceData.m_hFont);
}

/*
 *	読込
 */
bool CSkinPlugin::Load(){
	char *str = m_Script, *eee;
	int i;
	if(!ChDir()) return false;
	if(m_State==3) goto LOADED;
	if(!m_Script) return false;
	try{
		if(!(str = m_NormalCursorData.Read(eee = str, "NormalCursor"))) throw CSynErr(eee);
		for(i = 0; i<4; i++) if(!(str = m_ResizeCursorData[i].Read(
			eee = str, FlashIn("ResizeCursor%d", i+1)))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "Interface"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "TexFileName",
			&m_InterfaceData.m_TexFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "FontName",
			&m_InterfaceData.m_FontName))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "TitleBarFontColor",
			&m_InterfaceData.m_TitleBarFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "ButtonFontColor",
			&m_InterfaceData.m_ButtonFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "StaticFontColor",
			&m_InterfaceData.m_StaticFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "FocusFrameColor",
			&m_InterfaceData.m_FocusFrameColor))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Background"))) throw CSynErr(eee);
		bool wptmp;
		if(!(str = AsgnYesNo(eee = str, "UseWallpaper", &wptmp))) throw CSynErr(eee);
		if(wptmp){
			if(!(str = AsgnString(eee = str, "TexFileName",
				&m_BackgroundData.m_TexFileName))) throw CSynErr(eee);
			m_BackgroundData.m_BackgroundColor = 0;
			if(!(str = AsgnInteger(eee = str, "ImageSize",
				m_BackgroundData.m_ImageSize, 2, false))) throw CSynErr(eee);
		}else{
			if(!(str = AsgnColor(eee = str, "BackgroundColor",
				&m_BackgroundData.m_BackgroundColor))) throw CSynErr(eee);
			m_BackgroundData.m_ImageSize[0] = 0;
			m_BackgroundData.m_ImageSize[1] = 0;
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Frame"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "FrameTexFileName",
			&m_FrameData.m_FrameTexFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "IconTexFileName",
			m_FrameData.m_IconTexFileName, MODE_NUM, false))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "LabelFontColor",
			&m_FrameData.m_LabelFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "InfoFontColor",
			&m_FrameData.m_InfoFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "FloatFontColor",
			&m_FrameData.m_FloatFontColor))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "EditCtrl"))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultFontColor",
			&m_EditCtrlData.m_DefaultFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "EditBaseColor",
			m_EditCtrlData.m_EditBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "EditFontColor",
			&m_EditCtrlData.m_EditFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "ConvertFontColor",
			&m_EditCtrlData.m_ConvertFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "ConvertClauseColor",
			m_EditCtrlData.m_ConvertClauseColor, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedBaseColor",
			m_EditCtrlData.m_SelectedBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "ListView"))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultBaseColorOdd",
			m_ListViewData.m_DefaultBaseColorOdd, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultBaseColorEven",
			m_ListViewData.m_DefaultBaseColorEven, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultFontColor",
			&m_ListViewData.m_DefaultFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedBaseColor",
			m_ListViewData.m_SelectedBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedFontColor",
			&m_ListViewData.m_SelectedFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "FocusFrameColor",
			&m_ListViewData.m_FocusFrameColor))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "PluginTree"))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultBaseColor",
			m_PluginTreeData.m_DefaultBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultFontColor",
			&m_PluginTreeData.m_DefaultFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedBaseColor",
			m_PluginTreeData.m_SelectedBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedFontColor",
			&m_PluginTreeData.m_SelectedFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "FocusFrameColor",
			&m_PluginTreeData.m_FocusFrameColor))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "PopupMenu"))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DefaultFontColor",
			&m_PopupMenuData.m_DefaultFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DisabledFontColor",
			&m_PopupMenuData.m_DisabledFontColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "DisabledShadowColor",
			&m_PopupMenuData.m_DisabledShadowColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedBaseColor",
			m_PopupMenuData.m_SelectedBaseColor, 4, true))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "SelectedFontColor",
			&m_PopupMenuData.m_SelectedFontColor))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Model"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ArrowModelFileName",
			&m_ModelData.m_ArrowModelFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "ArrowModelScale",
			&m_ModelData.m_ArrowModelScale))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "LinkModelFileName",
			&m_ModelData.m_LinkModelFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "LinkModelScale",
			&m_ModelData.m_LinkModelScale))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "SegmentModelFileName",
			&m_ModelData.m_SegmentModelFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "SegmentModelScale",
			&m_ModelData.m_SegmentModelScale))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "CompassModelFileName",
			m_ModelData.m_CompassModelFileName, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "CompassModelScale",
			m_ModelData.m_CompassModelScale, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "WindDirModelFileName",
			&m_ModelData.m_WindDirModelFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "WindDirModelScale",
			&m_ModelData.m_WindDirModelScale))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Sound"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "MouseDownWaveFileName",
			&m_SoundData.m_MouseDownWaveFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "MouseUpWaveFileName",
			&m_SoundData.m_MouseUpWaveFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ErrorWaveFileName",
			&m_SoundData.m_ErrorWaveFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ScreenShotWaveFileName",
			&m_SoundData.m_ScreenShotWaveFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "VideoStartWaveFileName",
			&m_SoundData.m_VideoStartWaveFileName))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "VideoStopWaveFileName",
			&m_SoundData.m_VideoStopWaveFileName))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
LOADED:
	m_InterfaceData.m_hFont = CreateFont(FONT_HEIGHT, 0, 0, 0,
		FW_REGULAR, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH|FF_MODERN, m_InterfaceData.m_FontName.c_str());
	m_InterfaceTexture = g_TexList.Get(FALSE, m_InterfaceData.m_TexFileName.c_str(), 0, 1);
	m_FrameTexture = g_TexList.Get(FALSE, m_FrameData.m_FrameTexFileName.c_str(), 0, 1);
	for(i = 0; i<MODE_NUM; i++) m_IconTexture[i] =
		g_TexList.Get(FALSE, m_FrameData.m_IconTexFileName[i].c_str(), 0, 1);
	m_NormalCursorData.LoadData();
	for(i = 0; i<4; i++) m_ResizeCursorData[i].LoadData();
	if(*m_BackgroundData.m_ImageSize) m_WallpaperTexture =
		g_TexList.Get(FALSE, m_BackgroundData.m_TexFileName.c_str(), 0, 1);
	m_ArrowMesh = g_MeshList.Get(FALSE, m_ModelData.m_ArrowModelFileName.c_str());
	ChDir();
	m_LinkMesh = g_MeshList.Get(FALSE, m_ModelData.m_LinkModelFileName.c_str());
	ChDir();
	m_SegmentMesh = g_MeshList.Get(FALSE, m_ModelData.m_SegmentModelFileName.c_str());
	ChDir();
	for(i = 0; i<2; i++){
		m_CompassMesh[i] = g_MeshList.Get(
			FALSE, m_ModelData.m_CompassModelFileName[i].c_str());
		ChDir();
	}
	m_WindDirMesh = g_MeshList.Get(FALSE, m_ModelData.m_WindDirModelFileName.c_str());
	ChDir();
	m_MouseDown.Load((char *)m_SoundData.m_MouseDownWaveFileName.c_str(), 4, false);
	m_MouseUp.Load((char *)m_SoundData.m_MouseUpWaveFileName.c_str(), 2, false);
	m_Error.Load((char *)m_SoundData.m_ErrorWaveFileName.c_str(), 1, false);
	m_ScreenShot.Load((char *)m_SoundData.m_ScreenShotWaveFileName.c_str(), 2, false);
	m_VideoStart.Load((char *)m_SoundData.m_VideoStartWaveFileName.c_str(), 1, false);
	m_VideoStop.Load((char *)m_SoundData.m_VideoStopWaveFileName.c_str(), 1, false);
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CSkinPlugin::SetPreview(){
	ms_PreviewState = true;
	if(g_Skin!=this){
		g_Skin = this;
		g_StrTex->SetFont(m_InterfaceData.m_hFont);
		g_ArrowObject.SetMesh(m_ArrowMesh, V3ZERO, m_ModelData.m_ArrowModelScale);
		g_LinkObject.SetMesh(m_LinkMesh, V3ZERO, m_ModelData.m_LinkModelScale);
		g_SegmentObject.SetMesh(m_SegmentMesh, V3ZERO, m_ModelData.m_SegmentModelScale);
		int i;
		for(i = 0; i<2; i++) g_CompassObject[i].SetMesh(
			m_CompassMesh[i], V3ZERO, m_ModelData.m_CompassModelScale[i]);
		g_WindDirObject.SetMesh(m_WindDirMesh, V3ZERO, m_ModelData.m_WindDirModelScale);
	}
	if(!g_SkinSelectMode) return;
	string desc = g_Skin->GetBasicInfo();
	desc += "\n"+g_Skin->GetDescription();
	g_SkinSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	マウス PUSH 音
 */
void CSkinPlugin::MouseDown(){
	if(g_ConfigMode->GetInterfaceSound()) m_MouseDown.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	マウス PULL 音
 */
void CSkinPlugin::MouseUp(){
	if(g_ConfigMode->GetInterfaceSound()) m_MouseUp.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	エラー音
 */
void CSkinPlugin::Error(){
	if(g_ConfigMode->GetInterfaceSound()) m_Error.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	スクリーンショット撮影音
 */
void CSkinPlugin::ScreenShot(){
	if(g_ConfigMode->GetInterfaceSound()) m_ScreenShot.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	ビデオ撮影開始音
 */
void CSkinPlugin::VideoStart(){
	if(g_ConfigMode->GetInterfaceSound()) m_VideoStart.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	ビデオ撮影停止音
 */
void CSkinPlugin::VideoStop(){
	if(g_ConfigMode->GetInterfaceSound()) m_VideoStop.Add(V3ZERO, INTERFACE_SOUND_VOLUME);
}

/*
 *	背景描画
 */
void CSkinPlugin::DrawBackground(
	bool light	//	背景ライト
){
	D3DCOLOR lc = light ? 0xffffffff : 0xff202020;
	D3DCOLOR bg = MultiplyColor(m_BackgroundData.m_BackgroundColor, lc);
	BeginScene(bg);
	if(*m_BackgroundData.m_ImageSize){
		devSetTexture(0, m_WallpaperTexture);
		if(m_BackgroundData.m_ImageSize[0]<0) SetUVMap(0.0f, 0.0f, 1.0f, 1.0f);
		else SetUVMap(0.0f, 0.0f,
			(float)g_DispWidth/m_BackgroundData.m_ImageSize[0],
			(float)g_DispHeight/m_BackgroundData.m_ImageSize[1]);
		TexMap2DRect(0.0f, 0.0f, g_DispWidth, g_DispHeight, lc);
	}
}

/*
 *	コンパススケール設定
 */
void CSkinPlugin::ScaleCompass(
	float scale	//	スケール
){
	int i;
	for(i = 0; i<2; i++) g_CompassObject[i].SetMesh(
		m_CompassMesh[i], V3ZERO, scale*m_ModelData.m_CompassModelScale[i]);
	g_WindDirObject.SetMesh(m_WindDirMesh, V3ZERO, scale*m_ModelData.m_WindDirModelScale);
}
