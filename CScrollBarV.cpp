#include "stdafx.h"
#include "CScrollBarV.h"
#include "CSkinPlugin.h"

/*
 *	初期化
 */
void CScrollBarV::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	CInterface *p	//	親
){
	CInterface::Init(x, y, w, h, "", p);
	m_State = m_Repeat = m_Scroll = 0;
	m_Page = m_Range = 1;
	LinkTab(false);
	m_UpButton.Init(0, 0, m_Width, TILE_UNIT, this, 0.375f, 0.5f, true);
	m_DownButton.Init(0, m_Height-TILE_UNIT, m_Width, TILE_UNIT, this, 0.375f, 0.75f, true);
}

/*
 *	サイズ変更
 */
void CScrollBarV::SetSize(
	int w, int h	//	新規サイズ
){
	m_Width = w; m_Height = h;
	m_DownButton.SetPos(0, m_Height-TILE_UNIT);
}

/*
 *	スクロール位置設定
 */
void CScrollBarV::SetScroll(
	int s	//	スクロール位置
){
	m_Scroll = s;
	if(m_Scroll+m_Page>m_Range) m_Scroll = m_Range-m_Page;
	if(m_Scroll<0) m_Scroll = 0;
}

/*
 *	スライダ幅設定
 */
void CScrollBarV::SetPage(
	int p	//	スライダ幅
){
	m_Page = p;
	if(m_Scroll+m_Page>m_Range) m_Scroll = m_Range-m_Page;
	if(m_Scroll<0) m_Scroll = 0;
}

/*
 *	全範囲設定
 */
void CScrollBarV::SetRange(
	int r	//	全範囲
){
	m_Range = r;
	if(m_Scroll+m_Page>m_Range) m_Scroll = m_Range-m_Page;
	if(m_Scroll<0) m_Scroll = 0;
}

/*
 *	スライダ位置情報の計算
 */
void CScrollBarV::CalcSlider(
	int *by,	//	スライダ位置 (画面座標系)
	int *bh		//	スライダ幅
){
	int sr = m_Height-2*TILE_UNIT;
	if(m_Page>=m_Range){
		*bh = sr;
		*by = 0;
	}else{
		*bh = sr*m_Page/m_Range;
		if(*bh<SLIDER_MIN) *bh = SLIDER_MIN;
		*by = (sr-*bh)*m_Scroll/(m_Range-m_Page);
	}
}

/*
 *	指定座標がスクロール部内か調べる
 */
bool CScrollBarV::IsInsideScroll(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width && py+TILE_UNIT<=y && y<py+m_Height-TILE_UNIT;
}

/*
 *	指定座標がスライダ内か調べる
 */
bool CScrollBarV::IsInsideSlider(
	int x, int y	//	座標
){
	int px, py, by, bh;
	GetAbsPos(&px, &py);
	CalcSlider(&by, &bh);
	by += py+TILE_UNIT;
	return px<=x && x<px+m_Width && by<=y && y<by+bh;
}

/*
 *	入力チェック
 */
bool CScrollBarV::ScanInput(){
	POINT pos = g_Cursor.GetPos();
	if(m_UpButton.GetRepeat()==1 || m_UpButton.IsRepeat()) SetScroll(m_Scroll-1);
	if(m_DownButton.GetRepeat()==1 || m_DownButton.IsRepeat()) SetScroll(m_Scroll+1);
	int px, py, by, bh;
	GetAbsPos(&px, &py);
	CalcSlider(&by, &bh);
	by += py+TILE_UNIT;
	if(IsInsideSlider(pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			m_State = 1;
			m_Repeat = 0;
			m_OldS = m_Scroll;
			m_DragS = (pos.y-py-TILE_UNIT)*m_Range/(m_Height-2*TILE_UNIT);
			GiveFocus();
			return true;
		case S_HOLD:
			if(m_State){
				if(abs(px+m_Width/2-pos.x)>SLIDER_DRAG_DIST) SetScroll(m_OldS);
				else SetScroll(m_OldS-m_DragS
					+(pos.y-py-TILE_UNIT)*m_Range/(m_Height-2*TILE_UNIT));
				return true;
			}
			break;
		default:
			m_State = m_Repeat = 0;
			break;
		}
	}else if(m_State){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			if(abs(px+m_Width/2-pos.x)>SLIDER_DRAG_DIST) SetScroll(m_OldS);
			else SetScroll(m_OldS-m_DragS
				+(pos.y-py-TILE_UNIT)*m_Range/(m_Height-2*TILE_UNIT));
			return true;
		default:
			m_State = 0;
			break;
		}
	}else if(IsInsideScroll(pos.x, pos.y)){
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			if(pos.y<by){
				SetScroll(m_Scroll-m_Page);
				m_Repeat = -1;
			}else{
				SetScroll(m_Scroll+m_Page);
				m_Repeat = 1;
			}
			GiveFocus();
			return true;
		case S_HOLD:
			if(m_Repeat<0 && pos.y<by){
				if(--m_Repeat<-REPEAT_FRAME) SetScroll(m_Scroll-m_Page);
				return true;
			}else if(m_Repeat>0 && pos.y>by){
				if(++m_Repeat>REPEAT_FRAME) SetScroll(m_Scroll+m_Page);
				return true;
			}
			break;
		default:
			m_Repeat = 0;
			break;
		}
	}else{
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			if(m_Repeat) return true;
			break;
		}
		m_Repeat = 0;
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CScrollBarV::Render(){
	CInterface::RenderBrother();
	int px, py, push = m_State==2;
	float tv = m_State==2 ? 0.625f : 0.5f;
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	SetUVMap(0.375f, 0.375f, 0.5f, 0.5f);
	TexMap2DRect(px, py+TILE_UNIT, px+m_Width, py+TILE_UNIT+5, 0xffffffff);
	int by, bh;
	CalcSlider(&by, &bh);
	if(by>0){
		float tu = m_Repeat<0 ? 0.5f : 0.375f;
		SetUVMap(tu, 0.376f, tu+0.125f, 0.5f);
		TexMap2DRect(px, py+TILE_UNIT, px+m_Width, py+TILE_UNIT+by, 0xffffffff);
	}
	if(m_Height-2*TILE_UNIT-by-bh>0){
		float tu = m_Repeat>0 ? 0.5f : 0.375f;
		SetUVMap(tu, 0.376f, tu+0.125f, 0.5f);
		TexMap2DRect(px, py+TILE_UNIT+by+bh, px+m_Width, py+m_Height-TILE_UNIT, 0xffffffff);
	}
	by += py+TILE_UNIT;
	if(bh<2*TILE_UNIT){
		int h1 = bh/2, h2 = bh-h1;
		SetUVMap(0.5f, 0.5f, 0.625f, 0.5f+0.125f*h1/TILE_UNIT);
		TexMap2DRect(px, by, px+m_Width, by+h1, 0xffffffff);
		SetUVMap(0.5f, 0.875f-0.125*h2/TILE_UNIT, 0.625f, 0.875f);
		TexMap2DRect(px, by+h1, px+m_Width, by+bh, 0xffffffff);
	}else{
		SetUVMap(0.5f, 0.5f, 0.625f, 0.625f);
		TexMap2DRect(px, by, px+m_Width, by+TILE_UNIT, 0xffffffff);
		SetUVMap(0.5f, 0.625f, 0.625f, 0.75f);
		TexMap2DRect(px, by+TILE_UNIT, px+m_Width, by+bh-TILE_UNIT, 0xffffffff);
		SetUVMap(0.5f, 0.75f, 0.625f, 0.875f);
		TexMap2DRect(px, by+bh-TILE_UNIT, px+m_Width, by+bh, 0xffffffff);
	}
	CInterface::RenderChild();
	if(IsFocus()) DrawFocusFrame();
}
