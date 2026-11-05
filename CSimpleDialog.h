#ifndef CYESNODIALOG_H_INCLUDED
#define CYESNODIALOG_H_INCLUDED

#include "CWindowCtrl.h"
#include "CStaticCtrl.h"
#include "CPushButton.h"
#include "CEditCtrl.h"

class CMenuCommand;

/*
 *	シンプルダイアログ
 */
class CSimpleDialog: public CWindowCtrl{
private:
	CStaticCtrl m_Label;	//	ラベル
	CPushButton m_OKButton;	//	OK
public:
	CSimpleDialog(char *, char *);
	bool CheckOK(){ return m_OKButton.IsPushed(); }
	int GetWindowState(){ return CheckOK(); }
};

/*
 *	Yes / No ダイアログ
 */
class CYesNoDialog: public CWindowCtrl{
private:
	CStaticCtrl m_Label;		//	ラベル
	CPushButton m_YesButton;	//	はい
	CPushButton m_NoButton;		//	いいえ
	CMenuCommand *m_YesCommand;	//	「はい」選択時のコマンド
	CMenuCommand *m_NoCommand;	//	「いいえ」選択時のコマンド
public:
	CYesNoDialog(char *, char *, bool);
	~CYesNoDialog();
	void SetYesCommand(CMenuCommand *cmd){ m_YesCommand = cmd; }
	void SetNoCommand(CMenuCommand *cmd){ m_NoCommand = cmd; }
	bool CheckYes(){ return m_YesButton.IsPushed(); }
	bool CheckNo(){ return m_NoButton.IsPushed(); }
	int GetWindowState(){ return CheckYes() || CheckNo(); }
	bool ScanInputWindow();
};

/*
 *	入力ダイアログ
 */
class CInputDialog: public CWindowCtrl{
private:
	CStaticCtrl m_Label;		//	ラベル
	CEditCtrl m_Edit;			//	エディットボックス
	CPushButton m_OKButton;		//	はい
	CPushButton m_CancelButton;	//	いいえ
public:
	CInputDialog(char *, char *, char *, int);
	char *GetInputText(){ m_Edit.FinishInput(); return m_Edit.GetText(); }
	bool CheckOK(){ return m_OKButton.IsPushed(); }
	bool CheckCancel(){ return m_CancelButton.IsPushed(); }
	int GetWindowState(){ return CheckOK() || CheckCancel(); }
	bool ScanInputWindow();
};

/*
 *	複数項目入力ダイアログ
 */
class CMultiInputDialog: public CWindowCtrl{
private:
	int m_ItemNumber;			//	アイテム数
	CStaticCtrl *m_Label;		//	ラベル
	CEditCtrl *m_Edit;			//	エディットボックス
	CPushButton m_OKButton;		//	はい
	CPushButton m_CancelButton;	//	いいえ
public:
	CMultiInputDialog(char *, int, char **, char **, int *);
	~CMultiInputDialog();
	int GetItemNumber(){ return m_ItemNumber; }
	char *GetInputText(int i){ m_Edit[i].FinishInput(); return m_Edit[i].GetText(); }
	bool CheckOK(){ return m_OKButton.IsPushed(); }
	bool CheckCancel(){ return m_CancelButton.IsPushed(); }
	int GetWindowState(){ return CheckOK() || CheckCancel(); }
	bool ScanInputWindow();
};

//	関数宣言
void EnqueueCommonDialog(CWindowCtrl *);
void ProcessCommonDialog();

//	外部グローバル
extern list<CWindowCtrl *> g_DialogQueue;

#endif
