#include "stdafx.h"
#include "CEditCtrl.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CEditCtrl::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p,	//	親
	int smax		//	最大文字数
){
	CInterface::Init(x, y, w, h, t, p);
	m_State = 0;
	m_StrMax = smax;
	m_CompDelay = false;
	m_KeepImm = false;
	LinkTab(true);
}

/*
 *	フォーカス付与
 */
void CEditCtrl::GiveFocus(
	bool snd	//	音再生フラグ
){
	if(m_State) return;
	m_State = 1;
	m_EditBox.Create(0, 0, m_Width-EB_OFSX*2, m_StrMax, m_Text, m_KeepImm ? 0 : -1);
	CInterface::GiveFocus(snd);
}

/*
 *	入力終了
 */
void CEditCtrl::FinishInput(){
	if(m_State){
		m_EditBox.GetText(m_Text);
		m_EditBox.Release(m_KeepImm ? 0 : -1);
		m_State = 0;
		ms_Focus = NULL;
	}
}

/*
 *	リアルタイムにテキスト取得
 */
string CEditCtrl::GetRealtimeText(){
	if(m_State){
		string str;
		m_EditBox.GetText(str);
		return str;
	}else{
		return GetText();
	}
}

/*
 *	入力チェック
 */
bool CEditCtrl::ScanInput(){
	POINT pos = g_Cursor.GetPos();
	if(m_State) m_EditBox.ScanInput();
	if(IsInside(pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			if(m_State) return true;
			GiveFocus();
			return true;
		}
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CEditCtrl::Render(){
	CInterface::RenderBrother();
	int px, py;
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	if(m_Width<2*TILE_UNIT){
		int w1 = m_Width/2, w2 = m_Width-w1;
		SetUVMap(0.0f, 0.375f, 0.125f*w1/TILE_UNIT, 0.5f);
		TexMap2DRect(px, py, px+w1, py+m_Height, 0xffffffff);
		SetUVMap(0.375f-0.125*w2/TILE_UNIT, 0.375f, 0.375f, 0.5f);
		TexMap2DRect(px+w1, py, px+m_Width, py+m_Height, 0xffffffff);
	}else{
		SetUVMap(0.0f, 0.375f, 0.125f, 0.5f);
		TexMap2DRect(px, py, px+TILE_UNIT, py+m_Height, 0xffffffff);
		SetUVMap(0.125f, 0.375f, 0.25f, 0.5f);
		TexMap2DRect(px+TILE_UNIT, py, px+m_Width-TILE_UNIT, py+m_Height, 0xffffffff);
		SetUVMap(0.25f, 0.375f, 0.375f, 0.5f);
		TexMap2DRect(px+m_Width-TILE_UNIT, py, px+m_Width, py+m_Height, 0xffffffff);
	}
	if(m_State){
		if(ms_Focus!=this){
			m_EditBox.GetText(m_Text);
			m_EditBox.Release(m_KeepImm ? 0 : -1);
			m_State = 0;
			goto STABLE;
		}
		m_CompDelay = !!m_EditBox.IsComp();
		m_EditBox.SetPos(px+EB_OFSX, py+FontY(TILE_UNIT));
		m_EditBox.Render();
	}else{
STABLE:
		g_StrTex->RenderLeft(px+EB_OFSX, py+FontY(m_Height),
			g_Skin->m_EditCtrlData.m_DefaultFontColor, 0,
			m_Text.c_str(), m_Width-EB_OFSX*2, m_Height);
	}
	CInterface::RenderChild();
}
