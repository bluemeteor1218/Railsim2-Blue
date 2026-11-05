#ifndef CEXITMODE_H_INCLUDED
#define CEXITMODE_H_INCLUDED

#include "CInterfaceMode.h"

class CYesNoDialog;

/*
 *	終了モード
 */
class CExitMode: public CInterfaceMode{
private:
	CYesNoDialog *m_Dialog;	//	ダイアログ
public:
	CExitMode();
	~CExitMode(){}
	void EnterInterface();
	void ModalFuncInterface();
	void ScanInputInterface(){}
};

//	外部グローバル
extern CExitMode *g_ExitMode;

#endif
