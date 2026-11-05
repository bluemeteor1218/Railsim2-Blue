#ifndef CSTRINGTEXTURE_H_INCLUDED
#define CSTRINGTEXTURE_H_INCLUDED

/*
 *	文字列描画子
 */
class CStringDrawer{
	friend class CStringTexture;
private:
	int m_PosX, m_PosY;		//	位置
	int m_Width, m_Height;	//	サイズ
	int m_Cols;				//	占有列数 (0, 1, ..., STRTEX_COLS-1)
	DWORD m_Date;			//	更新時刻
	string m_String;		//	文字列
	D3DCOLOR m_ShadowColor;	//	陰影色
	CTexture *m_Texture;	//	テクスチャ
	CStringDrawer *m_Key;	//	基準位置
public:
	void Init(int, int, CTexture *);
	void Reset(CStringDrawer *);
	void Enable(const char *, int, int, int, HFONT, D3DCOLOR);
	bool Check(const char *, D3DCOLOR);
	DWORD FindNewest(int);
	int GetWidth(){ return m_Width; }
	void RenderLeft(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderCenter(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderRight(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderLeftV(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderCenterV(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderRightV(int, int, D3DCOLOR, int mw = -1, int mh = -1);
	void RenderLeft3D(VEC3, VEC3, VEC3, D3DCOLOR, float);
	void RenderCenter3D(VEC3, VEC3, VEC3, D3DCOLOR, float);
	void RenderRight3D(VEC3, VEC3, VEC3, D3DCOLOR, float);
};

/*
 *	文字列テクスチャ
 */
class CStringTexture{
private:
	int m_FontSize;	//	フォント高
	int m_Rows;		//	行数
	int m_Page;		//	ページ数
	int m_Entry;	//	エントリ数
	HFONT m_hFont;	//	フォント
	CTexture *m_Texture;		//	テクスチャ
	CStringDrawer *m_Drawer;	//	描画子
public:
	CStringTexture(int, int);
	~CStringTexture();
	void SetFont(HFONT hf){ m_hFont = hf; }
	CStringDrawer *DrawString(const char *, D3DCOLOR);
	void RenderAll(int, int, int);
	void RenderLeft(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderLeft(x, y, c, mw, mh);
	}
	void RenderCenter(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderCenter(x, y, c, mw, mh);
	}
	void RenderRight(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderRight(x, y, c, mw, mh);
	}
	void RenderLeftS(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		CStringDrawer *sd = DrawString(str, 0);
		if(sdw) sd->RenderLeft(x+1, y+1, sdw, mw, mh);
		sd->RenderLeft(x, y, c, mw, mh);
	}
	void RenderCenterS(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		CStringDrawer *sd = DrawString(str, 0);
		if(sdw) sd->RenderCenter(x+1, y+1, sdw, mw, mh);
		sd->RenderCenter(x, y, c, mw, mh);
	}
	void RenderRightS(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		CStringDrawer *sd = DrawString(str, 0);
		if(sdw) sd->RenderRight(x+1, y+1, sdw, mw, mh);
		sd->RenderRight(x, y, c, mw, mh);
	}
	void RenderLeftV(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderLeftV(x, y, c, mw, mh);
	}
	void RenderCenterV(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderCenterV(x, y, c, mw, mh);
	}
	void RenderRightV(int x, int y, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, int mw = -1, int mh = -1){
		DrawString(str, sdw)->RenderRightV(x, y, c, mw, mh);
	}
	void RenderLeft3D(VEC3 p, VEC3 d, VEC3 u, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, float s){
		DrawString(str, sdw)->RenderLeft3D(p, d, u, c, s);
	}
	void RenderCenter3D(VEC3 p, VEC3 d, VEC3 u, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, float s){
		DrawString(str, sdw)->RenderCenter3D(p, d, u, c, s);
	}
	void RenderRight3D(VEC3 p, VEC3 d, VEC3 u, D3DCOLOR c, D3DCOLOR sdw,
		const char *str, float s){
		DrawString(str, sdw)->RenderRight3D(p, d, u, c, s);
	}
};

//	外部グローバル
extern CStringTexture *g_StrTex;

#endif
