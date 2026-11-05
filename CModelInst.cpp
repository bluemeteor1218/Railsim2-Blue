#include "stdafx.h"
#include "CCamera.h"
#include "CTrainGroup.h"
#include "CModelInst.h"
#include "CModelPlugin.h"
#include "CNeutralMode.h"

//	static メンバ
int CModelInst::ms_DetectMode;
int CModelInst::ms_DetectTemp;
float CModelInst::ms_MinDist;
VEC3 CModelInst::ms_DetectRect1;
VEC3 CModelInst::ms_DetectRect2;
CModelInst *CModelInst::ms_CurrentInst;
CDetectInfo CModelInst::ms_DetectInfo;

void ClearStaticSwitchTable(){
	g_StaticSwitchTable.clear();
}

void SetStaticSwitchOption(int switch_id, int switch_opt){
	if(switch_id<0 || g_StaticSwitchTable.size()<=switch_id) ErrorDialog("SetStaticSwitchOption switch_id error.");
	CStaticSwitchID &sw = g_StaticSwitchTable[switch_id];
	sw.inst->SetSwitchOption(sw.index, switch_opt);
	sw.net_sync = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	[static]
 *	オブジェクト検出リセット
 */
void CModelInst::ResetDetect(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	if(mode==2 || mode==3) V3MinMax(rect1, rect2, &rect1, &rect2);
	ms_DetectMode = mode;
	ms_DetectRect1 = rect1;
	ms_DetectRect2 = rect2;
	ms_MinDist = -1.0f;
}

/*
 *	[static]
 *	オブジェクト検出チェック
 */
void CModelInst::CheckDetect(
	CNamedObject *nobj,	//	オブジェクト
	CPartsInst *parts	//	パーツインスタンス
){
	CObject *obj = parts ? parts->GetObject() : nobj->GetObject();
	VEC3 center = obj->GetCenter();
	VEC3 sc = WorldToScreen(center);
	if(!obj->IsMeshValid() || sc.z<0.0f) return;
	float dist = V3Len(&(ms_DetectRect1-sc));
	float th = 500.0f*obj->GetRadius()/(V3Len(&(GetVPos()-center))*g_FovRatio);
	if(th<DETECT_2D_MIN) th = DETECT_2D_MIN;
	switch(ms_DetectMode){
	case 0:
	case 4:
		if(dist<th && (ms_MinDist<0.0f || ms_MinDist>dist)){
			ms_DetectTemp = 1;
			ms_MinDist = dist;
			ms_DetectInfo = CDetectInfo(nobj, ms_CurrentInst, parts);
		}
		break;
	case 2:
	case 3:
		if(ms_DetectRect1.x<=sc.x && sc.x<=ms_DetectRect2.x
			&& ms_DetectRect1.y<=sc.y && sc.y<=ms_DetectRect2.y) ms_DetectTemp = 1;
		break;
	}
}

/*
 *	コンストラクタ (読込用)
 */
CModelInst::CModelInst(){
	m_ModelPlugin = NULL;
	m_Selected = 0;
	m_Pos = V3ZERO; m_Right = V3RIGHT; m_Up = V3UP; m_Dir = V3DIR;
}

/*
 *	コンストラクタ
 */
CModelInst::CModelInst(
	CModelPlugin *mpi	//	モデルプラグイン
){
	AttachPlugin(mpi);
	m_Selected = 0;
	m_Pos = V3ZERO; m_Right = V3RIGHT; m_Up = V3UP; m_Dir = V3DIR;
}

/*
 *	デストラクタ
 */
CModelInst::~CModelInst(){
	if(g_NeutralMode) g_NeutralMode->DeleteModelInst(this);
}

/*
 *	モデルプラグインのアタッチ
 */
void CModelInst::AttachPlugin(
	CModelPlugin *mpi	//	モデルプラグイン
){
	m_ModelPlugin = mpi;
	m_PartsInst.clear();
	m_AnimationState.clear();
	m_MoverState.clear();
	m_ParticleState.clear();
	m_SoundState.clear();
	if(m_ModelPlugin){
		m_ModelPlugin->CopySwitch(m_SwitchOption);
		m_PartsInst.insert(
			m_PartsInst.begin(), m_ModelPlugin->GetPartsNum(), CPartsInst());
		m_AnimationState.insert(
			m_AnimationState.begin(), m_ModelPlugin->GetAnimationNum(), CTexAnimState());
		m_MoverState.insert(
			m_MoverState.begin(), m_ModelPlugin->GetMoverNum(), CMoverState());
		m_ParticleState.insert(
			m_ParticleState.begin(), m_ModelPlugin->GetParticleNum(), CParticleState());
		m_SoundState.insert(
			m_SoundState.begin(), m_ModelPlugin->GetSoundNum(), CSoundState());
		m_ModelPlugin->LoadSoundWave(this);
	}
}

/*
 *	スイッチ値設定
 */
void CModelInst::SetSwitchOption(int index, int opt){
	m_SwitchOption[index] = opt;
	CModelSwitch *sw = m_ModelPlugin->GetModelSwitch(index);
	if(sw && sw->IsGroupCommon()){
		CTrainGroup *group = GetTrainGroup();
		if(group) group->SetGroupCommonSwitch(sw, opt);
	}
}

/*
 *	音停止
 */
void CModelInst::StopSound(){
	ISoundState iss = m_SoundState.begin();
	for(; iss!=m_SoundState.end(); iss++) iss->Confirm(NULL, false);
}

/*
 *	ローカル系をセット
 */
void CModelInst::SetLocalAxis(){
	g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(m_Pos, m_Dir, m_Up);
}

/*
 *	姿勢設定
 */
void CModelInst::FixPosture(
	VEC3 pos,	//	位置
	VEC3 dir,	//	dir
	VEC3 up		//	up
){
	m_Pos = pos;
	m_Dir = dir;
	m_Up = up;
	V3NormAxis(&m_Right, &m_Up, &m_Dir);
}

/*
 *	状態リセット
 */
void CModelInst::ResetState(){
	ITexAnimState itas = m_AnimationState.begin();
	for(; itas!=m_AnimationState.end(); itas++) itas->Reset();
	IMoverState ims = m_MoverState.begin();
	for(; ims!=m_MoverState.end(); ims++) ims->Reset();
	IParticleState ips = m_ParticleState.begin();
	for(; ips!=m_ParticleState.end(); ips++) ips->Reset();
	ISoundState iss = m_SoundState.begin();
	for(; iss!=m_SoundState.end(); iss++) iss->Reset();
}

/*
 *	シミュレーション進行
 */
void CModelInst::Simulate(){
	ms_CurrentInst = this;
	ITexAnimState itas = m_AnimationState.begin();
	for(; itas!=m_AnimationState.end(); itas++) itas->Step();
	SimulateModelInst();
}

/*
 *	読込
 */
char *CModelInst::ReadModelInst(
	char *str,	//	対象文字列
	bool pst	//	姿勢情報読込
){
	char *eee, *tmp;
	int snum;
	if(!(str = AsgnInteger(eee = str, "SwitchNum", &snum))) throw CSynErr(eee);
	if(snum){
		if(!(str = Assignment(eee = str, "SwitchOption"))) throw CSynErr(eee);
		int sidx = 0;
		do{
			if(m_SwitchOption.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
			int opt;
			if(!(str = ConstInteger(eee = str, &opt))) throw CSynErr(eee);
			m_SwitchOption.push_back(opt);
			m_StaticSwitchID.push_back(g_StaticSwitchTable.size());
			g_StaticSwitchTable.push_back(CStaticSwitchID(this, sidx));
			sidx++;
		} while(!(tmp = Character2(str, ';')));
		if(sidx!=snum) throw CSynErr(eee);
		str = tmp;
	}
	if(pst){
		if(!(str = AsgnVector3D(eee = str, "Pos", &m_Pos))) throw CSynErr(eee);
		if(!(str = AsgnVector3D(eee = str, "Dir", &m_Dir))) throw CSynErr(eee);
		if(!(str = AsgnVector3D(eee = str, "Up", &m_Up))) throw CSynErr(eee);
	}
	if(m_ModelPlugin) m_PartsInst.insert(
		m_PartsInst.begin(), m_ModelPlugin->GetPartsNum(), CPartsInst());
	CTexAnimState anim;
	while(tmp = anim.Read(eee = str)){
		str = tmp;
		m_AnimationState.push_back(anim);
	}
	CMoverState mover;
	while(tmp = mover.Read(eee = str)){
		str = tmp;
		m_MoverState.push_back(mover);
	}
	CParticleState ptcl;
	while(tmp = ptcl.Read(eee = str)){
		str = tmp;
		m_ParticleState.push_back(ptcl);
	}
	if(m_ModelPlugin) m_ModelPlugin->LoadSoundWave(this);
	return str;
}

/*
 *	保存
 */
void CModelInst::SaveModelInst(
	FILE *df,	//	ファイル
	char *ind,	//	インデント
	bool pst	//	姿勢情報保存
){
	int i;
	fprintf(df, "%sSwitchNum = %d;\n", ind, m_SwitchOption.size());
	if(m_SwitchOption.size()){
		fprintf(df, "%sSwitchOption = ", ind);
		for(i = 0; i<m_SwitchOption.size(); i++)
			fprintf(df, i ? ", %d" : "%d", m_SwitchOption[i]);
		fprintf(df, ";\n");
	}
	if(pst){
		fprintf(df, "%sPos = ", ind); V3Save(df, m_Pos, ";\n");
		fprintf(df, "%sDir = ", ind); V3Save(df, m_Dir, ";\n");
		fprintf(df, "%sUp = ", ind); V3Save(df, m_Up, ";\n");
	}
	ITexAnimState ias = m_AnimationState.begin();
	for(; ias!=m_AnimationState.end(); ias++) ias->Save(df, ind);
	IMoverState ims = m_MoverState.begin();
	for(; ims!=m_MoverState.end(); ims++) ims->Save(df, ind);
	IParticleState ips = m_ParticleState.begin();
	for(; ips!=m_ParticleState.end(); ips++) ips->Save(df, ind);
}
