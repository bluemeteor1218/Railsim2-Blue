#ifndef CCUSTOMIZER_H_INCLUDED
#define CCUSTOMIZER_H_INCLUDED

class CTextureAnimation;
class CModelPlugin;

//	タイプフラグ
const int CSTM_MISCELLANEOUS = 0x0001;
const int CSTM_MODELCHANGER = 0x0002;
const int CSTM_MODELMOVER = 0x0004;

/*
 *	ムーバー状態変数
 */
class CMoverState{
private:
	VEC3 m_Pos;	//	位置
	VEC3 m_Dir;	//	方向
public:
	CMoverState(){ Reset(); }
	void Reset(){ m_Pos = V3ZERO; m_Dir = V3DIR; }
	VEC3 GetPos(){ return m_Pos; }
	VEC3 GetDir(){ return m_Dir; }
	void SetPos(const VEC3 &pos){ m_Pos = pos; }
	void SetDir(const VEC3 &dir){ m_Dir = dir; }
	char *Read(char *);
	void Save(FILE *, char *);
};

//	反復子
typedef list<CMoverState>::iterator IMoverState;
typedef list<CMoverState *>::iterator IPMoverState;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	カスタマイザ基本クラス
 */
class CCustomizerBase{
protected:
//	static CNamedObject *ms_CurrentObject;	//	現在のオブジェクト
public:
//	static void SetObject(CNamedObject *obj){ ms_CurrentObject = obj; }
	virtual ~CCustomizerBase(){}
	virtual CCustomizerBase *Duplicate() = 0;
	virtual void LoadDataCustomizer(CModelPlugin *){}
	virtual void SetOffListCustomizer(CNamedObject *){}
	virtual void ResetOffCustomizer(int){}
	virtual int GetTypeFlagCustomizer(){ return CSTM_MISCELLANEOUS; }
	virtual CMesh *GetMeshCustomizer(float *asc){ return NULL; }
	virtual void SetPostureCustomizer(CObject *){}
	virtual void ApplyCustomizer(CMesh *){}
};

//	反復子
typedef list<CCustomizerBase *>::iterator IPCustomizerBase;

/*
 *	カスタマイザコンテナ
 */
class CCustomizerContainer{
private:
	CCustomizerBase *m_Customizer;	//	カスタマイザ
public:
	CCustomizerContainer();
	CCustomizerContainer(const CCustomizerContainer &);
	~CCustomizerContainer();
	char *Read(char *, CModelPlugin *);
	void LoadData(CModelPlugin *mpi){ m_Customizer->LoadDataCustomizer(mpi); }
	void SetOffList(CNamedObject *nobj){ m_Customizer->SetOffListCustomizer(nobj); }
	int GetTypeFlag(){ return m_Customizer->GetTypeFlagCustomizer(); }
	CMesh *GetMesh(float *asc){ return m_Customizer->GetMeshCustomizer(asc); }
	void SetPosture(CObject *obj){ m_Customizer->SetPostureCustomizer(obj); }
	void Apply(CMesh *mesh){ m_Customizer->ApplyCustomizer(mesh); }
};

//	反復子
typedef list<CCustomizerContainer>::iterator ICustomizerContainer;

#endif
