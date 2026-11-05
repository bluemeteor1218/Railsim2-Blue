#include "stdafx.h"
#include "CEditCtrl.h"
#include "CSkinPlugin.h"
#include "CInterfaceMode.h"

//	内部グローバル
float g_SkinFileIconRect[4] = {0.875f, 0.75f, 0.125f, 0.125f};	//	ファイルアイコン領域

/*
 *	コンストラクタ
 */
CTreeFileElement::CTreeFileElement(
	char *str,			//	文字列
	CTreeDirElement *p,	//	親
	CPluginTree *ctrl,	//	ツリーコントロール
	CPlugin *pi			//	プラグイン
):
	CTreeElement(str, p, ctrl)	//	基本クラス
{
	m_CommandType = CMD_PITVELEM;
	m_Plugin = pi;
}

/*
 *	リネーム可能か
 */
bool CTreeFileElement::IsRenamable(){
	return m_Plugin && m_Plugin->IsRenamable();
}

/*
 *	リネーム確認
 */
bool CTreeFileElement::ConfirmRename(
	string &newname	//	新規名
){
	return m_Plugin && m_Plugin->ConfirmRename(newname);
}

/*
 *	デリート可能か
 */
bool CTreeFileElement::IsDeletable(){
	return m_Plugin && m_Plugin->IsDeletable();
}

/*
 *	リストビューに要素を追加
 */
void CTreeFileElement::PushListElement(
	CPluginListView *lv	//	リストビュー
){
	int index = lv->GetItemNum();
	LPTEX8 icon = m_Plugin->GetIconTexture();
	CListElement *le = lv->InsertItem(index, (char *)m_Plugin->m_Name.c_str(),
		icon, icon ? m_Plugin->GetIconRect() : g_SkinFileIconRect);
	le->SetData((DWORD)this);
	le->SetString(1, (char *)m_Plugin->m_ID.c_str());
	le->SetString(2, (char *)m_Plugin->m_Author.c_str());
}

/*
 *	保存
 */
void CTreeFileElement::Save(
	FILE *file,		//	ファイル
	string indent	//	インデント量
){
	fprintf(file, "%sFile = %s, \"%s\";\n",
		indent.c_str(), m_Plugin->DirName(), m_Plugin->SaveString());
}

/*
 *	アイテムカウント
 */
int CTreeFileElement::CountItem(
	int sum,	//	それまでの合計
	int *fcp,	//	フォーカスアイテムの位置
	bool exflag	//	展開されているもののみ
){
	if(fcp && m_Owner->GetFocusItem()==this) *fcp = sum;
	return 1;
}

/*
 *	入力チェック
 */
int CTreeFileElement::ScanInput(
	int x, int y,	//	座標
	int cnt,		//	表示カウント
	int begin,		//	開始位置
	int end			//	終了位置
){
	if(m_Owner->GetFocusItem()==this) ms_FocusNext = this;
	else if(!ms_FocusNext) ms_FocusPrev = this;
	else if(ms_FocusNext==m_Owner->GetFocusItem()) ms_FocusNext = this;
	POINT pos = g_Cursor.GetPos();
	if(m_EditBox && !IsInsideItem(x, y, pos.x, pos.y)
		&& GetButton(DIM_LEFT)==S_PUSH) EndRename(true);
	if(m_EditBox){
		switch(m_EditBox->ScanInput()){
		case EDIT_OK:
			EndRename(true);
			break;
		case EDIT_CANCEL:
			EndRename(false);
			break;
		}
	}
	if(cnt<begin || end<=cnt) return 1;
	if(m_RenameWait && GetFrameCount()-m_ClickTime==DBLCLK_FRAME && m_Parent
		&& m_Owner->IsFocus() && m_Owner->GetFocusItem()==this){
		m_ClickTime = 0;
		BeginRename();
	}else if(IsInsideItem(x, y, pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			m_RenameWait = false;
			if(m_Owner->IsFocus() && m_Owner->GetFocusItem()==this){
				if(m_EditBox){
					EndRename(true);
					m_State = 2;
				}else{
					m_State = 3;
				}
			}else{
				m_State = 2;
			}
			m_Owner->SetFocusItem(this);
			m_Owner->GiveFocus();
			m_Owner->m_PushedItem = this;
			m_DownPos = pos;
			break;
		case S_HOLD:
			if(CDragContainer::GetType()==DRAG_PLUGIN) m_Owner->SetDropItem(
				m_Owner->GetFocusItem()!=m_Parent ? m_Parent : NULL);
			if(m_Parent && m_State && Manhattan(pos, m_DownPos)>DRAG_THD
				&& !CDragContainer::IsDragging()) PrepareDrag();
			break;
		default:
			if(CDragContainer::GetType()==DRAG_PLUGIN
				&& (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=m_Parent)){
				m_Parent->Drop();
				return true;
			}
			switch(m_State){
			case 2:
				m_ClickTime = GetFrameCount();
				break;
			case 3:
				if(GetFrameCount()-m_ClickTime<DBLCLK_FRAME){
					m_ClickTime = 0;
					m_Owner->m_Commander->DoubleClick(m_CommandType, (DWORD)GetPlugin());
					g_Skin->MouseUp();
				}else{
					m_RenameWait = true;
					m_ClickTime = GetFrameCount();
				}
				break;
			}
			m_State = 0;
			break;
		}
		if(m_Owner->m_Commander && GetButton(DIM_RIGHT)==S_PUSH){
			CPopMenu *pop = m_Owner->m_Commander->Dispatch(m_CommandType, (DWORD)GetPlugin());
			if(pop){
				m_Owner->SetFocusItem(this);
				m_Owner->GiveFocus(false);
				pop->Popup(pos.x, pos.y);
			}
		}
	}else if(m_State){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			if(m_Parent && Manhattan(pos, m_DownPos)>DRAG_THD
				&& !CDragContainer::IsDragging()) PrepareDrag();
			break;
		default:
			m_State = 0;
			break;
		}
	}
	return 1;
}

/*
 *	レンダリング
 */
int CTreeFileElement::Render(
	int x, int y,	//	座標
	int cnt,		//	表示カウント
	int begin,		//	開始位置
	int end			//	終了位置
){
	if(m_EditBox && (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=this))
		EndRename(true);
	if(cnt<begin) return 1;
	CStringDrawer *sd = g_StrTex->DrawString(m_String.c_str(), 0);
	D3DCOLOR *bc = m_Owner->GetFocusItem()==this
		? g_Skin->m_PluginTreeData.m_SelectedBaseColor
		: g_Skin->m_PluginTreeData.m_DefaultBaseColor;
	if(m_Width<0) m_Width = sd->GetWidth();
	if(*bc){
		devSetTexture(0, NULL);
		Grad2DRect(x+TILE_UNIT+TREE_ICON_OFS, y, x+TILE_UNIT+TREE_ICON_OFS
			+TREE_ICON_MARGIN*2+m_Width, y+TILE_UNIT, bc);
	}
	if(m_Owner->IsFocus() && m_Owner->GetFocusItem()==this)
		Draw2DRect(x+TILE_UNIT+TREE_ICON_OFS, y, x+TILE_UNIT+TREE_ICON_OFS
			+TREE_ICON_MARGIN*2+m_Width, y+TILE_UNIT,
			g_Skin->m_PluginTreeData.m_FocusFrameColor);
	m_Plugin->SetIconTexture();
	TexMap2DRect(x, y, x+TILE_UNIT, y+TILE_UNIT, 0xffffffff);
	if(m_EditBox){
		m_EditBox->SetPos(
			x+TILE_UNIT+TREE_ICON_OFS+EB_OFSX, y+FontY(TILE_UNIT));
		m_EditBox->Render();
	}else{
		sd->RenderLeft(x+TILE_UNIT
			+TREE_ICON_OFS+TREE_ICON_MARGIN, y+FontY(TILE_UNIT),
			g_Skin->m_PluginTreeData.m_DefaultFontColor);
	}
	return 1;
}
