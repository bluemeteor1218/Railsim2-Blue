#include "stdafx.h"
#include "CModelPlugin.h"
#include "CConfigMode.h"

/*
 *	デストラクタ
 */
CSoundState::~CSoundState(){
	CSoundEffector::DeleteSound(this);
}

/*
 *	保存
 */
void CSoundState::Confirm(
	CSoundEffector *eff,	//	エフェクタ
	bool enabled			//	再生有効フラグ
){
	if(m_ApplyFlag && enabled){
		eff->SetPos(this);
		if(!m_State){
			m_State = 1;
			eff->PlayWave(this);
		}
	}else{
		if(m_State){
			m_State = 0;
			m_Wave.Stop();
			CSoundEffector::DeleteSound(this);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	内部グローバル
set<CSoundState *> CSoundEffector::ms_PlayList;

/*
 *	[static]
 *	リスト初期化
 */
void CSoundEffector::InitPlayList(){
	ISPSoundState isps = ms_PlayList.begin();
	for(; isps!=ms_PlayList.end(); isps++){
		(*isps)->m_State = 0;
		(*isps)->m_Wave.Stop();
	}
	ms_PlayList.clear();
}

/*
 *	読込
 */
char *CSoundEffector::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	車輌プラグイン
){
	char *tmp, *eee;
	string obj;
	if(!(str = BeginBlock(str, "SoundEffect"))) return NULL;
	if(!(str = AsgnString(eee = str, "WaveFileName", &m_WaveFileName))) throw CSynErr(eee);
	if(!(str = AsgnString(eee = str, "AttachObject", &obj))) throw CSynErr(eee);
	if(!(m_Link = mpi->FindObject(obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(!(str = AsgnVector3D(eee = str, "SourceCoord", &m_SourceCoord))) throw CSynErr(eee);
	if(tmp = AsgnInteger(eee = str, "Volume", &m_Volume)) str = tmp;
	else m_Volume = 0;
	if(tmp = AsgnYesNo(eee = str, "Loop", &m_Loop)) str = tmp;
	else m_Loop = true;
	ValueArea(&m_Volume, DSBVOLUME_MIN, DSBVOLUME_MAX);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	m_LinkState = NULL;
	return str;
}

/*
 *	データ読込
 */
void CSoundEffector::LoadData(
	CSoundState *ss	//	ステート
){
	ss->m_Wave.Load((char *)m_WaveFileName.c_str());
}

/*
 *	定位設定
 */
void CSoundEffector::SetPos(
	CSoundState *ss	//	ステート
){
	CObject *obj = m_Link->GetObject();
	VEC3 oright = obj->GetRight(), oup = obj->GetUp(), odir = obj->GetDir();
	V3Norm(&oright, &oright);
	V3Norm(&oup, &oup);
	V3Norm(&odir, &odir);
	ss->m_Wave.SetPos(obj->GetPos()+V3LocalToWorld(&m_SourceCoord, &oright, &oup, &odir));
}

/*
 *	バッファ再生
 */
void CSoundEffector::PlayWave(
	CSoundState *ss	//	ステート
){
	ss->m_Wave.SetVolume(m_Volume);
	ss->m_Wave.Play(m_Loop ? -1 : 0);
	ms_PlayList.insert(ss);
}

/*
 *	状態変数をアタッチ
 */
void CSoundEffector::Link(
	CSoundState *stt	//	状態変数
){
	m_LinkState = stt;
	m_LinkState->m_ApplyFlag = false;
}

/*
 *	レンダリングリストに登録
 */
void CSoundEffector::Register(
	CScene *scene	//	シーン
){
	if(!m_LinkState || !scene) return;
	m_LinkState->m_ApplyFlag = true;
}
