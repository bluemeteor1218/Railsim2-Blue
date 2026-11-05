//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "font.h"

/*
 *	フォント作成
 *
 *	size	: 縦サイズ
 *	c		: 色
 *	weight	: 太さ(FW_NORMAL, FW_BOLD, FW_HEAVY...)
 */
void CreateFont(int size, D3DCOLOR c, LONG weight){
	FreeFont();

	LOGFONT logFont = {
		size, 0, 0, 0,
		weight,
		FALSE, FALSE, FALSE,
		SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,
		"ＭＳ ゴシック"
	};

	D3DXCreateFontIndirect(sv3.pDev, &logFont, &svf.pFont);
	svf.size = size;
	svf.color = c;
}

/*
 *	フォント解放
 */
void FreeFont(){
	RELEASE(svf.pFont);
}

/*
 *	フォント使用開始
 */
void BeginFont(){
	svf.pFont->Begin();
}

/*
 *	フォント使用終了
 */
void EndFont(){
	svf.pFont->End();
}

/*
 *	テキスト描画
 *
 *	x	: X 座標
 *	y	: Y 座標
 *	str	: 文字列
 *
 *	※負荷が高いのであまり頻繁に使用しないように。
 *	静的な文字列の描画はCTexture: :CreateText()を利用する方が吉。
 */
void Text(int x, int y, D3DCOLOR c, const char *str){
	RECT rect = {x, y, sv3.width, sv3.height};

	svf.color = c;
	svf.pFont->DrawTextA(str, -1, &rect, DT_LEFT|DT_EXPANDTABS|DT_NOPREFIX, svf.color);
}

/*
 *	テキスト描画（書式付）
 *
 *	x	: X 座標
 *	y	: Y 座標
 *	str	: 文字列
 */
void TextF(int x, int y, D3DCOLOR c, const char *str, ...){
	char buf[4096];

	va_list	vl;

	va_start(vl, str);
	vsprintf(buf, str, vl);
	va_end(vl);

	Text(x, y, c, buf);
}
