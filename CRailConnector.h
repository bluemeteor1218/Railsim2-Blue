#ifndef CRAILCONNECTOR_H_INCLUDED
#define CRAILCONNECTOR_H_INCLUDED

#include "CRailLink.h"
#include "CDiaInst.h"

class CTrainGroup;
class CScene;

/*
 *	レールコネクタ
 */ 
class CRailConnector{
	friend class CRailConnectorLink;
	friend class CRailWay;
private:
	static CRailConnector **ms_Root;	//	接続ルート
	static float ms_MinDist;			//	最小検出距離
	static CRailConnector *ms_Detect;	//	検出情報
	void *m_OldAdr;						//	旧アドレス
	int m_Selected;						//	選択フラグ
	int m_Side;							//	進行方向
	int m_TrailPoint[2];				//	トレール方向
	float m_Cant;						//	カント量
	bool m_CantProc;					//	カント処理フラグ
	int m_NetPoint;						//	ネットワーク用ポイント方向
	CPointInst m_PointInst;				//	ポイント切替
	CRailSplitter m_Splitter;			//	分割子
	CTrainGroup *m_User;				//	通過中編成
	CRailWayLink m_Link[2][2];			//	接続レール
	CScene *m_Scene;					//	シーン
	CRailConnector *m_Next;				//	リスト次
public:
	static void SetRoot(CRailConnector **r){ ms_Root = r; }
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static float GetMinDist(){ return ms_MinDist; }
	static CRailConnector *GetDetect(){ return ms_Detect; }
	CRailConnector();
	CRailConnector(VEC3 &, VEC3 &);
	~CRailConnector();
	CRailConnector *Next(){ return m_Next; }
	CRailConnector **NextAdr(){ return &m_Next; }
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int s){ m_Selected |= s; }
	void SetSelectFlag(int s){ m_Selected = s; }
	CPointInst *GetPointInst(){ return &m_PointInst; }
	int GetNetPoint(){ return m_NetPoint; }
	void SetNetPoint(int);
	void SwitchNetPoint();
	CRailConnectorLink CreateLink(int s, int p){ return CRailConnectorLink(s, p, this); }
	CRailSplitter GetSplitter(int rev){ return m_Splitter.Get(rev); }
	VEC3 GetPos(){ return m_Splitter.m_Pos; }
	VEC3 GetUp(){ return m_Splitter.m_Up; }
	CTrainGroup *GetUser(){ return m_User; }
	void SetUser(CTrainGroup *group){ m_User = group; }
	CScene *GetScene(){ return m_Scene; }
	CRailConnector *Delete();
	void DeleteGroup(CTrainGroup *g){ m_PointInst.DeleteGroup(g); }
	void RemoveGroup();
	void Stabilize(VEC3);
	int GetLinkCount();
	bool CheckRailBlock(CTrainGroup *);
	void ScanInput(int, VEC3 &, VEC3 &);
	bool Dump();
	void Render(D3DCOLOR, bool);
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	反復子
typedef set<CRailConnector *>::iterator ISPRailConnector;

/*
 *	レール接続
 */
inline void CRailConnectorLink::Connect(
	CRailWayLink &link	//	リンク
){
	m_Link->m_Link[m_Side][m_Point] = link;
}

/*
 *	レール接続解除
 */
inline void CRailConnectorLink::Disconnect(){
	m_Link->m_Link[m_Side][m_Point].m_Link = NULL;
	m_Link = NULL;
}

/*
 *	接続先座標を求める
 */
inline VEC3 CRailConnectorLink::GetPos(){
	return m_Link->m_Splitter.m_Pos;
}

/*
 *	接続先 right ベクトルを求める
 */
inline VEC3 CRailConnectorLink::GetRight(){
	return m_Side ? -m_Link->m_Splitter.m_Right : m_Link->m_Splitter.m_Right;
}

/*
 *	接続先 up ベクトルを求める
 */
inline VEC3 CRailConnectorLink::GetUp(){
	return m_Link->m_Splitter.m_Up;
}

/*
 *	接続先 dir ベクトルを求める
 */
inline VEC3 CRailConnectorLink::GetDir(){
	return m_Side ? -m_Link->m_Splitter.m_Dir : m_Link->m_Splitter.m_Dir;
}

/*
 *	接続先カント量を求める
 */
inline float CRailConnectorLink::GetCant(){
	return m_Side ? -m_Link->m_Cant : m_Link->m_Cant;
}

/*
 *	接続先分割子を求める
 */
inline CRailSplitter CRailConnectorLink::GetSplitter(
	int rev	//	反転フラグ
){
	return m_Link->GetSplitter(rev^m_Side);
}

#endif
