#include "stdafx.h"
#include "CSkinPlugin.h"
#include "CInterfaceMode.h"
#include "CConfigMode.h"

/*
 *	コンストラクタ
 */
CPluginTree::CPluginTree(){
	m_PushedItem = m_FocusItem = NULL;
	m_DropItem = m_Root = NULL;
}

/*
 *	デストラクタ
 */
CPluginTree::~CPluginTree(){
	DELETE_V(m_Root);
}

/*
 *	初期化
 */
void CPluginTree::Init(
	int x, int y,			//	座標
	int w, int h,			//	サイズ
	char *t,				//	ルート名
	CInterface *p,			//	親
	CPluginListView *lv,	//	平衡リストビュー
	CMenuCommander *cmd		//	コマンダ
){
	CInterface::Init(x, y, w, h, t, p);
	m_Rows = m_Height/TILE_UNIT;
	m_FocusItem = m_Root = new CTreeDirElement(t, NULL, this);
	LinkTab(true);
	m_ScrollV.Init(0, 0, TILE_UNIT, m_Height, this);
	m_SyncList = lv;
	m_Commander = cmd;
}

/*
 *	フォーカスアイテムの設定
 */
void CPluginTree::SetFocusItem(
	CTreeElement *f	//	フォーカスアイテム
){
	if(m_FocusItem && m_FocusItem->m_EditBox) m_FocusItem->EndRename(true);
	m_PushedItem = m_FocusItem = f;
	CTreeDirElement *de = m_FocusItem->IsDirectory();
	if(de){
		de->ListDirectory();
		CPlugin::ResetPreview();
	}else{
		m_SyncList->GetParent()->Show(false);
		CTreeFileElement *fe = m_FocusItem->IsFile();
		CPlugin *pi = fe->m_Plugin->LoadAndGet();
		if(pi) pi->SetPreview();
	}
}

/*
 *	ドロップ先の設定
 */
void CPluginTree::SetDropItem(
	CTreeDirElement *d	//	アイテム
){
	if(!ms_Drop || IsFocus()){
		ms_Drop = this;
		m_DropItem = d;
	}
}

/*
 *	プラグインを選択
 */
void CPluginTree::SelectPlugin(
	CPlugin *t	//	ターゲット
){
	CTreeFileElement *fe = m_Root->FindPlugin(t);
	SetFocusItem(fe ? (CTreeElement *)fe : m_Root);
	EnsureVisible();
}

/*
 *	フォーカスアイテムを表示範囲内に
 */
void CPluginTree::EnsureVisible(){
	if(m_FocusItem){
		CTreeDirElement *de = m_FocusItem->m_Parent;
		while(de){
			de->m_Expand = true;
			de = de->m_Parent;
		}
	}
	m_Rows = m_Height/TILE_UNIT;
	int fcp, inum = m_Root->CountItem(0, &fcp, true);
	m_ScrollV.SetRange(inum ? inum : 1);
	m_ScrollV.SetPage(m_Rows);
	if(fcp>=m_ScrollV.GetScroll()+m_Rows) m_ScrollV.SetScroll(fcp-m_Rows+1);
	else if(fcp<m_ScrollV.GetScroll()) m_ScrollV.SetScroll(fcp);
}

/*
 *	スクロールバーの更新
 */
void CPluginTree::SetScroll(){
	m_Rows = m_Height/TILE_UNIT;
	int inum = m_Root->CountItem(0, NULL, true);
	m_ScrollV.SetRange(inum ? inum : 1);
	m_ScrollV.SetPage(m_Rows);
}

/*
 *	フォーカス付与
 */
void CPluginTree::GiveFocus(
	bool snd	//	音再生フラグ
){
	m_Focused = true;
	CInterface::GiveFocus(snd);
}

/*
 *	ドラッグアイテム表示
 */
void CPluginTree::RenderDragItem(){
	if(m_FocusItem) m_FocusItem->RenderDragItem();
}

/*
 *	読込
 */
char *CPluginTree::Load(
	char *str,			//	対象文字列
	char *head,			//	ヘッダ名
	CPluginMode *pimode	//	プラグインモード
){
	char *eee;
	char *tmp_head = head;
	if(g_ConfigVersion<=2.12f && !strcmp(head, "Surface")) tmp_head = "Scene";
	if(!(str = BeginBlock(eee = str, FlashIn("%sPluginTree", tmp_head)))) throw CSynErr(eee);
	if(!(str = m_Root->Load(eee = str, pimode))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CPluginTree::Save(
	FILE *file,	//	ファイル
	char *head	//	ヘッダ名
){
	fprintf(file, "%sPluginTree{\n", head);
	m_Root->Save(file, "");
	fprintf(file, "}\n\n");
}

/*
 *	入力チェック
 */
bool CPluginTree::ScanInput(){
	int px, py;
	GetAbsPos(&px, &py);
	SetScroll();
	m_Focused = false;
	m_PushedItem = NULL;
	m_Root->ScanInput(px+TILE_UNIT+TREE_ICON_OFS, py,
		0, m_ScrollV.GetScroll(), m_ScrollV.GetScroll()+m_Rows);
	if(m_Focused || m_DropItem) return true;
	if(IsFocus() && m_FocusItem && !m_FocusItem->m_EditBox){
		static int rep;
		int s;
		if((s = GetKey(DIK_UP))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				CTreeElement *tmp = CTreeElement::ms_FocusPrev;
				if(tmp) SetFocusItem(tmp);
			}
			EnsureVisible();
			return true;
		}else if((s = GetKey(DIK_DOWN))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				CTreeElement *tmp = CTreeElement::ms_FocusNext;
				if(tmp) SetFocusItem(tmp);
			}
			EnsureVisible();
			return true;
		}else if((s = GetKey(DIK_LEFT))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				CTreeDirElement *de = m_FocusItem->IsDirectory();
				if(de){
					if(de->m_Expand){
						de->m_Expand = false;
						g_Skin->MouseUp();
						EnsureVisible();
						return true;
					}
				}
				CTreeElement *tmp = m_FocusItem->m_Parent;
				if(tmp) SetFocusItem(tmp);
			}
			EnsureVisible();
			return true;
		}else if((s = GetKey(DIK_RIGHT))>=S_PUSH){
			if(PickRepeat(&rep, s)){
				CTreeDirElement *de = m_FocusItem->IsDirectory();
				if(de){
					if(!de->m_Expand){
						de->m_Expand = true;
						g_Skin->MouseUp();
						EnsureVisible();
						return true;
					}
				}
				CTreeElement *tmp = CTreeElement::ms_FocusNext;
				if(tmp) SetFocusItem(tmp);
			}
			EnsureVisible();
			return true;
		}else if(CheckCtrl() && GetKey(DIK_F2)==S_PUSH){
			if(m_FocusItem && m_FocusItem->m_Parent) m_FocusItem->BeginRename();
		}else if(GetKey(DIK_DELETE)==S_PUSH){
			if(m_FocusItem && m_FocusItem->IsDeletable()
				&& m_FocusItem->m_Parent) m_FocusItem->Delete();
		}else if(GetKey(DIK_BACK)==S_PUSH){
			if(m_FocusItem && m_FocusItem->m_Parent)
				SetFocusItem(m_FocusItem->m_Parent);
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
void CPluginTree::Render(){
	CInterface::RenderBrother();
	if(!CDragContainer::IsDragging() || ms_Drop!=this) m_DropItem = NULL;
	int px, py;
	GetAbsPos(&px, &py);
	m_Root->Render(px+TILE_UNIT+TREE_ICON_OFS, py,
		0, m_ScrollV.GetScroll(), m_ScrollV.GetScroll()+m_Rows);
	CInterface::RenderChild();
}
