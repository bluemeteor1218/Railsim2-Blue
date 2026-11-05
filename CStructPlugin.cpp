#include "stdafx.h"
#include "CCamera.h"
#include "CStructPlugin.h"
#include "CStruct.h"
#include "CStructSelectMode.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"
#include "CSaveFile.h"

//	外部グローバル
extern bool g_MoverEnabled;

//	内部グローバル
CStructPlugin *g_Struct = NULL;

/*
 *	[static]
 *	プレビュー
 */
void CStructPlugin::RenderPreview(
	VEC3 pos,	//	位置
	VEC3 dir,	//	dir
	VEC3 up		//	up
){
	CNamedObjectAfterRenderer::SetCurrentInst(NULL);
	g_SaveFile->ResetSwitch();
	if(ms_PreviewState && g_Struct){
		g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(pos, dir, up);
		SetCamDistSwitch(pos);
		g_Struct->Preview();
	}
}

/*
 *	デストラクタ
 */
CStructPlugin::~CStructPlugin(){
}

/*
 *	ロード
 */
bool CStructPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	g_NamedObjectMipMap = g_StructMipMap;
	CNamedObject::SetCastShadowDefault(true);
	try{
		if(!(str = LoadStructBefore(eee = str))) throw CSynErr(eee);

		if(!(str = ReadModelSwitch(eee = str))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "PrimaryAssembly"))) throw CSynErr(eee);
		CFreeObjectContainer freeobj;
		while(tmp = freeobj.Read(str, this)){
			str = tmp;
			m_FreeObject.push_back(freeobj);
		}

		if(!(str = LoadStructAfter(eee = str))) throw CSynErr(eee);

		if(!(str = ReadEffect(eee = str))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->LoadModel(this);
	LoadData();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	ロード
 */
char *CStructPlugin::LoadStructBefore(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(eee = str, "StructInfo"))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	ロード
 */
bool CStructPlugin::LoadOldForm(){
	if(!ChDir()) return false;
	g_NamedObjectMipMap = g_StructMipMap;
	CNamedObject::SetCastShadowDefault(true);
	FILE *file = fopen(TextName(), "rt");
	char *dummy = FlashOut();
	float sc;
	fscanf(file, "%s %s %f", dummy, dummy, &sc);
	fclose(file);
	float oldscale = 2.0f/sc;
	m_FreeObject.push_back(CFreeObjectContainer(new CFreeObject3D("MainObject", "Model.x", oldscale)));
	m_FreeObject.begin()->LoadModel(this);
	m_PartsNum = 1;
	return true;
}

/*
 *	プレビュー設定
 */
void CStructPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Struct = this;
	string desc = g_Struct->GetBasicInfo();
	desc += "\n"+g_Struct->GetDescription();
	g_StructSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	プレビュー
 */
void CStructPlugin::Preview(){
	SetPartsInst(NULL);
	SetMoverState(NULL);
	SetPosture();
	Render(NULL);
	PreviewStruct();
}

/*
 *	オブジェクト検索
 */
CNamedObject *CStructPlugin::FindObject(
	const string &name	//	オブジェクト名
){
	CNamedObject *ret;
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) if(ret = ifo->Check(name)) return ret;
	return NULL;
}

/*
 *	サウンド有効かどうか
 */
bool CStructPlugin::IsSoundEnabled(){
	return !!g_ConfigMode->GetStructSound();
}

/*
 *	姿勢設定
 */
void CStructPlugin::SetPosture(){
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->SetPosture();
}

/*
 *	入力チェック
 */
void CStructPlugin::ScanInput(
	CStruct *strct	//	施設インスタンス
){
	strct->SetLocalAxis();
	SetCamDistSwitch(strct->GetPos());
	g_PreSimulationFlag = false;
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->ScanInput();
}

/*
 *	レンダリング
 */
void CStructPlugin::Render(
	CStruct *strct	//	施設インスタンス
){
	SetAnimation(strct);
	if(strct){
		strct->SetLocalAxis();
		SetCamDistSwitch(strct->GetPos());
	}
	g_PreSimulationFlag = false;
	CNamedObject::SetSetMaterial(m_Version>=2.00f);
	if(strct && !g_SimulationMode->GetSimSpeed()){
		g_MoverEnabled = false;
		SetMoverState(strct);
		SetPosture();
		SetPartsInst(strct);
		g_MoverEnabled = true;
	}
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->Render();
	SimulateEffect(strct);
}

/*
 *	シミュレーション進行
 */
void CStructPlugin::Simulate(
	CStruct *strct	//	施設インスタンス
){
	SetMoverState(strct);
	strct->SetLocalAxis();
	SetCamDistSwitch(strct->GetPos());
	g_PreSimulationFlag = true;
	SetPosture();
	SimulateEffect(strct);
}
