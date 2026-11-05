#ifndef CPOLEPLUGIN_H_INCLUDED
#define CPOLEPLUGIN_H_INCLUDED

#include "CPlugin.h"

/*
 *	架線柱プラグイン
 */
class CPolePlugin: public CPlugin{
	friend class CPole;
private:
	static CObject ms_PreviewObject;	//	プレビュー用オブジェクト
	int m_TrackNum;			//	軌道数
	float m_TrackInterval;	//	軌道間隔
	float m_ModelScale;		//	モデルスケール
	string m_ModelFileName;	//	モデルファイル名
	CMesh *m_Mesh;			//	メッシュ
public:
	CPolePlugin(char *id): CPlugin(id){}
	~CPolePlugin(){}
	char *DirName(){ return "Pole"; }
	char *TextName2(){ return "Pole2.txt"; }
	bool IsMultiTrack(){ return m_TrackNum>1; }
	int ConfirmMultiTrack(int tn, float ti){
		return tn!=m_TrackNum ? 1 : (tn>1 && ti!=m_TrackInterval ? 2 : 0);
	}
	int GetTrackNum(){ return m_TrackNum; }
	float GetTrackInterval(){ return m_TrackInterval; }
	bool Load();
	void SetPreview();
	void Preview(VEC3);
	CPLUGIN_CASTFUNC(CPolePlugin);
};

/*
 *	架線柱プラグインリスト
 */
class CPolePluginList: public CPluginList{
private:
public:
	char *DirName(){ return "Pole"; }
	char *TextName2(){ return "Pole2.txt"; }
	char *Default(){ return "Default_JRS_Single"; }
	CPlugin *NewEntry(char *id){ return new CPolePlugin(id); }
	CPLUGINLIST_CASTFUNC(CPolePlugin);
};

//	外部グローバル
extern CPolePlugin *g_Pole;
extern CPolePluginList *g_PolePluginList;

#endif
