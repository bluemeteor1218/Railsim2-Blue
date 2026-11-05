#ifndef CLISTVIEW_H_INCLUDED
#define CLISTVIEW_H_INCLUDED

#include "CInterface.h"
#include "CPushButton.h"
#include "CScrollBarV.h"
#include "CDragContainer.h"
#include "CPopMenu.h"

/*
 *	リスト要素
 */
class CListElement{
	friend class CListView;
protected:
	bool m_Selected;			//	選択フラグ
	DWORD m_Data;				//	関連データ
	vector<string> m_String;	//	文字列
	CListElement *m_Next;		//	次
	CListView *m_Owner;			//	リストビュー
	CEditBox *m_EditBox;		//	名称変更用エディットボックス
public:
	CListElement(int, char *, CListView *);
	virtual ~CListElement(){}
	DWORD GetData(){ return m_Data; }
	void SetData(DWORD d){ m_Data = d; }
	bool IsSelected(){ return m_Selected; }
	char *GetString(int i){ return (char *)m_String[i].c_str(); }
	void SetString(int i, char *s){ m_String[i] = s; }
	void BeginRename();
	void EndRename(bool);
	virtual void RenderDragItem(int, int, bool);
	virtual bool Render(int, int, int, int, bool, bool);
};

/*
 *	アイコン付リスト要素
 */
class CIconListElement: public CListElement{
private:
	LPTEX8 m_IconTex;		//	アイコンテクスチャ
	float m_IconRect[4];	//	アイコン位置
public:
	CIconListElement(int, char *, CListView *, LPTEX8, float *);
	void RenderDragItem(int, int, bool);
	bool Render(int, int, int, int, bool, bool);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	オプションフラグ
const int LISTVIEW_MULTISELECTABLE = 1;
const int LISTVIEW_INSERTABLE = 2;
const int LISTVIEW_RENAMABLE = 4;
const int LISTVIEW_SORTABLE = 8;

/*
 *	リストビュー
 */
class CListView: public CInterface, public CDragInterface{
	friend class CListElement;
	friend class CIconListElement;
protected:
	int m_State;					//	状態
	int m_Cols;						//	列数
	int m_Rows;						//	表示可能行数
	int m_RowHeight;				//	列高
	int m_ColResize;				//	リサイズ列
	int m_ColOldSize;				//	旧サイズ
	int m_GrabPosX;					//	つかみ位置
	int m_FocusIndex;				//	フォーカス位置
	int m_LastSel;					//	最後に選択したアイテム
	int m_ItemNum;					//	データ数
	int m_InsertRow;				//	挿入位置
	int m_DownScrollV;				//	クリック時スクロール位置
	DRAGTYPE m_DragType;			//	ドラッグ可能 /*CP932対応*/
	bool m_MultiSelect;				//	複数選択可能 /*CP932対応*/
	bool m_Insertable;				//	挿入可能 /*CP932対応*/
	bool m_Renamable;				//	リネーム可能 /*CP932対応*/
	bool m_RenameWait;				//	リネーム待ち
	bool m_Sortable;				//	列ソート可能 /*CP932対応*/
	DWORD m_ClickTime;				//	クリック時刻
	POINT m_DownPos;				//	クリック座標
	CListElement *m_FocusItem;		//	フォーカスアイテム
	CListElement *m_DropItem;		//	ドロップ先アイテム
	CListElement *m_Data;			//	データ
	CPushButton *m_ColHeader;		//	列ヘッダ
	CScrollBarV m_ScrollV;			//	スクロールバー
	CMenuCommander *m_Commander;	//	コマンダ
	CMDTYPE m_CmdType;				//	コマンドタイプ
public:
	CListView();
	virtual ~CListView();
	void Init(int, int, int, int, CInterface *, int, char **,
		DRAGTYPE, DWORD, CMenuCommander *cmd = NULL, CMDTYPE ctype = CMD_NONE);
	void SetSize(int, int);
	void ResizeCol(int, int);
	CListElement *InsertItem(int, CListElement *);
	CListElement *InsertItem(int, char *);
	int GetItemNum(){ return m_ItemNum; }
	CListElement *GetElement(int);
	void DeleteAllItems();
	void DeleteItem(int);
	int GetSelectionMark();
	void SetSelectionMark(int, int);
	void SetSelectionArea(int, int, int);
	CListElement *GetFocusItem(){ return m_FocusItem; }
	void EnsureVisible(int);
	void SetScroll();
	int CalcInsertRow(int);
	int GetInsertRow(){ return m_InsertRow; }
	bool IsInsideList(int, int);
	int HitTest(int, int);
	virtual bool IsRenamable(CListElement *){ return true; }
	virtual bool IsDroppable(CListElement *){ return false; }
	virtual bool ConfirmRename(CListElement *, string &){ return true; }
	virtual void EndRename(CListElement *){}
	virtual void Drop(){}
	virtual void DoubleClick(){}
	void PrepareDrag();
	void RenderDragItem();
	bool CheckDrop();
	bool ScanInput();
	void Render();
};

/*
 *	アイコン付リストビュー
 */
class CIconListView: public CListView{
protected:
public:
	virtual ~CIconListView(){}
	void Init(int, int, int, int, CInterface *, int, char **,
		DRAGTYPE, DWORD, CMenuCommander *cmd = NULL, CMDTYPE ctype = CMD_NONE);
	CListElement *InsertItem(int, char *, LPTEX8, float *);
};

#endif
