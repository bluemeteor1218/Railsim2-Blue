#ifndef CRAILLINK_H_INCLUDED
#define CRAILLINK_H_INCLUDED

class CRailWayLink;
class CRailConnector;
class CRailWay;

#include "CVertexDump.h"

/*
 *	レール分割子
 */
class CRailSplitter{
public:
	VEC3 m_Pos;		//	座標
	VEC3 m_Right;	//	up
	VEC3 m_Up;		//	right
	VEC3 m_Dir;		//	dir
	int m_Selected;	//	選択フラグ
public:
	CRailSplitter(){}
	CRailSplitter(const VEC3 &p, const VEC3 &d){
		m_Pos = p; m_Dir = d; m_Selected = 0;
	}
	CRailSplitter(const VEC3 &p, const VEC3 &r, const VEC3 &u, const VEC3 &d){
		m_Pos = p; m_Right = r; m_Up = u; m_Dir = d; m_Selected = 0;
	}
	CRailSplitter Get(int rev){
		return rev ? CRailSplitter(m_Pos, -m_Right, m_Up, -m_Dir) : *this;
	}
	float CalcDist(CRailSplitter &rhs){ return V3Len(&(rhs.m_Pos-m_Pos)); }
	void CalcCantAxis(float);
	CRailSplitter CalcMid(CRailSplitter *, float);
	char *Read(char *);
	void Save(FILE *, char *);
};

//	反復子
typedef list<CRailSplitter>::iterator IRailSplitter;
typedef list<CRailSplitter>::reverse_iterator RIRailSplitter;

/*
 *	コネクタリンク
 */
class CRailConnectorLink{
public:
	int m_Side;				//	サイド
	int m_Point;			//	ポイント
	float m_Cant;			//	カント量
	CRailConnector *m_Link;	//	リンク
public:
	CRailConnectorLink(){ m_Side = m_Point = 0; m_Link = NULL; }
	CRailConnectorLink::CRailConnectorLink(int side, int point, CRailConnector *link){
		m_Side = side; m_Point = point; m_Link = link;
	}
	void Connect(CRailWayLink &);
	void Disconnect();
	VEC3 GetPos();
	VEC3 GetRight();
	VEC3 GetUp();
	VEC3 GetDir();
	CRailSplitter GetSplitter(int);
	float GetCant();
	bool GetShadowTerminate();
	int GetLinkCount();
	void RestoreAddress();
	char *Read(char *, char *);
	void Save(FILE *, char *);
};

/*
 *	レールリンク
 */
class CRailWayLink{
public:
	int m_Side;			//	サイド
	CRailWay *m_Link;	//	リンク
public:
	CRailWayLink(){ m_Side = 0; m_Link = NULL; }
	CRailWayLink::CRailWayLink(int side, CRailWay *link){ m_Side = side; m_Link = link; }
	float GetCant();
	VEC3 GetPos();
	VEC3 GetOppositePos();
	VEC3 GetFirstDir();
	void WarpToOppositeEnd();
	void RestoreAddress();
	char *Read(char *, char *);
	void Save(FILE *, char *);
};

#endif
