#include "stdafx.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CRailSelectMode.h"
#include "CRailBuildMode.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"

//	内部定数
extern const float RAIL_PREV_LEN = 100.0f;	//	レールプレビュー長
const int WHEEL_SOUND_MARGIN = 100;			//	車輪音空白時間 [ms]

//	外部グローバル
extern float g_FrameDelta;
extern bool g_EnableCant;

/*
 *	[static]
 *	プレビュー
 */
void CRailPlugin::RenderPreview(){
	if(!ms_PreviewState || !(g_Rail || g_Tie || g_Girder)) return;
	VEC3 op1(0.0f, 0.0f, -RAIL_PREV_LEN), op2(0.0f, 0.0f, RAIL_PREV_LEN);
	int i, j, split = 8;
	bool multi = CRailwayMode::IsMultiTrack();
	int tnum = multi ? CRailwayMode::GetTrackNum() : 1;
	float tint = multi ? CRailwayMode::GetTrackInterval() : 0.0f;
	for(j = 0; j<=tnum; j++){
		float x = (j-(tnum-1)*0.5f)*tint;
		if(j==tnum){
			if(!g_Girder || !g_Girder->IsMultiTrack()) break;
			x = 0.0f;
		}
		if(g_Rail) g_Rail->ResetMapTemp();
		if(g_Tie) g_Tie->ResetMapTemp();
		if(g_Girder) g_Girder->ResetMapTemp();
		for(i = 0; i<split; i++){
			VEC3 op1 = VEC3(x, 0.0f, (i/split-0.5f)*2*RAIL_PREV_LEN);
			VEC3 op2 = VEC3(x, 0.0f, ((i+1)/split-0.5f)*2*RAIL_PREV_LEN);
			VEC3 p1 = op1, ip1 = p1, r1 = V3RIGHT, u1 = V3UP, d1 = V3DIR;
			VEC3 p2 = op2, ip2 = p2, r2 = V3RIGHT, u2 = V3UP, d2 = V3DIR;
			VEC3 gfix;
			float len = V3Len(&(p2-p1));
			bool g_flag = g_Girder && (g_Girder->IsMultiTrack() ? j==tnum : j<tnum);
			devResetMatrix();
			devResetMaterial();
			devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
			devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
			if(g_Rail){
				if(j<tnum){
					g_Rail->Dump(p1, r1, u1, ip1, r1, u1, p2, r2, u2, ip2, r2, u2, len, 7);
				}else{
					g_Rail->BeforeDump(p1, r1, u1, p2, r2, u2);
					g_Rail->AfterDump(p1, u1, ip1, u1, p2, u2, ip2, u2);
				}
			}
			if(g_Tie){
				if(j<tnum){
					g_Tie->Dump(p1, r1, u1, ip1, r1, u1, p2, r2, u2, ip2, r2, u2, len, 7);
				}else{
					g_Tie->BeforeDump(p1, r1, u1, p2, r2, u2);
					g_Tie->AfterDump(p1, u1, ip1, u1, p2, u2, ip2, u2);
				}
			}
			if(g_flag) g_Girder->Dump(
				p1, r1, u1, ip1, r1, u1, p2, r2, u2, ip2, r2, u2, len, 7);
			p1 = ip1 = op1; p2 = ip2 = op2;
			devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
			devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
			if(g_Rail){
				if(j<tnum){
					g_Rail->Render(
						p1, r1, u1, d1, ip1, r1, u1, p2, r2, u2, d2, ip2, r2, u2, 3, len);
				}else{
					g_Rail->BeforeDump(p1, r1, u1, p2, r2, u2);
					g_Rail->AfterDump(p1, u1, ip1, u1, p2, u2, ip2, u2);
				}
			}
			if(g_Tie){
				if(j<tnum){
					g_Tie->Render(
					p1, r1, u1, d1, ip1, r1, u1, p2, r2, u2, d2, ip2, r2, u2, 3, len);
				}else{
					g_Tie->BeforeDump(p1, r1, u1, p2, r2, u2);
					g_Tie->AfterDump(p1, u1, ip1, u1, p2, u2, ip2, u2);
				}
			}
			if(g_flag) g_Girder->Render(
				p1, r1, u1, d1, ip1, r1, u1, p2, r2, u2, d2, ip2, r2, u2, 3, len);
			devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
			devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
		}
	}
}

/*
 *	コンストラクタ
 */
CRailPlugin::CRailPlugin(
	char *id	//	プラグイン ID
):
	CProfilePlugin(id)	//	基本クラス
{
	m_WheelSound = NULL;
}

/*
 *	デストラクタ
 */
CRailPlugin::~CRailPlugin(){
	DELETE_V(m_WheelSound);
}

/*
 *	ロード
 */
bool CRailPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	string wheelsound;
	try{
		if(!(str = BeginBlock(eee = str, "RailInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Gauge", &m_Gauge))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Height", &m_Height))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "SurfaceAlt", &m_SurfaceAlt))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "CantRatio", &m_CantRatio))) throw CSynErr(eee);
		m_CantRatio = D3DXToRadian(m_CantRatio);
		if(!(str = AsgnFloat(eee = str, "MaxCant", &m_MaxCant))) throw CSynErr(eee);
		m_MaxCant = tanf(D3DXToRadian(m_MaxCant));
		if(tmp = AsgnYesNo(eee = str, "FlattenCant", &m_FlattenCant)) str = tmp;
		else m_FlattenCant = false;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "SoundInfo"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "WheelSoundFile", &wheelsound))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "JointInterval", &m_JointInterval))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = ReadProfile(eee = str))) throw CSynErr(eee);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	LoadData();
	if(wheelsound.size()){
		ChDir();
		m_WheelSound = new CWaveArray;
		m_WheelSound->Load((char *)wheelsound.c_str(), 10, true);
	}
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CRailPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Rail = this;
	string desc = g_Rail->GetBasicInfo();
	desc += FlashIn("\n%s: %.3f [m]", lang(Gauge), m_Gauge);
	desc += "\n"+g_Rail->GetDescription();
	g_RailSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	ダンプ前処理
 */
float CRailPlugin::CantFunc(
	float radius	//	曲率半径
){
	if(!g_EnableCant) return 0.0f;
	float tmp = m_CantRatio/radius;
	return m_MaxCant*(1.0f-expf(-tmp/m_MaxCant));
}

/*
 *	車輪音再生
 */
void CRailPlugin::PlayWheelSound(
	float dist1,	//	距離 1
	float dist2,	//	距離 2
	VEC3 &pos		//	位置
){
	if(!m_WheelSound || !g_ConfigMode->GetRailSound()
		|| g_SimulationMode->GetSimSpeed()>1) return;
	float f1 = dist1/m_JointInterval, f2 = dist2/m_JointInterval;
	int n1 = Round(f1), n2 = Round(f2);
	if(n1!=n2){
		float delay = (0.5f*(n1+n2)-f1)/(f2-f1);
		ValueArea(&delay, 0.0f, 1.0f);
		delay *= g_FrameDelta>WHEEL_SOUND_MARGIN ? WHEEL_SOUND_MARGIN : g_FrameDelta;
		m_WheelSound->Add(pos, 0, (int)(WHEEL_SOUND_MARGIN-delay));
	}
}

/*
 *	ダンプ前処理
 */
void CRailPlugin::BeforeDump(
	VEC3 &p1, VEC3 &r1, VEC3 &u1,	//	始点 (正規化済)
	VEC3 &p2, VEC3 &r2, VEC3 &u2	//	終点 (正規化済)
){
	float sft1 = 1.0f-sqrtf(r1.x*r1.x+r1.z*r1.z);
	float sft2 = 1.0f-sqrtf(r2.x*r2.x+r2.z*r2.z);
	if(r1.y>0.0f) sft1 = -sft1;
	if(r2.y>0.0f) sft2 = -sft2;
	VEC3 fr1(r1.x, 0.0f, r1.z), fr2(r2.x, 0.0f, r2.z);
	V3Norm(&fr1, &fr1);
	V3Norm(&fr2, &fr2);
	p1 += 0.5f*m_Gauge*(fabsf(r1.y)*V3UP+sft1*fr1);
	p2 += 0.5f*m_Gauge*(fabsf(r2.y)*V3UP+sft2*fr2);
}

/*
 *	ダンプ後処理
 */
void CRailPlugin::AfterDump(
	VEC3 &p1, VEC3 &u1,		//	始点 (正規化済)
	VEC3 &ip1, VEC3 &iu1,	//	非カント始点 (正規化済)
	VEC3 &p2, VEC3 &u2,		//	終点 (正規化済)
	VEC3 &ip2, VEC3 &iu2	//	非カント終点 (正規化済)
){
	ip1 -= m_Height*iu1;
	ip2 -= m_Height*iu2;
	if(m_FlattenCant){
		p1 = ip1; u1 = iu1;
		p2 = ip2; u2 = iu2;
	}else{
		p1 -= m_Height*u1;
		p2 -= m_Height*u2;
	}
}

/*
 *	橋脚設置位置計算
 */
void CRailPlugin::CalcPierPos(
	VEC3 *pos,		//	位置
	VEC3 *right,	//	right (正規化済)
	VEC3 *up,		//	up (正規化済)
	VEC3 *dir		//	dir (正規化済)
){
	if(m_FlattenCant){
		right->y = 0.0f;
		V3Norm(up, V3Cross(up, dir, V3Norm(right, right)));
	}
	*pos -= *up*m_Height;
}
