#include "stdafx.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CPierSelectMode.h"
#include "CRailBuildMode.h"

//	外部定数
extern const float RAIL_PREV_LEN;
extern const float RAIL_SEG_MAX;
extern const float TAPER_DIV_RATIO;

//	外部グローバル
extern bool g_RailMipMap;

/*
 *	[static]
 *	プレビュー
 */
void CPierPlugin::RenderPreview(){
	if(!ms_PreviewState || !g_Pier) return;
	if(g_Pier){
		devSetLighting(TRUE);
		bool multi = CRailwayMode::IsMultiTrack();
		int tnum = multi ? CRailwayMode::GetTrackNum() : 1;
		float tint = multi ? CRailwayMode::GetTrackInterval() : 0.0f;
		int j;
		for(j = g_Pier->IsMultiTrack() ? tnum : 0; j<=tnum; j++){
			float x = (j-(tnum-1)*0.5f)*tint;
			if(j==tnum){
				if(!g_Pier->IsMultiTrack()) break;
				x = 0.0f;
			}
			VEC3 p1;
			float tmp = -RAIL_PREV_LEN;
			while(true){
				p1 = VEC3(x, 0.0f, tmp)-(
					(g_Rail ? g_Rail->m_Height : 0.0f)+
					(g_Tie ? g_Tie->m_Height : 0.0f)+
					(g_Girder ? g_Girder->m_Height : 0.0f))*V3UP;
				g_Pier->Preview(p1,	20.0f);
				if(tmp>RAIL_PREV_LEN) break;
				tmp += g_Pier->m_Interval;
			}
		}
	}
}

/*
 *	ロード
 */
bool CPierPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "PierInfo"))) throw CSynErr(eee);
		if(tmp = AsgnInteger(eee = str, "TrackNum", &m_TrackNum)) str = tmp;
		else m_TrackNum = 1;
		if(m_TrackNum<1){
			throw CSynErr(eee, lang(InvalidTrackNum));
		}else if(m_TrackNum>1){
			if(!(str = AsgnFloat(eee = str, "TrackInterval", &m_TrackInterval))) throw CSynErr(eee);
		}
		string tdir;
		if(tmp = AsgnIdentifier(eee = str, "Direction", &tdir)){
			str = tmp;
			if(tdir=="Down") m_Direction = false;
			else if(tdir=="Up") m_Direction = true;
			else throw CSynErr(eee);
		}else{
			m_Direction = false;
		}
		if(!(str = AsgnFloat(eee = str, "Interval", &m_Interval))) throw CSynErr(eee);
		if(tmp = AsgnFloat(eee = str, "Offset", &m_Offset)) str = tmp;
		else m_Offset = 0.0f;
		if(!(str = AsgnFloat(eee = str, "BuildMinAlt", &m_BuildMinAlt))) throw CSynErr(eee);
		m_TaperX = m_TaperY = m_TaperZ = 0.0f;
		if(tmp = AsgnFloat(str, "TaperX", &m_TaperX)) str = tmp;
		if(tmp = AsgnFloat(str, "TaperY", &m_TaperY)) str = tmp;
		if(tmp = AsgnFloat(str, "TaperZ", &m_TaperZ)) str = tmp;
		if(m_TaperX<0.0f) m_TaperX = 0.0f;
		if(m_TaperY<0.0f) m_TaperX = 0.0f;
		if(m_TaperZ<0.0f) m_TaperX = 0.0f;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Joint"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_JointFile))) throw CSynErr(eee);
		if(tmp = AsgnFloat(eee = str, "ModelScale", &m_JointScale)) str = tmp;
		else m_JointScale = 1.0f;
		if(!(str = AsgnVector3D(eee = str, "JointToHeadLocal", &m_JointToHeadLocal))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Head"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_HeadFile))) throw CSynErr(eee);
		if(tmp = AsgnFloat(eee = str, "ModelScale", &m_HeadScale)) str = tmp;
		else m_HeadScale = 1.0f;
		if(!(str = AsgnVector3D(eee = str, "HeadToPierLocal", &m_HeadToPierLocal))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Base"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_BaseFile))) throw CSynErr(eee);
		if(tmp = AsgnFloat(eee = str, "ModelScale", &m_BaseScale)) str = tmp;
		else m_BaseScale = 1.0f;
		if(!(str = AsgnVector3D(eee = str, "BaseToPierLocal", &m_BaseToPierLocal))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = ReadProfile(eee = str))) throw CSynErr(eee);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	LoadData();
	m_JointMesh = g_MeshList.Get(FALSE, m_JointFile.c_str(), 0, !g_RailMipMap);
	ChDir();
	m_HeadMesh = g_MeshList.Get(FALSE, m_HeadFile.c_str(), 0, !g_RailMipMap);
	ChDir();
	m_BaseMesh = g_MeshList.Get(FALSE, m_BaseFile.c_str(), 0, !g_RailMipMap);
	ChDir();
	m_JointObject.SetMesh(m_JointMesh, V3ZERO, m_JointScale);
	m_HeadObject.SetMesh(m_HeadMesh, V3ZERO, m_HeadScale);
	m_BaseObject.SetMesh(m_BaseMesh, V3ZERO, m_BaseScale);
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CPierPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Pier = this;
	string desc = g_Pier->GetBasicInfo();
	if(m_TrackNum>1) desc += FlashIn("\n%s: %d\n%s: %.3f [m]",
		lang(TrackNum), m_TrackNum, lang(Interval), m_TrackInterval);
	desc += "\n"+g_Pier->GetDescription();
	g_PierSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	プレビュー
 */
void CPierPlugin::Preview(
	VEC3 pos,	//	位置
	float plen	//	橋脚部長さ
){
	VEC3 hpos = pos+m_JointToHeadLocal;
	VEC3 top = hpos+m_HeadToPierLocal;
	VEC3 bottom = top-V3UP*plen;
	VEC3 bpos = bottom-m_BaseToPierLocal;
	VEC3 begin, end, right, up = V3DIR, dir;
	if(m_Direction){
		begin = bottom;
		end = top;
		right = -V3RIGHT;
		dir = V3UP;
	}else{
		begin = top;
		end = bottom;
		right = V3RIGHT;
		dir = -V3UP;
	}
	m_JointObject.SetPos(pos);
	m_JointObject.SetDir(V3DIR, V3UP);
	m_JointObject.Render();
	CastShadow(&m_JointObject);
	m_HeadObject.SetPos(hpos);
	m_HeadObject.SetDir(V3DIR, V3UP);
	m_HeadObject.Render();
	CastShadow(&m_HeadObject);
	float tsx = 1.0f+plen*m_TaperX, tsy = 1.0f+plen*m_TaperY;
	m_BaseObject.SetPos(bpos);
	m_BaseObject.SetDir(V3DIR, V3UP);
	m_BaseObject.SetScale(tsx, 1.0f, tsy);
	m_BaseObject.Render();
	CastShadow(&m_BaseObject);
	devResetMatrix();
	devResetMaterial();
	ResetMapTemp();
	int i, div = 1+Round(((tsx>tsy ? tsx : tsy)-1.0f)/TAPER_DIV_RATIO);
	int smax = Round(2.0f*plen/RAIL_SEG_MAX);
	if(div<smax) div = smax;
	VEC3 tr1, tu1, tr2, tu2;
	if(m_Direction){
		tr1 = right*tsx; tu1 = up*tsy;
		tr2 = right; tu2 = up;
	}else{
		tr1 = right; tu1 = up;
		tr2 = right*tsx; tu2 = up*tsy;
	}
	float divarea = plen/div;
	for(i = 0; i<div; i++){
		float q12 = (float)i/div, q11 = 1.0f-q12;
		float q22 = (float)(i+1)/div, q21 = 1.0f-q22;
		VEC3 p1 = q11*begin+q12*end, r1 = q11*tr1+q12*tr2, u1 = q11*tu1+q12*tu2;
		VEC3 p2 = q21*begin+q22*end, r2 = q21*tr1+q22*tr2, u2 = q21*tu1+q22*tu2;
		Dump(p1, r1, u1, p1, r1, u1, p2, r2, u2, p2, r2, u2, divarea, 7);
	}
	if(plen<=0.0f || !HasInterval() && !g_ShadowNeeded) return;
	Render(begin, tr1, tu1, dir, begin, tr1, tu1,
		end, tr2, tu2, dir, end, tr2, tu2, 3, plen);
}

/*
 *	橋脚距離加算
 */
void CPierPlugin::AddPierPos(
	float len	//	距離
){
	m_PierPos -= len;
	while(m_PierPos<=0.0f) m_PierPos += m_Interval;
}
