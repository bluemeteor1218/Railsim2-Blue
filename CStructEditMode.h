#ifndef CSTRUCTEDITMODE_H_INCLUDED
#define CSTRUCTEDITMODE_H_INCLUDED

#include "CSceneryMode.h"

/*
 *	施設編集モード
 */
class CStructEditMode: public CCursorSceneryMode{
private:
	int m_DragState;	//	範囲選択状態
	VEC3 m_DragBegin;	//	範囲選択開始座標
	VEC3 m_DragEnd;		//	範囲選択終了座標
public:
	CStructEditMode();
	~CStructEditMode();
	void EnterCursorScenery();
	virtual void EnterStructEdit(){}
	void ScanInputCursorScenery();
	virtual void ScanInputStructEdit(int, VEC3, VEC3);
	virtual void Delete();
	void RenderCursorScenery();
};

//	外部グローバル
extern CStructEditMode *g_StructEditMode;

#endif
