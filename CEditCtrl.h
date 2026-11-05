#ifndef CEDITCTRL_H_INCLUDED
#define CEDITCTRL_H_INCLUDED

#include "CInterface.h"

//	内部定数
const int EB_OFSX = 3;	//	編集位置

/*
 *	エディットコントロール
 */
class CEditCtrl: public CInterface{
private:
	int m_State;		//	状態
	int m_StrMax;		//	最大文字数
	bool m_KeepImm;		//	IMM 状態保持
	bool m_CompDelay;	//	変換ディレイ
	CEditBox m_EditBox;	//	エディットボックス
public:
	void Init(int, int, int, int, char *, CInterface *, int);
	void GiveFocus(bool snd = true);
	void SetKeepIMM(bool imm){ m_KeepImm = imm; }
	void FinishInput();
	bool IsComp(){ return m_CompDelay; }
	string GetRealtimeText();
	bool ScanInput();
	void Render();
};

#endif
