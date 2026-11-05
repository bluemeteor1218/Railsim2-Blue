#ifndef CRAILWAYMODE_H_INCLUDED
#define CRAILWAYMODE_H_INCLUDED

#include "CWindowCtrl.h"
#include "CPushButton.h"
#include "CCheckBox.h"
#include "CStaticCtrl.h"
#include "CEditCtrl.h"
#include "CCamera.h"

class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;
class CRailBuilder;

/*
 *	線路関係モード基本クラス
 */
class CRailwayMode{
protected:
	static int ms_CurrentType;				//	モードタイプ
	static int ms_WindowPos[][2];			//	ウィンドウ座標
	static int ms_TrackNum;					//	線数
	static float ms_TrackInterval;			//	線間隔
	static CWindowCtrl ms_RailWindow;		//	線路設定窓
	static CPushButton ms_PluginSetButton;	//	プラグインセットボタン
	static CStaticCtrl ms_TypeLabel;		//	種類ラベル
	static CCheckBox ms_Type[];				//	種類チェック
	static CCheckBox ms_EnableCant;			//	カントチェック
	static CCheckBox ms_LiftRailSurface;	//	高度補正チェック
	static CStaticCtrl ms_MultiTrackLabel;	//	複線ラベル
	static CCheckBox ms_MultiTrackCheck;	//	複線チェック
	static CStaticCtrl ms_TrackNumLabel;	//	軌道数ラベル
	static CEditCtrl ms_TrackNumEdit;		//	軌道数
	static CStaticCtrl ms_TrackIntLabel;	//	軌道間隔ラベル
	static CEditCtrl ms_TrackIntEdit;		//	軌道間隔
	static CCamera ms_RailwayModeCamera;	//	カメラ
public:
	static void InitRailwayInterface();
	static void GetPlugin(CRailPlugin **, CTiePlugin **,
		CGirderPlugin **, CPierPlugin **, CLinePlugin **, CPolePlugin **);
	static void SetRailwayOption(
		bool, bool, bool, bool, bool, bool, bool, bool, bool, int, float);
	static bool IsMultiTrack(){ return !!ms_MultiTrackCheck.GetCheck(); }
	static bool IsCantEnabled(){ return !!ms_EnableCant.GetCheck(); }
	static bool IsLiftRail(){ return !!ms_LiftRailSurface.GetCheck(); }
	static int GetTrackNum(){
		int ret; sscanf(ms_TrackNumEdit.GetText(), "%d", &ret);
		ValueArea(&ret, 1, 100); return ret;
	}
	static float GetTrackInterval(){
		float ret; sscanf(ms_TrackIntEdit.GetText(), "%f", &ret);
		ValueArea(&ret, 0.01f, 100.0f); return Round(ret*100.0f)*0.01f;
	}
	static char *LoadRailwaySetting(char *);
	static void SaveRailwaySetting(FILE *);
	static void EnterRailway(int);
	static void ScanInputRailway();
	static void ReformEditValue();
};

#endif
