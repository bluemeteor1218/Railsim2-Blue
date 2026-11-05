#ifndef CSTATICCTRL_H_INCLUDED
#define CSTATICCTRL_H_INCLUDED

#include "CInterface.h"

/*
 *	スタティックコントロール
 */
class CStaticCtrl: public CInterface{
private:
	int m_HorzPos;	//	水平位置 (0: left, 1: center, 2: right)
	int m_VertPos;	//	垂直位置 (0: top, 1: center, 2: bottom)
public:
	void Init(int, int, int, int, char *, CInterface *, int, int);
	void Render();
};

#endif
