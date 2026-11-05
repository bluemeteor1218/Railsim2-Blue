#ifndef CPIXELBIT_H_INCLUDED
#define CPIXELBIT_H_INCLUDED

#include <windows.h>

//#define ENABLE_SPECIFIC_STAMP	//	特殊スタンプを使う
//#define ENABLE_RECT_BOUNDARY	//	領域制限を使う

/*
 *	DIB ビットマップ管理クラス
 */
class CPixelbit{
private:
	static HMODULE ms_hMSIMG32;
	static BOOL (WINAPI *ms_TransBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT);
	int m_Width;			//	幅
	int m_Height;			//	高さ
#ifdef ENABLE_RECT_BOUNDARY
	int m_BoundLeft;		//	描画制限領域左 X (内)
	int m_BoundTop;			//	描画制限領域上 Y (内)
	int m_BoundRight;		//	描画制限領域右 X (外)
	int m_BoundBottom;		//	描画制限領域下 Y (外)
#endif
	DWORD m_BitsPerPixel;	//	ピクセル当たりビット数
	DWORD m_BytesPerPixel;	//	ピクセル当たりバイト数
	DWORD m_BytesPerLine;	//	1行あたりバイト数
	DWORD m_DwordsPerLine;	//	1行あたりDWORD数
	DWORD m_ColorKey;		//	透過色
	HBITMAP m_BmpHnd;		//	ビットマップハンドル
	HBITMAP m_BmpHndOld;	//	ビットマップハンドル
	HDC m_BmpHdc;			//	デバイスコンテキスト
	BITMAPINFO m_BmpInfo;	//	ビットマップヘッダ
	PDWORD m_PixelAdr;		//	画像データのアドレス
public:
	static BOOL InitExtFunc();
	static void FreeExtFunc();
	CPixelbit();
	CPixelbit(int, int);
	~CPixelbit();

	LPBITMAPINFO GetBmpInfo(){ return &m_BmpInfo; }

	BOOL Clear(int w, int h, int sbit = 32);
	int Load(char *);
	BOOL Save(char *, int);
	int PasteFromClipboard(HWND);
	BOOL CopyToClipboard(HWND, int);
	BOOL Assign(PBYTE);
	void Set4PAL1(int, int, PBYTE);
	void Set4PAL4(int, int, PBYTE);
	void Set4PAL8(int, int, PBYTE);
	void Set4BGR24(int, int, PBYTE);
	void Set4BGR32(int, int, PBYTE);
	void PrepareDIB(PBYTE, int, int);

	//	CPixelbitPNG.cpp
	int LoadPNG(char *, double *, bool *);
	BOOL SavePNG(char *, int, double);

	//	inline
	DWORD GetBytesPerPixel() const { return m_BytesPerPixel; }
	DWORD GetBytesPerLine() const { return m_BytesPerLine; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int Index(int x, int y) const { return x+y*m_Width; } // for 32bit only
	HDC GetHDC(){ return m_BmpHdc; }
	BOOL CheckSize(CPixelbit *img){
		return m_Width==img->m_Width && m_Height==img->m_Height;
	}
	BOOL CheckSize(int w, int h){
		return m_Width==w && m_Height==h;
	}
	BOOL CheckRect(int x, int y){
		return 0<=x && x<m_Width && 0<=y && y<m_Height;
	}
	DWORD GetPixel(int x, int y){ // for 32bit only
		return m_PixelAdr[Index(x, y)];
	}
	void SetPixel(int x, int y, DWORD color){ // for 32bit only
		m_PixelAdr[Index(x, y)] = color;
	}
	PBYTE GetByteAdr(int x, int y){ // for 32bit only
		return (PBYTE)(&m_PixelAdr[Index(x, y)]);
	}
	PDWORD GetScanLine(int y){
		return m_PixelAdr+m_DwordsPerLine*y;
	}
	DWORD GetColorKey(){
		return m_ColorKey;
	}
	void SetColorKey(DWORD col){
		m_ColorKey = col;
	}

	//	CPixelbitStamp.cpp
#ifdef ENABLE_RECT_BOUNDARY
	void ResetBoundary();
	void SetBoundary(int, int, int, int);
#endif
	void ResetAlphaChannel(BYTE);
	BOOL FixLocation(CPixelbit *, int *, int *, int *, int *, int *, int *);
	BOOL WindowStamp(HDC hdcDst, int, int, int, int, int, int);
	BOOL PlainStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL PlainStamp32(CPixelbit *, int, int, int, int, int, int);
	BOOL AlphaBlendStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL StretchStamp(CPixelbit *, int, int, int, int, int, int, int, int, int);
	BOOL BilinearStamp(CPixelbit *, int, int, int, int, int, int, int, int);
	BOOL NearestStamp(CPixelbit *, int, int, int, int, int, int, int, int);
#ifdef ENABLE_SPECIFIC_STAMP
	BOOL DarkStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL LightStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL HalfStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL DarkHalfStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL LightHalfStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL RedStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL GreenStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL BlueStamp(CPixelbit *, int, int, int, int, int, int);
	BOOL ColorStamp(CPixelbit *, int, int, int, int, int, int, DWORD);
	BOOL AlphaStamp(CPixelbit *, int, int, int, int, int, int, int);
	BOOL AlphaColorStamp(CPixelbit *, int, int, int, int, int, int, DWORD, int);
	BOOL AddStamp(CPixelbit *, int, int, int, int, int, int, int);
	BOOL AddColorStamp(CPixelbit *, int, int, int, int, int, int, DWORD, int);
	BOOL SubStamp(CPixelbit *, int, int, int, int, int, int, int);
	BOOL SubColorStamp(CPixelbit *, int, int, int, int, int, int, DWORD, int);
#endif

	//	CPixelbitDraw.cpp
	void DrawRect(int, int, int, int, DWORD);
	void FillRect(int, int, int, int, DWORD);
	void DrawLine(int, int, int, int, DWORD);
	void BoldLine(int, int, int, int, DWORD);
	BOOL CreateAAText(int, int, char *, HFONT, DWORD, int);
	void GradRectH(int, int, int, int, DWORD, DWORD);
	void GradRectV(int, int, int, int, DWORD, DWORD);

	//	CPixelbitEffect.cpp
	void Nega();
	void Monotone(DWORD);
	void Darkness(int);
	void Lightness(int);

	//	CPixelbitCrypt.cpp
	static unsigned int EncodePassword(char *);
	void Encode(char *);
	void Decode(char *);
};

//	関数宣言
POINT MakePoint(int, int);
RECT MakeRect(int, int, int, int);
void RGBtoHSV(BYTE, BYTE, BYTE, int *, int *, int *);

/*
 *	輝度値をk 0～255 で丸める
 */
inline BYTE FixCV(int v){ return v<0 ? 0 : (v>255 ? 255 : v); }

/*
 *	DWORD に RGB をまとめる
 *
 *	戻り値: 色
 */
inline DWORD MakePixel(
	BYTE r, BYTE g, BYTE b	//	RGB 色
){
	return (r<<16)|(g<<8)|b;
}

/*
 *	DWORD に RGB をまとめる
 *
 *	戻り値: 色
 */
inline DWORD MakePixelA(
	BYTE a, BYTE r, BYTE g, BYTE b	//	RGB 色
){
	return (a<<24)|(r<<16)|(g<<8)|b;
}

/*
 *	R と B を入れ替える
 *
 *	戻り値: 色
 */
inline DWORD RGBtoBGR(
	DWORD c	//	色
){
	return (c&0xff00ff00)|((c&0x00ff0000)>>16)|((c&0x000000ff)<<16);
}

/*
 *	DWORD を RGB に分解
 */
inline void SplitColor(
	DWORD c,				//	分解する色
	int *r, int *g, int *b	//	代入先
){
	*r = (c&0x00ff0000)>>16;
	*g = (c&0x0000ff00)>>8;
	*b = c&0x000000ff;
}

/*
 *	DWORD を ARGB に分解
 */
inline void SplitColorA(
	DWORD c,						//	分解する色
	int *a, int *r, int *g, int *b	//	代入先
){
	*a = (c&0xff000000)>>24;
	*r = (c&0x00ff0000)>>16;
	*g = (c&0x0000ff00)>>8;
	*b = c&0x000000ff;
}

/*
 *	RGB を Y に変換 (RGB 入力)
 *
 *	戻り値: Y 値
 */
inline int BlackScale(BYTE r, BYTE g, BYTE b){
	return (int)(r*0.298912f+g*0.586610f+b*0.114478f);
}

/*
 *	RGB を Y に変換 (DWORD 入力)
 *
 *	戻り値: Y 値
 */
inline int BlackScale(DWORD c){
	return (int)(((c&0x00ff0000)>>16)*0.298912f
		+((c&0x0000ff00)>>8)*0.586610f+(c&0x000000ff)*0.114478f);
}

/*
 *	RGB を Y に変換
 *
 *	戻り値: Y 値 (最大値 256)
 */
inline int BlackScale256(DWORD c){
	return (int)(((c&0x00ff0000)>>16)*0.300084f
		+((c&0x0000ff00)>>8)*0.5889104f+(c&0x000000ff)*0.114927f);
}

#ifndef ENABLE_RECT_BOUNDARY
#define m_BoundLeft		0
#define m_BoundTop		0
#define m_BoundRight	m_Width
#define m_BoundBottom	m_Height
#endif

#endif
