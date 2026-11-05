#include "stdafx.h"
#include "CPopMenu.h"
#include "CSimpleDialog.h"
#include "CRailWay.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CStation.h"
#include "CStationPlugin.h"
#include "CStationEditMode.h"
#include "CDiaEditMode.h"

//	static メンバ
CStation **CStation::ms_Root = NULL;

/*
 *	コンストラクタ (読込用)
 */
CPlatformInst::CPlatformInst(
	CStation *station	//	駅
){
	m_Length = -1.0f;
	m_Stoppable = false;
	m_OpenDoor[0] = m_OpenDoor[1] = false;
	m_Station = station;
}

/*
 *	コンストラクタ
 */
CPlatformInst::CPlatformInst(
	CStation *station,	//	駅
	bool stoppable,		//	停車可能 /*CP932対応*/
	bool *opendoor		//	ドア開
){
	m_Length = -1.0f;
	m_Stoppable = stoppable;
	int i;
	for(i = 0; i<2; i++) m_OpenDoor[i] = opendoor[i];
	m_Station = station;
}

/*
 *	レール削除
 */
void CPlatformInst::DeleteRailWay(
	CRailWay *way	//	レール
){
	m_Length = -1.0f;
	m_RailList.remove(way);
}

/*
 *	プラットフォーム削除
 */
void CPlatformInst::DeletePlatform(){
	g_SaveFile->DeletePlatform(this);
	IPRailWay ipr = m_RailList.begin();
	for(; ipr!=m_RailList.end(); ipr++){
		(*ipr)->SetPlatform(NULL);
		(*ipr)->SetParent(NULL);
	}
}

/*
 *	プラットフォーム親設定
 */
void CPlatformInst::SetPlatformParent(CDetectInfo *d_info){
	IPRailWay ipr = m_RailList.begin();
	for(; ipr!=m_RailList.end(); ipr++) (*ipr)->SetParent(d_info);
}

/*
 *	長さ計算
 */
float CPlatformInst::GetLength(){
	if(m_Length<0.0f){
		m_Length = 0.0f;
		IPRailWay ipr = m_RailList.begin();
		for(; ipr!=m_RailList.end(); ipr++) m_Length += (*ipr)->GetSegLen();
	}
	return m_Length;
}

/*
 *	状態取得 (1: approach, 2: stop)
 */
int CPlatformInst::GetPlatformState(){
	int state = 0;
	IPRailWay ipr = m_RailList.begin();
	for(; ipr!=m_RailList.end(); ipr++) state |= (*ipr)->GetPlatformState();
	return state;
}

/*
 *	シミュレーション進行
 */
void CPlatformInst::Simulate(){
	IPRailWay ipr = m_RailList.begin();
	for(; ipr!=m_RailList.end(); ipr++) (*ipr)->UpdateSplitList();
}

/*
 *	アドレス復元
 */
void CPlatformInst::RestoreAddress(){
	IPRailWay ipr = m_RailList.begin();
	for(; ipr!=m_RailList.end(); ipr++) *ipr = (CRailWay *)ReplaceAdr(*ipr);
}

/*
 *	読込
 */
char *CPlatformInst::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "Platform"))) return NULL;
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "Stoppable", &m_Stoppable))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "OpenDoor", m_OpenDoor, 2, false))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	if(tmp = Assignment(str, "RailList")){
		str = tmp;
		do{
			if(m_RailList.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
			CRailWay *way;
			if(!(str = HexPointer(eee = str, (void **)&way))) throw CSynErr(eee);
			m_RailList.push_back(way);
		} while(!(tmp = Character2(str, ';')));
		str = tmp;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CPlatformInst::Save(
	FILE *df	//	ファイル
){	
	fprintf(df, "\t\t\t\t\tPlatform{\n");
	fprintf(df, "\t\t\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\t\t\tStoppable = %s;\n", YESNO[m_Stoppable]);
	fprintf(df, "\t\t\t\t\t\tOpenDoor = %s, %s;\n",
		YESNO[m_OpenDoor[0]], YESNO[m_OpenDoor[1]]);
	if(m_RailList.size()){
		fprintf(df, "\t\t\t\t\t\tRailList = ");
		IPRailWay ipr = m_RailList.begin();
		for(; ipr!=m_RailList.end(); ipr++) fprintf(
			df, ipr==m_RailList.begin() ? "%p" : ", %p", *ipr);
		fprintf(df, ";\n");
	}
	fprintf(df, "\t\t\t\t\t}\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ (読込用)
 */
CStation::CStation(){
	m_StationPlugin = NULL;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CStation::CStation(
	CStationPlugin *spi,	//	駅舎プラグイン
	VEC3 pos,				//	位置
	VEC3 dir,				//	dir
	VEC3 up					//	up
):
	CStruct(spi, pos, dir, up, false)	//	基本クラス
{
	m_StationPlugin = spi;
	m_Next = *ms_Root;
	*ms_Root = this;
	g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(m_Pos, m_Dir, m_Up);
	m_StationPlugin->BuildPlatform(this);
}

/*
 *	デストラクタ
 */
CStation::~CStation(){
	DELETE_V(m_Next);	//	CStruct::m_Next is not linked
}

/*
 *	プラットフォーム追加
 */
CPlatformInst *CStation::PushPlatformInst(
	CPlatformInst &pfi	//	停車可能 /*CP932対応*/
){
	m_PlatformList.push_back(pfi);
	return &*m_PlatformList.rbegin();
}

/*
 *	プラットフォーム配列構築
 */
void CStation::MakePlatformArray(){
	m_PlatformArray.clear();
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) m_PlatformArray.push_back(&*ipi);
}

/*
 *	プラグイン選択モード取得
 */
bool CStation::IsSelectVisible(){
	return g_StationEditMode->IsModeActive() || g_DiaEditMode->IsModeActive();
}

/*
 *	撤去
 */
void CStation::Remove(){
	m_Scene->DeleteStation(this);
}

/*
 *	リストから削除
 */
void CStation::Delete(){
	m_Next = NULL;
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) ipi->DeletePlatform();
	delete this;
}

/*
 *	操作
 */
CModelInst *CStation::Control(){
	if(CEditBox::IsActive()) return this;
	if(GetKey(DIK_DELETE)==S_PUSH){
		class CStationDeleter: public CMenuCommand{
		private:
			CStation *m_Station;	//	駅舎
		public:
			CStationDeleter(CStation *s){ m_Station = s; }
			void Exec(){
				void PushUndoStack();
				PushUndoStack();
				m_Station->Remove();
			}
		};
		CYesNoDialog *dlg = new CYesNoDialog(
			lang(DeleteStationCfmMessage), lang(Confirmation), false);
		dlg->SetYesCommand(new CStationDeleter(this));
		EnqueueCommonDialog(dlg);
	}
	return this;
}

/*
 *	シミュレーション進行
 */
void CStation::SimulateStruct(){
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) ipi->Simulate();
}

/*
 *	専用スイッチ設定
 */
void CStation::SetSwitchStruct(){
	int i = 0, apr1 = 0, apr2 = 0, stpp = 0;
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++, i++){
		int state = ipi->GetPlatformState();
		if(state&1) apr1 |= 1<<i;
		if(state&2) apr2 |= 1<<i;
		if(state&4) stpp |= 1<<i;
	}
	g_SystemSwitch[SYS_SW_APPROACH1].SetValue(apr1);
	g_SystemSwitch[SYS_SW_APPROACH2].SetValue(apr2);
	g_SystemSwitch[SYS_SW_STOPPING].SetValue(stpp);
}

/*
 *	停車可能か
 */
bool CStation::IsStoppable(){
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) if(ipi->IsStoppable()) return true;
	return false;
}

/*
 *	アドレス復元
 */
void CStation::RestoreAddress(){
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) ipi->RestoreAddress();
	m_DiaInst.RestoreAddress();
	list<CPlatformInst>::iterator pfi_itr = m_PlatformList.begin();
	list<CPlatform>::iterator pf_itr = m_StationPlugin->m_Platform.begin();
	for(; pfi_itr!=m_PlatformList.end(); pfi_itr++){
		if(pf_itr==m_StationPlugin->m_Platform.end()) break;
		if(pf_itr->m_ParentObject){
			CPartsInst *parts = FindParts(pf_itr->m_ParentObject);
			if(!parts) ErrorDialog("INTERNAL ERROR: PLATFORM PARENT PARTS NOT FOUND: %s", pf_itr->m_ParentObject->GetName());
			CDetectInfo d_info = CDetectInfo(pf_itr->m_ParentObject, this, parts);
			pfi_itr->SetPlatformParent(&d_info);
		}
		++pf_itr;
	}
}

/*
 *	読込
 */
char *CStation::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "Station"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "StationPlugin", &pid))) throw CSynErr(eee);
	m_ModelPlugin = m_StructPlugin =
		m_StationPlugin = g_StationPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = ReadModelInst(eee = str, true))) throw CSynErr(eee);

	if(!(str = BeginBlock(eee = str, "PlatformList"))) throw CSynErr(eee);
	while(true){
		m_PlatformList.push_back(CPlatformInst(this));
		if(tmp = m_PlatformList.rbegin()->Read(eee = str)){
			str = tmp;
		}else{
			m_PlatformList.pop_back();
			break;
		}
	}
	MakePlatformArray();
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	if(tmp = m_DiaInst.Read(str)) str = tmp;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CStation::Save(
	FILE *df	//	ファイル
){	
	fprintf(df, "\t\t\tStation{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tStationPlugin = \"%s\";\n", CheckPluginID(m_StationPlugin));
	SaveModelInst(df, "\t\t\t\t", true);
	fprintf(df, "\t\t\t\tPlatformList{\n");
	IPlatformInst ipi = m_PlatformList.begin();
	for(; ipi!=m_PlatformList.end(); ipi++) ipi->Save(df);
	fprintf(df, "\t\t\t\t}\n");
	m_DiaInst.Save(df, "\t\t\t\t");
	fprintf(df, "\t\t\t}\n");
}
