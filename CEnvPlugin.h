#ifndef CENVPLUGIN_H_INCLUDED
#define CENVPLUGIN_H_INCLUDED

#include "CPlugin.h"
#include "CLensFlare.h"

/*
 *	ライトセッティング
 */
class CLightSetting{
	friend class CEnvPlugin;
private:
	float m_SunAlt;			//	太陽高度
	D3DCOLOR m_Directional;	//	平行光源色
	D3DCOLOR m_Ambient;		//	環境光源色
	D3DCOLOR m_SkyColor;	//	空の色
public:
	char *Read(char *);
	bool operator<(const CLightSetting &rhs){
		return m_SunAlt<rhs.m_SunAlt;
	}
};

//	反復子
typedef list<CLightSetting>::iterator ILightSetting;

/*
 *	衛星データ
 */
class CMoon{
	friend class CEnvPlugin;
private:
	string m_MoonFile;			//	ファイル名
	CMesh *m_MoonMesh;			//	メッシュ
	CObject m_MoonObject;		//	オブジェクト
	float m_MoonScale;			//	スケール
	float m_AxialInclination;	//	公転軸の傾き
	float m_RevolutionPeriod;	//	公転周期
	float m_InitialPhase;		//	初期位相
	float m_RevolutionPerDay;	//	1 日当たり公転角度
public:
	char *Read(char *);
	void LoadData();
	void Render(double, float, VEC3, float);
};

//	反復子
typedef list<CMoon>::iterator IMoon;

/*
 *	環境プラグイン
 */
class CEnvPlugin: public CPlugin{
private:
	float m_Latitude;				//	緯度
	string m_EnvMapTexFile;			//	環境マップファイル名
	LPTEX8 m_EnvMapTexture;			//	環境マッピング
	string m_LandscapeFile;			//	景観ファイル名
	CMesh *m_LandscapeMesh;			//	景観メッシュ
	CObject m_LandscapeObject;		//	景観オブジェクト
	float m_LandscapeScale;			//	景観スケール
	string m_SunFile;				//	景観ファイル名
	CMesh *m_SunMesh;				//	景観メッシュ
	CObject m_SunObject;			//	景観オブジェクト
	float m_SunScale;				//	景観スケール
	float m_SunAxialInclination;	//	地軸の傾き
	CLensFlare m_SunLensFlare;		//	太陽レンズフレア
	CWhiteout m_SunWhiteout;		//	太陽ホワイトアウト
	float m_NightThreshold;			//	昼夜閾値
	D3DCOLOR m_ShadowColor;			//	影の色
	list<CLightSetting> m_Light;	//	光源設定
	list<CMoon> m_Moon;				//	月リスト
public:
	CEnvPlugin(char *id): CPlugin(id){}
	~CEnvPlugin(){}
	char *DirName(){ return "Env"; }
	char *TextName2(){ return "Env2.txt"; }
	bool Load();
	bool GetHemisphere(){ return m_Latitude<0.0f; }
	void SetPreview();
	void SetEnvMapTexture(){ devSetTexture(1, m_EnvMapTexture); }
	void Render(double abstime = -1.0);
	void RenderAfter();
	D3DCOLOR GetShadowColor(){ return m_ShadowColor; }
	CPLUGIN_CASTFUNC(CEnvPlugin);
};

/*
 *	環境プラグインリスト
 */
class CEnvPluginList: public CPluginList{
private:
public:
	char *DirName(){ return "Env"; }
	char *TextName2(){ return "Env2.txt"; }
	char *Default(){ return "Default"; }
	CPlugin *NewEntry(char *id){ return new CEnvPlugin(id); }
	CPLUGINLIST_CASTFUNC(CEnvPlugin);
};

//	外部グローバル
extern float g_DayAlpha;
extern float g_NightAlpha;
extern D3DCOLOR g_NoLightColor;
extern CEnvPlugin *g_Env;
extern CEnvPlugin *g_DefaultEnv;
extern CEnvPluginList *g_EnvPluginList;

#endif
