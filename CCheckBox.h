#ifndef CCHECKBOX_H_INCLUDED
#define CCHECKBOX_H_INCLUDED

#include "CInterface.h"

/*
 *	チェックボックス
 */
class CCheckBox: public CInterface{
private:
	int m_State;	//	状態
	int m_Check;	//	チェックフラグ
public:
	void Init(int, int, int, int, char *, CInterface *);
	void SetCheck(int chk){ m_Check = chk; }
	int GetCheck(){ return m_Check; }
	bool ScanInput();
	void Render();
};

#endif
