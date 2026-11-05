#include "stdafx.h"
#include "CModelPlugin.h"
#include "CStation.h"
#include "CRailWay.h"

//	外部グローバル
extern CScene *g_Scene;

//	内部グローバル
CHeadlight g_TempHeadlight;
CParticle g_TempParticle;
CSoundEffector g_TempSoundEffector;

/*
 *	コンストラクタ
 */
CEffectorContainer::CEffectorContainer(){
	m_Effector = NULL;
}

/*
 *	コピーコンストラクタ
 */
CEffectorContainer::CEffectorContainer(
	const CEffectorContainer &src	//	コピー元
){
	m_Effector = src.m_Effector->Duplicate();
}

/*
 *	デストラクタ
 */
CEffectorContainer::~CEffectorContainer(){
	DELETE_V(m_Effector);
}

/*
 *	読込
 */
char *CEffectorContainer::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	bool is_station = !strcmp(mpi->DirName(), "Station");
	DELETE_V(m_Effector);
	CHeadlightApplier headlightapplier;
	CParticleApplier particleapplier;
	CSoundApplier soundapplier;
	CEffectorRailConnector effectorrailconnector;
	CEffectorRailBrancher effectorrailbrancher;
	CEffectorRailDisconnector effectorraildisconnector;
	CEffectorSwitchApplier effectorswitchapplier;
	if(tmp = headlightapplier.Read(str, mpi)){
		str = tmp;
		m_Effector = new CHeadlightApplier(headlightapplier);
	}else if(tmp = particleapplier.Read(str, mpi)){
		str = tmp;
		m_Effector = new CParticleApplier(particleapplier);
	}else if(tmp = soundapplier.Read(str, mpi)){
		str = tmp;
		m_Effector = new CSoundApplier(soundapplier);
	}else if(is_station && (tmp = effectorrailconnector.Read(str, mpi))){
		str = tmp;
		m_Effector = new CEffectorRailConnector(effectorrailconnector);
	}else if(is_station && (tmp = effectorrailbrancher.Read(str, mpi))){
		str = tmp;
		m_Effector = new CEffectorRailBrancher(effectorrailbrancher);
	}else if(is_station && (tmp = effectorraildisconnector.Read(str, mpi))){
		str = tmp;
		m_Effector = new CEffectorRailDisconnector(effectorraildisconnector);
	}else if(tmp = effectorswitchapplier.Read(str, mpi)){
		str = tmp;
		m_Effector = new CEffectorSwitchApplier(effectorswitchapplier);
	}else{
		str = NULL;
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CHeadlightApplier::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	if(!(str = g_TempHeadlight.Read(str, mpi))) return NULL;
	m_Headlight = mpi->AddHeadlight(g_TempHeadlight);
	return str;
}

/*
 *	データ読込
 */
void CHeadlightApplier::LoadDataEffector(
	CModelPlugin *mpi	//	呼び出し元
){
	m_Headlight->LoadData();
}

/*
 *	変更子適用
 */
void CHeadlightApplier::ApplyEffector(
	CScene *scene	//	シーン
){
	if(!g_PreSimulationFlag && (!scene || scene==g_Scene)) m_Headlight->Register();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CParticleApplier::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	if(!(str = g_TempParticle.Read(str, mpi))) return NULL;
	m_Particle = mpi->AddParticle(g_TempParticle);
	return str;
}

/*
 *	データ読込
 */
void CParticleApplier::LoadDataEffector(
	CModelPlugin *mpi	//	呼び出し元
){
	m_Particle->LoadData();
}

/*
 *	変更子適用
 */
void CParticleApplier::ApplyEffector(
	CScene *scene	//	シーン
){
	if(g_PreSimulationFlag) m_Particle->Register(scene);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CSoundApplier::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	if(!(str = g_TempSoundEffector.Read(str, mpi))) return NULL;
	m_SoundEffector = mpi->AddSoundEffector(g_TempSoundEffector);
	return str;
}

/*
 *	変更子適用
 */
void CSoundApplier::ApplyEffector(
	CScene *scene	//	シーン
){
	if(!g_PreSimulationFlag) m_SoundEffector->Register(scene);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CEffectorRailConnectorBase::CalculatePlatformID(){
	if(m_RailEnd1>=0){
		m_PlatformID1 = m_RailEnd1/2;
		m_PlatformEnd1 = m_RailEnd1%2;
	}
	if(m_RailEnd2>=0){
		m_PlatformID2 = m_RailEnd2/2;
		m_PlatformEnd2 = m_RailEnd2%2;
	}
}

/*
 *	変更子適用
 */
void CEffectorRailConnectorBase::ApplyEffector(
	CScene *scene	//	シーン
){
	if(m_RailEnd1<0 || m_RailEnd2<0) return;
	if(g_PreSimulationFlag){
		CStation *station = (CStation *)CModelInst::GetCurrentInst();
		int pn = station->GetPlatformCount();
		if(m_PlatformID1>=pn || m_PlatformID2>=pn) return;
		CPlatformInst *pf1 = station->GetPlatformInst(m_PlatformID1);
		CPlatformInst *pf2 = station->GetPlatformInst(m_PlatformID2);
		CRailWay *rail1 = m_PlatformEnd1 ? pf1->GetRailWayBack() : pf1->GetRailWayFront();
		CRailWay *rail2 = m_PlatformEnd2 ? pf2->GetRailWayBack() : pf2->GetRailWayFront();
		if(!rail1 || !rail2) return;//ErrorDialog("INTERNAL ERROR: RAIL CONNECTOR FAILED.");
		ApplyEffectorRailConnectorBase(scene, rail1, rail2);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CEffectorRailDisconnectorBase::CalculatePlatformID(){
	if(m_RailEnd>=0){
		m_PlatformID = m_RailEnd/2;
		m_PlatformEnd = m_RailEnd%2;
	}
}

/*
 *	変更子適用
 */
void CEffectorRailDisconnectorBase::ApplyEffector(
	CScene *scene	//	シーン
){
	if(m_RailEnd<0) return;
	if(g_PreSimulationFlag){
		CStation *station = (CStation *)CModelInst::GetCurrentInst();
		int pn = station->GetPlatformCount();
		if(m_PlatformID>=pn) return;
		CPlatformInst *pf = station->GetPlatformInst(m_PlatformID);
		CRailWay *rail = m_PlatformEnd ? pf->GetRailWayBack() : pf->GetRailWayFront();
		if(!rail) return;//ErrorDialog("INTERNAL ERROR: RAIL DISCONNECTOR FAILED.");
		ApplyEffectorRailDisconnectorBase(scene, rail);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEffectorRailConnector::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *eee;
	if(!(str = Assignment(str, "ConnectRail"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_RailEnd1))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_RailEnd2))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	CalculatePlatformID();
	return str;
}

/*
 *	変更子適用
 */
void CEffectorRailConnector::ApplyEffectorRailConnectorBase(
	CScene *scene,		//	シーン
	CRailWay *rail1,	//	レール1
	CRailWay *rail2		//	レール2
){
	rail1->ConnectRailWay(m_PlatformEnd1, rail2, m_PlatformEnd2, scene);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEffectorRailBrancher::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *eee;
	if(!(str = Assignment(str, "BranchRail"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_RailEnd1))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_RailEnd2))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	CalculatePlatformID();
	return str;
}

/*
 *	変更子適用
 */
void CEffectorRailBrancher::ApplyEffectorRailConnectorBase(
	CScene *scene,		//	シーン
	CRailWay *rail1,	//	レール1
	CRailWay *rail2		//	レール2
){
	rail1->BranchRailWay(m_PlatformEnd1, rail2, m_PlatformEnd2, scene);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEffectorRailDisconnector::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *eee;
	if(!(str = Assignment(str, "DisconnectRail"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_RailEnd))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	CalculatePlatformID();
	return str;
}

/*
 *	変更子適用
 */
void CEffectorRailDisconnector::ApplyEffectorRailDisconnectorBase(
	CScene *scene,	//	シーン
	CRailWay *rail	//	レール
){
	rail->DisconnectRailWay(m_PlatformEnd, scene);
}
