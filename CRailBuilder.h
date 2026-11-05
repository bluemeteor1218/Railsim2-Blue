#ifndef CRAILBUILDER_H_INCLUDED
#define CRAILBUILDER_H_INCLUDED

#include "CRailWay.h"

class CRailCurve;
class CRailConnectorLink;
class CRailPlugin;
class CTiePlugin;
class CGirderPlugin;

/*
 *	レール設置子
 */
class CRailBuilder{
private:
	static int ms_CurrentTrack;		//	現在のトラック
	static int ms_TrackNum;			//	線数
	static float ms_TrackInterval;	//	線間隔
	static float ms_TrackShift;		//	中心からのシフト量
	static bool ms_LiftRailSurface;	//	レール持ち上げ
	static VEC3 ms_BeginPosSum;		//	開始位置総和
	static VEC3 ms_BeginDirSum;		//	開始方向総和
	static VEC3 ms_EndPosSum;		//	終了位置総和
	static VEC3 ms_EndDirSum;		//	終了方向総和
	bool m_HitFlag;					//	判定フラグ
	VEC3 m_Pos;						//	座標
	VEC3 m_HitPos;					//	判定座標
	VEC3 m_HitNorm;					//	判定法線
	vector<CRailLinkTemp> m_Link;	//	接続情報
	CRailBuilder *m_Prev;			//	前
	CRailBuilder *m_Next;			//	次
public:
	static void ResetDirSum();
	static void SetTrack(int, int, float, bool);
	static int GetCurrentTrack(){ return ms_CurrentTrack; }
	CRailBuilder(VEC3, CRailBuilder *);
	~CRailBuilder();
	CRailBuilder *Pop();
	CRailBuilder *GetPrev(){ return m_Prev; }
	VEC3 GetPos(){ return m_Pos; }
	VEC3 SetPos(VEC3, int);
	bool CheckLink(){
		return ms_CurrentTrack<ms_TrackNum
			&& m_Link.size()>ms_CurrentTrack && m_Link[ms_CurrentTrack].m_Link;
	}
	bool IsLinkEmpty(){
		return !m_Link.size() || m_Link.size()==1 && !m_Link.begin()->m_Link;
	}
	bool IsLinkFilled(){
		return m_Link.size()==ms_TrackNum && m_Link.rbegin()->m_Link;
	}
	CRailLinkTemp &GetLink(){ return m_Link[ms_CurrentTrack]; }
	bool SetLink(CRailLinkTemp &);
	void PushLink(){ if(m_Link.size()<ms_TrackNum) m_Link.push_back(CRailLinkTemp()); }
	bool IsLast();
	void CalcRight(VEC3 *, VEC3 *, VEC3 *, VEC3 *);
	VEC3 CalcSplitPos();
	VEC3 CalcTrackPos(VEC3 *);
	bool Curve(CRailCurve *, CRailPlugin *, CTiePlugin *, CGirderPlugin *);
	void Render(CLineDumpL *, CRailPlugin *, CTiePlugin *, CGirderPlugin *, bool);
	void BuildRail(CRailConnectorLink &, CRailConnectorLink &,
		CRailPlugin *, CTiePlugin *, CGirderPlugin *);
};

#endif
