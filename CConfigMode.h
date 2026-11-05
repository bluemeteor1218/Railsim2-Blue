#ifndef CCONFIGMODE_H_INCLUDED
#define CCONFIGMODE_H_INCLUDED

#include "CStaticCtrl.h"
#include "CEditCtrl.h"
#include "CGroupBox.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CWindowDivInfo.h"
#include "CInterfaceMode.h"

const int RES_MODE_NUM = 4;			//	解像度タイプ数
const int MIPMAP_NUM = 4;			//	ミップマップ区分数
const int PISND_NUM = 4;			//	プラグインサウンド区分数
const int STEREO_METHOD_NUM = 2;	//	ステレオ手法数

/*
 *	環境設定モード
 */
class CConfigMode: public CInterfaceMode{
private:
	CWindowCtrl m_ConfigWindow1;		//	窓 1

	CGroupBox m_InterfaceGroup;			//	インターフェイス
	CCheckBox m_HideTopPanel;			//	上パネルを隠す
	CCheckBox m_HideRightPanel;			//	右パネルを隠す
	CCheckBox m_WindowShadow;			//	ウィンドウの影

	CGroupBox m_DeviceGroup;			//	デバイス
	CStaticCtrl m_NeedRestart;			//	要再起動
	CCheckBox m_FullScreen;				//	フルスクリーン
	CStaticCtrl m_ResLabel;				//	解像度
	CRadioButton m_Resolution[RES_MODE_NUM];	//	解像度チェック
	CStaticCtrl m_MipMapLabel;			//	ミップマップ
	CCheckBox m_MipMap[MIPMAP_NUM];		//	ミップマップチェック

	CGroupBox m_AccessoryGroup;			//	アクセサリ
	CCheckBox m_Compass;				//	コンパス
	CCheckBox m_WindMeter;				//	風力計
	CCheckBox m_ShowMap;				//	地図

	CGroupBox m_EffectGroup;			//	視覚効果
	CCheckBox m_Shadow;					//	影
	CCheckBox m_LinearFilter;			//	テクスチャフィルタ
	CCheckBox m_EnvMap;					//	環境マッピング
	CCheckBox m_SpecularLight;			//	鏡面反射光
	CCheckBox m_SunLensFlare;			//	太陽レンズフレア
	CCheckBox m_SunWhiteout;			//	太陽ホワイトアウト
	CCheckBox m_MiscLensFlare;			//	その他レンズフレア
	CCheckBox m_MiscParticle;			//	パーティクル
	CCheckBox m_Wind;					//	風
	CGroupBox m_SoundGroup;				//	サウンド
	CCheckBox m_InterfaceSound;			//	インターフェイス
	CCheckBox m_PluginSound[PISND_NUM];	//	プラグインサウンド

	CGroupBox m_MiscGroup;				//	その他
	CCheckBox m_UseUndo;				//	アンドゥ使用

	CWindowCtrl m_ConfigWindow2;		//	窓 2
	CGroupBox m_StereoGroup;			//	ステレオ
	CCheckBox m_StereoEnabled;			//	ステレオ有効
	CStaticCtrl m_StereoMethodLabel;	//	ステレオ手法
	CRadioButton m_StereoMethod[STEREO_METHOD_NUM];	//	ステレオ手法
	CStaticCtrl m_StereoIntervalLabel;	//	ステレオ手法
	CEditCtrl m_StereoIntervalEdit;		//	視点間隔

	CWindowInfo m_RootWindow;			//	画面分割情報
	CWindowInfo* m_ActiveWindow;		//	ポイントしている画面

public:
	CConfigMode();
	~CConfigMode();
	void EnterInterface();
	void ScanInputInterface();
	int ScanInputWindowDiv();
	void RenderInterface();
	void RenderWindowDiv();
	bool Load();
	bool Save();
	int GetHideTopPanel(){ return m_HideTopPanel.GetCheck(); }
	int GetHideRightPanel(){ return m_HideRightPanel.GetCheck(); }
	int GetWindowShadow(){ return m_WindowShadow.GetCheck(); }
	int GetCompass(){ return m_Compass.GetCheck(); }
	int GetWindMeter(){ return m_WindMeter.GetCheck(); }
	int GetShowMap(){ return m_ShowMap.GetCheck(); }
	void SetShowMap(int s){ m_ShowMap.SetCheck(s); }
	int GetShadow(){ return m_Shadow.GetCheck(); }
	void SetShadow(int s){ m_Shadow.SetCheck(s); }
	void SetTexFilter();
	int GetEnvMap(){ return m_EnvMap.GetCheck(); }
	void SetSpecularLight(){ devSetSpecular(m_SpecularLight.GetCheck()); }
	int GetSunLensFlare(){ return m_SunLensFlare.GetCheck(); }
	int GetSunWhiteout(){ return m_SunWhiteout.GetCheck(); }
	int GetMiscLensFlare(){ return m_MiscLensFlare.GetCheck(); }
	int GetMiscParticle(){ return m_MiscParticle.GetCheck(); }
	int GetWind(){ return m_Wind.GetCheck(); }
	int GetInterfaceSound(){ return m_InterfaceSound.GetCheck(); }
	int GetRailSound(){ return m_PluginSound[0].GetCheck(); }
	int GetTrainSound(){ return m_PluginSound[1].GetCheck(); }
	int GetStructSound(){ return m_PluginSound[2].GetCheck(); }
	int GetSurfaceSound(){ return m_PluginSound[3].GetCheck(); }
	int GetUseUndo(){ return m_UseUndo.GetCheck(); }
	int GetStereo(){ return m_StereoEnabled.GetCheck(); }
	void SetStereo(int s){ m_StereoEnabled.SetCheck(s); }
	int GetStereoMethod(){ return m_StereoMethod->GetNumber(); }
	float GetStereoInterval();
	void CheckHardware();
	CWindowInfo* GetRootWindow() { return &m_RootWindow; }
	bool IsWindowDiv() { return m_RootWindow.GetDiv()!=NULL; }
	CWindowInfo* GetActiveWindow() { return m_ActiveWindow; }
	void SetActiveWindow(CWindowInfo* wnd) { m_ActiveWindow = wnd; }
	void FreeWindowDiv();
};

//	外部グローバル
extern char *g_ConfigBuffer;
extern char *g_ConfigScript;
extern float g_ConfigVersion;
extern bool g_RailMipMap;
extern bool g_TrainMipMap;
extern bool g_StructMipMap;
extern bool g_SurfaceMipMap;
extern bool g_NamedObjectMipMap;
extern CConfigMode *g_ConfigMode;

#endif
