#include "stdafx.h"
#include "CInterface.h"
#include "CSkinPlugin.h"

//	static メンバ
CInterface *CInterface::ms_Focus = NULL;
CInterface *CInterface::ms_Drop = NULL;
CInterface *CInterface::ms_TabHead = NULL;

/*
 *	[static]
 *	共通キー入力処理
 */
void CInterface::ProcessKey(){
	static int rep;
	int s;
	if(ms_Focus && !CheckAlt() && (s = GetKey(DIK_TAB))>=S_PUSH){
		if(PickRepeat(&rep, s)){
			CInterface *tabtmp = ms_Focus;
			while(true){
				if(CheckShift()) tabtmp = tabtmp->m_TabPrev;
				else tabtmp = tabtmp->m_TabNext;
				if(!tabtmp || tabtmp==ms_Focus
					|| tabtmp->IsVisible() && tabtmp->IsEnabled()) break;
			}
			if(tabtmp) tabtmp->GiveFocus();
		}
	}
	ms_Drop = NULL;
}

/*
 *	コンストラクタ
 */
CInterface::CInterface(){
	m_Parent = m_Brother = m_Child = m_Owner = NULL;
	m_TabPrev = m_TabNext = NULL;
}

/*
 *	デストラクタ
 */
CInterface::~CInterface(){
	if(ms_Focus==this) ms_Focus = NULL;
}

/*
 *	初期化
 */
void CInterface::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p	//	親
){
	m_PosX = x; m_PosY = y;
	m_Width = w; m_Height = h;
	m_Text = t;
	m_Enabled = true;
	m_Visible = true;
	if(p) p->SetChild(this);
}

/*
 *	子の設定
 */
void CInterface::SetChild(
	CInterface *child	//	子
){
	if(child->m_Parent){
		if(child->m_Parent==this) return;
		child->m_Parent->RemoveChild(child);
	}
	child->m_Parent = this;
	if(m_Child) m_Child->SetBrother(child);
	else m_Child = child;
}

/*
 *	兄弟の設定
 */
void CInterface::SetBrother(
	CInterface *bros	//	兄弟
){
	if(m_Brother) m_Brother->SetBrother(bros);
	else m_Brother = bros;
}

/*
 *	子を削除
 */
void CInterface::RemoveChild(
	CInterface *child	//	子
){
	CInterface **adr = &m_Child;
	while(*adr){
		if(*adr==child){
			*adr = child->m_Brother;
			child->m_Parent = child->m_Brother = NULL;
			return;
		}
		adr = &(*adr)->m_Brother;
	}
}

/*
 *	タブリンク確立
 */
void CInterface::LinkTab(
	bool tab	//	タブを使う
){
	CInterface *ptr = m_Parent;
	while(ptr){
		if(ptr==ptr->m_Owner){
			m_Owner = ptr;
			return;
		}
		ptr = ptr->m_Parent;
	}
	if(!tab) return;
	m_Owner = this;
	if(ms_TabHead){
		m_TabPrev = ms_TabHead;
		m_TabNext = m_TabPrev->m_TabNext;
		ms_TabHead = m_TabPrev->m_TabNext = m_TabNext->m_TabPrev = this;
	}else{
		ms_TabHead = m_TabPrev = m_TabNext = this;
	}
}

/*
 *	タブを受け取るアイテムを探す
 */
CInterface *CInterface::FindTabItem(){
	if(m_Owner==this) return this;
	CInterface *ptr = m_Child;
	while(ptr){
		CInterface *tmp = ptr->FindTabItem();
		if(tmp) return tmp;
		ptr = ptr->m_Brother;
	}
	return NULL;
}

/*
 *	絶対座標の取得
 */
void CInterface::GetAbsPos(
	int *x, int *y	//	座標
){
	*x = m_PosX; *y = m_PosY;
	CInterface *ptr = m_Parent;
	while(ptr){
		*x += ptr->m_PosX; *y += ptr->m_PosY;
		ptr = ptr->m_Parent;
	}
}

/*
 *	表示状態の取得
 */
bool CInterface::IsVisible(){
	CInterface *ptr = this;
	while(ptr){
		if(!ptr->m_Visible) return false;
		ptr = ptr->m_Parent;
	}
	return true;
}

/*
 *	指定座標が範囲内か調べる
 */
bool CInterface::IsInside(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width && py<=y && y<py+m_Height;
}

/*
 *	フォーカス枠の描画
 */
void CInterface::DrawFocusFrame(){
	int px, py;
	GetAbsPos(&px, &py);
	devSetTexture(0, NULL);
	Draw2DRect(px, py, px+m_Width, py+m_Height,
		g_Skin->m_InterfaceData.m_FocusFrameColor);
}

void CInterface::GiveFocus(
	bool snd	//	音再生フラグ
){
	ms_Focus = m_Owner;
	if(snd) g_Skin->MouseDown();
	CInterface *lev = m_Parent, *cur = this;
	while(lev){
		CInterface *ptr = lev->m_Child, **adr = &lev->m_Child;
		bool top = true;
		while(ptr){
			if(ptr==cur){
				*adr = ptr->m_Brother;
				ptr->m_Brother = lev->m_Child;
				lev->m_Child = ptr;
				break;
			}
			top = false;
			ptr = *(adr = &ptr->m_Brother);
		}
		cur = lev;
		lev = lev->m_Parent;
	}
}

/*
 *	入力チェック
 */
bool CInterface::ScanInput(){
	if(m_Visible && m_Child && m_Child->ScanInput()) return true;
	if(m_Brother && m_Brother->ScanInput()) return true;
	return false;
}

/*
 *	レンダリング
 */
void CInterface::Render(){
	RenderBrother();
	if(m_Visible) RenderChild();
}
