#include "stdafx.h"
#include "CInterface.h"
#include "CPopMenu.h"
#include "CSkinPlugin.h"

//	内部定数
const int MENU_RECT_MARGIN = 4;	//	選択部余白

//	static メンバ
CPopMenu *CPopMenu::ms_ActiveMenu = NULL;

/*
 *	[static]
 *	メニュー処理
 */
int CPopMenu::ScanInputAll(){
	if(!ms_ActiveMenu) return 0;
	if(CInterface::ms_Focus){
		ms_ActiveMenu = NULL;
		return 0;
	}
	int ret = ms_ActiveMenu->ScanInput();
	if(ret>0 || !ret && (GetButton(DIM_LEFT)==S_PUSH
		|| GetButton(DIM_RIGHT)==S_PUSH)) ms_ActiveMenu = NULL;
	return ret;
}

/*
 *	[static]
 *	メニュー表示
 */
void CPopMenu::RenderAll(){
	if(!ms_ActiveMenu) return;
	ms_ActiveMenu->Render();
}

/*
 *	コンストラクタ
 */
CPopMenu::CPopMenu(
	char *str,	//	文字列
	CPopMenu *p	//	親
){
	m_ChildNum = 0;
	m_Width = TILE_UNIT*3/2;
	m_Height = TILE_UNIT;
	m_Enabled = true;
	m_String = str;
	m_Command = NULL;
	m_Parent = m_Brother = m_Child = NULL;
	if(p) p->InsertMenu(this);
}

/*
 *	デストラクタ
 */
CPopMenu::~CPopMenu(){
	DELETE_V(m_Child);
	DELETE_V(m_Brother);
	ClearCommand();
}

/*
 *	子メニュー挿入
 */
void CPopMenu::InsertMenu(
	CPopMenu *menu	//	メニュー
){
	CPopMenu **adr = &m_Child;
	while(*adr) adr = &(*adr)->m_Brother;
	*adr = menu;
	menu->m_Parent = this;
	m_ChildNum++;
	int tw = g_StrTex->DrawString(menu->m_String.c_str(), 0)->GetWidth();
	if(m_Width<tw+TILE_UNIT*3/2) m_Width = tw+TILE_UNIT*3/2;
	m_Height += TILE_UNIT;
}

/*
 *	指定座標が範囲内か調べる
 */
bool CPopMenu::IsInside(
	int x, int y	//	座標
){
	return m_PosX<=x && x<m_PosX+m_Width && m_PosY<=y && y<=m_PosY+m_Height;
}

/*
 *	指定座標に対応する番号を取得
 */
int CPopMenu::HitTest(
	int x, int y	//	座標
){
	if(x<m_PosX+MENU_RECT_MARGIN || m_PosX+m_Width-MENU_RECT_MARGIN<=x
		|| y<m_PosY+TILE_UNIT/2) return -1;
	int index = (y-m_PosY-TILE_UNIT/2)/TILE_UNIT;
	return m_ChildNum<=index ? -1 : index;
}

/*
 *	番号からメニュー取得
 */
CPopMenu *CPopMenu::GetMenu(
	int index	//	番号
){
	if(index<0 || m_ChildNum<=index) return NULL;
	CPopMenu *ptr = m_Child;
	while(index--) ptr = ptr->m_Brother;
	return ptr;
}

/*
 *	メニューポップアップ
 */
void CPopMenu::Popup(
	int x, int y,		//	左上ポップアップ座標
	int tw, int th,		//	呼び出し元領域サイズ
	int dir				//	ポップアップ方向 (0: 左上, 1: 右上, 2: 左下, 3: 右下)
){
	CInterface::ms_Focus = NULL;
	m_Pointed = NULL;
	if(dir&1){
		if(x+m_Width>=g_DispWidth) dir ^= 1;
	}else{
		if(x-tw-m_Width<0) dir |= 1;
	}
	if(dir&2){
		if(y+m_Height>=g_DispHeight) dir ^= 2;
	}else{
		if(y+th-m_Height<0) dir |= 2;
	}
	if(!(dir&1)) x -= tw+m_Width;
	if(!(dir&2)) y -= m_Height-th;
	m_PosX = x; m_PosY = y;
	m_Expand = dir;
	if(!ms_ActiveMenu) ms_ActiveMenu = this;
	g_Skin->MouseDown();
	CPopMenu *ptr = m_Child;
	while(ptr){
		ptr->m_Expand = -1;
		ptr = ptr->m_Brother;
	}
}

/*
 *	入力チェック
 *
 *	戻り値: 0: none, >0: command, <0: inside
 */
int CPopMenu::ScanInput(){
	CPopMenu *ptr = m_Child;
	bool exp = false;
	while(ptr){
		if(ptr->IsExpand()){
			if(ptr!=m_Pointed){
				ptr->m_Expand = -1;
			}else{
				int tmp = ptr->ScanInput();
				if(tmp) return tmp;
				exp = true;
			}
		}
		ptr = ptr->m_Brother;
	}
	POINT pos = g_Cursor.GetPos();
	int index = HitTest(pos.x, pos.y);
	CPopMenu *pm = GetMenu(index);
	if(pm && (!pm->m_Enabled || pm->m_String=="-")) pm = NULL;
	if(!exp || pm && pm!=m_Pointed){
		m_Pointed = pm;
		if(m_Pointed && m_Pointed->m_Child) m_Pointed->Popup(
			m_PosX+m_Width-MENU_RECT_MARGIN, m_PosY+TILE_UNIT/2+index*TILE_UNIT,
			m_Width-2*MENU_RECT_MARGIN, TILE_UNIT, m_Expand);
	}
	if(GetButton(DIM_LEFT)==S_PULL || GetButton(DIM_RIGHT)==S_PULL){
		if(!pm) return 0;
		if(pm->m_Child) return -1;
		g_Skin->MouseUp();
		if(pm->m_Command) pm->m_Command->Exec();
		return 1;
	}
	return IsInside(pos.x, pos.y) ? -1 : 0;
}

/*
 *	レンダリング
 */
void CPopMenu::Render(){
	g_Skin->SetInterfaceTexture();
	SetUVMap(0.625f, 0.375f, 0.75f, 0.5f);
	TexMap2DRect(m_PosX, m_PosY, m_PosX+TILE_UNIT, m_PosY+TILE_UNIT, 0xffffffff);
	SetUVMap(0.75f, 0.375f, 0.875f, 0.5f);
	TexMap2DRect(m_PosX+TILE_UNIT, m_PosY, m_PosX+m_Width-TILE_UNIT, m_PosY+TILE_UNIT, 0xffffffff);
	SetUVMap(0.875f, 0.375f, 1.0f, 0.5f);
	TexMap2DRect(m_PosX+m_Width-TILE_UNIT, m_PosY, m_PosX+m_Width, m_PosY+TILE_UNIT, 0xffffffff);
	SetUVMap(0.625f, 0.5f, 0.75f, 0.625f);
	TexMap2DRect(m_PosX, m_PosY+TILE_UNIT, m_PosX+TILE_UNIT, m_PosY+m_Height-TILE_UNIT, 0xffffffff);
	SetUVMap(0.75f, 0.5f, 0.875f, 0.625f);
	TexMap2DRect(m_PosX+TILE_UNIT, m_PosY+TILE_UNIT, m_PosX+m_Width-TILE_UNIT, m_PosY+m_Height-TILE_UNIT, 0xffffffff);
	SetUVMap(0.875f, 0.5f, 1.0f, 0.625f);
	TexMap2DRect(m_PosX+m_Width-TILE_UNIT, m_PosY+TILE_UNIT, m_PosX+m_Width, m_PosY+m_Height-TILE_UNIT, 0xffffffff);
	SetUVMap(0.625f, 0.625f, 0.75f, 0.75f);
	TexMap2DRect(m_PosX, m_PosY+m_Height-TILE_UNIT, m_PosX+TILE_UNIT, m_PosY+m_Height, 0xffffffff);
	SetUVMap(0.75f, 0.625f, 0.875f, 0.75f);
	TexMap2DRect(m_PosX+TILE_UNIT, m_PosY+m_Height-TILE_UNIT, m_PosX+m_Width-TILE_UNIT, m_PosY+m_Height, 0xffffffff);
	SetUVMap(0.875f, 0.625f, 1.0f, 0.75f);
	TexMap2DRect(m_PosX+m_Width-TILE_UNIT, m_PosY+m_Height-TILE_UNIT, m_PosX+m_Width, m_PosY+m_Height, 0xffffffff);
	CPopMenu *ptr = m_Child;
	int ty = m_PosY+TILE_UNIT/2;
	while(ptr){
		if(ptr->m_String=="-"){
			devSetTexture(0, NULL);
			Draw2DLine(m_PosX+TILE_HALF, ty+TILE_HALF-1,
				m_PosX+m_Width-TILE_HALF, ty+TILE_HALF-1,
				g_Skin->m_PopupMenuData.m_DisabledFontColor);
			Draw2DLine(m_PosX+TILE_HALF, ty+TILE_HALF,
				m_PosX+m_Width-TILE_HALF, ty+TILE_HALF,
				g_Skin->m_PopupMenuData.m_DisabledShadowColor);
		}else{
			D3DCOLOR fc, sdw = 0;
			if(ptr->m_Enabled){
				if(ptr==m_Pointed){
					devSetTexture(0, NULL);
					Grad2DRect(m_PosX+MENU_RECT_MARGIN, ty,
						m_PosX+m_Width-MENU_RECT_MARGIN, ty+TILE_UNIT,
						g_Skin->m_PopupMenuData.m_SelectedBaseColor);
					fc = g_Skin->m_PopupMenuData.m_SelectedFontColor;
				}else{
					fc = g_Skin->m_PopupMenuData.m_DefaultFontColor;
				}
			}else{
				fc = g_Skin->m_PopupMenuData.m_DisabledFontColor;
				sdw = g_Skin->m_PopupMenuData.m_DisabledShadowColor;
			}
			if(ptr->m_Child){
				g_Skin->SetInterfaceTexture();
				SetUVMap(0.5f, 0.875f, 0.625f, 1.0f);
				TexMap2DRect(m_PosX+m_Width-TILE_UNIT, ty,
					m_PosX+m_Width, ty+TILE_UNIT, fc);
			}
			if(sdw) g_StrTex->RenderLeft(
				m_PosX+TILE_UNIT/2+1, ty+FontY(TILE_UNIT)+1, sdw, 0, ptr->m_String.c_str());
			g_StrTex->RenderLeft(
				m_PosX+TILE_UNIT/2, ty+FontY(TILE_UNIT), fc, 0, ptr->m_String.c_str());
		}
		ty += TILE_UNIT;
		ptr = ptr->m_Brother;
	}
	if(m_Pointed && m_Pointed->IsExpand()) m_Pointed->Render();
}
