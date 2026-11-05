#include <time.h>
#include <windows.h>

LONGLONG g_HighTimerFreq = 0;
double g_HighTimer_FromCountToMilliseconds = 0.0;

/*
 *	高分解能タイマの初期化
 *
 *	戻り値: CPU が対応していれば true を返す
 */
bool InitHighTimer(){
	if(QueryPerformanceFrequency((LARGE_INTEGER *)(&g_HighTimerFreq))){
		g_HighTimer_FromCountToMilliseconds = 1000.0/g_HighTimerFreq;
		return true;
	}else{
		g_HighTimerFreq = 0;
		g_HighTimer_FromCountToMilliseconds = 1.0;
		return false;
	}
}

/*
 *	現在時刻の取得
 *
 *	戻り値: 内部カウント値
 */
LONGLONG HighTimer(){
	if(g_HighTimerFreq){
		LONGLONG cnt;
		QueryPerformanceCounter((LARGE_INTEGER *)(&cnt));
		return cnt;
	}else{
		return clock();
	}
}

/*
 *	ミリ秒に変換
 *
 *	戻り値: ミリ秒
 */
double FromHighTimerCountToMs(LONGLONG t){
	return t*g_HighTimer_FromCountToMilliseconds;
}
