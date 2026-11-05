#include "stdafx.h"
#include "CJobTimer.h"
#include "CRailDetectCurve.h"
#include "CRailConnector.h"
#include "CPier.h"
#include "CLine.h"
#include "CStation.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CStructPlugin.h"
#include "CSurfacePlugin.h"
#include "CRailBuildMode.h"
#include "CSceneEditMode.h"
#include "CEnvPlugin.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"

//	内部定数
const float SURFACE_PICK_OFFSET = 10.0f;	//	地形ピックオフセット

//	外部グローバル
extern CLineDumpN *g_LineDumpN;
extern COffScreen g_HidefCapture;
extern bool g_HidefCaptureFlag;
extern bool g_SelectMultiTrackDummy;

//	内部グローバル
CSurfacePlugin *g_Surface;	//	カレントサーフェイス
CEnvPlugin *g_Env;			//	カレント環境

/*
 *	コンストラクタ (読込用)
 */
CScene::CScene(){
	m_ArrowPos = V3ZERO;
	m_ListElement = NULL;
	m_Camera.Init(200.0f, 2.0f, 5000.0f, true);
	m_RailConnector = NULL;
	m_RailWay = NULL;
	m_Pier = NULL;
	m_Line = NULL;
	m_Pole = NULL;
	m_Station = NULL;
	m_Struct = NULL;
	m_SurfacePlugin = NULL;
	m_EnvPlugin = NULL;
	m_IsDumpReady = false;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CScene::CScene(
	CSurfacePlugin *spi,	//	地形プラグイン
	CEnvPlugin *epi,		//	環境プラグイン
	char *name				//	名称
):
	CStruct(spi)	//	基本クラス
{
	m_Name = name;
	m_ArrowPos = V3ZERO;
	m_ListElement = NULL;
	m_Camera.Init(200.0f, 2.0f, 5000.0f, true);
	m_RailConnector = NULL;
	m_RailWay = NULL;
	m_Pier = NULL;
	m_Line = NULL;
	m_Pole = NULL;
	m_Station = NULL;
	m_Struct = NULL;
	m_SurfacePlugin = spi;
	m_EnvPlugin = epi;
	m_Next = NULL;
	m_SurfacePlugin->SetSwitch(this);
	m_SurfacePlugin->SetPartsInst(this);
	m_SurfacePlugin->SetMoverState(this);
	SetLocalAxis();
	SetCamDistSwitch(V3ZERO);
	m_SurfacePlugin->SetPosture();
	if(PickScene(100.0f*V3UP, -V3UP, &m_ArrowPos))
		m_Camera.SetFocus(m_ArrowPos);
//	else Dialog("not hit");
}

/*
 *	デストラクタ
 */
CScene::~CScene(){
	DELETE_V(m_RailConnector);
	DELETE_V(m_RailWay);
	DELETE_V(m_Pier);
	DELETE_V(m_Line);
	DELETE_V(m_Pole);
	DELETE_V(m_Station);
	DELETE_V(m_Struct);
	DELETE_V(m_Next);
}

/*
 *	番号付きシーン名取得
 */
char *CScene::GetNumberedName(){
	return FlashIn("(%d/%d) %s", m_Serial+1, g_SaveFile->GetSceneNum(), m_Name.c_str());
}

/*
 *	季節設定
 */
void CScene::SetSeason(){
	static int season[12] = {3, 3, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3};
	int frev = g_SimulationMode->GetEarthRevolution();
	g_SystemSwitch[SYS_SW_SEASON].SetValue(frev ? frev-1
		: season[((m_EnvPlugin->GetHemisphere() ? 6 : 0)+g_SaveFile->GetMonth())%12]);
}

/*
 *	シーン適用
 */
void CScene::Enter(
	bool selectcamera	//	カメラ選択フラグ
){
	g_Scene = this;
	g_Surface = m_SurfacePlugin;
	g_Env = m_EnvPlugin;
	CSoundEffector::InitPlayList();
	CRailConnector::SetRoot(&m_RailConnector);
	CRailWay::SetRoot(&m_RailWay);
	CPier::SetRoot(&m_Pier);
	CLine::SetRoot(&m_Line);
	CPole::SetRoot(&m_Pole);
	CStruct::SetRoot(&m_Struct);
	CStation::SetRoot(&m_Station);
	m_Camera.SetFocusInfo(R2L(CDetectInfo()));
	if(selectcamera) m_Camera.Select();
	m_ArrowPos = m_Camera.GetFocus();
	if(!m_IsDumpReady) Dump();
	if(g_RailBuildMode) g_RailBuildMode->ResetBuilder();
}

/*
 *	システム座標系設定
 */
void CScene::SetGlobalAxis(){
	VEC3 ldir, lup = V3UP;
	V3Norm(&ldir, (VEC3*)&svl.dir.Direction);
	if(ldir.x==0.0f && ldir.y==0.0f) ldir.z = 1.0f;
	g_SystemObject[SYS_OBJ_CAMERA].SetPreviewPosture(GetVPos(), GetVDir(), GetVUp());
	g_SystemObject[SYS_OBJ_LIGHT].SetPreviewPosture(-ldir*1.0e8f, ldir, lup);
}

/*
 *	レールリストルート再設定
 */
void CScene::ResetRailWayRoot(){
	CRailWay::SetRoot(&m_RailWay);
}

/*
 *	レールリストルート再設定
 */
void CScene::ResetRailConnectorRoot(){
	CRailConnector::SetRoot(&m_RailConnector);
}

/*
 *	シーン削除処理
 */
void CScene::Delete(){
	while(m_Station) DeleteStation(m_Station);
	CRailConnector *con = m_RailConnector;
	while(con){
		con->RemoveGroup();
		con = con->Next();
	}
	CRailWay *way = m_RailWay;
	while(way){
		way->RemoveGroup();
		way = way->Next();
	}
}

/*
 *	編成の削除
 */
void CScene::DeleteGroup(
	CTrainGroup *group	//	編成
){
	CRailConnector *con = m_RailConnector;
	while(con){
		con->DeleteGroup(group);
		con = con->Next();
	}
	CStation *sta = m_Station;
	while(sta){
		sta->DeleteGroup(group);
		sta = sta->Next();
	}
}

/*
 *	橋脚リンクの削除
 */
void CScene::DeletePierLink(
	CPier *pier	//	橋脚
){
	CRailWay *ptr = m_RailWay;
	while(ptr){
		ptr->DeletePier(pier);
		ptr = ptr->Next();
	}
}

/*
 *	架線リンクの削除
 */
void CScene::DeletePoleLink(
	CPole *pole	//	架線柱
){
	CRailWay *ptr = m_RailWay;
	while(ptr){
		ptr->DeletePole(pole);
		ptr = ptr->Next();
	}
}

/*
 *	未使用コネクタ撤去
 */
bool CScene::DeleteRailConnector(){
	bool deleted = false;
	CRailConnector *ptr, **adr = &m_RailConnector;
	while(ptr = *adr){
		*adr = ptr->Delete();
		if(ptr==*adr) adr = ptr->NextAdr();
		else deleted = true;
	}
	return deleted;
}

/*
 *	レール撤去
 */
bool CScene::DeleteRailWay(){
	bool deleted = false;
	CRailWay *ptr = m_RailWay;
	while(ptr){
		ptr->SplitSelect();
		ptr = ptr->Next();
	}
	CRailWay **adr = &m_RailWay;
	while(ptr = *adr){
		*adr = ptr->Delete();
		if(ptr==*adr) adr = ptr->NextAdr();
		else deleted = true;
	}
	DeleteRailConnector();
	g_SaveFile->DeleteWarp();
	return deleted;
}

/*
 *	閉塞区間設定
 */
bool CScene::SetRailBlock(char *rb){
	bool changed = false;
	CRailWay *ptr = m_RailWay;
	while(ptr){
		ptr->SplitSelect();
		ptr = ptr->Next();
	}
	ptr = m_RailWay;
	while(ptr){
		changed |= ptr->SetRailBlock(rb);
		ptr = ptr->Next();
	}
	return changed;
}

/*
 *	制限速度設定
 */
bool CScene::SetSpeedLimit(int sl){
	bool changed = false;
	CRailWay *ptr = m_RailWay;
	while(ptr){
		ptr->SplitSelect();
		ptr = ptr->Next();
	}
	ptr = m_RailWay;
	while(ptr){
		changed |= ptr->SetSpeedLimit(sl);
		ptr = ptr->Next();
	}
	return changed;
}

/*
 *	橋脚撤去
 */
bool CScene::DeletePier(
	CPier *pier	//	橋脚
){
	bool deleted = false;
	CPier **adr = &m_Pier;
	while(*adr){
		CPier *ptr = *adr;
		if(pier ? ptr==pier : ptr->GetSelectFlag()&2){
			*adr = ptr->Next();
			ptr->Delete();
			deleted = true;
			continue;
		}
		adr = ptr->NextAdr();
	}
	return deleted;
}

/*
 *	架線撤去
 */
bool CScene::DeleteLine(
	CLine *line	//	架線
){
	bool deleted = false;
	CLine **adr = &m_Line;
	while(*adr){
		CLine *ptr = *adr;
		if(line ? ptr==line : ptr->GetSelectFlag()&2){
			*adr = ptr->Next();
			ptr->Delete();
			deleted = true;
			continue;
		}
		adr = ptr->NextAdr();
	}
	return deleted;
}

/*
 *	架線柱撤去
 */
bool CScene::DeletePole(
	CPole *pole	//	架線柱
){
	bool deleted = false;
	CPole **adr = &m_Pole;
	while(*adr){
		CPole *ptr = *adr;
		if(pole ? ptr==pole : ptr->GetSelectFlag()&2){
			*adr = ptr->Next();
			ptr->Delete(this);
			deleted = true;
			continue;
		}
		adr = ptr->NextAdr();
	}
	return deleted;
}

/*
 *	駅舎撤去
 */
bool CScene::DeleteStation(
	CStation *station	//	駅舎
){
	bool deleted = false;
	CStation **adr = &m_Station;
	while(*adr){
		CStation *ptr = *adr;
		if(station ? ptr==station : ptr->GetSelectFlag()&2){
			*adr = ptr->Next();
			ptr->Delete();
			deleted = true;
			continue;
		}
		adr = ptr->NextAdr();
	}
	if(deleted) Dump();
	return deleted;
}

/*
 *	施設撤去
 */
bool CScene::DeleteStruct(
	CStruct *strct	//	施設
){
	bool deleted = false;
	CStruct **adr = &m_Struct;
	while(*adr){
		CStruct *ptr = *adr;
		if(strct ? ptr==strct : ptr->GetSelectFlag()&2){
			*adr = ptr->Next();
			ptr->Delete();
			deleted = true;
			continue;
		}
		adr = ptr->NextAdr();
	}
	return deleted;
}

/*
 *	レール入力チェック
 */
void CScene::ScanInputRailWay(
	int mode,	//	モード (0: link)
	VEC3 rect1,	//	領域始点
	VEC3 rect2,	//	領域終点
	bool mtd	//	複線ダミー選択フラグ
){
	g_SelectMultiTrackDummy = mtd;
	if(mode==2) V3MinMax(rect1, rect2, &rect1, &rect2);
	CRailDetectCurve3D::ResetDetect();
	CRailDetectCurve2D::ResetDetect();
	CRailWay *way = m_RailWay;
	while(way){
		way->ScanInput(mode, rect1, rect2);
		way = way->Next();
	}
	g_SelectMultiTrackDummy = true;
}

/*
 *	コネクタ入力チェック
 */
void CScene::ScanInputRailConnector(
	int mode,	//	モード (0: link)
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	if(mode==2) V3MinMax(rect1, rect2, &rect1, &rect2);
	CRailConnector::ResetDetect();
	CRailConnector *con = m_RailConnector;
	while(con){
		con->ScanInput(mode, rect1, rect2);
		con = con->Next();
	}
}

/*
 *	橋脚入力チェック
 */
void CScene::ScanInputPier(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	if(mode==2) V3MinMax(rect1, rect2, &rect1, &rect2);
	CPier::ResetDetect();
	CPier *pier = m_Pier;
	while(pier){
		pier->ScanInput(mode, rect1, rect2);
		pier = pier->Next();
	}
}

/*
 *	架線入力チェック
 */
void CScene::ScanInputLine(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	if(mode==2) V3MinMax(rect1, rect2, &rect1, &rect2);
	CLine::ResetDetect();
	CLine *line = m_Line;
	while(line){
		line->ScanInput(mode, rect1, rect2);
		line = line->Next();
	}
}

/*
 *	架線柱入力チェック
 */
void CScene::ScanInputPole(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	if(mode==2) V3MinMax(rect1, rect2, &rect1, &rect2);
	CPole::ResetDetect();
	CPole *pole = m_Pole;
	while(pole){
		pole->ScanInput(mode, rect1, rect2);
		pole = pole->Next();
	}
}

/*
 *	駅舎入力チェック
 */
void CScene::ScanInputStation(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2,	//	領域終点
	bool init	//	初期化フラグ
){
	if(init) CModelInst::ResetDetect(mode, rect1, rect2);
	CStation *station = m_Station;
	while(station){
		station->ScanInput(mode, rect1, rect2);
		station = station->Next();
	}
}

/*
 *	施設入力チェック
 */
void CScene::ScanInputStruct(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2,	//	領域終点
	bool init	//	初期化フラグ
){
	if(init) CModelInst::ResetDetect(mode, rect1, rect2);
	CStruct *strct = m_Struct;
	while(strct){
		strct->ScanInput(mode, rect1, rect2);
		strct = strct->Next();
	}
}

/*
 *	架線検索
 */
CLine *CScene::FindLine(
	CPoleLink *from,	//	接続元
	CPoleLink *to		//	接続先
){
	CLine *line = m_Line;
	while(line){
		CPoleLink p1 = line->GetLink(0);
		CPoleLink p2 = line->GetLink(1);
		if(from->m_Link==p1.m_Link && from->m_Track==p1.m_Track
			&& to->m_Link==p2.m_Link && to->m_Track==p2.m_Track
			|| from->m_Link==p2.m_Link && from->m_Track==p2.m_Track
			&& to->m_Link==p1.m_Link && to->m_Track==p1.m_Track) return line;
		line = line->Next();
	}
	return NULL;
}

/*
 *	橋脚・架線設置
 */
void CScene::BuildLine(
	CPierPlugin *ipi,	//	橋脚プラグイン
	CLinePlugin *lpi,	//	架線プラグイン
	CPolePlugin *ppi	//	架線柱プラグイン
){
	CRailConnector *con = m_RailConnector;
	while(con){
		if(!con->Dump()) break;
		con = con->Next();
	}
	m_RailWay->BuildLine(ipi, lpi, ppi);
	if(lpi && g_MultiTrackDummy==(ppi ? ppi->IsMultiTrack() : false)){
		int i;
		for(i = 0; i<g_LastPole.size(); i++)
			if(g_LastPole[i].IsValid() && g_FinishPole[i].IsValid())
				new CLine(g_LastPole[i], g_FinishPole[i], lpi);
	}
}

/*
 *	地形ピック
 */
bool CScene::PickScene(
	VEC3 pos,	//	座標
	VEC3 dir,	//	方向
	VEC3 *hit,	//	ヒット座標格納先
	VEC3 *tri,	//	三角形頂点格納先
	int inv		//	裏面フラグ
){
	m_SurfacePlugin->SetSwitch(this);
	m_SurfacePlugin->SetPartsInst(this);
	m_SurfacePlugin->SetMoverState(this);
	return m_SurfacePlugin->PickSurface(pos, dir, hit, tri, inv);
}

/*
 *	地形ピック・高度制限
 */
bool CScene::ClipAlt(
	VEC3 *pos,	//	座標
	VEC3 *hit,	//	ヒット座標格納先
	VEC3 *norm,	//	法線格納先
	int clip	//	クリップモード (1: 地下, 2: 高架)
){
	VEC3 opos = *pos, tmp, tri[3];
	bool ret = false, ugpick = false;
	if(PickScene(opos+SURFACE_PICK_OFFSET*V3UP, -V3UP, &tmp, tri, 1)){
		ret = true;
		if((clip&2) && opos.y+SURFACE_PICK_OFFSET>tmp.y
			&& ((clip&1) || pos->y>tmp.y)){
			ugpick = true;
			pos->y = tmp.y;
		}
		if(hit) *hit = tmp;
		if(norm) *norm = CalcPlaneNormal(tri[0], tri[1], tri[2]);
	}
	if(PickScene(opos-SURFACE_PICK_OFFSET*V3UP, V3UP, &tmp, tri, 2)){
		ret = true;
		if((clip&1) && opos.y-SURFACE_PICK_OFFSET<tmp.y
			&& ((clip&2) || pos->y<tmp.y) && (!ugpick || pos->y<tmp.y)) pos->y = tmp.y;
		if(hit) *hit = tmp;
		if(norm) *norm = CalcPlaneNormal(tri[0], tri[1], tri[2]);
	}
	return ret;
}

/*
 *	各種インスタンスをダンプ
 */
void CScene::Dump(){
	g_RailPluginList->ClearDump();
	g_TiePluginList->ClearDump();
	g_GirderPluginList->ClearDump();
	g_PierPluginList->ClearDump();
	g_LinePluginList->ClearDump();
	CRailWay *way = m_RailWay;
	while(way){
		way->Dump();
		way = way->Next();
	}
	CPier *pier = m_Pier;
	while(pier){
		pier->Dump();
		pier = pier->Next();
	}
	CLine *line = m_Line;
	while(line){
		line->Dump();
		line = line->Next();
	}
	g_RailPluginList->PrepareVertex();
	g_TiePluginList->PrepareVertex();
	g_GirderPluginList->PrepareVertex();
	g_PierPluginList->PrepareVertex();
	g_LinePluginList->PrepareVertex();
	m_IsDumpReady = true;
}

/*
 *	レンダリング
 */
void CScene::RenderScene(){
	TIMER_RAII("CScene::RenderScene()");
	SetSeason();
	SetGlobalAxis();
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(m_Serial);
	if(m_EnvPlugin){
		m_EnvPlugin->Render();
	}else{
		devSetZRead(TRUE);
		devSetZWrite(TRUE);
		devSetLighting(TRUE);
		SetDirLight(VEC3(-1.0f, -2.0f, -1.0f), MAKE_CV(1.0f, 1.0f, 1.0f, 1.0f));
		if(g_HidefCaptureFlag) g_HidefCapture.Begin();
		else BeginScene();
	}
	Render();
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	{
		TIMER_RAII("profile RenderAll()");
		g_RailPluginList->RenderAll();
		g_TiePluginList->RenderAll();
		g_GirderPluginList->RenderAll();
		g_PierPluginList->RenderAll();
		g_LinePluginList->RenderAll();
	}
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	devSetLighting(TRUE);
	{
		TIMER_RAII("railway");
		CRailWay *way = m_RailWay;
		while(way){
			way->Render();
			way = way->Next();
		}
	}
	{
		TIMER_RAII("pier");
		CPier *pier = m_Pier;
		while(pier){
			pier->Render();
			pier = pier->Next();
		}
	}
	{
		TIMER_RAII("line");
		CLine *line = m_Line;
		while(line){
			line->Render();
			line = line->Next();
		}
	}
	{
		TIMER_RAII("pole");
		CPole *pole = m_Pole;
		while(pole){
			pole->Render();
			pole = pole->Next();
		}
	}
	{
		TIMER_RAII("station");
		CStation *station = m_Station;
		while(station){
			station->Render();
			station = station->Next();
		}
	}
	{
		TIMER_RAII("struct");
		CStruct *strct = m_Struct;
		while(strct){
			strct->Render();
			strct = strct->Next();
		}
	}
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
}

/*
 *	レンダリング仕上げ
 */
void CScene::RenderAfter(){
	if(m_EnvPlugin) m_EnvPlugin->RenderAfter();
}

/*
 *	シミュレート進行
 */
void CScene::SimulateScene(){
	SetSeason();
	SetGlobalAxis();
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(m_Serial);
	CStation *station = m_Station;
	while(station){
		station->Simulate();
		station = station->Next();
	}
	CStruct *strct = m_Struct;
	while(strct){
		strct->Simulate();
		strct = strct->Next();
	}
	Simulate();
}

/*
 *	アドレス復元
 */
void CScene::RestoreAddress(){
	CRailConnector *con = m_RailConnector;
	while(con){
		con->RestoreAddress();
		con = con->Next();
	}
	CRailWay *way = m_RailWay;
	while(way){
		way->RestoreAddress();
		way = way->Next();
	}
//	CPier *pier = m_Pier;
//	while(pier){
//		pier->RestoreAddress();
//		pier = pier->Next();
//	}
	CLine *line = m_Line;
	while(line){
		line->RestoreAddress();
		line = line->Next();
	}
	CPole *pole = m_Pole;
	while(pole){
		pole->RestoreAddress();
		pole = pole->Next();
	}
	CStation *station = m_Station;
	while(station){
		station->RestoreAddress();
		station = station->Next();
	}
//	CStruct *strct = m_Struct;
//	while(strct){
//		strct->RestoreAddress();
//		strct = strct->Next();
//	}
}

/*
 *	読込
 */
char *CScene::Read(
	char *str,		//	対象文字列
	CScene ***root	//	格納先
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "Scene"))){
		delete this;
		return NULL;
	}
	m_Scene = NULL;
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "Name", &m_Name))) throw CSynErr(eee);
	m_Name = RestoreDoubleQuote(m_Name);
	if(!(str = AsgnString(eee = str, "SurfacePlugin", &pid))) throw CSynErr(eee);
	m_ModelPlugin = m_StructPlugin =
		m_SurfacePlugin = g_SurfacePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnString(eee = str, "EnvPlugin", &pid))) throw CSynErr(eee);
	m_EnvPlugin = g_EnvPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = ReadModelInst(eee = str, false))) throw CSynErr(eee);
	if(!(str = m_Camera.Read(eee = str))) throw CSynErr(eee);

	if(!(str = BeginBlock(str, "RailConnectorList"))) throw CSynErr(eee);
	CRailConnector::SetRoot(&m_RailConnector);
	while(true){
		if(tmp = (new CRailConnector)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "RailWayList"))) throw CSynErr(eee);
	CRailWay::SetRoot(&m_RailWay);
	while(true){
		if(tmp = (new CRailWay)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "PierList"))) throw CSynErr(eee);
	CPier::SetRoot(&m_Pier);
	while(true){
		if(tmp = (new CPier)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "LineList"))) throw CSynErr(eee);
	CLine::SetRoot(&m_Line);
	while(true){
		if(tmp = (new CLine)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "PoleList"))) throw CSynErr(eee);
	CPole::SetRoot(&m_Pole);
	while(true){
		if(tmp = (new CPole)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "StationList"))) throw CSynErr(eee);
	CStation::SetRoot(&m_Station);
	while(true){
		if(tmp = (new CStation)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = BeginBlock(str, "StructList"))) throw CSynErr(eee);
	CStruct::SetRoot(&m_Struct);
	while(true){
		if(tmp = (new CStruct)->Read(str)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	**root = this;
	*root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CScene::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\tScene{\n");
	fprintf(df, "\t\tAddress = %p;\n", this);
	fprintf(df, "\t\tName = \"%s\";\n", ExpandDoubleQuote(m_Name).c_str());
	fprintf(df, "\t\tSurfacePlugin = \"%s\";\n", CheckPluginID(m_SurfacePlugin));
	fprintf(df, "\t\tEnvPlugin = \"%s\";\n", CheckPluginID(m_EnvPlugin));
	SaveModelInst(df, "\t\t", false);
	fprintf(df, "\n");
	m_Camera.Save(df);
	fprintf(df, "\n");

	fprintf(df, "\t\tRailConnectorList{\n");
	CRailConnector *con = m_RailConnector;
	while(con){
		con->Save(df);
		con = con->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tRailWayList{\n");
	CRailWay *way = m_RailWay;
	while(way){
		way->Save(df);
		way = way->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tPierList{\n");
	CPier *pier = m_Pier;
	while(pier){
		pier->Save(df);
		pier = pier->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tLineList{\n");
	CLine *line = m_Line;
	while(line){
		line->Save(df);
		line = line->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tPoleList{\n");
	CPole *pole = m_Pole;
	while(pole){
		pole->Save(df);
		pole = pole->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tStationList{\n");
	CStation *station = m_Station;
	while(station){
		station->Save(df);
		station = station->Next();
	}
	fprintf(df, "\t\t}\n\n");

	fprintf(df, "\t\tStructList{\n");
	CStruct *strct = m_Struct;
	while(strct){
		strct->Save(df);
		strct = strct->Next();
	}
	fprintf(df, "\t\t}\n");

	fprintf(df, "\t}\n");
}
