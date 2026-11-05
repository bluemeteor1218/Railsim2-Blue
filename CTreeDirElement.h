#ifndef CTREEDIRELEMENT_H_INCLUDED
#define CTREEDIRELEMENT_H_INCLUDED

#include "CTreeElement.h"

class CPluginMode;

/*
 *	ツリーディレクトリ要素
 */
class CTreeDirElement: public CTreeElement{
	friend class CPluginTree;
	friend class CTreeElement;
private:
	static CPopMenu *ms_DirMenu;	//	ディレクトリメニュー
	bool m_Expand;					//	展開フラグ
	CTreeElement *m_Child;			//	子
	CMiniButton m_ExButton;			//	展開ボタン
public:
	static void InitMenu();
	static void FreeMenu();
	CTreeDirElement(char *, CTreeDirElement *, CPluginTree *);
	~CTreeDirElement();
	void Delete();
	void InsertItem(CTreeElement *);
	void DeleteItem(CTreeElement *);
	CTreeElement **Find(CTreeElement *, bool);
	CTreeFileElement *FindPlugin(CPlugin *);
	void Drop();
	void ListDirectory();
	CTreeDirElement *IsDirectory(){ return this; }
	bool IsRenamable(){ return true; }
	bool ConfirmRename(string &){ return true; }
	bool IsDeletable(){ return true; }
	void PushListElement(CPluginListView *);
	void CreateNewDir();
	void Sort(bool);
	char *Load(char *, CPluginMode *);
	void Save(FILE *, string);
	int CountItem(int, int *, bool);
	int ScanInput(int, int, int, int, int);
	int Render(int, int, int, int, int);
};

#endif
