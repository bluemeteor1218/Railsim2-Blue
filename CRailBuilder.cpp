#include "stdafx.h"
#include "RailMap.h"
#include "CInterface.h"
#include "CScene.h"
#include "CSurfacePlugin.h"
#include "CRailPlanCurve.h"
#include "CRailBuilder.h"
#include "CRailWay.h"
#include "CStation.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CSkinPlugin.h"

//	内部定数
extern const float CORNER_MIN_DIST = 4.5f;	//	制御点間最小距離

//	内部グローバル
CPlatformInst *g_PlatformInst = NULL;					//	カレントプラットフォーム
list<list<list<CRailWay *> > > g_MultiTrackRailList;	//	複線レールリスト
ILLPRailWay g_MultiTrackSegment;						//	カレント複線セグメント
ILPRailWay g_SingleTrackSegment;						//	カレント単線セグメント

//	static メンバ
int CRailBuilder::ms_CurrentTrack;
int CRailBuilder::ms_TrackNum;
float CRailBuilder::ms_TrackInterval;
float CRailBuilder::ms_TrackShift;
bool CRailBuilder::ms_LiftRailSurface;
VEC3 CRailBuilder::ms_BeginPosSum;
VEC3 CRailBuilder::ms_BeginDirSum;
VEC3 CRailBuilder::ms_EndPosSum;
VEC3 CRailBuilder::ms_EndDirSum;

/*
 *	[static]
 *	ベクトル総和リセット
 */
void CRailBuilder::ResetDirSum(){
	ms_BeginPosSum = ms_EndPosSum = V3ZERO;
	ms_BeginDirSum = ms_EndDirSum = V3ZERO;
	g_MultiTrackRailList.clear();
}

/*
 *	[static]
 *	トラック情報設定
 */
void CRailBuilder::SetTrack(
	int cur,	//	現在のトラック
	int tnum,	//	線数
	float tint,	//	線間隔
	bool lift	//	レール持ち上げ
){
	ms_CurrentTrack = cur;
	ms_TrackNum = tnum;
	ms_TrackInterval = tint;
	ms_LiftRailSurface = lift;
	float p = ms_TrackNum==ms_CurrentTrack ? 0.0f : 0.5f*(ms_TrackNum-1)-ms_CurrentTrack;
	CRailPlanCurve::SetRadiusDrawPos(p);
	ms_TrackShift = ms_TrackInterval*p;
	g_MultiTrackSegment = g_MultiTrackRailList.begin();
}

/*
 *	コンストラクタ
 */
CRailBuilder::CRailBuilder(
	VEC3 pos,			//	座標
	CRailBuilder *prev	//	前
){
	m_Pos = pos;
	m_HitFlag = false;
	if(m_Prev = prev) m_Prev->m_Next = this;
	m_Next = NULL;
}

/*
 *	デストラクタ
 */
CRailBuilder::~CRailBuilder(){
	DELETE_V(m_Next);
}

/*
 *	制御点削除
 */
CRailBuilder *CRailBuilder::Pop(){
	if(!IsLinkEmpty()){
		if(m_Link.rbegin()->m_Link){
			m_Link.pop_back();
			if(m_Link.size()) m_Link.pop_back();
			else goto POP;
		}else{
			m_Link.pop_back();
		}
		return this;
	}
POP:
	CRailBuilder *prev = m_Prev;
	if(prev){
		prev->m_Pos = m_Pos;
		prev->m_Next = NULL;
	}
	delete this;
	return prev;
}

/*
 *	座標設定
 */
VEC3 CRailBuilder::SetPos(
	VEC3 p,		//	座標
	int clip	//	クリップモード (1: 地下, 2: 高架)
){
	m_Pos = p;
	m_HitFlag = g_Scene->ClipAlt(&m_Pos, &m_HitPos, &m_HitNorm, clip);
	return m_Pos;
}

/*
 *	リンク設定
 */
bool CRailBuilder::SetLink(
	CRailLinkTemp &link	//	リンク
){
	if(m_Link.size()) m_Link.rbegin()->m_Link = NULL;
	else PushLink();
	if(link.m_Link && m_Prev){
		if(m_Prev->IsLast()) return false;
		if(m_Prev->CheckLink()){
			VEC3 tdir;
			V3Norm(&tdir, &(link.m_Pos-m_Prev->GetLink().m_Pos));
			float dpdot = V3Dot(&tdir, &m_Prev->GetLink().m_Dir);
			float dddot = -V3Dot(&link.m_Dir, &m_Prev->GetLink().m_Dir);
			if(dddot<-0.999f && fabsf(dpdot)>0.999f
				|| dddot>0.999f && dpdot<0.0f) return false;
		}
	}
	if(link.m_Link){
		*m_Link.rbegin() = link;
		VEC3 tpos = V3ZERO;
		int i, n = 0;
		for(i = 0; i<m_Link.size(); i++){
			if(m_Link[i].m_Link){
				tpos += m_Link[i].m_Pos;
				n++;
			}
		}
		SetPos(tpos/n, 0);
	}
	return true;
}

/*
 *	最終点か調べる
 */
bool CRailBuilder::IsLast(){
	return !m_Next || m_Prev && m_Link.size()==ms_TrackNum && m_Link[ms_TrackNum-1].m_Link
		|| V3Len(&(m_Next->m_Pos-m_Pos))<CORNER_MIN_DIST;
}

/*
 *	right ベクトル計算
 */
void CRailBuilder::CalcRight(
	VEC3 *r1, VEC3 *d1,	//	点 1
	VEC3 *r2, VEC3 *d2	//	点 2
){
	V3Norm(r1, &VEC3(-d1->z, 0.0f, d1->x));
	V3Norm(r2, &VEC3(-d2->z, 0.0f, d2->x));
}

/*
 *	分割座標計算
 */
VEC3 CRailBuilder::CalcSplitPos(){
	if(m_Next){
		if(m_Prev && m_Next->m_Next){
			float len0 = V3Len(&(m_Next->m_Pos-m_Pos));
			float len1 = V3Len(&(m_Pos-m_Prev->m_Pos));
			float len2 = V3Len(&(m_Next->m_Next->m_Pos-m_Next->m_Pos));
			if(!m_Prev->m_Prev) len1 *= 2.0f;
			if(m_Next->m_Next->IsLast()) len2 *= 2.0f;
			if(len0<len1) len1 = len0;
			if(len0<len2) len2 = len0;
			return (len2*m_Pos+len1*m_Next->m_Pos)/(len1+len2);
		}else{
			return 0.5f*(m_Pos+m_Next->m_Pos);
		}
	}else{
		return m_Pos;
	}
}

/*
 *	トラック座標計算
 */
VEC3 CRailBuilder::CalcTrackPos(
	VEC3 *right	//	右ベクトル
){
	return *right*ms_TrackShift;
}

/*
 *	曲線計算
 */
bool CRailBuilder::Curve(
	CRailCurve *curve,	//	曲線オブジェクト
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi	//	橋桁プラグイン
){
	VEC3 hfix = V3ZERO;
	if(ms_LiftRailSurface) hfix.y = rpi ? rpi->m_SurfaceAlt :
		(tpi ? tpi->m_Height : 0.0f)+(gpi ? gpi->m_Height : 0.0f);
	VEC3 pos1, right1, dir1, pos2, right2, dir2;
	bool comp = false, linked1 = false, linked2 = false;
	bool last0 = IsLast();
	bool last1 = last0 || !m_Next || m_Next->IsLast();
	bool last2 = last1 || !m_Next->m_Next || m_Next->m_Next->IsLast();
	if(m_Prev){
		if(!last0 && !last1){
			pos1 = CalcSplitPos()+hfix;
			dir1 = m_Next->m_Pos-m_Pos;
			V3Norm(&dir1, &dir1);
			if(m_Next->m_Next->CheckLink()){
				linked2 = true;
				pos2 = m_Next->m_Next->GetLink().m_Pos;
				dir2 = -m_Next->m_Next->GetLink().m_Dir;
			}else{
				pos2 = hfix+((comp = !last2)
					? m_Next->CalcSplitPos() : m_Next->m_Next->m_Pos);
				dir2 = m_Next->m_Next->m_Pos-m_Next->m_Pos;
			}
			V3Norm(&dir2, &dir2);
			if(ms_CurrentTrack==ms_TrackNum){
				if(last2){
					pos2 = ms_EndPosSum/ms_TrackNum;
					V3Norm(&dir2, &ms_EndDirSum);
				}
			}else{
				if(last2){
					ms_EndPosSum += pos2;
					ms_EndDirSum += dir2;
				}
			}
			CalcRight(&right1, &dir1, &right2, &dir2);
			if(linked2) right2 = V3ZERO;
			curve->Curve(R2L(pos1+CalcTrackPos(&right1)), dir1,
				R2L(pos2+CalcTrackPos(&right2)), dir2, comp, true);
			return true;
		}
	}else{
		if(!last0){
			if(CheckLink()){
				linked1 = true;
				pos1 = GetLink().m_Pos;
				dir1 = GetLink().m_Dir;
			}else{
				pos1 = m_Pos+hfix;
				dir1 = m_Next->m_Pos-m_Pos;
			}
			V3Norm(&dir1, &dir1);
			if(last1){
				if(m_Next->CheckLink()){
					linked2 = true;
					pos2 = m_Next->GetLink().m_Pos;
					dir2 = -m_Next->GetLink().m_Dir;
					if(!CheckLink()){
						VEC3 r = pos2-pos1;
						V3Norm(&r, &r);
						dir1 = -dir2+2.0f*V3Dot(&r, &dir2)*r;
					}
				}else{
					pos2 = m_Next->m_Pos+hfix;
					if(CheckLink()){
						VEC3 r = pos2-pos1;
						V3Norm(&r, &r);
						dir2 = -dir1+2.0f*V3Dot(&r, &dir1)*r;
					}else{
						dir2 = dir1;
					}
				}
			}else{
				if(m_Next->m_Next->CheckLink()){
					linked2 = true;
					pos2 = m_Next->m_Next->GetLink().m_Pos;
					dir2 = -m_Next->m_Next->GetLink().m_Dir;
				}else{
					pos2 = hfix+((comp = !last2)
						? m_Next->CalcSplitPos() : m_Next->m_Next->m_Pos);
					dir2 = m_Next->m_Next->m_Pos-m_Next->m_Pos;
				}
			}
			V3Norm(&dir2, &dir2);
			if(ms_CurrentTrack==ms_TrackNum){
				pos1 = ms_BeginPosSum/ms_TrackNum;
				V3Norm(&dir1, &ms_BeginDirSum);
				if(last1 || last2){
					pos2 = ms_EndPosSum/ms_TrackNum;
					V3Norm(&dir2, &ms_EndDirSum);
				}
			}else{
				ms_BeginPosSum += pos1;
				ms_BeginDirSum += dir1;
				if(last1 || last2){
					ms_EndPosSum += pos2;
					ms_EndDirSum += dir2;
				}
			}
			CalcRight(&right1, &dir1, &right2, &dir2);
			if(linked1) right1 = V3ZERO;
			if(linked2) right2 = V3ZERO;
			curve->Curve(R2L(pos1+CalcTrackPos(&right1)), dir1,
				R2L(pos2+CalcTrackPos(&right2)), dir2, comp, true);
			return true;
		}
	}
	return false;
}

/*
 *	レンダリング
 */
void CRailBuilder::Render(
	CLineDumpL *dump,	//	ダンパ
	CRailPlugin *rpi,	//	レールプラグイン
	CTiePlugin *tpi,	//	枕木プラグイン
	CGirderPlugin *gpi,	//	枕木プラグイン
	bool draw			//	描画フラグ
){
	VEC3 hfix = V3ZERO;
	if(ms_LiftRailSurface) hfix.y = rpi ? rpi->m_SurfaceAlt :
		(tpi ? tpi->m_Height : 0.0f)+(gpi ? gpi->m_Height : 0.0f);
	if(draw){
		bool renderlink = !IsLinkEmpty();
		int i, lnum = 0;
		if(renderlink){
			devSetZRead(TRUE);
			devSetZWrite(TRUE);
			devSetLighting(TRUE);
			for(i = 0; i<m_Link.size() && i<ms_TrackNum; i++){
				if(!m_Link[i].m_Link) continue;
				lnum++;
				g_LinkObject.SetPos(m_Link[i].m_Pos);
				g_LinkObject.SetDir(m_Link[i].m_Dir, m_Link[i].m_Up);
				g_LinkObject.Render();
			}
		}
		if(!m_Prev || renderlink){
			devSetZRead(FALSE);
			devSetZWrite(FALSE);
			devSetLighting(FALSE);
			devResetMaterial();
			devResetMatrix();
			if(renderlink){
				if(m_Prev) g_StrTex->RenderLeft(
					TILE_QUAD, TILE_UNIT*4+TILE_QUAD, 0xffffffff, 0xff000000,
					FlashIn("%s: %d/%d", lang(JointDest), lnum, ms_TrackNum));
				else g_StrTex->RenderLeft(
					TILE_QUAD, TILE_UNIT*3+TILE_QUAD, 0xffffffff, 0xff000000,
					FlashIn("%s: %d/%d", lang(JointSrc), lnum, ms_TrackNum));
			}
		}
	}
	devResetMatrix();
	CRailPlanCurve curve(dump);
	Curve(&curve, rpi, tpi, gpi);
	if(m_HitFlag){
		DrawTangent(m_HitPos, m_HitNorm, 0xffff0000, dump);
		dump->Add(m_HitPos, 0xffff0000, m_Pos, 0x80ff0000);
		//RailMapLine(m_HitPos, 0xffff0000, m_Pos, 0x80ff0000, false);
	}
	if(IsLast()){
		if(m_Next){
			if(m_Next->m_HitFlag){
				DrawTangent(m_Next->m_HitPos, m_Next->m_HitNorm, 0xff0000ff, dump);
				dump->Add(m_Next->m_HitPos, 0xff0000ff, m_Next->m_Pos, 0x800000ff);
				//RailMapLine(m_Next->m_HitPos, 0xff0000ff, m_Next->m_Pos, 0x800000ff, false);
			}
			VEC3 pp1 = m_Pos+(CheckLink() ? -hfix : V3ZERO);
			VEC3 pp2 = m_Next->m_Pos+(m_Next->CheckLink() ? -hfix : V3ZERO);
			dump->Add(pp1, 0xc0ff0000, pp2, 0xc0ff0000);
			RailMapLine(pp1, 0xc0ff0000, pp2, 0xc0ff0000, false);
		}
		if(draw){
			dump->Render(true);
			devSetZRead(TRUE);
			devSetZWrite(TRUE);
			devSetLighting(TRUE);
			g_ArrowObject.SetPos(m_Next ? m_Next->m_Pos : m_Pos);
			g_ArrowObject.RotY(2.0f*D3DX_PI/MAXFPS);
			g_ArrowObject.Render();
		}
	}else{
		VEC3 pp1 = m_Pos+(CheckLink() ? -hfix : V3ZERO);
		VEC3 pp2 = m_Next->m_Pos+(m_Next->CheckLink() ? -hfix : V3ZERO);
		dump->Add(pp1, 0xc0ff0000, pp2, 0xc0ff0000);
		RailMapLine(pp1, 0xc0ff0000, pp2, 0xc0ff0000, false);
		m_Next->Render(dump, rpi, tpi, gpi, draw);
	}
}

/*
 *	建設
 */
void CRailBuilder::BuildRail(
	CRailConnectorLink &begin,	//	開始リンク
	CRailConnectorLink &end,	//	終了リンク
	CRailPlugin *rpi,			//	レールプラグイン
	CTiePlugin *tpi,			//	枕木プラグイン
	CGirderPlugin *gpi			//	枕木プラグイン
){
	if(!ms_CurrentTrack){
		g_MultiTrackRailList.push_back(list<list<CRailWay *> >());
		g_MultiTrackSegment = --g_MultiTrackRailList.end();
	}
	g_MultiTrackSegment->push_back(list<CRailWay *>());
	g_SingleTrackSegment = --g_MultiTrackSegment->end();
	if(m_Next->IsLast() || m_Next->m_Next && m_Next->m_Next->IsLast()){
		CRailBuildCurve curve(begin, end, rpi, tpi, gpi);
		Curve(&curve, rpi, tpi, gpi);
	}else{
		CRailBuildCurve curve(begin, R2L(CRailConnectorLink()), rpi, tpi, gpi);
		Curve(&curve, rpi, tpi, gpi);
		g_MultiTrackSegment++;
		m_Next->BuildRail(curve.GetNext(), end, rpi, tpi, gpi);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CRailLinkTemp::CRailLinkTemp(
	int side,			//	サイド
	float sumlen,		//	積算距離
	VEC3 &pos,			//	座標
	VEC3 &right,		//	right
	VEC3 &up,			//	up
	VEC3 &dir,			//	dir
	CRailWay *link,		//	接続レール
	IRailSplitter sit	//	splice 位置
){
	m_Side = side;
	m_SumLen = sumlen;
	m_SpliceItr = sit;
	m_Pos = pos;
	m_Right = right;
	m_Up = up;
	m_Dir = dir;
	m_Link = link;
}
