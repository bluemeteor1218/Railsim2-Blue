#ifndef CDRAGCONTAINER_H_INCLUDED
#define CDRAGCONTAINER_H_INCLUDED

const int DRAG_THD = FONT_HEIGHT/2;

//	反復子
typedef list<DWORD>::iterator IDWORD;

//	ドラッグタイプ
typedef enum{
	DRAG_NONE = 0,	//	不明
	DRAG_PLUGIN,	//	プラグイン・ディレクトリ
	DRAG_INSERT,	//	同一リスト内挿入のみ
} DRAGTYPE;

/*
 *	ドラッグコンテナ
 */
class CDragContainer{
	friend class CDragInterface;
private:
	static CDragContainer *ms_Drag;	//	ドラッグ物
	DRAGTYPE m_Type;			//	タイプ
	list<DWORD> m_Data;			//	データ
	CDragInterface *m_Owner;	//	所有者
public:
	static void BeginDrag(DRAGTYPE, CDragInterface *);
	static void EndDrag();
	static bool IsDragging(){ return !!ms_Drag; }
	static void Insert(DWORD);
	static void Render();
	static list<DWORD> &GetData(){ return ms_Drag->m_Data; }
	static DRAGTYPE GetType(){ return ms_Drag ? ms_Drag->m_Type : DRAG_NONE; }
	static CDragInterface *GetOwner(){ return ms_Drag ? ms_Drag->m_Owner : NULL; }
	CDragContainer(DRAGTYPE, CDragInterface *);
	void InsertData(DWORD);
	void RenderDragItem();
};

/*
 *	ドラッグ可能インターフェイス
 */
class CDragInterface{
protected:
public:
	virtual void RenderDragItem() = 0;
};

#endif CDRAGCONTAINER_H_INCLUDED
