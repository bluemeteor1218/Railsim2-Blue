#include "stdafx.h"
#include "CListView.h"
#include "CModelInst.h"
#include "CTrainGroup.h"
#include "CEnvPlugin.h"
#include "CConfigMode.h"

//	外部グローバル
extern CHeadlight g_TempHeadlight;
extern CParticle g_TempParticle;
extern CSoundEffector g_TempSoundEffector;

/*
 *	コンストラクタ
 */
CModelPlugin::CModelPlugin(
	char *id	//	プラグイン ID
):
	CPlugin(id)	//	基本クラス
{
	m_SelectSwitchID = 0;
	m_PartsNum = 0;
}

/*
 *	スイッチ読込
 */
char *CModelPlugin::ReadModelSwitch(
	char *str	//	対象文字列
){
	char *tmp;
	while(true){
		CModelSwitch msw(m_ModelSwitch.size());
		CTextureAnimation anim;
		if(tmp = msw.Read(str, this)){
			str = tmp;
			m_ModelSwitch.push_back(msw);
		}else if(tmp = anim.Read(str, this)){
			str = tmp;
			m_Animation.push_back(anim);
		}else{
			break;
		}
	}
	if(m_ModelSwitch.size()){
		m_TempSwitch.resize(m_ModelSwitch.size());
		int i;
		for(i = 0; i<m_TempSwitch.size(); i++) m_TempSwitch[i] = 0;
	}
	return str;
}

/*
 *	エフェクト読込
 */
char *CModelPlugin::ReadEffect(
	char *str	//	対象文字列
){
	g_TempHeadlight = CHeadlight();
	g_TempParticle = CParticle();
	char *tmp;
	if(tmp = m_Effector.Read(str, this, 1)) str = tmp;
	return str;
}

/*
 *	データ読み込み
 */
void CModelPlugin::LoadData(){
	ChDir();
	ITextureAnimation ia = m_Animation.begin();
	for(; ia!=m_Animation.end(); ia++) ia->LoadData();
	m_Effector.LoadData(this);
}

/*
 *	スイッチ検索
 */
CModelSwitch *CModelPlugin::FindModelSwitch(
	const string &name	//	スイッチ名
){
	CModelSwitch *ret;
	IModelSwitch is = m_ModelSwitch.begin();
	for(; is!=m_ModelSwitch.end(); is++) if(ret = is->Check(name)) return ret;
	return NULL;
}

/*
 *	スイッチ取得
 */
CModelSwitch *CModelPlugin::GetModelSwitch(
	int index	//	番号
){
	int i = 0;
	IModelSwitch is = m_ModelSwitch.begin();
	for(; is!=m_ModelSwitch.end(); is++) if(i++==index) return &*is;
	return NULL;
}

/*
 *	アニメーション検索
 */
CTextureAnimation *CModelPlugin::FindAnimation(
	const string &name	//	アニメーション名
){
	CTextureAnimation *ret;
	ITextureAnimation ia = m_Animation.begin();
	for(; ia!=m_Animation.end(); ia++) if(ret = ia->Check(name)) return ret;
	return NULL;
}

/*
 *	スイッチリスト作成
 */
void CModelPlugin::ListSwitch(
	CListView *slv,		//	スイッチリストビュー
	CListView *olv,		//	オプションリストビュー
	CModelInst *minst	//	リンクインスタンス
){
	slv->DeleteAllItems();
	olv->DeleteAllItems();
	m_LinkInst = minst;
	if(!m_ModelSwitch.size()) return;
	IModelSwitch im = m_ModelSwitch.begin();
	for(; im!=m_ModelSwitch.end(); im++){
		CListElement *sle = slv->InsertItem(-1, (char *)im->m_SwitchName.c_str());
		sle->SetString(1, im->GetCurrentOptionName());
		sle->SetData((DWORD)&*im);
		im->SetListElement(sle);
	}
	slv->SetSelectionMark(m_SelectSwitchID, 0);
	slv->EnsureVisible(m_SelectSwitchID);
	m_SelectSwitch = NULL;
}

/*
 *	スイッチ編集
 */
void CModelPlugin::EditSwitch(
	CListView *slv,	//	スイッチリストビュー
	CListView *olv	//	オプションリストビュー
){
	if(!m_ModelSwitch.size()) return;
	if(m_LinkInst){
		SetSwitch(m_LinkInst);
		CNamedObjectAfterRenderer::SetCurrentInst(NULL);
	}
	CListElement *sle = slv->GetFocusItem();
	if(sle && sle->IsSelected()){
		m_SelectSwitchID = slv->GetSelectionMark();
		CModelSwitch *sw = (CModelSwitch *)sle->GetData();
		if(sw!=m_SelectSwitch){
			m_SelectSwitch = sw;
			sw->ListEntry(olv);
		}
	}else if(m_SelectSwitch){
		m_SelectSwitch = NULL;
		olv->DeleteAllItems();
	}
	if(m_SelectSwitch){
		if(m_LinkInst){
			int *pval = &m_LinkInst->m_SwitchOption[m_SelectSwitch->m_ID], before = *pval;
			int switch_id = m_SelectSwitch->m_ID<m_LinkInst->m_StaticSwitchID.size()
				? m_LinkInst->m_StaticSwitchID[m_SelectSwitch->m_ID] : -1;
			m_SelectSwitch->EditSwitch(olv, pval, switch_id);
			if(m_SelectSwitch->IsGroupCommon()){
				CTrainGroup *group = m_LinkInst->GetTrainGroup();
				if(group && before!=*pval) group->SetGroupCommonSwitch(m_SelectSwitch, *pval);
			}
		}else{
			m_SelectSwitch->EditSwitch(olv, NULL, -1);
		}
	}
}

/*
 *	スイッチ設定セット
 */
bool CModelPlugin::SetSwitch(
	vector<int> &sopt	//	スイッチ設定値
){
	LoadAndGet();
	IModelSwitch im = m_ModelSwitch.begin();
	int i = 0;
	for(; im!=m_ModelSwitch.end() && i<sopt.size(); im++, i++) im->m_Value = sopt[i];
	for(; im!=m_ModelSwitch.end(); im++) im->m_Value = 0;
	return sopt.size()==m_ModelSwitch.size();
}

/*
 *	スイッチ設定セット
 */
void CModelPlugin::SetSwitch(
	CModelInst *minst	//	インスタンス
){
	LoadAndGet();
	CNamedObjectAfterRenderer::SetCurrentInst(minst);
	CNamedObjectAfterRenderer::SetCurrentPlugin(this);
	IModelSwitch im = m_ModelSwitch.begin();
	if(minst->m_SwitchOption.size()!=m_ModelSwitch.size()) minst->AttachPlugin(this);
	vector<int>::iterator ptr = minst->m_SwitchOption.begin();
	for(; im!=m_ModelSwitch.end(); im++, ptr++) im->m_Value = *ptr;
}

/*
 *	スイッチ設定コピー
 */
void CModelPlugin::CopySwitch(
	vector<int> &sopt	//	スイッチ設定値
){
	sopt.resize(m_ModelSwitch.size());
	vector<int>::iterator ptr = sopt.begin();
	IModelSwitch im = m_ModelSwitch.begin();
	for(; im!=m_ModelSwitch.end(); im++, ptr++) *ptr = im->m_Value;
}

/*
 *	編成共通スイッチのチェック
 */
void CModelPlugin::CheckGroupCommonSwitch(
	CModelInst *minst,	//	インスタンス (変更側)
	CModelSwitch *sw,	//	対象スイッチ (固定側)
	int val				//	対象値 (固定側)
){
	vector<int>::iterator ptr = minst->m_SwitchOption.begin();
	IModelSwitch im = m_ModelSwitch.begin();
	for(; im!=m_ModelSwitch.end(); im++, ptr++) if(im->CheckGroupCommon(sw)) *ptr = val;
}

/*
 *	編成共通スイッチのチェック
 */
void CModelPlugin::CheckGroupCommonSwitchAll(
	CModelInst *minst1,	//	インスタンス (変更側)
	CModelInst *minst2	//	インスタンス (固定側)
){
	CModelPlugin *mpi = minst2->GetModelPlugin();
	vector<int>::iterator ptr = minst2->m_SwitchOption.begin();
	IModelSwitch im = mpi->m_ModelSwitch.begin();
	for(; im!=mpi->m_ModelSwitch.end(); im++, ptr++)
		if(im->IsGroupCommon()) CheckGroupCommonSwitch(minst1, &*im, *ptr);
}

/*
 *	パーツインスタンスセット
 */
void CModelPlugin::SetPartsInst(
	CModelInst *minst	//	インスタンス
){
	if(minst){
		if(minst->m_PartsInst.size()!=GetPartsNum()) minst->AttachPlugin(this);
		CNamedObject::SetPartsInst(&minst->m_PartsInst);
	}else{
		CNamedObject::SetPartsInst(NULL);
	}
}

/*
 *	アニメーション状態セット
 */
void CModelPlugin::SetAnimation(
	CModelInst *minst	//	インスタンス
){
	ITextureAnimation ia = m_Animation.begin();
	if(minst){
		if(minst->m_AnimationState.size()!=m_Animation.size()) minst->AttachPlugin(this);
		ITexAnimState itas = minst->m_AnimationState.begin();
		for(; ia!=m_Animation.end(); ia++, itas++) ia->SetState(&*itas);;
	}else{
		for(; ia!=m_Animation.end(); ia++) ia->SetState(NULL);
	}
}

/*
 *	ムーバー状態セット
 */
void CModelPlugin::SetMoverState(
	CModelInst *minst	//	インスタンス
){
	IPMoverState ipms = m_MoverState.begin();
	if(minst){
		if(minst->m_MoverState.size()!=m_MoverState.size()) minst->AttachPlugin(this);
		IMoverState ims = minst->m_MoverState.begin();
		for(; ipms!=m_MoverState.end(); ipms++, ims++) *ipms = &*ims;
	}else{
		for(; ipms!=m_MoverState.end(); ipms++) *ipms = NULL;
	}
}

/*
 *	サウンドのロード
 */
void CModelPlugin::LoadSoundWave(
	CModelInst *minst	//	インスタンス
){
	ChDir();
	if(minst){
		minst->m_SoundState.clear();
		minst->m_SoundState.insert(minst->m_SoundState.begin(), GetSoundNum(), CSoundState());
		ISoundEffector is = m_SoundEffector.begin();
		ISoundState iss = minst->m_SoundState.begin();
		for(; is!=m_SoundEffector.end(); is++, iss++) is->LoadData(&*iss);
	}
}

/*
 *	エフェクト描画
 */
void CModelPlugin::SimulateEffect(
	CModelInst *minst	//	インスタンス
){
	if(minst){
		if(minst->IsWarping()) return;
		if(g_PreSimulationFlag){
			if(minst->m_ParticleState.size()!=m_Particle.size()) minst->AttachPlugin(this);
			IParticle ip = m_Particle.begin();
			IParticleState ips = minst->m_ParticleState.begin();
			for(; ip!=m_Particle.end(); ip++, ips++) ip->Link(&*ips);
		}else{
			if(minst->m_SoundState.size()!=m_SoundEffector.size()) minst->AttachPlugin(this);
			ISoundEffector is = m_SoundEffector.begin();
			ISoundState iss = minst->m_SoundState.begin();
			for(; is!=m_SoundEffector.end(); is++, iss++) is->Link(&*iss);
		}
	}
	m_Effector.Apply(minst ? minst->GetScene() : NULL);
	if(minst){
		if(g_PreSimulationFlag){
			IParticleState ips = minst->m_ParticleState.begin();
			for(; ips!=minst->m_ParticleState.end(); ips++) ips->Confirm();
		}else{
			ISoundEffector is = m_SoundEffector.begin();
			ISoundState iss = minst->m_SoundState.begin();
			for(; is!=m_SoundEffector.end(); is++, iss++)
				iss->Confirm(&*is, IsSoundEnabled());
		}
	}
}
