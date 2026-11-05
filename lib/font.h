//	Copyright (c) 2002 Midikyou

struct SYSVALUE_F{
	LPD3DXFONT	pFont;	//	フォント・インターフェイス
	int			size;	//	
	D3DCOLOR	color;	//	フォントの色
};
extern SYSVALUE_F svf;

void CreateFont(int size = 16, D3DCOLOR c = 0xffffffff, LONG weight = FW_NORMAL);
void FreeFont();
void BeginFont();
void EndFont();
void Text(int x, int y, D3DCOLOR c, const char *str);
void TextF(int x, int y, D3DCOLOR c, const char *str, ...);

/*
 *	某FPSゲームチックなフォントを使用
 */
inline void UseHLFont(){
	CreateFont(16, 0xffffee00, FW_BOLD);
}
