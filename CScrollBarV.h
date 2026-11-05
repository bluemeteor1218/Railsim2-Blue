#ifndef CSCROLLBARV_H_INCLUDED
#define CSCROLLBARV_H_INCLUDED

#include "CMiniButton.h"

const int SLIDER_MIN = 4;			//	スライダ最小幅
const int SLIDER_DRAG_DIST = 100;	//	ドラッグ最大距離

/*
 *	垂直スクロールバー
 */
class CScrollBarV: public CInterface{
private:
	int m_State;				//	状態
	int m_Repeat;				//	長押カウンタ
	int m_Scroll;				//	スクロール位置
	int m_Page;					//	スライダ幅
	int m_Range;				//	全範囲
	int m_OldS;					//	移動前座標
	int m_DragS;				//	タイトルバードラッグ座標
	CMiniButton m_UpButton;		//	上ボタン
	CMiniButton m_DownButton;	//	下ボタン
public:
	void Init(int, int, int, int, CInterface *);
	void SetSize(int, int);
	int GetScroll(){ return m_Scroll; }
	void SetScroll(int);
	void SetPage(int);
	void SetRange(int);
	void CalcSlider(int *, int *);
	bool IsInsideScroll(int, int);
	bool IsInsideSlider(int, int);
	bool ScanInput();
	void Render();
};

#endif
