#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CSkinPlugin.h"
#include "CTrainPlugin.h"
#include "CTrainGroupTemplate.h"
#include "CTrainGroup.h"
#include "CTrainEditMode.h"

//	内部定数
const char *TGT_DIRNAME = "TrainGroupTemplate";

//	内部グローバル
CTrainGroupTemplate *g_TrainGroupTemplate = NULL;
list<CTrainGroupTemplate> g_TrainGroupTemplateList;

int g_TGTModalState;					//	モーダル状態
string g_TGTTempString;					//	テンポラリ文字列
ITrainGroupTemplate g_TGTTarget;		//	処理対象
ITrainGroupTemplate g_TGTErasing;		//	削除対象
CYesNoDialog *g_TGTYesNoDialog = NULL;	//	問合せダイアログ

/*
 *	初期化
 */
void CTrainTemplate::Init(
	CTrainPlugin *tpi,	//	車輌プラグイン
	bool turn,			//	反転フラグ
	vector<int> &sopt	//	スイッチ設定値
){
	m_Reverse = turn;
	if(m_TrainPlugin = tpi) m_TrainID = m_TrainPlugin->GetID();
	m_SwitchOption = sopt;
}

/*
 *	読込
 */
char *CTrainTemplate::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(eee = str, "TrainTemplate"))) return NULL;
	if(!(str = AsgnString(eee = str, "TrainPlugin", &m_TrainID))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "Reverse", &m_Reverse))) throw CSynErr(eee);
	int snum;
	if(!(str = AsgnInteger(eee = str, "SwitchNum", &snum))) throw CSynErr(eee);
	if(snum){
		m_SwitchOption.resize(snum);
		if(!(str = AsgnInteger(eee = str, "SwitchOption",
			&m_SwitchOption[0], snum, false))) throw CSynErr(eee);
	}else{
		m_SwitchOption = vector<int>();
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CTrainTemplate::Save(
	FILE *file	//	設定ファイル
){
	int i;
	fprintf(file, "\tTrainTemplate{\n");
	fprintf(file, "\t\tTrainPlugin = \"%s\";\n", m_TrainID.c_str());
	fprintf(file, "\t\tReverse = %s;\n", YESNO[m_Reverse]);
	fprintf(file, "\t\tSwitchNum = %d;\n", m_SwitchOption.size());
	if(m_SwitchOption.size()){
		fprintf(file, "\t\tSwitchOption = %d", m_SwitchOption[0]);
		for(i = 1; i<m_SwitchOption.size(); i++) fprintf(file, ", %d", m_SwitchOption[i]);
		fprintf(file, ";\n");
	}
	fprintf(file, "\t}\n");
}

/*
 *	車輌プラグイン読込
 */
void CTrainTemplate::LoadTrainPlugin(){
	m_TrainPlugin = g_TrainPluginList->FindPlugin(m_TrainID.c_str());
}

/*
 *	編成に追加
 */
void CTrainTemplate::AddToGroup(
	CTrainGroup *group	//	編成
){
	if(m_TrainPlugin){
		m_TrainPlugin->SetSwitch(m_SwitchOption);
		group->AddTrain(m_TrainPlugin, m_Reverse);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CTrainGroupTemplate::CTrainGroupTemplate(
	char *name	//	設定名
):
	CPlugin(lang(ConsistTemplate))	//	基本クラス
{
	m_Name = name;
	m_Author = "RailSim II";
	m_Version = 2.00;
	m_State = 1;
	m_TreeElement = NULL;
}

/*
 *	定義ファイルアタッチ
 */
bool CTrainGroupTemplate::PreLoadTGT(
	FILE *file	//	ファイル
){
	char *str = m_Buffer = LoadBinaryText(file), *eee;
	try{
		if(!(str = Space(eee = str))) return false;
		if(!(str = BeginBlock(eee = str, "DatafileHeader"))) return false;
		if(!(str = AsgnFloat(eee = str, "RailSimVersion", &m_Version))) throw CSynErr(eee);
		if(m_Version<2.00f) throw CSynErr(eee, "%s: %.2f", lang(InvalidVersion), m_Version);
		if(RAILSIM_VERSION<m_Version) throw CSynErr(eee, "%s: %.2f", lang(UnsupportedVersion), m_Version);
		string datafiletype;
		if(!(str = AsgnIdentifier(eee = str, "DatafileType", &datafiletype))) throw CSynErr(eee);
		if(datafiletype!=DirName()) throw CSynErr(eee, lang(InvalidDatafileType));
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	m_Script = str;
	return true;
}

/*
 *	読込
 */
bool CTrainGroupTemplate::Load(){
	char *str = m_Script, *tmp, *eee;
	try{
		if(!(str = BeginBlock(eee = str, DirName()))) throw CSynErr(eee);
		while(true){
			CTrainTemplate templ;
			if(tmp = templ.Read(str)){
				str = tmp;
				m_TrainList.push_back(templ);
			}else{
				break;
			}
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	ITrainTemplate itt = m_TrainList.begin();
	for(; itt!=m_TrainList.end(); itt++) itt->LoadTrainPlugin();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	保存
 */
void CTrainGroupTemplate::Save(
	FILE *file	//	設定ファイル
){
	fprintf(file, "DatafileHeader{\n");
	fprintf(file, "\tRailSimVersion = %.2f;\n", RAILSIM_VERSION);
	fprintf(file, "\tDatafileType = %s;\n", DirName());
	fprintf(file, "}\n\n");
	fprintf(file, "TrainGroupTemplate{\n");
	ITrainTemplate itt = m_TrainList.begin();
	for(; itt!=m_TrainList.end(); itt++) itt->Save(file);
	fprintf(file, "}\n");
}

/*
 *	ツリー要素設定
 */
void CTrainGroupTemplate::SetTreeElement(
	CTreeFileElement *fe	//	要素
){
	m_TreeElement = fe;
	m_TreeElement->SetCommandType(CMD_GROUPTMP);
}

/*
 *	リネーム確認
 */
bool CTrainGroupTemplate::ConfirmRename(
	string &newname	//	新規名
){
	if(m_Name==newname) return false;
	g_TGTTarget = FindTrainGroupTemplate(this);
	g_TGTErasing = FindTrainGroupTemplate((char *)newname.c_str());
	if(g_TGTErasing!=g_TrainGroupTemplateList.end()){
		g_TGTModalState = 31;
		g_TGTTempString = newname;
		g_ModalDialog = g_TGTYesNoDialog = new CYesNoDialog(
			FlashIn("%s: \"%s\"", lang(ReplaceTemplateCfmMessage),
			(char *)newname.c_str()), lang(OverlappedTemplateName), false);
		return false;
	}
	if(!Rename(newname)) return false;
	g_TrainEditMode->GetTree()->SelectPlugin(this);
	return false;
}

/*
 *	デリート可能か
 */
bool CTrainGroupTemplate::IsDeletable(){
	g_TGTModalState = 21;
	g_TGTErasing = FindTrainGroupTemplate(this);
	g_ModalDialog = g_TGTYesNoDialog = new CYesNoDialog(
		FlashIn("%s: \"%s\"", lang(DeleteTemplateCfmMessage),
			m_Name.c_str()), lang(DeleteTemplate), false);
	return false;
}

/*
 *	ツリー要素削除
 */
void CTrainGroupTemplate::DeleteFromTree(){
	if(!m_TreeElement) return;
	m_TreeElement->Delete();
	m_TreeElement = NULL;
}

/*
 *	データファイル削除
 */
bool CTrainGroupTemplate::DeleteFromDisk(){
	if(chdir(g_BaseDir) || chdir(DirName()) || remove(TextName2())){
		EnqueueCommonDialog(new CSimpleDialog(
			lang(ErrorDuringDelete), TextName2()));
		g_Skin->Error();
		return false;
	}
	return true;
}

/*
 *	データファイル名変更
 */
bool CTrainGroupTemplate::Rename(
	string &newname	//	新規名
){
	string fname2 = FlashIn("%s.txt", newname.c_str());
	if(CheckSlash(fname2.c_str()) || chdir(g_BaseDir) || chdir(DirName()) ||
		(remove(fname2.c_str()), rename(TextName2(), fname2.c_str()))){
		EnqueueCommonDialog(new CSimpleDialog(
			lang(ErrorDuringRename), TextName2()));
		g_Skin->Error();
		return false;
	}
	m_Name = newname;
	m_TreeElement->SetString((char *)newname.c_str());
	m_TreeElement->GetParent()->Sort(false);
	return true;
}

/*
 *	編成に追加
 */
void CTrainGroupTemplate::AddToGroup(
	CTrainGroup *group	//	編成
){
	ITrainTemplate itt = m_TrainList.begin();
	for(; itt!=m_TrainList.end(); itt++) itt->AddToGroup(group);
}

/*
 *	プレビュー設定
 */
void CTrainGroupTemplate::SetPreview(){
	g_Train = NULL;
	g_TrainGroupTemplate = this;
	ResetPreview();
	CListView *lv = g_TrainEditMode->GetTemplateListView();
	lv->GetParent()->SetText((char *)m_Name.c_str());
	lv->DeleteAllItems();
	ITrainTemplate itt = m_TrainList.begin();
	for(; itt!=m_TrainList.end(); itt++){
		CListElement *le = lv->InsertItem(-1, itt->m_TrainPlugin
			? itt->m_TrainPlugin->GetName() : FlashIn("\"%s\" not found", itt->m_TrainID.c_str()));
		le->SetString(1, itt->m_Reverse ? lang(Yes) : lang(No));
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	テンプレートリスト読込
 */
void LoadTrainGroupTemplateList(){
	long filelist;
	_finddata_t data;
	if(chdir(g_BaseDir) || chdir(TGT_DIRNAME)) return;
	if((filelist = _findfirst("*.txt", &data))>=0){
		do{
			FILE *file;
			if(data.attrib&_A_SUBDIR) continue;
			string name = data.name;
			name[name.size()-4] = 0;
			g_TrainGroupTemplateList.push_back(CTrainGroupTemplate((char *)name.c_str()));
			CTrainGroupTemplate *tgt = &*g_TrainGroupTemplateList.rbegin();
			if(file = fopen(data.name, "rb")){
				if(!tgt->PreLoadTGT(file)){
					g_TrainGroupTemplateList.pop_back();
					continue;
				}
			}else{
				g_TrainGroupTemplateList.pop_back();
				continue;
			}
		} while(!_findnext(filelist, &data));
		_findclose(filelist);
	}
}

/*
 *	テンプレート検索
 */
ITrainGroupTemplate FindTrainGroupTemplate(
	CTrainGroupTemplate *tgt	//	テンプレート
){
	ITrainGroupTemplate itgt = g_TrainGroupTemplateList.begin();
	for(; itgt!=g_TrainGroupTemplateList.end(); itgt++) if(tgt==&*itgt) return itgt;
	return itgt;
}

/*
 *	テンプレート検索
 */
ITrainGroupTemplate FindTrainGroupTemplate(
	string name	//	テンプレート名
){
	ITrainGroupTemplate itgt = g_TrainGroupTemplateList.begin();
	for(; itgt!=g_TrainGroupTemplateList.end(); itgt++)
		if(name==itgt->GetNameRef()) return itgt;
	return itgt;
}

/*
 *	テンプレートメニューコマンド設定
 */
void SetTemplateMenu(
	CPopMenu *menu,				//	メニュー
	CTrainGroupTemplate *tmp,	//	テンプレート
	CListElement *renametarget	//	リネーム対象 (ListView)
){
	class CTemplateNewer: public CMenuCommand{
	private:
		CTrainEditMode *m_Owner;			//	発行元
		CTrainGroupTemplate *m_Template;	//	テンプレート
	public:
		CTemplateNewer(CTrainEditMode *o, CTrainGroupTemplate *t){
			m_Owner = o; m_Template = t;
		}
		void Exec(){ m_Owner->NewFromTemplate(m_Template); }
	};
	class CTemplateAdder: public CMenuCommand{
	private:
		CTrainEditMode *m_Owner;			//	発行元
		CTrainGroupTemplate *m_Template;	//	テンプレート
	public:
		CTemplateAdder(CTrainEditMode *o, CTrainGroupTemplate *t){
			m_Owner = o; m_Template = t;
		}
		void Exec(){ m_Owner->AddFromTemplate(m_Template); }
	};
	class CTemplateDeleter: public CMenuCommand{
	private:
		CTrainGroupTemplate *m_Template;	//	テンプレート
	public:
		CTemplateDeleter(CTrainGroupTemplate *t){ m_Template = t; }
		void Exec(){
			g_TGTModalState = 21;
			g_TGTErasing = FindTrainGroupTemplate(m_Template);
			g_ModalDialog = g_TGTYesNoDialog = new CYesNoDialog(
				lang(DeleteTemplateCfmMessage),
				(char *)m_Template->GetName(), false);
		}
	};
	class CTemplateRenamer: public CMenuCommand{
	private:
		CTrainGroupTemplate *m_Template;	//	テンプレート
		CListElement *m_ListElement;		//	リスト要素
	public:
		CTemplateRenamer(CTrainGroupTemplate *t, CListElement *le){
			m_Template = t; m_ListElement = le;
		}
		void Exec(){
			if(m_ListElement) m_ListElement->BeginRename();
			else m_Template->GetTreeElement()->BeginRename();
		}
	};
	menu->GetMenu(0)->Enable(!g_NetworkInitialized);
	menu->GetMenu(0)->SetCommand(new CTemplateNewer(g_TrainEditMode, tmp));
	if(g_TrainGroup){
		menu->GetMenu(1)->Enable(!g_NetworkInitialized);
		menu->GetMenu(1)->SetCommand(new CTemplateAdder(g_TrainEditMode, tmp));
	}else{
		menu->GetMenu(1)->Enable(false);
	}
	menu->GetMenu(2)->SetCommand(new CTemplateDeleter(tmp));
	menu->GetMenu(3)->SetCommand(new CTemplateRenamer(tmp, renametarget));
}

/*
 *	編成セーブコマンド発行
 */
CMenuCommand *MakeGroupSaver(
	CTrainGroup *group	//	編成
){
	class CGroupSaver: public CMenuCommand{
	public:
		void Exec(){ AddTrainGroupTemplate(); }
	};
	return new CGroupSaver();
}

/*
 *	テンプレート追加
 */
void AddTrainGroupTemplate(){
	FILE *file;
	if(chdir(g_BaseDir) || chdir(TGT_DIRNAME)) return;
	int i;
	for(i = 1; i<1000; i++){
		char *tname = i>1 ? FlashIn("%s (%d)", lang(NewTemplate), i) : lang(NewTemplate);
		if(FindTrainGroupTemplate(tname)!=g_TrainGroupTemplateList.end()) continue;
		char *fname = FlashIn("%s.txt", tname);
		if(file = fopen(fname, "rb")){
			fclose(file);
			continue;
		}
		if(!(file = fopen(fname, "wt"))) continue;
		CPluginTree *tree = g_TrainEditMode->GetTree();
		CTreeDirElement *insertpoint = tree->GetRoot();
		g_TrainGroupTemplateList.push_back(CTrainGroupTemplate(tname));
		CTrainGroupTemplate *tgt = &*g_TrainGroupTemplateList.rbegin();
		g_TrainGroup->MakeTemplate(tgt);
		CTreeFileElement *fe = tgt->InsertItem(tree->GetRoot(), tree);
		tgt->Save(file);
		fclose(file);
		insertpoint->Sort(false);
		tree->SelectPlugin(tgt);
		fe->BeginRename();
		return;
	}
}

/*
 *	テンプレートリストアップ
 */
void ListTrainGroupTemplate(){
	CPluginTree *tree = g_TrainEditMode->GetTree();
	CTreeDirElement *root = tree->GetRoot();
	ITrainGroupTemplate itgt = g_TrainGroupTemplateList.begin();
	for(; itgt!=g_TrainGroupTemplateList.end(); itgt++){
		if(itgt->IsInserted()){
			continue;
		}
		CTreeFileElement *fe = itgt->InsertItem(root, tree);
	}
	root->Sort(false);
}

/*
 *	モーダル処理
 */
void ModalFuncTrainGroupTemplate(){
	CPluginTree *tree = g_TrainEditMode->GetTree();
	if(g_TGTYesNoDialog){
		if(g_TGTYesNoDialog->CheckYes()){
			DELETE_V(g_ModalDialog);
			g_TGTYesNoDialog = NULL;
			switch(g_TGTModalState){
			case 21: {
				CTreeDirElement *parent = g_TGTErasing->GetTreeElement()->GetParent();
				if(!g_TGTErasing->DeleteFromDisk()) return;
				g_TGTErasing->DeleteFromTree();
				g_TrainGroupTemplateList.erase(g_TGTErasing);
				parent->ListDirectory();
				break; }
			case 31: {
				string newname = g_TGTErasing->TextName2();
				if(!g_TGTErasing->DeleteFromDisk()) return;
				g_TGTErasing->DeleteFromTree();
				g_TrainGroupTemplateList.erase(g_TGTErasing);
				if(!g_TGTTarget->Rename(g_TGTTempString)) return;
				tree->SelectPlugin(&*g_TGTTarget);
				break; }
			}
		}else if(g_TGTYesNoDialog->CheckNo()){
			DELETE_V(g_ModalDialog);
			g_TGTYesNoDialog = NULL;
		}
	}
}
