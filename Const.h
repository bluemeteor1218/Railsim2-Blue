#ifndef CONST_H_INCLUDED
#define CONST_H_INCLUDED

extern const float RAILSIM_VERSION;
extern const char* VERSION_STRING;

const int FONT_WIDTH = 6;				//	標準フォント幅
const int FONT_HEIGHT = 12;				//	標準フォント高
const int MODE_NUM = 6;					//	モード数
const int REPEAT_FRAME = MAXFPS/4;		//	長押フレーム数
const int DBLCLK_FRAME = MAXFPS/2;		//	ダブルクリックフレーム数
const int DETECT_2D_MIN = 5;			//	2D 検出最小距離
const int DETECT_2D_MAX = 50;			//	2D 検出最大距離

extern char *YESNO[];

//	エイリアスを渡せないので const VEC3 ではなくマクロにする
#define V2ZERO	VEC2(0.0f, 0.0f)		//	ゼロベクトル
#define V3ZERO	VEC3(0.0f, 0.0f, 0.0f)	//	ゼロベクトル
#define V3RIGHT	VEC3(1.0f, 0.0f, 0.0f)	//	x 単位ベクトル
#define V3UP	VEC3(0.0f, 1.0f, 0.0f)	//	y 単位ベクトル
#define V3DIR	VEC3(0.0f, 0.0f, 1.0f)	//	z 単位ベクトル

#endif
