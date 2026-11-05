#include "stdafx.h"
#include "CPixelbit.h"

//	内部定数
const char *const DIALOG_TITLE = "RailSim II";

const int FLASHBUF_SIZE = 1024;	//	一時バッファサイズ
const int FLASHBUF_NUM = 8;		//	一時バッファ数

//	内部グローバル
char g_FlashBuf[FLASHBUF_NUM][FLASHBUF_SIZE];	//	一時バッファ

int g_FlashBufSelect = 0;	//	一時バッファ番号

/*
 *	一時バッファに書式付出力
 */
char *FlashIn(
	char *format,	//	書式
	...				//	任意パラメタ
){
	g_FlashBufSelect = (g_FlashBufSelect+1)%FLASHBUF_NUM;
	va_list	vl;
	va_start(vl, format);
	vsprintf(g_FlashBuf[g_FlashBufSelect], format, vl);
	va_end(vl);
	return g_FlashBuf[g_FlashBufSelect];
}

/*
 *	一時バッファのアドレスを取得
 */
char *FlashOut(
	int n	//	番号 (-1 で前回 FlashIn したバッファ)
){
	return g_FlashBuf[n<0 ? g_FlashBufSelect : n];
}

/*
 *	書式付ダイアログ
 */
void Dialog(
	char *format,	//	書式
	...				//	任意パラメタ
){
	g_FlashBufSelect = (g_FlashBufSelect+1)%FLASHBUF_NUM;
	va_list	vl;
	va_start(vl, format);
	vsprintf(g_FlashBuf[g_FlashBufSelect], format, vl);
	va_end(vl);
	MessageBox(GetActiveWindow(),
		g_FlashBuf[g_FlashBufSelect], DIALOG_TITLE, MB_APPLMODAL);
}

/*
 *	エラーダイアログ
 */
void ErrorDialog(
	char *format,	//	書式
	...				//	任意パラメタ
){
	g_FlashBufSelect = (g_FlashBufSelect+1)%FLASHBUF_NUM;
	va_list	vl;
	va_start(vl, format);
	vsprintf(g_FlashBuf[g_FlashBufSelect], format, vl);
	va_end(vl);
	ShowCursor(TRUE);
	DestroyWindow(svw.hWnd);
	MessageBox(GetActiveWindow(),
		g_FlashBuf[g_FlashBufSelect], DIALOG_TITLE, MB_APPLMODAL);
	//PostQuitMessage(0);
	ExitProcess(0);
}

/*
 *	はい・いいえダイアログ
 */
int YesNo(
	char *format,	//	書式
	...				//	任意パラメタ
){
	g_FlashBufSelect = (g_FlashBufSelect+1)%FLASHBUF_NUM;
	va_list	vl;
	va_start(vl, format);
	vsprintf(g_FlashBuf[g_FlashBufSelect], format, vl);
	va_end(vl);
	return MessageBox(GetActiveWindow(),
		g_FlashBuf[g_FlashBufSelect], DIALOG_TITLE, MB_YESNO|MB_APPLMODAL);
}

/*
 *	はい・いいえ・キャンセルダイアログ
 */
int YesNoCancel(
	char *format,	//	書式
	...				//	任意パラメタ
){
	g_FlashBufSelect = (g_FlashBufSelect+1)%FLASHBUF_NUM;
	va_list	vl;
	va_start(vl, format);
	vsprintf(g_FlashBuf[g_FlashBufSelect], format, vl);
	va_end(vl);
	return MessageBox(GetActiveWindow(),
		g_FlashBuf[g_FlashBufSelect], DIALOG_TITLE, MB_YESNOCANCEL|MB_APPLMODAL);
}

/*
 *	ファイル選択ダイアログ
 *
 *	戻り値: TRUE = 選択成功、FALSE = キャンセル、失敗
 */
BOOL SelectFile(
	HWND hWnd,			//	呼び出し元ハンドル
	char *file,			//	ファイル名格納先
	int len,			//	ファイル名の最大文字数
	const char *filt,	//	フィルタ
	const char *def,	//	デフォルト拡張子
	int flag			//	TRUE = 書込、FALSE = 読込
){
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	memset(file, 0, sizeof(file));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filt;
	ofn.nFilterIndex = 0;
	ofn.lpstrDefExt = def;
	ofn.nMaxFile = len;
	ofn.lpstrFile = file;
	BOOL ret;
	if(flag){
		ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
		ret = GetSaveFileName(&ofn);
	}else{
		ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
		ret = GetOpenFileName(&ofn);
	}
	return ret;
}

/*
 *	色作成ダイアログ
 *
 *	戻り値: TRUE = 選択成功、FALSE = キャンセル、失敗
 */
BOOL ColorDialog(
	HWND hWnd,		//	呼び出し元ハンドル
	PDWORD color,	//	色格納先
	DWORD init		//	初期化色
){
	static COLORREF colorsave[16];
	CHOOSECOLOR ccs;
	int r, g, b;
	SplitColor(init, &r, &g, &b);
	ccs.lStructSize = sizeof(CHOOSECOLOR);
	ccs.hwndOwner = hWnd;
	ccs.rgbResult = RGB(r, g, b);
	ccs.lpCustColors = colorsave;
	ccs.Flags = CC_FULLOPEN|CC_RGBINIT;
	BOOL ret = ChooseColor(&ccs);
	if(!ret) return FALSE;
	*color = MakePixel(GetRValue(ccs.rgbResult),
		GetGValue(ccs.rgbResult), GetBValue(ccs.rgbResult));
	return TRUE;
}

/*
 *	GetLastError() の詳細を表示
 */
void ShowLastError(){
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM
		|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPSTR)lpMsgBuf, "Error", MB_APPLMODAL);
	LocalFree(lpMsgBuf);
}

/*
 *	拡張子チェック
 */
bool CheckFileExt(
	char *fname,	//	ファイル名
	char *ext		//	拡張子 (ピリオド含む)
){
	int len1 = strlen(fname), len2 = strlen(ext);
	return len1>len2 && !strcmpi(&fname[len1-len2], ext);
}

/*
 *	拡張子自動付加
 */
string FixFileExt(
	char *fname,	//	ファイル名
	char *ext		//	拡張子 (ピリオドまず)
){
	int len = strlen(fname), next = strlen(ext);
	if(*CharPrev(fname, fname+len)=='.') return string(fname)+ext;
	if(len<next+1) return string(fname)+string(".")+ext;
	if(_strcmpi(fname+len-next, ext)) return string(fname)+string(".")+ext;
	if(*CharPrev(fname, fname+len-next)=='.') return fname;
	return string(fname)+string(".")+ext;
}

/*
 *	ファイル名フルパスからディレクトリ部分のみ取り出す
 */
void CutPath(
	char *path	//	パス格納先
){
	char *ptr = path+strlen(path);
	for(; ptr>path && *ptr!='\\'; ptr = CharPrev(path, ptr)) *ptr = 0;
}

/*
 *	ファイル名フルパスからファイル名部分のみ取り出す
 */
void CutFileName(
	char *name,	//	ファイル名格納先
	char *path	//	フルパス
){
	char *ptr = path+strlen(path);
	while(ptr>path && *ptr!='\\') ptr = CharPrev(path, ptr);
	strcpy(name, *ptr=='\\' ? ptr+1 : ptr);
}

/*
 *	実行ファイルのディレクトリを取得
 */
void GetAppPath(
	char *path	//	パス格納先
){
	GetModuleFileName(GetModuleHandle(NULL), path, 1024);
	CutPath(path);
}

/*
 *	指定されたファイルパスと同じ場所をカレントディレクトリに指定
 */
void MoveToFile(
	char *path	//	パス格納先
){
	char *tmp = FlashIn("%s", path);
	CutPath(tmp);
	chdir(tmp);
}

/*
 *	ファイル名に '/' '\' が含まれているか調べる
 */
bool CheckSlash(
	const char *chk	//	パス格納先
){
	for(; *chk; chk = CharNext(chk)) if(*chk=='\\' || *chk=='/') return true;
	return false;
}

/*
 *	convert from " to ''
 */
string ExpandDoubleQuote(
	const string &str	//	対象文字列
){
	string ret = str;
	int i = 0;
	while(ret[i] && i<ret.size()){
		if(ret[i]=='\"'){
			ret[i] = '\'';
			ret.insert(i, 1, '\'');
		}
		const char *cs = ret.c_str();
		i = CharNext(cs+i)-cs;
	}
	return ret;
}

/*
 *	convert from '' to "
 */
string RestoreDoubleQuote(
	const string &str	//	対象文字列
){
	string ret = str;
	int i = 0;
	while(i<(int)ret.size()-1 && ret[i]){
		if(ret[i]=='\'' && ret[i+1]=='\''){
			ret[i] = '\"';
			ret.erase(i+1, 1);
		}
		const char *cs = ret.c_str();
		i = CharNext(cs+i)-cs;
	}
	return ret;
}
