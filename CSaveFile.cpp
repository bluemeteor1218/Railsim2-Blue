#include "stdafx.h"
#include "md5.h"
#include "RailMap.h"
#include "Network.h"
#include "CJobTimer.h"
#include "CListView.h"
#include "CSimpleDialog.h"
#include "CRailWay.h"
#include "CTrainGroup.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CDiaInst.h"
#include "CModelInst.h"
#include "CSurfacePlugin.h"
#include "CEnvPlugin.h"
#include "CSkinPlugin.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"
#include "CNeutralMode.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"

//	関数宣言
bool IsLeapYear(int);
int GetDaysPerMonth(int, int);

//	外部定数
extern const char *LAYOUT_DIRNAME;

//	内部定数
const float WIND_MAX = 0.5f;	//	最大風速

//	外部グローバル
extern CTrain *g_CabinViewTrain;
extern char *YESNO[];

//	内部グローバル
char *g_DayOfWeek[7];	//	曜日
char *g_TrainSetString[2] = {lang(NotSet), lang(Set)};	//	編成設置状態
CScene *g_Scene;					//	カレントシーン
CTrainGroup *g_TrainGroup;			//	カレント編成
map<void *, void *> g_AddressMap;	//	アドレス変換
set<string> g_LackPlugin;			//	不足プラグイン
//int g_GroupEndCount = 0;			//	debug

/*
 *	コンストラクタ
 */
CSaveFile::CSaveFile(
	bool preset	//	プリセット編成・シーン準備
){
	m_NetworkSyncCount = 0;
	m_Year = m_Month = m_Day = m_Hour = m_Minute
		= m_Second = m_Frame = m_DayOfWeek = m_SumDays = 0;
	for(; m_Month<3; m_Month++) m_SumDays += GetDaysPerMonth(0, m_Month);
	m_Hour = 12;
	m_GroupNum = m_SceneNum = 1;
	m_WarpList = NULL;
	if(preset){
		g_TrainGroup = m_GroupList = new CTrainGroup(lang(InitialConsist));
		g_Scene = m_SceneList = new CScene(g_DefaultSurface, g_DefaultEnv, lang(InitialScene));
	}else{
		g_TrainGroup = m_GroupList = NULL;
		g_Scene = m_SceneList = NULL;
	}
	ClearRailBlockUser(NULL);
	NumberGroup();
	NumberScene();
	UpdateWind();
	UpdateWind();
	CParticle::InitRenderList();
	if(g_Scene) g_Scene->Enter(false);
}

/*
 *	デストラクタ
 */
CSaveFile::~CSaveFile(){
	g_Scene = NULL;
	if(g_ConfigMode) g_ConfigMode->FreeWindowDiv();
	DELETE_V(m_WarpList);
	DELETE_V(m_GroupList);
	DELETE_V(m_SceneList);
}

/*
 *	風方向の更新
 */
void CSaveFile::UpdateWind(){
	float theta = FRand(2.0f*D3DX_PI);
	float speed = FRand(WIND_MAX)*FRand(WIND_MAX)/WIND_MAX;
	m_WindDir1 = m_WindDir2;
	m_WindDir2 = speed*VEC3(sinf(theta), 0.0f, cosf(theta));
	m_WindCount = 0;
	m_WindTime = Rand2(12*60*60*MAXFPS, 48*60*60*MAXFPS);
}

/*
 *	ワープリストルート設定
 */
void CSaveFile::SetWarpRoot(){
	CRailWay::SetRoot(&m_WarpList);
}

/*
 *	閉塞区間設定
 */
/*bool CSaveFile::SetRailBlock(char *rb){
	bool changed = false;
	CScene *ptr = m_SceneList;
	while(ptr){
		changed |= ptr->SetRailBlock(rb);
		ptr = ptr->Next();
	}
	return changed;
}*/

/*
 *	ワープ削除
 */
bool CSaveFile::DeleteWarp(){
	bool deleted = false;
	CRailWay *ptr, **adr = &m_WarpList;
	while(ptr = *adr){
		*adr = ptr->Delete();
		if(ptr==*adr) adr = ptr->NextAdr();
		else deleted = true;
	}
	return deleted;
}

/*
 *	編成追加
 */
void CSaveFile::AddGroup(){
	m_GroupNum++;
	CTrainGroup **adr = &m_GroupList;
	while(*adr) adr = (*adr)->NextAdr();
	*adr = new CTrainGroup(lang(NewConsist));
	NumberGroup();
}

/*
 *	編成削除
 */
void CSaveFile::DeleteGroup(
	CTrainGroup *gr	//	編成
){
	CTrainGroup **adr = &m_GroupList;
	g_TrainGroup = NULL;
	while(*adr){
		if(*adr==gr){
			CScene *scene = m_SceneList;
			while(scene){
				scene->DeleteGroup(gr);
				scene = scene->Next();
			}
			CTrainGroup *tmp = *adr;
			*adr = tmp->Next();
			*tmp->NextAdr() = NULL;
			delete tmp;
			if(!g_TrainGroup) g_TrainGroup = *adr;
			m_GroupNum--;
			break;
		}
		g_TrainGroup = *adr;
		adr = (*adr)->NextAdr();
	}
	NumberGroup();
}

/*
 *	編成リスト作成
 */
void CSaveFile::ListGroup(
	CListView *lv	//	リストビュー
){
	char *train_set_string[2] = {lang(NotSet), lang(Set)};	//	編成設置状態
	CTrainGroup *ptr = m_GroupList;
	lv->DeleteAllItems();
	int i = 0;
	while(ptr){
		CListElement *le = lv->InsertItem(-1, ptr->GetName());
		le->SetString(1, FlashIn("%d", ptr->GetTrainNum()));
		le->SetString(2, train_set_string[ptr->IsSet()]);
		le->SetData((DWORD)ptr);
		ptr->SetListElement(le);
		if(ptr==g_TrainGroup) lv->SetSelectionMark(i, 0);
		i++;
		ptr = ptr->Next();
	}
	lv->EnsureVisible(lv->GetSelectionMark());
}

/*
 *	編成リスト作成 (ダイヤ設定用)
 */
void CSaveFile::ListGroupDia(
	CListView *lv,		//	リストビュー
	CDiaInstBase *dinst	//	ダイヤ
){
	CTrainGroup *ptr = m_GroupList;
	lv->DeleteAllItems();
	int i = 0;
	while(ptr){
		CListElement *le = lv->InsertItem(-1, ptr->GetName());
		le->SetString(1, FlashIn("%d", ptr->GetTrainNum()));
		le->SetString(2, g_DiaDefaultString[dinst->IsDefault(ptr)]);
		le->SetData((DWORD)ptr);
		ptr->SetListElement(le);
		if(ptr==g_TrainGroup) lv->SetSelectionMark(i, 0);
		i++;
		ptr = ptr->Next();
	}
}

/*
 *	プラットフォーム削除
 */
void CSaveFile::DeletePlatform(
	CPlatformInst *pf	//	プラットフォーム
){
	CTrainGroup *ptr = m_GroupList;
	while(ptr){
		ptr->DeletePlatform(pf);
		ptr = ptr->Next();
	}
}

/*
 *	編成番号を付ける
 */
void CSaveFile::NumberGroup(){
	int i = 0;
	m_GroupNum = 0;
	CTrainGroup *ptr = m_GroupList;
	while(ptr){
		m_GroupNum++;
		ptr->SetSerial(i++);
		ptr = ptr->Next();
	}
}

/*
 *	次の編成を選択
 */
void CSaveFile::NextGroup(
	bool prev	//	逆方向
){
	if(prev){
		CTrainGroup *ptr = m_GroupList;
		while(ptr){
			if(!ptr->Next() || ptr->Next()==g_TrainGroup){
				g_TrainGroup = ptr;
				break;
			}
			ptr = ptr->Next();
		}
	}else{
		g_TrainGroup = g_TrainGroup && g_TrainGroup->Next()
			? g_TrainGroup->Next() : m_GroupList;
	}
}

/*
 *	順序取得
 */
vector<CTrainGroup *> CSaveFile::GetTrainGroupByVector(){
	vector<CTrainGroup *> group_list;
	CTrainGroup *ptr = m_GroupList;
	while(ptr){
		group_list.push_back(ptr);
		ptr = ptr->Next();
	}
	return group_list;
}

/*
 *	編成順序変更
 */
void CSaveFile::SetTrainGroupByVector(vector<CTrainGroup *> group_list){
	int i;
	m_GroupList = group_list.size() ? group_list[0] : NULL;
	for(i = 0; i<group_list.size(); ++i)
		*group_list[i]->NextAdr() = i<group_list.size()-1 ? group_list[i+1] : NULL;
	NumberGroup();
}

/*
 *	ワープ入力チェック
 */
void CSaveFile::ScanInputWarp(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	CRailWay::ResetDetect();
	CRailWay *warp = m_WarpList;
	while(warp){
		warp->ScanInputWarp(mode, rect1, rect2);
		warp = warp->Next();
	}
}

/*
 *	インスタンス入力チェック
 */
void CSaveFile::ScanInputInst(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	CModelInst::ResetDetect(mode, rect1, rect2);
	CTrainGroup *group = m_GroupList;
	while(group){
		group->ScanInput(mode, rect1, rect2);
		group = group->Next();
	}
	g_Scene->ScanInputStation(mode, rect1, rect2);
	g_Scene->ScanInputStruct(mode, rect1, rect2);
}

/*
 *	シーン追加
 */
void CSaveFile::AddScene(
	CSurfacePlugin *spi	//	地形プラグイン
){
	m_SceneNum++;
	CScene **adr = &m_SceneList;
	while(*adr) adr = (*adr)->NextAdr();
	(*adr = new CScene(spi, g_DefaultEnv, lang(NewScene)))->Enter(false);
	NumberScene();
}

/*
 *	シーン削除
 */
void CSaveFile::DeleteScene(
	CScene *sc	//	シーン
){
	sc->Delete();
	CRailWay *warp = m_WarpList;
	while(warp){
		warp->CheckWarpEndScene(sc);
		warp = warp->Next();
	}
	DeleteWarp();
	CScene **adr = &m_SceneList;
	g_Scene = sc;
	g_RailPluginList->ClearDumpAll();
	g_TiePluginList->ClearDumpAll();
	g_GirderPluginList->ClearDumpAll();
	g_PierPluginList->ClearDumpAll();
	g_LinePluginList->ClearDumpAll();
	g_Scene = NULL;
	while(*adr){
		if(*adr==sc){
			CScene *tmp = *adr;
			*adr = tmp->Next();
			*tmp->NextAdr() = NULL;
			delete tmp;
			if(!g_Scene) g_Scene = *adr;
			m_SceneNum--;
			break;
		}
		g_Scene = *adr;
		adr = (*adr)->NextAdr();
	}
	if(!m_SceneList){
		g_Scene = m_SceneList = new CScene(g_DefaultSurface, g_DefaultEnv, lang(InitialScene));
		m_SceneNum = 1;
	}
	CScene *ptr = m_SceneList;
	while(ptr){
		ptr->SetDumpReady(false);
		ptr = ptr->Next();
	}
	NumberScene();
	g_Scene->Enter(false);
	g_ConfigMode->GetRootWindow()->OnDeleteScene(sc);
}

/*
 *	シーンリスト作成
 */
void CSaveFile::ListScene(
	CListView *lv	//	リストビュー
){
	CScene *ptr = m_SceneList;
	lv->DeleteAllItems();
	int i = 0;
	while(ptr){
		CListElement *le = lv->InsertItem(-1, ptr->GetName());
		le->SetString(1, ptr->GetSurface()->GetName());
		le->SetData((DWORD)ptr);
		ptr->SetListElement(le);
		if(ptr==g_Scene) lv->SetSelectionMark(i, 0);
		i++;
		ptr = ptr->Next();
	}
}

/*
 *	シーン番号を付ける
 */
void CSaveFile::NumberScene(){
	int i = 0;
	m_SceneNum = 0;
	CScene *ptr = m_SceneList;
	while(ptr){
		m_SceneNum++;
		ptr->SetSerial(i++);
		ptr = ptr->Next();
	}
}

/*
 *	次のシーンを選択
 */
void CSaveFile::NextScene(
	bool prev	//	逆方向
){
	CWindowInfo* active_wnd = g_ConfigMode->GetActiveWindow();
	CScene *origscene = g_Scene;
	if(active_wnd && active_wnd->GetScene()) g_Scene = active_wnd->GetScene();
	CScene *oldscene = g_Scene;
	if(prev){
		CScene *ptr = m_SceneList;
		while(ptr){
			if(!ptr->Next() || ptr->Next()==g_Scene){
				g_Scene = ptr;
				break;
			}
			ptr = ptr->Next();
		}
	}else{
		g_Scene = g_Scene && g_Scene->Next() ? g_Scene->Next() : m_SceneList;
	}
	if(g_Scene!=oldscene) g_Scene->Enter(true);
	if(active_wnd){
		active_wnd->SetScene(g_Scene);
		g_Scene = origscene;
	}
}

/*
 *	順序取得
 */
vector<CScene *> CSaveFile::GetSceneByVector(){
	vector<CScene *> scene_list;
	CScene *ptr = m_SceneList;
	while(ptr){
		scene_list.push_back(ptr);
		ptr = ptr->Next();
	}
	return scene_list;
}

/*
 *	編成順序変更
 */
void CSaveFile::SetSceneByVector(vector<CScene *> scene_list){
	int i;
	m_SceneList = scene_list.size() ? scene_list[0] : NULL;
	for(i = 0; i<scene_list.size(); ++i)
		*scene_list[i]->NextAdr() = i<scene_list.size()-1 ? scene_list[i+1] : NULL;
	NumberScene();
}

/*
 *	レンダリング
 */
void CSaveFile::RenderScene(
	int option	//	1: renderwarp
){
	TIMER_RAII("CSaveFile::RenderScene()");
	InitShadow();
	InitRailMap();
	CNamedObject::InitAfterRenderList();
	CHeadlight::InitRenderList();
	ResetSwitch();
	g_Scene->RenderScene();
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	{
		TIMER_RAII("train group");
		CTrainGroup *group = m_GroupList;
		while(group){
			group->Render();
			group = group->Next();
		}
	}
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	RenderShadow();
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	CNamedObject::AfterRenderAll();
	devSetState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	devSetState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	{
		TIMER_RAII("effect");
		CParticle::RenderAll();
		CHeadlight::RenderAll();
	}
	g_Scene->RenderAfter();
	devResetMatrix();
	devSetTexture(0, NULL);
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	if(option&1){
		CRailWay *warp = m_WarpList;
		while(warp){
			warp->RenderWarp();
			warp = warp->Next();
		}
	}
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);

	//SetWindowText(svw.hWnd, FlashIn("g_GroupEndCount = %d", g_GroupEndCount));
	//g_GroupEndCount = 0;
}

/*
 *	シミュレーション進行
 */
void CSaveFile::Simulate(
	int num	//	回数
){
	TIMER_RAII("CSaveFile::Simulate()");
	int scale = g_SimulationMode->GetTimeScale();
	int speed = num<0 ? g_SimulationMode->GetSimSpeed() : num;
	int i;
	bool exceed = true;
	void UpdateSyncLimit();
	UpdateSyncLimit();
	if(!speed && g_NetworkInitialized
		&& m_NetworkSyncCount<g_NetworkSyncLimitReceived
		&& m_NetworkSyncCount<g_NetworkSyncLimitSent){
		exceed = false;
		speed = g_SimulationMode->GetOldSpeed();
	}
	for(i = 0; i<speed; i++){
		TIMER_RAII("single simulation");
		if(g_NetworkInitialized){
			if(exceed) ExceedNetworkSyncLimit(m_NetworkSyncCount);
			if(m_NetworkSyncCount){
				if(m_NetworkSyncCount>=g_NetworkSyncLimitReceived
					|| m_NetworkSyncCount>=g_NetworkSyncLimitSent) return;
			}
			void SyncTrainControls(int);
			SyncTrainControls(m_NetworkSyncCount);
		}
		m_NetworkSyncCount++;
		//SetWindowText(svw.hWnd, FlashIn("net sync = %d", m_NetworkSyncCount));

		int dpm = GetDaysPerMonth(m_Year, m_Month);
		m_Frame += scale;
		m_Second += m_Frame/30;
		m_Frame %= 30;
		m_Minute += m_Second/60;
		m_Second %= 60;
		m_Hour += m_Minute/60;
		m_Minute %= 60;
		m_Day += m_Hour/24;
		m_SumDays += m_Hour/24;
		m_DayOfWeek += m_Hour/24;
		m_Hour %= 24;
		m_Month += m_Day/dpm;
		m_Day %= dpm;
		m_DayOfWeek %= 7;
		m_Year += m_Month/12;
		m_Month %= 12;

		float wtmp = (float)m_WindCount/m_WindTime;
		g_WindDir = (1.0f-wtmp)*m_WindDir1+wtmp*m_WindDir2;
		if(V3Len(&g_WindDir)) V3Norm(&g_WindDirNorm, &g_WindDir);
		if((m_WindCount += scale)>=m_WindTime) UpdateWind();

		ResetSwitch();

		CParticle::SimulateAll();
		CScene *scene = m_SceneList;
		while(scene){
			scene->SimulateScene();
			scene = scene->Next();
		}
		CTrainGroup *group = m_GroupList;
		while(group){
			group->Simulate();
			group = group->Next();
		}
		g_CabinViewTrain = NULL;
	}
}

/*
 *	スイッチリセット
 */
void CSaveFile::ResetSwitch(){
	g_SystemSwitch[SYS_SW_FRONT].SetValue(0);
	g_SystemSwitch[SYS_SW_CONNECT1].SetValue(0);
	g_SystemSwitch[SYS_SW_CONNECT2].SetValue(0);
	g_SystemSwitch[SYS_SW_DOOR1].SetValue(0);
	g_SystemSwitch[SYS_SW_DOOR2].SetValue(0);
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(0);
	g_SystemSwitch[SYS_SW_CAMDIST].SetValue(0);
	g_SystemSwitch[SYS_SW_VELOCITY].SetValue(0);
	g_SystemSwitch[SYS_SW_ACCEL].SetValue(0);
	g_SystemSwitch[SYS_SW_CABINVIEW].SetValue(0);
	g_SystemSwitch[SYS_SW_APPROACH1].SetValue(0);
	g_SystemSwitch[SYS_SW_APPROACH2].SetValue(0);
	g_SystemSwitch[SYS_SW_STOPPING].SetValue(0);
	if(g_RSPV){
		g_SystemSwitch[SYS_SW_YEAR].SetValue(1);
		g_SystemSwitch[SYS_SW_MONTH].SetValue(1);
		g_SystemSwitch[SYS_SW_DAY].SetValue(1);
		g_SystemSwitch[SYS_SW_DAYOFWEEK].SetValue(0);
		g_SystemSwitch[SYS_SW_HOUR].SetValue(0);
		g_SystemSwitch[SYS_SW_MINUTE].SetValue(0);
		g_SystemSwitch[SYS_SW_SECOND].SetValue(0);
	}else{
		g_SystemSwitch[SYS_SW_YEAR].SetValue(m_Year+1);
		g_SystemSwitch[SYS_SW_MONTH].SetValue(m_Month+1);
		g_SystemSwitch[SYS_SW_DAY].SetValue(m_Day+1);
		g_SystemSwitch[SYS_SW_DAYOFWEEK].SetValue(m_DayOfWeek);
		g_SystemSwitch[SYS_SW_HOUR].SetValue(m_Hour);
		g_SystemSwitch[SYS_SW_MINUTE].SetValue(m_Minute);
		g_SystemSwitch[SYS_SW_SECOND].SetValue(m_Second);
	}
}

/*
 *	日時文字列の取得
 */
char *CSaveFile::GetTimeText(){
	return FlashIn("%02d/%02d/%02d (%s) %02d:%02d",
		m_Year+1, m_Month+1, m_Day+1, g_DayOfWeek[m_DayOfWeek], m_Hour, m_Minute);
}

/*
 *	読込
 */
bool CSaveFile::Load(
	const char *fname,		//	ファイル名
	const char *dirname,	//	ディレクトリ名
	bool warn,				//	開けない場合警告
	bool upname,			//	ファイル名更新
	char **copy,			//	データコピー先
	int *copysize,			//	コピーサイズ
	bool checkhash,			//	ハッシュチェック要求
	char *auxdata			//	外部入力
){
	FILE *df;
	if(!auxdata){
		if(chdir(g_BaseDir) || chdir(dirname) || !(df = fopen(fname, "rb"))){
			if(warn){
				EnqueueCommonDialog(new CSimpleDialog(lang(CannotOpenFile), (char *)fname));
				g_Skin->Error();
			}
			return false;
		}
	}
	void ClearStaticSwitchTable();
	ClearStaticSwitchTable();
	g_Scene = NULL;
	g_ConfigMode->FreeWindowDiv();
	g_TrainGroup = NULL;
	g_AddressMap.clear();
	g_AddressMap[NULL] = NULL;
	g_LackPlugin.clear();
	if(upname) m_FileName = fname;
	CScene *currentscene;
	char *buf;
	if(auxdata){
		int copysize = strlen(auxdata);
		buf = new char[copysize+1];
		memcpy(buf, auxdata, copysize);
		buf[copysize] = 0;
	}else{
		buf = LoadBinaryText(df);
	}
	char *str = buf, *eee, *tmp;
	if(copy){
		int tmp = strlen(buf);
		*copy = new char[tmp+1];
		*copysize = tmp;
		memcpy(*copy, buf, tmp);
		(*copy)[tmp] = 0;
	}
	if(checkhash){
		MD5 hash;
		int tmp = strlen(buf);
		hash.update((unsigned char *)buf, tmp);
		hash.finalize();
		CheckLayoutDigest(hash.raw_digest());
	}
	try{
		if(!(str = Space(eee = str))) throw CSynErr(eee);
		if(!(str = BeginBlock(eee = str, "DatafileHeader"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "RailSimVersion", &m_Version))) throw CSynErr(eee);
		if(m_Version<2.00f) throw CSynErr(eee, "%s: %.2f", lang(InvalidVersion), m_Version);
		if(RAILSIM_VERSION<m_Version) throw CSynErr(eee, "%s: %.2f", lang(UnsupportedVersion), m_Version);
		string datafiletype;
		if(!(str = AsgnIdentifier(eee = str, "DatafileType", &datafiletype))) throw CSynErr(eee);
		if(datafiletype!=LAYOUT_DIRNAME) throw CSynErr(eee, lang(InvalidDatafileType));
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "LayoutInfo"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Date", &m_FileDate))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "Note", &m_FileNote))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Time"))) throw CSynErr(eee);
		if(!(str = Assignment(eee = str, "Date"))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Year))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Month))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Day))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_DayOfWeek))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
		if(!(str = Assignment(eee = str, "Time"))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Hour))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Minute))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Second))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstInteger(eee = str, &m_Frame))) throw CSynErr(eee);
		if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "SumDays", &m_SumDays))) throw CSynErr(eee);
		int tsc, ssp, ert, erv;
		if(!(str = AsgnInteger(eee = str, "TimeScale", &tsc))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "SimulationSpeed", &ssp))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "EarthRotation", &ert))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "EarthRevolution", &erv))) throw CSynErr(eee);
		g_SimulationMode->m_TimeScale[tsc].SetCheck();
		g_SimulationMode->m_SimSpeed[ssp].SetCheck();
		g_SimulationMode->m_EarthRotation[ert].SetCheck();
		g_SimulationMode->m_EarthRevolution[erv].SetCheck();
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		bool man_ctrl = 0, ign_acc = 0;
		if(m_Version>=2.11f){
			if(!(str = BeginBlock(eee = str, "Simulation"))) throw CSynErr(eee);
			if(!(str = AsgnYesNo(eee = str, "ManualControl", &man_ctrl))) throw CSynErr(eee);
			if(!(str = AsgnYesNo(eee = str, "IgnoreAcceleration", &ign_acc))) throw CSynErr(eee);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}
		g_SimulationMode->m_ManualControl.SetCheck(man_ctrl);
		g_SimulationMode->m_IgnoreAcceleration.SetCheck(ign_acc);

		if(!(str = BeginBlock(eee = str, "Wind"))) throw CSynErr(eee);
		if(!(str = AsgnVector3D(eee = str, "Dir1", &m_WindDir1))) throw CSynErr(eee);
		if(!(str = AsgnVector3D(eee = str, "Dir2", &m_WindDir1))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "Count", &m_WindCount))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "Time", &m_WindTime))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "WarpList"))) throw CSynErr(eee);
		CRailWay::SetRoot(&m_WarpList);
		while(true){
			if(tmp = (new CRailWay)->ReadWarp(str)) str = tmp;
			else break;
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		ClearRailBlockUser(NULL);
		if(tmp = BeginBlock(str, "RailBlockList")){
			str = tmp;
			while(tmp = Assignment(str, "RailBlock")){
				str = tmp;
				std::string rb_name;
				CTrainGroup *rb_group = NULL;
				if(!(str = StringLiteral(str, &rb_name))) throw CSynErr(eee);
				if(!(str = Character2(str, ','))) throw CSynErr(eee);
				if(!(str = HexPointer(str, (void **)&rb_group))) throw CSynErr(eee);
				g_RailBlockMap[RestoreDoubleQuote(rb_name)] = rb_group;
				if(!(str = Character2(str, ';'))) throw CSynErr(eee);
			}
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}

		if(!(str = BeginBlock(eee = str, "TrainGroupList"))) throw CSynErr(eee);
		CTrainGroup **gadr = &m_GroupList;
		while(true){
			if(tmp = (new CTrainGroup(""))->Read(str, &gadr)) str = tmp;
			else break;
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "SceneList"))) throw CSynErr(eee);
		if(!(str = AsgnPointer(eee = str, "CurrentScene", (void **)&currentscene))) throw CSynErr(eee);
		CScene **sadr = &m_SceneList;
		while(true){
			g_Scene = new CScene();
			if(tmp = g_Scene->Read(str, &sadr)) str = tmp;
			else break;
		}
		g_Scene = NULL;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(m_Version>=2.14f){
			if(!(str = BeginBlock(eee = str, "Window"))) throw CSynErr(eee);
			str = g_ConfigMode->GetRootWindow()->Read(str);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}
	}
	catch(CSynErr err){
		err.Handle(FlashIn("%s <%s>", m_FileName.c_str(), "Layout"), buf);
		return false;
	}
	DELETE_A(buf);
	CRailWay *warp = m_WarpList;
	while(warp){
		warp->RestoreAddress();
		warp = warp->Next();
	}
	map<std::string, CTrainGroup *>::iterator railblock;
	for(railblock = g_RailBlockMap.begin(); railblock!=g_RailBlockMap.end(); ++railblock){
		railblock->second = (CTrainGroup *)ReplaceAdr(railblock->second);;
	}
	CTrainGroup *group = m_GroupList;
	while(group){
		group->RestoreAddress();
		group = group->Next();
	}
	CScene *scene = m_SceneList;
	while(scene){
		scene->RestoreAddress();
		scene = scene->Next();
	}
	if(g_LackPlugin.size()){
		chdir(g_BaseDir);
		FILE *lacklog = fopen("LackPlugin.txt", "wt");
		if(lacklog){
			set<string>::iterator is = g_LackPlugin.begin();
			for(; is!=g_LackPlugin.end(); is++) fprintf(lacklog, "%s\n", is->c_str());
			fclose(lacklog);
			ErrorDialog("%s\n%s", lang(LackedPluginMessage), lang(LackedPluginListSaved));
		}else{
			ErrorDialog("%s\n%s", lang(LackedPluginMessage), lang(LackedPluginListFailed));
		}
	}
	g_TrainGroup = m_GroupList;
	g_Scene = (CScene *)ReplaceAdr(currentscene);
	g_Scene->Enter(true);
	//g_AddressMap.clear();
	NumberGroup();
	NumberScene();
	group = m_GroupList;
	while(group){
		group->RestoreSet();
		group = group->Next();
	}
	scene = m_SceneList;
	while(scene){
		scene->ScanInputRailWay(4, V3ZERO, V3ZERO, true);
		scene->ScanInputPier(4, V3ZERO, V3ZERO);
		scene->ScanInputLine(4, V3ZERO, V3ZERO);
		scene->ScanInputPole(4, V3ZERO, V3ZERO);
		scene = scene->Next();
	}
	m_NetworkSyncCount = 0;
	Simulate(1);	//	1 回だけシミュレート
	return true;
}

/*
 *	保存
 *
 *	戻り値: 0: succeeded, 1: error, 2: file already exist
 */
int CSaveFile::Save(
	const char *fname,		//	ファイル名
	const char *dirname,	//	ディレクトリ名
	bool overwrite,			//	上書き
	bool upname				//	ファイル名更新
){
	FILE *df;
	if(CheckSlash(fname) || chdir(g_BaseDir) || chdir(dirname)){
		EnqueueCommonDialog(new CSimpleDialog(lang(CannotOpenFile), (char *)fname));
		g_Skin->Error();
		return 1;
	}
	if(!overwrite && (df = fopen(fname, "rb"))){
		fclose(df);
		return 2;
	}
	if(!(df = fopen(fname, "wt"))){
		EnqueueCommonDialog(new CSimpleDialog(lang(CannotOpenFile), (char *)fname));
		g_Skin->Error();
		return 1;
	}
	if(upname) m_FileName = fname;
	m_Version = RAILSIM_VERSION;

	fprintf(df, "DatafileHeader{\n");
	fprintf(df, "\tRailSimVersion = %.2f;\n", RAILSIM_VERSION);
	fprintf(df, "\tDatafileType = %s;\n", LAYOUT_DIRNAME);
	fprintf(df, "}\n\n");

	SYSTEMTIME systime;
	GetLocalTime(&systime);
	fprintf(df, "LayoutInfo{\n");
	fprintf(df, "\tDate = \"%04d/%02d/%02d (%s) %02d:%02d:%02d\";\n", 
		systime.wYear, systime.wMonth, systime.wDay, g_DayOfWeek[systime.wDayOfWeek],
		systime.wHour, systime.wMinute, systime.wSecond);
	fprintf(df, "\tNote = \"%s\";\n", ExpandDoubleQuote(m_FileNote).c_str());
	fprintf(df, "}\n\n");

	fprintf(df, "Time{\n");
	fprintf(df, "\tDate = %d, %d, %d, %d;\n", m_Year, m_Month, m_Day, m_DayOfWeek);
	fprintf(df, "\tTime = %d, %d, %d, %d;\n", m_Hour, m_Minute, m_Second, m_Frame);
	fprintf(df, "\tSumDays = %d;\n", m_SumDays);
	fprintf(df, "\tTimeScale = %d;\n", g_SimulationMode->m_TimeScale->GetNumber());
	fprintf(df, "\tSimulationSpeed = %d;\n", g_SimulationMode->m_SimSpeed->GetNumber());
	fprintf(df, "\tEarthRotation = %d;\n", g_SimulationMode->m_EarthRotation->GetNumber());
	fprintf(df, "\tEarthRevolution = %d;\n", g_SimulationMode->m_EarthRevolution->GetNumber());
	fprintf(df, "}\n\n");

	fprintf(df, "Simulation{\n");
	fprintf(df, "\tManualControl = %s;\n", YESNO[g_SimulationMode->GetManualControl()]);
	fprintf(df, "\tIgnoreAcceleration = %s;\n", YESNO[g_SimulationMode->GetIgnoreAcceleration()]);
	fprintf(df, "}\n\n");

	fprintf(df, "Wind{\n");
	fprintf(df, "\tDir1 = "); V3Save(df, m_WindDir1, ";\n");
	fprintf(df, "\tDir2 = "); V3Save(df, m_WindDir2, ";\n");
	fprintf(df, "\tCount = %d;\n", m_WindCount);
	fprintf(df, "\tTime = %d;\n", m_WindTime);
	fprintf(df, "}\n\n");

	fprintf(df, "WarpList{\n");
	CRailWay *warp = m_WarpList;
	while(warp){
		warp->SaveWarp(df);
		warp = warp->Next();
	}
	fprintf(df, "}\n\n");

	fprintf(df, "RailBlockList{\n");
	map<std::string, CTrainGroup *>::iterator railblock;
	for(railblock = g_RailBlockMap.begin(); railblock!=g_RailBlockMap.end(); ++railblock){
		if(railblock->second) fprintf(df, "\tRailBlock = \"%s\", %p;\n",
			ExpandDoubleQuote(railblock->first).c_str(), railblock->second);
	}
	fprintf(df, "}\n\n");

	fprintf(df, "TrainGroupList{\n");
	CTrainGroup *group = m_GroupList;
	while(group){
		group->Save(df);
		group = group->Next();
	}
	fprintf(df, "}\n\n");

	fprintf(df, "SceneList{\n");
	fprintf(df, "\tCurrentScene = %p;\n", g_Scene);
	CScene *scene = m_SceneList;
	while(scene){
		scene->Save(df);
		scene = scene->Next();
	}
	fprintf(df, "}\n\n");

	fprintf(df, "Window{\n");
	g_ConfigMode->GetRootWindow()->Save(df, "\t");
	fprintf(df, "}\n\n");

	fclose(df);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	閏年かどうか調べる
 */
bool IsLeapYear(
	int year	//	年 (A.D.1 = 0)
){
	year++;
	if(year%4) return false;
	if(year%100) return true;
	if(year%400) return false;
	return true;
}

/*
 *	月当たりの日数を調べる
 */
int GetDaysPerMonth(
	int year,	//	年
	int month	//	月
){
	static int sdpm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int ret = sdpm[month];
	if(month==1 && IsLeapYear(year)) ret++;
	return ret;
}

/*
 *	変換後アドレス取得
 */
void *ReplaceAdr(
	void *oldadr	//	旧アドレス
){
	if(oldadr){
		if(!g_AddressMap.count(oldadr)){
		//	Dialog("Address not found: %p", oldadr);
			return NULL;
		}
		return g_AddressMap[oldadr];
	}else{
		return NULL;
	}
}

/*
 *	マップにアドレスを登録 (ネットワーク同期用)
 */
void *RegisterNewMapAddress(void *new_adr){
	extern int g_NetworkDummyMapAddress;
	while(g_AddressMap.count((void *)g_NetworkDummyMapAddress)) g_NetworkDummyMapAddress++;
	g_AddressMap[(void *)g_NetworkDummyMapAddress] = new_adr;
	return (void *)g_NetworkDummyMapAddress;
}
