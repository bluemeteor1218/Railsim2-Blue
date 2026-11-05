#include "stdafx.h"
#include "CCheckBox.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CCheckBox::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p	//	親
){
	CInterface::Init(x, y, w, h, t, p);
	m_State = 0;
	m_Check = 0;
	LinkTab(true);
}

/*
 *	入力チェック
 */
bool CCheckBox::ScanInput(){
	if(!m_Enabled){
		m_State = 0;
		return CInterface::ScanInput();
	}
	if(!m_Visible) return ScanInputBrother();
	POINT pos = g_Cursor.GetPos();
	int key = GetKey(DIK_SPACE)|GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER);
	bool in = IsInside(pos.x, pos.y), focus = IsFocus();
	if(in || focus && key){
		int stt = 0;
		if(in) stt |= GetButton(DIM_LEFT);
		if(focus) stt |= key;
		switch(stt){
		case S_PUSH:
			m_State = 2;
			GiveFocus();
			return true;
		case S_HOLD:
			if(m_State){
				m_State = 2;
				return true;
			}
			break;
		default:
			if(m_State){
				g_Skin->MouseUp();
				m_Check = !m_Check;
			}
			m_State = 0;
			break;
		}
	}else if(m_State){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			m_State = 1;
			return true;
		default:
			m_State = 0;
			break;
		}
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CCheckBox::Render(){
	CInterface::RenderBrother();
	if(!m_Visible) return;
	int px, py, push = m_State==2;
	float tv = m_State==2 ? 0.125f : (m_Check ? 0.25f : 0.0f);
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	SetUVMap(0.375f, tv, 0.5f, tv+0.125f);
	TexMap2DRect(px, py, px+TILE_UNIT, py+m_Height, 0xffffffff);
	int lim = m_Width-TILE_UNIT*3/2-push;
	D3DCOLOR fc = g_Skin->m_InterfaceData.m_StaticFontColor, sdw = 0;
	if(!m_Enabled){
		fc = g_Skin->m_PopupMenuData.m_DisabledFontColor;
		sdw = g_Skin->m_PopupMenuData.m_DisabledShadowColor;
	}
	if(sdw) g_StrTex->RenderLeft(px+TILE_UNIT*3/2+push, py+FontY(m_Height)+push,
		sdw, 0, m_Text.c_str(), lim>0 ? lim : 1);
	g_StrTex->RenderLeft(px+TILE_UNIT*3/2+push, py+FontY(m_Height)+push,
		fc, 0, m_Text.c_str(), lim>0 ? lim : 1);
	CInterface::RenderChild();
	if(m_Text.size() && IsFocus()){
		devSetTexture(0, NULL);
		Draw2DRect(px+TILE_UNIT*5/4, py, px+m_Width, py+m_Height,
			g_Skin->m_InterfaceData.m_FocusFrameColor);
	}
}
