//	Copyright (c) 2002 Midikyou

#include <io.h>
#include <direct.h>
#include <windows.h>
#include <mmsystem.h>	//	timeGetTime()
#include <imm.h>			//	IME関連
#include <stdio.h>
#include <math.h>
#include <string>		//	for std::string
#include <mbstring.h>	//	文字判別
#include <time.h>		//	time()

#include <process.h>

#if defined(__BORLANDC__)	//	for BC++
	#include "bcc.h"
#endif

#if defined(_MSC_VER) && (_MSC_VER<1200)	//	for VC++6.0未満
	//	dinput.hの'UINT_PTR'未定義エラーが回避できるはず・・・（謎）
	#ifdef _WIN64
	typedef unsigned __int64 UINT_PTR, *PUINT_PTR;
	#else
	typedef unsigned long UINT_PTR, *PUINT_PTR;
	#endif
#endif

#ifndef __int3264
typedef unsigned long *DWORD_PTR;
#endif

#ifndef DIRECTINPUT_VERSION
	#define DIRECTINPUT_VERSION 0x0800
#endif

#include <d3d8.h>
#include <d3dx8.h>
#include <dinput.h>
#include <dmusicc.h>
#include <dmusici.h>
//#include <dshow.h>
#include <dxfile.h>

/*
 *	型名変更
 */
typedef D3DXVECTOR2		VEC2;
typedef D3DXVECTOR3		VEC3;
typedef D3DXVECTOR4		VEC4;
typedef D3DXMATRIX		MTX4;
typedef D3DMATERIAL8	MAT8;
typedef D3DXQUATERNION	QUAT;

typedef LPDIRECT3DTEXTURE8 LPTEX8;
typedef LPDIRECT3DSURFACE8 LPSURF8;

typedef LPDIRECTSOUNDBUFFER		LPSNDBUF;
typedef LPDIRECTSOUNDBUFFER8	LPSNDBUF8;
typedef LPDIRECTSOUND3DBUFFER	LP3DBUF;
typedef LPDIRECTSOUND3DLISTENER	LP3DLISTENER;
typedef LPDIRECTSOUNDNOTIFY		LPSNDNOTIFY;
