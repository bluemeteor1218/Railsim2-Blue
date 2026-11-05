#ifndef CMODELPLUGIN_H_INCLUDED
#define CMODELPLUGIN_H_INCLUDED

#include "CLensFlare.h"
#include "CParticle.h"
#include "CSoundEffector.h"
#include "CModelSwitch.h"
#include "CTextureAnimation.h"
#include "CNamedObject.h"
#include "CPlugin.h"

class CMoverState;

/*
 *	モデルプラグイン
 */
class CModelPlugin: public CPlugin{
protected:
	vector<int> m_TempSwitch;				//	一時スイッチバッファ
	int m_SelectSwitchID;					//	選択されているスイッチ番号
	int m_PartsNum;							//	パーツ数
	CModelSwitch *m_SelectSwitch;			//	選択されているスイッチ
	CModelInst *m_LinkInst;					//	リンクインスタンス
	list<CModelSwitch> m_ModelSwitch;		//	モデルスイッチ
	list<CTextureAnimation> m_Animation;	//	テクスチャアニメーション
	list<CMoverState *> m_MoverState;		//	ムーバー状態
	list<CHeadlight> m_Headlight;			//	ヘッドライト
	list<CParticle> m_Particle;				//	パーティクル
	list<CSoundEffector> m_SoundEffector;	//	サウンドエフェクタ
	CEffectorSwitchEntry m_Effector;		//	エフェクタ
public:
	CModelPlugin(char *);
	virtual ~CModelPlugin(){}
	virtual char *DirName() = 0;
	virtual char *TextName2() = 0;
	virtual bool Load() = 0;
	virtual void SetPreview() = 0;
	virtual CNamedObject *FindObject(const string &) = 0;
	virtual bool IsSoundEnabled() = 0;
	CMoverState **AddMover(){
		m_MoverState.push_back(NULL);
		return &*--m_MoverState.end();
	}
	char *ReadModelSwitch(char *);
	char *ReadEffect(char *);
	CHeadlight *AddHeadlight(const CHeadlight &headlight){
		m_Headlight.push_back(headlight);
		return &*m_Headlight.rbegin();
	}
	CParticle *AddParticle(const CParticle &particle){
		m_Particle.push_back(particle);
		return &*m_Particle.rbegin();
	}
	CSoundEffector *AddSoundEffector(const CSoundEffector &soundeffector){
		m_SoundEffector.push_back(soundeffector);
		return &*m_SoundEffector.rbegin();
	}
	void LoadData();
	CModelSwitch *FindModelSwitch(const string &);
	CModelSwitch *GetModelSwitch(int);
	CTextureAnimation *FindAnimation(const string &);
	void ListSwitch(CListView *, CListView *, CModelInst *);
	void EditSwitch(CListView *, CListView *);
	bool SetSwitch(vector<int> &);
	void SetSwitch(CModelInst *);
	void CopySwitch(vector<int> &);
	void SetTempSwitch(){ SetSwitch(m_TempSwitch); }
	void StoreTempSwitch(){ CopySwitch(m_TempSwitch); }
	void CheckGroupCommonSwitch(CModelInst *, CModelSwitch *, int);
	void CheckGroupCommonSwitchAll(CModelInst *, CModelInst *);
	void SetPartsInst(CModelInst *);
	void SetAnimation(CModelInst *);
	void SetMoverState(CModelInst *);
	void LoadSoundWave(CModelInst *);
	CModelInst *GetLinkInst(){ return m_LinkInst; }
	void FreeInst(){ m_LinkInst = NULL; }
	void AddPartsNum(int n){ m_PartsNum += n; }
	int GetPartsNum(){ return m_PartsNum; }
	int GetMoverNum(){ return m_MoverState.size(); }
	int GetAnimationNum(){ return m_Animation.size(); }
	int GetParticleNum(){ return m_Particle.size(); }
	int GetSoundNum(){ return m_SoundEffector.size(); }
	void SimulateEffect(CModelInst *);
	CPLUGIN_CASTFUNC(CModelPlugin);
};

/*
 *	モデルプラグインリスト
 */
class CModelPluginList: public CPluginList{
protected:
public:
	virtual char *DirName() = 0;
	virtual char *TextName2() = 0;
	virtual CPlugin *NewEntry(char *) = 0;
	CPLUGINLIST_CASTFUNC(CModelPlugin);
};

#endif
