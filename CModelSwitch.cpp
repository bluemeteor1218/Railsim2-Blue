#include "stdafx.h"
#include "CListView.h"
#include "CExpression.h"
#include "CModelPlugin.h"

//	外部グローバル
extern float g_FovRatio;

//	内部グローバル
CModelSwitch g_SystemSwitch[SYSTEM_SWITCH_NUM];

vector<CStaticSwitchID> g_StaticSwitchTable;

int CurrentEnabledEffectorTypes(){
	return g_PreSimulationFlag ? EFCT_PARTICLEAPPLIER|EFCT_RAILCONNECTOR : EFCT_MISCELLANEOUS;
}

/*
 *	コンストラクタ
 */
CModelSwitch::CModelSwitch(){
	m_ListElement = NULL;
}

/*
 *	コンストラクタ
 */
CModelSwitch::CModelSwitch(
	int id	//	番号
){
	m_ID = id;
	m_Value = 0;
	m_ListElement = NULL;
}

/*
 *	初期化
 */
void CModelSwitch::Init(
	int id,		//	番号
	char *name	//	スイッチ名
){
	m_ID = id;
	m_SwitchName = name;
}

/*
 *	読込
 */
char *CModelSwitch::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	if(!(str = BeginNamedBlock(eee = str, "DefineSwitch", &m_SwitchName))) return NULL;

	int i;
	for(i = 0; i<SYSTEM_SWITCH_NUM; i++) if(g_SystemSwitch[i].Check(m_SwitchName))
		throw CSynErr(eee, "%s: \"%s\"", lang(ReservedSwitchName), m_SwitchName.c_str());
	if(mpi->FindModelSwitch(m_SwitchName))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedSwitchName), m_SwitchName.c_str());

	if(tmp = AsgnString(str, "GroupCommon", &m_GroupCommon)) str = tmp;
	else m_GroupCommon = "";

	string entry;
	m_Entry.clear();
	while(true){
		if(tmp = AsgnString(str, "Entry", &entry)){
			str = tmp;
			m_Entry.push_back(entry);
		}else{
			break;
		}
	}

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	現在の設定を取得
 */
char *CModelSwitch::GetCurrentOptionName(){
	int i = 0;
	Istring is = m_Entry.begin();
	for(; is!=m_Entry.end(); is++, i++) if(i==m_Value) return (char *)is->c_str();
	return "";
}

/*
 *	エントリリスト作成
 */
void CModelSwitch::ListEntry(
	CListView *olv	//	リストビュー
){
	olv->DeleteAllItems();
	int i = 0;
	Istring is = m_Entry.begin();
	for(; is!=m_Entry.end(); is++, i++){
		CListElement *ole = olv->InsertItem(-1, (char *)is->c_str());
		ole->SetData(i);
	}
	olv->SetSelectionMark(m_Value, 0);
}

/*
 *	スイッチリスト作成
 */
void CModelSwitch::EditSwitch(
	CListView *olv,	//	オプションリストビュー
	int *link,		//	リンク変数
	int switch_id	//	スイッチ ID
){
	if(!m_Entry.size()) return;
	CListElement *ole = olv->GetFocusItem();
	bool force_reset = false;
	if(g_NetworkInitialized && switch_id>=0){
		if(g_StaticSwitchTable[switch_id].net_sync){
			force_reset = true;
			g_StaticSwitchTable[switch_id].tmp_opt = m_Value;
			g_StaticSwitchTable[switch_id].net_sync = false;
		}else{
			link = &g_StaticSwitchTable[switch_id].tmp_opt;
		}
	}
	if(ole && ole->IsSelected() && !force_reset){
		int opt = ole->GetData();
		if(opt!=m_Value){
			m_Value = opt;
			bool async = false;
			if(link && *link!=m_Value){
				async = true;
				*link = m_Value;
			}
			m_ListElement->SetString(1, GetCurrentOptionName());
			if(switch_id>=0 && g_NetworkInitialized && async){
				//Dialog("set switch [%d] <= %d", switch_id, m_Value);
				void EnqueueSwitchControl(int, int);
				EnqueueSwitchControl(switch_id, m_Value);
			}
		}
	}else{
		olv->SetSelectionMark(m_Value, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CSwitchEntry::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi,	//	モデルプラグイン
	int condtype		//	条件タイプ
){
	char *tmp, *eee;
	int value;
	m_TypeFlag = 0;
	m_ConditionType = condtype;
	m_Value.clear();
	if(!m_ConditionType){
		if(!(str = Identifier2(str, "Case"))) return NULL;
		if(!(str = ConstInteger(eee = str, &value))) throw CSynErr(eee);
		m_Value.push_back(value);
		while(tmp = Character2(str, ',')){
			str = tmp;
			if(!(str = ConstInteger(eee = str, &value))) throw CSynErr(eee);
			m_Value.push_back(value);
		}
		if(!(str = Character2(eee = str, ':'))) throw CSynErr(eee);
	}
	if(!(str = Read2(eee = str, mpi))) throw CSynErr(eee);
	return str;
}

/*
 *	スイッチ値チェック
 */
bool CSwitchEntry::CheckValue(
	int value,	//	判定値
	bool def	//	デフォルト
){
	switch(m_ConditionType){
	case 0: {
		int i, vn = m_Value.size();
		if(!vn) return true;
		for(i = 0; i<vn; i++) if(m_Value[i]==value) return true;
		return false; }
	case 1:
		return true;
	case 2:
		return !!value;
	case 3:
		return !value;
	case 4:
		return def;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CSwitchApplier::CSwitchApplier(
	const CSwitchApplier *src	//	コピー元
){
	m_TypeFlag = src->m_TypeFlag;
	m_Expression = src->m_Expression->Duplicate();
}

/*
 *	デストラクタ
 */
CSwitchApplier::~CSwitchApplier(){
	DELETE_V(m_Expression);
}

/*
 *	読込
 */
char *CSwitchApplier::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee, *tmp;
	m_TypeFlag = 0;
	if(tmp = Identifier2(eee = str, "ApplySwitch")){
		str = tmp;
		g_SwitchOwner = mpi;
		if(!(str = Expression(eee = str, &m_Expression))) throw CSynErr(eee, lang(InvalidExpression));
		g_SwitchOwner = NULL;
		if(!(str = Character2(eee = str, '{'))) return NULL;
		while(true){
			if(!(tmp = Read2(eee = str, mpi, 0))) throw CSynErr(eee);
			if(tmp==str) break;
			str = tmp;
		}
		if(tmp = Identifier2(str, "Default")){
			str = tmp;
			if(!(str = Character2(eee = str, ':'))) throw CSynErr(eee);
			if(!(str = Read2(eee = str, mpi, 4))) throw CSynErr(eee);
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		return str;
	}else if(tmp = Identifier2(eee = str, "If")){
		str = tmp;
		g_SwitchOwner = mpi;
		if(!(str = Expression(eee = str, &m_Expression))) throw CSynErr(eee, lang(InvalidExpression));
		g_SwitchOwner = NULL;
		if(!(str = Character2(eee = str, '{'))) return NULL;
		if(!(str = Read2(eee = str, mpi, 2))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		if(tmp = BeginBlock(str, "Else")){
			str = tmp;
			if(!(str = Read2(eee = str, mpi, 3))) throw CSynErr(eee);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}
		return str;
	}else{
		return NULL;
	}
}

/*
 *	数式計算
 */
int CSwitchApplier::GetSwitchValue(){
	return m_Expression->CalcInt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CCustomizerSwitchEntry::Read2(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	m_Customizer.clear();
	CCustomizerContainer cont;
	while(tmp = cont.Read(str, mpi)){
		str = tmp;
		m_Customizer.push_back(cont);
		m_TypeFlag |= cont.GetTypeFlag();
	}
	return str;
}

/*
 *	データ読込
 */
void CCustomizerSwitchEntry::LoadData(
	CModelPlugin *mpi	//	呼び出し元
){
	ICustomizerContainer icc = m_Customizer.begin();
	for(; icc!=m_Customizer.end(); icc++) icc->LoadData(mpi);
}

/*
 *	常時適用リスト作成
 */
void CCustomizerSwitchEntry::SetOffList(
	CNamedObject *nobj	//	呼び出し元
){
	ICustomizerContainer icc = m_Customizer.begin();
	for(; icc!=m_Customizer.end(); icc++) icc->SetOffList(nobj);
}

/*
 *	代替モデル取得
 */
CMesh *CCustomizerSwitchEntry::GetMesh(
	float *asc	//	スケール格納先
){
	if(!(m_TypeFlag&CSTM_MODELCHANGER)) return NULL;
	ICustomizerContainer icc = m_Customizer.begin();
	for(; icc!=m_Customizer.end(); icc++){
		CMesh *tm = icc->GetMesh(asc);
		if(*asc>=0.0f) return tm;
	}
	return NULL;
}

/*
 *	姿勢適用
 */
void CCustomizerSwitchEntry::SetPosture(
	CObject *obj	//	オブジェクト
){
	if(!(m_TypeFlag&CSTM_MODELMOVER)) return;
	ICustomizerContainer icc = m_Customizer.begin();
	for(; icc!=m_Customizer.end(); icc++) icc->SetPosture(obj);
}

/*
 *	変更子適用
 */
void CCustomizerSwitchEntry::Apply(
	CMesh *mesh	//	メッシュ
){
	if(!(m_TypeFlag&CSTM_MISCELLANEOUS)) return;
	ICustomizerContainer icc = m_Customizer.begin();
	for(; icc!=m_Customizer.end(); icc++) icc->Apply(mesh);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CCustomizerSwitchApplier::Read2(
	char *str,			//	対象文字列
	CModelPlugin *mpi,	//	モデルプラグイン
	int condtype		//	条件タイプ
){
	char *tmp;
	CCustomizerSwitchEntry swent;
	if(tmp = swent.Read(str, mpi, condtype)){
		str = tmp;
		m_Entry.push_back(swent);
		m_TypeFlag |= swent.GetTypeFlag();
	}
	return str;
}

/*
 *	データ読込
 */
void CCustomizerSwitchApplier::LoadDataCustomizer(
	CModelPlugin *mpi	//	呼び出し元
){
	ICustomizerSwitchEntry ise = m_Entry.begin();
	for(; ise!=m_Entry.end(); ise++) ise->LoadData(mpi);
}

/*
 *	常時適用リスト作成
 */
void CCustomizerSwitchApplier::SetOffListCustomizer(
	CNamedObject *nobj	//	呼び出し元
){
	ICustomizerSwitchEntry ise = m_Entry.begin();
	for(; ise!=m_Entry.end(); ise++) ise->SetOffList(nobj);
}

/*
 *	代替モデル取得
 */
CMesh *CCustomizerSwitchApplier::GetMeshCustomizer(
	float *asc	//	スケール格納先
){
	if(!(m_TypeFlag&CSTM_MODELCHANGER)) return NULL;
	ICustomizerSwitchEntry ise = m_Entry.begin();
	int eval = GetSwitchValue();
	bool def = true;
	for(; ise!=m_Entry.end(); ise++){
		if(ise->CheckValue(eval, def)){
			def = false;
			CMesh *tm = ise->GetMesh(asc);
			if(*asc>=0.0f) return tm;
		}
	}
	return NULL;
}

/*
 *	姿勢適用
 */
void CCustomizerSwitchApplier::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	if(!(m_TypeFlag&CSTM_MODELMOVER)) return;
	ICustomizerSwitchEntry ise = m_Entry.begin();
	int eval = GetSwitchValue();
	bool def = true;
	for(; ise!=m_Entry.end(); ise++){
		if(ise->CheckValue(eval, def)){
			def = false;
			ise->SetPosture(obj);
		}
	}
}

/*
 *	変更子適用
 */
void CCustomizerSwitchApplier::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!(m_TypeFlag&CSTM_MISCELLANEOUS)) return;
	ICustomizerSwitchEntry ise = m_Entry.begin();
	int eval = GetSwitchValue();
	bool def = true;
	for(; ise!=m_Entry.end(); ise++){
		if(ise->CheckValue(eval, def)){
			def = false;
			ise->Apply(mesh);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEffectorSwitchEntry::Read2(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	m_Effector.clear();
	CEffectorContainer cont;
	while(tmp = cont.Read(str, mpi)){
		str = tmp;
		m_Effector.push_back(cont);
		m_TypeFlag |= cont.GetTypeFlag();
	}
	return str;
}

/*
 *	データ読込
 */
void CEffectorSwitchEntry::LoadData(
	CModelPlugin *mpi	//	呼び出し元
){
	IEffectorContainer icc = m_Effector.begin();
	for(; icc!=m_Effector.end(); icc++) icc->LoadData(mpi);
}

/*
 *	変更子適用
 */
void CEffectorSwitchEntry::Apply(
	CScene *scene	//	シーン
){
	if(!(m_TypeFlag&CurrentEnabledEffectorTypes())) return;
	IEffectorContainer icc = m_Effector.begin();
	for(; icc!=m_Effector.end(); icc++) icc->Apply(scene);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEffectorSwitchApplier::Read2(
	char *str,			//	対象文字列
	CModelPlugin *mpi,	//	モデルプラグイン
	int condtype		//	条件タイプ
){
	char *tmp;
	CEffectorSwitchEntry swent;
	if(tmp = swent.Read(str, mpi, condtype)){
		str = tmp;
		m_Entry.push_back(swent);
		m_TypeFlag |= swent.GetTypeFlag();
	}
	return str;
}

/*
 *	データ読込
 */
void CEffectorSwitchApplier::LoadDataEffector(
	CModelPlugin *mpi	//	呼び出し元
){
	IEffectorSwitchEntry ise = m_Entry.begin();
	for(; ise!=m_Entry.end(); ise++) ise->LoadData(mpi);
}

/*
 *	変更子適用
 */
void CEffectorSwitchApplier::ApplyEffector(
	CScene *scene	//	シーン
){
	if(!(m_TypeFlag&CurrentEnabledEffectorTypes())) return;
	IEffectorSwitchEntry ise = m_Entry.begin();
	int eval = GetSwitchValue();
	bool def = true;
	for(; ise!=m_Entry.end(); ise++){
		if(ise->CheckValue(eval, def)){
			def = false;
			ise->Apply(scene);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	システムスイッチ初期化
 */
void InitSystemSwitch(){
	g_SystemSwitch[SYS_SW_FRONT].Init(-SYS_SW_FRONT-1, "_FRONT");
	g_SystemSwitch[SYS_SW_CONNECT1].Init(-SYS_SW_CONNECT1-1, "_CONNECT1");
	g_SystemSwitch[SYS_SW_CONNECT2].Init(-SYS_SW_CONNECT2-1, "_CONNECT2");
	g_SystemSwitch[SYS_SW_DOOR1].Init(-SYS_SW_DOOR1-1, "_DOOR1");
	g_SystemSwitch[SYS_SW_DOOR2].Init(-SYS_SW_DOOR2-1, "_DOOR2");
	g_SystemSwitch[SYS_SW_SERIAL].Init(-SYS_SW_SERIAL-1, "_SERIAL");
	g_SystemSwitch[SYS_SW_CAMDIST].Init(-SYS_SW_CAMDIST-1, "_CAMDIST");
	g_SystemSwitch[SYS_SW_VELOCITY].Init(-SYS_SW_VELOCITY-1, "_VELOCITY");
	g_SystemSwitch[SYS_SW_ACCEL].Init(-SYS_SW_ACCEL-1, "_ACCEL");
	g_SystemSwitch[SYS_SW_CABINVIEW].Init(-SYS_SW_CABINVIEW-1, "_CABINVIEW");
	g_SystemSwitch[SYS_SW_APPROACH1].Init(-SYS_SW_APPROACH1-1, "_APPROACH1");
	g_SystemSwitch[SYS_SW_APPROACH2].Init(-SYS_SW_APPROACH2-1, "_APPROACH2");
	g_SystemSwitch[SYS_SW_STOPPING].Init(-SYS_SW_STOPPING-1, "_STOPPING");

	g_SystemSwitch[SYS_SW_NIGHT].Init(-SYS_SW_NIGHT-1, "_NIGHT");
	g_SystemSwitch[SYS_SW_WEATHER].Init(-SYS_SW_WEATHER-1, "_WEATHER");
	g_SystemSwitch[SYS_SW_SEASON].Init(-SYS_SW_WEATHER-1, "_SEASON");
	g_SystemSwitch[SYS_SW_SHADOW].Init(-SYS_SW_SHADOW-1, "_SHADOW");
	g_SystemSwitch[SYS_SW_ENVMAP].Init(-SYS_SW_ENVMAP-1, "_ENVMAP");
	g_SystemSwitch[SYS_SW_YEAR].Init(-SYS_SW_YEAR-1, "_YEAR");
	g_SystemSwitch[SYS_SW_MONTH].Init(-SYS_SW_MONTH-1, "_MONTH");
	g_SystemSwitch[SYS_SW_DAY].Init(-SYS_SW_DAY-1, "_DAY");
	g_SystemSwitch[SYS_SW_DAYOFWEEK].Init(-SYS_SW_DAYOFWEEK-1, "_DAYOFWEEK");
	g_SystemSwitch[SYS_SW_HOUR].Init(-SYS_SW_HOUR-1, "_HOUR");
	g_SystemSwitch[SYS_SW_MINUTE].Init(-SYS_SW_MINUTE-1, "_MINUTE");
	g_SystemSwitch[SYS_SW_SECOND].Init(-SYS_SW_SECOND-1, "_SECOND");
}

/*
 *	カメラ距離スイッチ設定
 */
void SetCamDistSwitch(
	VEC3 pos	//	視点座標
){
	g_SystemSwitch[SYS_SW_CAMDIST].SetValue(Round(g_FovRatio*V3Len(&(pos-GetVPos()))));
}
