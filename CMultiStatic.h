#ifndef CMULTISTATIC_H_INCLUDED
#define CMULTISTATIC_H_INCLUDED

#include "CInterface.h"
#include "CScrollBarV.h"

/*
 *	マルチラインスタティックコントロール
 */
class CMultiStatic: public CInterface{
private:
	int m_State;				//	状態
	int m_Lines;				//	表示可能行数
	int m_LineHeight;			//	列高
	int m_FocusIndex;			//	フォーカス位置
	int m_LineNum;				//	データ数
	list<string> m_LineText;	//	行ごとのテキスト
	CScrollBarV m_ScrollV;		//	スクロールバー
public:
	void Init(int, int, int, int, CInterface *, int);
	void SetSize(int, int);
	void SetText(char *);
	void SetScroll();
	bool IsInsideList(int, int);
	bool ScanInput();
	void Render();
};

#endif
