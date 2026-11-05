//	Copyright (c) 2002 Midikyou

#define DEBUG_FILE "debug.txt"

extern char g_errMsg[];
extern char g_debugDest[];

BOOL InitDebugStream();
void GetAppPath(char *path);
void Debug(LPCTSTR szDebug, ...);
void DebugHL();
BOOL CheckArguments(LPCSTR str);
void DumpSysInfo();

/*
 *	条件式が偽なら警告ウインドウを表示してFALSEを返す
 *
 *	msg	: メッセージ
 *	b		: 条件式
 *
 *	※DirectXのエラーに限定するならDXTrace()を使いましょう。
 */
#define ASSERT(msg, b) \
if(!(b)) {\
	MessageBox(GetActiveWindow(), msg, "エラー", MB_SYSTEMMODAL); \
	return FALSE; \
}

/*
 *	関数の戻り値を判定するASSERT()
 *
 *	caption	: タイトル
 *	func		: 判定する関数
 */
#define FAILED_ASSERT(caption, func) ASSERT(caption, !FAILED(func))

/*
 *	DirectXオブジェクトの安全な解放
 */
#define RELEASE(x) \
if(x){ \
	(x)->Release(); \
	(x) = NULL; \
}

/*
 *	作業ディレクトリの変更
 */
#define CHANGE_DIR(dir) SetCurrentDirectory(dir)

//	関数宣言
void Dialog(char *, ...);
void ErrorDialog(char *, ...);
