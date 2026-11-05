#include "stdafx.h"

//	内部グローバル
int g_StrTexRadius;		//	テクスチャサイズ
int g_StrTexColWidth;	//	列幅
int g_StrTexCols;		//	列数

/*
 *	初期化
 */
void CStringDrawer::Init(
	int x, int y,	//	位置
	CTexture *tex	//	テクスチャ
){
	m_PosX = x; m_PosY = y;
	m_Cols = 0;
	m_Date = 0;
	m_Texture = tex;
	m_Key = NULL;
}

/*
 *	リセット
 */
void CStringDrawer::Reset(
	CStringDrawer *key	//	キー
){
	m_String = "";
	m_Cols = 0;
	m_Key = key;
	m_Date = m_Key ? GetFrameCount() : 0;
}

/*
 *	有効にする
 */
void CStringDrawer::Enable(
	const char *str,	//	文字列
	int w, int h,		//	サイズ
	int cols,			//	列数
	HFONT font,			//	フォント
	D3DCOLOR sdw		//	陰影色
){
	CStringDrawer *tmp = m_Key ? m_Key : this;
	int n = tmp->m_Cols, i, j = 0;
	for(i = 0; i<=n; i++) tmp[i].Reset(NULL);
	m_String = str;
	m_Width = w; m_Height = h;
	m_Cols = cols;
	m_Date = GetFrameCount();
	m_ShadowColor = sdw;
	m_Key = NULL;
	for(i = 1; i<=m_Cols; i++){
		int tc = (this+i)->m_Cols;
		if(tc && j<i+tc) j = i+tc;
		(this+i)->Reset(this);
	}
	for(; i<=j; i++) (this+i)->Reset(NULL);
	if(w && h) m_Texture->DrawInText(m_PosX, m_PosY,
		m_String.c_str(), font, 0xffffffff, m_ShadowColor, m_Width, m_Height);
	if(m_ShadowColor){ m_Width++; m_Height++; }
	if(m_Width>g_StrTexRadius) m_Width = g_StrTexRadius;
}

/*
 *	文字列照合
 */
bool CStringDrawer::Check(
	const char *str,	//	文字列
	D3DCOLOR sdw		//	陰影色
){
	if(!m_Key && m_Date && m_ShadowColor==sdw && m_String==str){
		int i;
		for(i = 0; i<=m_Cols; i++) (this+i)->m_Date = GetFrameCount();
		return true;
	}
	return false;
}

/*
 *	周辺で最新のものを返す
 */
DWORD CStringDrawer::FindNewest(
	int cw	//	列幅
){
	int i, date = m_Date;
	for(i = 1; i<=cw; i++){
		int t = (this+i)->m_Date;
		if(date<t) date = t;
	}
	return date;
}

/*
 *	左揃えで描画
 */
void CStringDrawer::RenderLeft(
	int x, int y,	//	左上座標
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)m_PosX/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+mw)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect(x, y, x+mw, y+mh, c);
}

/*
 *	中央揃えで描画
 */
void CStringDrawer::RenderCenter(
	int x, int y,	//	上辺中央座標
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	int tw = mw/2;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)(m_PosX+(m_Width-mw)/2)/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+(m_Width+mw)/2)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect(x-tw, y, x-tw+mw, y+mh, c);
}

/*
 *	右揃えで描画
 */
void CStringDrawer::RenderRight(
	int x, int y,	//	右上座標
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)(m_PosX+m_Width-mw)/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+m_Width)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect(x-mw, y, x, y+mh, c);
}

/*
 *	左揃えで描画 (縦)
 */
void CStringDrawer::RenderLeftV(
	int x, int y,	//	左上座標 (回転前)
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限 (回転前)
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)m_PosX/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+mw)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect90(x-mh, y, x, y+mw, c);
}

/*
 *	中央揃えで描画 (縦)
 */
void CStringDrawer::RenderCenterV(
	int x, int y,	//	上辺中央座標 (回転前)
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限 (回転前)
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	int tw = mw/2;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)(m_PosX+(m_Width-mw)/2)/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+(m_Width+mw)/2)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect90(x-mh, y-tw, x, y-tw+mw, c);
}

/*
 *	右揃えで描画 (縦)
 */
void CStringDrawer::RenderRightV(
	int x, int y,	//	右上座標 (回転前)
	D3DCOLOR c,		//	色
	int mw, int mh	//	サイズ制限 (回転前)
){
	if(mw<0 || mw>m_Width) mw = m_Width;
	if(mh<0 || mh>m_Height) mh = m_Height;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)(m_PosX+m_Width-mw)/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+m_Width)/g_StrTexRadius,
		(float)(m_PosY+mh)/g_StrTexRadius);
	TexMap2DRect90(x-mh, y-mw, x, y, c);
}

/*
 *	左揃えで描画 (3D)
 */
void CStringDrawer::RenderLeft3D(
	VEC3 pos,		//	左辺中央座標
	VEC3 right,		//	右ベクトル
	VEC3 up,		//	上ベクトル
	D3DCOLOR c,		//	色
	float scale		//	スケール [m/px]
){
	float tw = m_Width*scale, th = m_Height*scale;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)m_PosX/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+m_Width)/g_StrTexRadius,
		(float)(m_PosY+m_Height)/g_StrTexRadius);
	TexMap3DRect(pos+0.5f*th*up, pos+tw*right+0.5f*th*up,
		pos+tw*right-0.5f*th*up, pos-0.5f*th*up, c);
}

/*
 *	中央揃えで描画 (3D)
 */
void CStringDrawer::RenderCenter3D(
	VEC3 pos,		//	縦横中央座標
	VEC3 right,		//	右ベクトル
	VEC3 up,		//	上ベクトル
	D3DCOLOR c,		//	色
	float scale		//	スケール [m/px]
){
	float tw = m_Width*scale, th = m_Height*scale;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)m_PosX/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+m_Width)/g_StrTexRadius,
		(float)(m_PosY+m_Height)/g_StrTexRadius);
	TexMap3DRect(pos-0.5f*tw*right+0.5f*th*up, pos+0.5f*tw*right+0.5f*th*up,
		pos+0.5f*tw*right-0.5f*th*up, pos-0.5f*tw*right-0.5f*th*up, c);
}

/*
 *	右揃えで描画 (3D)
 */
void CStringDrawer::RenderRight3D(
	VEC3 pos,		//	右辺中央座標
	VEC3 right,		//	右ベクトル
	VEC3 up,		//	上ベクトル
	D3DCOLOR c,		//	色
	float scale		//	スケール [m/px]
){
	float tw = m_Width*scale, th = m_Height*scale;
	devSetTexture(0, m_Texture->GetObject());
	SetUVMap((float)m_PosX/g_StrTexRadius,
		(float)m_PosY/g_StrTexRadius,
		(float)(m_PosX+m_Width)/g_StrTexRadius,
		(float)(m_PosY+m_Height)/g_StrTexRadius);
	TexMap3DRect(pos-tw*right+0.5f*th*up, pos+0.5f*th*up,
		pos-0.5f*th*up, pos-tw*right-0.5f*th*up, c);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CStringTexture::CStringTexture(
	int font,	//	フォント高
	int page	//	ページ数
){
	int i, j, p;
	g_StrTexRadius = CheckArguments("-voodoo") ? 256 : 512;
	g_StrTexColWidth = 64;
	g_StrTexCols = g_StrTexRadius/g_StrTexColWidth;
	m_FontSize = font;
	m_Rows = g_StrTexRadius/(m_FontSize+1);
	m_Page = page;
	m_Entry = g_StrTexCols*m_Rows*m_Page;
	m_Texture = new CTexture[m_Page];
	m_Drawer = new CStringDrawer[m_Entry];
	for(p = 0; p<m_Page; p++){
		m_Texture[p].Create(g_StrTexRadius, g_StrTexRadius);
		for(i = 0; i<m_Rows; i++)
			for(j = 0; j<g_StrTexCols; j++)
				m_Drawer[(p*m_Rows+i)*g_StrTexCols+j].Init(
					j*g_StrTexColWidth, i*(m_FontSize+1), &m_Texture[p]);
	}
}

/*
 *	デストラクタ
 */
CStringTexture::~CStringTexture(){
	DELETE_A(m_Texture);
	DELETE_A(m_Drawer);
}

/*
 *	文字列描画
 */
CStringDrawer *CStringTexture::DrawString(
	const char *str,	//	文字列
	D3DCOLOR sdw		//	影色
){
	int i, j, p;
	DWORD oldtime[512/64];
	CStringDrawer *old[512/64] = {NULL};
	for(p = 0; p<m_Page; p++){
		for(i = 0; i<m_Rows; i++){
			for(j = 0; j<g_StrTexCols; j++){
				int k = (p*m_Rows+i)*g_StrTexCols+j, l;
				if(m_Drawer[k].Check(str, sdw)) return &m_Drawer[k];
				for(l = 0; l<g_StrTexCols-j; l++){
					int t = m_Drawer[k].FindNewest(l);
					if(!old[l] || oldtime[l]>t){
						oldtime[l] = t;
						old[l] = &m_Drawer[k];
					}
				}
			}
		}
	}
	int tw, th, cw = g_StrTexCols-1;
	CalcTextRect(&tw, &th, str, m_hFont);
	int ttw = sdw ? tw+1 : tw;
	for(i = 0; i<g_StrTexCols-1; i++){
		if(ttw<=(i+1)*g_StrTexColWidth){
			cw = i;
			break;
		}
	}
	old[cw]->Enable(str, tw, th, cw, m_hFont, sdw);
	return old[cw];
}

/*
 *	テクスチャ全描画
 */
void CStringTexture::RenderAll(
	int x, int y,	//	座標
	int p			//	ページ
){
	devSetTexture(0, m_Texture[p].GetObject());
	SetUVMap(0.0f, 0.0f, 1.0f, 1.0f);
	TexMap2DRect(x, y, x+g_StrTexRadius, y+g_StrTexRadius);
}
