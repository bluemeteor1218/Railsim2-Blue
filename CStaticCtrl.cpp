#include "stdafx.h"
#include "CStaticCtrl.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CStaticCtrl::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p,	//	親
	int horz,		//	水平位置
	int vert		//	垂直位置
){
	CInterface::Init(x, y, w, h, t, p);
	m_HorzPos = horz;
	m_VertPos = vert;
}

/*
 *	レンダリング
 */
void CStaticCtrl::Render(){
	CInterface::RenderBrother();
	if(!m_Visible) return;
	int px, py, ty;
	GetAbsPos(&px, &py);
	switch(m_VertPos){
	case 0: ty = py; break;
	case 1: ty = py+FontY(m_Height); break;
	case 2: ty = py+m_Height-FONT_HEIGHT; break;
	}
	switch(m_HorzPos){
	case 0:
		g_StrTex->RenderLeft(px, ty,
			g_Skin->m_InterfaceData.m_StaticFontColor, 0, m_Text.c_str());
		break;
	case 1:
		g_StrTex->RenderCenter(px+m_Width/2, ty,
			g_Skin->m_InterfaceData.m_StaticFontColor, 0, m_Text.c_str());
		break;
	case 2:
		g_StrTex->RenderRight(px+m_Width, ty,
			g_Skin->m_InterfaceData.m_StaticFontColor, 0, m_Text.c_str());
		break;
	}
	CInterface::RenderChild();
}
