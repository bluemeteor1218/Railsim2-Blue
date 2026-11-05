#include "stdafx.h"
#include "md5.h"
#include "Script.h"
#include "Network.h"
#include "CSimpleDialog.h"
#include "CSaveFile.h"
#include "CSkinPlugin.h"
#include "CSimulationMode.h"
#include "CFileMode.h"
#include "CConfigMode.h"

//	外部定数
extern const int RSN_SCRNAME_MAX;
extern const int RSN_SYNC_INTERVAL_MIN;
extern const int RSN_SYNC_INTERVAL_MAX;

//	内部定数
const int WW = 512, WH = 384;	//	窓サイズ
const int NWW = 384, NWH = 256;	//	ネットワーク窓サイズ
const int UNDO_MAX = 32;		//	アンドゥ回数
extern const char *LAYOUT_DIRNAME = "Layout";
extern const char *UNDO_DIRNAME = "Undo";

//	外部グローバル
extern float g_SkinFileIconRect[4];

extern int g_LastJoinedHostIP[];
extern int g_LastJoinedHostPort;
extern int g_LastJoinedLocalPort;
extern int g_LastCreatedHostPort;
extern string g_LastScreenName;

//	内部グローバル
list<CLayoutInfo> g_LayoutInfoList;	//	セーブファイルリスト
CInputDialog *g_NetworkConnectionInputDialog = NULL;	//	入力ダイアログ

/*
 *	ヘッダ読込
 */
bool CLayoutInfo::PreLoadSF(
	FILE *file	//	ファイル
){
	char *buf = LoadBinaryText(file, 1024), *str = buf, *eee;
	try{
		if(!(str = Space(eee = str))) return false;
		if(!(str = BeginBlock(eee = str, "DatafileHeader"))) return false;
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
	}
	catch(CSynErr err){
		err.Handle(FlashIn("%s <%s>", m_FileName.c_str(), "Layout"), buf);
		return false;
	}
	DELETE_A(buf);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	リネーム確認
 */
bool CFileListView::ConfirmRename(
	CListElement *item,	//	アイテム
	string &newname		//	新規名
){
	char *oldname = item->GetString(0);
	newname = FixFileExt((char *)newname.c_str(), "rs2");
	if(!_mbsicmp((PUCHAR)oldname, (PUCHAR)newname.c_str())) return false;
	if(CheckSlash(newname.c_str()) || chdir(g_BaseDir) || chdir(LAYOUT_DIRNAME) ||
		rename(oldname, newname.c_str())){
		FILE *file = fopen(newname.c_str(), "rb");
		if(file){
			EnqueueCommonDialog(new CSimpleDialog(
				lang(FileAlreadyExists), (char *)newname.c_str()));
			fclose(file);
		}else{
			EnqueueCommonDialog(new CSimpleDialog(
				lang(ErrorDuringRename), oldname));
		}
		g_Skin->Error();
		return false;
	}
	g_SaveFile->SetFileName((char *)newname.c_str());
	return true;
}

/*
 *	リネーム終了処理
 */
void CFileListView::EndRename(
	CListElement *item	//	アイテム
){
	g_FileMode->ListFile();
}

/*
 *	ダブルクリック
 */
void CFileListView::DoubleClick(){
	if(g_NetworkInitialized){
		g_Skin->Error();
		return;
	}
	CListElement *le = GetFocusItem();
	if(le && le->IsSelected()) g_FileMode->OpenFile(le->GetString(0));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CFileMode::CFileMode(){
	int wide = WW-TILE_UNIT*2, win = wide-TILE_UNIT*2;
	m_FileWindow.Init((g_DispWidth-WW)/2-TILE_UNIT, (g_DispHeight-WH)/2,
		WW, WH, lang(File), &m_Interface, true);
	m_FileWindow.SetResize(TILE_UNIT*27+TILE_HALF*4, TILE_UNIT*14,
		g_DispWidth, g_DispHeight, this);
	m_FileLabel.Init(TILE_UNIT, TILE_UNIT*2, 1, 1, "", &m_FileWindow, 0, 1);
	m_NoteLabel.Init(TILE_UNIT, TILE_UNIT*3+TILE_HALF,
		TILE_UNIT*2, TILE_UNIT, lang(Description), &m_FileWindow, 0, 1);
	m_NoteEdit.Init(TILE_UNIT*3, TILE_UNIT*3+TILE_HALF, 1, 1, "", &m_FileWindow, 64);
	m_NewButton.Init(TILE_UNIT, TILE_UNIT*5,
		TILE_UNIT*5, TILE_UNIT, lang(NewFile), &m_FileWindow);
	m_OpenButton.Init(TILE_UNIT*6+TILE_HALF, TILE_UNIT*5,
		TILE_UNIT*5, TILE_UNIT, lang(Open), &m_FileWindow);
	m_SaveButton.Init(TILE_UNIT*11+TILE_HALF*2, TILE_UNIT*5,
		TILE_UNIT*5, TILE_UNIT, lang(Overwrite), &m_FileWindow);
	m_SaveAsButton.Init(TILE_UNIT*16+TILE_HALF*3, TILE_UNIT*5,
		TILE_UNIT*5, TILE_UNIT, lang(SaveAs), &m_FileWindow);
	m_NetworkButton.Init(TILE_UNIT*21+TILE_HALF*4, TILE_UNIT*5,
		TILE_UNIT*5, TILE_UNIT, lang(Download), &m_FileWindow);
	char *fcol[3] = {lang(FileName), lang(Date), lang(Description)};
	m_FileListView.Init(TILE_UNIT, TILE_UNIT*6+TILE_HALF, WW-TILE_UNIT*2, 1,
		&m_FileWindow, 3, fcol, DRAG_NONE, LISTVIEW_RENAMABLE, this, CMD_FILE);
	m_AutoLoadCheck.Init(0, 0, 1, 1, lang(AutoReload), &m_FileWindow);
	m_AutoLoadCheck.SetCheck(1);
	WindowResized(WW, WH, &m_FileWindow);
	m_NetworkWindow.Init((g_DispWidth-WW)/2-TILE_UNIT*2, (g_DispHeight-WH)/2+TILE_UNIT,
		NWW, NWH, lang(Network), &m_Interface, true);
	m_NetworkWindow.SetResize(TILE_UNIT*17+TILE_HALF*2, TILE_UNIT*10,
		g_DispWidth, g_DispHeight, this);
	m_NetworkLabel.Init(TILE_UNIT, TILE_UNIT*2, 1, 1, "", &m_NetworkWindow, 0, 1);
	m_CloseEntryButton.Init(TILE_UNIT, TILE_UNIT*3+TILE_HALF,
		TILE_UNIT*5, TILE_UNIT, lang(CloseEntry), &m_NetworkWindow);
	m_DeleteMemberButton.Init(TILE_UNIT*6+TILE_HALF, TILE_UNIT*3+TILE_HALF,
		TILE_UNIT*5, TILE_UNIT, lang(DeleteMember), &m_NetworkWindow);
	m_DisconnectButton.Init(TILE_UNIT*11+TILE_HALF*2, TILE_UNIT*3+TILE_HALF,
		TILE_UNIT*5, TILE_UNIT, lang(Disconnect), &m_NetworkWindow);
	char *ncol[2] = {lang(ScreenName), lang(State)};
	m_NetworkListView.Init(TILE_UNIT, TILE_UNIT*5, NWW-TILE_UNIT*2, 1,
		&m_NetworkWindow, 2, ncol, DRAG_NONE, 0, this, CMD_NETWORK);
	WindowResized(NWW, NWH, &m_NetworkWindow);
	m_NetworkWindow.Show(false);
	m_YesNoDialog = NULL;
	m_InputDialog = NULL;
	m_MultiInputDialog = NULL;
	m_FileMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(Open), m_FileMenu);
	new CPopMenu(lang(Overwrite), m_FileMenu);
	new CPopMenu(lang(Delete), m_FileMenu);
	new CPopMenu(lang(ChangeName), m_FileMenu);
	new CPopMenu("-", m_FileMenu);
	new CPopMenu(lang(CreateSession), m_FileMenu);
	new CPopMenu(lang(JoinSession), m_FileMenu);
	chdir(g_BaseDir);
	if(chdir(LAYOUT_DIRNAME)) mkdir(LAYOUT_DIRNAME);
	chdir(g_BaseDir);
	if(chdir(UNDO_DIRNAME)) mkdir(UNDO_DIRNAME);
	chdir(g_BaseDir);
	ResetUndo();
	m_NetworkMode = false;
}

/*
 *	コンストラクタ
 */
CFileMode::~CFileMode(){
	DELETE_V(m_FileMenu);
}

/*
 *	ウィンドウリサイズ
 */
void CFileMode::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	if(wnd==&m_FileWindow){
		m_FileLabel.SetSize(w-TILE_UNIT*2, TILE_UNIT);
		m_NoteEdit.SetSize(w-TILE_UNIT*4, TILE_UNIT);
		m_FileListView.SetSize(w-TILE_UNIT*2, h-TILE_UNIT*9);
		m_AutoLoadCheck.SetPos(TILE_UNIT, h-TILE_UNIT*2);
		m_AutoLoadCheck.SetSize(w-TILE_UNIT*2, TILE_UNIT);
	}else if(wnd==&m_NetworkWindow){
		m_NetworkLabel.SetSize(w-TILE_UNIT*2, TILE_UNIT);
		m_NetworkListView.SetSize(w-TILE_UNIT*2, h-TILE_UNIT*6);
	}
}

/*
 *	メニュー発行
 */
CPopMenu *CFileMode::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	if(type==CMD_FILE){
		class CFileOpener: public CMenuCommand{
		private:
			string m_FileName;	//	ファイル名
		public:
			CFileOpener(char *fname){ m_FileName = fname; }
			void Exec(){
				g_FileMode->OpenFile((char *)m_FileName.c_str());
			}
		};
		class CFileSaver: public CMenuCommand{
		private:
			string m_FileName;	//	ファイル名
		public:
			CFileSaver(char *fname){ m_FileName = fname; }
			void Exec(){
				g_FileMode->SaveFile((char *)m_FileName.c_str());
			}
		};
		class CFileDeleter: public CMenuCommand{
		private:
			bool m_Confirm;		//	確認
			string m_FileName;	//	ファイル名
		public:
			CFileDeleter(bool c, char *fname){ m_Confirm = c; m_FileName = fname; }
			void Exec(){
				if(m_Confirm){
					if(chdir(g_BaseDir) || chdir(LAYOUT_DIRNAME)
						|| remove(m_FileName.c_str())){
						EnqueueCommonDialog(new CSimpleDialog(
							lang(ErrorDuringDelete), (char *)m_FileName.c_str()));
						g_Skin->Error();
					}else{
						g_FileMode->ListFile();
					}
				}else{
					CYesNoDialog *dlg = new CYesNoDialog(
						lang(DeleteCfmMessage), (char *)m_FileName.c_str(), false);
					dlg->SetYesCommand(new CFileDeleter(true, (char *)m_FileName.c_str()));
					EnqueueCommonDialog(dlg);
				}
			}
		};
		class CFileRenamer: public CMenuCommand{
		private:
			CListElement *m_ListElement;	//	リスト要素
		public:
			CFileRenamer(CListElement *le){ m_ListElement = le; }
			void Exec(){ m_ListElement->BeginRename(); }
		};
		class CNetworkStarter: public CMenuCommand{
		private:
			string m_FileName;	//	ファイル名
		public:
			CNetworkStarter(char *fname){ m_FileName = fname; }
			void Exec(){
				if(g_NetworkInitialized){
					g_Skin->Error();
					return;
				}
				g_FileMode->BeginNetworkHost((char *)m_FileName.c_str());
			}
		};
		class CNetworkConnector: public CMenuCommand{
		private:
			string m_FileName;	//	ファイル名
		public:
			CNetworkConnector(char *fname){ m_FileName = fname; }
			void Exec(){
				if(g_NetworkInitialized){
					g_Skin->Error();
					return;
				}
				g_FileMode->BeginNetworkJoin((char *)m_FileName.c_str());
			}
		};
		CListElement *le = (CListElement *)data;
		if(!le) return NULL;
		m_FileMenu->GetMenu(0)->Enable(!g_NetworkInitialized);
		m_FileMenu->GetMenu(0)->SetCommand(new CFileOpener(le->GetString(0)));
		m_FileMenu->GetMenu(1)->SetCommand(new CFileSaver(le->GetString(0)));
		m_FileMenu->GetMenu(2)->SetCommand(new CFileDeleter(false, le->GetString(0)));
		m_FileMenu->GetMenu(3)->SetCommand(new CFileRenamer(le));
		m_FileMenu->GetMenu(5)->Enable(!g_NetworkInitialized);
		m_FileMenu->GetMenu(5)->SetCommand(new CNetworkStarter(le->GetString(0)));
		m_FileMenu->GetMenu(6)->Enable(!g_NetworkInitialized);
		m_FileMenu->GetMenu(6)->SetCommand(new CNetworkConnector(le->GetString(0)));
		return m_FileMenu;
	}
	return NULL;
}

/*
 *	設定読込
 */
char *CFileMode::LoadInterfaceSetting(
	char *str	//	対象文字列
){
	char *eee;
	bool autoload = true;
	string lastfile;
	if(str){
		if(!(str = BeginBlock(eee = str, "FileMode"))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "AutoLoad", &autoload))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "LastFile", &lastfile))) throw CSynErr(eee);
		if(g_ConfigVersion>=2.10f){
			if(!(str = AsgnInteger(eee = str, "LastJoinedHostIP",
				g_LastJoinedHostIP, 4, false))) throw CSynErr(eee);
			if(!(str = AsgnInteger(eee = str, "LastJoinedHostPort",
				&g_LastJoinedHostPort))) throw CSynErr(eee);
			if(!(str = AsgnInteger(eee = str, "LastJoinedLocalPort",
				&g_LastJoinedLocalPort))) throw CSynErr(eee);
			if(!(str = AsgnInteger(eee = str, "LastCreatedHostPort",
				&g_LastCreatedHostPort))) throw CSynErr(eee);
			if(!(str = AsgnString(eee = str, "LastScreenName",
				&g_LastScreenName))) throw CSynErr(eee);
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}else{
		lastfile = "Sample.rs2";
	}
	m_AutoLoadCheck.SetCheck(autoload);
	if(!g_RSPV){
		if(autoload && lastfile.size()){
			g_SaveFile = new CSaveFile(false);
			if(!g_SaveFile->Load(lastfile.c_str(),
				LAYOUT_DIRNAME, false, true, NULL, NULL, false, NULL)){
				DELETE_V(g_SaveFile);
				g_SaveFile = new CSaveFile(true);
			}
		}else{
			g_SaveFile = new CSaveFile(true);
		}
	}
	return str;
}

/*
 *	設定保存
 */
void CFileMode::SaveInterfaceSetting(
	FILE *file	//	ファイル
){
	fprintf(file, "FileMode{\n");
	fprintf(file, "\tAutoLoad = %s;\n", YESNO[m_AutoLoadCheck.GetCheck()]);
	fprintf(file, "\tLastFile = \"%s\";\n", g_SaveFile->GetFileName());
	fprintf(file, "\tLastJoinedHostIP = %d, %d, %d, %d;\n",
		g_LastJoinedHostIP[0], g_LastJoinedHostIP[1],
		g_LastJoinedHostIP[2], g_LastJoinedHostIP[3]);
	fprintf(file, "\tLastJoinedHostPort = %d;\n", g_LastJoinedHostPort);
	fprintf(file, "\tLastJoinedLocalPort = %d;\n", g_LastJoinedLocalPort);
	fprintf(file, "\tLastCreatedHostPort = %d;\n", g_LastCreatedHostPort);
	fprintf(file, "\tLastScreenName = \"%s\";\n", g_LastScreenName.c_str());
	fprintf(file, "}\n\n");
}

/*
 *	モードを有効化
 */
void CFileMode::EnterInterface(){
	ms_ModeLabel = lang(File);
	UpdateFileName();
	UpdateFileNote();
	ListFile();
	if(g_NetworkInitialized) m_NetworkListView.GiveFocus(false);
	else m_FileListView.GiveFocus(false);
}

/*
 *	モーダル処理
 */
void CFileMode::ModalFuncInterface(){
	int i;
	if(m_YesNoDialog){
		if(m_YesNoDialog->CheckYes()){
			DELETE_V(g_ModalDialog);
			m_YesNoDialog = NULL;
			switch(m_ModalState){
			case 10:
				if(g_NetworkInitialized){
					g_Skin->Error();
					break;;
				}
				ResetUndo();
				DELETE_V(g_SaveFile);
				g_SaveFile = new CSaveFile(true);
				g_SimulationMode->InitTimeOption();
				SetNeutral();
				break;
			case 20:
				if(g_NetworkInitialized){
					g_Skin->Error();
					break;;
				}
				ResetUndo();
				DELETE_V(g_SaveFile);
				DELETE_A(g_NetworkFileCopy);
				g_SaveFile = new CSaveFile(false);
				g_SaveFile->Load(m_NewFileName.c_str(),
					LAYOUT_DIRNAME, true, true, NULL, NULL, false, NULL);
				SetNeutral();
				break;
			case 30:
				if(!g_SaveFile->Save(
					m_NewFileName.c_str(), LAYOUT_DIRNAME, true, true)) ListFile();
				break;
			case 80:
				g_NetworkCloseRequest = true;
				break;
			case 90:
				RSNDeleteMember(m_DeletingMemberID);
				break;
			case 100: {
				m_CloseEntryButton.Enable(false);
				void StartNetworkSimulation();
				StartNetworkSimulation();
				SetNeutral();
				break; }
			}
		}else if(m_YesNoDialog->CheckNo()){
			DELETE_V(g_ModalDialog);
			m_YesNoDialog = NULL;
		}
	}else if(m_InputDialog){
		if(m_InputDialog->CheckCancel()){
			DELETE_V(g_ModalDialog);
			m_InputDialog = NULL;
		}else if(m_InputDialog->CheckOK()){
			string input = m_InputDialog->GetInputText();
			DELETE_V(g_ModalDialog);
			m_InputDialog = NULL;
			bool join_with_file = false;
			switch(m_ModalState){
			case 40:
				input = FixFileExt((char *)input.c_str(), "rs2");
				SaveFile((char *)input.c_str());
				break;
			}
		}
	}else if(m_MultiInputDialog){
		if(m_MultiInputDialog->CheckCancel()){
			DELETE_V(g_ModalDialog);
			m_MultiInputDialog = NULL;
		}else if(m_MultiInputDialog->CheckOK()){
			vector<string> input;
			for(i = 0; i<m_MultiInputDialog->GetItemNumber(); i++)
				input.push_back(m_MultiInputDialog->GetInputText(i));
			DELETE_V(g_ModalDialog);
			m_MultiInputDialog = NULL;
			bool join_with_file = false;
			switch(m_ModalState){
			case 70:
				join_with_file = true;
			case 50: {
				if(sscanf(input[1].c_str(), "%d", &g_NetworkLocalPort)!=1
					|| !CheckPortArea(g_NetworkLocalPort)){
					EnqueueCommonDialog(new CSimpleDialog(
						lang(InvalidLocalPort), lang(NetworkError)));
					break;
				}
				switch(RSNJoinSession(input[0].c_str(), g_NetworkLocalPort,
					join_with_file, input[2].c_str())){
				case 0: {
					SwitchNetwork(true, false);
					m_NetworkLabel.SetText(lang(JoinedSession));
					if(join_with_file){
						m_NewFileName = m_NetworkFileName;
						ResetUndo();
						DELETE_V(g_SaveFile);
						DELETE_A(g_NetworkFileCopy);
						g_SaveFile = new CSaveFile(false);
						g_SaveFile->Load(m_NewFileName.c_str(),
							LAYOUT_DIRNAME, true, true, NULL, NULL, true, NULL);
						//SetNeutral();
					}
					break; }
				case 2:
					EnqueueCommonDialog(new CSimpleDialog(
						lang(ScreenNameEmpty), lang(NetworkError)));
					break;
				case 4:
					EnqueueCommonDialog(new CSimpleDialog(
						lang(InvalidAddressFormat), lang(NetworkError)));
					break;
				default:
					EnqueueCommonDialog(new CSimpleDialog(
						lang(FailedJoinSession), lang(NetworkError)));
					break;
				}
				break; }
			case 60: {
				if(sscanf(input[0].c_str(), "%d", &g_NetworkHostPort)!=1
					|| !CheckPortArea(g_NetworkHostPort)){
					EnqueueCommonDialog(new CSimpleDialog(
						lang(InvalidHostPort), lang(NetworkError)));
					break;
				}
				if(sscanf(input[1].c_str(), "%d", &g_NetworkSyncInterval)!=1
					|| g_NetworkSyncInterval<RSN_SYNC_INTERVAL_MIN
					|| g_NetworkSyncInterval>RSN_SYNC_INTERVAL_MAX){
					EnqueueCommonDialog(new CSimpleDialog(
						lang(InvalidSyncInterval), lang(NetworkError)));
					break;
				}
				m_NewFileName = m_NetworkFileName;
				ResetUndo();
				DELETE_V(g_SaveFile);
				DELETE_A(g_NetworkFileCopy);
				g_SaveFile = new CSaveFile(false);
				if(!g_SaveFile->Load(m_NewFileName.c_str(), LAYOUT_DIRNAME, true, true,
					&g_NetworkFileCopy, &g_NetworkFileCopySize, false, NULL)){
					DELETE_A(g_NetworkFileCopy);
				}
				switch(RSNCreateSession(g_NetworkHostPort,
					g_NetworkSyncInterval, input[2].c_str())){
				case 0:
					SwitchNetwork(true, true);
					m_NetworkLabel.SetText(lang(CreatedSession));
					break;
				case 2:
					EnqueueCommonDialog(new CSimpleDialog(
						lang(ScreenNameEmpty), lang(NetworkError)));
					break;
				default:
					EnqueueCommonDialog(new CSimpleDialog(
						lang(FailedCreateSession), lang(NetworkError)));
					break;
				}
				//SetNeutral();
				break; }
			}
		}
	}else if(m_InputDialog){
		if(m_InputDialog->CheckCancel()){
			DELETE_V(g_ModalDialog);
			m_InputDialog = NULL;
		}else if(m_InputDialog->CheckOK()){
			string input = m_InputDialog->GetInputText();
			DELETE_V(g_ModalDialog);
			m_InputDialog = NULL;
		}
	}
}

/*
 *	入力チェック
 */
void CFileMode::ScanInputInterface(){
	m_Interface.ScanInput();
	if(m_FileWindow.CheckClose() || m_NetworkWindow.CheckClose()){
		SetNeutral();
		return;
	}
	g_SaveFile->m_FileNote = m_NoteEdit.GetRealtimeText();
	if(m_NewButton.IsPushed()){
		m_ModalState = 10;
		g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
			lang(NewFileCfmMessage), lang(NewFile), false);
	}else if(m_OpenButton.IsPushed()){
		CListElement *le = m_FileListView.GetFocusItem();
		if(le && le->IsSelected()) OpenFile(le->GetString(0));
	}else if(m_SaveButton.IsPushed()){
		CListElement *le = m_FileListView.GetFocusItem();
		m_NewFileName = le && le->IsSelected() ? le->GetString(0) : g_SaveFile->m_FileName;
		if(m_NewFileName.size()){
			if(_mbsicmp((PUCHAR)m_NewFileName.c_str(), (PUCHAR)g_SaveFile->GetFileName()))
				SaveFile((char *)m_NewFileName.c_str());
			else if(!g_SaveFile->Save(
				m_NewFileName.c_str(), LAYOUT_DIRNAME, true, true)) ListFile();
		}else{
			goto SAVEAS;
		}
	}else if(m_SaveAsButton.IsPushed()){
SAVEAS:
		m_ModalState = 40;
		g_ModalDialog = m_InputDialog = new CInputDialog(
			lang(FileNameInquireMessage_NoExt),
			g_SaveFile->m_FileName.size() ? lang(SaveAs) : lang(SaveNew),
			(char *)g_SaveFile->GetFileName(), 64);
	}else if(m_NetworkButton.IsPushed()){
		if(g_NetworkInitialized){
			g_Skin->Error();
		}else{
			BeginNetworkJoin(NULL);
		}
	}else if(m_CloseEntryButton.IsPushed()){
		m_CloseEntryButton.SetPush(false);
		m_ModalState = 100;
		g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
			lang(CloseEntryCfmMessage), lang(Confirmation), false);
	}else if(m_DeleteMemberButton.IsPushed()){
		int sel = m_NetworkListView.GetSelectionMark();
		if(sel<=0){
			g_Skin->Error();
		}else{
			m_DeletingMemberID = m_NetworkListView.GetElement(sel)->GetData();
			m_DeleteMemberButton.SetPush(false);
			m_ModalState = 90;
			g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
				lang(DeleteMemberCfmMessage), lang(Confirmation), false);
		}
	}else if(m_DisconnectButton.IsPushed()){
		m_DisconnectButton.SetPush(false);
		m_ModalState = 80;
		g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
			lang(DisconnectCfmMessage), lang(Confirmation), false);
	}
	if(g_NetworkInitialized){
		ListNetworkMember(&m_NetworkListView);
		if(GetNetworkTransferState()==RSN_TRANS_LAYOUT){
			char *dlstate = FlashIn("%s %d%%", lang(ReceivingLayoutData),
				(g_NetworkTransferSize-g_NetworkTransferRestSize)*100/g_NetworkTransferSize);
			if(IsNetworkTransferComplete() && !strcmp(m_NetworkLabel.GetText(), dlstate)){
				m_NewFileName = "";
				ResetUndo();
				DELETE_V(g_SaveFile);
				g_SaveFile = new CSaveFile(false);
				MD5 hash;
				hash.update((unsigned char *)g_NetworkTransferData, g_NetworkTransferSize-1);
				hash.finalize();
				CheckLayoutDigest(hash.raw_digest());
				g_SaveFile->Load(m_NewFileName.c_str(),
					LAYOUT_DIRNAME, true, true, NULL, NULL, false, g_NetworkTransferData);
				//SetNeutral();
				RSNEndTransferData();
				m_NetworkLabel.SetText(lang(JoinedSession));
			}else{
				m_NetworkLabel.SetText(dlstate);
			}
		}
	}else{
		if(m_NetworkMode) SwitchNetwork(false, false);
	}
}

/*
 *	ファイルリスト作成
 */
void CFileMode::ListFile(){
	long filelist;
	_finddata_t data;
	m_FileListView.DeleteAllItems();
	m_LayoutInfoList.clear();
	if(chdir(g_BaseDir) || chdir(LAYOUT_DIRNAME)) return;
	if((filelist = _findfirst("*.rs2", &data))>=0){
		do{
			FILE *file;
			if(data.attrib&_A_SUBDIR) continue;
			m_LayoutInfoList.push_back(CLayoutInfo(data.name));
			CLayoutInfo *nfo = &*m_LayoutInfoList.rbegin();
			if(file = fopen(data.name, "rb")){
				if(!nfo->PreLoadSF(file)){
					m_LayoutInfoList.pop_back();
					continue;
				}
			}else{
				m_LayoutInfoList.pop_back();
				continue;
			}
		} while(!_findnext(filelist, &data));
		_findclose(filelist);
	}
	m_LayoutInfoList.sort();
	ILayoutInfo ili = m_LayoutInfoList.begin();
	int current = -1;
	for(; ili!=m_LayoutInfoList.end(); ili++){
		if(!_mbsicmp((PUCHAR)ili->m_FileName.c_str(),
			(PUCHAR)g_SaveFile->GetFileName())){
			current = m_FileListView.GetItemNum();
			g_SaveFile->SetFileName((char *)ili->m_FileName.c_str());
		}
		CListElement *le = m_FileListView.InsertItem(
			-1, (char *)ili->m_FileName.c_str(), NULL, g_SkinFileIconRect);
		le->SetString(1, (char *)ili->m_FileDate.c_str());
		le->SetString(2, (char *)ili->m_FileNote.c_str());
	}
	if(current>=0){
		m_FileListView.SetSelectionMark(current, 0);
		m_FileListView.EnsureVisible(current);
	}
	m_FileListView.GiveFocus(false);
	UpdateFileName();
}

/*
 *	ホストになる
 */
void CFileMode::BeginNetworkHost(char *fname){
	char *labels[] = {lang(HostPort),
		FlashIn("%s (%d-%d)", lang(SyncInterval), RSN_SYNC_INTERVAL_MIN, RSN_SYNC_INTERVAL_MAX),
		lang(ScreenName)};
	char *defaults[] = {FlashIn("%d", g_LastCreatedHostPort),
		"10", (char *)g_LastScreenName.c_str()};
	int maxlens[] = {5, 4, RSN_SCRNAME_MAX};
	m_ModalState = 60;
	g_ModalDialog = m_MultiInputDialog = new CMultiInputDialog(
		lang(CreateSession), 3, labels, defaults, maxlens);
	m_NetworkFileName = fname;
}

/*
 *	参加する
 */
void CFileMode::BeginNetworkJoin(char *fname){
	char *labels[] = {
		FlashIn("[%s %s]:[%s]", lang(Host), lang(IPAddress), lang(HostPort)),
		lang(LocalPort), lang(ScreenName)};
	char *defaults[] = {
		FlashIn("%d.%d.%d.%d:%d", g_LastJoinedHostIP[0], g_LastJoinedHostIP[1],
			g_LastJoinedHostIP[2], g_LastJoinedHostIP[3], g_LastJoinedHostPort),
		FlashIn("%d", g_LastJoinedLocalPort), (char *)g_LastScreenName.c_str()};
	int maxlens[] = {21, 5, RSN_SCRNAME_MAX};
	g_ModalDialog = m_MultiInputDialog = new CMultiInputDialog(
		lang(JoinSession), 3, labels, defaults, maxlens);
	if(fname){
		m_ModalState = 70;
		m_NetworkFileName = fname;
	}else{
		m_ModalState = 50;
	}
}

/*
 *	ネットワーク有効化・無効化
 */
void CFileMode::SwitchNetwork(bool enabled, bool host)
{
	m_NetworkMode = enabled;
	m_NetworkWindow.Show(enabled);
	m_CloseEntryButton.Enable(host);
	m_DeleteMemberButton.Enable(host);
	m_NetworkListView.GiveFocus(false);
	m_NewButton.Enable(!enabled);
	m_OpenButton.Enable(!enabled);
	m_NetworkButton.Enable(!enabled);
}

/*
 *	ファイルを開く
 */
void CFileMode::OpenFile(
	char *fname	//	ファイル名
){
	m_NewFileName = fname;
	m_ModalState = 20;
	g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
		lang(FileOpenCfmMessage), fname, false);
}

/*
 *	ファイルに保存
 */
void CFileMode::SaveFile(
	char *fname	//	ファイル名
){
	switch(g_SaveFile->Save(fname, LAYOUT_DIRNAME, false, true)){
	case 0:
		ListFile();
		break;
	case 2:
		m_ModalState = 30;
		m_NewFileName = fname;
		g_ModalDialog = m_YesNoDialog = new CYesNoDialog(
			lang(OverwriteCfmMessage), fname, false);
		break;
	}
}

/*
 *	ファイル名更新
 */
void CFileMode::UpdateFileName(){
	m_FileLabel.SetText(g_SaveFile->m_FileName.size()
		? FlashIn("%s: \"%s\"", lang(FileName), g_SaveFile->GetFileName())
		: FlashIn("%s: %s", lang(FileName), lang(NotSaved)));
}

/*
 *	備考更新
 */
void CFileMode::UpdateFileNote(){
	m_NoteEdit.SetText((char *)g_SaveFile->m_FileNote.c_str());
}

/*
 *	アンドゥ履歴を消去
 */
void CFileMode::ResetUndo(){
	m_RedoNum = m_UndoNum = m_UndoPos = 0;
}

/*
 *	アンドゥ履歴を退避
 */
void CFileMode::PushUndo(){
	if(g_NetworkInitialized) return;
	if(!g_ConfigMode->GetUseUndo()) return;
	g_SaveFile->Save(FlashIn("Undo%02d.rs2", m_UndoPos), UNDO_DIRNAME, true, false);
	m_RedoNum = 0;
	m_UndoPos++;
	if(m_UndoPos>=UNDO_MAX) m_UndoPos = 0;
	if(m_UndoNum<UNDO_MAX-1) m_UndoNum++;
}

/*
 *	アンドゥ履歴を読込
 */
void CFileMode::LoadUndo(){
	if(g_NetworkInitialized) return;
	if(!g_ConfigMode->GetUseUndo()) return;
	if(m_UndoNum){
		if(!m_RedoNum) g_SaveFile->Save(
			FlashIn("Undo%02d.rs2", m_UndoPos), UNDO_DIRNAME, true, false);
		m_UndoNum--;
		m_RedoNum++;
		m_UndoPos--;
		if(m_UndoPos<0) m_UndoPos = UNDO_MAX-1;
		string oldfile = g_SaveFile->GetFileName();
		DELETE_V(g_SaveFile);
		g_SaveFile = new CSaveFile(false);
		g_SaveFile->Load(FlashIn("Undo%02d.rs2", m_UndoPos),
			UNDO_DIRNAME, true, false, NULL, NULL, false, NULL);
		g_SaveFile->SetFileName((char *)oldfile.c_str());
		ms_ActiveMode->Enter();
	}else{
		g_Skin->Error();
	}
}

/*
 *	リドゥ履歴を読込
 */
void CFileMode::LoadRedo(){
	if(g_NetworkInitialized) return;
	if(!g_ConfigMode->GetUseUndo()) return;
	if(m_RedoNum){
		m_UndoNum++;
		m_RedoNum--;
		m_UndoPos++;
		if(m_UndoPos>=UNDO_MAX) m_UndoPos = 0;
		string oldfile = g_SaveFile->GetFileName();
		DELETE_V(g_SaveFile);
		g_SaveFile = new CSaveFile(false);
		g_SaveFile->Load(FlashIn("Undo%02d.rs2", m_UndoPos),
			UNDO_DIRNAME, true, false, NULL, NULL, false, NULL);
		g_SaveFile->SetFileName((char *)oldfile.c_str());
		ms_ActiveMode->Enter();
	}else{
		g_Skin->Error();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	アンドゥを積む
 */
void PushUndoStack(){
	if(g_NetworkInitialized) return;
	g_FileMode->PushUndo();
}
