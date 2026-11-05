#ifndef CCURSOR_H_INCLUDED
#define CCURSOR_H_INCLUDED

/*
 *	カーソルクラス
 */
class CCursor{
private:
	int m_State;	//	状態
	int m_Resizing;	//	リサイズ中
	float m_Alpha;	//	アルファ値
	POINT m_Pos;	//	座標
	POINT m_Delta;	//	中心からの差分
public:
	void Init();
	void Clip();
	void Center();
	void Render();
	void ScanInput(bool forcelock = false);
	void FixCursor();
	void SetResize(int r){ if(m_Resizing<-1) m_Resizing = r; }
	POINT GetPos(){ return m_Pos; }
	POINT GetDelta(){ return m_Delta; }
	bool CheckDrag();
	VEC3 GetVEC3(){ return VEC3((float)m_Pos.x, (float)m_Pos.y, 0.0f); }
	void Lock();
	void Release();
	bool IsLock(){ return !!m_State; }
};

//	外部グローバル
extern CCursor g_Cursor;

#endif
