#ifndef CRAILPLUGIN_H_INCLUDED
#define CRAILPLUGIN_H_INCLUDED

#include "CProfilePlugin.h"

/*
 *	レールプラグイン
 */
class CRailPlugin: public CProfilePlugin{
	friend class CRailBuilder;
	friend class CLineBuildCurve;
	friend class CPierPlugin;
private:
	float m_Gauge;				//	ゲージ
	float m_Height;				//	高さ
	float m_SurfaceAlt;			//	レール表面高度
	float m_CantRatio;			//	カント係数
	float m_MaxCant;			//	カント最大値 [deg]
	float m_JointInterval;		//	継ぎ目間隔
	bool m_FlattenCant;			//	カント無効化
	CWaveArray *m_WheelSound;	//	車輪音
public:
	static void RenderPreview();
	CRailPlugin(char *);
	~CRailPlugin();
	char *DirName(){ return "Rail"; }
	char *TextName2(){ return "Rail2.txt"; }
	bool Load();
	void SetPreview();
	float CantFunc(float);
	void PlayWheelSound(float, float, VEC3 &);
	void BeforeDump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &);
	void AfterDump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &);
	void CalcPierPos(VEC3 *, VEC3 *, VEC3 *, VEC3 *);
	CPLUGIN_CASTFUNC(CRailPlugin);
};

/*
 *	レールプラグインリスト
 */
class CRailPluginList: public CProfilePluginList{
private:
public:
	char *DirName(){ return "Rail"; }
	char *TextName2(){ return "Rail2.txt"; }
	char *Default(){ return "Default_JR_Narrow"; }
	CPlugin *NewEntry(char *id){ return new CRailPlugin(id); }
	CPLUGINLIST_CASTFUNC(CRailPlugin);
};

//	外部グローバル
extern CRailPlugin *g_Rail;
extern CRailPluginList *g_RailPluginList;

#endif
