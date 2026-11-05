#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CPopMenu.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CGirderPlugin.h"
#include "CPierPlugin.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"
#include "CRailwayPluginSet.h"
#include "CSkinPlugin.h"
#include "CRailBuildMode.h"

//	内部定数
const char *RPS_DIRNAME = "RailwayPluginSet";

//	内部グローバル
CPopMenu *g_RailwayPluginSetMenu = NULL;		//	プラグインセットメニュー
list<CRailwayPluginSet> g_RailwayPluginSetList;	//	プラグインセットリスト

int g_RPSModalState;					//	モーダル状態
string g_RPSTempString;					//	テンポラリ文字列
IRailwayPluginSet g_RPSTarget;			//	処理対象
IRailwayPluginSet g_RPSErasing;			//	削除対象
CYesNoDialog *g_RPSYesNoDialog = NULL;	//	問合せダイアログ
CInputDialog *g_RPSInputDialog = NULL;	//	入力ダイアログ

/*
 *	コンストラクタ
 */
CRailwayPluginSet::CRailwayPluginSet(
	char *name,	//	設定名
	bool create	//	現在の設定から作成
):
	CPlugin(lang(RailwayTemplate))	//	基本クラス
{
	m_Name = name;
	m_Author = "RailSim II";
	if(!create){
		m_State = 1;
		return;
	}
	m_State = 2;
	CRailwayMode::GetPlugin(&m_RailPlugin, &m_TiePlugin,
		&m_GirderPlugin, &m_PierPlugin, &m_LinePlugin, &m_PolePlugin);
	m_RailFlag = !!m_RailPlugin;
	m_TieFlag = !!m_TiePlugin;
	m_GirderFlag = !!m_GirderPlugin;
	m_PierFlag = !!m_PierPlugin;
	m_LineFlag = !!m_LinePlugin;
	m_PoleFlag = !!m_PolePlugin;
	if(m_RailPlugin = g_Rail) m_RailID = m_RailPlugin->GetID();
	if(m_TiePlugin = g_Tie) m_TieID = m_TiePlugin->GetID();
	if(m_GirderPlugin = g_Girder) m_GirderID = m_GirderPlugin->GetID();
	if(m_PierPlugin = g_Pier) m_PierID = m_PierPlugin->GetID();
	if(m_LinePlugin = g_Line) m_LineID = m_LinePlugin->GetID();
	if(m_PolePlugin = g_Pole) m_PoleID = m_PolePlugin->GetID();
	m_EnableCant = CRailwayMode::IsCantEnabled();
	m_LiftRailSurface = CRailwayMode::IsLiftRail();
	m_MultiTrack = CRailwayMode::IsMultiTrack();
	m_TrackNum = CRailwayMode::GetTrackNum();
	m_TrackInterval = CRailwayMode::GetTrackInterval();
}

/*
 *	デストラクタ
 */
CRailwayPluginSet::~CRailwayPluginSet(){
	DELETE_A(m_Buffer);
}

/*
 *	定義ファイルアタッチ
 */
bool CRailwayPluginSet::PreLoadRPS(
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
		if(datafiletype!=RPS_DIRNAME) throw CSynErr(eee, lang(InvalidDatafileType));
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
bool CRailwayPluginSet::Load(){
	char *str = m_Script, *eee;
	try{
		if(!(str = BeginBlock(eee = str, "RailwayPluginSet"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "SettingName", &m_Name))) throw CSynErr(eee);
		m_Name = RestoreDoubleQuote(m_Name);
		if(!(str = AsgnString(eee = str, "RailPlugin", &m_RailID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "RailCheck", &m_RailFlag))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "TiePlugin", &m_TieID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "TieCheck", &m_TieFlag))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "GirderPlugin", &m_GirderID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "GirderCheck", &m_GirderFlag))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "PierPlugin", &m_PierID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "PierCheck", &m_PierFlag))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "LinePlugin", &m_LineID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "LineCheck", &m_LineFlag))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "PolePlugin", &m_PoleID))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "PoleCheck", &m_PoleFlag))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "EnableCant", &m_EnableCant))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "LiftRailSurface", &m_LiftRailSurface))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "MultiTrack", &m_MultiTrack))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "TrackNum", &m_TrackNum))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "TrackInterval", &m_TrackInterval))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	m_RailPlugin = m_RailID.size()
		? g_RailPluginList->FindPlugin(m_RailID.c_str()) : NULL;
	m_TiePlugin = m_TieID.size()
		? g_TiePluginList->FindPlugin(m_TieID.c_str()) : NULL;
	m_GirderPlugin = m_GirderID.size()
		? g_GirderPluginList->FindPlugin(m_GirderID.c_str()) : NULL;
	m_PierPlugin = m_PierID.size()
		? g_PierPluginList->FindPlugin(m_PierID.c_str()) : NULL;
	m_LinePlugin = m_LineID.size()
		? g_LinePluginList->FindPlugin(m_LineID.c_str()) : NULL;
	m_PolePlugin = m_PoleID.size()
		? g_PolePluginList->FindPlugin(m_PoleID.c_str()) : NULL;
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	保存
 */
void CRailwayPluginSet::Save(
	FILE *file	//	設定ファイル
){
	fprintf(file, "DatafileHeader{\n");
	fprintf(file, "\tRailSimVersion = %.2f;\n", RAILSIM_VERSION);
	fprintf(file, "\tDatafileType = %s;\n", DirName());
	fprintf(file, "}\n\n");
	fprintf(file, "RailwayPluginSet{\n");
	fprintf(file, "\tSettingName = \"%s\";\n", ExpandDoubleQuote(m_Name).c_str());
	fprintf(file, "\tRailPlugin = \"%s\";\n", m_RailID.c_str());
	fprintf(file, "\tRailCheck = %s;\n", YESNO[m_RailFlag]);
	fprintf(file, "\tTiePlugin = \"%s\";\n", m_TieID.c_str());
	fprintf(file, "\tTieCheck = %s;\n", YESNO[m_TieFlag]);
	fprintf(file, "\tGirderPlugin = \"%s\";\n", m_GirderID.c_str());
	fprintf(file, "\tGirderCheck = %s;\n", YESNO[m_GirderFlag]);
	fprintf(file, "\tPierPlugin = \"%s\";\n", m_PierID.c_str());
	fprintf(file, "\tPierCheck = %s;\n", YESNO[m_PierFlag]);
	fprintf(file, "\tLinePlugin = \"%s\";\n", m_LineID.c_str());
	fprintf(file, "\tLineCheck = %s;\n", YESNO[m_LineFlag]);
	fprintf(file, "\tPolePlugin = \"%s\";\n", m_PoleID.c_str());
	fprintf(file, "\tPoleCheck = %s;\n", YESNO[m_PoleFlag]);
	fprintf(file, "\tEnableCant = %s;\n", YESNO[m_EnableCant]);
	fprintf(file, "\tLiftRailSurface = %s;\n", YESNO[m_LiftRailSurface]);
	fprintf(file, "\tMultiTrack = %s;\n", YESNO[m_MultiTrack]);
	fprintf(file, "\tTrackNum = %d;\n", m_TrackNum);
	fprintf(file, "\tTrackInterval = %f;\n", m_TrackInterval);
	fprintf(file, "}\n");
}

/*
 *	データファイル削除
 */
bool CRailwayPluginSet::DeleteFromDisk(){
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
bool CRailwayPluginSet::Rename(
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
	return true;
}

/*
 *	プラグインセット適用
 */
void CRailwayPluginSet::Apply(){
	LoadAndGet();
	g_Rail = m_RailPlugin;
	g_Tie = m_TiePlugin;
	g_Girder = m_GirderPlugin;
	g_Pier = m_PierPlugin;
	g_Line = m_LinePlugin;
	g_Pole = m_PolePlugin;
	CRailwayMode::SetRailwayOption(
		m_RailFlag, m_TieFlag, m_GirderFlag, m_PierFlag, m_LineFlag, m_PoleFlag,
		m_EnableCant, m_LiftRailSurface, m_MultiTrack, m_TrackNum, m_TrackInterval);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	設定リスト読込
 */
void LoadRailwayPluginSetList(){
	long filelist;
	_finddata_t data;
	if(chdir(g_BaseDir) || chdir(RPS_DIRNAME)) return;
	if((filelist = _findfirst("*.txt", &data))>=0){
		do{
			FILE *file;
			if(data.attrib&_A_SUBDIR) continue;
			string name = data.name;
			name[name.size()-4] = 0;
			g_RailwayPluginSetList.push_back(CRailwayPluginSet((char *)name.c_str(), false));
			CRailwayPluginSet *rps = &*g_RailwayPluginSetList.rbegin();
			if(file = fopen(data.name, "rb")){
				if(!rps->PreLoadRPS(file)){
					g_RailwayPluginSetList.pop_back();
					continue;
				}
			}else{
				g_RailwayPluginSetList.pop_back();
				continue;
			}
		} while(!_findnext(filelist, &data));
		_findclose(filelist);
	}
	g_RailwayPluginSetList.sort();
}

/*
 *	プラグインセット検索
 */
IRailwayPluginSet FindRailwayPluginSet(
	string name	//	設定名
){
	IRailwayPluginSet irps = g_RailwayPluginSetList.begin();
	for(; irps!=g_RailwayPluginSetList.end(); irps++)
		if(name==irps->GetNameRef()) return irps;
	return irps;
}

/*
 *	プラグインセット追加
 */
void AddRailwayPluginSet(
	string name,	//	設定名
	bool over		//	上書き
){
	if(!name.size()) return;
	IRailwayPluginSet irps = FindRailwayPluginSet(name);
	if(irps!=g_RailwayPluginSetList.end()){
		if(over){
			if(!irps->DeleteFromDisk()) return;
			g_RailwayPluginSetList.erase(irps);
		}else{
			g_RPSModalState = 11;
			g_RPSTempString = string(name);
			g_ModalDialog = g_RPSYesNoDialog = new CYesNoDialog(
				lang(ReplaceCfmMessage), (char *)name.c_str(), false);
			return;
		}
	}
	char *fname = FlashIn("%s.txt", name.c_str());
	FILE *file;
	if(CheckSlash(fname) || chdir(g_BaseDir) || chdir(RPS_DIRNAME)
		|| !(file = fopen(fname, "wt"))){
		EnqueueCommonDialog(new CSimpleDialog(
			lang(CannotOpen_CheckFileName), fname));
		return;
	}
	g_RailwayPluginSetList.push_back(CRailwayPluginSet((char *)name.c_str(), true));
	g_RailwayPluginSetList.rbegin()->Save(file);
	fclose(file);
	g_RailwayPluginSetList.sort();
	ListRailwayPluginSet();
}

/*
 *	プラグインセットリストアップ
 */
void ListRailwayPluginSet(){
	class CSettingAdder: public CMenuCommand{
	public:
		void Exec(){
			g_RPSModalState = 10;
			g_ModalDialog = g_RPSInputDialog = new CInputDialog(
				lang(FileNameInquireMessage), lang(SaveSettings), "", 64);
		}
	};
	class CSettingApplier: public CMenuCommand{
	private:
		IRailwayPluginSet m_PluginSet;	//	プラグインセット
	public:
		CSettingApplier(IRailwayPluginSet s){ m_PluginSet = s; }
		void Exec(){ m_PluginSet->Apply(); }
	};
	class CSettingDeleter: public CMenuCommand{
	private:
		IRailwayPluginSet m_PluginSet;	//	プラグインセット
	public:
		CSettingDeleter(IRailwayPluginSet s){ m_PluginSet = s; }
		void Exec(){
			g_RPSModalState = 21;
			g_RPSErasing = m_PluginSet;
			g_ModalDialog = g_RPSYesNoDialog = new CYesNoDialog(
				lang(SettingDeleteCfmMessage),
				(char *)m_PluginSet->GetName(), false);
		}
	};
	class CSettingRenamer: public CMenuCommand{
	private:
		IRailwayPluginSet m_PluginSet;	//	プラグインセット
	public:
		CSettingRenamer(IRailwayPluginSet s){ m_PluginSet = s; }
		void Exec(){
			g_RPSModalState = 30;
			g_RPSTarget = m_PluginSet;
			g_ModalDialog = g_RPSInputDialog = new CInputDialog(
				lang(FileNameInquireMessage), lang(ChangeName), m_PluginSet->GetName(), 64);
		}
	};
	DELETE_V(g_RailwayPluginSetMenu);
	g_RailwayPluginSetMenu = new CPopMenu("", NULL);
	(new CPopMenu(lang(SaveSettingNew), g_RailwayPluginSetMenu))->SetCommand(new CSettingAdder);
	new CPopMenu("-", g_RailwayPluginSetMenu);
	IRailwayPluginSet irps = g_RailwayPluginSetList.begin();
	for(; irps!=g_RailwayPluginSetList.end(); irps++){
		CPopMenu *entry = new CPopMenu(irps->GetName(), g_RailwayPluginSetMenu);
		(new CPopMenu(lang(Apply), entry))->SetCommand(new CSettingApplier(irps));
		(new CPopMenu(lang(Delete), entry))->SetCommand(new CSettingDeleter(irps));
		(new CPopMenu(lang(ChangeName), entry))->SetCommand(new CSettingRenamer(irps));
	}
}

/*
 *	モーダル処理
 */
void ModalFuncRailwayPluginSet(){
	if(g_RPSInputDialog){
		if(g_RPSInputDialog->CheckOK()){
			g_RPSTempString = g_RPSInputDialog->GetInputText();
			DELETE_V(g_ModalDialog);
			g_RPSInputDialog = NULL;
			switch(g_RPSModalState){
			case 10:	//	add new setting
				AddRailwayPluginSet((char *)g_RPSTempString.c_str(), false);
				break;
			case 30:
				if(g_RPSTarget->GetNameRef()==g_RPSTempString) break;
				g_RPSErasing = FindRailwayPluginSet((char *)g_RPSTempString.c_str());
				if(g_RPSErasing!=g_RailwayPluginSetList.end()){
					g_RPSModalState = 31;
					g_ModalDialog = g_RPSYesNoDialog = new CYesNoDialog(
						FlashIn("\"%s\": %s", (char *)g_RPSTempString.c_str(),
						lang(ReplaceCfmMessage)), lang(OverlappedSettingName), false);
					return;
				}else{
					if(!g_RPSTarget->Rename(g_RPSTempString)) return;
					g_RailwayPluginSetList.sort();
					ListRailwayPluginSet();
				}
				break;
			}
		}else if(g_RPSInputDialog->CheckCancel()){
			DELETE_V(g_ModalDialog);
			g_RPSInputDialog = NULL;
		}
	}
	if(g_RPSYesNoDialog){
		if(g_RPSYesNoDialog->CheckYes()){
			DELETE_V(g_ModalDialog);
			g_RPSYesNoDialog = NULL;
			switch(g_RPSModalState){
			case 11:	//	add new setting confirmed overwrite
				AddRailwayPluginSet((char *)g_RPSTempString.c_str(), true);
				break;
			case 21:
				if(!g_RPSErasing->DeleteFromDisk()) return;
				g_RailwayPluginSetList.erase(g_RPSErasing);
				ListRailwayPluginSet();
				break;
			case 31:
				if(!g_RPSErasing->DeleteFromDisk()) return;
				g_RailwayPluginSetList.erase(g_RPSErasing);
				if(!g_RPSTarget->Rename(g_RPSTempString)) return;
				g_RailwayPluginSetList.sort();
				ListRailwayPluginSet();
				break;
			}
		}else if(g_RPSYesNoDialog->CheckNo()){
			DELETE_V(g_ModalDialog);
			g_RPSYesNoDialog = NULL;
		}
	}
}
