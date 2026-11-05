#ifndef CPLUGINTREE_H_INCLUDED
#define CPLUGINTREE_H_INCLUDED

#include "CInterface.h"
#include "CScrollBarV.h"
#include "CDragContainer.h"
#include "CPopMenu.h"
#include "CTreeFileElement.h"
#include "CTreeDirElement.h"

class CPluginMode;

/*
 *	プラグインツリー
 */
class CPluginTree: public CInterface, public CDragInterface{
	friend class CTreeElement;
	friend class CTreeFileElement;
	friend class CTreeDirElement;
private:
	int m_State;					//	状態
	int m_Rows;						//	最大表示数
	bool m_Focused;					//	フォーカス追加フラグ
	CTreeElement *m_PushedItem;		//	押されたアイテム
	CTreeElement *m_FocusItem;		//	フォーカス
	CTreeDirElement *m_DropItem;	//	フォーカス
	CTreeDirElement *m_Root;		//	データルート
	CScrollBarV m_ScrollV;			//	スクロールバー
	CPluginListView *m_SyncList;	//	平衡リストビュー
	CMenuCommander *m_Commander;	//	コマンダ
public:
	CPluginTree();
	~CPluginTree();
	void Init(int, int, int, int, char *, CInterface *, CPluginListView *, CMenuCommander *);
	CTreeElement *GetPushedItem(){ return m_PushedItem; }
	CTreeElement *GetFocusItem(){ return m_FocusItem; }
	void SetFocusItem(CTreeElement *);
	CTreeDirElement *GetRoot(){ return m_Root; }
	CTreeDirElement *GetDropItem(){ return m_DropItem; }
	void SetDropItem(CTreeDirElement *);
	void SelectPlugin(CPlugin *);
	void EnsureVisible();
	void SetScroll();
	void GiveFocus(bool snd = true);
	void RenderDragItem();
	char *Load(char *, char *, CPluginMode *);
	void Save(FILE *, char *);
	bool ScanInput();
	void Render();
};

#endif
