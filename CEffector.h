#ifndef CEFFECTOR_H_INCLUDED
#define CEFFECTOR_H_INCLUDED

class CHeadlight;
class CParticle;
class CSoundEffector;
class CModelPlugin;
class CScene;
class CRailWay;

//	タイプフラグ
const int EFCT_MISCELLANEOUS = 0x0001;
const int EFCT_PARTICLEAPPLIER = 0x0002;
const int EFCT_RAILCONNECTOR = 0x0004;

/*
 *	エフェクタ基本クラス
 */
class CEffectorBase{
protected:
public:
	virtual ~CEffectorBase(){}
	virtual CEffectorBase *Duplicate() = 0;
	virtual void LoadDataEffector(CModelPlugin *){}
	virtual int GetTypeFlagEffector(){ return EFCT_MISCELLANEOUS; }
	virtual void ApplyEffector(CScene *){}
};

/*
 *	エフェクタコンテナ
 */
class CEffectorContainer{
private:
	CEffectorBase *m_Effector;	//	エフェクタ
public:
	CEffectorContainer();
	CEffectorContainer(const CEffectorContainer &);
	~CEffectorContainer();
	char *Read(char *, CModelPlugin *);
	void LoadData(CModelPlugin *mpi){ m_Effector->LoadDataEffector(mpi); }
	int GetTypeFlag(){ return m_Effector->GetTypeFlagEffector(); }
	void Apply(CScene *scene){ m_Effector->ApplyEffector(scene); }
};

//	反復子
typedef list<CEffectorContainer>::iterator IEffectorContainer;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ヘッドライト適用子
 */
class CHeadlightApplier: public CEffectorBase{
private:
	CHeadlight *m_Headlight;	//	ヘッドライト
public:
	CHeadlightApplier(){ m_Headlight = NULL; }
	CEffectorBase *Duplicate(){ return new CHeadlightApplier(*this); }
	char *Read(char *, CModelPlugin *);
	void LoadDataEffector(CModelPlugin *);
	void ApplyEffector(CScene *);
};

/*
 *	パーティクル適用子
 */
class CParticleApplier: public CEffectorBase{
private:
	CParticle *m_Particle;	//	パーティクル
public:
	CParticleApplier(){ m_Particle = NULL; }
	CEffectorBase *Duplicate(){ return new CParticleApplier(*this); }
	char *Read(char *, CModelPlugin *);
	void LoadDataEffector(CModelPlugin *);
	int GetTypeFlagEffector(){ return EFCT_PARTICLEAPPLIER; }
	void ApplyEffector(CScene *);
};

/*
 *	サウンド適用子
 */
class CSoundApplier: public CEffectorBase{
private:
	CSoundEffector *m_SoundEffector;	//	サウンドエフェクタ
public:
	CSoundApplier(){ m_SoundEffector = NULL; }
	CEffectorBase *Duplicate(){ return new CSoundApplier(*this); }
	char *Read(char *, CModelPlugin *);
	void ApplyEffector(CScene *);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	レール接続制御基本クラス
 */
class CEffectorRailConnectorBase: public CEffectorBase{
protected:
	int m_RailEnd1;	//	接続先1
	int m_RailEnd2;	//	接続先2
	int m_PlatformID1;
	int m_PlatformID2;
	int m_PlatformEnd1;
	int m_PlatformEnd2;
public:
	CEffectorRailConnectorBase(){ m_RailEnd1 = -1; m_RailEnd2 = -1; }
	virtual CEffectorBase *Duplicate() = 0;
	int GetTypeFlagEffector(){ return EFCT_RAILCONNECTOR; }
	virtual void ApplyEffector(CScene *);
	virtual void ApplyEffectorRailConnectorBase(CScene *, CRailWay *, CRailWay *) = 0;
	void CalculatePlatformID();
};

/*
 *	レール接続制御基本クラス
 */
class CEffectorRailDisconnectorBase: public CEffectorBase{
protected:
	int m_RailEnd;	//	接続先1
	int m_PlatformID;
	int m_PlatformEnd;
public:
	CEffectorRailDisconnectorBase(){ m_RailEnd = -1; }
	virtual CEffectorBase *Duplicate() = 0;
	int GetTypeFlagEffector(){ return EFCT_RAILCONNECTOR; }
	virtual void ApplyEffector(CScene *);
	virtual void ApplyEffectorRailDisconnectorBase(CScene *, CRailWay *) = 0;
	void CalculatePlatformID();
};

/*
 *	レール接続
 */
class CEffectorRailConnector: public CEffectorRailConnectorBase{
public:
	CEffectorBase *Duplicate(){ return new CEffectorRailConnector(*this); }
	char *Read(char *, CModelPlugin *);
	void ApplyEffectorRailConnectorBase(CScene *, CRailWay *, CRailWay *);
};

/*
 *	レール分岐
 */
class CEffectorRailBrancher: public CEffectorRailConnectorBase{
public:
	CEffectorBase *Duplicate(){ return new CEffectorRailBrancher(*this); }
	char *Read(char *, CModelPlugin *);
	void ApplyEffectorRailConnectorBase(CScene *, CRailWay *, CRailWay *);
};

/*
 *	レール接続解除
 */
class CEffectorRailDisconnector: public CEffectorRailDisconnectorBase{
public:
	CEffectorBase *Duplicate(){ return new CEffectorRailDisconnector(*this); }
	char *Read(char *, CModelPlugin *);
	void ApplyEffectorRailDisconnectorBase(CScene *, CRailWay *);
};

#endif
