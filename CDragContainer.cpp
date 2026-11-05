#include "stdafx.h"
#include "CDragContainer.h"

//	static メンバ
CDragContainer *CDragContainer::ms_Drag = NULL;

/*
 *	[static]
 *	ドラッグ開始
 */
void CDragContainer::BeginDrag(
	DRAGTYPE type,		//	タイプ
	CDragInterface *o	//	所有者
){
	EndDrag();
	ms_Drag = new CDragContainer(type, o);
}

/*
 *	[static]
 *	ドラッグ終了
 */
void CDragContainer::EndDrag(){
	DELETE_V(ms_Drag);
}

/*
 *	[static]
 *	データ挿入
 */
void CDragContainer::Insert(
	DWORD data	//	データ
){
	ms_Drag->InsertData(data);
}

/*
 *	[static]
 *	ドラッグアイテム表示
 */
void CDragContainer::Render(){
	if(ms_Drag) ms_Drag->RenderDragItem();
}

/*
 *	コンストラクタ
 */
CDragContainer::CDragContainer(
	DRAGTYPE type,		//	タイプ
	CDragInterface *o	//	所有者
){
	m_Type = type;
	m_Owner = o;
}

/*
 *	データ挿入
 */
void CDragContainer::InsertData(
	DWORD data	//	データ
){
	m_Data.insert(m_Data.end(), data);
}

/*
 *	ドラッグアイテム表示
 */
void CDragContainer::RenderDragItem(){
	m_Owner->RenderDragItem();
}
