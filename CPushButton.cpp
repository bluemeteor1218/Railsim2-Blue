#include "stdafx.h"
#include "CPushButton.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CPushButton::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p	//	親
){
	CInterface::Init(x, y, w, h, t, p);
	m_State = 0;
	m_Sound = true;
	m_Pushed = false;
	m_Pushable = true;
	LinkTab(true);
}

/*
 *	入力チェック
 */
bool CPushButton::ScanInput(){
	m_Pushed = false;
	if(!m_Pushable) return CInterface::ScanInput();
	if(!m_Enabled){
		m_State = 0;
		return CInterface::ScanInput();
	}
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
				m_Pushed = true;
				if(m_Sound) g_Skin->MouseUp();
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
void CPushButton::Render(){
	CInterface::RenderBrother();
	int px, py, push = m_State==2;
	float tv = m_State==2 ? 0.625f : 0.5f;
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	if(m_Width<2*TILE_UNIT){
		int w1 = m_Width/2, w2 = m_Width-w1;
		SetUVMap(0.0f, tv, 0.125f*w1/TILE_UNIT, tv+0.125f);
		TexMap2DRect(px, py, px+w1, py+m_Height, 0xffffffff);
		SetUVMap(0.375f-0.125*w2/TILE_UNIT, tv, 0.375f, tv+0.125f);
		TexMap2DRect(px+w1, py, px+m_Width, py+m_Height, 0xffffffff);
	}else{
		SetUVMap(0.0f, tv, 0.125f, tv+0.125f);
		TexMap2DRect(px, py, px+TILE_UNIT, py+m_Height, 0xffffffff);
		SetUVMap(0.125f, tv, 0.25f, tv+0.125f);
		TexMap2DRect(px+TILE_UNIT, py, px+m_Width-TILE_UNIT, py+m_Height, 0xffffffff);
		SetUVMap(0.25f, tv, 0.375f, tv+0.125f);
		TexMap2DRect(px+m_Width-TILE_UNIT, py, px+m_Width, py+m_Height, 0xffffffff);
	}
	D3DCOLOR fc = g_Skin->m_InterfaceData.m_ButtonFontColor, sdw = 0;
	if(!m_Enabled){
		fc = g_Skin->m_PopupMenuData.m_DisabledFontColor;
		sdw = g_Skin->m_PopupMenuData.m_DisabledShadowColor;
	}
	if(sdw) g_StrTex->RenderCenter(px+m_Width/2+push+1, py+FontY(m_Height)+push+1,
		sdw, 0, m_Text.c_str(), m_Width, m_Height);
	g_StrTex->RenderCenter(px+m_Width/2+push, py+FontY(m_Height)+push,
		fc, 0, m_Text.c_str(), m_Width, m_Height);
	CInterface::RenderChild();
	if(IsFocus()) DrawFocusFrame();
}
