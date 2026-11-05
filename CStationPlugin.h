#ifndef CSTATIONPLUGIN_H_INCLUDED
#define CSTATIONPLUGIN_H_INCLUDED

#include "CStructPlugin.h"

class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;
class CStationPlugin;
class CRailBuilder;
class CPlatformInst;
class CStation;

/*
 *	プラットフォーム
 */
class CPlatform{
	friend class CStation;
private:
	static CRailPlugin *ms_PlatformRail;		//	カレントレールプラグイン
	static CTiePlugin *ms_PlatformTie;			//	カレント枕木プラグイン
	static CGirderPlugin *ms_PlatformGirder;	//	カレント橋桁プラグイン
	static CPierPlugin *ms_PlatformPier;		//	カレント橋脚プラグイン
	static CLinePlugin *ms_PlatformLine;		//	カレント架線プラグイン
	static CPolePlugin *ms_PlatformPole;		//	カレント架線柱プラグイン
	int m_TrackNum;					//	軌道数
	float m_TrackInterval;			//	軌道間隔
	bool m_Stoppable;				//	停車可能 /*CP932対応*/
	bool m_OpenDoor[2];				//	ドア開 (left, right)
	bool m_RailPluginValid;			//	レールプラグイン有効フラグ
	bool m_TiePluginValid;			//	枕木プラグイン有効フラグ
	bool m_GirderPluginValid;		//	橋桁プラグイン有効フラグ
	bool m_PierPluginValid;			//	橋脚プラグイン有効フラグ
	bool m_LinePluginValid;			//	架線プラグイン有効フラグ
	bool m_PolePluginValid;			//	架線柱プラグイン有効フラグ
	CRailPlugin *m_RailPlugin;		//	レールプラグイン
	CTiePlugin *m_TiePlugin;		//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;	//	橋桁プラグイン
	CPierPlugin *m_PierPlugin;		//	橋脚プラグイン
	CLinePlugin *m_LinePlugin;		//	架線プラグイン
	CPolePlugin *m_PolePlugin;		//	架線柱プラグイン
	bool m_LiftRailSurface;			//	レール表面持ち上げ
	bool m_EnableCant;				//	カント有効
	char *m_ParentObjectScriptPos;	//	親オブジェクトパース箇所]
	string m_ParentObjectName;		//	親オブジェクト名
	CNamedObject *m_ParentObject;	//	親オブジェクト
	list<VEC3> m_CoordList;			//	制御点リスト
public:
	static void ResetPlatformPlugin();
	CPlatform();
	char *Read(char *);
	void PushCoord(VEC3 v){ m_CoordList.push_back(v); }
	void SetPlatformParent(CStationPlugin *);
	int GetCoordNum(){ return m_CoordList.size(); }
	void SetPlatformPlugin();
	CRailBuilder *SetBuilder(MTX4 *);
	void Preview(MTX4 *, CLineDumpL *);
	void Build(MTX4 *, CStation *);
};

//	反復子
typedef list<CPlatform>::iterator IPlatform;

/*
 *	駅舎プラグイン
 */
class CStationPlugin: public CStructPlugin{
	friend class CStation;
protected:
	list<CPlatform> m_Platform;	//	プラットフォーム
public:
	static void RenderPreview(VEC3, VEC3, VEC3);
	CStationPlugin(char *id): CStructPlugin(id){}
	~CStationPlugin();
	char *DirName(){ return "Station"; }
	char *TextName(){ return "Station.txt"; }
	char *TextName2(){ return "Station2.txt"; }
	char *LoadStructBefore(char *);
	char *LoadStructAfter(char *);
	bool LoadOldForm();
	void SetPreview();
	void PreviewStruct();
	void BuildPlatform(CStation *);
	CPLUGIN_CASTFUNC(CStationPlugin);
};

/*
 *	施設プラグインリスト
 */
class CStationPluginList: public CStructPluginList{
private:
public:
	char *DirName(){ return "Station"; }
	char *TextName(){ return "Station.txt"; }
	char *TextName2(){ return "Station2.txt"; }
	char *Default(){ return "MM02"; }
	CPlugin *NewEntry(char *id){ return new CStationPlugin(id); }
	CPLUGINLIST_CASTFUNC(CStationPlugin);
};

//	外部グローバル
extern CStationPlugin *g_Station;
extern CStationPluginList *g_StationPluginList;

#endif
