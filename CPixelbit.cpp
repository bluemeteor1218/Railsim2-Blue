#include "CPixelbit.h"

//	関数宣言
void Dialog(char *, ...);

//	内部グローバル
HMODULE CPixelbit::ms_hMSIMG32;
BOOL (WINAPI *CPixelbit::ms_TransBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT);

/*
 *	[static]
 *	外部ライブラリの初期化
 */
BOOL CPixelbit::InitExtFunc(){
	ms_hMSIMG32 = LoadLibrary("msimg32.dll");
	ms_TransBlt = (BOOL (WINAPI *)(HDC, int, int, int, int,
		HDC, int, int, int, int, UINT)) GetProcAddress(ms_hMSIMG32, "TransparentBlt");
	if(!ms_TransBlt) return FALSE;
	return TRUE;
}

/*
 *	[static]
 *	外部ライブラリの解放
 */
void CPixelbit::FreeExtFunc(){
	FreeLibrary(ms_hMSIMG32);
}

/*
 *	コンストラクタ
 */
CPixelbit::CPixelbit(){
	m_PixelAdr = NULL;
	m_ColorKey = 0xffffffff;
	m_Width = 0;
	m_Height = 0;
	Clear(1, 1);
}

/*
 *	コンストラクタ (サイズ指定)
 */
CPixelbit::CPixelbit(
	int w, int h	//	サイズ
){
	m_PixelAdr = NULL;
	m_ColorKey = 0xffffffff;
	m_Width = 0;
	m_Height = 0;
	Clear(w, h);
}

/*
 *	デストラクタ
 */
CPixelbit::~CPixelbit(){
	if(m_PixelAdr){
		SelectObject(m_BmpHdc, m_BmpHndOld);
		DeleteObject(m_BmpHnd);
		DeleteDC(m_BmpHdc);
		m_PixelAdr = NULL;
	}
}

/*
 *	サイズを指定して初期化
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::Clear(
	int w, int h,	//	サイズ
	int sbit		//	ビット深度
){
	if(sbit!=24) sbit = 32;
	m_BitsPerPixel = sbit;
	m_BytesPerPixel = m_BitsPerPixel/8;
	m_BytesPerLine = (w*m_BytesPerPixel+3)&~3;
	m_DwordsPerLine = m_BytesPerLine/4;
	PDWORD adr = NULL;
	BITMAPINFO bmpinfo;
	HBITMAP bmphnd;
	HDC hdc, hdcnew;
	if(w<=0 || h==0) return FALSE;
	ZeroMemory(&bmpinfo, sizeof(bmpinfo));
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = w;
	bmpinfo.bmiHeader.biHeight = -h;
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = sbit;
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmphnd = CreateDIBSection(NULL, &bmpinfo, DIB_RGB_COLORS, (void **)(&adr), NULL, 0);
	if(!bmphnd) return FALSE;
	hdc = GetDC(0);
	hdcnew = CreateCompatibleDC(hdc);
	ReleaseDC(0, hdc);
	if(!hdcnew){
		DeleteObject(bmphnd);
		return FALSE;
	}
	if(m_PixelAdr){
		SelectObject(m_BmpHdc, m_BmpHndOld);
		DeleteObject(m_BmpHnd);
		DeleteDC(m_BmpHdc);
		m_PixelAdr = NULL;
	}
	m_BmpInfo = bmpinfo;
	m_PixelAdr = adr;
	m_BmpHnd = bmphnd;
	m_BmpHdc = hdcnew;
	m_BmpHndOld = (HBITMAP)SelectObject(m_BmpHdc, m_BmpHnd);
	m_Width = w;
	m_Height = h < 0 ? -h : h;
#ifdef ENABLE_RECT_BOUNDARY
	ResetBoundary();
#endif
	ZeroMemory(m_PixelAdr, m_BytesPerLine*m_Height);
	return TRUE;
}

/*
 *	ファイルから読込
 *
 *	戻り値: 成功したらビット深度を返す
 */
int CPixelbit::Load(
	char *fname	//	ファイル名
){
	PBYTE buf;
	HANDLE fh;
	DWORD load;
	int size;
	if(!fname) return 0;
	fh = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fh==INVALID_HANDLE_VALUE) return 0;
	buf = new BYTE[size = GetFileSize(fh, NULL)];
	ReadFile(fh, buf, size, &load, NULL);
	CloseHandle(fh);
	if(Assign(buf+sizeof(BITMAPFILEHEADER))){
		int sbit = ((LPBITMAPINFOHEADER)(buf+sizeof(BITMAPFILEHEADER)))->biBitCount;
		delete [] buf;
		return sbit;
	}else{
		delete [] buf;
		return 0;
	}
}

/*
 *	ファイルに保存
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::Save(
	char *fname,	//	ファイル名
	int sbit		//	ビット深度
){
	DWORD wrote, size;
	if(m_Width*m_Height<1) return FALSE;
	if(sbit!=24) sbit = 32;
	int len = (m_Width*sbit/8+3)&~3;
	size = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+len*m_Height;
	PBYTE buf = new BYTE[size];
	ZeroMemory(buf, size);
	buf[0] = 'B';
	buf[1] = 'M';
	CopyMemory(buf+2, &size, 4);
	*(PDWORD)(buf+10) = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	PrepareDIB(buf+14, len, sbit);
	HANDLE fh = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(fh, buf, size, &wrote, NULL);
	CloseHandle(fh);
	delete [] buf;
	return wrote==size;
}

/*
 *	クリップボードから貼り付け
 *
 *	戻り値: 成功したらビット深度を返す
 */
int CPixelbit::PasteFromClipboard(
	HWND hWnd	//	窓ハンドル
){
	if(!OpenClipboard(hWnd)) return 0;
	PBYTE buf;
	HANDLE clip = GetClipboardData(CF_DIB);
	if(!clip || !(buf = (PBYTE)GlobalLock(clip)) || !Assign(buf)){
		CloseClipboard();
		return 0;
	}
	int sbit = ((LPBITMAPINFOHEADER)buf)->biBitCount;
	GlobalUnlock(clip);
	CloseClipboard();
	return sbit;
}

/*
 *	クリップボードにコピー
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::CopyToClipboard(
	HWND hWnd,	//	窓ハンドル
	int sbit	//	ビット深度
){
	if(m_Width*m_Height<1) return FALSE;
	if(!OpenClipboard(hWnd)) return FALSE;
	sbit = sbit==32 ? 32 : 24;
	int len = (m_Width*sbit/8+3)&~3;
	int size = sizeof(BITMAPINFOHEADER)+len*m_Height;
	PBYTE buf;
	HGLOBAL data = GlobalAlloc(GHND|GMEM_SHARE, size);
	if(!data || !(buf = (PBYTE)GlobalLock(data))){
		CloseClipboard();
		return FALSE;
	}
	PrepareDIB(buf, len, sbit);
	GlobalUnlock(data);
	EmptyClipboard();
	SetClipboardData(CF_DIB, data);
	CloseClipboard();
	return TRUE;
}

/*
 *	メモリ領域上のビットマップを割り当てる
 *
 *	戻り値: 成功したら TRUE を返す
 */
BOOL CPixelbit::Assign(
	PBYTE ptr	//	BITMAPINFOHEADER の先頭
){
	int w, h;
	LPBITMAPINFOHEADER head;
	if(!ptr) return FALSE;
	head = (LPBITMAPINFOHEADER)ptr;
	w = head->biWidth;
	h = head->biHeight;
	if(w*h<1) return FALSE;
	if(!Clear(w, h)) return FALSE;
	PBYTE adr = ptr+sizeof(BITMAPINFOHEADER);
	if(head->biCompression==BI_BITFIELDS) adr += 12;
	switch(head->biBitCount){
	case 1: Set4PAL1(m_Width, m_Height, adr); break;
	case 4: Set4PAL4(m_Width, m_Height, adr); break;
	case 8: Set4PAL8(m_Width, m_Height, adr); break;
	case 24: Set4BGR24(m_Width, m_Height, adr); break;
	case 32: Set4BGR32(m_Width, m_Height, adr); break;
	}
	return TRUE;
}

/*
 *	メモリからパレット画像 (4bit/pixel) を読み込む
 */
void CPixelbit::Set4PAL1(
	int w, int h,	//	画像サイズ
	PBYTE ptr		//	パレット開始アドレス
){
	int i, j, len = w%32 ? (w/32+1)*4 : w/8, bit;
	PDWORD pal = (PDWORD)ptr;
	ptr += 2*sizeof(DWORD);
	for(i = 0; i<h; i++){
		PBYTE src = ptr+(h-i-1)*len;
		PDWORD dest = m_PixelAdr+w*i;
		for(j = 0, bit = 7; j<w; j++, bit = (bit-1)&7){
			*dest++ = pal[(*src>>bit)&0x01];
			if(!bit) src++;
		}
	}
}

/*
 *	メモリからパレット画像 (4bit/pixel) を読み込む
 */
void CPixelbit::Set4PAL4(
	int w, int h,	//	画像サイズ
	PBYTE ptr		//	パレット開始アドレス
){
	int i, j, len = w%8 ? (w/8+1)*4 : w/2, bit;
	PDWORD pal = (PDWORD)ptr;
	ptr += 16*sizeof(DWORD);
	for(i = 0; i<h; i++){
		PBYTE src = ptr+(h-i-1)*len;
		PDWORD dest = m_PixelAdr+w*i;
		for(j = 0, bit = 4; j<w; j++, bit = (bit-4)&7){
			*dest++ = pal[(*src>>bit)&0x0f];
			if(!bit) src++;
		}
	}
}

/*
 *	メモリからパレット画像 (8bit/pixel) を読み込む
 */
void CPixelbit::Set4PAL8(
	int w, int h,	//	画像サイズ
	PBYTE ptr		//	パレット開始アドレス
){
	int i, j, len = w%4 ? (w/4+1)*4 : w;
	PDWORD pal = (PDWORD)ptr;
	ptr += 256*sizeof(DWORD);
	for(i = 0; i<h; i++){
		PBYTE src = ptr+(h-i-1)*len;
		PDWORD dest = m_PixelAdr+w*i;
		for(j = 0; j<w; j++) *dest++ = pal[*src++];
	}
}

/*
 *	メモリ上の BGR (24bit/pixel) 列から画像を読み込む
 */
void CPixelbit::Set4BGR24(
	int w, int h,	//	画像サイズ
	PBYTE ptr		//	ビットマップ開始アドレス
){
	int i, j, len = w%4 ? (w*3/4+1)*4 : w*3;
	for(i = 0; i<h; i++){
		PBYTE src = ptr+(h-i-1)*len;
		PBYTE dest = (PBYTE)(m_PixelAdr+w*i);
		for(j = 0; j<w; j++){
			*dest++ = *src++;
			*dest++ = *src++;
			*dest++ = *src++;
			*dest++ = 0;
		}
	}
}

/*
 *	メモリ上の BGR (32bit/pixel) 列から画像を読み込む
 */
void CPixelbit::Set4BGR32(
	int w, int h,	//	画像サイズ
	PBYTE ptr		//	ビットマップ開始アドレス
){
	int i, j, len = w*4;
	for(i = 0; i<h; i++){
		PDWORD src = (PDWORD)(ptr+(h-i-1)*len);
		PDWORD dest = m_PixelAdr+w*i;
		for(j = 0; j<w; j++) *dest++ = *src++;
	}
}

/*
 *	DIB データ作成 (保存・クリップボードコピー用)
 */
void CPixelbit::PrepareDIB(
	PBYTE buf,	//	BITMAPINFOHEADER の先頭
	int len,	//	ライン長
	int sbit	//	ビット深度
){
	int i, j;
	LPBITMAPINFOHEADER head = (LPBITMAPINFOHEADER)buf;
	PBYTE pixel = (PBYTE)head+sizeof(BITMAPINFOHEADER);
	head->biSize = sizeof(BITMAPINFOHEADER);
	head->biWidth = m_Width;
	head->biHeight = m_Height;
	head->biPlanes = 1;
	head->biBitCount = sbit;
	if(m_BitsPerPixel==(unsigned int)sbit){
		memcpy(pixel, m_PixelAdr, len*m_Height);
	}else if(m_BitsPerPixel==32 && sbit==24){
		for(i = 0; i<m_Height; i++){
			PDWORD src = m_PixelAdr+m_Width*(m_Height-i-1);
			PBYTE dest = pixel+i*len;
			for(j = 0; j <m_Width; j++, src++){
				*dest++ = (BYTE)*src;
				*dest++ = (BYTE)(*src>>8);
				*dest++ = (BYTE)(*src>>16);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	POINT 構造体に値を代入して生成
 *
 *	戻り値:	POINT のインスタンス
 */
POINT MakeRect(
	int x, int y	//	座標
){
	POINT pos;
	pos.x = x;
	pos.y = y;
	return pos;
}

/*
 *	RECT 構造体に値を代入して生成
 *
 *	戻り値:	RECT のインスタンス
 */
RECT MakeRect(
	int x, int y,	//	座標
	int w, int h	//	サイズ
){
	RECT rect;
	rect.left = x;
	rect.top = y;
	rect.right = x+w;
	rect.bottom = y+h;
	return rect;
}

/*
 *	RGB 値から HSV 値を計算
 */
void RGBtoHSV(
	BYTE r, BYTE g, BYTE b,	//	RGB 値
	int *hh,	//	h 値 (色相) 代入先
	int *ss,	//	s 値 (彩度) 代入先
	int *vv		//	v 値 (明度) 代入先
){
	int rr, gg, bb, x, h, v, t;
	v = r>g ? (r>b ? r : b) : (g>b ? g : b);	//	_max(r, g, b);
	x = r<g ? (r<b ? r : b) : (g<b ? g : b);	//	_min(r, g, b);
	t = v-x;
	if(vv) *vv = v;
	if(ss) *ss = v ? t*255/v : 0;
	if(!hh) return;
	if(t){
		rr = (v-r)*255/t;
		gg = (v-g)*255/t;
		bb = (v-b)*255/t;
	}else{
		rr = gg = bb = 0;
	}
	if(g==v){
		h = r==x ? 1280+bb : 256-rr;
	}else if(r==v){
		h = b==x ? 256+gg : 768-bb;
	}else{
		h = g==x ? 768+rr : 1280-gg;
	}
	*hh = 359-(((h*60)>>8)+240)%360;
	//*hh = h%1536;	//	h = (h*60)/256; h %= 360;
}
