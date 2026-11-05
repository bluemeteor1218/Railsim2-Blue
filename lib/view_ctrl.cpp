//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "frame.h"
#include "window.h"
#include "graphic.h"
#include "render.h"
#include "vertex.h"
#include "draw.h"
#include "texture.h"
#include "font.h"
#include "input.h"
#include "view.h"
#include "view_ctrl.h"

/*
 *	カメラ（ビュー）をキーボードのみで操作する
 *
 *	r		: 感度
 */
void KeyLook(float r){
	float t = r/10*(float)MAXFPS/GetFPS();
	if(GetKey(DIK_UP) >=S_PUSH) RotVX(-t);
	if(GetKey(DIK_DOWN)>=S_PUSH) RotVX(t);
	if(GetKey(DIK_LEFT)>=S_PUSH) RotVY(-t);
	if(GetKey(DIK_RIGHT)>=S_PUSH) RotVY(t);

	VEC3 v(0, 0, 0);

	//	[W]：前進、[S]：後退、[D]：右移動、[A]：左移動
	if(GetKey(DIK_W)>=S_PUSH) v.z = r;
	if(GetKey(DIK_S)>=S_PUSH) v.z = -r;
	if(GetKey(DIK_D)>=S_PUSH) v.x = r;
	if(GetKey(DIK_A)>=S_PUSH) v.x = -r;

	//	斜め移動の速度調節
	if(v.z!=0 && v.x!=0)
		v.z /= SQRT2, v.x /= SQRT2;

	float fps = GetFPS();
	if(fps>=1) MoveV((float)MAXFPS/fps*v);
}

/*
 *	カメラ（ビュー）をマウスとキーボードで操作する
 *
 *	r		:感度
 */
void MouseLook(float r){
	//	カーソル位置の変化分だけ回転
	static POINT p, op = {svw.winW/2, svw.winH/2};
	static float dx = svw.winW/(D3DX_PI/3);
	static float dy = svw.winH/(D3DX_PI/2);

	p = GetCursorXY();
	RotVUp((op.x-p.x)/dx);
	RotVX((p.y-op.y)/dy);

	//	カーソル水平位置が一定値に達すると中央にリセット（360度以上の回転を可能にする）
	if(p.x<svw.winW*0.25 || p.x>svw.winW*0.75){
		p.x = svw.winW/2;
		SetCursor(p.x, p.y);
	}
	op = p;

	VEC3 v(0, 0, 0);

	//	[W]：前進、[S]：後退、[D]：右移動、[A]：左移動
	if(GetKey(DIK_W)>=S_PUSH) v.z = r;
	if(GetKey(DIK_S)>=S_PUSH) v.z = -r;
	if(GetKey(DIK_D)>=S_PUSH) v.x = r;
	if(GetKey(DIK_A)>=S_PUSH) v.x = -r;

	//	斜め移動の速度調節
	if(v.z!=0 && v.x!=0)
		v.z /= SQRT2, v.x /= SQRT2;

	float fps = GetFPS();
	if(fps>=1) MoveV((float)MAXFPS/fps*v);
}

/*
 *	FPSを表示する
 *
 *	x, y	: 位置
 */
void ShowFPS(int x, int y){
	char msg[80];

	sprintf(msg, "%.1f", GetFPS());
	Text(x, y, 0xffffffff, msg);
}

/*
 *	FPSや視点、注視点を表示する
 *
 *	x, y	: 位置
 */
void ShowCameraInfo(int x, int y){
	char msg[80];

	VEC3 v = GetVPos();
	VEC3 d = GetVDir();

	sprintf(msg, "%.1f (%.1f, %.1f, %.1f)->(%.1f, %.1f, %.1f)",
		GetFPS(), v.x, v.y, v.z, d.x, d.y, d.z);

	Text(x, y, 0xffffffff, msg);
}

/*
 *	キーボード情報を表示
 *
 *	x, y	: 位置
 */
void ShowKeyboardInfo(int x, int y){
	char msg[256];

	wsprintf(
		msg, "Up = %d Down = %d Left = %d Right = %d Z = %d X = %d C = %d",
		GetKey(DIK_UP), GetKey(DIK_DOWN), GetKey(DIK_LEFT), GetKey(DIK_RIGHT),
		GetKey(DIK_Z), GetKey(DIK_X), GetKey(DIK_C));
	Text(x, y, 0xffffffff, msg);
}

/*
 *	マウス情報を表示
 *
 *	x, y	: 位置
 */
void ShowMouseInfo(int x, int y){
	char msg[256];

	wsprintf(
		msg, "X = %d Y = %d Left = %d Right = %d Middle = %d Wheel = %d",
		GetCursorX(), GetCursorY(),
		GetButton(DIM_LEFT), GetButton(DIM_RIGHT), GetButton(DIM_MIDDLE),
		GetWheel());
	Text(x, y, 0xffffffff, msg);
}

/*
 *	ジョイスティック情報を表示
 *
 *	x, y	: 位置
 */
void ShowJoyStickInfo(int x, int y){
	char msg[256];

	sprintf(
		msg,
		"Up = %d Down = %d Left = %d Right = %d\n",
		GetJoy(0, DIJ_UP), GetJoy(0, DIJ_DOWN),
		GetJoy(0, DIJ_LEFT), GetJoy(0, DIJ_RIGHT));

	char tmp[8];

	for(int i = 0; i<MAX_BUTTON; i++){
		sprintf(tmp, "B%d = %d ", i, GetJoy(0, i+6));
		strcat(msg, tmp);
	}
	strcat(msg, "\n");

	Text(x, y, 0xffffffff, msg);
}