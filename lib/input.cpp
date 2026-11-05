//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "graphic.h"
#include "input.h"
#include "wave_stream.h"

unsigned int __stdcall InputPollingThread(void* lpThreadParameter);
void InputPollOnce();

CCrtThread g_InputPollingThread;

bool g_FinishInputThread = false;
int g_InputPollCount = 0;
CRITICAL_SECTION g_InputPollCriticalSection;

/*
 *	DirectInputの初期化
 */
BOOL InitDirectInput(){
	DebugHL();
	Debug("InitDirectInput\n");

	FAILED_ASSERT(
		"DirectInputが初期化できませんでした.",
		DirectInput8Create(
			GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8,
			(void **)&svi.pDI, NULL
		));
	if(!InitKeyboard()) return FALSE;
	if(!InitMouse()) return FALSE;
	if(!InitJoyStick()) return FALSE;
	FlushInputDevice();
	InitializeCriticalSection(&g_InputPollCriticalSection);
	g_InputPollingThread.begin(InputPollingThread, NULL);
	//SetThreadPriority(g_InputPollingThread.getHandle(), THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(g_InputPollingThread.getHandle(), THREAD_PRIORITY_HIGHEST);

	return TRUE;
}

/*
 *	DirectInputの解放
 */
void FreeDirectInput(){
	if(!svi.pDI) return;

	DebugHL();
	Debug("FreeInput\n");

	g_FinishInputThread = true;
	g_InputPollingThread.end();
	DeleteCriticalSection(&g_InputPollCriticalSection);
	FreeJoyStick();
	FreeMouse();
	FreeKeyboard();

	RELEASE(svi.pDI);
}

/*
 *	キーボードの初期化
 */
BOOL InitKeyboard(){
	//	デバイスを作成
	FAILED_ASSERT(
		"キーボードデバイスが作成できません.",
		svi.pDI->CreateDevice(GUID_SysKeyboard, &svi.pKey, NULL));

	//	入力データフォーマットを指定
	FAILED_ASSERT(
		"キーボードフォーマットが設定できません.",
		svi.pKey->SetDataFormat(&c_dfDIKeyboard));

	//	協調レベルを指定
	FAILED_ASSERT(
		"キーボードの協調レベルが設定できません.",
		svi.pKey->SetCooperativeLevel(svw.hWnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE));

	//	アクセス権を得る
	svi.pKey->Acquire();

	//	終了キーの設定
	//SetExitKey(DIK_F9);

	return TRUE;
}

/*
 *	キーボードの解放
 */
void FreeKeyboard(){
	if(svi.pKey) svi.pKey->Unacquire();
	RELEASE(svi.pKey);
}

/*
 *	マウスの初期化
 */
BOOL InitMouse(){
	FAILED_ASSERT(
		"マウスデバイスが作成できません.",
		svi.pDI->CreateDevice(GUID_SysMouse, &svi.pMouse, NULL));

	FAILED_ASSERT(
		"キーボードフォーマットが設定できません.",
		svi.pMouse->SetDataFormat(&c_dfDIMouse));

	FAILED_ASSERT(
		"マウスの協調レベルが設定できません.",
		svi.pMouse->SetCooperativeLevel(svw.hWnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE));

	svi.pMouse->Acquire();

	//	フルスクリーン時はカーソルを非表示
	if(!sv3.fWindowed){
		ShowCursor(FALSE);

		//	カーソルをセンタリング
		SetCursor(svw.winW/2, svw.winH/2);
	}

	return TRUE;
}

/*
 *	マウスの解放
 */
void FreeMouse(){
	if(svi.pMouse) svi.pMouse->Unacquire();
	RELEASE(svi.pMouse);
}

/*
 *	ジョイスティックの初期化
 */
BOOL InitJoyStick(){
	//	デバイスの列挙
	svi.numJoy = 0;

	svi.pDI->EnumDevices(
		DI8DEVCLASS_GAMECTRL, EnumJoyCallback, NULL, DIEDFL_ATTACHEDONLY);

	if(svi.numJoy==0) return TRUE;	//	無いなら仕方が無い
	else Debug("ジョイスティック数 = %d\n", svi.numJoy);

	HRESULT hr;

	for(int i = 0; i<svi.numJoy; i++){
		hr = svi.pJoy[i]->SetDataFormat(&c_dfDIJoystick2);

		if(hr!=DI_OK){
			Debug("ジョイスティック[%d]のフォーマットが指定できません.\n", i);
			continue;
		}
		hr = svi.pJoy[i]->SetCooperativeLevel(
			svw.hWnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);

		if(hr!=DI_OK){
			Debug("ジョイスティック[%d]の協調レベルが設定できません.\n", i);
			continue;
		}

		//	可動範囲の指定
		hr = svi.pJoy[i]->EnumObjects(EnumAxisCallback, (VOID *)i, DIDFT_AXIS);

		if(hr!=DI_OK){
			Debug("ジョイスティック[%d]の軸の列挙ができません.\n", i);
			continue;
		}

		svi.pJoy[i]->Acquire();
	}
	return TRUE;
}

/*
 *	ジョイスティックの解放
 */
void FreeJoyStick(){
	for(int i = 0; i<svi.numJoy; i++){
		if(svi.pJoy[i]) svi.pJoy[i]->Unacquire();
		RELEASE(svi.pJoy[i]);
	}
}

/*
 *	ジョイスティック列挙時のコールバック
 */
BOOL CALLBACK EnumJoyCallback(const DIDEVICEINSTANCE *pInst, VOID *pContext){
	HRESULT hr = svi.pDI->CreateDevice(
		pInst->guidInstance, &svi.pJoy[svi.numJoy], NULL);

	if(hr!=DI_OK){
		Debug("ジョイスティック[%d]が初期化できません.\n", svi.numJoy);
	}
	if(++svi.numJoy==MAX_JOYSTICK) return DIENUM_STOP;

	return DIENUM_CONTINUE;
}

/*
 *	ジョイスティック軸列挙時のコールバック
 */
BOOL CALLBACK EnumAxisCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef){
	int i = (int)pvRef;

	DIPROPRANGE diprg;

	diprg.diph.dwSize = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow = DIPH_BYID;
	diprg.diph.dwObj = lpddoi->dwType;
	diprg.lMin = -1000;
	diprg.lMax = +1000;

	svi.pJoy[i]->SetProperty(DIPROP_RANGE, &diprg.diph);

	return DIENUM_CONTINUE;
}

/*
 *	入力デバイスのスキャン
 */
void ScanInputDevice(){
	EnterCriticalSection(&g_InputPollCriticalSection);
	
	if(!g_InputPollCount){
		InputPollOnce();
		//static int once = 0;
		//char *FlashIn(char *, ...);
		//SetWindowText(svw.hWnd, FlashIn("input poll skip %d", ++once));
	}
	g_InputPollCount = 0;

	ScanKeyboard();
	ScanMouse();
	ScanJoyStick();

	//	終了キー判定
	//if(svi.key[svi.exitKey]&0x80) SendWM_CLOSE();

	LeaveCriticalSection(&g_InputPollCriticalSection);
}

/*
 *	入力バッファをクリア
 */
void FlushInputDevice(){
	memset(svi.key, 0, sizeof(svi.key));
	memset(svi.keyPoll, 0, sizeof(svi.keyPoll));
	memset(svi.keyOld, 0, sizeof(svi.keyOld));

	memset(svi.btn, 0, sizeof(svi.btn));
	memset(svi.btnPoll, 0, sizeof(svi.btnPoll));
	memset(svi.btnOld, 0, sizeof(svi.btnOld));

	memset(svi.joy, 0, sizeof(svi.joy));
	memset(svi.joyOld, 0, sizeof(svi.joyOld));

	svi.wheel = svi.wheelPoll = 0;
}

void InputPollOnce(){
	int i;
	BYTE keyTemp[256];
	HRESULT hr;

	//	デバイスの状態を得る
	hr = svi.pKey->GetDeviceState(sizeof(keyTemp), &keyTemp);
	if(FAILED(hr)){
		memset(svi.key, 0, 256);	//	クリア
		//	アクセス権を再取得
		if(hr==DIERR_INPUTLOST) {
			Debug("キーボードのアクセス権を失いました.\n");
			svi.pKey->Acquire();
		}
	}else{
		for(i = 0; i<256; ++i) svi.keyPoll[i] |= keyTemp[i];
	}

	//	クリックの検出
	DIMOUSESTATE ms;
	hr = svi.pMouse->GetDeviceState(sizeof(ms), &ms);
	if(hr==DIERR_INPUTLOST){
		Debug("マウスのアクセス権を失いました.\n");
		svi.pMouse->Acquire();
	}else{
		svi.wheelPoll += ms.lZ;
		for(i = 0; i<4; ++i) svi.btnPoll[i] |= ms.rgbButtons[i];
	}
	++g_InputPollCount;
}

/*
 *	DirectInputの初期化
 */
unsigned int __stdcall InputPollingThread(void* lpThreadParameter){
	while(!g_FinishInputThread){
		while(!svw.fActive) Sleep(100);
		Sleep(10);
		EnterCriticalSection(&g_InputPollCriticalSection);
		InputPollOnce();
		LeaveCriticalSection(&g_InputPollCriticalSection);
	}
	//Dialog("manual finish");
	return 0;
}

/*
 *	キーボードのスキャン
 */
void ScanKeyboard(){
	//	直前の状態を保存
	memcpy(svi.keyOld, svi.key, 256);
	memcpy(svi.key, svi.keyPoll, 256);
	memset(svi.keyPoll, 0, sizeof(svi.key));
}

POINT GetCursorPosClient(){
	POINT cur;
	GetCursorPos(&cur);
	//	ウインドウモード時は座標系を変換
	if(sv3.fWindowed) ScreenToClient(svw.hWnd, &cur);
	return cur;
}

/*
 *	マウスのスキャン
 */
void ScanMouse(){
	//	カーソル位置の検出

	//	DirectInputは座標のマッピングがうまくできないため、
	//	カーソル位置の検出には Win32 API を使用する。
	svi.cur = GetCursorPosClient();

	//	ウインドウ内判定
	if(sv3.fWindowed){
		svi.inside = 0<=svi.cur.x && svi.cur.x<svw.winW
			&& 0<=svi.cur.y && svi.cur.y<svw.winH ? TRUE : FALSE;
	}else{
		svi.inside = TRUE;
	}

	//	直前の状態を保存
	memcpy(svi.btnOld, svi.btn, 4);
	memcpy(svi.btn, svi.btnPoll, 4);
	memset(svi.btnPoll, 0, sizeof(svi.btnPoll));

	svi.wheel = svi.wheelPoll;
	svi.wheelPoll = 0;
}

/*
 *	ジョイスティックのスキャン
 */
void ScanJoyStick(){
	if(!svi.fJoy) return;

	//	直前の状態を保存
	memcpy(svi.joyOld, svi.joy, MAX_JOYSTICK*(MAX_BUTTON+6));

	HRESULT hr;
	DIJOYSTATE2 js;

	for(int i = 0; i<svi.numJoy; i++){
		hr = svi.pJoy[i]->Poll();

		if(hr==DIERR_INPUTLOST){
			Debug("ジョイスティック[%d]のアクセス権を失いました.\n", i);
			svi.pMouse->Acquire();
			continue;
		}

		hr = svi.pJoy[i]->GetDeviceState(sizeof(DIJOYSTATE2), &js);

		if(hr!=DI_OK){
			Debug("ジョイスティック[%d]の状態が取得できません.\n", i);
			continue;
		}

		memset(&svi.joy[i][0], 0, 6);

		if(js.lY<-500)		svi.joy[i][DIJ_UP	] = 0x80;
		else if(js.lY>500)	svi.joy[i][DIJ_DOWN ] = 0x80;

		if(js.lX<-500)		svi.joy[i][DIJ_LEFT ] = 0x80;
		else if(js.lX>500)	svi.joy[i][DIJ_RIGHT ] = 0x80;

		if(js.lZ<-500)		svi.joy[i][DIJ_TOP ] = 0x80;
		else if(js.lZ>500)	svi.joy[i][DIJ_BOTTOM] = 0x80;

		memcpy(&svi.joy[i][6], js.rgbButtons, MAX_BUTTON);
	}
}

/*
 *	キーの状態取得
 *
 *	id	: DIK_UP, DIK_SPACE など
 */
int GetKey(int id){
	return ((svi.key[id]&0x80)>>6)|((svi.keyOld[id]&0x80)>>7);
}

/*
 *	どれかキーが押された（瞬間）かを判定
 *	押された瞬間がなければ押されているものを返す
 */
int CheckKeyDown(){
	int i, s, t = -1;
	for(i = 0; i<256; i++){
		//	最初に検出したキーを返す
		s = GetKey(i);
		if(s==S_PUSH) return i;
		if(t<0 && s>S_PUSH) t = i;
	}
	return t;
}

/*
 *	キーバッファのクリア
 */
void FlushKey(){
	memset(svi.key, 0, sizeof(svi.key));
}

/*
 *	押された文字キーを取得
 */
int DequeueChar(){
	if(!svi.charKey.size()) return 0;
	int key = *svi.charKey.begin();
	svi.charKey.pop_front();
	return key;
}

/*
 *	WM_CHARメッセージへのハンドラ
 */
void OnChar(WPARAM wParam){
	svi.charKey.push_back((TCHAR)wParam);
}

/*
 *	終了キーの設定
 *
 *	id	: キーID
 */
void SetExitKey(int id){
	svi.exitKey = id;
}

/*
 *	マウスボタンの状態取得
 *
 *	id	: DIM_LEFT, DIM_RIGHT, DIM_MIDDLE
 */
int GetButton(int id){
	return ((svi.btn[id]&0x80)>>6)|((svi.btnOld[id]&0x80)>>7);
}

/*
 *	カーソル座標の取得
 */
POINT GetCursorXY(){
	return svi.cur;
}

/*
 *	カーソルX座標の取得
 */
int GetCursorX(){
	return svi.cur.x;
}

/*
 *	カーソルY座標の取得
 */
int GetCursorY(){
	return svi.cur.y;
}

/*
 *	カーソルがウィンドウ内か調べる
 */
BOOL IsCursorInside(){
	return svi.inside;
}

/*
 *	ホイール位置の取得
 */
LONG GetWheel(){
	return svi.wheel;
}

/*
 *	カーソル座標の設定
 *
 *	x, y	：ウインドウ座標
 */
void SetCursor(int x, int y){
	POINT cur = {x, y};

	//	ウインドウモード時は座標系を変換
	if(sv3.fWindowed) ClientToScreen(svw.hWnd, &cur);

	SetCursorPos(cur.x, cur.y);
}

/*
 *	ジョイスティック使用の有無を指定
 *
 *	f		: TRUE＝有効にする、FALSE＝無効にする
 */
void EnableJoyStick(BOOL f){
	svi.fJoy = f;

	if(!f) memset(svi.joy, FALSE, sizeof(svi.joy));
}

/*
 *	ジョイスティックボタンの状態取得
 *
 *	n		: ジョイスティックID
 *	id	: DIJ_UP, DIJ_BT1 など
 */
int GetJoy(int n, int id){
	return ((svi.joy[n][id]&0x80)>>6)|((svi.joyOld[n][id]&0x80)>>7);
}

/*
 *	メッセージボックス表示
 */
void MsgBox(char *msg){
	MessageBox(svw.hWnd, msg, "", MB_OK);
	FlushInputDevice();
}

/*
 *	メッセージボックス表示(Yes/No)
 *
 *	msg	: 文字列
 */
int MsgYesNo(char *msg){
	int ret = MessageBox(
		svw.hWnd, msg, "", MB_YESNO|MB_SYSTEMMODAL|MB_ICONQUESTION);
	FlushInputDevice();
	return ret==IDYES;
}
