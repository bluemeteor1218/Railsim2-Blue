#ifndef CPUSHBUTTON_H_INCLUDED
#define CPUSHBUTTON_H_INCLUDED

#include "CInterface.h"

/*
 *	プッシュボタン
 */
class CPushButton: public CInterface{
private:
	int m_State;		//	状態
	bool m_Sound;		//	音設定
	bool m_Pushed;		//	押下フラグ
	bool m_Pushable;	//	プッシュ可能 /*CP932対応*/
public:
	void Init(int, int, int, int, char *, CInterface *);
	void SetSound(bool s){ m_Sound = s; }
	void SetPushable(bool p){ m_Pushable = p; }
	bool IsPushed(){ return m_Pushed; }
	void SetPush(bool p){ m_Pushed = p; }
	bool ScanInput();
	void Render();
};

#endif
