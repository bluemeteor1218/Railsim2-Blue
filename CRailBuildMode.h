#ifndef CRAILBUILDMODE_H_INCLUDED
#define CRAILBUILDMODE_H_INCLUDED

#include "CSceneryMode.h"
#include "CRailwayMode.h"

/*
 *	レール設置モード
 */
class CRailBuildMode: public CArrowSceneryMode, public CRailwayMode{
private:
	CRailBuilder *m_Builder;	//	レール設置子
	CRailBuilder *m_LastPos;	//	最後尾アドレス
public:
	CRailBuildMode();
	~CRailBuildMode();
	char *LoadArrowScenerySetting(char *);
	void SaveArrowScenerySetting(FILE *);
	void EnterArrowScenery();
	void ModalFuncArrowScenery();
	void ScanInputArrowScenery();
	void RenderArrowScenery();
	void ResetBuilder();
};

//	外部グローバル
extern CRailBuildMode *g_RailBuildMode;

#endif
