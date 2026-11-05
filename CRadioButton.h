#ifndef CRADIOBUTTON_H_INCLUDED
#define CRADIOBUTTON_H_INCLUDED

#include "CInterface.h"

/*
 *	ラジオボタン
 */
class CRadioButton: public CInterface{
private:
	int m_State;	//	状態
	int m_Check;	//	チェックフラグ
	CRadioButton *m_Prev, *m_Next;	//	グループリンク
public:
	void Init(int, int, int, int, char *, CInterface *, CRadioButton *);
	void ClearGroupCheck();
	void SetCheck(){
		ClearGroupCheck();
		m_Check = 1;
	}
	int GetCheck(){ return m_Check; }
	int GetNumber();
	bool ScanInput();
	void Render();
};

#endif
