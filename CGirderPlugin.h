#ifndef CGIRDERPLUGIN_H_INCLUDED
#define CGIRDERPLUGIN_H_INCLUDED

#include "CProfilePlugin.h"

/*
 *	橋桁プラグイン
 */
class CGirderPlugin: public CProfilePlugin{
	friend class CRailBuilder;
	friend class CLineBuildCurve;
	friend class CPierPlugin;
private:
	float m_Height;			//	高さ
	int m_TrackNum;			//	軌道数
	float m_TrackInterval;	//	軌道間隔
	bool m_FlattenCant;		//	カント無効化
public:
	CGirderPlugin(char *id): CProfilePlugin(id){}
	char *DirName(){ return "Girder"; }
	char *TextName2(){ return "Girder2.txt"; }
	bool IsMultiTrack(){ return m_TrackNum>1; }
	int ConfirmMultiTrack(int tn, float ti){
		return tn!=m_TrackNum ? 1 : (tn>1 && ti!=m_TrackInterval ? 2 : 0);
	}
	bool Load();
	float GetPreviewFix(){ return 0.5f*m_TrackInterval*(m_TrackNum-1); }
	void SetPreview();
	void CalcPierPos(VEC3 *, VEC3 *, VEC3 *, VEC3 *);
	CPLUGIN_CASTFUNC(CGirderPlugin);
};

/*
 *	橋桁プラグインリスト
 */
class CGirderPluginList: public CProfilePluginList{
private:
public:
	char *DirName(){ return "Girder"; }
	char *TextName2(){ return "Girder2.txt"; }
	char *Default(){ return "Default_JRN_SinglePC"; }
	CPlugin *NewEntry(char *id){ return new CGirderPlugin(id); }
	CPLUGINLIST_CASTFUNC(CGirderPlugin);
};

//	外部グローバル
extern CGirderPlugin *g_Girder;
extern CGirderPluginList *g_GirderPluginList;

#endif
