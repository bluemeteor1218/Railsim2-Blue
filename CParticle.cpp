#include "stdafx.h"
#include "CModelPlugin.h"
#include "CEnvPlugin.h"
#include "CScene.h"
#include "CConfigMode.h"

//	内部グローバル
VEC3 g_WindDir = V3ZERO;	//	風方向
VEC3 g_WindDirNorm = V3DIR;	//	風方向 (正規化済)

/*
 *	コンストラクタ
 */
CParticleInst::CParticleInst(
	CParticle *emitter,	//	エミッタ
	VEC3 pos,			//	座標
	VEC3 dir,			//	速度
	float initradius,	//	半径
	float finradius,	//	アルファ
	int lifetime,		//	寿命
	CScene *scene		//	シーン
){
	m_Emitter = emitter;
	m_Pos = pos;
	m_Dir = dir;
	m_InitRadius = initradius;
	m_FinRadius = finradius;
	m_Alpha = 0.0f;
	m_Angle = FRand(2.0f*D3DX_PI);
	m_Color = MixColor(m_Emitter->m_Color[0], m_Emitter->m_Color[1], FRand(1.0f));
	m_Lifetime = lifetime;
	if(m_Lifetime<=0) m_Lifetime = 1;
	m_Timer = 0;
	m_Scene = scene;
}

/*
 *	レンダリング
 */
void CParticleInst::Render(){
	if(m_Timer>m_Lifetime || m_Scene!=g_Scene) return;
	devTransBillboard(m_Pos);
	devSetTexture(0, m_Emitter->m_Texture);
	float radius = 1.41421356f*(m_Alpha*m_InitRadius+(1.0f-m_Alpha)*m_FinRadius);
	float si = sinf(m_Angle)*radius, co = cosf(m_Angle)*radius;
	D3DCOLOR col = ScaleColor(m_Color, m_Alpha);
	if(m_Emitter->m_BlendMode){
		devBLEND_ADD2();
	}else{
		devBLEND_ALPHA();
		col = MultiplyColor(g_NoLightColor, col);
	}
	SetUVMap(0.0f, 0.0f, 1.0f, 1.0f);
	TexMap3DRect(
		VEC3(co, si, 0.0f), VEC3(si, -co, 0.0f),
		VEC3(-co, -si, 0.0f), VEC3(-si, co, 0.0f), col);
}

/*
 *	シミュレーション進行
 */
bool CParticleInst::Simulate(){
	m_Timer++;
	if(m_Timer>m_Lifetime) return false;
	VEC3 wind = g_ConfigMode->GetWind() ? g_WindDir : V3ZERO;
	m_Alpha = (float)(m_Lifetime-m_Timer)/m_Lifetime;
	m_Pos += m_Dir;
	m_Dir.y -= m_Emitter->m_Gravity;
	m_Dir = (1.0f-m_Emitter->m_AirResistance)*m_Dir
		+m_Emitter->m_AirResistance*(wind+V3RandS(m_Emitter->m_Turbulence));
	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CParticleState::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = Assignment(eee = str, "ParticleState"))) return NULL;
	if(!(str = ConstFloat(eee = str, &m_EmissionCredit))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = Vector3D(eee = str, &m_OldPos))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CParticleState::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sParticleState = %f, ", ind, m_EmissionCredit);
	V3Save(df, m_OldPos, ";\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	内部グローバル
list<CParticleInst> CParticle::ms_RenderList;

/*
 *	[static]
 *	リスト初期化
 */
void CParticle::InitRenderList(){
	ms_RenderList.clear();
}

/*
 *	[static]
 *	全てレンダリング
 */
void CParticle::RenderAll(){
	if(!g_ConfigMode->GetMiscParticle()) return;
	devResetMaterial();
	//devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	IParticleInst ipi;
	for(ipi = ms_RenderList.begin(); ipi!=ms_RenderList.end(); ipi++) ipi->CalcDist();
	ms_RenderList.sort();
	for(ipi = ms_RenderList.begin(); ipi!=ms_RenderList.end(); ipi++) ipi->Render();
	//devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
	devBLEND_ALPHA();
}

/*
 *	[static]
 *	全てシミュレート
 */
void CParticle::SimulateAll(){
	IParticleInst ipi = ms_RenderList.begin();
//	Dialog("particles %d", ms_RenderList.size());
	while(ipi!=ms_RenderList.end()){
		if(ipi->Simulate()) ipi++;
		else ipi = ms_RenderList.erase(ipi);
	}
}

/*
 *	読込
 */
char *CParticle::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	車輌プラグイン
){
	char *tmp, *eee;
	int i;
	string obj, blend;
	m_Texture = NULL;
	if(!(str = BeginBlock(str, "Particle"))) return NULL;
	if(!(str = AsgnString(eee = str, "TextureFileName", &m_TextureFileName))) throw CSynErr(eee);
	if(!(str = AsgnString(eee = str, "AttachObject", &obj))) throw CSynErr(eee);
	if(!(m_Link = mpi->FindObject(obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(!(str = AsgnVector3D(eee = str, "SourceCoord", &m_SourceCoord))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "MinQty", &m_MinQty))) throw CSynErr(eee);
	if(tmp = AsgnFloat(eee = str, "MaxQty", &m_MaxQty)) str = tmp;
	else m_MaxQty = m_MinQty;
	if(tmp = AsgnFloat(eee = str, "VelocityRel", &m_VelocityRel)) str = tmp;
	else m_VelocityRel = 0.0f;
	if(tmp = AsgnFloat(eee = str, "AccelerationRel", &m_AccelerationRel)) str = tmp;
	else m_AccelerationRel = 0.0f;
	if(tmp = AsgnFloat(eee = str, "DecelerationRel", &m_DecelerationRel)) str = tmp;
	else m_DecelerationRel = 0.0f;
	m_MinQty /= MAXFPS;
	m_MaxQty /= MAXFPS;
	m_VelocityRel *= 3.6f;
	m_AccelerationRel *= 3.6f*MAXFPS;
	m_DecelerationRel *= 3.6f*MAXFPS;
	if(!(str = AsgnFloat(eee = str, "Lifetime", m_Lifetime, 2, true))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Direction", m_Direction, 2, true))) throw CSynErr(eee);
	for(i = 0; i<2; i++) m_Direction[i] /= MAXFPS;
	if(!(str = AsgnFloat(eee = str, "InitialRadius", m_InitialRadius, 2, true))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "FinalRadius", m_FinalRadius, 2, true))) throw CSynErr(eee);
	if(!(str = AsgnColor(eee = str, "Color", m_Color, 2, true))) throw CSynErr(eee);
	if(!(str = AsgnIdentifier(eee = str, "BlendMode", &blend))) throw CSynErr(eee);
	if(blend=="Alpha") m_BlendMode = 0;
	else if(blend=="Add") m_BlendMode = 1;
	else throw CSynErr(eee, "%s: \"%s\"", lang(InvalidBlendMode), blend.c_str());
	if(tmp = AsgnFloat(eee = str, "AirResistance", &m_AirResistance)) str = tmp;
	else m_AirResistance = 0.0f;
	ValueArea(&m_AirResistance, 0.0f, 1.0f);
	m_AirResistance = 1.0f-powf(1.0f-m_AirResistance, 1.0f/MAXFPS);
	if(tmp = AsgnFloat(eee = str, "Gravity", &m_Gravity)) str = tmp;
	else m_Gravity = 0.0f;
	m_Gravity /= MAXFPS;
	if(tmp = AsgnFloat(eee = str, "Turbulence", &m_Turbulence)) str = tmp;
	else m_Turbulence = 0.0f;
	m_Turbulence /= MAXFPS;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	m_LinkState = NULL;
	return str;
}

/*
 *	データ読込
 */
void CParticle::LoadData(){
	m_Texture = g_TexList.Get(FALSE, m_TextureFileName.c_str(), 0, !g_NamedObjectMipMap);
}

/*
 *	状態変数をアタッチ
 */
void CParticle::Link(
	CParticleState *stt	//	状態変数
){
	m_LinkState = stt;
	m_LinkState->m_ApplyFlag = false;
}

/*
 *	レンダリングリストに登録
 */
void CParticle::Register(
	CScene *scene	//	シーン
){
	if(!m_LinkState || !scene) return;
	m_LinkState->m_ApplyFlag = true;
	CObject *obj = m_Link->GetObject();
	VEC3 oright = obj->GetRight(), oup = obj->GetUp(), odir = obj->GetDir();
	V3Norm(&oright, &oright);
	V3Norm(&oup, &oup);
	V3Norm(&odir, &odir);
	VEC3 pos = obj->GetPos()+V3LocalToWorld(&m_SourceCoord, &oright, &oup, &odir);
	if(m_LinkState->m_EmissionCredit<0.0f){
		m_LinkState->m_EmissionCredit = 0.0f;
		m_LinkState->m_EmissionCredit = 0.0f;
		m_LinkState->m_OldPos = pos;
		return;
	}
	VEC3 wind = g_ConfigMode->GetWind() ? g_WindDir : V3ZERO;
	VEC3 delta = pos-m_LinkState->m_OldPos;
	float speed = V3Len(&delta);
	if(m_LinkState->m_OldSpeed<0.0f) m_LinkState->m_OldSpeed = speed;
	float acc = speed-m_LinkState->m_OldSpeed;
	float qty = m_MinQty+m_VelocityRel*speed
		+(acc<0.0f ? -m_DecelerationRel : m_AccelerationRel)*acc;
	ValueArea(&qty, m_MinQty, m_MaxQty);
	m_LinkState->m_EmissionCredit += qty;
	int i = 0, credit = (int)m_LinkState->m_EmissionCredit;
	while(m_LinkState->m_EmissionCredit>=1.0f){
		m_LinkState->m_EmissionCredit -= 1.0f;
		VEC3 rad = V3Rand2(m_Direction[0], m_Direction[1]);
		VEC3 dir = V3LocalToWorld(&rad, &oright, &oup, &odir)+delta;
		float fix = (float)(i++)/credit, vfix = powf(1.0f-m_AirResistance, fix);
		ms_RenderList.push_back(CParticleInst(
			this, pos+fix*rad, vfix*dir+(1.0f-vfix)*wind,
			FRand2(m_InitialRadius[0], m_InitialRadius[1]),
			FRand2(m_FinalRadius[0], m_FinalRadius[1]),
			Round(FRand2(m_Lifetime[0], m_Lifetime[1])*MAXFPS), scene));
	}
	m_LinkState->m_OldPos = pos;
	m_LinkState->m_OldSpeed = speed;
}
