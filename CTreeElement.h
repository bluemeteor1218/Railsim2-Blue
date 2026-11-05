#ifndef CTREEELEMENT_H_INCLUDED
#define CTREEELEMENT_H_INCLUDED

const int TREE_ICON_OFS = 2;	//	アイコンと文字の距離
const int TREE_ICON_MARGIN = 2;	//	文字余白

class CPlugin;
class CPluginListView;

/*
 *	ツリー要素
 */
class CTreeElement{
	friend class CPluginTree;
	friend class CTreeFileElement;
	friend class CTreeDirElement;
protected:
	static CTreeElement *ms_FocusPrev;	//	フォーカスアイテムの前
	static CTreeElement *ms_FocusNext;	//	フォーカスアイテムの次
	int m_State;						//	状態
	int m_Width;						//	文字列幅
	bool m_RenameWait;					//	リネーム待ち
	DWORD m_ClickTime;					//	クリック時刻
	POINT m_DownPos;					//	クリック座標
	string m_String;					//	文字列
	CEditBox *m_EditBox;				//	名称変更用エディットボックス
	CTreeDirElement *m_Parent;			//	親
	CTreeElement *m_Brother;			//	兄弟
	CPluginTree *m_Owner;				//	ツリーコントロール
public:
	CTreeElement(char *, CTreeDirElement *, CPluginTree *);
	virtual ~CTreeElement(){}
	CTreeDirElement *GetParent(){ return m_Parent; }
	void SetString(char *str){ m_String = str; m_Width = -1; }
	virtual void Delete();
	bool IsInsideItem(int, int, int, int);
	void BeginRename();
	void EndRename(bool);
	void PrepareDrag();
	void RenderDragItem();
	int Compare(CTreeElement *);
	void MergeSort(CTreeElement **, CTreeElement *);
	virtual CTreeFileElement *IsFile(){ return NULL; }
	virtual CTreeDirElement *IsDirectory(){ return NULL; }
	virtual bool IsRenamable() = 0;
	virtual bool ConfirmRename(string &) = 0;
	virtual bool IsDeletable() = 0;
	virtual void Open();
	virtual void PushListElement(CPluginListView *) = 0;
	virtual void Save(FILE *, string) = 0;
	virtual int CountItem(int, int *, bool) = 0;
	virtual int ScanInput(int, int, int, int, int) = 0;
	virtual int Render(int, int, int, int, int) = 0;
};

#endif
