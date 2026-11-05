#include "stdafx.h"
#include "CListView.h"
#include "CDiaInst.h"

//	関数宣言
void *ReplaceAdr(void *);

//	外部グローバル
extern map<void *, void *> g_AddressMap;

/*
 *	コンストラクタ
 */
CDiaElementBase::CDiaElementBase(){
	m_ListElement = NULL;
}

/*
 *	リスト要素文字列更新
 */
void CDiaElementBase::UpdateCaption(){
	if(m_ListElement) m_ListElement->SetString(0, (char *)GetListCaption().c_str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CDiaListBase::CDiaListBase(){
	m_UseDefault = true;
}

/*
 *	要素解放
 */
void CDiaListBase::Free(){
	IPDiaElementBase ipde = m_DiaList.begin();
	for(; ipde!=m_DiaList.end(); ipde++) DELETE_V(*ipde);
	m_DiaList.clear();
}

/*
 *	要素リスト作成
 */
void CDiaListBase::ListElement(
	CListView *lv,	//	リストビュー
	int sel			//	選択位置
){
	lv->DeleteAllItems();
	if(!Size()) m_DiaList.push_back(NewEntry());
	IPDiaElementBase ipde = m_DiaList.begin();
	for(; ipde!=m_DiaList.end(); ipde++){
		CListElement *le = lv->InsertItem(-1, (char *)(*ipde)->GetListCaption().c_str());
		(*ipde)->m_ListElement = le;
		le->SetData((DWORD)(*ipde));
	}
	if(sel<0){
		if(!m_UseDefault){
			lv->SetSelectionMark(0, 0);
			lv->EnsureVisible(0);
		}
	}else{
		lv->SetSelectionMark(sel, 0);
		lv->EnsureVisible(sel);
	}
}

/*
 *	要素追加
 */
int CDiaListBase::AddElement(
	CDiaElementBase *el	//	この後に追加
){
	int i = 0;
	IPDiaElementBase ipde = m_DiaList.begin();
	for(; ipde!=m_DiaList.end(); ipde++, i++){
		if(*ipde==el){
			m_DiaList.insert(++ipde, NewEntry());
			return i+1;
		}
	}
	m_DiaList.insert(ipde, NewEntry());
	return i;
}

/*
 *	要素削除
 */
int CDiaListBase::DeleteElement(
	CDiaElementBase *el	//	削除要素
){
	int i = 0;
	IPDiaElementBase ipde = m_DiaList.begin();
	for(; ipde!=m_DiaList.end(); ipde++, i++){
		if(*ipde==el){
			DELETE_V(*ipde);
			ipde = m_DiaList.erase(ipde);
			return ipde==m_DiaList.end() ? i-1 : i;
		}
	}
	return -1;
}

/*
 *	要素回転
 */
void CDiaListBase::RotateElement(
	bool prev	//	逆回転
){
	if(Size()<2) return;
	if(prev) m_DiaList.splice(m_DiaList.begin(), m_DiaList, --m_DiaList.end(), m_DiaList.end());
	else m_DiaList.splice(m_DiaList.end(), m_DiaList, m_DiaList.begin(), ++m_DiaList.begin());
}

/*
 *	最初の要素を取得し、ひとつ送る
 */
CDiaElementBase *CDiaListBase::DequeueBase(bool rotate){
	if(m_DiaList.size()){
		CDiaElementBase *ret = *m_DiaList.begin();
		if(rotate) RotateElement(false);
		return ret;
	}else{
		m_DiaList.push_back(NewEntry());
		return *m_DiaList.begin();
	}
}

/*
 *	読込
 */
char *CDiaListBase::Read(
	char *str,			//	対象文字列
	CTrainGroup **gadr	//	編成アドレス格納先
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, Label()))) return NULL;
	if(!(str = AsgnPointer(eee = str, "TrainGroup", (void **)gadr))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "UseDefault", &m_UseDefault))) throw CSynErr(eee);
	while(true){
		CDiaElementBase *de = NewEntry();
		if(tmp = de->Read(str)){
			str = tmp;
			m_DiaList.push_back(de);
		}else{
			delete de;
			break;
		}
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CDiaListBase::Save(
	FILE *df,			//	ファイル
	char *ind,			//	インデント
	CTrainGroup *group	//	編成
){
	string ind2 = string(ind)+"\t";
	fprintf(df, "%s%s{\n", ind, Label());
	fprintf(df, "%s\tTrainGroup = %p;\n", ind, group);
	fprintf(df, "%s\tUseDefault = %s;\n", ind, YESNO[m_UseDefault]);
	IPDiaElementBase ipde = m_DiaList.begin();
	for(; ipde!=m_DiaList.end(); ipde++) (*ipde)->Save(df, (char *)ind2.c_str());
	fprintf(df, "%s}\n", ind);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	デストラクタ
 */
CDiaInstBase::~CDiaInstBase(){
	IPDiaListBase ipdl = m_DiaMap.begin();
	for(; ipdl!=m_DiaMap.end(); ipdl++){
		ipdl->second->Free();
		DELETE_V(ipdl->second);
	}
}

/*
 *	ダイヤリスト検索
 */
CDiaListBase *CDiaInstBase::Search(
	CTrainGroup *group	//	編成
){
	CDiaListBase *&dlist = m_DiaMap[group];
	if(!dlist){
		dlist = NewEntry();
		if(!group) dlist->m_UseDefault = false;
	}
	return dlist;
}

/*
 *	ダイヤリスト検索 (デフォルト設定ならデフォルトを返す)
 */
CDiaListBase *CDiaInstBase::SearchEffectBase(
	CTrainGroup *group	//	編成
){
	if(!m_DiaMap.count(group)) return Search(NULL);
	CDiaListBase *dlist = m_DiaMap[group];
	if(dlist->m_UseDefault) return Search(NULL);
	return dlist;
}

/*
 *	アドレス復元
 */
void CDiaInstBase::RestoreAddress(){
	map<CTrainGroup *, CDiaListBase *> diamap;
	IPDiaListBase ipdl = m_DiaMap.begin();
	for(; ipdl!=m_DiaMap.end(); ipdl++)
		diamap[(CTrainGroup *)ReplaceAdr(ipdl->first)] = ipdl->second;
	m_DiaMap = diamap;
}

/*
 *	読込
 */
char *CDiaInstBase::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, Label()))) return NULL;
	if(!(str = AsgnString(eee = str, "Name", &m_Name))) throw CSynErr(eee);
	m_Name = RestoreDoubleQuote(m_Name);
	while(true){
		CTrainGroup *group;
		CDiaListBase *dlist = NewEntry();
		if(tmp = dlist->Read(str, &group)){
			str = tmp;
			m_DiaMap[group] = dlist;
		}else{
			delete dlist;
			break;
		}
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CDiaInstBase::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	string ind2 = string(ind)+"\t";
	fprintf(df, "%s%s{\n", ind, Label());
	fprintf(df, "%s\tName = \"%s\";\n", ind, ExpandDoubleQuote(m_Name).c_str());
	IPDiaListBase ipdl = m_DiaMap.begin();
	for(; ipdl!=m_DiaMap.end(); ipdl++) ipdl->second->Save(
		df, (char *)ind2.c_str(), ipdl->first);
	fprintf(df, "%s}\n", ind);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CPointElement::CPointElement(){
	m_PointSwitch = 0;
}

/*
 *	リスト文字列取得
 */
string CPointElement::GetListCaption(){
	static char *cap[3] = {lang(Random), lang(Left), lang(Right)};
	return cap[m_PointSwitch];
}

/*
 *	ポイント計算
 */
int CPointElement::CalcPoint(){
	return m_PointSwitch ? m_PointSwitch-1 : Rand(2);
}

/*
 *	読込
 */
char *CPointElement::Read(
	char *str	//	対象文字列
){
	if(!(str = AsgnInteger(str, "Point", &m_PointSwitch))) return NULL;
	return str;
}

/*
 *	保存
 */
void CPointElement::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sPoint = %d;\n", ind, m_PointSwitch);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CDiaElement::CDiaElement(){
	m_Action = 0;
	m_TimeType = 0;
	m_Hour = 1;
	m_Minute = 0;
	m_Second = 0;
//	m_Joint = 0;
	m_Offset = 0.0f;
}

/*
 *	リスト文字列取得
 */
string CDiaElement::GetListCaption(){
	char *timetype[2] = {lang(Duration), lang(Time)};
	string ret;
	switch(m_Action){
	case 0:
		ret = FlashIn("%s / %s %02d:%02d:%02d",
			lang(Stop), timetype[m_TimeType], m_Hour, m_Minute, m_Second);
		break;
	case 1:
		ret = FlashIn("%s / %s %02d:%02d:%02d",
			lang(Return), timetype[m_TimeType], m_Hour, m_Minute, m_Second);
		break;
	case 2:
		ret = lang(Pass);
		break;
	}
//	if(m_Joint) ret += " / 連結";
	return ret;
}

/*
 *	読込
 */
char *CDiaElement::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "DiaElement"))) return NULL;
	if(!(str = AsgnInteger(eee = str, "Action", &m_Action))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "TimeType", &m_TimeType))) throw CSynErr(eee);
	if(!(str = Assignment(eee = str, "Time"))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Hour))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Minute))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Second))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Offset", &m_Offset))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CDiaElement::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sDiaElement{\n", ind);
	fprintf(df, "%s\tAction = %d;\n", ind, m_Action);
	fprintf(df, "%s\tTimeType = %d;\n", ind, m_TimeType);
	fprintf(df, "%s\tTime = %d, %d, %d;\n", ind, m_Hour, m_Minute, m_Second);
	fprintf(df, "%s\tOffset = %f;\n", ind, m_Offset);
	fprintf(df, "%s}\n", ind, m_Offset);
}
