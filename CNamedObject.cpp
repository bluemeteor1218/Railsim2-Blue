#include "stdafx.h"
#include "CListView.h"
#include "CModelInst.h"
#include "CEnvPlugin.h"
#include "CConfigMode.h"
#include "CSceneryMode.h"

//	内部定数
const int SYSTEM_OBJECT_NUM = 4;	//	システムスイッチ数

//	外部グローバル
extern MAT8 *g_AltMaterial;
extern MAT8 g_MatSelect[];

//	内部グローバル
bool g_PreSimulationFlag;
CNamedObject g_SystemObject[SYSTEM_OBJECT_NUM];

//	static メンバ
CModelInst *CNamedObjectAfterRenderer::ms_CurrentInst;
CModelPlugin *CNamedObjectAfterRenderer::ms_CurrentPlugin;

/*
 *	コンストラクタ
 */
CNamedObjectAfterRenderer::CNamedObjectAfterRenderer(
	CNamedObject *obj,	//	名前付きオブジェクト
	CPartsInst *parts	//	パーツインスタンス
){
	int i;
	for(i = 0; i<INSTANCE_SWITCH_NUM; i++) m_InstSwitch[i] = g_SystemSwitch[i].GetValue();
	m_ModelInst = ms_CurrentInst;
	m_ModelPlugin = ms_CurrentPlugin;
	m_NamedObject = obj;
	if(m_PartsInst = parts){
		m_PartsInstCreation = false;
	}else{
		m_PartsInstCreation = true;
		m_PartsInst = new CPartsInst;
		*m_PartsInst->GetObject() = *obj->GetPreviewObject();
	}
}

/*
 *	コピーコンストラクタ
 */
CNamedObjectAfterRenderer::CNamedObjectAfterRenderer(
	const CNamedObjectAfterRenderer &src	//	コピー元
){
	int i;
	for(i = 0; i<INSTANCE_SWITCH_NUM; i++) m_InstSwitch[i] = src.m_InstSwitch[i];
	m_ModelInst = src.m_ModelInst;
	m_ModelPlugin = src.m_ModelPlugin;
	m_NamedObject = src.m_NamedObject;
	if(m_PartsInstCreation = src.m_PartsInstCreation)
		m_PartsInst = src.m_PartsInst ? new CPartsInst(*src.m_PartsInst) : NULL;
	else m_PartsInst = src.m_PartsInst;
}

/*
 *	デストラクタ
 */
CNamedObjectAfterRenderer::~CNamedObjectAfterRenderer(){
	if(m_PartsInstCreation) DELETE_V(m_PartsInst);
}

/*
 *	レンダリング
 */
void CNamedObjectAfterRenderer::Render(
	int mode	//	1: noshadow, 2: transparent
){
	if(m_ModelInst){
		if(m_ModelInst->GetSelectFlag() && m_ModelInst->IsSelectVisible())
			g_AltMaterial = &g_MatSelect[m_ModelInst->GetSelectFlag()];
		m_ModelPlugin->SetSwitch(m_ModelInst);
		m_ModelPlugin->SetAnimation(m_ModelInst);
	}
	int i;
	for(i = 0; i<INSTANCE_SWITCH_NUM; i++) g_SystemSwitch[i].SetValue(m_InstSwitch[i]);
	m_NamedObject->RenderAfter(m_PartsInst, mode);
	g_AltMaterial = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
int CNamedObject::ms_AfterMode;
int CNamedObject::ms_AfterRender;
bool CNamedObject::ms_CastShadowDefault;
bool CNamedObject::ms_SetMaterial;
bool CNamedObject::ms_PartsInstValid;
IPartsInst CNamedObject::ms_PartsInst;
list<CNamedObjectAfterRenderer> CNamedObject::ms_AfterRenderList;
list<CNamedObjectAfterRenderer> CNamedObject::ms_TransRenderList;

/*
 *	[static]
 *	パーツインスタンス設定
 */
void CNamedObject::SetPartsInst(
	list<CPartsInst> *pl	//	パーツインスタンスリスト
){
	if(ms_PartsInstValid = !!pl){
		ms_PartsInst = pl->begin();
		//Debug("----\nCNamedObject::SetPartsInst(): pl->size() = %d\nobjects =", pl->size());
		//list<CPartsInst>::iterator itr = pl->begin();
		//for(; itr!=pl->end(); ++itr) Debug(" %p", &*itr);
		//Debug("\n----\n");
	}
}

/*
 *	[static]
 *	リスト初期化
 */
void CNamedObject::InitAfterRenderList(){
	ms_AfterRenderList.clear();
	ms_TransRenderList.clear();
}

/*
 *	[static]
 *	全てレンダリング
 */
void CNamedObject::AfterRenderAll(){
	devBLEND_ALPHA();
	INamedObjectAfterRenderer inoar;
	inoar = ms_AfterRenderList.begin();
	for(; inoar!=ms_AfterRenderList.end(); inoar++) inoar->Render(1);
	inoar = ms_TransRenderList.begin();
	for(; inoar!=ms_TransRenderList.end(); inoar++) inoar->Render(2);
}

/*
 *	読込
 */
CNamedObject::CNamedObject(){
	m_Mesh = NULL;
	m_LastObject = &m_PreviewObject;
	m_CastShadow = ms_CastShadowDefault;
}

/*
 *	読込
 */
char *CNamedObject::ReadModelInfo(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	if(!(str = AsgnString(str, "ModelFileName", &m_ModelFileName))) return NULL;
	if(tmp = AsgnFloat(str, "ModelScale", &m_ModelScale)) str = tmp;
	else m_ModelScale = 1.0f;
	if(m_ModelScale<0.0f) m_ModelScale = 0.0f;
	if(tmp = AsgnYesNo(str, "Turn", &m_Turn)) str = tmp;
	else m_Turn = false;
	if(tmp = AsgnYesNo(str, "CastShadow", &m_CastShadow)) str = tmp;
	else m_CastShadow = ms_CastShadowDefault;
	if(tmp = m_Customizer.Read(str, mpi, 1)) str = tmp;
	m_OffList.clear();
	return str;
}

/*
 *	モデル読込
 */
void CNamedObject::LoadModel(
	CModelPlugin *mpi	//	呼び出し元
){
	m_Mesh = g_MeshList.Get(FALSE, m_ModelFileName.c_str(), 0, !g_NamedObjectMipMap);
	mpi->ChDir();
	m_Customizer.LoadData(mpi);
	m_Customizer.SetOffList(this);
}

/*
 *	メッシュ設定
 */
void CNamedObject::SetMesh(
	CObject *obj	//	オブジェクト
){
	float asc = -1.0f;
	CMesh *altmesh = m_Customizer.GetMesh(&asc);
	if(asc<0.0f) obj->SetMesh(m_Mesh, V3ZERO, m_ModelScale);
	else obj->SetMesh(altmesh, V3ZERO, asc);
}

/*
 *	姿勢固定
 */
void CNamedObject::SetPreviewPosture(
	VEC3 pos,	//	位置
	VEC3 dir,	//	方向
	VEC3 up		//	アップ
){
	m_PreviewObject.SetScale(1.0f);
	m_PreviewObject.SetPos(pos);
	m_PreviewObject.SetDir(dir, up);
}

/*
 *	検出判定
 */
void CNamedObject::CheckDetect(){
	CModelInst::CheckDetect(this, GetParts());
}

/*
 *	レンダリング
 */
void CNamedObject::Render(){
	CPartsInst *parts = GetParts();
	CObject *obj = m_LastObject = (parts ? parts->GetObject() : &m_PreviewObject);
//	CCustomizerBase::SetObject(this);
	ms_AfterRender = 0;
	ms_AfterMode = 0;
	obj->ResetMatFlag();
	if(ms_SetMaterial) obj->RenderCustom(this); else obj->Render();
	if(g_ShadowNeeded && ms_AfterRender&1)
		ms_AfterRenderList.push_back(CNamedObjectAfterRenderer(this, parts));
	if(ms_AfterRender&2)
		ms_TransRenderList.push_back(CNamedObjectAfterRenderer(this, parts));
	if(m_CastShadow) CastShadow(obj);
}

/*
 *	アフターレンダリング
 */
void CNamedObject::RenderAfter(
	CPartsInst *parts,	//	パーツインスタンス
	int mode			//	1: noshadow, 2: transparent
){
	CObject *obj = parts ? parts->GetObject() : &m_PreviewObject;
	ms_AfterMode = mode;
	obj->ResetMatFlag(2);
//	CCustomizerBase::SetObject(this);
	obj->RenderCustom(this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CObjectJoint3D::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	string obj;
	if(!(str = BeginNamedBlock(eee = str, "Joint3D", &obj))) return NULL;
	if(!(m_Link = FindObjectHybrid(mpi, obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(!(str = AsgnVector3D(eee = str, "AttachCoord", &m_AttachCoord))) throw CSynErr(eee);
	if(tmp = AsgnVector3D(eee = str, "LocalCoord", &m_LocalCoord)) str = tmp;
	else m_LocalCoord = V3ZERO;
	m_FixCenter = m_LocalCoord!=V3ZERO ? true : false;
	if(tmp = AsgnString(eee = str, "DirLink", &obj)){
		str = tmp;
		if(!(m_DirLink = FindObjectHybrid(mpi, obj)))
			throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	}else{
		m_DirLink = m_Link;
	}
	if(tmp = AsgnVector3D(eee = str, "AttachDir", &m_AttachDir)) str = tmp;
	else m_AttachDir = V3DIR;
	if(tmp = AsgnString(eee = str, "UpLink", &obj)){
		str = tmp;
		if(!(m_UpLink = FindObjectHybrid(mpi, obj)))
			throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	}else{
		m_UpLink = m_Link;
	}
	if(tmp = AsgnVector3D(eee = str, "AttachUp", &m_AttachUp)) str = tmp;
	else m_AttachUp = V3UP;
	VEC3 right;
	V3NormAxis(&right, &m_AttachUp, &m_AttachDir);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	ローカル系をセット
 */
void CObjectJoint3D::SetTurnLocal(){
	m_Link = m_DirLink = m_UpLink = &g_SystemObject[SYS_OBJ_LOCAL];
	m_AttachCoord = m_LocalCoord = V3ZERO;
	m_AttachDir = -V3DIR;
	m_AttachUp = V3UP;
	m_FixCenter = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ (旧規格用)
 */
CFreeObject3D::CFreeObject3D(
	char *objname,		//	オブジェクト名
	char *modelname,	//	モデルファイル名
	float scale			//	スケール
){
	m_ObjectName = objname;
	m_ModelFileName = modelname;
	m_ModelScale = scale;
	m_Turn = false;
	m_Joint.SetTurnLocal();
}

/*
 *	読込
 */
char *CFreeObject3D::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginNamedBlock(eee = str, "Object3D", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(1);
	return str;
}

/*
 *	姿勢設定
 */
void CFreeObject3D::SetPostureFreeObject(){
	CObject *obj = GetPartsObject();
	SetMesh(obj);
	obj->SetDir(m_Joint.GetDir(), m_Joint.GetUp());
	obj->SetPos(m_Joint.GetPos());
	if(m_Joint.m_FixCenter){
		VEC3 pos;
		D3DXVec3TransformCoord(&pos,
			&(-m_Joint.m_LocalCoord/obj->GetScale()), &obj->GetMatrix());
		obj->SetPos(pos);
	}
	IPCustomizerBase ipc = m_OffList.begin();
	for(; ipc!=m_OffList.end(); ipc++) (*ipc)->ResetOffCustomizer(0);
	m_Customizer.SetPosture(obj);
	ipc = m_OffList.begin();
	for(; ipc!=m_OffList.end(); ipc++){
		(*ipc)->ResetOffCustomizer(2);
		(*ipc)->SetPostureCustomizer(obj);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	システムオブジェクト初期化
 */
void InitSystemObject(){
	g_SystemObject[SYS_OBJ_WORLD].Init("_WORLD");
	g_SystemObject[SYS_OBJ_LOCAL].Init("_LOCAL");
	g_SystemObject[SYS_OBJ_CAMERA].Init("_CAMERA");
	g_SystemObject[SYS_OBJ_LIGHT].Init("_LIGHT");
}

/*
 *	システムオブジェクトの検索
 */
CNamedObject *FindSystemObject(
	const string &name	//	オブジェクト名
){
	int i;
	for(i = 0; i<SYSTEM_OBJECT_NUM; i++)
		if(g_SystemObject[i].Check(name)) return &g_SystemObject[i];
	return NULL;
}

/*
 *	システムオブジェクト + 指定プラグイン内オブジェクトの検索
 */
CNamedObject *FindObjectHybrid(
	CModelPlugin *mpi,	//	モデルプラグイン
	const string &name	//	オブジェクト名
){
	CNamedObject *ret = mpi->FindObject(name);
	return ret ? ret : FindSystemObject(name);
}

/*
 *	オブジェクト重複チェック
 */
void CheckObjectHybrid(
	char *eee,			//	エラー報告位置
	CModelPlugin *mpi,	//	モデルプラグイン
	const string &name	//	オブジェクト名
){
	if(FindSystemObject(name))
		throw CSynErr(eee, "%s: \"%s\"", lang(ReservedObjectName), name.c_str());
	if(mpi->FindObject(name))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedObjectName), name.c_str());
}
