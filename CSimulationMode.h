#ifndef CSIMULATIONMODE_H_INCLUDED
#define CSIMULATIONMODE_H_INCLUDED

#include "CStaticCtrl.h"
#include "CGroupBox.h"
#include "CRadioButton.h"
#include "CCheckBox.h"
#include "CInterfaceMode.h"

const int TIME_SCALE_NUM = 10;
const int SIM_SPEED_NUM = 5;

/*
 *	シミュレーション設定モード
 */
class CSimulationMode: public CInterfaceMode{
	friend class CSaveFile;
private:
	int m_OldSpeed;								//	ポーズ前の速度
	CWindowCtrl m_SimulationWindow;				//	窓
	CGroupBox m_ScaleGroup;						//	スケールグループ
	CRadioButton m_TimeScale[TIME_SCALE_NUM];	//	時間軸スケール
	CGroupBox m_SpeedGroup;						//	速度グループ
	CRadioButton m_SimSpeed[SIM_SPEED_NUM];		//	シミュレーション速度
	CGroupBox m_TimeGroup;						//	時間経過
	CStaticCtrl m_RotLabel;						//	地球の自転
	CRadioButton m_EarthRotation[3];			//	自転チェック
	CStaticCtrl m_RevLabel;						//	地球の公転
	CRadioButton m_EarthRevolution[5];			//	公転チェック
	CGroupBox m_MiscGroup;						//	時間経過
	CCheckBox m_ManualControl;					//	マニュアル操作
	CCheckBox m_IgnoreAcceleration;				//	加減速度を無視
	CStaticCtrl m_NoteLabel;					//	注意書き
public:
	CSimulationMode();
	~CSimulationMode(){}
	void EnterInterface();
	void ScanInputInterface();
	void InitTimeOption();
	int GetEarthRotation(){ return m_EarthRotation->GetNumber(); }
	int GetEarthRevolution(){ return m_EarthRevolution->GetNumber(); }
	int GetTimeScale();
	int GetSimSpeed();
	int GetOldSpeed(){ return m_OldSpeed; }
	void SetSimSpeed(int);
	void TogglePause();
	int GetManualControl(){ return m_ManualControl.GetCheck(); }
	int GetIgnoreAcceleration(){ return m_IgnoreAcceleration.GetCheck(); }
};

//	外部グローバル
extern CSimulationMode *g_SimulationMode;

#endif
