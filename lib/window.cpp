//	Copyright (c) 2002 Midikyou

#include <windows.h>
#include <windowsx.h>

#include "headers.h"
#include "debug.h"
#include "graphic.h"	//	sv3.fWindowes
#include "window.h"
#include "input.h"

#if defined(__BORLANDC__)	//	for BC++
	#define IDI_ICON1 1001
#else
	#include "..\resource.h"
#endif

void AdjustMovieLayer();	//	movie.cpp

//	外部グローバル
extern int g_DispWidth;
extern int g_DispHeight;

/*
 *	メインウインドウの作成
 *
 *	hInst	: インスタンスハンドル
 */
BOOL CreateMainWindow(HINSTANCE hInst){
	//	ウインドウクラスの登録
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),	//	サイズ
		0,					//	スタイル
		MessageProc,		//	メッセージハンドラ
		0L,					//	拡張クラス
		0L,					//	拡張ウインドウ
		hInst,				//	インスタンスハンドル
		LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)),	//	アイコン指定
		LoadCursor(NULL, IDC_ARROW),			//	標準カーソル
		(HBRUSH)GetStockObject(BLACK_BRUSH),	//	ウインドウの下地を黒に
		NULL,				//	メニューなし
		CLASSNAME,			//	ウインドウクラス名
		NULL				//	スモールアイコンなし（縮小して使用）
	};
	RegisterClassEx(&wc);

	//	ウインドウの作成
	HWND hWnd = CreateWindow(
		CLASSNAME,			//	クラス名
		WINDOWNAME,			//	タイトル
		WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,	//	スタイル（仮）
		//WS_OVERLAPPEDWINDOW,	//	スタイル（仮）
		0,					//	表示位置
		0,					//	
		g_DispWidth,		//	横幅(仮)
		g_DispHeight,		//	縦幅(仮)
		GetDesktopWindow(),	//	親ウインドウをデスクトップに
		NULL,				//	メニュー（なし）
		wc.hInstance,		//	インスタンス
		NULL				//	追加パラメータ（なし）
	);
	if(!hWnd) return FALSE;

	//	グローバル変数に退避
	svw.hWnd = hWnd;
	svw.winW = g_DispWidth;
	svw.winH = g_DispHeight;

	//	ウインドウ枠等を考慮にいれて、サイズを再設定
	AdjustWindow();

	//	アクティブフラグを立てる
	svw.fActive = FALSE;

	return TRUE;
}

/*
 *	クライアント領域のサイズを基準にウインドウをリサイズ
 */
void AdjustWindow(){
	RECT r = {0, 0, svw.winW, svw.winH}, d;
	GetWindowRect(GetDesktopWindow(), &d);
	AdjustWindowRectEx(
		&r,
		GetWindowStyle(svw.hWnd),
		!!GetMenu(svw.hWnd),
		GetWindowExStyle(svw.hWnd));
	int dw = d.right-d.left, dh = d.bottom-d.top;
	int cw = r.right-r.left, ch = r.bottom-r.top;
	SetWindowPos(
		svw.hWnd, NULL, (dw-cw)/2, (dh-ch)/2, cw, ch,
		SWP_NOZORDER|SWP_NOACTIVATE);
}

/*
 *	メッセージ処理
 */
BOOL PeekAllMessage(){
	static MSG msg;

	//	空になるまでループ
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
		//	終了メッセージか？
		if(msg.message==WM_QUIT) return FALSE;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return TRUE;
}

/*
 *	ウインドウメッセージ処理
 *
 *	引数の説明を省略
 */
LRESULT WINAPI MessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
	//	アクティブ／非アクティブが切り替わった
	case WM_ACTIVATEAPP:
		OnActivateApp(wParam);
		break;

	//	サイズが変更された
	case WM_SIZE:
		OnSize(wParam);
		break;

	//	移動した
	case WM_MOVE:
		OnMove(wParam);
		break;

	//	ディスプレイモードが変更された
	case WM_DISPLAYCHANGE:
		Debug("<ディスプレイモードの変更>\n");
		break;

	//	再描画要求
	case WM_PAINT:
		OnPaint(wParam);
		break;

	//	システムメッセージ
	case WM_SYSCOMMAND:
		//	スクリーンセーバー等の発動を阻止
		if(wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) return 1;
		else return DefWindowProc(hWnd, msg, wParam, lParam);

	//	IMEウインドウが起動した
	case WM_IME_SETCONTEXT:
		//	FEP(IME等)のウインドウを強制的に閉じる
		SendMessage(ImmGetDefaultIMEWnd(hWnd), WM_CLOSE, 0, 0);
		break;

	//	文字キーが押された
	case WM_CHAR:
		OnChar(wParam);	//	input.cpp
		break;

	//	Disable Alt & F10
	case WM_SYSKEYDOWN:
		break;

	//	DirectShowメッセージ
#ifdef UDX_USE_MOVIE
	case WM_GRAPHNOTIFY:
		OnGrapNotify();	//	movie.cpp
		break;
#endif // #ifdef UDX_USE_MOVIE

	//	ウインドウが放棄されようとした
	case WM_CLOSE:
		//	※DestoryWindow()はDirect3Dの解放後に行う
		PostQuitMessage(0);
		break;

	//	残りはデフォルト処理させる
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 *	アクティブ化、非アクティブ化への応答
 */
void OnActivateApp(WPARAM wParam){
	svw.fActive = (BOOL)wParam;
	Debug(svw.fActive ? "<アクティブ>\n" : "<非アクティブ>\n");
	if(svw.fActive){
		svi.wheel = 0;
		ShowCursor(FALSE);
	}else{
		ShowCursor(TRUE);
		ClipCursor(NULL);
	}
}

/*
 *	サイズ変更への応答
 */
void OnSize(WPARAM wParam){
	//	フルスクリーンモードではサイズ変更を許さない
	if(!sv3.fWindowed) return;
	/*
	 *	※9x系でフルスクリーンモード移行直前にウインドウサイズが変更され、
	 *		ウインドウサイズが解像度以上になるのを回避　↑
	 */

	//	クライアント領域サイズの取得
	RECT rect;
	GetClientRect(svw.hWnd, &rect);

	svw.winW = rect.right-rect.left;
	svw.winH = rect.bottom-rect.top;

	Debug("<クライアント領域 %d x %d>\n", svw.winW, svw.winH);

#ifdef UDX_USE_MOVIE
	//	ムービーレイヤーのリサイズ
	AdjustMovieLayer();
#endif // #ifdef UDX_USE_MOVIE
}

/*
 *	ウインドウ移動への応答
 */
void OnMove(WPARAM wParam){
#ifdef UDX_USE_MOVIE
	//	ムービーレイヤーの移動
	AdjustMovieLayer();
#endif // #ifdef UDX_USE_MOVIE
}

/*
 *	再描画要求への応答
 */
void OnPaint(WPARAM wParam){
	//	取り合えず無効領域をクリアする
	PAINTSTRUCT ps;
	HDC hDC;

	hDC = BeginPaint(svw.hWnd, &ps);
	EndPaint(svw.hWnd, &ps);
}

/*
 *	ウインドウタイトルの設定
 *
 *	str	: 文字列
 */
void SetCaption(char *str){
	SetWindowText(svw.hWnd, str);
}

/*
 *	終了要求
 *
 *	※メッセージが処理されるまでプログラムは終了しません。
 */
void SendWM_CLOSE(){
	SendMessage(svw.hWnd, WM_CLOSE, 0, 0);
}