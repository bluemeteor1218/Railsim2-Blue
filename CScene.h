#ifndef CSCENE_H_INCLUDED
#define CSCENE_H_INCLUDED

#include "CCamera.h"
#include "CStruct.h"

class CListElement;
class CRailConnector;
class CRailWay;
class CPier;
class CLine;
class CPole;
class CPoleLink;
class CStation;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;
class CSurfacePlugin;
class CEnvPlugin;

/*
 *	シーン
 */
class CScene: public CStruct{
	friend class CSceneEditMode;
private:
	int m_Serial;						//	シリアルナンバー
	string m_Name;						//	シーン名
	VEC3 m_ArrowPos;					//	編集座標
	CListElement *m_ListElement;		//	リスト要素
	CCamera m_Camera;					//	カメラ
	CRailConnector *m_RailConnector;	//	コネクタ
	CRailWay *m_RailWay;				//	レール
	CPier *m_Pier;						//	橋脚
	CLine *m_Line;						//	架線
	CPole *m_Pole;						//	架線柱
	CStation *m_Station;				//	駅舎
	CStruct *m_Struct;					//	施設
	CSurfacePlugin *m_SurfacePlugin;	//	地形プラグイン
	CEnvPlugin *m_EnvPlugin;			//	環境プラグイン
	bool m_IsDumpReady;					//	ダンプ完了
	CScene *m_Next;						//	次
public:
	CScene();
	CScene(CSurfacePlugin *, CEnvPlugin *, char *);
	~CScene();
	char *GetName(){ return (char *)m_Name.c_str(); }
	char *GetNumberedName();
	void SetName(char *name){ m_Name = name; }
	VEC3 *GetArrowPos(){ return &m_ArrowPos; }
	int GetSerial(){ return m_Serial; }
	void SetSerial(int s){ m_Serial = s; }
	void SetListElement(CListElement *le){ m_ListElement = le; }
	CListElement *GetListElement(){ return m_ListElement; }
	CCamera *GetCamera(){ return &m_Camera; }
	CSurfacePlugin *GetSurface(){ return m_SurfacePlugin; }
	CEnvPlugin *GetEnv(){ return m_EnvPlugin; }
	void SetEnv(CEnvPlugin *epi){ m_EnvPlugin = epi; }
	void SetSeason();
	void Enter(bool);
	void SetGlobalAxis();
	void ResetRailWayRoot();
	void ResetRailConnectorRoot();
	void Delete();
	void DeleteGroup(CTrainGroup *);
	void DeletePierLink(CPier *);
	void DeletePoleLink(CPole *);
	bool DeleteRailWay();
	bool SetRailBlock(char *);
	bool SetSpeedLimit(int);
	bool DeleteRailConnector();
	bool DeletePier(CPier *);
	bool DeleteLine(CLine *);
	bool DeletePole(CPole *);
	bool DeleteStation(CStation *);
	bool DeleteStruct(CStruct *);
	void ScanInputRailWay(int, VEC3, VEC3, bool);
	void ScanInputRailConnector(int, VEC3, VEC3);
	void ScanInputPier(int, VEC3, VEC3);
	void ScanInputLine(int, VEC3, VEC3);
	void ScanInputPole(int, VEC3, VEC3);
	void ScanInputStation(int, VEC3, VEC3, bool init = false);
	void ScanInputStruct(int, VEC3, VEC3, bool init = false);
	CLine *FindLine(CPoleLink *, CPoleLink *);
	void BuildLine(CPierPlugin *, CLinePlugin *, CPolePlugin *);
	bool PickScene(VEC3, VEC3, VEC3 *, VEC3 *tri = NULL, int inv = 1);
	bool ClipAlt(VEC3 *, VEC3 *, VEC3 *, int);
	void Dump();
	void SetDumpReady(bool r){ m_IsDumpReady = r; }
	void RenderScene();
	void RenderAfter();
	void SimulateScene();
	CScene *GetScene(){ return this; }
	void RestoreAddress();
	char *Read(char *, CScene ***);
	void Save(FILE *);
	CMODELINST_CASTFUNC(CScene);
};

//	外部グローバル
extern CScene *g_Scene;
extern map<void *, void *> g_AddressMap;

//	関数宣言
void *ReplaceAdr(void *);

/*
 *	カレントシーンの地形を取得
 */
inline CSurfacePlugin *Surface(){ return g_Scene->GetSurface(); }

#endif
