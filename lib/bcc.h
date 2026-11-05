//	Copyright (c) 2002 Midikyou

//	警告抑制
#pragma warn -rch	//	実行されないコード
#pragma warn -par	//	パラメータ '？' は一度も使用されていない
#pragma warn -aus	//	識別子に代入された値は使われていない
#pragma warn -csu	//	符号付き値と符号なし値の比較

//	未定義の回避
inline float sqrtf(float x){return (float)sqrt((double)x);}

inline float atanf(float x){return (float)atan((double)x);}

#define __isascii(x) isascii(x)
#define min(a, b) ((a)<=(b)) ? (a) : (b)
#define max(a, b) ((a)>=(b)) ? (a) : (b)
