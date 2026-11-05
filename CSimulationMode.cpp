#include "stdafx.h"
#include "CSkinPlugin.h"
#include "CSimulationMode.h"

//	内部定数
const int WW = 512, WH = 384;	//	窓サイズ
const int TIME_SCALE_MAX = 10;

//	内部グローバル
bool g_ManualControl = false;
bool g_IgnoreAcceleration = false;

const int SCALE_TABLE[TIME_SCALE_NUM] = {
	2880, 1400, 480, 288, 144, 72, 48, 24, 6, 1};	//	実世界 1 秒あたりゲーム内秒数
const int SPEED_TABLE[SIM_SPEED_NUM] = {
	0, 1, 2, 10, 100};	//	シミュレーション倍速

/*
 *	ゲーム内 1 日あたりフレーム数
 *	900, 1800, 5400, 9000, 18000, 36000, 54000, 108000, 432000, 2592000
 */

/*
 *	コンストラクタ
 */
CSimulationMode::CSimulationMode(){
	int i, wide = WW-TILE_UNIT*2, win = wide-TILE_UNIT*2;
	m_SimulationWindow.Init((g_DispWidth-WW)/2-TILE_UNIT, (g_DispHeight-WH)/2,
		WW, WH, lang(SimulationSetting), &m_Interface, true);
	m_ScaleGroup.Init(TILE_UNIT, TILE_UNIT*2, wide, TILE_UNIT*4,
		lang(TimeScale_Exp), &m_SimulationWindow);
	string dlstr[TIME_SCALE_MAX] = {
		FlashIn("30 %s", lang(Sec)), FlashIn("1 %s", lang(Min)),
		FlashIn("3 %s", lang(Min)), FlashIn("5 %s (%s)", lang(Min), lang(Init)),
		FlashIn("10 %s", lang(Min)), FlashIn("20 %s", lang(Min)),
		FlashIn("30 %s", lang(Min)), FlashIn("1 %s", lang(Hour)),
		FlashIn("4 %s", lang(Hour)), FlashIn("24 %s", lang(Hour))
	};
	for(i = 0; i<5; i++) m_TimeScale[i].Init(
		TILE_UNIT+win*i/5, TILE_UNIT+TILE_QUAD,
		win*(i+1)/5-win*i/5-TILE_HALF, TILE_UNIT,
		(char *)dlstr[i].c_str(), &m_ScaleGroup, i ? &m_TimeScale[i-1] : NULL);
	for(i = 5; i<TIME_SCALE_MAX; i++) m_TimeScale[i].Init(
		TILE_UNIT+win*(i-5)/5, TILE_UNIT*2+TILE_QUAD,
		win*(i-4)/5-win*(i-5)/5-TILE_HALF, TILE_UNIT,
		(char *)dlstr[i].c_str(), &m_ScaleGroup, &m_TimeScale[i-1]);
	m_SpeedGroup.Init(TILE_UNIT, TILE_UNIT*7,
		wide, TILE_UNIT*3, lang(SimulationSpeed), &m_SimulationWindow);
	string ffstr[5] = {
		lang(Paused), lang(UnitSp),
		FlashIn("2 %s", lang(XSpeed)),
		FlashIn("10 %s", lang(XSpeed)),
		FlashIn("100 %s", lang(XSpeed))
	};
	for(i = 0; i<5; i++) m_SimSpeed[i].Init(
		TILE_UNIT+win*i/5, TILE_UNIT+TILE_QUAD,
		win*(i+1)/5-win*i/5-TILE_HALF, TILE_UNIT,
		(char *)ffstr[i].c_str(), &m_SpeedGroup, i ? &m_SimSpeed[i-1] : NULL);
	m_TimeGroup.Init(TILE_UNIT, TILE_UNIT*11,
		WW-TILE_UNIT*2, TILE_UNIT*4, lang(TimeProgress), &m_SimulationWindow);
	m_RotLabel.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, TILE_UNIT*8, TILE_UNIT,
		lang(Rotation_Time), &m_TimeGroup, 0, 1);
	int trw = WW-TILE_UNIT*12;
	string rotstr[3] = {lang(Auto), lang(Daytime), lang(Night)};
	for(i = 0; i<3; i++) m_EarthRotation[i].Init(
		TILE_UNIT*9+trw*i/5, TILE_UNIT+TILE_QUAD,
		trw*(i+1)/5-trw*i/5-TILE_HALF, TILE_UNIT,
		(char *)rotstr[i].c_str(), &m_TimeGroup, i ? &m_EarthRotation[i-1] : NULL);
	m_RevLabel.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, TILE_UNIT*8, TILE_UNIT,
		lang(Revolution_Season), &m_TimeGroup, 0, 1);
	string revstr[5] = {lang(Auto), lang(Spring), lang(Summer), lang(Autumn), lang(Winter)};
	for(i = 0; i<5; i++) m_EarthRevolution[i].Init(
		TILE_UNIT*9+trw*i/5, TILE_UNIT*2+TILE_QUAD,
		trw*(i+1)/5-trw*i/5-TILE_HALF, TILE_UNIT,
		(char *)revstr[i].c_str(), &m_TimeGroup, i ? &m_EarthRevolution[i-1] : NULL);
	m_MiscGroup.Init(TILE_UNIT, TILE_UNIT*16,
		WW-TILE_UNIT*2, TILE_UNIT*4, lang(Miscellaneous), &m_SimulationWindow);
	m_ManualControl.Init(TILE_UNIT, TILE_UNIT+TILE_QUAD, WW-TILE_UNIT*4, TILE_UNIT,
		lang(ManualControl), &m_MiscGroup);
	m_IgnoreAcceleration.Init(TILE_UNIT, TILE_UNIT*2+TILE_QUAD, WW-TILE_UNIT*4, TILE_UNIT,
		lang(IgnoreAcceleration), &m_MiscGroup);
	m_NoteLabel.Init(TILE_UNIT, WH-TILE_UNIT*2, wide, TILE_UNIT,
		lang(SavedInEveryFile), &m_SimulationWindow, 0, 1);
	InitTimeOption();
}

/*
 *	モードを有効化
 */
void CSimulationMode::EnterInterface(){
	ms_ModeLabel = lang(SimulationSetting);
	int i;
	for(i = 0; i<TIME_SCALE_MAX; i++) m_TimeScale[i].Enable(!g_NetworkInitialized);
	m_ManualControl.Enable(!g_NetworkInitialized);
	m_IgnoreAcceleration.Enable(!g_NetworkInitialized);
}

/*
 *	入力チェック
 */
void CSimulationMode::ScanInputInterface(){
	m_Interface.ScanInput();
	if(m_SimulationWindow.CheckClose()){
		SetNeutral();
		return;
	}
	int sp = m_SimSpeed->GetNumber();
	if(sp) m_OldSpeed = sp;
}

/*
 *	時間設定初期化
 */
void CSimulationMode::InitTimeOption(){
	m_TimeScale[3].SetCheck();
	m_SimSpeed[m_OldSpeed = 1].SetCheck();
	m_EarthRotation[0].SetCheck();
	m_EarthRevolution[0].SetCheck();
	m_ManualControl.SetCheck(0);
	m_IgnoreAcceleration.SetCheck(0);
}

/*
 *	時間軸スケール取得
 */
int CSimulationMode::GetTimeScale(){
	return SCALE_TABLE[m_TimeScale->GetNumber()];
}

/*
 *	シミュレーション速度取得
 */
int CSimulationMode::GetSimSpeed(){
	return SPEED_TABLE[m_SimSpeed->GetNumber()];
}

/*
 *	シミュレーション速度設定
 */
void CSimulationMode::SetSimSpeed(
	int sp	//	スピード
){
	if(0<=sp && sp<SIM_SPEED_NUM) m_SimSpeed[sp].SetCheck();
}

/*
 *	ポーズ切り替え
 */
void CSimulationMode::TogglePause(){
	int sp = m_SimSpeed->GetNumber();
	if(sp){
		m_OldSpeed = sp;
		m_SimSpeed[0].SetCheck();
	}else{
		m_SimSpeed[m_OldSpeed].SetCheck();
	}
}
