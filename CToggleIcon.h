#ifndef CTOGGLEICON_H_INCLUDED
#define CTOGGLEICON_H_INCLUDED

#include "CInterface.h"

class CGameMode;

/*
 *	サイドパネル・メニューアイコン
 */
class CToggleIcon: public CInterface{
private:
	static CToggleIcon *ms_PointIcon;	//	ポイントされたアイコン
	int m_State;					//	状態
	int m_Check;					//	チェックフラグ
	int m_ModeID;					//	モード ID
	float m_SlideY;					//	スライド Y 座標
	float m_TexPullU, m_TexPullV;	//	開放時テクスチャ座標
	float m_TexPushU, m_TexPushV;	//	押下時テクスチャ座標
	float m_TexIconU, m_TexIconV;	//	アイコンテクスチャ座標
	string m_Expression;			//	説明文
	DWORD m_HotKey;					//	ホットキー
	CToggleIcon *m_Prev, *m_Next;	//	グループリンク
	CGameMode *m_SyncMode;			//	対応モード
public:
	static void RenderPopupText();
	void Init(int, int, int, int, char *, char *,
		CInterface *, CToggleIcon*, CGameMode *, DWORD,
		float, float, float, float, float, float, int);
	void ClearGroupCheck();
	void SetCheck(bool);
	int GetCheck(){ return m_Check; }
	void SetSlidePos(int, int, bool fix = false);
	bool ScanInput(bool, bool);
	void Render(float altu = -1.0f, float altv = -1.0f);
};

#endif
