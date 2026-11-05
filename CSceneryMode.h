#ifndef CSCENERYMODE_H_INCLUDED
#define CSCENERYMODE_H_INCLUDED

#include "CWindowCtrl.h"
#include "CStaticCtrl.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CEditCtrl.h"
#include "CGameMode.h"

/*
 *	シーンモード
 */
class CSceneryMode: public CGameMode{
protected:
	static int ms_PhotoMode;	//	撮影モード
	static bool ms_NeedResetViewport;
public:
	static int GetPhotoMode(){ return ms_PhotoMode; }
	CSceneryMode(){}
	virtual ~CSceneryMode(){}
	virtual bool CameraCtrlExp(){ return false; }
	virtual bool CameraCtrlLock(){ return false; }
	char *LoadSetting(char *str){ return LoadScenerySetting(str); }
	virtual char *LoadScenerySetting(char *str){ return str; }
	void SaveSetting(FILE *file){ SaveScenerySetting(file); }
	virtual void SaveScenerySetting(FILE *){}
	CCamera *GetCamera();
	void EnterGame();
	virtual void EnterScenery() = 0;
	void SpinGame();
	void ResetViewport();
	virtual void ApplyCamera();
	virtual void ScanInputScenery() = 0;
	virtual void RenderScenery() = 0;
	bool IsPaused(){ return IsPausedScenery(); }
	virtual bool IsPausedScenery() = 0;
	bool ProcessShortcutKey();
	virtual bool IsArrowMode(){ return false; }
	virtual bool IsWindowDivisible(){ return false; }
};

/*
 *	矢印シーンモード
 */
class CArrowSceneryMode: public CSceneryMode{
protected:
	static CWindowCtrl ms_OptionWindow;	//	汎用設定窓
	static CStaticCtrl ms_SnapLabel;	//	スナップラベル
	static CCheckBox ms_Grid;			//	グリッドチェック
	static CRadioButton ms_Snap[];		//	スナップラジオ
	static CCheckBox ms_Axle[];			//	スナップ適用軸
	static CStaticCtrl ms_BuildLabel;	//	設置方法ラベル
	static CCheckBox ms_Build[];		//	高架チェック
	bool m_ArrowMode;	//	3D モード
	VEC3 m_SnapPos;		//	スナップ適用後座標
public:
	static void InitInterface();
	static int GetAltMask();
	static float GetSnapScale();
	CArrowSceneryMode();
	virtual ~CArrowSceneryMode(){}
	VEC3 *ArrowPos();
	VEC3 GetSnapPos();
	void DrawGrid();
	void EnterScenery();
	char *LoadScenerySetting(char *str){ return LoadArrowScenerySetting(str); }
	virtual char *LoadArrowScenerySetting(char *str){ return str; }
	void SaveScenerySetting(FILE *file){ SaveArrowScenerySetting(file); }
	virtual void SaveArrowScenerySetting(FILE *){}
	virtual void EnterArrowScenery() = 0;
	virtual void ModalFuncArrowScenery(){}
	void ScanInputScenery();
	virtual void ScanInputArrowScenery() = 0;
	void RenderScenery();
	virtual void RenderArrowScenery(){}
	bool IsPausedScenery(){ return IsPausedArrowScenery(); }
	virtual bool IsPausedArrowScenery(){ return false; }
	virtual bool IsArrowMode(){ return m_ArrowMode; }
};

/*
 *	カーソルシーンモード
 */
class CCursorSceneryMode: public CSceneryMode{
protected:
public:
	CCursorSceneryMode(){}
	virtual ~CCursorSceneryMode(){}
	virtual bool CameraCtrlExp(){ return false; }
	virtual bool CameraCtrlLock(){ return false; }
	char *LoadScenerySetting(char *str){ return LoadCursorScenerySetting(str); }
	virtual char *LoadCursorScenerySetting(char *str){ return str; }
	void SaveScenerySetting(FILE *file){ SaveCursorScenerySetting(file); }
	virtual void SaveCursorScenerySetting(FILE *){}
	void EnterScenery();
	virtual void EnterCursorScenery() = 0;
	virtual void ModalFuncCursorScenery(){}
	virtual void ApplyCamera(){ CSceneryMode::ApplyCamera(); }
	void ScanInputScenery();
	virtual void ScanInputCursorScenery() = 0;
	void RenderScenery();
	virtual void RenderCursorScenery(){}
	virtual void RenderCursorSceneryFull(){}
	bool IsPausedScenery(){ return IsPausedCursorScenery(); }
	virtual bool IsPausedCursorScenery(){ return false; }
};

#endif
