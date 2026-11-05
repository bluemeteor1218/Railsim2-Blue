#include "stdafx.h"
#include "CCamera.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CStationPlugin.h"
#include "CStation.h"
#include "CRailLink.h"
#include "CRailBuilder.h"
#include "CLine.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailBuildMode.h"
#include "CStationSelectMode.h"
#include "CConfigMode.h"

//	外部グローバル
extern bool g_EnableCant;

//	内部グローバル
CStationPlugin *g_Station = NULL;
CDetectInfo g_StationPlatformParentDetectInfo;

//	static メンバ
CRailPlugin *CPlatform::ms_PlatformRail = NULL;
CTiePlugin *CPlatform::ms_PlatformTie = NULL;
CGirderPlugin *CPlatform::ms_PlatformGirder = NULL;
CPierPlugin *CPlatform::ms_PlatformPier = NULL;
CLinePlugin *CPlatform::ms_PlatformLine = NULL;
CPolePlugin *CPlatform::ms_PlatformPole = NULL;

/*
 *	[static]
 *	プラットフォーム用プラグインをリセット
 */
void CPlatform::ResetPlatformPlugin(){
	CRailwayMode::GetPlugin(&ms_PlatformRail, &ms_PlatformTie,
		&ms_PlatformGirder, &ms_PlatformPier, &ms_PlatformLine, &ms_PlatformPole);
}

/*
 *	コンストラクタ
 */
CPlatform::CPlatform(){
	m_TrackNum = 1;
	m_TrackInterval = 0.0f;
	m_RailPluginValid = m_TiePluginValid = m_GirderPluginValid =
		m_PierPluginValid = m_LinePluginValid = m_PolePluginValid = false;
	m_RailPlugin = NULL;
	m_TiePlugin = NULL;
	m_GirderPlugin = NULL;
	m_PierPlugin = NULL;
	m_LinePlugin = NULL;
	m_PolePlugin = NULL;
	m_Stoppable = true;
	m_OpenDoor[0] = m_OpenDoor[1] = false;
	m_LiftRailSurface = true;
	m_EnableCant = true;
	m_ParentObject = NULL;
}

/*
 *	読込
 */
char *CPlatform::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	string id;
	if(!(str = BeginBlock(eee = str, "Platform"))) return NULL;
	m_TrackNum = 1;
	m_TrackInterval = 0.0f;
	if(tmp = AsgnInteger(eee = str, "TrackNum", &m_TrackNum)) str = tmp;
	if(m_TrackNum<1){
		throw CSynErr(eee, lang(InvalidTrackNum));
	}else if(m_TrackNum>1){
		if(!(str = AsgnFloat(eee = str, "TrackInterval", &m_TrackInterval))) throw CSynErr(eee);
	}
	if(tmp = AsgnYesNo(eee = str, "Stoppable", &m_Stoppable)) str = tmp;
	else m_Stoppable = true;
	if(tmp = AsgnYesNo(eee = str, "OpenDoor", m_OpenDoor, 2, false)) str = tmp;
	else m_OpenDoor[0] = m_OpenDoor[1] = false;
	bool *valid[6] = {&m_RailPluginValid, &m_TiePluginValid, &m_GirderPluginValid,
		&m_PierPluginValid, &m_LinePluginValid, &m_PolePluginValid};
	CPlugin **adr[6] = {
		(CPlugin **)&m_RailPlugin, (CPlugin **)&m_TiePlugin, (CPlugin **)&m_GirderPlugin,
		(CPlugin **)&m_PierPlugin, (CPlugin **)&m_LinePlugin, (CPlugin **)&m_PolePlugin};
	CPluginList *pilist[6] = {g_RailPluginList, g_TiePluginList,
		g_GirderPluginList, g_PierPluginList, g_LinePluginList, g_PolePluginList};
	char *pitype[6] = {
		lang(Rail), lang(Tie), lang(Girder), lang(Pier), lang(Line), lang(Pole)};
	int i;
	for(i = 0; i<6; i++){
		if(tmp = AsgnString(eee = str, FlashIn("%sPlugin", pilist[i]->DirName()), &id)){
			str = tmp;
			*valid[i] = true;
			if(id.size()){
				if(!(*adr[i] = pilist[i]->FindPlugin(id.c_str()))) throw CSynErr(
					eee, "%s (%s): \"%s\"", lang(LackedPlugin), pitype[i], id.c_str());
			}else{
				*adr[i] = NULL;
			}
		}else{
			*valid[i] = false;
			*adr = NULL;
		}
	}
	if(tmp = AsgnYesNo(eee = str, "LiftRailSurface", &m_LiftRailSurface)) str = tmp;
	else m_LiftRailSurface = true;
	if(tmp = AsgnYesNo(eee = str, "EnableCant", &m_EnableCant)) str = tmp;
	else m_EnableCant = true;
	if(tmp = AsgnString(eee = str, "ParentObject", &m_ParentObjectName)) str = tmp;
	m_ParentObjectScriptPos = eee;
	m_CoordList.clear();
	VEC3 coord;
	while(tmp = AsgnVector3D(eee = str, "Coord", &coord)){
		str = tmp;
		m_CoordList.push_back(coord);
	}
	if(m_CoordList.size()<2) throw CSynErr(eee, lang(TwoControlPointNeeded));
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	プラットフォーム親オブジェクト設定
 */
void CPlatform::SetPlatformParent(CStationPlugin *spi){
	if(!m_ParentObjectName.size()) return;
	CNamedObject *pobj = spi->FindObject(m_ParentObjectName);
	if(!pobj) throw CSynErr(m_ParentObjectScriptPos, "%s: \"%s\"", lang(UndefinedObject), m_ParentObjectName.c_str());
	m_ParentObject = pobj;
}

/*
 *	プラットフォーム用プラグインをセット
 */
void CPlatform::SetPlatformPlugin(){
	if(m_RailPluginValid) ms_PlatformRail = m_RailPlugin;
	if(m_TiePluginValid) ms_PlatformTie = m_TiePlugin;
	if(m_GirderPluginValid) ms_PlatformGirder = m_GirderPlugin;
	if(m_PierPluginValid) ms_PlatformPier = m_PierPlugin;
	if(m_LinePluginValid) ms_PlatformLine = m_LinePlugin;
	if(m_PolePluginValid) ms_PlatformPole = m_PolePlugin;
}

/*
 *	ホームのプレビュー
 */
CRailBuilder *CPlatform::SetBuilder(
	MTX4 *mtx	//	ローカル座標系
){
	SetPlatformPlugin();
	list<VEC3>::iterator iv = m_CoordList.begin();

	CRailBuilder *builder = NULL, *prev = NULL;
	for(; iv!=m_CoordList.end(); iv++){
		VEC3 p;
		if(m_ParentObject) p = *iv;
		else D3DXVec3TransformCoord(&p, &*iv, mtx);
		prev = new CRailBuilder(p, prev);
		if(!builder) builder = prev;
	}
	return builder;
}

/*
 *	ホームのプレビュー
 */
void CPlatform::Preview(
	MTX4 *mtx,			//	ローカル座標系
	CLineDumpL *dump	//	ラインダンパ
){
	CRailBuilder *builder = SetBuilder(mtx);
	int i;
	CRailBuilder::ResetDirSum();
	for(i = 0; i<m_TrackNum; i++){
		CRailBuilder::SetTrack(i, m_TrackNum, m_TrackInterval, m_LiftRailSurface);
		builder->Render(dump, ms_PlatformRail, ms_PlatformTie, ms_PlatformGirder, false);
	}
	delete builder;
}

/*
 *	ホームの設置
 */
void CPlatform::Build(
	MTX4 *mtx,			//	ローカル座標系
	CStation *station	//	親ステーション
){
	CRailBuilder *builder = SetBuilder(mtx);
	CRailBuilder::ResetDirSum();
	g_LastPole.resize(m_TrackNum);
	g_FinishPole.resize(m_TrackNum);
	g_EnableCant = m_EnableCant;
	int i;
	for(i = 0; i<m_TrackNum; i++) g_LastPole[i] = g_FinishPole[i] = CPoleLink();
	for(i = 0; i<=m_TrackNum; i++){
		if(i==m_TrackNum){
			if(!(ms_PlatformGirder && ms_PlatformGirder->IsMultiTrack())
				&& !(ms_PlatformPier && ms_PlatformPier->IsMultiTrack())
				&& !(ms_PlatformPole && ms_PlatformPole->IsMultiTrack())) break;
			g_MultiTrackDummy = true;
			g_DummyTrackNum = m_TrackNum;
			g_DummyTrackInterval = m_TrackInterval;
		}
		if(ms_PlatformRail) ms_PlatformRail->ResetMapTemp();
		if(ms_PlatformTie) ms_PlatformTie->ResetMapTemp();
		if(ms_PlatformGirder) ms_PlatformGirder->ResetMapTemp();
		if(ms_PlatformPier) ms_PlatformPier->ResetPierPos();
		if(ms_PlatformLine){
			ms_PlatformLine->ResetMapTemp();
			ms_PlatformLine->ResetPolePos();
		}
		CRailBuilder::SetTrack(i, m_TrackNum, m_TrackInterval, m_LiftRailSurface);
		if(i<m_TrackNum) g_PlatformInst =
			station->PushPlatformInst(R2L(CPlatformInst(station, m_Stoppable, m_OpenDoor)));
		if(m_ParentObject){
			CPartsInst *parts = station->FindParts(m_ParentObject);
			if(!parts) ErrorDialog("INTERNAL ERROR: PLATFORM PARENT PARTS NOT FOUND: %s", m_ParentObject->GetName());
			g_StationPlatformParentDetectInfo = CDetectInfo(m_ParentObject, station, parts);
		}
		builder->BuildRail(R2L(CRailConnectorLink()),
			R2L(CRailConnectorLink()), ms_PlatformRail, ms_PlatformTie, ms_PlatformGirder);
		g_Scene->BuildLine(ms_PlatformPier, ms_PlatformLine, ms_PlatformPole);
		if(m_ParentObject) g_StationPlatformParentDetectInfo = CDetectInfo();
		g_PlatformInst = NULL;
		g_MultiTrackDummy = false;
	}
	station->MakePlatformArray();
	delete builder;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	[static]
 *	プレビュー
 */
void CStationPlugin::RenderPreview(
	VEC3 pos,	//	位置
	VEC3 dir,	//	dir
	VEC3 up		//	up
){
	CNamedObjectAfterRenderer::SetCurrentInst(NULL);
	g_SaveFile->ResetSwitch();
	if(ms_PreviewState && g_Station){
		g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(pos, dir, up);
		SetCamDistSwitch(pos);
		g_Station->Preview();
	}
}

/*
 *	デストラクタ
 */
CStationPlugin::~CStationPlugin(){
}

/*
 *	ロード
 */
char *CStationPlugin::LoadStructBefore(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "StationInfo"))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	CPlatform platform;
	while(tmp = platform.Read(str)){
		str = tmp;
		m_Platform.push_back(platform);
	}
	ChDir();
	return str;
}

/*
 *	ロード
 */
char *CStationPlugin::LoadStructAfter(
	char *str	//	対象文字列
){
	IPlatform ip = m_Platform.begin();
	for(; ip!=m_Platform.end(); ip++) ip->SetPlatformParent(this);
	return str;
}

/*
 *	ロード
 */
bool CStationPlugin::LoadOldForm(){
	if(!ChDir()) return false;
	g_NamedObjectMipMap = g_StructMipMap;
	CNamedObject::SetCastShadowDefault(true);
	FILE *file = fopen(TextName(), "rt");
	char *dummy = FlashOut();
	float sc;
	int i, platforms = 0;
	bool needpush = true;
	fscanf(file, "%s %s %f %d", dummy, dummy, &sc, &platforms);
	float oldscale = 2.0f/sc;
	for(i = 0; i<platforms; i++){
		VEC3 v1, v2;
		if(fscanf(file, "\n(%f,%f,%f),(%f,%f,%f)", &v1.x, &v1.y, &v1.z, &v2.x, &v2.y, &v2.z)<6)
			ErrorDialog("Station <%s>\n%s", m_ID.c_str(), lang(PlatformReadError));
		v1 = oldscale*VEC3(-v1.x, v1.y, -v1.z);
		v2 = oldscale*VEC3(-v2.x, v2.y, -v2.z);
		if(needpush) m_Platform.push_back(CPlatform());
		if(v1==v2){
			(--m_Platform.end())->PushCoord(v1);
			needpush = false;
		}else{
			(--m_Platform.end())->PushCoord(v1);
			(--m_Platform.end())->PushCoord(v2);
			needpush = true;
		}
	}
	IPlatform ip = m_Platform.begin();
	for(; ip!=m_Platform.end(); ip++) if(ip->GetCoordNum()<2)
		ErrorDialog("Station <%s>\n%s", m_ID.c_str(), lang(TwoControlPointNeeded));
	fclose(file);
	m_FreeObject.push_back(CFreeObjectContainer(new CFreeObject3D("MainObject", "Model.x", oldscale)));
	m_FreeObject.begin()->LoadModel(this);
	m_PartsNum = 1;
	return true;
}

/*
 *	プレビュー設定
 */
void CStationPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Station = this;
	string desc = g_Station->GetBasicInfo();
	desc += FlashIn("\n%s: %d", lang(PlatformNum), m_Platform.size());
	desc += "\n"+g_Station->GetDescription();
	g_StationSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	プレビュー
 */
void CStationPlugin::PreviewStruct(){
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devResetMaterial();
	devResetMatrix();
	devSetLighting(FALSE);
	devTEX_POINT(0);
	devTEX_POINT(1);
	CLineDumpL dump(256);
	MTX4 mtx = g_SystemObject[SYS_OBJ_LOCAL].GetPreviewObject()->GetMatrix();
	CPlatform::ResetPlatformPlugin();
	IPlatform ip = m_Platform.begin();
	for(; ip!=m_Platform.end(); ip++) ip->Preview(&mtx, &dump);
	dump.Render(true);
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
	g_ConfigMode->SetTexFilter();
}

/*
 *	プラットフォーム建設
 */
void CStationPlugin::BuildPlatform(
	CStation *station	//	駅
){
	MTX4 mtx = g_SystemObject[SYS_OBJ_LOCAL].GetPreviewObject()->GetMatrix();
	CPlatform::ResetPlatformPlugin();
	IPlatform ip = m_Platform.begin();
	for(; ip!=m_Platform.end(); ip++) ip->Build(&mtx, station);
	g_Scene->Dump();
}
