#ifndef CTIEPLUGIN_H_INCLUDED
#define CTIEPLUGIN_H_INCLUDED

#include "CProfilePlugin.h"

/*
 *	枕木プラグイン
 */
class CTiePlugin: public CProfilePlugin{
	friend class CRailBuilder;
	friend class CLineBuildCurve;
	friend class CPierPlugin;
private:
	float m_Height;		//	高さ
	bool m_FlattenCant;	//	カント無効化
public:
	CTiePlugin(char *id): CProfilePlugin(id){}
	char *DirName(){ return "Tie"; }
	char *TextName2(){ return "Tie2.txt"; }
	bool Load();
	void SetPreview();
	void AfterDump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &);
	void CalcPierPos(VEC3 *, VEC3 *, VEC3 *, VEC3 *);
	CPLUGIN_CASTFUNC(CTiePlugin);
};

/*
 *	枕木プラグインリスト
 */
class CTiePluginList: public CProfilePluginList{
private:
public:
	char *DirName(){ return "Tie"; }
	char *TextName2(){ return "Tie2.txt"; }
	char *Default(){ return "Default_JRN_BallastPC"; }
	CPlugin *NewEntry(char *id){ return new CTiePlugin(id); }
	CPLUGINLIST_CASTFUNC(CTiePlugin);
};

//	外部グローバル
extern CTiePlugin *g_Tie;
extern CTiePluginList *g_TiePluginList;

#endif
