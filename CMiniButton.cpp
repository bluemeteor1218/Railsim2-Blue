#include "stdafx.h"
#include "CMiniButton.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CMiniButton::Init(
	int x, int y,		//	座標
	int w, int h,		//	サイズ
	CInterface *p,		//	親
	float u, float v,	//	テクスチャ座標
	bool m				//	モード
){
	CInterface::Init(x, y, w, h, "", p);
	m_State = m_Repeat = 0;
	m_Pushed = false;
	m_TexU = u; m_TexV = v;
	m_Mode = m;
	LinkTab(false);
}

/*
 *	入力チェック
 */
bool CMiniButton::ScanInput(){
	if(!m_Visible) return ScanInputBrother();
	POINT pos = g_Cursor.GetPos();
	m_Pushed = false;
	if(IsInside(pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			m_State = 2;
			GiveFocus();
			return true;
		case S_HOLD:
			if(m_State){
				m_State = 2;
				m_Repeat++;
				return true;
			}
			break;
		default:
			if(m_State){
				m_Pushed = true;
				if(!m_Mode) g_Skin->MouseUp();
			}
			m_State = m_Repeat = 0;
			break;
		}
	}else if(m_State){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			m_State = 1;
			return true;
		default:
			m_State = m_Repeat = 0;
			break;
		}
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CMiniButton::Render(){
	CInterface::RenderBrother();
	int px, py, push = m_State==2;
	float tv = m_State==2 ? m_TexV+0.125f : m_TexV;
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	SetUVMap(m_TexU, tv, m_TexU+0.125f, tv+0.125f);
	TexMap2DRect(px, py, px+m_Width, py+m_Height, 0xffffffff);
	CInterface::RenderChild();
	if(IsFocus()) DrawFocusFrame();
}
