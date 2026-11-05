#include "stdafx.h"
#include "CListView.h"
#include "CPopMenu.h"
#include "CSkinPlugin.h"
#include "CGameMode.h"

//	内部定数
const int LV_COL_MARGIN = 4;	//	列余白
const int LV_NAME_MAX = 256;	//	名称最大長
const int COL_HEAD_GRAB = 4;	//	ヘッドつまみ幅
const int COL_HEAD_MIN = 16;	//	ヘッド最小幅
const int COL_HEAD_MAX = 512;	//	ヘッド最大幅

/*
 *	コンストラクタ
 */
CListElement::CListElement(
	int cols,		//	列数
	char *str,		//	第一要素
	CListView *ctrl	//	リストビュー
):
	m_String(cols)	//	データ
{
	m_Selected = false;
	m_Data = 0;
	m_String[0] = str;
	m_Next = NULL;
	m_Owner = ctrl;
	m_EditBox = NULL;
}

/*
 *	リネーム開始
 */
void CListElement::BeginRename(){
	m_EditBox = new CEditBox;
	m_EditBox->Create(0, 0,
		(m_String[0].size()+1)*FONT_WIDTH, LV_NAME_MAX, m_String[0]);
	m_Owner->GiveFocus(false);
}

/*
 *	リネーム終了
 */
void CListElement::EndRename(
	bool update	//	更新フラグ
){
	if(update){
		string newname;
		m_EditBox->GetText(newname);
		DELETE_V(m_EditBox);
		if(newname.size() && m_Owner->ConfirmRename(this, newname)){
			m_String[0] = newname;
			m_Owner->EndRename(this);
		}
	}else{
		DELETE_V(m_EditBox);
	}
}

/*
 *	ドラッグアイテム表示
 */
void CListElement::RenderDragItem(
	int x, int y,	//	座標
	bool focus		//	フォーカス
){
	CStringDrawer *sd = g_StrTex->DrawString(m_String[0].c_str(), 0);
	int tw = sd->GetWidth()+LV_COL_MARGIN*2;
	D3DCOLOR fc;
	devSetTexture(0, NULL);
	if(m_Selected){
		Grad2DRect(x, y, x+tw, y+FONT_HEIGHT,
			g_Skin->m_ListViewData.m_SelectedBaseColor);
		fc = g_Skin->m_ListViewData.m_SelectedFontColor;
	}else{
		fc = g_Skin->m_ListViewData.m_DefaultFontColor;
	}
	if(focus) Draw2DRect(x, y, x+tw, y+FONT_HEIGHT,
		g_Skin->m_ListViewData.m_FocusFrameColor);
	sd->RenderLeft(x+LV_COL_MARGIN, y, fc);
}

/*
 *	行描画
 */
bool CListElement::Render(
	int x, int y,	//	座標
	int tw, int th,	//	最大サイズ
	bool focus,		//	フォーカス
	bool drop		//	ドロップ
){
	D3DCOLOR fc;
	devSetTexture(0, NULL);
	if(m_Selected || drop){
		Grad2DRect(x, y, x+tw, y+th,
			g_Skin->m_ListViewData.m_SelectedBaseColor);
		fc = g_Skin->m_ListViewData.m_SelectedFontColor;
	}else{
		fc = g_Skin->m_ListViewData.m_DefaultFontColor;
	}
	if(focus) Draw2DRect(x+1, y, x+tw, y+th,
		g_Skin->m_ListViewData.m_FocusFrameColor);
	int j, cw = m_Owner->m_ColHeader[0].GetWidth();
	if(m_EditBox && (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=this || !m_Selected)){
		EndRename(true);
		return true;
	}
	if(m_EditBox){
		m_EditBox->SetPos(x+LV_COL_MARGIN, y);
	}else{
		g_StrTex->RenderLeft(x+LV_COL_MARGIN, y, fc, 0,
			m_String[0].c_str(), cw-LV_COL_MARGIN, th);
	}
	x += cw;
	for(j = 1; j<m_Owner->m_Cols; j++){
		int cw = m_Owner->m_ColHeader[j].GetWidth();
		g_StrTex->RenderLeft(x+LV_COL_MARGIN, y, fc, 0,
			m_String[j].c_str(), cw-LV_COL_MARGIN, th);
		x += cw;
	}
	if(m_EditBox) m_EditBox->Render();
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	行描画
 */
CIconListElement::CIconListElement(
	int cols,			//	列数
	char *str,			//	第一要素
	CListView *ctrl,	//	リストビュー
	LPTEX8 icon,		//	アイコンテクスチャ
	float *rect			//	アイコン位置
):
	CListElement(cols, str, ctrl)	//	基本クラス
{
	m_IconTex = icon;
	memcpy(m_IconRect, rect, sizeof(float)*4);
}

/*
 *	ドラッグアイテム表示
 */
void CIconListElement::RenderDragItem(
	int x, int y,	//	座標
	bool focus		//	フォーカス
){
	CStringDrawer *sd = g_StrTex->DrawString(m_String[0].c_str(), 0);
	int tw = TILE_UNIT+sd->GetWidth()+LV_COL_MARGIN*3;
	D3DCOLOR fc;
	if(m_IconTex) devSetTexture(0, m_IconTex);
	else g_Skin->SetInterfaceTexture();
	SetUVMap(m_IconRect[0], m_IconRect[1],
		m_IconRect[0]+m_IconRect[2], m_IconRect[1]+m_IconRect[3]);
	TexMap2DRect(x+LV_COL_MARGIN, y,
		x+LV_COL_MARGIN+TILE_UNIT, y+TILE_UNIT, 0xffffffff);
	devSetTexture(0, NULL);
	if(m_Selected){
		Grad2DRect(x, y, x+tw, y+TILE_UNIT,
			g_Skin->m_ListViewData.m_SelectedBaseColor);
		fc = g_Skin->m_ListViewData.m_SelectedFontColor;
	}else{
		fc = g_Skin->m_ListViewData.m_DefaultFontColor;
	}
	if(focus) Draw2DRect(x, y, x+tw, y+TILE_UNIT,
		g_Skin->m_ListViewData.m_FocusFrameColor);
	sd->RenderLeft(x+LV_COL_MARGIN*2+TILE_UNIT, y+FontY(TILE_UNIT), fc);
}

/*
 *	レンダリング
 */
bool CIconListElement::Render(
	int x, int y,	//	座標
	int tw, int th,	//	最大サイズ
	bool focus,		//	フォーカス
	bool drop		//	ドロップ
){
	D3DCOLOR fc;
	if(m_IconTex) devSetTexture(0, m_IconTex);
	else g_Skin->SetInterfaceTexture();
	int tiw = m_Owner->m_ColHeader[0].GetWidth()-LV_COL_MARGIN;
	if(tiw>TILE_UNIT) tiw = TILE_UNIT;
	SetUVMap(m_IconRect[0], m_IconRect[1],
		m_IconRect[0]+m_IconRect[2]*tiw/TILE_UNIT, m_IconRect[1]+m_IconRect[3]*th/TILE_UNIT);
	TexMap2DRect(x+LV_COL_MARGIN, y, x+LV_COL_MARGIN+tiw, y+th, 0xffffffff);
	devSetTexture(0, NULL);
	if(m_Selected || drop){
		Grad2DRect(x, y, x+tw, y+th,
			g_Skin->m_ListViewData.m_SelectedBaseColor);
		fc = g_Skin->m_ListViewData.m_SelectedFontColor;
	}else{
		fc = g_Skin->m_ListViewData.m_DefaultFontColor;
	}
	if(focus) Draw2DRect(x+1, y, x+tw, y+th,
		g_Skin->m_ListViewData.m_FocusFrameColor);
	int fix = FontY(TILE_UNIT);
	y += fix;
	th -= fix;
	if(th<=0) return false;
	int j, cw1 = m_Owner->m_ColHeader[0].GetWidth();
	int iconfix = LV_COL_MARGIN*2+TILE_UNIT;
	if(m_EditBox && (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=this || !m_Selected)){
		EndRename(true);
		return true;
	}
	if(m_EditBox){
		m_EditBox->SetPos(x+iconfix, y);
	}else{
		if(cw1>iconfix) g_StrTex->RenderLeft(x+iconfix, y,
			fc, 0, m_String[0].c_str(), cw1-iconfix, th);
	}
	x += cw1;
	for(j = 1; j<m_Owner->m_Cols; j++){
		int cw = m_Owner->m_ColHeader[j].GetWidth();
		g_StrTex->RenderLeft(x+LV_COL_MARGIN, y, fc, 0,
			m_String[j].c_str(), cw-LV_COL_MARGIN, th);
		x += cw;
	}
	if(m_EditBox) m_EditBox->Render();
	return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CListView::CListView(){
	m_ColHeader = NULL;
	m_FocusItem = m_DropItem = m_Data = NULL;
	m_ItemNum = 0;
}

/*
 *	デストラクタ
 */
CListView::~CListView(){
	DELETE_A(m_ColHeader);
	DeleteAllItems();
}

/*
 *	初期化
 */
void CListView::Init(
	int x, int y,			//	座標
	int w, int h,			//	サイズ
	CInterface *p,			//	親
	int cols,				//	列数
	char **coln,			//	列名
	DRAGTYPE drag,			//	ドラッグタイプ
	DWORD flags,			//	フラグ
	CMenuCommander *cmd,	//	コマンダ
	CMDTYPE ctype			//	コマンドタイプ
){
	CInterface::Init(x, y, w, h, "", p);
	m_State = 0;
	m_Cols = cols;
	m_RowHeight = FONT_HEIGHT;
	m_ColResize = -1;
	m_FocusIndex = m_LastSel = 0;
	m_InsertRow = -1;
	m_DragType = drag;
	m_MultiSelect = !!(flags&LISTVIEW_MULTISELECTABLE);
	m_Insertable = !!(flags&LISTVIEW_INSERTABLE);
	m_Renamable = !!(flags&LISTVIEW_RENAMABLE);
	m_Sortable = !!(flags&LISTVIEW_SORTABLE);
	m_RenameWait = false;
	m_ClickTime = 0;
	m_ColHeader = new CPushButton[m_Cols];
	LinkTab(true);
	int i, tw = m_Width-TILE_UNIT;
	for(i = 0; i<m_Cols; i++){
		m_ColHeader[i].Init(tw*i/m_Cols, 0,
			tw*(i+1)/m_Cols-tw*i/m_Cols, TILE_UNIT, coln[i], this);
		m_ColHeader[i].SetPushable(m_Sortable);
	}
	m_ScrollV.Init(m_Width-TILE_UNIT, 0, TILE_UNIT, m_Height, this);
	m_Commander = cmd;
	m_CmdType = ctype;
}

/*
 *	サイズ変更
 */
void CListView::SetSize(
	int w, int h	//	新規サイズ
){
	m_Width = w; m_Height = h;
	m_ScrollV.SetPos(m_Width-TILE_UNIT, 0);
	m_ScrollV.SetSize(TILE_UNIT, m_Height);
	SetScroll();
	ResizeCol(0, m_Cols>1 ? m_ColHeader[0].GetWidth() : w-TILE_UNIT);
}

/*
 *	列幅変更
 */
void CListView::ResizeCol(
	int col,	//	列番号
	int cw		//	新規サイズ
){
	int j, tmax = m_Width-TILE_UNIT, smax = tmax;
	for(j = 0; j<m_Cols; j++){
		if(j<col){
			smax -= m_ColHeader[j].GetWidth();
			tmax -= m_ColHeader[j].GetWidth();
		}
		if(j>col){
			tmax -= m_ColHeader[j].GetWidth();
			smax -= COL_HEAD_MIN;
		}
	}
	if(tmax>COL_HEAD_MAX) tmax = COL_HEAD_MAX;
	if(smax>COL_HEAD_MAX) smax = COL_HEAD_MAX;
	int tx = cw;
	if(tx<COL_HEAD_MIN) tx = COL_HEAD_MIN;
	if(tx>smax) tx = smax;
	int over = tx-tmax;
	m_ColHeader[col].SetSize(tx, TILE_UNIT);
	tx += m_ColHeader[col].GetPosX();
	for(j = m_Cols-1; j>col; j--){
		if(!over) break;
		int sw = m_ColHeader[j].GetWidth()-over;
		if(sw<COL_HEAD_MIN) sw = COL_HEAD_MIN;
		if(sw>COL_HEAD_MAX) sw = COL_HEAD_MAX;
		over -= m_ColHeader[j].GetWidth()-sw;
		m_ColHeader[j].SetSize(sw, TILE_UNIT);
	}
	for(j = col+1; j<m_Cols; j++){
		m_ColHeader[j].SetPos(tx, 0);
		tx += m_ColHeader[j].GetWidth();
	}
}

/*
 *	アイテム挿入
 */
CListElement *CListView::InsertItem(
	int index,			//	行
	CListElement *item	//	アイテム
){
	if(index<0 || index>m_ItemNum) index = m_ItemNum;
	if(!m_FocusItem){
		m_FocusIndex = index;
		m_FocusItem = item;
	}
	CListElement **ie = &m_Data;
	while(index--) ie = &(*ie)->m_Next;
	CListElement *next = *ie;
	*ie = item;
	item->m_Next = next;
	m_ItemNum++;
	//EnsureVisible(index);
	//SetSelectionMark(index, false);
	return item;
}

/*
 *	番号からアイテム取得
 */
CListElement *CListView::GetElement(
	int index	//	番号
){
	if(index<0 || m_ItemNum<=index) return NULL;
	CListElement *ie = m_Data;
	while(index--) ie = ie->m_Next;
	return ie;
}

/*
 *	アイテム挿入
 */
CListElement *CListView::InsertItem(
	int index,	//	行
	char *str	//	文字列
){
	return InsertItem(index, new CListElement(m_Cols, str, this));
}

/*
 *	全アイテム削除
 */
void CListView::DeleteAllItems(){
	CListElement *ie = m_Data;
	while(ie){
		CListElement *next = ie->m_Next;
		delete ie;
		ie = next;
	}
	m_FocusItem = m_Data = NULL;
	m_ItemNum = 0;
}

/*
 *	アイテム削除
 */
void CListView::DeleteItem(
	int index	//	行
){
	if(index<0 || m_ItemNum<=index) return;
	CListElement **ie = &m_Data;
	while(index--) ie = &(*ie)->m_Next;
	CListElement *next = (*ie)->m_Next;
	delete *ie;
	*ie = next;
	m_ItemNum--;
	if(m_FocusIndex==index){
		if(m_FocusIndex==m_ItemNum) m_FocusIndex--;
		m_FocusItem = GetElement(m_FocusIndex);
	}
}

/*
 *	選択されている最初のアイテム番号を取得
 */
int CListView::GetSelectionMark(){
	int i;
	CListElement *ie = m_Data;
	for(i = 0; ie; i++){
		if(ie->m_Selected) return i;
		ie = ie->m_Next;
	}
	return -1;
}

/*
 *	指定したひとつのアイテムを選択状態にする
 */
void CListView::SetSelectionMark(
	int index,	//	列
	int mode	//	追加選択モード (0: normal, 1: add, 2: invert)
){
	int i;
	CListElement *ie = m_Data;
	for(i = 0; ie; i++){
		if(i==index){
			m_FocusIndex = m_LastSel = i;
			m_FocusItem = ie;
		}
		switch(mode){
		case 0: ie->m_Selected = i==index; break;
		case 1: if(i==index) ie->m_Selected = true; break;
		case 2: if(i==index) ie->m_Selected = !ie->m_Selected; break;
		}
		ie = ie->m_Next;
	}
}

/*
 *	指定した範囲を選択状態にする
 */
void CListView::SetSelectionArea(
	int begin,	//	選択開始行
	int end,	//	選択終了行
	int mode	//	追加選択モード
){
	int i;
	CListElement *ie = m_Data;
	if(begin>end){
		int tmp = begin;
		begin = end;
		end = tmp;
	}
	for(i = 0; ie; i++){
		if(mode){
			if(begin<=i && i<=end) ie->m_Selected = true;
		}else{
			ie->m_Selected = begin<=i && i<=end;
		}
		ie = ie->m_Next;
	}
}

/*
 *	指定したアイテムを可視状態にする
 */
void CListView::EnsureVisible(
	int index	//	列
){
	if(index<0 || m_ItemNum<=index) return;
	SetScroll();
	if(index>=m_ScrollV.GetScroll()+m_Rows) m_ScrollV.SetScroll(index-m_Rows+1);
	else if(index<m_ScrollV.GetScroll()) m_ScrollV.SetScroll(index);
}

/*
 *	スクロールバーの更新
 */
void CListView::SetScroll(){
	m_Rows = (m_Height-TILE_UNIT)/m_RowHeight;
	m_ScrollV.SetRange(m_ItemNum ? m_ItemNum : 1);
	m_ScrollV.SetPage(m_Rows);
}

/*
 *	挿入位置取得
 */
int CListView::CalcInsertRow(int cy){
	int px, py;
	GetAbsPos(&px, &py);
	int ofs = m_ScrollV.GetScroll();
	int insert_row = (cy-TILE_UNIT+m_RowHeight/2-py)/m_RowHeight+ofs;
	int drop_max = ofs+(m_Height-TILE_UNIT)/m_RowHeight;
	if(drop_max>GetItemNum()) drop_max = GetItemNum();
	ValueArea(&insert_row, ofs, drop_max);
	return insert_row;
}

/*
 *	指定座標がリスト内か調べる
 */
bool CListView::IsInsideList(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width-TILE_UNIT && py+TILE_UNIT<=y && y<py+m_Height;
}

/*
 *	指定座標の対応番号を調べる
 */
int CListView::HitTest(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width-TILE_UNIT && py+TILE_UNIT<=y && y<py+m_Height
		? (y-py-TILE_UNIT)/m_RowHeight+m_ScrollV.GetScroll() : -1;
}

/*
 *	ドラッグ操作の準備
 */
void CListView::PrepareDrag(){
	if(!m_DragType || !m_FocusItem) return;
	if(!m_FocusItem->m_Selected) m_FocusItem->m_Selected = true;
	m_State = 1;
	CDragContainer::BeginDrag(m_DragType, this);
	CListElement *ptr = m_Data;
	while(ptr){
		if(ptr->m_Selected) CDragContainer::Insert(ptr->m_Data);
		ptr = ptr->m_Next;
	}
}

/*
 *	ドラッグアイテム表示
 */
void CListView::RenderDragItem(){
	CInterface::RenderBrother();
	int i, px, py;
	GetAbsPos(&px, &py);
	POINT pos = g_Cursor.GetPos();
	px = pos.x-TILE_UNIT/2;
	py += pos.y-m_DownPos.y+TILE_UNIT-m_DownScrollV*m_RowHeight;
	CListElement *ie = m_Data;
	for(i = 0; ie; ie = ie->m_Next, i++){
		if(ie->m_Selected) ie->RenderDragItem(px, py, i==m_FocusIndex);
		py += m_RowHeight;
	}
}

/*
 *	ドロップ操作の検出
 *	assuming cursor is inside list
 */
bool CListView::CheckDrop(){
	m_DropItem = NULL;
	if(m_DragType && CDragContainer::GetType()==m_DragType && (!ms_Drop || IsFocus())){
		POINT pos = g_Cursor.GetPos();
		ms_Drop = this;
		CListElement *tmp = GetElement(HitTest(pos.x, pos.y));
		if(tmp && !tmp->m_Selected && IsDroppable(tmp)) m_DropItem = tmp;
	}
	return !!m_DropItem;
}

/*
 *	入力チェック
 */
bool CListView::ScanInput(){
	POINT pos = g_Cursor.GetPos();
	int j, px, py;
	GetAbsPos(&px, &py);
	SetScroll();
	m_InsertRow = -1;
	if(m_FocusItem && m_FocusItem->m_EditBox
		&& GetButton(DIM_LEFT)==S_PUSH && !IsInsideList(pos.x, pos.y)){
		m_FocusItem->EndRename(true);
		return true;
	}
	if(m_ColResize>=0){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			g_Cursor.SetResize(2);
			ResizeCol(m_ColResize, m_ColOldSize+pos.x-m_GrabPosX);
			return true;
		default:
			m_ColResize = -1;
			break;
		}
	}else if(!m_State){
		int tx = px;
		for(j = 0; j<m_Cols; j++){
			tx += m_ColHeader[j].GetWidth();
			int tx2 = tx+COL_HEAD_GRAB;
			if(tx2>=px+m_Width-TILE_UNIT) tx2 = px+m_Width-TILE_UNIT-1;
			if(tx-COL_HEAD_GRAB<=pos.x && pos.x<tx2
				&& py<=pos.y && pos.y<py+TILE_UNIT){
				g_Cursor.SetResize(2);
				if(GetButton(DIM_LEFT)==S_PUSH){
					m_ColResize = j;
					m_ColOldSize = m_ColHeader[j].GetWidth();
					m_GrabPosX = pos.x;
					return true;
				}
			}
		}
	}
	if(m_RenameWait && GetFrameCount()-m_ClickTime==DBLCLK_FRAME
		&& IsFocus() && m_FocusItem && m_FocusItem->m_Selected){
		m_ClickTime = 0;
		m_FocusItem->BeginRename();
	}else if(IsInsideList(pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH: {
			m_RenameWait = false;
			int oldindex = m_FocusIndex;
			CListElement *oldfocus = m_FocusItem;
			m_FocusIndex = HitTest(pos.x, pos.y);
			if(!(m_FocusItem = GetElement(m_FocusIndex))){
				SetSelectionMark(-1, 0);
				m_FocusIndex = oldindex;
				m_FocusItem = oldfocus;
				GiveFocus();
				return true;
			}
			m_DownPos = pos;
			m_DownScrollV = m_ScrollV.GetScroll();
			if(m_DragType && CheckCtrl()){
				m_State = 2;
			}else{
				if(CheckShift() || !m_FocusItem->m_Selected){
					if(m_MultiSelect && CheckShift()){
						m_State = 1;
						SetSelectionArea(
							m_LastSel, m_FocusIndex, CheckCtrl() ? 1 : 0);
					}else{
						m_State = 3;
						SetSelectionMark(m_FocusIndex, m_MultiSelect && CheckCtrl() ? 2 : 0);
					}
				}else{
					m_State = IsFocus() && oldindex==m_FocusIndex
						&& m_FocusItem->m_Selected ? 4 : 3;
				}
			}
			GiveFocus();
			return true; }
		case S_HOLD:
			if(m_Insertable)
			{
				static int insert_scroll_cnt = 0;
				if(++insert_scroll_cnt>=6){
					insert_scroll_cnt = 0;
					if(CDragContainer::GetOwner()==this){
						if(pos.y<py+TILE_UNIT+m_RowHeight/2) m_ScrollV.SetScroll(m_ScrollV.GetScroll()-1);
						else if(pos.y>=py+m_Height-m_RowHeight/2) m_ScrollV.SetScroll(m_ScrollV.GetScroll()+1);
					}
				}
			}
			if(CheckDrop()) return true;
			if(m_State){
				if(Manhattan(pos, m_DownPos)>DRAG_THD
					&& !CDragContainer::IsDragging()) PrepareDrag();
				return true;
			}
			break;
		default:
			if(m_Insertable && CDragContainer::GetOwner()==this) m_InsertRow = CalcInsertRow(pos.y);
			if(CheckDrop()){
				Drop();
				return true;
			}
			if(m_State){
				if(m_State==2){
					if(CheckShift()) SetSelectionArea(m_LastSel, m_FocusIndex, 1);
					else SetSelectionMark(m_FocusIndex, 2);
				}else if(m_State==3){
					if(CheckShift()){
						SetSelectionArea(m_LastSel, m_FocusIndex, 0);
					}else{
						m_ClickTime = GetFrameCount();
						SetSelectionMark(m_FocusIndex, 0);
					}
				}else if(m_State==4){
					SetSelectionMark(m_FocusIndex, 0);
					if(GetFrameCount()-m_ClickTime<DBLCLK_FRAME){
						m_ClickTime = 0;
						DoubleClick();
					}else{
						m_ClickTime = GetFrameCount();
						if(m_Renamable && IsRenamable(m_FocusItem)){
							if(m_FocusItem->m_EditBox)
								m_FocusItem->EndRename(true);
							else m_RenameWait = true;
						}
					}
				}
			}
			m_State = 0;
			break;
		}
		if(m_Commander){
			switch(GetButton(DIM_RIGHT)){
			case S_PUSH:
				if(m_FocusItem && m_FocusItem->m_EditBox){
					m_FocusItem->EndRename(true);
					return true;
				}
				int oldindex = m_FocusIndex;
				CListElement *oldfocus = m_FocusItem, *popitem;
				m_FocusIndex = HitTest(pos.x, pos.y);
				if(popitem = m_FocusItem = GetElement(m_FocusIndex)){
					SetSelectionMark(m_FocusIndex, 0);
				}else{
					SetSelectionMark(-1, 0);
					m_FocusIndex = oldindex;
					m_FocusItem = oldfocus;
				}
				CPopMenu *pop = m_Commander->Dispatch(m_CmdType, (DWORD)popitem);
				GiveFocus(!pop);
				if(pop) pop->Popup(pos.x, pos.y);
				return true;
			}
		}
	}else{
		if(m_State){
			switch(GetButton(DIM_LEFT)){
			case S_HOLD:
				if(CDragContainer::IsDragging()) return CInterface::ScanInput();
				if(Manhattan(pos, m_DownPos)>DRAG_THD) PrepareDrag();
				return true;
			default:
				m_State = 0;
				break;
			}
		}
	}
	if(m_FocusItem && m_FocusItem->m_EditBox){
		switch(m_FocusItem->m_EditBox->ScanInput()){
		case EDIT_OK:
			m_FocusItem->EndRename(true);
			break;
		case EDIT_CANCEL:
			m_FocusItem->EndRename(false);
			break;
		}
	}else if(IsFocus() && m_FocusItem){
		static int rep;
		int s;
		if((s = GetKey(DIK_UP))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				if(m_FocusIndex>0){
					if(m_MultiSelect && CheckShift())
						SetSelectionArea(m_LastSel, --m_FocusIndex, false);
					else if(m_MultiSelect && CheckCtrl()) m_FocusIndex--;
					else SetSelectionMark(--m_FocusIndex, false);
					m_FocusItem = GetElement(m_FocusIndex);
				}
				EnsureVisible(m_FocusIndex);
			}
		}else if((s = GetKey(DIK_DOWN))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				if(m_FocusIndex<m_ItemNum-1){
					if(m_MultiSelect && CheckShift())
						SetSelectionArea(m_LastSel, ++m_FocusIndex, false);
					else if(m_MultiSelect && CheckCtrl()) m_FocusIndex++;
					else SetSelectionMark(++m_FocusIndex, false);
					m_FocusItem = GetElement(m_FocusIndex);
				}
				EnsureVisible(m_FocusIndex);
			}
		}else if(GetKey(DIK_SPACE)==S_PUSH){
			if(CheckShift()) SetSelectionArea(
				m_LastSel, m_FocusIndex, CheckCtrl() ? 1 : 0);
			else SetSelectionMark(m_FocusIndex, CheckCtrl() ? 2 : 0);
		}else if(m_Renamable && CheckCtrl() && GetKey(DIK_F2)==S_PUSH){
			if(IsRenamable(m_FocusItem)) m_FocusItem->BeginRename();
		}else if((GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH){
			DoubleClick();
		}else if(GetWheel()<0){
			m_ScrollV.SetScroll(m_ScrollV.GetScroll()+3);
		}else if(GetWheel()>0){
			m_ScrollV.SetScroll(m_ScrollV.GetScroll()-3);
		}
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CListView::Render(){
	CInterface::RenderBrother();
	CInterface::RenderChild();
	int i, j, px, py;
	GetAbsPos(&px, &py);
	devSetTexture(0, NULL);
	int tx = px;
	for(j = 0; j<m_Cols; j++){
		int tw = m_ColHeader[j].GetWidth();
		Grad2DRect(tx, py+TILE_UNIT, tx+tw, py+m_Height,
			j&1 ? g_Skin->m_ListViewData.m_DefaultBaseColorEven
			: g_Skin->m_ListViewData.m_DefaultBaseColorOdd);
		tx += tw;
	}
	int rest = px+m_Width-TILE_UNIT-tx;
	if(rest>0){
		Grad2DRect(tx, py+TILE_UNIT, tx+rest, py+m_Height,
			j&1 ? g_Skin->m_ListViewData.m_DefaultBaseColorEven
			: g_Skin->m_ListViewData.m_DefaultBaseColorOdd);
		g_Skin->SetInterfaceTexture();
		if(rest<2*TILE_UNIT){
			int w1 = rest/2, w2 = rest-w1;
			SetUVMap(0.0f, 0.5f, 0.125f*w1/TILE_UNIT, 0.625f);
			TexMap2DRect(tx, py, tx+w1, py+TILE_UNIT, 0xffffffff);
			SetUVMap(0.375f-0.125*w2/TILE_UNIT, 0.5f, 0.375f, 0.625f);
			TexMap2DRect(tx+w1, py, tx+rest, py+TILE_UNIT, 0xffffffff);
		}else{
			SetUVMap(0.0f, 0.5f, 0.125f, 0.625f);
			TexMap2DRect(tx, py, tx+TILE_UNIT, py+TILE_UNIT, 0xffffffff);
			SetUVMap(0.125f, 0.5f, 0.25f, 0.625f);
			TexMap2DRect(tx+TILE_UNIT, py, tx+rest-TILE_UNIT, py+TILE_UNIT, 0xffffffff);
			SetUVMap(0.25f, 0.5f, 0.375f, 0.625f);
			TexMap2DRect(tx+rest-TILE_UNIT, py, tx+rest, py+TILE_UNIT, 0xffffffff);
		}
	}
	int ofs = m_ScrollV.GetScroll(), ty = py+TILE_UNIT, by = py+m_Height;
	CListElement *ie = m_Data;
	for(i = 0; ie; ie = ie->m_Next, i++){
		if(i<ofs) continue;
		int th = by-ty;
		if(th<=0) break;
		if(th>m_RowHeight) th = m_RowHeight;
		if(ie->Render(px, ty, m_Width-TILE_UNIT, th,
			IsFocus() && i==m_FocusIndex, ie==m_DropItem)) break;
		ty += m_RowHeight;
	}
	if(m_Insertable && CDragContainer::GetOwner()==this){
		POINT pos = g_Cursor.GetPos();
		if(IsInsideList(pos.x, pos.y)){
			devSetTexture(0, NULL);
			int insert_row = CalcInsertRow(pos.y);
			int insert_py = py+(insert_row-ofs)*m_RowHeight+TILE_UNIT;
			Fill2DRect(px, insert_py-1, px+m_Width-TILE_UNIT, insert_py+1,
				g_Skin->m_ListViewData.m_FocusFrameColor);
		}
	}
	if(IsFocus()) DrawFocusFrame();
	m_DropItem = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	初期化
 */
void CIconListView::Init(
	int x, int y,			//	座標
	int w, int h,			//	サイズ
	CInterface *p,			//	親
	int cols,				//	列数
	char **coln,			//	列名
	DRAGTYPE drag,			//	ドラッグタイプ
	DWORD flags,			//	フラグ
	CMenuCommander *cmd,	//	コマンダ
	CMDTYPE ctype			//	コマンドタイプ
){
	CListView::Init(x, y, w, h, p, cols, coln, drag, flags, cmd, ctype);
	m_RowHeight = TILE_UNIT;
}

/*
 *	アイテム挿入
 */
CListElement *CIconListView::InsertItem(
	int index,		//	行
	char *str,		//	文字列
	LPTEX8 icon,	//	アイコンテクスチャ
	float *rect		//	アイコン位置
){
	return CListView::InsertItem(
		index, new CIconListElement(m_Cols, str, this, icon, rect));
}
