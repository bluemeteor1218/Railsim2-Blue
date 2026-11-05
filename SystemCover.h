#ifndef SYSTEMCOVER_H_INCLUDED
#define SYSTEMCOVER_H_INCLUDED

char *FlashIn(char *, ...);
char *FlashOut(int n = -1);

void Dialog(char *, ...);
void ErrorDialog(char *, ...);
int YesNo(char *, ...);
int YesNoCancel(char *, ...);

BOOL SelectFile(HWND, char *, int, const char *, const char *, BOOL);
BOOL ColorDialog(HWND, PDWORD, DWORD);

void ShowLastError();

bool CheckFileExt(char *, char *);
string FixFileExt(char *, char *);
void CutPath(char *);
void CutFileName(char *, char *);
void GetAppPath(char *);
void MoveToFile(char *);
bool CheckSlash(const char *);

string ExpandDoubleQuote(const string &);
string RestoreDoubleQuote(const string &);

/*
 *	キーチェック
 */
inline bool CheckShift(){
	return (GetKey(DIK_LSHIFT)|GetKey(DIK_RSHIFT))>=S_PUSH;
}
inline bool CheckCtrl(){
	return (GetKey(DIK_LCONTROL)|GetKey(DIK_RCONTROL))>=S_PUSH;
}
inline bool CheckAlt(){
	return (GetKey(DIK_LALT)|GetKey(DIK_RALT))>=S_PUSH;
}

/*
 *	高速移動チェック
 */
inline float CheckFast(){
	return CheckCtrl() ? 10.0f : 1.0f;
}

/*
 *	リピート判定
 */
inline bool PickRepeat(int *r, int s){
	*r = s==S_PUSH ? 1 : (*r<1 ? 2 : *r+1);
	return *r==1 || *r>REPEAT_FRAME;
}

/*
 *	四捨五入
 */
inline int Round(float v){ return (int)(v>0.0f ? v+0.5f : v-0.5f); }
inline int Round(double v){ return (int)(v>0.0 ? v+0.5 : v-0.5); }

/*
 *	マンハッタン距離
 */
inline int Manhattan(POINT pa, POINT pb){
	return abs(pa.x-pb.x)+abs(pa.y-pb.y);
}

/*
 *	ファイルにベクトル値の保存
 */
inline void V3Save(
	FILE *df,	//	ファイル
	VEC3 &v,	//	ベクトル
	char *e		//	サフィックス
){
	fprintf(df, "(%f, %f, %f)%s", v.x, v.y, v.z, e);
}

#endif
