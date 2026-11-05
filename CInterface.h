#ifndef CINTERFACE_H_INCLUDED
#define CINTERFACE_H_INCLUDED

#include "CCursor.h"

const int TILE_UNIT = 16;	//	単位タイル幅 [px]
const int TILE_HALF = TILE_UNIT/2;	//	半分
const int TILE_QUAD = TILE_UNIT/4;	//	1/4

/*
 *	ユーザーインターフェイスクラス
 */
class CInterface{
	friend class CPopMenu;
protected:
	static CInterface *ms_Focus;	//	フォーカス
	static CInterface *ms_Drop;		//	ドロップ先
	static CInterface *ms_TabHead;	//	タブリンク
	int m_PosX, m_PosY;		//	座標
	int m_Width, m_Height;	//	サイズ
	bool m_Enabled;			//	有効フラグ
	bool m_Visible;			//	表示フラグ
	string m_Text;			//	テキスト
	CInterface *m_Parent;	//	親
	CInterface *m_Brother;	//	兄弟
	CInterface *m_Child;	//	子
	CInterface *m_Owner;	//	フォーカス所有者
	CInterface *m_TabPrev;	//	タブ次
	CInterface *m_TabNext;	//	タブ前
public:
	static void ResetTabHead(){ ms_TabHead = NULL; }
	static void ProcessKey();
	static CInterface *GetFocus(){ return ms_Focus; }
	static void SetFocus(CInterface *f){ ms_Focus = f; }
	CInterface();
	virtual ~CInterface();
	void Init(int, int, int, int, char *, CInterface *);
	CInterface *GetParent(){ return m_Parent; }
	void SetChild(CInterface *);
	void SetBrother(CInterface *);
	void RemoveChild(CInterface *);
	void SetOwner(CInterface *o){ m_Owner = o; }
	void LinkTab(bool);
	bool IsFocus(){ return ms_Focus==this; }
	CInterface *FindTabItem();
	int GetPosX(){ return m_PosX; }
	int GetPosY(){ return m_PosY; }
	void SetPos(int x, int y){ m_PosX = x; m_PosY = y; }
	int GetWidth(){ return m_Width; }
	int GetHeight(){ return m_Height; }
	virtual void SetSize(int w, int h){ m_Width = w; m_Height = h; }
	char *GetText(){ return (char *)m_Text.c_str(); }
	void SetText(char *t){ m_Text = t; }
	void GetAbsPos(int *, int *);
	bool IsEnabled(){ return m_Enabled; }
	void Enable(bool en){ m_Enabled = en; }
	bool IsVisible();
	void Show(bool f){ m_Visible = f; }
	bool IsInside(int, int);
	void DrawFocusFrame();
	virtual void GiveFocus(bool snd = true);
	virtual bool ScanInput();
	bool ScanInputChild(){ return m_Child && m_Child->ScanInput(); }
	bool ScanInputBrother(){ return m_Brother && m_Brother->ScanInput(); }
	virtual void Render();
	void RenderChild(){ if(m_Child) m_Child->Render(); }
	void RenderBrother(){ if(m_Brother) m_Brother->Render(); }
};

/*
 *	フォント縦位置修正
 */
inline int FontY(int h){ return (h-FONT_HEIGHT)>>1; }

#endif
