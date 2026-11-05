#include "stdafx.h"

#ifdef _MSC_VER
// メモリリーク検出
#include <crtdbg.h>
#endif // #ifdef _MSC_VER

#include "HighTimer.h"
#include "CGameMode.h"

//	内部グローバル
char g_BaseDir[1024];			//	基準ディレクトリ
char *g_PluginViewArg = NULL;	//	プラグインビューア用コマンドライン引数

/*
 *	初期化の初期化
 */
void WakeUp(){
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(50751);
#endif // #ifdef _MSC_VER
	GetAppPath(g_BaseDir);
	InitHighTimer();
	int i;
	for(i = 1; i<__argc; i++){
		char *cparam = __argv[i];
		int cplen = strlen(cparam);
		if(cplen<8) continue;
		if(cparam[1]!=':' || cparam[2]!='\\') continue;
		if(strcmpi(cparam+cplen-4, ".txt")) continue;
		g_PluginViewArg = cparam;
		break;
	}
	CGameMode::WakeUp();
}
