#ifndef CTREEFILEELEMENT_H_INCLUDED
#define CTREEFILEELEMENT_H_INCLUDED

#include "CTreeElement.h"

/*
 *	ツリーファイル要素
 */
class CTreeFileElement: public CTreeElement{
	friend class CPluginTree;
	friend class CTreeElement;
private:
	CMDTYPE m_CommandType;	//	コマンドタイプ
	CPlugin *m_Plugin;		//	プラグイン
public:
	CTreeFileElement(char *, CTreeDirElement *, CPluginTree *, CPlugin *);
	~CTreeFileElement(){}
	CTreeFileElement *IsFile(){ return this; }
	CPlugin *GetPlugin(){ return m_Plugin; }
	void SetCommandType(CMDTYPE t){ m_CommandType = t; }
	CMDTYPE GetCommandType(){ return m_CommandType; }
	bool IsRenamable();
	bool ConfirmRename(string &);
	bool IsDeletable();
	void PushListElement(CPluginListView *);
	void Save(FILE *, string);
	int CountItem(int, int *, bool);
	int ScanInput(int, int, int, int, int);
	int Render(int, int, int, int, int);
};

#endif
