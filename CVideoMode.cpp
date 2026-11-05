#include "stdafx.h"
#include "Capture.h"
#include "CSkinPlugin.h"
#include "CVideoMode.h"
#include "CConfigMode.h"

//	内部定数
const int WW = 512, WH = 384;	//	窓サイズ

//	内部グローバル
char *g_VideoStateString[2] = {lang(Paused), lang(Recording)};

/*
 *	コンストラクタ
 */
CVideoMode::CVideoMode(){
	int wide = WW-TILE_UNIT*2, win = wide-TILE_UNIT*2;
	m_VideoWindow.Init((g_DispWidth-WW)/2-TILE_UNIT, (g_DispHeight-WH)/2,
		WW, WH, lang(Video), &m_Interface, true);

	m_PictureGroup.Init(TILE_UNIT, TILE_UNIT*2,
		wide, TILE_UNIT*5, lang(CaptureScreenShot), &m_VideoWindow);
	m_PictureExpr.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, wide-TILE_UNIT*2, TILE_UNIT,
		 FlashIn("%s F11: %s / F12: %s", lang(Kome), lang(ChangeCapMode),
		 lang(CaptureScreenShot)), &m_PictureGroup, 0, 1);
	m_PictureInfo.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD,
		wide-TILE_UNIT*2, TILE_UNIT, "", &m_PictureGroup, 0, 1);
	int i, trw = WW-TILE_UNIT*12;
	m_QualityLabel.Init(TILE_UNIT, TILE_UNIT*3+TILE_QUAD, TILE_UNIT*8, TILE_UNIT,
		lang(PictureQuality), &m_PictureGroup, 0, 1);
	string qtystr[4] = {lang(Quality1), lang(Quality2), lang(Quality4), lang(Quality8)};
	for(i = 0; i<4; i++) m_Quality[i].Init(
		TILE_UNIT*9+trw*i/4, TILE_UNIT*3+TILE_QUAD,
		trw*(i+1)/4-trw*i/4-TILE_HALF, TILE_UNIT,
		(char *)qtystr[i].c_str(), &m_PictureGroup, i ? &m_Quality[i-1] : NULL);

	m_VideoGroup.Init(TILE_UNIT, TILE_UNIT*8,
		wide, TILE_UNIT*9, lang(RecordVideo), &m_VideoWindow);
	m_VideoExpr.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, wide-TILE_UNIT*2, TILE_UNIT,
		 FlashIn("%s Ctrl + F12: %s / Ctrl + Shift + F12: %s", lang(Kome),
		 lang(BeginRecording), lang(StopRecording)), &m_VideoGroup, 0, 1);
	m_VideoInfo.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD,
		wide-TILE_UNIT*2, TILE_UNIT, "", &m_VideoGroup, 0, 1);
	m_StartButton.Init(TILE_UNIT, TILE_UNIT*3+TILE_HALF+TILE_QUAD,
		TILE_UNIT*5, TILE_UNIT, lang(BeginRecording), &m_VideoGroup);
	m_StartButton.SetSound(false);
	m_StopButton.Init(TILE_UNIT*6+TILE_HALF, TILE_UNIT*3+TILE_HALF+TILE_QUAD,
		TILE_UNIT*5, TILE_UNIT, lang(StopRecording), &m_VideoGroup);
	m_StopButton.SetSound(false);
	m_RewindButton.Init(TILE_UNIT*11+TILE_HALF*2, TILE_UNIT*3+TILE_HALF+TILE_QUAD,
		TILE_UNIT*5, TILE_UNIT, lang(Rewind), &m_VideoGroup);
	m_ForwardButton.Init(TILE_UNIT*16+TILE_HALF*3, TILE_UNIT*3+TILE_HALF+TILE_QUAD,
		TILE_UNIT*5, TILE_UNIT, lang(Forward), &m_VideoGroup);
	int tw = wide-TILE_UNIT*2+TILE_HALF;
	m_ExceptPause.Init(TILE_UNIT, TILE_UNIT*5+TILE_QUAD,
		tw/2-TILE_HALF, TILE_UNIT, lang(DontRecordWhilePaused), &m_VideoGroup);
	m_OnlyPhotoMode.Init(TILE_UNIT+tw/2, TILE_UNIT*5+TILE_QUAD,
		tw-tw/2-TILE_HALF, TILE_UNIT, lang(RecordOnlyInRecordingMode), &m_VideoGroup);
	m_ExceptPause.SetCheck(1);
	m_OnlyPhotoMode.SetCheck(1);
	m_DownsampleLabel.Init(TILE_UNIT, TILE_UNIT*6+TILE_QUAD, TILE_UNIT*8, TILE_UNIT,
		"Downsample", &m_VideoGroup, 0, 1);
	string dnsplstr[3] = {"None", "1/2", "1/4"};
	for(i = 0; i<3; i++) m_Downsample[i].Init(
		TILE_UNIT*9+trw*i/3, TILE_UNIT*6+TILE_QUAD,
		trw*(i+1)/3-trw*i/3-TILE_HALF, TILE_UNIT,
		(char *)dnsplstr[i].c_str(), &m_VideoGroup, i ? &m_Downsample[i-1] : NULL);
	m_FormatLabel.Init(TILE_UNIT, TILE_UNIT*7+TILE_QUAD, TILE_UNIT*8, TILE_UNIT,
		"VideoFormat", &m_VideoGroup, 0, 1);
	string vfmtstr[3] = {"BMP", "AVI"};
	for(i = 0; i<2; i++) m_Format[i].Init(
		TILE_UNIT*9+trw*i/2, TILE_UNIT*7+TILE_QUAD,
		trw*(i+1)/2-trw*i/2-TILE_HALF, TILE_UNIT,
		(char *)vfmtstr[i].c_str(), &m_VideoGroup, i ? &m_Format[i-1] : NULL);
//	m_VideoSound.Init(TILE_UNIT, TILE_UNIT*8+TILE_QUAD,
//		tw/2-TILE_HALF, TILE_UNIT, "VideoSound (AVI only)", &m_VideoGroup);
//	m_VideoSound.SetCheck(1);
}

/*
 *	設定読込
 */
char *CVideoMode::LoadInterfaceSetting(
	char *str	//	対象文字列
){
	char *eee;
	int picturequality = 2;
	bool exceptpause = true, onlyphotomode = true;
	if(str && g_ConfigVersion>=2.03f){
		if(!(str = BeginBlock(eee = str, "VideoMode"))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "PictureQuality", &picturequality))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "ExceptPause", &exceptpause))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "OnlyPhotoMode", &onlyphotomode))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}
	m_Quality[picturequality].SetCheck();
	m_ExceptPause.SetCheck(exceptpause);
	m_OnlyPhotoMode.SetCheck(onlyphotomode);
	return str;
}

/*
 *	設定保存
 */
void CVideoMode::SaveInterfaceSetting(
	FILE *file	//	ファイル
){
	fprintf(file, "VideoMode{\n");
	fprintf(file, "\tPictureQuality = %d;\n", m_Quality->GetNumber());
	fprintf(file, "\tExceptPause = %s;\n", YESNO[m_ExceptPause.GetCheck()]);
	fprintf(file, "\tOnlyPhotoMode = %s;\n", YESNO[m_OnlyPhotoMode.GetCheck()]);
	fprintf(file, "}\n\n");
}

/*
 *	モードを有効化
 */
void CVideoMode::EnterInterface(){
	ms_ModeLabel = lang(Video);
	m_PictureInfo.SetText("");
	m_VideoInfo.SetText("");
}

/*
 *	入力チェック
 */
void CVideoMode::ScanInputInterface(){
	m_Interface.ScanInput();
	if(m_VideoWindow.CheckClose()){
		SetNeutral();
		return;
	}
	if(m_StartButton.IsPushed()) StartVideoCapture();
	if(m_StopButton.IsPushed()) StopVideoCapture();
	if(m_RewindButton.IsPushed()) g_VideoFrame = 0;
	if(m_ForwardButton.IsPushed()) CountVideoBMP();
	m_PictureInfo.SetText(FlashIn("%s: %d", lang(Count), g_PictureCount));
	m_VideoInfo.SetText(FlashIn("%s: %d / %s", lang(Frame),
		g_VideoFrame, g_VideoStateString[g_VideoState]));
}

/*
 *	撮影画質取得
 */
int CVideoMode::GetPictureQuality(){
	return 1<<m_Quality->GetNumber();
}
