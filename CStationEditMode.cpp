#include "stdafx.h"
#include "CScene.h"
#include "CStationEditMode.h"

/*
 *	モードを有効化
 */
void CStationEditMode::EnterStructEdit(){
	ms_ModeLabel = lang(EditStation);
}

/*
 *	入力チェック
 */
void CStationEditMode::ScanInputStructEdit(
	int mode,	//	モード
	VEC3 rect1,	//	領域始点
	VEC3 rect2	//	領域終点
){
	g_Scene->ScanInputStation(mode, rect1, rect2, true);
}

/*
 *	撤去
 */
void CStationEditMode::Delete(){
	g_Scene->DeleteStation(NULL);
}
