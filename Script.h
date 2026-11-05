#ifndef SCRIPT_H_INCLUDED
#define SCRIPT_H_INCLUDED

//	定形エラー
#define ERR_ENDBLOCK lang(SyntaxError)

/*
 *	エラーハンドラ
 */
class CSynErr{
private:
	char *m_ErrorPos;	//	エラー発生箇所
	string m_Message;	//	データ
public:
	CSynErr(char *);
	CSynErr(char *, const char *, ...);
	char *Get(){ return (char *)m_Message.c_str(); }
	void Handle(string, char *);
};

//	関数宣言
char *Space(char *);
char *ConstInteger(char *, int *);
char *ConstFloat(char *, float *);
char *ColorValue(char *, D3DCOLOR *);
char *HexPointer(char *, void **);
char *Vector2D(char *, VEC3 *);
char *Vector3D(char *, VEC3 *);
char *Identifier(char *, string *);
char *StringLiteral(char *, string *);
char *BoolYesNo(char *, bool *);
char *Identifier2(char *, char *);
char *Assignment(char *, char *);
char *AsgnInteger(char *, char *, int *, int n = 1, bool fill = false);
char *AsgnFloat(char *, char *, float *, int n = 1, bool fill = false);
char *AsgnColor(char *, char *, D3DCOLOR *, int n = 1, bool fill = false);
char *AsgnPointer(char *, char *, void **, int n = 1, bool fill = false);
char *AsgnVector2D(char *, char *, VEC2 *, int n = 1, bool fill = false);
char *AsgnVector3D(char *, char *, VEC3 *, int n = 1, bool fill = false);
char *AsgnIdentifier(char *, char *, string *, int n = 1, bool fill = false);
char *AsgnString(char *, char *, string *, int n = 1, bool fill = false);
char *AsgnYesNo(char *, char *, bool *, int n = 1, bool fill = false);
char *BeginBlock(char *, char *);
char *BeginNamedBlock(char *, char *, string *);
char *EndBlock(char *);

/*
 *	任意文字
 */
inline char *Character(
	char *str,	//	対象文字列
	char read	//	読取文字
){
	if(*str==read) return str+1;
	return NULL;
}

/*
 *	任意文字 + 空白スキップ
 */
inline char *Character2(
	char *str,	//	対象文字列
	char read	//	読取文字
){
	return (str = Character(str, read)) ? Space(str) : NULL;
}

/*
 *	任意列
 */
inline char *String(
	char *str,	//	対象文字列
	char *read	//	読取文字列
){
	while(*read) if(*read++!=*str++) return NULL;
	return str;
}

/*
 *	任意列 + 空白スキップ
 */
inline char *String2(
	char *str,	//	対象文字列
	char *read	//	読取文字列
){
	return (str = String(str, read)) ? Space(str) : NULL;
}

#endif
