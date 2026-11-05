#ifndef CGAMEMODE_H_INCLUDED
#define CGAMEMODE_H_INCLUDED

#include "CCursor.h"
#include "CCamera.h"
#include "CInterface.h"

class CToggleIcon;

/*
 *	ゲームモード
 */
class CGameMode{
protected:
	static int ms_TopPanelTime;					//	上パネル表示時間
	static int ms_RightPanelTime;				//	右パネル表示時間
	static float ms_TopPanelShow;				//	上パネル表示度
	static float ms_RightPanelShow;				//	右パネル表示度
	static float ms_WindDirTemp;				//	風力計変数
	static string ms_ModeLabel;					//	モードラベル
	static CGameMode *ms_ActiveMode;			//	現在のモード
	static CToggleIcon *ms_MenuIcon[MODE_NUM];	//	モードアイコン
	CInterface m_Interface;	//	統括インターフェイス
public:
	static void WakeUp();
	static void InitMenu();
	static void MainLoop();
	static void LoadModeSettings();
	static void SaveModeSettings(FILE *);
	static void SetNeutral();
	static void Exit(){ ms_ActiveMode = NULL; }
	bool ScanInputFrame(int);
	void RenderFrame(int);
	void RenderCompass();
	bool RenderDialog();
	CGameMode();
	virtual ~CGameMode(){}
	bool IsModeActive(){ return ms_ActiveMode==this; }
	virtual char *LoadSetting(char *str){ return str; }
	virtual void SaveSetting(FILE *){}
	void Enter();
	virtual void EnterGame() = 0;
	void Spin();
	void SpinSound();
	virtual void SpinGame() = 0;
	virtual bool IsPaused(){ return false; }
	int GetEffectSpeed();
};

#endif
