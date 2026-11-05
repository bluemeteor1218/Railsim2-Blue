//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"

/*
 *	デバッグ出力の初期化
 */
BOOL InitDebugStream(){
	if(CheckArguments("-dbf")){
		GetAppPath(g_debugDest);
		strcat(g_debugDest, DEBUG_FILE);
	}
	DebugHL();
	Debug("InitDebugStream\n");
	DumpSysInfo();
	return TRUE;
}

/*
 *	アプリケーションのパスを取得
 */
/*	void GetAppPath(char *path){
	GetModuleFileName(GetModuleHandle(NULL), path, _MAX_PATH);

	for(int i = strlen(path); (i>=0 && path[i]!='\\'); i--)
		path[i] = '\0';
}*/

/*
 *	デバッグ出力
 *
 *	szDebug	: 出力書式
 *	...		: 可変引数
 */
void Debug(LPCTSTR szDebug, ...){
	char	szBuffer[256] = "";
	va_list	vl;

	va_start(vl, szDebug);
	vsprintf(szBuffer, szDebug, vl);
	va_end(vl);

	if(g_debugDest[0]){
		FILE *fp = fopen(g_debugDest, "a");
		fprintf(fp, szBuffer);
		fclose(fp);
	}else{
		OutputDebugString(szBuffer);
	}
}

/*
 *	デバッグ出力（水平線を引く）
 */
#define _H_LINE	"----------------------------------------\n"

void DebugHL(){
	if(g_debugDest[0]){
		FILE *fp = fopen(g_debugDest, "a");
		fprintf(fp, _H_LINE);
		fclose(fp);
	}else{
		OutputDebugString(_H_LINE);
	}
}

/*
 *	実行時引数のチェック
 */
BOOL CheckArguments(LPCSTR str){
	int i;
	for(i = 1; i<__argc; i++) if(strcmpi(__argv[i], str)==0) return TRUE;
	return FALSE;
}

/*
 *	システム情報のダンプ
 */
void DumpSysInfo(){
	//	日時の取得
	SYSTEMTIME stm;

	GetLocalTime(&stm);
	Debug("%4d/%02d/%02d %02d:%02d\n",
		stm.wYear, stm.wMonth, stm.wDay, stm.wHour, stm.wMinute);

	//	OSの取得
	OSVERSIONINFO ver = {0};

	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);

	Debug("OS = ID %d ver, %d.%d build %d\n",
		ver.dwPlatformId, ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber);

	//	メモリーサイズの取得
	MEMORYSTATUS mems;
	mems.dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus(&mems);

	Debug("利用可能／搭載メモリ = %d/%d MB\n",
		mems.dwAvailPhys>>20, mems.dwTotalPhys>>20);
}