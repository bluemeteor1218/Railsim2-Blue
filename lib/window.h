//	Copyright (c) 2002 Midikyou

#define CLASSNAME		"RAILSIM2_WINDOW_CLASS"
#define WINDOWNAME		"RailSim II"
#define WM_GRAPHNOTIFY WM_APP+1	//	for DirectShow

struct SYSVALUE_W{
	HWND hWnd;	//	メイン・ウインドウのハンドル
	BOOL fActive;	//	メイン・ウインドウのアクティブフラグ
	int winW;	//	クライアント領域の横幅（sv3.widthとは必ずしも一致しない）
	int winH;	//	クライアント領域の縦幅（sv3.heightとは必ずしも一致しない）
};
extern SYSVALUE_W svw;

BOOL CreateMainWindow(HINSTANCE hInst);
void AdjustWindow();
BOOL PeekAllMessage();
LRESULT WINAPI MessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void OnActivateApp(WPARAM wParam);
void OnSize(WPARAM wParam);
void OnMove(WPARAM wParam);
void OnPaint(WPARAM wParam);
void OnChar(WPARAM wParam);	//	input.cpp
void OnGrapNotify();	//	movie.cpp

void SetCaption(char *str);
void SendWM_CLOSE();

/*
 *	アクティブ状態の判定
 */
inline BOOL IsActive(){
	return svw.fActive;
}
