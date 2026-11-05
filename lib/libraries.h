//	Copyright (c) 2002 Midikyou

#ifndef LIBLARIES_H
#define LIBLARIES_H

//	BC++用（動作未保証）
#if defined(__BORLANDC__) && defined (__WIN32__)
	#pragma link "dxguid.lib"
	#pragma link "d3d8.lib"
	#pragma link "d3dx8.lib"
	#pragma link "d3dxof.lib"
	#pragma link "dinput8.lib"
	#pragma link "c_dinput.lib"
	#pragma link "dsound.lib"
	//#pragma link "strmiids.lib"
	//#pragma link "winmm.lib"	//	不要
	#pragma link "imm32.lib"

	int _matherr(struct _exception *e){e; return 1;}

//	VC++用
#else
	#pragma comment(lib, "dxguid.lib")
	#pragma comment(lib, "d3d8.lib")
	#pragma comment(lib, "d3dx8.lib")
	#pragma comment(lib, "d3dxof.lib")
	#pragma comment(lib, "dinput8.lib")
	#pragma comment(lib, "dsound.lib")
	//#pragma comment(lib, "strmiids.lib")
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "imm32.lib")	//	IME関係
#endif

#endif LIBLARIES_H