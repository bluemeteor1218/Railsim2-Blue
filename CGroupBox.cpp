#include "stdafx.h"
#include "CGroupBox.h"
#include "CSkinPlugin.h"

//	内部定数
const int GB_FONT_MARGIN = 4;	//	枠と文字の余白

/*
 *	入力チェック
 */
bool CGroupBox::ScanInput(){
	if(!m_Visible) return ScanInputBrother();
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CGroupBox::Render(){
	CInterface::RenderBrother();
	if(!m_Visible) return;
	int px, py;
	GetAbsPos(&px, &py);
	CStringDrawer *sd = g_StrTex->DrawString(m_Text.c_str(), 0);
	int sw = sd->GetWidth(), shift = sw ? GB_FONT_MARGIN*2 : 0;
	g_Skin->SetInterfaceTexture();
	SetUVMap(0.625f, 0.0f, 0.75f, 0.125f);
	TexMap2DRect(px, py, px+TILE_UNIT, py+TILE_UNIT, 0xffffffff);
	SetUVMap(0.75f, 0.0f, 0.875f, 0.125f);
	TexMap2DRect(px+TILE_UNIT+sw+shift, py, px+m_Width-TILE_UNIT, py+TILE_UNIT, 0xffffffff);
	SetUVMap(0.875f, 0.0f, 1.0f, 0.125f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py, px+m_Width, py+TILE_UNIT, 0xffffffff);
	SetUVMap(0.625f, 0.125f, 0.75f, 0.25f);
	TexMap2DRect(px, py+TILE_UNIT, px+TILE_UNIT, py+m_Height-TILE_UNIT, 0xffffffff);
	SetUVMap(0.875f, 0.125f, 1.0f, 0.25f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py+TILE_UNIT, px+m_Width, py+m_Height-TILE_UNIT, 0xffffffff);
	SetUVMap(0.625f, 0.25f, 0.75f, 0.375f);
	TexMap2DRect(px, py+m_Height-TILE_UNIT, px+TILE_UNIT, py+m_Height, 0xffffffff);
	SetUVMap(0.75f, 0.25f, 0.875f, 0.375f);
	TexMap2DRect(px+TILE_UNIT, py+m_Height-TILE_UNIT, px+m_Width-TILE_UNIT, py+m_Height, 0xffffffff);
	SetUVMap(0.875f, 0.25f, 1.0f, 0.375f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py+m_Height-TILE_UNIT, px+m_Width, py+m_Height, 0xffffffff);
	sd->RenderLeft(px+TILE_UNIT+GB_FONT_MARGIN, py+FontY(TILE_UNIT),
		g_Skin->m_InterfaceData.m_StaticFontColor);
	CInterface::RenderChild();
}
