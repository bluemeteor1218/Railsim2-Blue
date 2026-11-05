#ifndef CFILEMODE_H_INCLUDED
#define CFILEMODE_H_INCLUDED

#include "CStaticCtrl.h"
#include "CEditCtrl.h"
#include "CPushButton.h"
#include "CCheckBox.h"
#include "CListView.h"
#include "CInterfaceMode.h"

class CYesNoDialog;
class CInputDialog;
class CMultiInputDialog;

/*
 *	セーブファイルリスト要素
 */
class CLayoutInfo{
	friend class CFileMode;
private:
	float m_Version;	//	対応バージョン
	string m_FileName;	//	ファイル名
	string m_FileDate;	//	更新日時
	string m_FileNote;	//	備考
public:
	CLayoutInfo(char *fname){ m_FileName = fname; }
	bool PreLoadSF(FILE *file);
	bool operator<(const CLayoutInfo &rhs){ return m_FileName<rhs.m_FileName; }
};

//	反復子
typedef list<CLayoutInfo>::iterator ILayoutInfo;

/*
 *	ファイルリストビュー
 */
class CFileListView: public CIconListView{
public:
	bool ConfirmRename(CListElement *, string &);
	void EndRename(CListElement *);
	void DoubleClick();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ファイルモード
 */
class CFileMode: public CInterfaceMode, public CWindowResizer, public CMenuCommander{
private:
	int m_ModalState;						//	モーダル状態
	int m_RedoNum;							//	リドゥ数
	int m_UndoNum;							//	アンドゥ数
	int m_UndoPos;							//	アンドゥ位置
	bool m_NetworkMode;						//	ネットワークモード
	DPNID m_DeletingMemberID;				//	削除対象メンバーID
	string m_NewFileName;					//	新規ファイル名
	string m_NetworkFileName;				//	ネットモードで使用するファイル名
	CYesNoDialog *m_YesNoDialog;			//	yes/no ダイアログ
	CInputDialog *m_InputDialog;			//	入力ダイアログ
	CMultiInputDialog *m_MultiInputDialog;	//	複数項目入力ダイアログ
	CPopMenu *m_FileMenu;					//	ファイルメニュー
	list<CLayoutInfo> m_LayoutInfoList;		//	ファイルリスト
	CWindowCtrl m_FileWindow;				//	窓
	CStaticCtrl m_FileLabel;				//	ファイル名ラベル
	CStaticCtrl m_NoteLabel;				//	備考ラベル
	CEditCtrl m_NoteEdit;					//	備考エディット
	CPushButton m_NewButton;				//	新規作成
	CPushButton m_OpenButton;				//	開く
	CPushButton m_SaveButton;				//	上書き保存
	CPushButton m_SaveAsButton;				//	別名で保存
	CPushButton m_NetworkButton;			//	ネットワーク
	CFileListView m_FileListView;			//	ファイルリストビュー
	CCheckBox m_AutoLoadCheck;				//	自動読込
	CWindowCtrl m_NetworkWindow;			//	窓
	CStaticCtrl m_NetworkLabel;				//	ファイル名ラベル
	CPushButton m_CloseEntryButton;			//	受付終了ボタン
	CPushButton m_DeleteMemberButton;		//	メンバ削除ボタン
	CPushButton m_DisconnectButton;			//	新規作成
	CListView m_NetworkListView;			//	メンバリストビュー
public:
	CFileMode();
	~CFileMode();
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	char *LoadInterfaceSetting(char *);
	void SaveInterfaceSetting(FILE *);
	void EnterInterface();
	void ModalFuncInterface();
	void ScanInputInterface();
	void ListFile();
	void BeginNetworkHost(char *);
	void BeginNetworkJoin(char *);
	void SwitchNetwork(bool, bool);
	void OpenFile(char *);
	void SaveFile(char *);
	void UpdateFileName();
	void UpdateFileNote();
	void ResetUndo();
	void PushUndo();
	void LoadUndo();
	void LoadRedo();
};

//	外部グローバル
extern CFileMode *g_FileMode;

#endif
