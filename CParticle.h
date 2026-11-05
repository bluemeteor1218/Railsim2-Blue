#ifndef CPARTICLE_H_INCLUDED
#define CPARTICLE_H_INCLUDED

class CNamedObject;
class CModelPlugin;
class CParticle;
class CParticleState;
class CScene;

/*
 *	パーティクルインスタンス
 */
class CParticleInst{
private:
	float m_CameraDist;		//	カメラからの距離
	CParticle *m_Emitter;	//	エミッタ
	VEC3 m_Pos;				//	座標
	VEC3 m_Dir;				//	速度
	float m_InitRadius;		//	初期半径
	float m_FinRadius;		//	最終半径
	float m_Alpha;			//	アルファ
	float m_Angle;			//	角度
	D3DCOLOR m_Color;		//	色
	int m_Lifetime;			//	寿命
	int m_Timer;			//	経過時間
	CScene *m_Scene;		//	シーン
public:
	CParticleInst(CParticle *, VEC3, VEC3, float, float, int, CScene *);
	void CalcDist(){ m_CameraDist = V3Len(&(GetVPos()-m_Pos)); }
	bool operator<(const CParticleInst &rhs){ return m_CameraDist>rhs.m_CameraDist; }
	void Render();
	bool Simulate();
};

//	反復子
typedef list<CParticleInst>::iterator IParticleInst;

/*
 *	パーティクルエミッタ状態
 */
class CParticleState{
	friend class CParticle;
private:
	bool m_ApplyFlag;		//	適用フラグ
	float m_EmissionCredit;	//	放出残り個数
	float m_OldSpeed;		//	前回速度
	VEC3 m_OldPos;			//	前回基準座標
public:
	CParticleState(){ Reset(); }
	void Reset(){ m_EmissionCredit = -1.0f; }
	void Confirm(){ if(!m_ApplyFlag) m_EmissionCredit = -1.0f; }
	char *Read(char *);
	void Save(FILE *, char *);
};

//	反復子
typedef list<CParticleState>::iterator IParticleState;

/*
 *	パーティクルエミッタ
 */
class CParticle{
	friend class CParticleInst;
private:
	static list<CParticleInst> ms_RenderList;	//	レンダリングリスト
	string m_TextureFileName;		//	テクスチャファイル名
	LPTEX8 m_Texture;				//	テクスチャ
	CNamedObject *m_Link;			//	接続先オブジェクト
	float m_MinQty, m_MaxQty;		//	最小・最大放射量
	float m_VelocityRel;			//	速度比例成分
	float m_AccelerationRel;		//	加速度比例成分
	float m_DecelerationRel;		//	減速度比例成分
	float m_EmissionQuantity[3];	//	1 秒当たり放射量
	float m_Lifetime[2];			//	寿命 [秒]
	VEC3 m_SourceCoord;				//	位置
	VEC3 m_Direction[2];			//	方向
	float m_InitialRadius[2];		//	初期半径
	float m_FinalRadius[2];			//	最終半径
	D3DCOLOR m_Color[2];			//	色
	int m_BlendMode;				//	ブレンドモード
	float m_AirResistance;			//	空気抵抗
	float m_Gravity;				//	重力
	float m_Turbulence;				//	乱気流
	CParticleState *m_LinkState;	//	状態変数
public:
	static void InitRenderList();
	static void RenderAll();
	static void SimulateAll();
	char *Read(char *, CModelPlugin *);
	void LoadData();
	void Link(CParticleState *);
	void Register(CScene *);
};

//	反復子
typedef list<CParticle>::iterator IParticle;

//	外部グローバル
extern VEC3 g_WindDir;
extern VEC3 g_WindDirNorm;

#endif
