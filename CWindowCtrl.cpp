#include "stdafx.h"
#include "CWindowCtrl.h"
#include "CSkinPlugin.h"
#include "CConfigMode.h"

//	内部定数
const int WND_RESIZE_GRAB = 4;	//	窓リサイズつまみ幅

/*
 *	コンストラクタ
 */
CWindowCtrl::CWindowCtrl(){
	m_CloseButton = NULL;
}

/*
 *	デストラクタ
 */
CWindowCtrl::~CWindowCtrl(){
	DELETE_V(m_CloseButton);
}

/*
 *	初期化
 */
void CWindowCtrl::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	char *t,		//	テキスト
	CInterface *p,	//	親
	bool close		//	クローズ有効
){
	CInterface::Init(x, y, w, h, t, p);
	m_State = 0;
	m_MinWidth = m_MinHeight = m_MaxWidth = m_MaxHeight = 0;
	m_Color = 0xffffffff;
	if(close){
		m_CloseButton = new CMiniButton;
		m_CloseButton->Init(w-TILE_UNIT, 0,
			TILE_UNIT, TILE_UNIT, this, 0.25f, 0.75f, false);
	}
}

/*
 *	サイズ設定
 */
void CWindowCtrl::SetSize(
	int w, int h	//	サイズ
){
	m_Width = w;
	m_Height = h;
	if(m_CloseButton) m_CloseButton->SetPos(m_Width-TILE_UNIT, 0);
	if(m_Resizer) m_Resizer->WindowResized(m_Width, m_Height, this);
}

/*
 *	リサイズ設定
 */
void CWindowCtrl::SetResize(
	int minw, int minh,	//	最小サイズ
	int maxw, int maxh,	//	最大サイズ
	CWindowResizer *res	//	リサイザ
){
	m_MinWidth = minw; m_MinHeight = minh;
	m_MaxWidth = maxw; m_MaxHeight = maxh;
	m_Resizer = res;
}

/*
 *	カーソルがウィンドウ外なら自動的に不透明にする
 */
void CWindowCtrl::SetAutoTransparent(){
	POINT pos = g_Cursor.GetPos();
	SetColor(IsInside(pos.x, pos.y) ? 0xffffffff : 0x80ffffff);
}

/*
 *	指定座標がタイトルバー内か調べる
 */
bool CWindowCtrl::IsInsideTitleBar(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width && py<=y && y<py+TILE_UNIT;
}

/*
 *	指定座標が左リサイズつまみ内か調べる
 */
bool CWindowCtrl::IsInsideLeftGrab(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+WND_RESIZE_GRAB && py<=y && y<py+m_Height;
}

/*
 *	指定座標が上リサイズつまみ内か調べる
 */
bool CWindowCtrl::IsInsideTopGrab(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width && py<=y && y<py+WND_RESIZE_GRAB;
}

/*
 *	指定座標が右リサイズつまみ内か調べる
 */
bool CWindowCtrl::IsInsideRightGrab(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px+m_Width-WND_RESIZE_GRAB<=x && x<px+m_Width && py<=y && y<py+m_Height;
}

/*
 *	指定座標が下リサイズつまみ内か調べる
 */
bool CWindowCtrl::IsInsideBottomGrab(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width && py+m_Height-WND_RESIZE_GRAB<=y && y<py+m_Height;
}

/*
 *	ドラッグ開始
 */
void CWindowCtrl::BeginDrag(
	int state,		//	状態
	int ox, int oy	//	旧数値
){
	POINT pos = g_Cursor.GetPos();
	m_State = state;
	m_OldX = ox; m_OldY = oy;
	m_DragX = pos.x; m_DragY = pos.y;
	CInterface *ti = FindTabItem();
	if(ti) ti->GiveFocus();
}

/*
 *	入力チェック
 */
bool CWindowCtrl::ScanInput(){
	if(!m_Visible){
		if(m_CloseButton) m_CloseButton->SetPush(false);
		return ScanInputBrother();
	}
	if(m_CloseButton) m_CloseButton->Show(false);
	bool childret = CInterface::ScanInputChild();
	if(m_CloseButton) m_CloseButton->Show(true);
	if(childret) return true;
	if(ScanInputWindow()) return true;
	POINT pos = g_Cursor.GetPos();
	if(m_State){
		switch(GetButton(DIM_LEFT)){
		case S_HOLD:
			if(m_State&16){
				int oh = m_Height;
				m_Height = m_OldY-pos.y+m_DragY;
				ValueArea(&m_Height, m_MinHeight, m_MaxHeight);
				m_PosY += oh-m_Height;
			}
			if(m_State&32){
				m_Height = m_OldY+pos.y-m_DragY;
				ValueArea(&m_Height, m_MinHeight, m_MaxHeight);
			}
			if(m_State&64){
				int ow = m_Width;
				m_Width = m_OldX-pos.x+m_DragX;
				ValueArea(&m_Width, m_MinWidth, m_MaxWidth);
				m_PosX += ow-m_Width;
			}
			if(m_State&128){
				m_Width = m_OldX+pos.x-m_DragX;
				ValueArea(&m_Width, m_MinWidth, m_MaxWidth);
			}
			if(m_State&240){
				if(m_CloseButton) m_CloseButton->SetPos(m_Width-TILE_UNIT, 0);
				if(m_Resizer) m_Resizer->WindowResized(m_Width, m_Height, this);
			}
			switch(m_State){
			case 16: case 32: g_Cursor.SetResize(0); break;
			case 144: case 96: g_Cursor.SetResize(1); break;
			case 64: case 128: g_Cursor.SetResize(2); break;
			case 80: case 160: g_Cursor.SetResize(3); break;
			}
			g_Cursor.SetResize(-1);
			if(m_State==1){
				m_PosX = m_OldX+pos.x-m_DragX;
				m_PosY = m_OldY+pos.y-m_DragY;
			}
			return true;
		default:
			m_State = 0;
			break;
		}
	}else if(GetButton(DIM_LEFT)==S_PUSH){
		if(m_MaxHeight && IsInsideTopGrab(pos.x, pos.y)){
			if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)){
				g_Cursor.SetResize(3);
				BeginDrag(16|64, m_Width, m_Height);
				return true;
			}else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)){
				g_Cursor.SetResize(1);
				BeginDrag(16|128, m_Width, m_Height);
				return true;
			}else{
				g_Cursor.SetResize(0);
				BeginDrag(16, m_Width, m_Height);
				return true;
			}
		}else if(m_MaxHeight && IsInsideBottomGrab(pos.x, pos.y)){
			if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)){
				g_Cursor.SetResize(1);
				BeginDrag(32|64, m_Width, m_Height);
				return true;
			}else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)){
				g_Cursor.SetResize(3);
				BeginDrag(32|128, m_Width, m_Height);
				return true;
			}else{
				g_Cursor.SetResize(0);
				BeginDrag(32, m_Width, m_Height);
				return true;
			}
		}else if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)){
			g_Cursor.SetResize(2);
			BeginDrag(64, m_Width, m_Height);
			return true;
		}else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)){
			g_Cursor.SetResize(2);
			BeginDrag(128, m_Width, m_Height);
			return true;
		}else if(IsInsideTitleBar(pos.x, pos.y)){
			g_Cursor.SetResize(-1);
			if(m_CloseButton && m_CloseButton->ScanInput()) return true;
			BeginDrag(1, m_PosX, m_PosY);
			return true;
		}else{
CLICKFOCUS:
			if(IsInside(pos.x, pos.y)){
				g_Cursor.SetResize(-1);
				CInterface *ti = FindTabItem();
				if(ti) ti->GiveFocus();
				return true;
			}
		}
	}else if(GetButton(DIM_RIGHT)==S_PUSH){
		goto CLICKFOCUS;
	}else{
		if(m_MaxHeight && IsInsideTopGrab(pos.x, pos.y)){
			if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)) g_Cursor.SetResize(3);
			else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)) g_Cursor.SetResize(1);
			else g_Cursor.SetResize(0);
		}else if(m_MaxHeight && IsInsideBottomGrab(pos.x, pos.y)){
			if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)) g_Cursor.SetResize(1);
			else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)) g_Cursor.SetResize(3);
			else g_Cursor.SetResize(0);
		}else if(m_MaxWidth && IsInsideLeftGrab(pos.x, pos.y)){
			g_Cursor.SetResize(2);
		}else if(m_MaxWidth && IsInsideRightGrab(pos.x, pos.y)){
			g_Cursor.SetResize(2);
		}else if(IsInside(pos.x, pos.y)){
			g_Cursor.SetResize(-1);
		}
	}
	if(m_CloseButton && m_CloseButton->ScanInput()) return true;
	return ScanInputBrother();
}

/*
 *	レンダリング
 */
void CWindowCtrl::Render(){
	CInterface::RenderBrother();
	if(!m_Visible) return;
	int px, py;
	GetAbsPos(&px, &py);
	g_Skin->SetInterfaceTexture();
	if(g_ConfigMode->GetWindowShadow()){
		const int SO = TILE_QUAD;
		D3DCOLOR sc = (m_Color>>1)&0xff000000;
		SetUVMap(0.0f, 0.0f, 0.125f, 0.125f);
		TexMap2DRect(px+SO, py+SO, px+TILE_UNIT+SO, py+TILE_UNIT+SO, sc);
		SetUVMap(0.125f, 0.0f, 0.25f, 0.125f);
		TexMap2DRect(px+TILE_UNIT+SO, py+SO, px+m_Width-TILE_UNIT+SO, py+TILE_UNIT+SO, sc);
		SetUVMap(0.25f, 0.0f, 0.375f, 0.125f);
		TexMap2DRect(px+m_Width-TILE_UNIT+SO, py+SO, px+m_Width+SO, py+TILE_UNIT+SO, sc);
		SetUVMap(0.0f, 0.125f, 0.125f, 0.25f);
		TexMap2DRect(px+SO, py+TILE_UNIT+SO, px+TILE_UNIT+SO, py+m_Height-TILE_UNIT+SO, sc);
		SetUVMap(0.125f, 0.125f, 0.25f, 0.25f);
		TexMap2DRect(px+TILE_UNIT+SO, py+TILE_UNIT+SO, px+m_Width-TILE_UNIT+SO, py+m_Height-TILE_UNIT+SO, sc);
		SetUVMap(0.25f, 0.125f, 0.375f, 0.25f);
		TexMap2DRect(px+m_Width-TILE_UNIT+SO, py+TILE_UNIT+SO, px+m_Width+SO, py+m_Height-TILE_UNIT+SO, sc);
		SetUVMap(0.0f, 0.25f, 0.125f, 0.375f);
		TexMap2DRect(px+SO, py+m_Height-TILE_UNIT+SO, px+TILE_UNIT+SO, py+m_Height+SO, sc);
		SetUVMap(0.125f, 0.25f, 0.25f, 0.375f);
		TexMap2DRect(px+TILE_UNIT+SO, py+m_Height-TILE_UNIT+SO, px+m_Width-TILE_UNIT+SO, py+m_Height+SO, sc);
		SetUVMap(0.25f, 0.25f, 0.375f, 0.375f);
		TexMap2DRect(px+m_Width-TILE_UNIT+SO, py+m_Height-TILE_UNIT+SO, px+m_Width+SO, py+m_Height+SO, sc);
	}
	SetUVMap(0.0f, 0.0f, 0.125f, 0.125f);
	TexMap2DRect(px, py, px+TILE_UNIT, py+TILE_UNIT, m_Color);
	SetUVMap(0.125f, 0.0f, 0.25f, 0.125f);
	TexMap2DRect(px+TILE_UNIT, py, px+m_Width-TILE_UNIT, py+TILE_UNIT, m_Color);
	SetUVMap(0.25f, 0.0f, 0.375f, 0.125f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py, px+m_Width, py+TILE_UNIT, m_Color);
	SetUVMap(0.0f, 0.125f, 0.125f, 0.25f);
	TexMap2DRect(px, py+TILE_UNIT, px+TILE_UNIT, py+m_Height-TILE_UNIT, m_Color);
	SetUVMap(0.125f, 0.125f, 0.25f, 0.25f);
	TexMap2DRect(px+TILE_UNIT, py+TILE_UNIT, px+m_Width-TILE_UNIT, py+m_Height-TILE_UNIT, m_Color);
	SetUVMap(0.25f, 0.125f, 0.375f, 0.25f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py+TILE_UNIT, px+m_Width, py+m_Height-TILE_UNIT, m_Color);
	SetUVMap(0.0f, 0.25f, 0.125f, 0.375f);
	TexMap2DRect(px, py+m_Height-TILE_UNIT, px+TILE_UNIT, py+m_Height, m_Color);
	SetUVMap(0.125f, 0.25f, 0.25f, 0.375f);
	TexMap2DRect(px+TILE_UNIT, py+m_Height-TILE_UNIT, px+m_Width-TILE_UNIT, py+m_Height, m_Color);
	SetUVMap(0.25f, 0.25f, 0.375f, 0.375f);
	TexMap2DRect(px+m_Width-TILE_UNIT, py+m_Height-TILE_UNIT, px+m_Width, py+m_Height, m_Color);
	g_StrTex->RenderCenter(px+m_Width/2, py+FontY(TILE_UNIT),
		g_Skin->m_InterfaceData.m_TitleBarFontColor, 0, m_Text.c_str(), m_Width-TILE_UNIT);
	CInterface::RenderChild();
	RenderWindow();
}
