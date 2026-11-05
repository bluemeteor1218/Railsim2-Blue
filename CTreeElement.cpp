#include "stdafx.h"
#include "CPluginTree.h"

//	内部定数
const int PT_NAME_MAX = 64;	//	名称最大長

//	static メンバ
CTreeElement *CTreeElement::ms_FocusPrev;
CTreeElement *CTreeElement::ms_FocusNext;

/*
 *	コンストラクタ
 */
CTreeElement::CTreeElement(
	char *str,			//	文字列
	CTreeDirElement *p,	//	親
	CPluginTree *ctrl	//	ツリーコントロール
){
	m_State = 0;
	m_Width = -1;
	m_ClickTime = 0;
	m_String = str;
	m_Parent = p;
	m_Brother = NULL;
	m_Owner = ctrl;
	m_EditBox = NULL;
}

/*
 *	親階層から自身を削除
 */
void CTreeElement::Delete(){
	if(m_Parent) m_Parent->DeleteItem(this);
}

/*
 *	指定座標が範囲内か調べる
 */
bool CTreeElement::IsInsideItem(
	int ix, int iy,	//	
	int px, int py	//	
){
	return ix<=px && px<ix+TILE_UNIT+TREE_ICON_OFS
		+TREE_ICON_MARGIN*2+m_Width && iy<=py && py<iy+TILE_UNIT;
}

/*
 *	リネーム開始
 */
void CTreeElement::BeginRename(){
	if(!IsRenamable()) return;
	m_EditBox = new CEditBox;
	m_Width = (m_String.size()+1)*FONT_WIDTH+2;
	m_EditBox->Create(0, 0, m_Width-2, PT_NAME_MAX, m_String);
	m_Owner->GiveFocus(false);
}

/*
 *	リネーム終了
 */
void CTreeElement::EndRename(
	bool update	//	更新フラグ
){
	if(update){
		string newname;
		m_EditBox->GetText(newname);
		if(newname.size() && ConfirmRename(newname)){
			m_String = newname;
			m_Parent->Sort(false);
		}
	}
	DELETE_V(m_EditBox);
	m_Width = -1;
}

/*
 *	ドラッグ操作の準備
 */
void CTreeElement::PrepareDrag(){
	m_State = 1;
	CDragContainer::BeginDrag(DRAG_PLUGIN, m_Owner);
	CDragContainer::Insert((DWORD)this);
}

/*
 *	ドラッグアイテム表示
 */
void CTreeElement::RenderDragItem(){
	POINT pos = g_Cursor.GetPos();
	Render(pos.x-TILE_UNIT/2, pos.y-TILE_UNIT/2, 0, 0, -1);
}

/*
 *	ソート用比較関数
 */
int CTreeElement::Compare(
	CTreeElement *rhs	//	右辺
){
	int ret;
	if(IsDirectory()){
		if(rhs->IsDirectory()){
			ret = _mbsicmp(
				(PUCHAR)m_String.c_str(), (PUCHAR)rhs->m_String.c_str());
		}else{
			return -1;
		}
	}else{
		if(rhs->IsDirectory()){
			return 1;
		}else{
			ret = _mbsicmp(
				(PUCHAR)m_String.c_str(), (PUCHAR)rhs->m_String.c_str());
		}
	}
	return ret ? ret : (this<rhs ? 1 : -1);
}

/*
 *	マージソート
 */
void CTreeElement::MergeSort(
	CTreeElement **seed,	//	開始ポインタ
	CTreeElement *end		//	終了ポインタ
){
	int num1 = 0, num2 = 0, id = 0;
	CTreeElement *ptr = this, *mid = this, *last = this, **carrier = seed;
	while(ptr!=end){
		if(num1>num2){
			num2++;
		}else{
			num1++;
			last = mid;
			mid = mid->m_Brother;
		}
		ptr = ptr->m_Brother;
	}
	if(num1>1) MergeSort(seed, mid);
	if(num2>1) mid->MergeSort(&mid, end);
	CTreeElement *adr1 = *seed, *adr2 = mid;
	while(true){
		int *cnt;
		CTreeElement **next;
		if(num1){
			if(num2){
				if(adr1->Compare(adr2)<0){
					next = &adr1; cnt = &num1;
				}else{
					next = &adr2; cnt = &num2;
				}
			}else{
				next = &adr1; cnt = &num1;
			}
		}else{
			if(num2){
				next = &adr2; cnt = &num2;
			}else{
				break;
			}
		}
		*carrier = *next;
		carrier = &(*next)->m_Brother;
		*next = (*next)->m_Brother;
		(*cnt)--;
		id++;
	}
	*carrier = NULL;
}

/*
 *	開く
 */
void CTreeElement::Open(){
	m_Owner->SetFocusItem(this);
	m_Owner->EnsureVisible();
	if(IsFile()) m_Owner->GiveFocus(false);
}
