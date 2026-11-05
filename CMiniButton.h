#ifndef CMINIBUTTON_H_INCLUDED
#define CMINIBUTTON_H_INCLUDED

#include "CInterface.h"

/*
 *	正方形ミニボタン
 */
class CMiniButton: public CInterface{
private:
	int m_State;			//	状態
	int m_Repeat;			//	長押カウンタ
	bool m_Mode;			//	モード (0: ボタン, 1: リピータ)
	bool m_Pushed;			//	押下フラグ
	float m_TexU, m_TexV;	//	テクスチャ座標
public:
	void Init(int, int, int, int, CInterface *, float, float, bool);
	void SetUV(float u, float v){ m_TexU = u; m_TexV = v; }
	int GetRepeat(){ return m_Repeat; }
	bool IsRepeat(){ return m_Repeat>REPEAT_FRAME; }
	bool IsPushed(){ return m_Pushed; }
	void SetPush(bool p){ m_Pushed = p; }
	bool ScanInput();
	void Render();
};

#endif
