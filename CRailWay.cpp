#include "stdafx.h"
#include "CRailSplitCurve.h"
#include "CRailDumpCurve.h"
#include "CLineBuildCurve.h"
#include "CRailDetectCurve.h"
#include "CTrainSetCurve.h"
#include "CRailConnector.h"
#include "CRailWay.h"
#include "CTrainGroup.h"
#include "CStation.h"
#include "CScene.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CSkinPlugin.h"
#include "CRailEditMode.h"

#define EXTEND_RAIL 0	//	延長設置

//	内部定数
const float RAIL_SEG_MIN = 4.0f;	//	レール分割最小値
const float WARP_DRAW_LEN = 0.1f;	//	ワープ表示長さ
const float POINT_DEC_MIN = 5.0f;	//	ポイント左右決定最小距離

//	外部グローバル
extern bool g_ShowWarpSelect;
extern D3DCOLOR g_ColorSelect[];
extern CDetectInfo g_StationPlatformParentDetectInfo;

//	内部グローバル
bool g_MultiTrackDummy = false;
int g_DummyTrackNum;
float g_DummyTrackInterval;
CGroupEndLocator g_HitTrainGroup;	//	衝突列車代入先
map<std::string, CTrainGroup *> g_RailBlockMap;	//	閉塞情報

/*
 *	動くレールの親オブジェクト情報
 */
class CRailWayParentLink{
	friend class CRailWay;
private:
	CDetectInfo m_DetectInfo;			//	親オブジェクト
	list<CRailSplitter> m_SplitList;	//	分割点リスト
};

//	static メンバ
CRailWay **CRailWay::ms_Root = NULL;
float CRailWay::ms_MinDist;
CRailWayLink CRailWay::ms_Detect;

/*
 *	コンストラクタ (読込用)
 */
CRailWay::CRailWay(){
	m_OldAdr = NULL;
	m_Selected = 0;
	m_Parent = NULL;
	m_Platform = NULL;
	m_BuildLine = true;
	m_MultiTrackDummy = false;
	m_DummyTrackNum = 0;
	m_DummyTrackInterval = 0.0f;
	m_WarpDummy = false;
	m_SpeedLimit = -1;
	m_Scene = g_Scene;
	m_RailPlugin = NULL;
	m_TiePlugin = NULL;
	m_GirderPlugin = NULL;
	m_PierPlugin = NULL;
	m_LinePlugin = NULL;
	m_PolePlugin = NULL;
	m_PierPos = m_PolePos = 0.0f;
	m_SegLen = -1.0f;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CRailWay::CRailWay(
	CRailConnectorLink &link1,	//	始点
	CRailConnectorLink &link2,	//	終点
	CRailPlugin *rpi,			//	レールプラグイン
	CTiePlugin *tpi,			//	枕木プラグイン
	CGirderPlugin *gpi,			//	橋桁プラグイン
	CPierPlugin *ipi,			//	橋脚プラグイン
	CLinePlugin *lpi,			//	架線プラグイン
	CPolePlugin *ppi,			//	架線柱プラグイン
	list<CRailSplitter> *splist	//	繋ぎ変え元リスト
){
	m_OldAdr = NULL;
	m_Selected = 0;
	m_Parent = NULL;
	m_Platform = NULL;
	m_BuildLine = false;
	if(m_MultiTrackDummy = g_MultiTrackDummy){
		m_DummyTrackNum = g_DummyTrackNum;
		m_DummyTrackInterval = g_DummyTrackInterval;
	}else{
		m_DummyTrackNum = 0;
		m_DummyTrackInterval = 0.0f;
	}
	m_WarpDummy = false;
	m_SpeedLimit = -1;
	m_Link[0] = link1;
	m_Link[1] = link2;
	m_Scene = g_Scene;
	m_RailPlugin = rpi;
	m_TiePlugin = tpi;
	m_GirderPlugin = gpi;
	m_PierPlugin = ipi;
	m_LinePlugin = lpi;
	m_PolePlugin = ppi;
	m_PierPos = m_PolePos = 0.0f;
	m_SegLen = -1.0f;
	if(splist){
		AddSplitter(m_Link[0].GetSplitter(true));
		m_SplitList.splice(m_SplitList.end(), *splist, splist->begin(), splist->end());
		SetMapTemp();
	}
	m_Next = *ms_Root;
	*ms_Root = this;
}

/*
 *	デストラクタ
 */
CRailWay::~CRailWay(){
	DELETE_V(m_Parent);
	DELETE_V(m_Next);
}

/*
 *	親オブジェクト設定
 */
void CRailWay::SetParent(CDetectInfo *d_info){
	DELETE_V(m_Parent);
	if(d_info){
		m_Parent = new CRailWayParentLink;
		m_Parent->m_DetectInfo = *d_info;
		m_Parent->m_SplitList = m_SplitList;
	}
}

/*
 *	ワープ設定
 */
void CRailWay::SetWarpDummy(){
	m_BuildLine = true;
	m_WarpDummy = true;
	m_Scene = NULL;
	Stabilize();
}

/*
 *	リンク可能か調べる
 */
bool CRailWay::IsLinkable(
	int s,		//	サイド
	VEC3 &pos	//	座標
){
	CRailConnectorLink& rlink = m_Link[s];
	if(pos!=rlink.GetPos()) return true;
	CRailWayLink *link = rlink.m_Link->m_Link[!rlink.m_Side];
	int i;
	for(i = 0; i<2; i++) if(link[i].m_Link && !link[i].m_Link->m_Scene) return false;
	return !link[0].m_Link || !link[1].m_Link;
}

/*
 *	セグメント長取得
 */
float CRailWay::GetSegLen(){
	if(m_SegLen>=0.0f) return m_SegLen;
	m_SegLen = 0.0f;
	if(m_WarpDummy) return 0.0f;
	IRailSplitter irs = m_SplitList.begin();
	VEC3 tpos = (irs++)->m_Pos;
	for(; irs!=m_SplitList.end(); irs++){
		VEC3 npos = irs->m_Pos;
		m_SegLen += V3Len(&(npos-tpos));
		tpos = npos;
	}
	return m_SegLen;
}

/*
 *	確定
 */
void CRailWay::Stabilize(){
	m_BuildLine = true;
	m_Link[0].Connect(R2L(CreateLink(0)));
	m_Link[1].Connect(R2L(CreateLink(1)));
	//	コンストラクタでやってるので不要
	//	if(m_RailPlugin) m_RailPlugin->CopyMapTemp(&m_RailMapV);
	//	if(m_TiePlugin) m_TiePlugin->CopyMapTemp(&m_TieMapV);
	//	if(m_GirderPlugin) m_GirderPlugin->CopyMapTemp(&m_GirderMapV);
	if(m_PierPlugin) m_PierPos = m_PierPlugin->GetPierPos();
	if(m_LinePlugin) m_LinePlugin->CopyMapTemp(m_LineMapV);
}

/*
 *	分割連結点取得
 */
CRailConnectorLink CRailWay::SplitLink(
	CRailLinkTemp *spl,	//	分割点
	CRailWay **wadr,	//	接続レール格納先
	IRailSplitter sit	//	splice 位置
){
	int s = spl->m_Side;
	if(spl->m_Pos==m_Link[s].GetPos()){
		int cs = m_Link[s].m_Side;
		CRailConnector *con = m_Link[s].m_Link;
		CRailWayLink *link = con->m_Link[!cs];
		if(wadr) *wadr = this;
		return con->CreateLink(!cs, link[0].m_Link ? 1 : 0);
	}
	CRailConnector *con = new CRailConnector(spl->m_Pos, R2L(s ? spl->m_Dir : -spl->m_Dir));
	con->Stabilize(spl->m_Up);
	CRailConnectorLink cl1 = con->CreateLink(1, 0), cl2 = m_Link[1];
	m_Link[1] = con->CreateLink(0, 0);
	m_Link[1].Connect(R2L(CreateLink(1)));
	m_SegLen = -1.0f;
	//GetSegLen(); CopyMapTemp で求めるので不要
	list<CRailSplitter> splist;
	IRailSplitter irs = m_SplitList.begin();
	bool splfound = false;
	for(; irs!=m_SplitList.end(); irs++){
		if(spl->m_Pos==irs->m_Pos){
			splfound = true;
			sit = ++irs;
			break;
		}
	}
	splist.splice(splist.begin(), m_SplitList, sit, m_SplitList.end());
	if(!splfound) m_SplitList.push_back(m_Link[1].GetSplitter(false));
	CopyMapTemp(1);
	CRailWay *way = new CRailWay(cl1, cl2, m_RailPlugin, m_TiePlugin,
		m_GirderPlugin, m_PierPlugin, m_LinePlugin, m_PolePlugin, &splist);
	way->m_MultiTrackDummy = m_MultiTrackDummy;
	way->m_DummyTrackNum = m_DummyTrackNum;
	way->m_DummyTrackInterval = m_DummyTrackInterval;
	way->GetSegLen();
	way->Stabilize();
	if(way->m_Platform = m_Platform) m_Platform->AddRailWay(way);
	way->m_RailBlock = m_RailBlock;
	way->m_SpeedLimit = m_SpeedLimit;
	m_PoleList.sort();
	IPolePos ippo = m_PoleList.begin();
	for(; ippo!=m_PoleList.end(); ippo++){
		if(ippo->m_Pos>=m_SegLen){
			IPolePos ippo2 = ippo;
			for(; ippo2!=m_PoleList.end(); ippo2++) ippo2->m_Pos -= m_SegLen;
			way->m_PoleList.splice(way->m_PoleList.begin(), m_PoleList, ippo, m_PoleList.end());
			break;
		}
	}
	m_PierList.sort();
	IPierPos ippi = m_PierList.begin();
	for(; ippi!=m_PierList.end(); ippi++){
		if(ippi->m_Pos>=m_SegLen){
			IPierPos ippi2 = ippi;
			for(; ippi2!=m_PierList.end(); ippi2++) ippi2->m_Pos -= m_SegLen;
			way->m_PierList.splice(way->m_PierList.begin(), m_PierList, ippi, m_PierList.end());
			break;
		}
	}
	float len0 = GetSegLen(), len1 = way->GetSegLen();
	IPGroupEndLocator ipge = m_GroupEnd.begin();
	while(ipge!=m_GroupEnd.end()){
		if((*ipge)->m_Side ? (*ipge)->m_Offset<len1 : (*ipge)->m_Offset>len0){
			(*ipge)->m_SetRail = way;
			if(!(*ipge)->m_Side) (*ipge)->m_Offset -= len0;
			way->m_GroupEnd.push_back(*ipge);
			ipge = m_GroupEnd.erase(ipge);
		}else{
			ipge++;
		}
	}
	if(wadr) *wadr = spl->m_Side ? this : way;
	return con->CreateLink(s, 1);
}

/*
 *	選択部分を分離
 */
void CRailWay::SplitSelect(){
	IRailSplitter irs = ++m_SplitList.begin(), spl = irs;
	int sel = (irs++)->m_Selected&2;
	for(; irs!=m_SplitList.end(); irs++){
		if(sel!=(irs->m_Selected&2)){
			CRailWay *newway;
			SplitLink(&CRailLinkTemp(0, 0.0f,
				spl->m_Pos, R2L(-spl->m_Right), spl->m_Up, R2L(-spl->m_Dir),
				NULL, IRailSplitter()), &newway, IRailSplitter());
			newway->SplitSelect();
			return;
		}
		spl = irs;
	}
}

/*
 *	選択されていればリストから削除
 */
CRailWay *CRailWay::Delete(){
	if(m_GroupEnd.size()
		|| m_Link[0].m_Link && m_Link[0].m_Link->m_User
		|| m_Link[1].m_Link && m_Link[1].m_Link->m_User) return this;
	if(m_WarpDummy){
		if(!(m_Selected&2) && m_Link[0].GetLinkCount()>1
			&& m_Link[1].GetLinkCount()>1) return this;
	}else{
		if(!(m_SplitList.rbegin()->m_Selected&2)) return this;
		if(m_Platform) m_Platform->DeleteRailWay(this);
		while(m_PoleList.size()){
			if(m_PoleList.begin()->m_Multi){
				m_PoleList.pop_front();
			}else{
				CPole *pole = m_PoleList.begin()->m_Link;
				m_PoleList.pop_front();
				m_Scene->DeletePole(pole);
			}
		}
		while(m_PierList.size()){
			CPier *pier = m_PierList.begin()->m_Link;
			m_PierList.pop_front();
			m_Scene->DeletePier(pier);
		}
	}
	m_Link[0].Disconnect();
	m_Link[1].Disconnect();
	CRailWay *next = m_Next;
	m_Next = NULL;
	delete this;
	return next;
}

/*
 *	別のレールを接続
 */
void CRailWay::ConnectRailWay(int my_side, CRailWay *o_rail, int o_side, CScene *scene){
	CRailConnectorLink& rlink = m_Link[my_side];
	CRailConnectorLink& orlink = o_rail->m_Link[o_side];
	CRailWayLink *link = rlink.m_Link->m_Link[!rlink.m_Side];
	if(link[0].m_Link==o_rail || link[1].m_Link==o_rail) return;
	if(rlink.m_Link->GetUser() || orlink.m_Link->GetUser()) return;
	scene->ResetRailWayRoot();
	CRailSplitter spl = rlink.GetSplitter(false);
	CRailConnector *con = new CRailConnector(spl.m_Pos, spl.m_Dir);
	con->Stabilize(spl.m_Up);
	rlink.Disconnect();
	orlink.Disconnect();
	rlink = con->CreateLink(0, 0);
	rlink.Connect(R2L(CreateLink(my_side)));
	orlink = con->CreateLink(1, 0);
	orlink.Connect(R2L(o_rail->CreateLink(o_side)));
	scene->DeleteRailConnector();
	g_Scene->ResetRailConnectorRoot();
//	Dialog("CONNECTED!");
}

/*
 *	別のレールに分岐
 */
void CRailWay::BranchRailWay(int my_side, CRailWay *o_rail, int o_side, CScene *scene){
	CRailConnectorLink& rlink = m_Link[my_side];
	CRailConnectorLink& orlink = o_rail->m_Link[o_side];
	CRailWayLink *link = rlink.m_Link->m_Link[!rlink.m_Side];
	if(link[0].m_Link==o_rail || link[1].m_Link==o_rail) return;
	if(link[0].m_Link && link[1].m_Link) return;
	if(rlink.m_Link->GetUser() || orlink.m_Link->GetUser()) return;
	scene->ResetRailWayRoot();
	CRailConnector *con = rlink.m_Link;
	orlink.Disconnect();
	orlink = con->CreateLink(!rlink.m_Side, !!link[0].m_Link);
	orlink.Connect(R2L(o_rail->CreateLink(o_side)));
	scene->DeleteRailConnector();
	g_Scene->ResetRailConnectorRoot();
//	Dialog("BRANCHED!");
}

/*
 *	レールの接続を解除
 */
void CRailWay::DisconnectRailWay(int my_side, CScene *scene){
	CRailConnectorLink& rlink = m_Link[my_side];
	CRailWayLink *link = rlink.m_Link->m_Link[!rlink.m_Side];
	if(!link[0].m_Link && !link[1].m_Link) return;
	if(rlink.m_Link->GetUser()) return;
	scene->ResetRailWayRoot();
	CRailSplitter spl = rlink.GetSplitter(false);
	CRailConnector *con = new CRailConnector(spl.m_Pos, spl.m_Dir);
	con->Stabilize(spl.m_Up);
	rlink.Disconnect();
	rlink = con->CreateLink(0, 0);
	rlink.Connect(R2L(CreateLink(my_side)));
	scene->DeleteRailConnector();
	g_Scene->ResetRailConnectorRoot();
//	Dialog("DISCONNECTED!");
}

/*
 *	選択されていれば閉塞区間設定
 */
bool CRailWay::SetRailBlock(char *rb){
	if(m_WarpDummy || m_MultiTrackDummy) return false;
	if(!(m_SplitList.rbegin()->m_Selected&2)) return false;
	m_RailBlock = rb;
	return true;
}

/*
 *	進入可能性チェック
 */
bool CRailWay::CheckRailBlock(CTrainGroup *group){
	if(IsRailBlock()){
		CTrainGroup *blockuser = GetRailBlockUser(m_RailBlock);
		if(blockuser && blockuser!=group) return false;
	}
	return true;
}

/*
 *	選択されていれば制限速度設定
 */
bool CRailWay::SetSpeedLimit(int sl){
	if(m_WarpDummy || m_MultiTrackDummy) return false;
	if(!(m_SplitList.rbegin()->m_Selected&2)) return false;
	m_SpeedLimit = sl;
	return true;
}

/*
 *	マッピング座標セット
 */
void CRailWay::SetMapTemp(){
	GetSegLen();
	if(m_RailPlugin){
		m_RailPlugin->CopyMapTemp(m_RailMapV);
		m_RailPlugin->AddMapTemp(m_SegLen);
	}
	if(m_TiePlugin){
		m_TiePlugin->CopyMapTemp(m_TieMapV);
		m_TiePlugin->AddMapTemp(m_SegLen);
	}
	if(m_GirderPlugin){
		m_GirderPlugin->CopyMapTemp(m_GirderMapV);
		m_GirderPlugin->AddMapTemp(m_SegLen);
	}
	if(m_LinePlugin){
		m_LinePlugin->CopyMapTemp(m_LineMapV);
		m_LinePlugin->AddMapTemp(m_SegLen); // 本当は架線の折れ線距離を計算する必要があるが…
	}
}

/*
 *	マッピング座標コピー
 */
void CRailWay::CopyMapTemp(
	int side	//	サイド
){
	GetSegLen();
	if(side){
		if(m_RailPlugin){
			m_RailPlugin->SetMapTemp(m_RailMapV);
			m_RailPlugin->AddMapTemp(m_SegLen);
		}
		if(m_TiePlugin){
			m_TiePlugin->SetMapTemp(m_TieMapV);
			m_TiePlugin->AddMapTemp(m_SegLen);
		}
		if(m_GirderPlugin){
			m_GirderPlugin->SetMapTemp(m_GirderMapV);
			m_GirderPlugin->AddMapTemp(m_SegLen);
		}
		if(m_PierPlugin){
			m_PierPlugin->SetPierPos(m_PierPos);
			m_PierPlugin->AddPierPos(m_SegLen);
		}
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = --m_PoleList.end();
			m_LinePlugin->SetMapTemp(m_LineMapV);
			m_LinePlugin->SetPolePos(ippo->m_Pos);
			m_LinePlugin->AddPolePos(m_SegLen);
		}
	}else{
		if(m_RailPlugin) m_RailPlugin->ResetMapTemp();
		if(m_TiePlugin) m_TiePlugin->ResetMapTemp();
		if(m_GirderPlugin) m_GirderPlugin->ResetMapTemp();
		if(m_PierPlugin){
			m_PierPlugin->SetPierPos(m_SegLen-m_PierPos);
			m_PierPlugin->AddPierPos(m_SegLen);
		}
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = m_PoleList.begin();
			m_LinePlugin->SetMapTemp(m_LineMapV);
			m_LinePlugin->SetPolePos(m_SegLen-ippo->m_Pos);
			m_LinePlugin->AddPolePos(m_SegLen);
		}
	}
}

/*
 *	マッピング座標コピー (複線橋脚・架線柱位置)
 */
void CRailWay::CopyMapTempMulti(
	int side,		//	サイド
	float *pipos,	//	橋脚位置
	float *pisum,	//	累積距離
	int *pinum,		//	橋脚数
	float *popos,	//	架線柱位置
	float *posum,	//	累積距離
	int *ponum		//	架線柱数
){
	GetSegLen();
	if(side){
		if(m_PierPlugin){
			*pipos += m_PierPos;
			*pisum += m_SegLen;
			(*pinum)++;
		}
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = --m_PoleList.end();
			*popos += ippo->m_Pos;
			*posum += m_SegLen;
			(*ponum)++;
		}
	}else{
		if(m_PierPlugin){
			*pipos += m_SegLen-m_PierPos;
			*pisum += m_SegLen;
			(*pinum)++;
		}
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = m_PoleList.begin();
			*popos += m_SegLen-ippo->m_Pos;
			*posum += m_SegLen;
			(*ponum)++;
		}
	}
}

/*
 *	架線柱リンクの取得
 */
CPoleLink CRailWay::GetPoleLink(
	int side	//	サイド
){
	GetSegLen();
	if(side){
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = m_PoleList.end();
			ippo--;
			return CPoleLink(ippo->m_Link->CreateLink(1, ippo->m_Track));
		}
	}else{
		if(m_LinePlugin && m_PoleList.size()){
			IPolePos ippo = m_PoleList.begin();
			return CPoleLink(ippo->m_Link->CreateLink(0, ippo->m_Track));
		}
	}
	return CPoleLink();
}

/*
 *	架線柱をリストに追加
 */
void CRailWay::AddPole(
	float ofs,		//	現在位置からのオフセット
	CPole *pole,	//	架線柱
	int track,		//	軌道番号
	bool multi		//	複線用リンク
){
	m_PoleList.push_back(CPolePos(m_PolePos+ofs, pole, track, multi));
}

/*
 *	架線柱をリストに追加
 */
void CRailWay::InsertPole(
	float ofs,		//	端からのオフセット
	CPole *pole,	//	架線柱
	int track,		//	軌道番号
	bool multi		//	複線用リンク
){
	IPolePos ipo = m_PoleList.begin();
	while(ipo!=m_PoleList.end() && ipo->m_Pos<ofs) ipo++;
	m_PoleList.insert(ipo, CPolePos(ofs, pole, track, multi));
}

/*
 *	架線柱をリストから削除
 */
void CRailWay::DeletePole(
	CPole *pole	//	架線柱
){
	IPolePos ipo = m_PoleList.begin();
	while(ipo!=m_PoleList.end()){
		if(ipo->m_Link==pole) ipo = m_PoleList.erase(ipo);
		else ipo++;
	}
}

/*
 *	橋脚をリストに追加
 */
void CRailWay::AddPier(
	float ofs,	//	現在位置からのオフセット
	CPier *pier	//	架線柱
){
	m_PierList.push_back(CPierPos(m_PierPos+ofs, pier));
}

/*
 *	橋脚をリストに追加
 */
void CRailWay::InsertPier(
	float ofs,	//	端からのオフセット
	CPier *pier	//	架線柱
){
	IPierPos ipi = m_PierList.begin();
	while(ipi!=m_PierList.end() && ipi->m_Pos<ofs) ipi++;
	m_PierList.insert(ipi, CPierPos(ofs, pier));
}

/*
 *	橋脚をリストから削除
 */
void CRailWay::DeletePier(
	CPier *pier	//	架線柱
){
	IPierPos ipi = m_PierList.begin();
	while(ipi!=m_PierList.end()){
		if(ipi->m_Link==pier) ipi = m_PierList.erase(ipi);
		else ipi++;
	}
}

/*
 *	橋脚・架線建設
 */
void CRailWay::BuildLine(
	CPierPlugin *ipi,	//	橋脚プラグイン
	CLinePlugin *lpi,	//	架線プラグイン
	CPolePlugin *ppi	//	架線柱プラグイン
){
	if(m_BuildLine) return;
	if(m_PierPlugin = ipi) m_PierPos = m_PierPlugin->GetPierPos();
	m_LinePlugin = lpi;
	m_PolePlugin = ppi;
	m_BuildLine = true;
	bool initialsplit = false;
	if(!m_SplitList.size()){
		initialsplit = true;
		AddSplitter(m_Link[0].GetSplitter(true));
		CRailSplitCurve curve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, this);
		curve.Trace(
			R2L(m_Link[0].GetPos()), R2L(-m_Link[0].GetRight()), R2L(m_Link[0].GetUp()), R2L(-m_Link[0].GetDir()),
			R2L(m_Link[1].GetPos()), R2L(m_Link[1].GetRight()), R2L(m_Link[1].GetUp()), R2L(m_Link[1].GetDir()),
			R2L(m_Link[0].GetPos()), R2L(-m_Link[0].GetCant()), R2L(m_Link[1].GetPos()), R2L(m_Link[1].GetCant()));
	}
	if(m_Next) m_Next->BuildLine(m_PierPlugin, m_LinePlugin, m_PolePlugin);
	if(initialsplit) SetMapTemp();
	if(g_MultiTrackDummy){
		g_MultiTrackSegment = g_MultiTrackRailList.begin();
		for(; g_MultiTrackSegment!=g_MultiTrackRailList.end(); g_MultiTrackSegment++){
			ILPRailWay ilpr = --g_MultiTrackSegment->end();
			IPRailWay ipr = ilpr->begin();
			for(; ipr!=ilpr->end(); ipr++) if(*ipr==this) goto FOUND;
		}
FOUND:;
	}
	TraceRail(1, &CLineBuildCurve(this, m_RailPlugin, m_TiePlugin,
		m_GirderPlugin, m_PierPlugin, m_LinePlugin, m_PolePlugin));
	if(g_PlatformInst){
		m_Platform = g_PlatformInst;
		g_PlatformInst->AddRailWay(this);
		if(g_StationPlatformParentDetectInfo.GetObject()){
			m_Parent = new CRailWayParentLink;
			m_Parent->m_DetectInfo = g_StationPlatformParentDetectInfo;
			m_Parent->m_SplitList = m_SplitList;
		}
	}
}

/*
 *	セグメント表示
 */
void CRailWay::ShowSegment(){
	int i;
	for(i = 0; i<2; i++){
		g_SegmentObject.SetPos(m_Link[i].GetPos());
		g_SegmentObject.SetDir(-m_Link[i].GetDir(), m_Link[i].GetUp());
		g_SegmentObject.Render();
	}
}

/*
 *	入力チェック
 */
void CRailWay::ScanInput(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	if(m_MultiTrackDummy && !g_RailEditMode->IsModeActive()) return;
	switch(mode){
	case 0:
		TraceRailSplit(1, &CRailDetectCurve3D(
			this, m_RailPlugin, m_TiePlugin, m_GirderPlugin, rect1));
		break;
	case 1:
		TraceRailSplit(1, &CRailDetectCurve2D(
			this, m_RailPlugin, m_TiePlugin, m_GirderPlugin, mode, rect1, rect2));
		break;
	case 2:
	case 3:
	case 4:
		TraceRail(1, &CRailDetectCurve2D(
			this, m_RailPlugin, m_TiePlugin, m_GirderPlugin, mode, rect1, rect2));
		break;
	case 5:
		if(m_MultiTrackDummy) return;
		CRailDetectCurve2D(this, m_RailPlugin, m_TiePlugin, m_GirderPlugin,
			mode, rect1, rect2).FinishTrace(
			R2L(m_Link[0].GetPos()), R2L(-m_Link[0].GetRight()), R2L(m_Link[0].GetUp()), R2L(-m_Link[0].GetDir()),
			R2L(m_Link[1].GetPos()), R2L(m_Link[1].GetRight()), R2L(m_Link[1].GetUp()), R2L(m_Link[1].GetDir()),
			(m_Link[0].GetLinkCount()>1 ? 0 : 1)+(m_Link[1].GetLinkCount()>1 ? 0 : 2),
			0.0f, *m_SplitList.begin());
		break;
	}
}

/*
 *	入力チェック (ワープ用)
 */
void CRailWay::ScanInputWarp(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	int i;
	bool flag = false;
	for(i = 0; i<2; i++){
		if(m_Link[i].m_Link->GetScene()!=g_Scene) continue;
		VEC3 center = m_Link[i].GetPos();
		VEC3 sc = WorldToScreen(center);
		if(sc.z<0.0f) continue;
		switch(mode){
		case 2:
			if(rect1.x<=sc.x && sc.x<=rect2.x && rect1.y<=sc.y && sc.y<=rect2.y){
				m_Selected |= 1;
				flag = true;
			}
			if(i && !flag) m_Selected &= 2;
			break;
		case 3:
			if(CheckCtrl()){
				m_Selected &= ~m_Selected<<1;
			}else{
				if(CheckShift()) m_Selected |= m_Selected<<1;
				else m_Selected = m_Selected<<1;
			}
			m_Selected &= 2;
			return;	//	1 回でよい
		case 4:
			float dist = V3Len(&(rect1-sc));
			if(dist<DETECT_2D_MAX && (ms_MinDist<0.0f || ms_MinDist>dist)){
				ms_MinDist = dist;
				ms_Detect = CRailWayLink(i, this);
			}
			m_Selected &= 2;
			break;
		}
	}
}

class CRailWay_TraceRail_CTempTracer{
public:
	template<class _II>
	CRailWay_TraceRail_CTempTracer(_II from, _II to, CRailTraceCurve *curve, bool rev){
		float sumlen = 0.0f;
		bool t1 = true;
		CRailSplitter tspl = (from++)->Get(rev);
		while(true){
			CRailTraceCurve::SetSplitItr(from);
			CRailTraceCurve::SetTerminate(t1, from==to);
			CRailSplitter nspl = from->Get(rev);
			float seglen = nspl.CalcDist(tspl);
			if(!curve->Confirm(tspl.m_Pos, nspl.m_Pos)) return;
			curve->FinishTrace(tspl.m_Pos, tspl.m_Right, tspl.m_Up, tspl.m_Dir,
				nspl.m_Pos, nspl.m_Right, nspl.m_Up, nspl.m_Dir, sumlen, seglen, *from);
			if(from==to) return;
			from++;
			sumlen += seglen;
			tspl = nspl;
			t1 = false;
		}
	}
};

/*
 *	トレース
 */
void CRailWay::TraceRail(
	int side,				//	方向
	CRailTraceCurve *curve	//	トレーサ
){
	if(side) CRailWay_TraceRail_CTempTracer(m_SplitList.begin(), --m_SplitList.end(), curve, false);
	else CRailWay_TraceRail_CTempTracer(m_SplitList.rbegin(), --m_SplitList.rend(), curve, true);
}

class CRailWay_TraceRailSplit_CTempTracerSplit{
public:
	template<class _II>
	CRailWay_TraceRailSplit_CTempTracerSplit(_II from, _II to, CRailTraceCurve *curve, bool rev){
		float sumlen = 0.0f;
		bool t1 = true, t2;
		CRailSplitter tspl = (from++)->Get(rev);
		while(true){
			CRailTraceCurve::SetTerminate(t1, t2 = from==to);
			CRailSplitter nspl = from->Get(rev);
			float seglen = nspl.CalcDist(tspl), sumnext = sumlen+seglen;
			int i, div = (int)(seglen/RAIL_SEG_MIN);
			if(div>1){
				float seglen2 = seglen/div;
				CRailSplitter tspl2 = tspl;
				for(i = 1; i<=div; i++){
					CRailTraceCurve::SetSplitItr(from);
					CRailTraceCurve::SetTerminate(t1, t2 && i==div);
					CRailSplitter nspl2 = nspl.CalcMid(&tspl, (float)i/div);
					if(!curve->Confirm(tspl2.m_Pos, nspl2.m_Pos)) return;
					curve->FinishTrace(
						tspl2.m_Pos, tspl2.m_Right, tspl2.m_Up, tspl2.m_Dir,
						nspl2.m_Pos, nspl2.m_Right, nspl2.m_Up, nspl2.m_Dir,
						sumlen, seglen2, *from);
					sumlen += seglen2;
					tspl2 = nspl2;
					t1 = false;
				}
			}else{
				CRailTraceCurve::SetSplitItr(from);
				if(!curve->Confirm(tspl.m_Pos, nspl.m_Pos)) return;
				curve->FinishTrace(tspl.m_Pos, tspl.m_Right, tspl.m_Up, tspl.m_Dir,
					nspl.m_Pos, nspl.m_Right, nspl.m_Up, nspl.m_Dir, sumlen, seglen, *from);
			}
			if(from==to) return;
			from++;
			sumlen = sumnext;
			tspl = nspl;
			t1 = false;
		}
	}
};

/*
 *	詳細トレース
 */
void CRailWay::TraceRailSplit(
	int side,				//	方向
	CRailTraceCurve *curve	//	トレーサ
){
	if(side) CRailWay_TraceRailSplit_CTempTracerSplit(m_SplitList.begin(), --m_SplitList.end(), curve, false);
	else CRailWay_TraceRailSplit_CTempTracerSplit(m_SplitList.rbegin(), --m_SplitList.rend(), curve, true);
}

class CRailWay_GetFirstDir_CSolver{
public:
	template<class _II>
	VEC3 operator()(_II from, _II to){
		VEC3 first = from->m_Pos, dir;
		_II prev = from++;
		float sumlen = 0.0f;
		while(true){
			dir = from->m_Pos-first;
			sumlen += V3Len(&(from->m_Pos-prev->m_Pos));
			if(from==to || sumlen>=POINT_DEC_MIN) break;
			prev = from++;
		}
		return dir;
	}
};

/*
 *	最初の分割点までの dir ベクトルを求める
 */
VEC3 CRailWay::GetFirstDir(
	int side	//	サイド
){
	return side ? CRailWay_GetFirstDir_CSolver()(m_SplitList.rbegin(), --m_SplitList.rend())
		: CRailWay_GetFirstDir_CSolver()(m_SplitList.begin(), --m_SplitList.end());
}

/*
 *	編成撤去
 */
void CRailWay::RemoveGroup(){
	while(m_GroupEnd.size()) (*m_GroupEnd.begin())->m_Group->Remove();
}

/*
 *	ワープ端のシーンをチェック
 */
void CRailWay::CheckWarpEndScene(
	CScene *scene	//	シーン
){
	int i, j;
	m_Selected = 0;
	for(i = 0; i<2; i++){
		for(j = 0; j<2; j++){
			CRailWay *way = m_Link[i].m_Link->m_Link[!m_Link[i].m_Side][j].m_Link;
			if(way && way->GetScene()==scene) m_Selected = 2;
		}
	}
}

/*
 *	編成の中か調べる
 */
bool CRailWay::IsInsideGroup(
	int side,	//	方向
	float ofs	//	オフセット
){
	set<CTrainGroup *> inset, erset;
	CTrainGroup *conuser[2];
	int i;
	for(i = 0; i<2; i++){
		conuser[i] = m_Link[i].m_Link ? m_Link[i].m_Link->m_User : NULL;
		if(conuser[i]) inset.insert(conuser[i]);
	}
	IPGroupEndLocator ipge = m_GroupEnd.begin();
	for(; ipge!=m_GroupEnd.end(); ipge++){
		float end = (*ipge)->m_Offset;
		int ts = (*ipge)->m_Side;
		CTrainGroup *tg = (*ipge)->m_Group;
		if(ts==side){
			end = GetSegLen()-end;
			if(end<=ofs) erset.insert(tg);
			else if(conuser[ts]==tg) return true;
			else inset.insert(tg);
		}else{
			if(ofs<=end) erset.insert(tg);
			else if(conuser[ts]==tg) return true;
			else inset.insert(tg);
		}
	}
	set<CTrainGroup *>::iterator ersitr = erset.begin();
	for(; ersitr!=erset.end(); ersitr++) inset.erase(*ersitr);
	if(inset.size()){
		//Dialog("inside %d", inset.size());
		return true;
	}
	return false;
}

/*
 *	車輌設置
 */
bool CRailWay::SetTrain(
	int side,				//	方向
	float ofs,				//	オフセット
	CGroupEndLocator *tail,	//	最後尾レール格納先
	bool rev,				//	後退フラグ
	ITrainSetBuffer *icur,	//	現在位置
	ITrainSetBuffer *iend,	//	終了位置
	CTrainGroup *group,		//	編成
	bool extend,			//	拡張トレール
	bool hittest			//	衝突判定
){
	if(!CheckRailBlock(group)) return false;
	CRailWay *ptr = this, *prev = NULL;
	float tmp;
	if(extend && ofs+(rev ? -(*icur)->m_SumLen : (*icur)->m_SumLen)<0.0f){
		CRailConnectorLink &con = ptr->m_Link[!side];
		VEC3 pos = con.GetPos(), dir = -con.GetDir(), up = con.GetUp();
		while((tmp = (rev ? -(*icur)->m_SumLen : (*icur)->m_SumLen)+ofs)<0.0f){
			(*icur)->SetPosture(pos+dir*tmp, rev ? dir : -dir, up, ptr);
			if(rev) (*icur)--; else (*icur)++;
			if(*icur==*iend){
				tail->m_Side = !side;
				tail->m_Offset = tmp;
				tail->m_SetRail = ptr;
				return true;
			}
		}
	}
	while(ptr){
		if(ptr->IsRailBlock()) SetRailBlockUser(ptr->GetRailBlock(), group);
		if(ptr->IsSpeedLimit()) group->SetSpeedLimit(ptr->GetSpeedLimit());
		if(ptr->m_WarpDummy){
			group->NotifyWarp();
		}else{
			if(hittest){
				tmp = (rev ? ++ITrainSetBuffer(*iend)
					: --ITrainSetBuffer(*iend))->m_SumLen+ofs;
				IPGroupEndLocator ipge = ptr->m_GroupEnd.begin();
				for(; ipge!=ptr->m_GroupEnd.end(); ipge++){
					float fend = (*ipge)->m_Offset;
					if((*ipge)->m_Side==side) fend = ptr->GetSegLen()-fend;
					if(ofs-0.001f<=fend && fend<=tmp+0.001f){
						return false;
					}
				}
			}
			CTrainSetCurve curve(ptr->m_RailPlugin, ptr->m_TiePlugin, ptr->m_GirderPlugin,
				rev, CGroupEndLocator(!side, ofs, ptr, group), tail, icur, iend);
			ptr->TraceRail(side, &curve);
			if(*icur==*iend) return true;
			ofs -= ptr->GetSegLen();
		}
		prev = ptr;
		CRailConnectorLink &con = ptr->m_Link[side];
		CRailConnector *pcon = con.m_Link;	
		if(pcon->GetUser() || !pcon->CheckRailBlock(group)) return false;
		pcon->SetUser(group);
		group->AddPoint(pcon);
		pcon->m_TrailPoint[con.m_Side] = con.m_Point;
		pcon->m_Side = con.m_Side;
		CRailWayLink &next = pcon->m_Link[!con.m_Side][pcon->m_TrailPoint[!con.m_Side]];
		if(ptr = next.m_Link) side = !next.m_Side;
	}
	if(extend && prev){
		CRailConnectorLink &con = prev->m_Link[side];
		VEC3 pos = con.GetPos(), dir = con.GetDir(), up = con.GetUp();
		while(*icur!=*iend){
			tmp = (rev ? -(*icur)->m_SumLen : (*icur)->m_SumLen)+ofs;
			(*icur)->SetPosture(pos+dir*tmp, rev ? dir : -dir, up, prev);
			if(rev) (*icur)--; else (*icur)++;
		}
		tail->m_Side = !side;
		tail->m_Offset = prev->GetSegLen()+tmp;
		tail->m_SetRail = prev;
		return true;
	}
	return false;
}

/*
 *	車輌進行
 *
 *	戻り値 (0: succeed, 1: railend, 2: hit train)
 */
int CRailWay::MarchTrain(
	float *limit,			//	進行距離
	CGroupEndLocator *head,	//	終端格納先
	CTrainGroup *caller,	//	呼び出し元(予約時のみ)
	CTrainGroup *group		//	編成(閉塞チェック用)
){
	float ofs = head->m_Offset;
	bool fin = false;
	CRailWay *ptr = this, *prev = NULL;
	head->m_SetRail = NULL;
	while(ptr){
		float tmp = *limit+ofs, seg = ptr->GetSegLen();
		if(caller && ptr->IsRailBlock()) SetRailBlockUser(ptr->GetRailBlock(), caller);
		if(caller && ptr->IsSpeedLimit()) caller->SetSpeedLimit(ptr->GetSpeedLimit());
		if(!ptr->m_WarpDummy){
			if(caller && ptr->m_Platform) caller->NotifyPlatform(
				ptr->m_Platform, -ofs, !!head->m_Side);
			IPGroupEndLocator ipge = ptr->m_GroupEnd.begin();
			for(; ipge!=ptr->m_GroupEnd.end(); ipge++){
				if(caller && caller==(*ipge)->m_Group) continue;
				float end = (*ipge)->m_Offset;
				if((*ipge)->m_Side==head->m_Side) end = seg-end;
				if(ofs-0.001f<=end && end<=tmp+0.001f){
					g_HitTrainGroup = **ipge;
					head->m_Offset = end;
					head->m_SetRail = ptr;
					*limit = end-ofs;
					return 2;
				}
			}
			if(tmp<=seg){
				head->m_Offset = tmp;
				head->m_SetRail = ptr;
				return 0;
			}
		}
		prev = ptr;
		CRailConnectorLink &con = ptr->m_Link[head->m_Side];
		CRailConnector *pcon = con.m_Link;
		bool randomize = false;
		if(pcon->GetUser() || !pcon->CheckRailBlock(group)){
		//	if(!caller || caller!=pcon->GetUser()){
				head->m_Offset = seg;
				head->m_SetRail = ptr;
				*limit = seg-ofs;
				return 2;
		//	}
		}else if(g_ManualControl){
			randomize = true;
		}else if(caller){
			pcon->SetUser(caller);
			randomize = !caller->AddSeek(pcon);
		}
		ofs -= seg;
		int newpoint = 0;
		pcon->m_TrailPoint[con.m_Side] = con.m_Point;
		CRailWayLink *next2 = pcon->m_Link[!con.m_Side];
		if(next2[0].m_Link){
			if(next2[1].m_Link){
				if(randomize){
					//	Dialog("randomize\ncaller = %p", caller);
					if(g_ManualControl){
						newpoint = pcon->GetNetPoint();
					}else{
						CPointElement *pe = pcon->m_PointInst.Dequeue(caller);
						newpoint = pe->CalcPoint();	//	0: left, 1: right
					}
					VEC3 d1, d2, right = con.GetRight();
					V3Norm(&d1, &next2[0].GetFirstDir());
					V3Norm(&d2, &next2[1].GetFirstDir());
					if(V3Dot(&right, &d1)>V3Dot(&right, &d2)) newpoint = !newpoint;
				}else{
					newpoint = pcon->m_TrailPoint[!con.m_Side];
				}
			}else{
				newpoint = 0;
			}
		}else{
			if(next2[1].m_Link) newpoint = 1;
			else break;
		}
		pcon->m_Side = !con.m_Side;
		pcon->m_TrailPoint[pcon->m_Side] = newpoint;
		CRailWayLink &next = next2[newpoint];
		ptr = next.m_Link;
		head->m_Side = !next.m_Side;
	}
#if EXTEND_RAIL
	if(prev){	//	延長設置
		head->m_SetRail = prev;
		head->m_Offset = ofs+prev->GetSegLen()+*limit;
		return 0;
	}
#endif
	head->m_Offset = prev->GetSegLen();
	head->m_SetRail = prev;
	*limit = -ofs;
	return 1;
}

/*
 *	プラットフォームが続いているかどうか調べる
 */
bool CRailWay::CheckPlatformExtend(
	int side,			//	方向
	CPlatformInst *pf	//	プラットフォーム
){
	CRailWay *ptr = this;
	while(ptr){
		if(ptr->m_Platform!=pf || ptr->m_WarpDummy) return true;
		CRailConnectorLink &con = ptr->m_Link[side];
		CRailConnector *pcon = con.m_Link;
		CRailWayLink *next2 = pcon->m_Link[!con.m_Side];
		int point = 0;
		if(next2[0].m_Link){
			if(next2[1].m_Link) return true;
			else point = 0;
		}else{
			if(next2[1].m_Link) point = 1;
			else return false;
		}
		ptr = next2[point].m_Link;
		side = !next2[point].m_Side;
	}
	return false;
}

/*
 *	状態取得 (1, 2: approach, 4: stop)
 */
int CRailWay::GetPlatformState(){
	int state = 0;
	if(m_Link[0].m_Link && m_Link[0].m_Link->m_User
		&& m_Link[1].m_Link && m_Link[1].m_Link->m_User==m_Link[0].m_Link->m_User){
		state |= m_Link[0].m_Link->m_Side==m_Link[0].m_Side ? 2 : 1;
		if(m_Link[0].m_Link->m_User->GetState()==2) state |= 4;
	}
	IPGroupEndLocator ipge = m_GroupEnd.begin();
	for(; ipge!=m_GroupEnd.end(); ipge++){
		state |= (*ipge)->GetDirection() ? 2 : 1;
		if((*ipge)->m_Group->GetState()==2) state |= 4;
	}
	return state;
}

/*
 *	動くレールの処理
 */
void CRailWay::UpdateSplitList(){
	if(!m_Parent) return;
	MTX4 mtx = m_Parent->m_DetectInfo.GetPartsInst()->GetObject()->GetMatrix();
	NormalizeMatrix(&mtx);
	if(m_SplitList.size()!=m_Parent->m_SplitList.size()) m_SplitList = m_Parent->m_SplitList;
	list<CRailSplitter>::iterator itr1 = m_SplitList.begin(), itr2 = m_Parent->m_SplitList.begin();
	for(; itr1!=m_SplitList.end(); ++itr1, ++itr2){
		D3DXVec3TransformCoord(&itr1->m_Pos, &itr2->m_Pos, &mtx);
		D3DXVec3TransformNormal(&itr1->m_Right, &itr2->m_Right, &mtx);
		D3DXVec3TransformNormal(&itr1->m_Up, &itr2->m_Up, &mtx);
		D3DXVec3TransformNormal(&itr1->m_Dir, &itr2->m_Dir, &mtx);
	}
	m_Link[0].m_Link->m_Splitter = m_SplitList.front();
	m_Link[1].m_Link->m_Splitter = m_SplitList.back();
}

/*
 *	断面をダンプ
 */
void CRailWay::Dump(){
	if(!m_RailPlugin && !m_TiePlugin && !m_GirderPlugin) return;
	if(m_Parent) return;
	if(m_RailPlugin) m_RailPlugin->SetMapTemp(m_RailMapV);
	if(m_TiePlugin) m_TiePlugin->SetMapTemp(m_TieMapV);
	if(m_GirderPlugin) m_GirderPlugin->SetMapTemp(m_GirderMapV);
	g_MultiTrackDummy = m_MultiTrackDummy;
	TraceRail(1, &CRailDumpCurve(m_RailPlugin, m_TiePlugin, m_GirderPlugin));
}

/*
 *	等間隔オブジェクトをレンダリング
 */
void CRailWay::Render(){
#if 0
	devResetMatrix();
	devResetMaterial();
	devSetLighting(FALSE);
	devSetState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	IPGroupEndLocator ipge = m_GroupEnd.begin();
	for(; ipge!=m_GroupEnd.end(); ipge++){
		float end = (*ipge)->m_Offset;
		int side = (*ipge)->m_Side;
		CRailConnectorLink &con = m_Link[side];
		VEC3 tmp = con.GetPos()-con.GetDir()*end;
		D3DCOLOR col = side ? 0x80ff0000 : 0x800000ff;
		Draw3DLine(tmp, tmp+V3UP*10.0f, col, col);
	}
	devSetState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	devSetLighting(TRUE);
#endif
//	if(!g_ShadowNeeded && !g_ShowRailSelect
//		&& !(m_RailPlugin && m_RailPlugin->HasInterval())
//		&& !(m_TiePlugin && m_TiePlugin->HasInterval())
//		&& !(m_GirderPlugin && m_GirderPlugin->HasInterval())) return;
	if(m_RailPlugin) m_RailPlugin->SetMapTemp(m_RailMapV);
	if(m_TiePlugin) m_TiePlugin->SetMapTemp(m_TieMapV);
	if(m_GirderPlugin) m_GirderPlugin->SetMapTemp(m_GirderMapV);
	g_DummyTrackNum = m_DummyTrackNum;
	g_DummyTrackInterval = m_DummyTrackInterval;
	g_MultiTrackDummy = m_MultiTrackDummy;
	TraceRail(1, &CRailRenderCurve(m_RailPlugin, m_TiePlugin, m_GirderPlugin, this));
	//extern int g_GroupEndCount;
	//g_GroupEndCount += m_GroupEnd.size();
}

/*
 *	ワープの描画
 */
void CRailWay::RenderWarp(){
	int i;
	bool draw[2];
	D3DCOLOR lc = g_ShowWarpSelect && m_Selected
		? g_ColorSelect[m_Selected] : 0xff0080ff;
	D3DCOLOR lca = ScaleColor(lc, g_BlinkAlpha);
	for(i = 0; i<2; i++)
		if(draw[i] = m_Link[i].m_Link->GetScene()==g_Scene) m_Link[i].m_Link->Render(lc, false);
	if(draw[0] && draw[1]){
		Draw3DLineWithShadow(m_Link[0].GetPos(), m_Link[1].GetPos(), lca);
	}else{
		for(i = 0; i<2; i++){
			float len = WARP_DRAW_LEN*V3Len(&(GetVPos()-m_Link[i].GetPos()))*g_FovRatio;
			if(draw[i]) Draw3DLineWithShadow(m_Link[i].GetPos(),
				m_Link[i].GetPos()-m_Link[i].GetDir()*len, lca, lca&0x00ffffff);
		}
	}
}

/*
 *	アドレス復元
 */
void CRailWay::RestoreAddress(){
	m_Link[0].RestoreAddress();
	m_Link[1].RestoreAddress();
	m_Platform = (CPlatformInst *)ReplaceAdr(m_Platform);
	IPierPos ipi = m_PierList.begin();
	for(; ipi!=m_PierList.end(); ipi++) ipi->RestoreAddress();
	IPolePos ipo = m_PoleList.begin();
	for(; ipo!=m_PoleList.end(); ipo++) ipo->RestoreAddress();
	IPGroupEndLocator ipge = m_GroupEnd.begin();
	for(; ipge!=m_GroupEnd.end();){
		*ipge = (CGroupEndLocator *)ReplaceAdr(*ipge);
		if(*ipge){
			++ipge;
		}else{
			//Dialog("GroupEnd not found!");
			ipge = m_GroupEnd.erase(ipge);
		}
	}
}

/*
 *	読込
 */
char *CRailWay::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "RailWay"))){
		delete this;
		return NULL;
	}
	if(!(str = AsgnPointer(eee = str, "Address", &m_OldAdr))) throw CSynErr(eee);
	g_AddressMap[m_OldAdr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "RailPlugin", &pid))) throw CSynErr(eee);
	m_RailPlugin = g_RailPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "TiePlugin", &pid))) throw CSynErr(eee);
	m_TiePlugin = g_TiePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "GirderPlugin", &pid))) throw CSynErr(eee);
	m_GirderPlugin = g_GirderPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "PierPlugin", &pid))) throw CSynErr(eee);
	m_PierPlugin = g_PierPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "LinePlugin", &pid))) throw CSynErr(eee);
	m_LinePlugin = g_LinePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "PolePlugin", &pid))) throw CSynErr(eee);
	m_PolePlugin = g_PolePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = ReadMapVector(eee = str, "RailMapV", m_RailMapV))) throw CSynErr(eee);
	if(!(str = ReadMapVector(eee = str, "TieMapV", m_TieMapV))) throw CSynErr(eee);
	if(!(str = ReadMapVector(eee = str, "GirderMapV", m_GirderMapV))) throw CSynErr(eee);
	if(!(str = ReadMapVector(eee = str, "LineMapV", m_LineMapV))) throw CSynErr(eee);

	if(!(str = AsgnYesNo(eee = str, "MultiTrackDummy", &m_MultiTrackDummy))) throw CSynErr(eee);
	if(m_MultiTrackDummy){
		if(!(str = AsgnInteger(eee = str, "DummyTrackNum", &m_DummyTrackNum))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "DummyTrackInterval", &m_DummyTrackInterval))) throw CSynErr(eee);
	}
	if(!(str = m_Link[0].Read(eee = str, "Link0"))) throw CSynErr(eee);
	if(!(str = m_Link[1].Read(eee = str, "Link1"))) throw CSynErr(eee);
	if(!(str = AsgnPointer(eee = str, "Platform", (void **)&m_Platform))) throw CSynErr(eee);
	if(tmp = AsgnString(str, "RailBlock", &m_RailBlock)){
		str = tmp;
		m_RailBlock = RestoreDoubleQuote(m_RailBlock);
	}
	if(tmp = AsgnInteger(str, "SpeedLimit", &m_SpeedLimit)){
		str = tmp;
		if(m_SpeedLimit<0) m_SpeedLimit = -1;
	}

	if(!(str = BeginBlock(eee = str, "SplitList"))) throw CSynErr(eee);
	CRailSplitter spl;
	while(tmp = spl.Read(eee = str)){
		str = tmp;
		m_SplitList.push_back(spl);
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "PierList"))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "PierPos", &m_PierPos))) throw CSynErr(eee);
	CPierPos pierpos;
	while(tmp = pierpos.Read(eee = str)){
		str = tmp;
		m_PierList.push_back(pierpos);
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "PoleList"))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "PolePos", &m_PolePos))) throw CSynErr(eee);
	CPolePos polepos;
	while(tmp = polepos.Read(eee = str)){
		str = tmp;
		m_PoleList.push_back(polepos);
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(tmp = Assignment(str, "GroupEnd")){
		str = tmp;
		do{
			if(m_GroupEnd.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
			CGroupEndLocator *end;
			if(!(str = HexPointer(eee = str, (void **)&end))) throw CSynErr(eee);
			m_GroupEnd.push_back(end);
		} while(!(tmp = Character2(str, ';')));
		str = tmp;
	}

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	読込
 */
char *CRailWay::ReadWarp(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Warp"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	if(!(str = m_Link[0].Read(eee = str, "Link0"))) throw CSynErr(eee);
	if(!(str = m_Link[1].Read(eee = str, "Link1"))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	m_WarpDummy = true;
	m_Scene = NULL;
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CRailWay::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tRailWay{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tRailPlugin = \"%s\";\n", CheckPluginID(m_RailPlugin));
	fprintf(df, "\t\t\t\tTiePlugin = \"%s\";\n", CheckPluginID(m_TiePlugin));
	fprintf(df, "\t\t\t\tGirderPlugin = \"%s\";\n", CheckPluginID(m_GirderPlugin));
	fprintf(df, "\t\t\t\tPierPlugin = \"%s\";\n", CheckPluginID(m_PierPlugin));
	fprintf(df, "\t\t\t\tLinePlugin = \"%s\";\n", CheckPluginID(m_LinePlugin));
	fprintf(df, "\t\t\t\tPolePlugin = \"%s\";\n", CheckPluginID(m_PolePlugin));
	SaveMapVector(df, "\t\t\t\tRailMapV = ", m_RailMapV);
	SaveMapVector(df, "\t\t\t\tTieMapV = ", m_TieMapV);
	SaveMapVector(df, "\t\t\t\tGirderMapV = ", m_GirderMapV);
	SaveMapVector(df, "\t\t\t\tLineMapV = ", m_LineMapV);

	fprintf(df, "\t\t\t\tMultiTrackDummy = %s;\n", YESNO[m_MultiTrackDummy]);
	if(m_MultiTrackDummy){
		fprintf(df, "\t\t\t\tDummyTrackNum = %d;\n", m_DummyTrackNum);
		fprintf(df, "\t\t\t\tDummyTrackInterval = %f;\n", m_DummyTrackInterval);
	}
	m_Link[0].Save(df, "\t\t\t\tLink0 = ");
	m_Link[1].Save(df, "\t\t\t\tLink1 = ");
	fprintf(df, "\t\t\t\tPlatform = %p;\n", m_Platform);
	if(IsRailBlock()) fprintf(df, "\t\t\t\tRailBlock = \"%s\";\n", ExpandDoubleQuote(m_RailBlock).c_str());
	if(IsSpeedLimit()) fprintf(df, "\t\t\t\tSpeedLimit = %d;\n", m_SpeedLimit);

	fprintf(df, "\t\t\t\tSplitList{\n");
	list<CRailSplitter> *saving_split_list = m_Parent ? &m_Parent->m_SplitList : &m_SplitList;
	IRailSplitter irs = saving_split_list->begin();
	for(; irs!=saving_split_list->end(); irs++) irs->Save(df, "\t\t\t\t\t");
	fprintf(df, "\t\t\t\t}\n");

	fprintf(df, "\t\t\t\tPierList{\n");
	fprintf(df, "\t\t\t\t\tPierPos = %f;\n", m_PierPos);
	IPierPos ipi = m_PierList.begin();
	for(; ipi!=m_PierList.end(); ipi++) ipi->Save(df);
	fprintf(df, "\t\t\t\t}\n");

	fprintf(df, "\t\t\t\tPoleList{\n");
	fprintf(df, "\t\t\t\t\tPolePos = %f;\n", m_PolePos);
	IPolePos ipo = m_PoleList.begin();
	for(; ipo!=m_PoleList.end(); ipo++) ipo->Save(df);
	fprintf(df, "\t\t\t\t}\n");

	if(m_GroupEnd.size()){
		fprintf(df, "\t\t\t\tGroupEnd = ");
		IPGroupEndLocator ipge = m_GroupEnd.begin();
		for(; ipge!=m_GroupEnd.end(); ipge++)
			fprintf(df, ipge==m_GroupEnd.begin() ? "%p" : ", %p", *ipge);
		fprintf(df, ";\n");
	}

	fprintf(df, "\t\t\t}\n");
}

/*
 *	保存 (ワープ用)
 */
void CRailWay::SaveWarp(
	FILE *df	//	ファイル
){
	fprintf(df, "\tWarp{\n");
	fprintf(df, "\t\tAddress = %p;\n", this);
	m_Link[0].Save(df, "\t\tLink0 = ");
	m_Link[1].Save(df, "\t\tLink1 = ");
	fprintf(df, "\t}\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CTrainGroup* GetRailBlockUser(std::string& name){
	map<std::string, CTrainGroup *>::iterator itr = g_RailBlockMap.find(name);
	if(itr==g_RailBlockMap.end()) return NULL;
	return itr->second;
}

void SetRailBlockUser(std::string& name, CTrainGroup *group){
	g_RailBlockMap[name] = group;
}

void ClearRailBlockUser(CTrainGroup *group){
	if(group){
		map<std::string, CTrainGroup *>::iterator itr;
		for(itr = g_RailBlockMap.begin(); itr!=g_RailBlockMap.end(); ++itr){
			if(itr->second==group) itr->second = NULL;
		}
	}else{
		g_RailBlockMap.clear();
	}
}
