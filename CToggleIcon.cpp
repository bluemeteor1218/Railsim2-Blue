#include "stdafx.h"
#include "CToggleIcon.h"
#include "CSkinPlugin.h"
#include "CNeutralMode.h"

//	内部定数
const float TI_SLIDE_RATIO = 0.2f;		//	スライド速度
const float FRAME_TEX_UNIT = 0.125f;	//	フレームマッピング単位
const float ICON_TEX_UNIT = 0.25f;		//	アイコンマッピング単位

//	static メンバ
CToggleIcon *CToggleIcon::ms_PointIcon;

/*
 *	[static]
 *	ポップアップテキストの描画
 */
void CToggleIcon::RenderPopupText(){
	if(!ms_PointIcon) return;
	g_StrTex->RenderRight(
		g_DispWidth-TILE_UNIT*2-TILE_QUAD, g_DispHeight-TILE_UNIT*2,
		0xffffffff, 0xff000000, ms_PointIcon->m_Text.c_str());
	g_StrTex->RenderRight(
		g_DispWidth-TILE_UNIT*2-TILE_QUAD, g_DispHeight-TILE_UNIT,
		0xffffffff, 0xff000000, ms_PointIcon->m_Expression.c_str());
	ms_PointIcon = NULL;
}

/*
 *	初期化
 */
void CToggleIcon::Init(
	int x, int y,		//	座標
	int w, int h,		//	サイズ
	char *t,			//	テキスト
	char *expr,			//	説明文
	CInterface *p,		//	親
	CToggleIcon *l,		//	前リンク
	CGameMode *mode,	//	対応モード
	DWORD key,			//	ホットキー
	float u1, float v1,	//	開放時テクスチャ座標
	float u2, float v2,	//	押下時テクスチャ座標
	float u3, float v3,	//	アイコンテクスチャ座標
	int mid				//	モード ID
){
	CInterface::Init(x, y, w, h, t, p);
	m_State = 0;
	m_Check = 0;
	m_ModeID = mid;
	m_TexPullU = u1, m_TexPullV = v1;
	m_TexPushU = u2, m_TexPushV = v2;
	m_TexIconU = u3, m_TexIconV = v3;
	m_Expression = expr;
	m_HotKey = key;
	m_SyncMode = mode;
	if(l){
		m_Check = 0;
		m_Prev = l;
		m_Next = m_Prev->m_Next;
		m_Prev->m_Next = m_Next->m_Prev = this;
	}else{
		m_Check = 1;
		m_Prev = m_Next = this;
	}
}

/*
 *	チェッククリア
 */
void CToggleIcon::ClearGroupCheck(){
	CToggleIcon *ptr = this;
	do{
		ptr->m_Check = 0;
		ptr = ptr->m_Next;
	} while(ptr!=this);
}

/*
 *	チェック設定
 */
void CToggleIcon::SetCheck(
	bool inv	//	反転フラグ
){
	if(inv && m_Check){
		m_Check = 0;
	}else if(!m_Check){
		ClearGroupCheck();
		m_Check = 1;
	}
	if(m_SyncMode && !g_RSPV){
		if(m_Check) m_SyncMode->Enter();
		else g_NeutralMode->Enter();
	}
}

/*
 *	スライド座標設定
 */
void CToggleIcon::SetSlidePos(
	int x, int y,	//	座標
	bool fix		//	固定フラグ
){
	if(fix){
		m_SlideY = y;
		SetPos(x, y);
	}else{
		m_SlideY = y*TI_SLIDE_RATIO+m_SlideY*(1.0f-TI_SLIDE_RATIO);
		SetPos(x, Round(m_SlideY));
	}
}

/*
 *	入力チェック
 */
bool CToggleIcon::ScanInput(
	bool enkey,		//	ホットキー有効
	bool enmouse	//	マウス有効
){
	POINT pos = g_Cursor.GetPos();
	if(enmouse && IsInside(pos.x, pos.y)){
		ms_PointIcon = this;
		switch(GetButton(DIM_LEFT)){
		case S_PUSH:
			m_State = 2;
			g_Skin->MouseDown();
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
				SetCheck(true);
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
	if(enkey && GetKey(m_HotKey)==S_PUSH
		&& !CheckShift() && !CheckCtrl() && !CheckAlt()){
		g_Skin->MouseUp();
		SetCheck(true);
		return true;
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CToggleIcon::Render(
	float altu, float altv	//	代替テクスチャ座標
){
	CInterface::RenderBrother();
	int px, py, push = m_State==2;
	GetAbsPos(&px, &py);
	g_Skin->SetFrameTexture();
	if(altu>=0.0f) SetUVMap(altu, altv, altu+FRAME_TEX_UNIT, altv+FRAME_TEX_UNIT);
	else if(m_Check) SetUVMap(m_TexPushU, m_TexPushV,
		m_TexPushU+FRAME_TEX_UNIT, m_TexPushV+FRAME_TEX_UNIT);
	else SetUVMap(m_TexPullU, m_TexPullV,
		m_TexPullU+FRAME_TEX_UNIT, m_TexPullV+FRAME_TEX_UNIT);
	TexMap2DRect(px, py, px+m_Width, py+m_Height, 0xffffffff);
	if(m_State==2) return;
	g_Skin->SetIconTexture(m_ModeID);
	SetUVMap(m_TexIconU, m_TexIconV,
		m_TexIconU+ICON_TEX_UNIT, m_TexIconV+ICON_TEX_UNIT);
	if(m_Check) px++, py++;
	TexMap2DRect(px, py, px+m_Width, py+m_Height, 0xffffffff);
}
