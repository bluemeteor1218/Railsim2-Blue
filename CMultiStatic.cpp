#include "stdafx.h"
#include "CMultiStatic.h"
#include "CSkinPlugin.h"

//	内部定数
const int MULTISTATIC_MARGIN = 2;	//	余白

/*
 *	初期化
 */
void CMultiStatic::Init(
	int x, int y,	//	座標
	int w, int h,	//	サイズ
	CInterface *p,	//	親
	int lh			//	行高さ
){
	CInterface::Init(x, y, w, h, "", p);
	LinkTab(true);
	m_ScrollV.Init(m_Width-TILE_UNIT, 0, TILE_UNIT, m_Height, this);
	m_LineHeight = lh;
}

/*
 *	サイズ変更
 */
void CMultiStatic::SetSize(
	int w, int h	//	新規サイズ
){
	m_Width = w; m_Height = h;
	m_ScrollV.SetPos(m_Width-TILE_UNIT, 0);
	m_ScrollV.SetSize(TILE_UNIT, m_Height);
	SetText(NULL);
	SetScroll();
}

/*
 *	テキスト設定
 */
void CMultiStatic::SetText(
	char *t	//	テクスト
){
	if(t) m_Text = t;
	m_LineText.clear();
	int tw = m_Width-TILE_UNIT-MULTISTATIC_MARGIN*2;
	char *str = (char *)m_Text.c_str();
	while(*str){
		if(*str=='\n'){
			m_LineText.push_back("");
			str++;
		}else if(*str=='\r'){
			str++;
		}else{
			char *tmp = CharNext(str), *tmp2 = CharNext(tmp);
			for(; *tmp && *tmp!='\n'; tmp2 = CharNext(tmp2)){
				string test(str, tmp2);
				if(g_StrTex->DrawString(test.c_str(), 0)->GetWidth()>tw) break;
				tmp = tmp2;
			}
			m_LineText.push_back(string(str, tmp));
			str = tmp;
			if(*str=='\n') str++;
		}
	}
}

/*
 *	スクロールバーの更新
 */
void CMultiStatic::SetScroll(){
	m_Lines = (m_Height-MULTISTATIC_MARGIN*2)/m_LineHeight;
	m_ScrollV.SetRange(m_LineText.size() ? m_LineText.size() : 1);
	m_ScrollV.SetPage(m_Lines);
}

/*
 *	指定座標がリスト内か調べる
 */
bool CMultiStatic::IsInsideList(
	int x, int y	//	座標
){
	int px, py;
	GetAbsPos(&px, &py);
	return px<=x && x<px+m_Width-TILE_UNIT && py+TILE_UNIT<=y && y<py+m_Height;
}

/*
 *	入力チェック
 */
bool CMultiStatic::ScanInput(){
	POINT pos = g_Cursor.GetPos();
	int px, py;
	GetAbsPos(&px, &py);
	SetScroll();
	if(IsInsideList(pos.x, pos.y) && GetButton(DIM_LEFT)==S_PUSH){
		GiveFocus();
		return true;
	}
	if(IsFocus()){
		static int rep;
		int s;
		if((s = GetKey(DIK_UP))>=S_PUSH){
			if(PickRepeat(&rep, s)) m_ScrollV.SetScroll(m_ScrollV.GetScroll()-1);
		}else if((s = GetKey(DIK_DOWN))>=S_PUSH){
			if(PickRepeat(&rep, s)) m_ScrollV.SetScroll(m_ScrollV.GetScroll()+1);
		}else if(GetWheel()<0){
			m_ScrollV.SetScroll(m_ScrollV.GetScroll()+3);
		}else if(GetWheel()>0){
			m_ScrollV.SetScroll(m_ScrollV.GetScroll()-3);
		}
	}
	return CInterface::ScanInput();
}

/*
 *	レンダリング
 */
void CMultiStatic::Render(){
	CInterface::RenderBrother();
	CInterface::RenderChild();
	int i, px, py;
	GetAbsPos(&px, &py);
	devSetTexture(0, NULL);
	px += MULTISTATIC_MARGIN;
	int ofs = m_ScrollV.GetScroll(), ty = py+MULTISTATIC_MARGIN, by = py+m_Height;
	list<string>::iterator it = m_LineText.begin();
	for(i = 0; it!=m_LineText.end(); it++, i++){
		if(i<ofs) continue;
		int th = by-ty;
		if(th<=0) break;
		if(th>m_LineHeight) th = m_LineHeight;
		g_StrTex->RenderLeft(px, ty, g_Skin->m_InterfaceData.m_StaticFontColor,
			0, it->c_str(), m_Width-TILE_UNIT, th);
		ty += m_LineHeight;
	}
	if(IsFocus()) DrawFocusFrame();
}
