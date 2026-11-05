#ifndef CRAILWAYPLUGINSET_H_INCLUDED
#define CRAILWAYPLUGINSET_H_INCLUDED

#include "CPlugin.h"

class CPopMenu;
class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;

/*
 *	レール関連プラグインセット
 */
class CRailwayPluginSet: public CPlugin{
private:
	string m_RailID;				//	レール ID
	string m_TieID;					//	枕木 ID
	string m_GirderID;				//	橋桁 ID
	string m_PierID;				//	橋脚 ID
	string m_LineID;				//	架線 ID
	string m_PoleID;				//	架線柱 ID
	bool m_RailFlag;				//	レールフラグ
	bool m_TieFlag;					//	枕木フラグ
	bool m_GirderFlag;				//	橋桁フラグ
	bool m_PierFlag;				//	橋脚フラグ
	bool m_LineFlag;				//	架線フラグ
	bool m_PoleFlag;				//	架線柱フラグ
	CRailPlugin *m_RailPlugin;		//	レールプラグイン
	CTiePlugin *m_TiePlugin;		//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;	//	橋桁プラグイン
	CPierPlugin *m_PierPlugin;		//	橋脚プラグイン
	CLinePlugin *m_LinePlugin;		//	架線プラグイン
	CPolePlugin *m_PolePlugin;		//	架線柱プラグイン
	bool m_EnableCant;				//	カントフラグ
	bool m_LiftRailSurface;			//	持ち上げフラグ
	bool m_MultiTrack;				//	複線フラグ
	int m_TrackNum;					//	軌道数
	float m_TrackInterval;			//	軌道間隔
public:
	CRailwayPluginSet(char *, bool);
	~CRailwayPluginSet();
	char *DirName(){ return "RailwayPluginSet"; }
	char *TextName2(){ return FlashIn("%s.txt", m_Name.c_str()); }
	bool PreLoadRPS(FILE *);
	bool Load();
	void Save(FILE *);
	bool DeleteFromDisk();
	bool Rename(string &);
	bool operator<(const CRailwayPluginSet &rhs){ return m_Name<rhs.m_Name; }
	string &GetNameRef(){ return m_Name; }
	char *GetName(){ return (char *)m_Name.c_str(); }
	void Apply();
};

//	反復子
typedef list<CRailwayPluginSet>::iterator IRailwayPluginSet;

//	関数宣言
void LoadRailwayPluginSetList();
IRailwayPluginSet FindRailwayPluginSet(string);
void AddRailwayPluginSet(string, bool);
void ListRailwayPluginSet();
void ModalFuncRailwayPluginSet();

//	外部グローバル
extern CPopMenu *g_RailwayPluginSetMenu;
extern list<CRailwayPluginSet> g_RailwayPluginSetList;

#endif
