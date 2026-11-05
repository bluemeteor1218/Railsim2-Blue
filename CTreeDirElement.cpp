#include "stdafx.h"
#include "CSimpleDialog.h"
#include "CSkinPlugin.h"
#include "CInterfaceMode.h"

//	内部グローバル
float g_SkinDirIconRect[4] = {0.625f, 0.75f, 0.125f, 0.125f};	//	フォルダアイコン領域

//	static メンバ
CPopMenu *CTreeDirElement::ms_DirMenu;

/*
 *	メニュー初期化
 */
void CTreeDirElement::InitMenu(){
	ms_DirMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(CreateDir), ms_DirMenu);
	new CPopMenu(lang(DeleteDir), ms_DirMenu);
	new CPopMenu(lang(ChangeName), ms_DirMenu);
}

/*
 *	メニュー解放
 */
void CTreeDirElement::FreeMenu(){
	DELETE_V(ms_DirMenu);
}

/*
 *	コンストラクタ
 */
CTreeDirElement::CTreeDirElement(
	char *str,			//	文字列
	CTreeDirElement *p,	//	親
	CPluginTree *ctrl	//	ツリーコントロール
):
	CTreeElement(str, p, ctrl)	//	基本クラス
{
	m_Expand = true;
	m_Child = NULL;
	m_ExButton.Init(0, 0, TILE_UNIT, TILE_UNIT, NULL, 0.0f, 0.0f, false);
	m_ExButton.SetOwner(ctrl);
}

/*
 *	デストラクタ
 */
CTreeDirElement::~CTreeDirElement(){
	CTreeElement *ptr = m_Child;
	m_Child = NULL;
	while(ptr){
		CTreeElement *tmp = ptr->m_Brother;
		ptr->m_Brother = NULL;
		delete ptr;
		ptr = tmp;
	}
}

/*
 *	親階層から自身を削除
 */
void CTreeDirElement::Delete(){
	if(m_Parent){
		CTreeElement *ptr = m_Child;
		while(ptr){
			CTreeElement *next = ptr->m_Brother;
			m_Parent->InsertItem(ptr);
			ptr = next;
		}
		m_Child = NULL;
		CTreeDirElement *p = m_Parent;
		p->DeleteItem(this);
		p->Sort(false);
	}
}

/*
 *	アイテム挿入
 */
void CTreeDirElement::InsertItem(
	CTreeElement *item	//	挿入アイテム
){
	item->m_Parent = this;
	item->m_Brother = m_Child;
	m_Child = item;
}

/*
 *	アイテム削除
 */
void CTreeDirElement::DeleteItem(
	CTreeElement *item	//	挿入アイテム
){
	CTreeElement **ptr = &m_Child;
	while(*ptr){
		if(*ptr==item){
			if(m_Owner->m_FocusItem==item) m_Owner->m_FocusItem = NULL;
			*ptr = (*ptr)->m_Brother;
			delete item;
			m_Owner->SetFocusItem(this);
			m_Owner->GiveFocus(false);
			m_Owner->EnsureVisible();
			return;
		}
		ptr = &(*ptr)->m_Brother;
	}
}

/*
 *	アイテム検索
 */
CTreeElement **CTreeDirElement::Find(
	CTreeElement *t,	//	ターゲット
	bool rec			//	再帰
){
	CTreeElement **adr = &m_Child;
	while(*adr){
		if(*adr==t) return adr;
		if(rec){
			CTreeDirElement *de = (*adr)->IsDirectory();
			if(de){
				CTreeElement **tmp = de->Find(t, true);
				if(tmp) return tmp;
			}
		}
		adr = &(*adr)->m_Brother;
	}
	return NULL;
}

/*
 *	プラグイン検索
 */
CTreeFileElement *CTreeDirElement::FindPlugin(
	CPlugin *t	//	ターゲット
){
	CTreeElement *ptr = m_Child;
	while(ptr){
		CTreeFileElement *fe = ptr->IsFile();
		if(fe){
			if(fe->GetPlugin()==t) return fe;
		}else{
			CTreeDirElement *de = ptr->IsDirectory();
			CTreeFileElement *tmp = de->FindPlugin(t);
			if(tmp) return tmp;
		}
		ptr = ptr->m_Brother;
	}
	return NULL;
}

/*
 *	ドロップ
 */
void CTreeDirElement::Drop(){
	if(CDragContainer::GetType()!=DRAG_PLUGIN) return;
	list<DWORD> &data = CDragContainer::GetData();
	IDWORD idat = data.begin();
	CTreeDirElement *p = ((CTreeElement *)*idat)->m_Parent;
	if(p==this) return;
	for(; idat!=data.end(); idat++){
		CTreeElement *tmp = (CTreeElement *)*idat;
		if(!tmp) continue;
		CTreeDirElement *de = tmp->IsDirectory();
		if(de==this || de && de->Find(this, true)){
			EnqueueCommonDialog(new CSimpleDialog(
				lang(DestIsSubdirOfSrc), lang(Error)));
			g_Skin->Error();
			return;
		}
	}
	for(idat = data.begin(); idat!=data.end(); idat++){
		CTreeElement *tmp = (CTreeElement *)*idat;
		*p->Find(tmp, false) = tmp->m_Brother;
		tmp->m_Brother = NULL;
	}
	for(idat = data.begin(); idat!=data.end(); idat++)
		InsertItem((CTreeElement *)*idat);
	Sort(false);
	g_Skin->MouseUp();
	CDragContainer::EndDrag();
	CTreeDirElement *de = m_Owner->GetFocusItem()->IsDirectory();
	if(de) de->ListDirectory();
}

/*
 *	ディレクトリ内をリスティング
 */
void CTreeDirElement::ListDirectory(){
	if(g_RSPV) return;
	CPluginListView *lv = m_Owner->m_SyncList;
	lv->DeleteAllItems();
	lv->m_ShowDir = this;
	CTreeElement *ptr = m_Child;
	while(ptr){
		ptr->PushListElement(lv);
		ptr = ptr->m_Brother;
	}
	lv->m_Parent->Show(true);
}

/*
 *	リストビューに要素を追加
 */
void CTreeDirElement::PushListElement(
	CPluginListView *lv	//	リストビュー
){
	int index = lv->GetItemNum();
	lv->InsertItem(index, (char *)m_String.c_str(), NULL, g_SkinDirIconRect);
	lv->GetElement(index)->SetData((DWORD)this);
}

/*
 *	新規ディレクトリ
 */
void CTreeDirElement::CreateNewDir(){
	CTreeDirElement *nd = new CTreeDirElement(lang(NewDir), this, m_Owner);
	InsertItem(nd);
	m_Owner->SetFocusItem(nd);
	m_Owner->GiveFocus(false);
	m_Owner->EnsureVisible();
	nd->BeginRename();
}

/*
 *	ディレクトリ内ソート
 */
void CTreeDirElement::Sort(
	bool rec	//	再帰ソート
){
	m_Child->MergeSort(&m_Child, NULL);
	if(!rec) return;
	CTreeElement *ptr = m_Child;
	while(ptr){
		CTreeDirElement *de = ptr->IsDirectory();
		if(de) de->Sort(true);
		ptr = ptr->m_Brother;
	}
}

/*
 *	読込
 */
char *CTreeDirElement::Load(
	char *str,			//	対象文字列
	CPluginMode *pimode	//	プラグインモード
){
	char *tmp, *eee;
	if(m_Parent && !(str = BeginNamedBlock(
		eee = str, "Directory", &m_String))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "Expand", &m_Expand))) throw CSynErr(eee);
	while(true){
		if(tmp = Assignment(str, "File")){
			str = tmp;
			string type, id;
			if(!(str = Identifier(eee = str, &type))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
			if(!(str = StringLiteral(eee = str, &id))) throw CSynErr(eee);
			if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
			CPlugin *pi = pimode->FindPluginBase((char *)type.c_str(), (char *)id.c_str());
			if(pi && !pi->IsInserted()) pi->InsertItem(this, m_Owner);
		}else if(tmp = Identifier2(str, "Directory")){
			CTreeDirElement *nd = new CTreeDirElement("", this, m_Owner);
			if(!(str = nd->Load(str, pimode))) throw CSynErr(eee);
			InsertItem(nd);
		}else{
			break;
		}
	}
	Sort(false);
	if(m_Parent && !(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CTreeDirElement::Save(
	FILE *file,		//	ファイル
	string indent	//	インデント量
){
	if(m_Parent) fprintf(file, "%sDirectory \"%s\"{\n",
		indent.c_str(), ExpandDoubleQuote(m_String).c_str());
	string indent2 = indent+"\t";
	fprintf(file, "%sExpand = %s;\n", indent2.c_str(), YESNO[m_Expand]);
	CTreeElement *ptr = m_Child;
	while(ptr){
		ptr->Save(file, indent2);
		ptr = ptr->m_Brother;
	}
	if(m_Parent) fprintf(file, "%s}\n", indent.c_str());
}

/*
 *	アイテムカウント
 */
int CTreeDirElement::CountItem(
	int sum,	//	それまでの合計
	int *fcp,	//	フォーカスアイテムの位置
	bool exflag	//	展開されているもののみ
){
	if(fcp && m_Owner->GetFocusItem()==this) *fcp = sum;
	if(exflag && !m_Expand) return 1;
	CTreeElement *ptr = m_Child;
	int inum = 1;
	while(ptr){
		inum += ptr->CountItem(sum+inum, fcp, exflag);
		ptr = ptr->m_Brother;
	}
	return inum;
}

/*
 *	入力チェック
 */
int CTreeDirElement::ScanInput(
	int x, int y,	//	座標
	int cnt,		//	表示カウント
	int begin,		//	開始位置
	int end			//	終了位置
){
	if(!m_Parent) ms_FocusNext = NULL;
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
	if(begin<=cnt && cnt<end){
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
					m_Owner->GetFocusItem()!=this ? this : NULL);
				if(m_Parent && m_State && Manhattan(pos, m_DownPos)>DRAG_THD
					&& !CDragContainer::IsDragging()) PrepareDrag();
				break;
			default:
				if(CDragContainer::GetType()==DRAG_PLUGIN
					&& (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=this)){
					Drop();
					return true;
				}
				switch(m_State){
				case 2:
					m_ClickTime = GetFrameCount();
					break;
				case 3:
					if(GetFrameCount()-m_ClickTime<DBLCLK_FRAME){
						m_ClickTime = 0;
						m_Expand = !m_Expand;
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
			if(GetButton(DIM_RIGHT)==S_PUSH){
				class CTreeDirCreator: public CMenuCommand{
				private:
					CTreeDirElement *m_Parent;
				public:
					CTreeDirCreator(CTreeDirElement *p){ m_Parent = p; }
					void Exec(){ m_Parent->CreateNewDir(); }
				};
				class CTreeDirDeleter: public CMenuCommand{
				private:
					CTreeDirElement *m_Parent;
				public:
					CTreeDirDeleter(CTreeDirElement *p){ m_Parent = p; }
					void Exec(){ m_Parent->Delete(); }
				};
				class CTreeDirRenamer: public CMenuCommand{
				private:
					CTreeDirElement *m_Parent;
				public:
					CTreeDirRenamer(CTreeDirElement *p){ m_Parent = p; }
					void Exec(){ m_Parent->BeginRename(); }
				};
				m_Owner->SetFocusItem(this);
				m_Owner->GiveFocus(false);
				if(m_Parent){
					ms_DirMenu->GetMenu(1)->Enable(true);
					ms_DirMenu->GetMenu(2)->Enable(true);
					ms_DirMenu->GetMenu(0)->SetCommand(new CTreeDirCreator(this));
					ms_DirMenu->GetMenu(1)->SetCommand(new CTreeDirDeleter(this));
					ms_DirMenu->GetMenu(2)->SetCommand(new CTreeDirRenamer(this));
					ms_DirMenu->Popup(pos.x, pos.y);
				}else{
					ms_DirMenu->GetMenu(1)->Enable(false);
					ms_DirMenu->GetMenu(2)->Enable(false);
					ms_DirMenu->GetMenu(0)->SetCommand(new CTreeDirCreator(this));
					ms_DirMenu->Popup(pos.x, pos.y);
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
		m_ExButton.ScanInput();
		if(m_ExButton.IsPushed()){
			if(!(m_Expand = !m_Expand)){
				CTreeDirElement *ptr = m_Owner->GetFocusItem()->m_Parent;
				while(ptr){
					if(!ptr->m_Expand) m_Owner->SetFocusItem(ptr);
					ptr = ptr->m_Parent;
				}
			}
		}
		y += TILE_UNIT;
	}
	if(!m_Expand) return 1;
	int i, tcnt = 1;
	CTreeElement *ptr = m_Child;
	while(ptr){
		int tmp = ptr->ScanInput(x+TILE_UNIT, y, cnt+tcnt, begin, end);
		if(ptr = ptr->m_Brother){
			if(cnt+tcnt>=begin) y += TILE_UNIT;
			for(i = 1; i<tmp; i++) if(cnt+tcnt+i>=begin) y += TILE_UNIT;
		}
		tcnt += tmp;
	}
	return tcnt;
}

/*
 *	レンダリング
 */
int CTreeDirElement::Render(
	int x, int y,	//	座標
	int cnt,		//	表示カウント
	int begin,		//	開始位置
	int end			//	終了位置
){
	if(m_EditBox && (!m_Owner->IsFocus() || m_Owner->GetFocusItem()!=this))
		EndRename(true);
	if(cnt>=begin){
		CStringDrawer *sd = g_StrTex->DrawString(m_String.c_str(), 0);
		D3DCOLOR *bc, fc;
		if(m_Owner->GetFocusItem()==this || m_Owner->GetDropItem()==this){
			bc = g_Skin->m_PluginTreeData.m_SelectedBaseColor;
			fc = g_Skin->m_PluginTreeData.m_SelectedFontColor;
		}else{
			bc = g_Skin->m_PluginTreeData.m_DefaultBaseColor;
			fc = g_Skin->m_PluginTreeData.m_DefaultFontColor;
		}
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
		g_Skin->SetInterfaceTexture();
		float tu = m_Expand ? 0.125f : 0.0f;
		float tu2 = m_Owner->GetFocusItem()==this ? 0.75f : 0.625f;
		if(m_Parent && m_Child && end>=0){
			m_ExButton.SetPos(x-TILE_UNIT, y);
			m_ExButton.SetUV(tu, 0.75f);
			m_ExButton.Render();
		}
		SetUVMap(tu2, 0.75f, tu2+0.125f, 0.875f);
		TexMap2DRect(x, y, x+TILE_UNIT, y+TILE_UNIT, 0xffffffff);
		if(m_EditBox){
			m_EditBox->SetPos(
				x+TILE_UNIT+TREE_ICON_OFS+EB_OFSX, y+FontY(TILE_UNIT));
			m_EditBox->Render();
		}else{
			sd->RenderLeft(x+TILE_UNIT+TREE_ICON_OFS+TREE_ICON_MARGIN, y+FontY(TILE_UNIT), fc);
		}
		y += TILE_UNIT;
	}
	if(!m_Expand) return 1;
	int i, tcnt = 1;
	CTreeElement *ptr = m_Child;
	while(ptr && cnt+tcnt<end){
		float tu = ptr->m_Brother ? 0.75f : 0.875f;
		if(cnt+tcnt>=begin){
			g_Skin->SetInterfaceTexture();
			SetUVMap(tu, 0.875f, tu+0.125f, 1.0f);
			TexMap2DRect(x, y, x+TILE_UNIT, y+TILE_UNIT, 0xffffffff);
		}
		int tmp = ptr->Render(x+TILE_UNIT, y, cnt+tcnt, begin, end);
		if(ptr = ptr->m_Brother){
			if(cnt+tcnt>=begin) y += TILE_UNIT;
			g_Skin->SetInterfaceTexture();
			for(i = 1; i<tmp; i++){
				if(cnt+tcnt+i>=begin){
					SetUVMap(0.625f, 0.875f, 0.75f, 1.0f);
					TexMap2DRect(x, y, x+TILE_UNIT, y+TILE_UNIT, 0xffffffff);
					y += TILE_UNIT;
				}
			}
		}
		tcnt += tmp;
	}
	return tcnt;
}
