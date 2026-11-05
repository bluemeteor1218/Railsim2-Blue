#ifndef CSTATION_H_INCLUDED
#define CSTATION_H_INCLUDED

#include "CStruct.h"
#include "CDiaInst.h"

class CStationPlugin;
class CStation;
class CRailWay;

/*
 *	プラットフォームインスタンス
 */
class CPlatformInst{
private:
	float m_Length;					//	長さ
	bool m_Stoppable;				//	停車可能 /*CP932対応*/
	bool m_OpenDoor[2];				//	ドア開
	CStation *m_Station;			//	駅
	list<CRailWay *> m_RailList;	//	レールリスト
public:
	CPlatformInst(CStation *);
	CPlatformInst(CStation *, bool, bool *);
	void AddRailWay(CRailWay *way){ m_RailList.push_back(way); }
	CRailWay *GetRailWayFront(){ return m_RailList.size() ? m_RailList.front() : NULL; }
	CRailWay *GetRailWayBack(){ return m_RailList.size() ? m_RailList.back() : NULL; }
	void DeleteRailWay(CRailWay *);
	void DeletePlatform();
	void SetPlatformParent(CDetectInfo *);
	float GetLength();
	int GetPlatformState();
	bool IsStoppable(){ return m_Stoppable; }
	bool GetOpenDoor(int s){ return m_OpenDoor[s]; }
	CStation *GetStation(){ return m_Station; }
	void Simulate();
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	反復子
typedef list<CPlatformInst>::iterator IPlatformInst;

/*
 *	施設インスタンス
 */
class CStation: public CStruct{
protected:
	static CStation **ms_Root;					//	接続ルート
	CDiaInst m_DiaInst;							//	ダイヤ設定
	list<CPlatformInst> m_PlatformList;			//	プラットフォームリスト
	vector<CPlatformInst *> m_PlatformArray;	//	プラットフォームリスト
	CStationPlugin *m_StationPlugin;			//	施設プラグイン
	CStation *m_Next;							//	次
public:
	static void SetRoot(CStation **r){ ms_Root = r; }
	CStation();
	CStation(CStationPlugin *);
	CStation(CStationPlugin *, VEC3, VEC3, VEC3);
	~CStation();
	CPlatformInst *PushPlatformInst(CPlatformInst &);
	void MakePlatformArray();
	int GetPlatformCount(){ return m_PlatformArray.size(); }
	CPlatformInst *GetPlatformInst(int i){ return m_PlatformArray[i]; }
	bool IsSelectVisible();
	void Remove();
	void Delete();
	void DeleteGroup(CTrainGroup *g){ m_DiaInst.DeleteGroup(g); }
	virtual CModelInst *Control();
	virtual void SimulateStruct();
	virtual void SetSwitchStruct();
	bool IsStoppable();
	CDiaInst *GetDiaInst(){ return &m_DiaInst; }
	CMODELINST_CASTFUNC(CStation);
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	外部グローバル
extern CPlatformInst *g_PlatformInst;

#endif
