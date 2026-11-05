#ifndef CMODELINST_H_INCLUDED
#define CMODELINST_H_INCLUDED

#include "CModelPlugin.h"
#include "CPartsInst.h"

class CTrainGroup;
class CCursorSceneryMode;

/*
 *	検出情報
 */
class CDetectInfo{
private:
	CNamedObject *m_Object;
	CModelInst *m_ModelInst;
	CPartsInst *m_PartsInst;
public:
	CDetectInfo(){
		m_Object = NULL; m_ModelInst = NULL; m_PartsInst = NULL;
	}
	CDetectInfo(CNamedObject *no, CModelInst *mi, CPartsInst *pi){
		m_Object = no; m_ModelInst = mi; m_PartsInst = pi;
	}
	CNamedObject *GetObject(){ return m_Object; }
	CModelInst *GetModelInst(){ return m_ModelInst; }
	CPartsInst *GetPartsInst(){ return m_PartsInst; }
};

/*
 *	モデルインスタンス
 */
class CModelInst{
	friend class CModelPlugin;
protected:
	static int ms_DetectMode;				//	検出モード
	static int ms_DetectTemp;
	static float ms_MinDist;				//	最小検出距離
	static VEC3 ms_DetectRect1;				//	領域始点
	static VEC3 ms_DetectRect2;				//	領域終点
	static BOX8 ms_FocusBox;				//	フォーカスボックス
	static CModelInst *ms_CurrentInst;		//	対象インスタンス
	static CDetectInfo ms_DetectInfo;		//	検出情報
	vector<int> m_StaticSwitchID;			//	同期用スイッチ ID リスト
	vector<int> m_SwitchOption;				//	スイッチ設定値
	int m_Selected;							//	選択フラグ
	VEC3 m_Pos;								//	中心座標
	VEC3 m_Right, m_Up, m_Dir;				//	姿勢
	list<CPartsInst> m_PartsInst;			//	パーツインスタンス
	list<CTexAnimState> m_AnimationState;	//	アニメーション状態
	list<CMoverState> m_MoverState;			//	ムーバー状態
	list<CParticleState> m_ParticleState;	//	パーティクルエミッタ状態
	list<CSoundState> m_SoundState;			//	サウンド状態
	CModelPlugin *m_ModelPlugin;			//	モデルプラグイン
public:
	static void ResetDetect(int, VEC3, VEC3);
	static void CheckDetect(CNamedObject *, CPartsInst *);
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static float GetMinDist(){ return ms_MinDist; }
	static CDetectInfo GetDetectInfo(){ return ms_DetectInfo; }
	static CModelInst *GetCurrentInst(){ return ms_CurrentInst; }
	CModelInst();
	CModelInst(CModelPlugin *);
	virtual ~CModelInst();
	virtual bool IsSelectVisible(){ return false; }
	CModelPlugin *GetModelPlugin(){ return m_ModelPlugin; }
	void AttachPlugin(CModelPlugin *);
	void SetSwitchOption(int, int);
	virtual CTrainGroup *GetTrainGroup(){ return NULL; }
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int sel){ m_Selected |= sel; }
	void SetSelectFlag(int sel){ m_Selected = sel; }
	VEC3 GetPos(){ return m_Pos; }
	VEC3 GetRight(){ return m_Right; }
	VEC3 GetUp(){ return m_Up; }
	VEC3 GetDir(){ return m_Dir; }
	IPartsInst BeginParts(){ return m_PartsInst.begin(); }
	virtual CPartsInst *FindParts(CNamedObject *){ return NULL; }
	void StopSound();
	void SetLocalAxis();
	void FixPosture(VEC3, VEC3, VEC3);
	void ResetState();
	void Simulate();
	virtual bool IsWarping(){ return false; }
	virtual void SimulateModelInst() = 0;
	virtual void PrintInfo() = 0;
	virtual CModelInst *Control() = 0;
	virtual CScene *GetScene() = 0;
	char *ReadModelInst(char *, bool);
	void SaveModelInst(FILE *, char *, bool);
};

#define CMODELINST_CASTFUNC(type) \
	type *Next(){ return m_Next; } \
	type **NextAdr(){ return &m_Next; }

#endif
