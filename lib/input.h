//	Copyright (c) 2002 Midikyou

#include <list>

using namespace std;

//	キー／ボタンの状態
#define S_FREE		0	//	離されている：b'00
#define S_PULL		1	//	離された	：b'01
#define S_PUSH		2	//	押された	：b'10
#define S_HOLD		3	//	押されている：b'11

//	マウスボタン
#define DIM_LEFT	0	//	左ボタン
#define DIM_RIGHT	1	//	右ボタン
#define DIM_MIDDLE	2	//	中央ボタン（ホイール）

#define MAX_JOYSTICK	8	//	最大ジョイスティック数
#define MAX_BUTTON		8	//	最大ボタン数

//	ジョイスティックのキー／ボタン
#define DIJ_UP		0
#define DIJ_DOWN	1
#define DIJ_LEFT	2
#define DIJ_RIGHT	3
#define DIJ_TOP		4
#define DIJ_BOTTOM	5
#define DIJ_BT1		6
#define DIJ_BT2		7
#define DIJ_BT3		8
#define DIJ_BT4		9
#define DIJ_BT5		10
#define DIJ_BT6		11
#define DIJ_BT7		12
#define DIJ_BT8		13

struct SYSVALUE_I{
	LPDIRECTINPUT8		pDI;		//	DirectPlayオブジェクト
	LPDIRECTINPUTDEVICE8 pKey;		//	キーボード・オブジェクト
	LPDIRECTINPUTDEVICE8 pMouse;	//	マウス・オブジェクト
	LPDIRECTINPUTDEVICE8 pJoy[MAX_JOYSTICK];	//	ジョイスティックオブジェクト

	BYTE key[256];		//	キーの状態
	BYTE keyPoll[256];	//	キーの状態
	BYTE keyOld[256];	//	キーの状態（旧）
	BYTE exitKey;		//	終了キー
	list<int> charKey;	//	文字コード

	//	マウス
	BYTE btn[4];		//	ボタンの状態
	BYTE btnPoll[4];	//	ボタンの状態
	BYTE btnOld[4];		//	ボタンの状態（旧）
	POINT cur;			//	カーソル座標
	LONG wheel;			//	ホイールの状態
	LONG wheelPoll;		//	ホイールの状態
	BOOL inside;		//	カーソルがウィンドウ内か

	//	ジョイスティック
	BOOL fJoy;		//	使用の有無
	int numJoy;		//	デバイス数
	BYTE joy[MAX_JOYSTICK][MAX_BUTTON+6];	//	ボタンの状態
	BYTE joyOld[MAX_JOYSTICK][MAX_BUTTON+6];//	ボタンの状態（旧）
};
extern SYSVALUE_I svi;

BOOL InitDirectInput();
void FreeDirectInput();
BOOL InitKeyboard();
void FreeKeyboard();
BOOL InitMouse();
void FreeMouse();
BOOL InitJoyStick();
void FreeJoyStick();
BOOL CALLBACK EnumJoyCallback(const DIDEVICEINSTANCE *pInst, VOID *pContext);
BOOL CALLBACK EnumAxisCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);

void ScanInputDevice();
void FlushInputDevice();
void ScanKeyboard();
POINT GetCursorPosClient();
void ScanMouse();
void ScanJoyStick();

int GetKey(int id);
int CheckKeyDown();
void SetExitKey(int id);
void FlushKey();
int DequeueChar();
inline void ClearCharQueue(){ svi.charKey.clear(); }

int GetButton(int id);
int GetButton(int id);
POINT GetCursorXY();
int GetCursorX();
int GetCursorY();
BOOL IsCursorInside();
LONG GetWheel();
void SetCursor(int x, int y);

void EnableJoyStick(BOOL f);
int GetJoy(int n, int id);

void MsgBox(char *msg);
int MsgYesNo(char *msg);
