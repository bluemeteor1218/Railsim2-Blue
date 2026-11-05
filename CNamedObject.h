#ifndef CNAMEDOBJECT_H_INCLUDED
#define CNAMEDOBJECT_H_INCLUDED

#include "CModelSwitch.h"
#include "CPartsInst.h"

class CModelInst;
class CNamedObject;
class CModelPlugin;

const float SYMMETRIC_ROTATION_MAX = 0.45f;	//	最高対称回転速度

//	システムオブジェクト番号
extern enum{
	SYS_OBJ_WORLD = 0,
	SYS_OBJ_LOCAL,
	SYS_OBJ_CAMERA,
	SYS_OBJ_LIGHT,
	SYS_OBJ_FORCE_DWORD = 0xffffffff
} SYS_OBJ_ID;

/*
 *	名前付きオブジェクトアフターレンダラ
 */
class CNamedObjectAfterRenderer{
private:
	static CModelInst *ms_CurrentInst;		//	現在のインスタンス
	static CModelPlugin *ms_CurrentPlugin;	//	現在のモデルプラグイン
	int m_InstSwitch[INSTANCE_SWITCH_NUM];	//	インスタンススイッチ
	CModelInst *m_ModelInst;				//	現在のインスタンス
	CModelPlugin *m_ModelPlugin;			//	現在のモデルプラグイン
	CNamedObject *m_NamedObject;			//	名前付きオブジェクト
	CPartsInst *m_PartsInst;				//	パーツインスタンス
	bool m_PartsInstCreation;				//	パーツインスタンス作成フラグ
public:
	static void SetCurrentInst(CModelInst *inst){ ms_CurrentInst = inst; }
	static void SetCurrentPlugin(CModelPlugin *mpi){ ms_CurrentPlugin = mpi; }
	CNamedObjectAfterRenderer(CNamedObject *, CPartsInst *);
	CNamedObjectAfterRenderer(const CNamedObjectAfterRenderer &);
	~CNamedObjectAfterRenderer();
	void Render(int);
};

//	反復子
typedef list<CNamedObjectAfterRenderer>::iterator INamedObjectAfterRenderer;

/*
 *	名前付きオブジェクト
 */
class CNamedObject{
	friend class CNamedObjectAfterRenderer;
protected:
	static int ms_AfterMode;				//	アフターレンダリングモード
	static int ms_AfterRender;				//	アフターレンダリングフラグ
											//	(1: noshadow, 2: transparent)
	static bool ms_CastShadowDefault;		//	影描画フラグデフォルト値
	static bool ms_SetMaterial;				//	マテリアル変更フラグ
	static bool ms_PartsInstValid;			//	パーツインスタンス使用フラグ
	static IPartsInst ms_PartsInst;			//	パーツインスタンス反復子
	static list<CNamedObjectAfterRenderer> ms_AfterRenderList;	//	アフターレンダラ
	static list<CNamedObjectAfterRenderer> ms_TransRenderList;	//	透明ポリレンダラ
	float m_ModelScale;						//	モデルスケール
	bool m_Turn;							//	反転フラグ
	bool m_CastShadow;						//	影キャストフラグ
	string m_ModelFileName;					//	モデルファイル名
	string m_ObjectName;					//	オブジェクト名
	CMesh *m_Mesh;							//	メッシュ
	CObject *m_LastObject;					//	最後に使用したオブジェクト
	CObject m_PreviewObject;				//	プレビュー用オブジェクト
	list<CCustomizerBase *> m_OffList;		//	常時適用カスタマイザエントリ
	CCustomizerSwitchEntry m_Customizer;	//	ルートカスタマイザ
public:
	static void SetCastShadowDefault(bool s){ ms_CastShadowDefault = s; }
	static void SetSetMaterial(bool s){ ms_SetMaterial = s; }
	static void SetPartsInst(list<CPartsInst> *);
	static CPartsInst *GetParts(){ return ms_PartsInstValid ? &*(ms_PartsInst++) : NULL; }
	static void SetRenderAfter(int m){ ms_AfterRender |= m; }
	static int GetAfterRender(){ return ms_AfterMode; }
	static void InitAfterRenderList();
	static void AfterRenderAll();
	CNamedObject();
	virtual int GetType(){ return 0; }
	void Init(char *name){ m_ObjectName = name; }
	char *ReadModelInfo(char *, CModelPlugin *);
	char *GetName(){ return (char *)m_ObjectName.c_str(); }
	CNamedObject *Check(const string &name){ return m_ObjectName==name ? this : NULL; }
	CObject *GetObject(){ return m_LastObject; }
	CObject *GetPreviewObject(){ return &m_PreviewObject; }
	void LoadModel(CModelPlugin *);
	void AddOffList(CCustomizerBase *cb){ m_OffList.push_back(cb); }
	void SetMesh(CObject *);
	bool GetTurn(){ return m_Turn; }
	void SetPreviewPosture(VEC3, VEC3, VEC3);
	CObject *GetPartsObject(){
		//Debug("&*ms_PartsInst = %p\n", &*ms_PartsInst);
		return m_LastObject = ms_PartsInstValid
			? (ms_PartsInst++)->GetObject() : &m_PreviewObject;
	}
	void CheckDetect();
	void Render();
	void RenderAfter(CPartsInst *, int);
	void SetMaterial(CMesh *mesh){ m_Customizer.Apply(mesh); }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	オブジェクトジョイント基本クラス
 */
class CObjectJoint{
public:
	CNamedObject *m_Link;	//	接続先オブジェクト
public:
	CObjectJoint(){ m_Link = NULL; }
	char *Read(char *, CModelPlugin *);
	bool IsLinked(){ return !!m_Link; }
	VEC3 GetRight(){ return m_Link->GetObject()->GetRight(); }
	VEC3 GetUp(){ return m_Link->GetObject()->GetUp(); }
	VEC3 GetDir(){ return m_Link->GetObject()->GetDir(); }
};

/*
 *	オブジェクトジョイント pos, dir, up 指定
 */
class CObjectJoint3D: public CObjectJoint{
public:
	bool m_FixCenter;			//	中心修正フラグ
	VEC3 m_AttachCoord;			//	接続先ローカル座標
	VEC3 m_LocalCoord;			//	接続元ローカル座標
	VEC3 m_AttachDir;			//	接続先ローカル dir
	VEC3 m_AttachUp;			//	接続先ローカル up
	CNamedObject *m_DirLink;	//	dir 基準オブジェクト
	CNamedObject *m_UpLink;		//	up 基準オブジェクト
public:
	CObjectJoint3D(){ m_DirLink = m_UpLink = NULL; }
	char *Read(char *, CModelPlugin *);
	void SetTurnLocal();
	VEC3 GetPos(){
		VEC3 ret;
		CObject *obj = m_Link->GetObject();
		return *D3DXVec3TransformCoord(
			&ret, &(m_AttachCoord/obj->GetScale()), &obj->GetMatrix());
	}
	VEC3 GetDir(){
		VEC3 ret;
		return *D3DXVec3TransformNormal(
			&ret, &m_AttachDir, &m_DirLink->GetObject()->GetMatrix());
	}
	VEC3 GetUp(){
		VEC3 ret;
		return *D3DXVec3TransformNormal(
			&ret, &m_AttachUp, &m_UpLink->GetObject()->GetMatrix());
	}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	フリーオブジェクト基本クラス
 */
class CFreeObjectBase{
protected:
public:
	virtual ~CFreeObjectBase(){}
	virtual CFreeObjectBase *Duplicate() = 0;
	virtual void SetPostureFreeObject() = 0;
	virtual CNamedObject *CheckFreeObject(const string &) = 0;
	virtual int GetNamedObjectNum() = 0;
	virtual CNamedObject *GetNamedObject(int i) = 0;
	virtual void LoadModelFreeObject(CModelPlugin *) = 0;
	virtual void AttachPartsFreeObject() = 0;
	virtual void ScanInputFreeObject() = 0;
	virtual void RenderFreeObject() = 0;
};

/*
 *	ベクトル配置
 */
class CFreeObject3D: public CNamedObject, public CFreeObjectBase{
	friend class CTrainPlugin;
private:
	CObjectJoint3D m_Joint;	//	接続情報
public:
	CFreeObject3D(){}
	CFreeObject3D(char *, char *, float);
	CFreeObjectBase *Duplicate(){ return new CFreeObject3D(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureFreeObject();
	CNamedObject *CheckFreeObject(const string &name){ return Check(name); }
	int GetNamedObjectNum(){ return 1; }
	CNamedObject *GetNamedObject(int i){ return this; }
	void LoadModelFreeObject(CModelPlugin *mpi){ LoadModel(mpi); }
	void AttachPartsFreeObject(){ GetPartsObject(); }
	void ScanInputFreeObject(){ CheckDetect(); }
	void RenderFreeObject(){ Render(); }
};

//	反復子
typedef list<CFreeObject3D>::iterator IFreeObject3D;

//	関数宣言
void InitSystemObject();
CNamedObject *FindSystemObject(const string &);
CNamedObject *FindObjectHybrid(CModelPlugin *, const string &);
void CheckObjectHybrid(char *, CModelPlugin *, const string &);

//	外部グローバル
extern bool g_PreSimulationFlag;
extern CNamedObject g_SystemObject[];

#endif
