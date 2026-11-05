#ifndef CSTRUCTBUILDMODE_H_INCLUDED
#define CSTRUCTBUILDMODE_H_INCLUDED

#include "CSceneryMode.h"

/*
 *	施設設置モード
 */
class CStructBuildMode: public CArrowSceneryMode{
protected:
	static CWindowCtrl ms_StructWindow;	//	施設設定窓
	static CStaticCtrl ms_DirLabel;		//	方向ラベル
	static CRadioButton ms_Dir[];		//	方向ラジオ
	static CCheckBox ms_FitNormal;		//	地形に合わせて傾斜
	int m_BuildAngle;	//	設置角度
	bool m_HitFlag;		//	判定フラグ
	VEC3 m_Pos;			//	座標
	VEC3 m_HitPos;		//	判定座標
	VEC3 m_HitNorm;		//	判定法線
public:
	static void InitInterface();
	static int GetAngleSplit();
	CStructBuildMode();
	~CStructBuildMode();
	void GetBuildDir(VEC3 *, VEC3 *);
	void EnterArrowScenery();
	virtual void EnterStructBuild();
	void ScanInputArrowScenery();
	virtual void Build();
	void RenderArrowScenery();
	virtual void RenderStructBuild();
};

//	外部グローバル
extern CStructBuildMode *g_StructBuildMode;

#endif
