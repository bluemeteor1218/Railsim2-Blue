#ifndef CSTATIONEDITMODE_H_INCLUDED
#define CSTATIONEDITMODE_H_INCLUDED

#include "CStructEditMode.h"

/*
 *	駅舎編集モード
 */
class CStationEditMode: public CStructEditMode{
private:
public:
	CStationEditMode(){}
	~CStationEditMode(){}
	void EnterStructEdit();
	void ScanInputStructEdit(int, VEC3, VEC3);
	void Delete();
};

//	外部グローバル
extern CStationEditMode *g_StationEditMode;

#endif
