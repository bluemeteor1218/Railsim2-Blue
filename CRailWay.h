#ifndef CRAILWAY_H_INCLUDED
#define CRAILWAY_H_INCLUDED

#include "CLine.h"
#include "CTrainSetBuffer.h"
#include "CRailLink.h"

class CRailTraceCurve;
class CRailLinkTemp;
class CPier;
class CPlatformInst;
class CScene;
class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;
class CPierPlugin;
class CLinePlugin;
class CPolePlugin;
class CDetectInfo;
class CRailWayParentLink;

/*
 *	橋脚位置データ
 */
class CPierPos{
public:
	float m_Pos;	//	位置
	CPier *m_Link;	//	架線柱
public:
	CPierPos(){ m_Pos = 0.0f; m_Link = NULL; }
	CPierPos(float p, CPier *l){ m_Pos = p; m_Link = l; }
	bool operator<(const CPierPos &rhs){ return m_Pos<rhs.m_Pos; }
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	反復子
typedef list<CPierPos>::iterator IPierPos;

/*
 *	架線柱位置データ
 */
class CPolePos{
public:
	float m_Pos;	//	位置
	int m_Track;	//	軌道番号
	bool m_Multi;	//	複線用リンク
	CPole *m_Link;	//	架線柱
public:
	CPolePos(){ m_Pos = 0.0f; m_Link = NULL; }
	CPolePos(float p, CPole *l, int t, bool m){
		m_Pos = p; m_Link = l; m_Track = t; m_Multi = m;
	}
	bool operator<(const CPolePos &rhs){ return m_Pos<rhs.m_Pos; }
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	反復子
typedef list<CPolePos>::iterator IPolePos;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	レールインスタンス
 */
class CRailWay{
	friend class CRailWayLink;
	friend class CGroupEndLocator;
	friend class CRailBuildMode;
private:
	static CRailWay **ms_Root;				//	接続ルート
	static float ms_MinDist;				//	最小検出距離
	static CRailWayLink ms_Detect;			//	検出情報
	void *m_OldAdr;							//	旧アドレス
	int m_Selected;							//	選択フラグ
	vector<float> m_RailMapV;				//	レールマッピング V 座標
	vector<float> m_TieMapV;				//	枕木マッピング V 座標
	vector<float> m_GirderMapV;				//	橋桁マッピング V 座標
	vector<float> m_LineMapV;				//	架線マッピング V 座標
	float m_PierPos;						//	橋脚設置位置
	float m_PolePos;						//	架線柱位置
	float m_SegLen;							//	セグメント長
	bool m_BuildLine;						//	架線設置フラグ
	bool m_WarpDummy;						//	ワープ用ダミーレール
	bool m_MultiTrackDummy;					//	複線用ダミーレール
	int m_DummyTrackNum;					//	ダミー用軌道数
	float m_DummyTrackInterval;				//	ダミー用軌道間隔	
	std::string m_RailBlock;				//	閉塞区間
	int m_SpeedLimit;						//	制限速度
	CRailConnectorLink m_Link[2];			//	接続コネクタ
	list<CRailSplitter> m_SplitList;		//	分割点リスト
	CRailWayParentLink *m_Parent;			//	親オブジェクト
	CPlatformInst *m_Platform;				//	プラットフォーム
	CScene *m_Scene;						//	シーン
	CRailPlugin *m_RailPlugin;				//	レールプラグイン
	CTiePlugin *m_TiePlugin;				//	枕木プラグイン
	CGirderPlugin *m_GirderPlugin;			//	橋桁プラグイン
	CPierPlugin *m_PierPlugin;				//	橋脚プラグイン
	CLinePlugin *m_LinePlugin;				//	架線プラグイン
	CPolePlugin *m_PolePlugin;				//	架線柱プラグイン
	list<CPierPos> m_PierList;				//	橋脚リスト
	list<CPolePos> m_PoleList;				//	架線柱リスト
	list<CGroupEndLocator *> m_GroupEnd;	//	編成終端データ
	CRailWay *m_Next;						//	リスト次
public:
	static void SetRoot(CRailWay **r){ ms_Root = r; }
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static float GetMinDist(){ return ms_MinDist; }
	static CRailWayLink GetDetect(){ return ms_Detect; }
	CRailWay();
	CRailWay(CRailConnectorLink &, CRailConnectorLink &,
		CRailPlugin *, CTiePlugin *, CGirderPlugin *,
		CPierPlugin *ipi = NULL, CLinePlugin *lpi = NULL, CPolePlugin *ppi = NULL,
		list<CRailSplitter> *splist = NULL);
	~CRailWay();
	CRailWay *Next(){ return m_Next; }
	CRailWay **NextAdr(){ return &m_Next; }
	void *OldAdr(){ return m_OldAdr; }
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int s){ m_Selected |= s; }
	CRailWayLink CreateLink(int s){ return CRailWayLink(s, this); }
	CScene *GetScene(){ return m_Scene; }
	CRailPlugin *GetRailPlugin(){ return m_RailPlugin; }
	CTiePlugin *GetTiePlugin(){ return m_TiePlugin; }
	CGirderPlugin *GetGirderPlugin(){ return m_GirderPlugin; }
	CRailWayParentLink *GetParent(){ return m_Parent; }
	void SetParent(CDetectInfo *);
	CPlatformInst *GetPlatform(){ return m_Platform; }
	void SetPlatform(CPlatformInst *pl){ m_Platform = pl; }
	void SetWarpDummy();
	bool IsMultiTrackDummy(){ return m_MultiTrackDummy; }
	bool IsLinkable(int, VEC3 &);
	float GetSegLen();
	void Stabilize();
	void AddSplitter(const CRailSplitter &spl){ m_SplitList.push_back(spl); }
	CRailConnectorLink SplitLink(CRailLinkTemp *, CRailWay **, IRailSplitter);
	void SplitSelect();
	CRailWay *Delete();
	void ConnectRailWay(int, CRailWay *, int, CScene *);
	void BranchRailWay(int, CRailWay *, int, CScene *);
	void DisconnectRailWay(int, CScene *);
	bool SetRailBlock(char *rb);
	bool IsRailBlock(){ return !m_RailBlock.empty(); }
	std::string& GetRailBlock(){ return m_RailBlock; }
	bool CheckRailBlock(CTrainGroup *);
	bool SetSpeedLimit(int);
	bool IsSpeedLimit(){ return m_SpeedLimit>=0; }
	int GetSpeedLimit(){ return m_SpeedLimit; }
	void SetMapTemp();
	void CopyMapTemp(int);
	void CopyMapTempMulti(int, float *, float *, int *, float *, float *, int *);
	CPoleLink GetPoleLink(int);
	void AddPolePos(float p){ m_PolePos += p; }
	void AddPole(float, CPole *, int, bool);
	void InsertPole(float, CPole *, int, bool);
	void DeletePole(CPole *);
	void AddPierPos(float p){ m_PierPos += p; }
	void AddPier(float, CPier *);
	void InsertPier(float, CPier *);
	void DeletePier(CPier *);
	void BuildLine(CPierPlugin *, CLinePlugin *, CPolePlugin *);
	void ShowSegment();
	void ScanInput(int, VEC3 &, VEC3 &);
	void ScanInputWarp(int, VEC3 &, VEC3 &);
	void TraceRail(int, CRailTraceCurve *);
	void TraceRailSplit(int, CRailTraceCurve *);
	VEC3 GetFirstDir(int);
	void RemoveGroup();
	void CheckWarpEndScene(CScene *);
	bool IsInsideGroup(int, float);
	bool SetTrain(int, float, CGroupEndLocator *, bool,
		ITrainSetBuffer *, ITrainSetBuffer *, CTrainGroup *, bool, bool);
	int MarchTrain(float *, CGroupEndLocator *, CTrainGroup *, CTrainGroup *);
	bool CheckPlatformExtend(int, CPlatformInst *);
	int GetPlatformState();
	void UpdateSplitList();
	void Dump();
	void Render();
	void RenderWarp();
	void RestoreAddress();
	char *Read(char *);
	char *ReadWarp(char *);
	void Save(FILE *);
	void SaveWarp(FILE *);
};

//	反復子
typedef list<CRailWay *>::iterator IPRailWay;
typedef list<list<CRailWay *> >::iterator ILPRailWay;
typedef list<list<list<CRailWay *> > >::iterator ILLPRailWay;

/*
 *	接続先カント量を求める
 */
inline float CRailWayLink::GetCant(){
	return m_Side ? -m_Link->m_Link[m_Side].m_Cant : m_Link->m_Link[m_Side].m_Cant;
}

/*
 *	接続先座標を求める
 */
inline VEC3 CRailWayLink::GetPos(){
	return m_Link->m_Link[m_Side].GetPos();
}

/*
 *	接続先反対座標を求める
 */
inline VEC3 CRailWayLink::GetOppositePos(){
	return m_Link->m_Link[!m_Side].GetPos();
}

/*
 *	最初の分割点までの dir ベクトルを求める
 */
inline VEC3 CRailWayLink::GetFirstDir(){
	return m_Link->GetFirstDir(m_Side);
}

/*
 *	レール接続一時データ
 */
class CRailLinkTemp{
public:
	int m_Side;					//	サイド
	float m_SumLen;				//	積算距離
	IRailSplitter m_SpliceItr;	//	スプリット位置
	VEC3 m_Pos;					//	座標
	VEC3 m_Right;				//	right
	VEC3 m_Up;					//	up
	VEC3 m_Dir;					//	dir
	CRailWay *m_Link;			//	レール
public:
	CRailLinkTemp(){ m_Link = NULL; }
	CRailLinkTemp(int, float, VEC3 &, VEC3 &, VEC3 &, VEC3 &, CRailWay *, IRailSplitter);
	CRailConnectorLink SplitLink(CRailWay **wadr){
		return m_Link->SplitLink(this, wadr, m_SpliceItr);
	}
	void CopyMapTemp(){ m_Link->CopyMapTemp(m_Side); }
};

//	外部グローバル
extern list<list<list<CRailWay *> > > g_MultiTrackRailList;
extern ILLPRailWay g_MultiTrackSegment;
extern ILPRailWay g_SingleTrackSegment;
extern bool g_MultiTrackDummy;
extern int g_DummyTrackNum;
extern float g_DummyTrackInterval;
extern map<std::string, CTrainGroup *> g_RailBlockMap;

//	関数宣言
CTrainGroup* GetRailBlockUser(std::string&);
void SetRailBlockUser(std::string&, CTrainGroup *);
void ClearRailBlockUser(CTrainGroup *);

#endif
