#include "stdafx.h"
#include "CRailPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CLineSelectMode.h"
#include "CRailBuildMode.h"

//	外部定数
extern const float RAIL_PREV_LEN;

/*
 *	[static]
 *	プレビュー
 */
void CLinePlugin::RenderPreview(){
	if(!ms_PreviewState || (!g_Line && !g_Pole)) return;
	VEC3 op1(0.0f, 0.0f, -RAIL_PREV_LEN), op2(0.0f, 0.0f, RAIL_PREV_LEN);
	VEC3 r1 = V3RIGHT, u1 = V3UP, d1 = V3DIR;
	VEC3 r2 = V3RIGHT, u2 = V3UP, d2 = V3DIR;
	if(g_Line){
		bool multi = CRailwayMode::IsMultiTrack();
		int tnum = multi ? CRailwayMode::GetTrackNum() : 1;
		float tint = multi ? CRailwayMode::GetTrackInterval() : 0.0f;
		if(g_Pole && g_Pole->IsMultiTrack()){
			if(tnum>g_Pole->GetTrackNum()) tnum = g_Pole->GetTrackNum();
			tint = g_Pole->GetTrackInterval();
		}
		int j;
		for(j = 0; j<=tnum; j++){
			float x = (j-(tnum-1)*0.5f)*tint;
			if(j==tnum){
				if(g_Pole && !g_Pole->IsMultiTrack()) break;
				x = 0.0f;
			}
			g_Line->ResetMapTemp();
			bool p_flag = g_Pole && (g_Pole->IsMultiTrack() ? j==tnum : j<tnum);
			VEC3 p1, ip1, p2, ip2;
			float tmp = -RAIL_PREV_LEN;
			do{
				ip1 = p1 = VEC3(x, 0.0f, tmp)+g_Line->m_TrolleyAlt*u1;
				ip2 = p2 = p1+g_Line->m_MaxInterval*V3DIR;
				devSetLighting(TRUE);
				if(p_flag) g_Pole->Preview(p1+g_Line->m_Height*u1);
				devResetMatrix();
				if(j<tnum) g_Line->Dump(p1, r1, u1, ip1, r1, u1,
					p2, r2, u2, ip2, r2, u2, g_Line->m_MaxInterval, 7);
				p1 = ip1; p2 = ip2;
				g_Line->Render(p1, r1, u1, d1, ip1, r1, u1,
					p2, r2, u2, d2, ip2, r2, u2, 3, g_Line->m_MaxInterval);
				tmp += g_Line->m_MaxInterval;
			} while(tmp<RAIL_PREV_LEN);
			if(p_flag) g_Pole->Preview(tmp*d1+(g_Line->m_TrolleyAlt+g_Line->m_Height)*u1);
		}
	}else if(g_Pole){
		g_Pole->Preview(V3ZERO);
	}
}

/*
 *	ロード
 */
bool CLinePlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "LineInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "TrolleyAlt", &m_TrolleyAlt))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Height", &m_Height))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "MaxInterval", &m_MaxInterval))) throw CSynErr(eee);
		if(tmp = AsgnFloat(str, "Offset", &m_Offset)) str = tmp;
		else m_Offset = 0.0f;
		if(!(str = AsgnFloat(eee = str, "MaxDeflection", &m_MaxDeflection))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = ReadProfile(eee = str))) throw CSynErr(eee);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	LoadData();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CLinePlugin::SetPreview(){
	ms_PreviewState = true;
	g_Line = this;
	string desc = g_Line->GetBasicInfo();
	desc += "\n"+g_Line->GetDescription();
	g_LineSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	架線柱距離加算
 */
void CLinePlugin::AddPolePos(
	float len	//	距離
){
	m_PolePos -= len;
	while(m_PolePos<=0.0f) m_PolePos += m_MaxInterval;
}
