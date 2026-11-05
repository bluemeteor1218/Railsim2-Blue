#include "stdafx.h"

/*
 *	コンストラクタ
 */
CSynErr::CSynErr(
	char *pos	//	エラー発生箇所
){
	m_Message = lang(SyntaxError);
	m_ErrorPos = pos;
}

/*
 *	コンストラクタ
 */
CSynErr::CSynErr(
	char *pos,			//	エラー発生箇所
	const char *format,	//	書式
	...					//	任意パラメタ
){
	char *buf = FlashIn("");
	va_list	vl;
	va_start(vl, format);
	vsprintf(buf, format, vl);
	va_end(vl);
	m_Message = buf;
	m_ErrorPos = pos;
}

/*
 *	コンストラクタ
 */
void CSynErr::Handle(
	string head,	//	メッセージヘッダ
	char *buf		//	スクリプトバッファ
){
	if(!buf || !m_ErrorPos || m_ErrorPos<buf){
		ErrorDialog("%s\n%s", head.c_str(), m_Message.c_str());
		return;
	}
	char *ptr = buf;
	int line = 1;
	bool cr = false;
	while(*ptr && ptr<m_ErrorPos){
		char *next = CharNext(ptr);
		if(next-ptr>1){
			cr = false;
		}else if(*ptr==0x0d){
			line++;
			cr = true;
		}else if(*ptr==0x0a){
			if(!cr) line++;
			cr = false;
		}else{
			cr = false;
		}
		ptr = next;
	}
	if(ptr<m_ErrorPos){
		ErrorDialog("%s\n%s", head.c_str(), m_Message.c_str());
	}else{
		int i = 0;
		char *tmp = ptr;
		for(; *tmp && *tmp!=0x0d && *tmp!=0x0a && i<512; i++, tmp++);
		*tmp = 0;
		ErrorDialog("%s\nLine %d\n--------------------\n%s\n--------------------\n%s",
			head.c_str(), line, ptr, m_Message.c_str());
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	アルファベット + アンダースコア
 */
inline char *Nondigit(
	char *str	//	数式文字列
){
	if('A'<=*str && *str<='Z' || 'a'<=*str && *str<='z'
 		|| *str=='_') return str+1;
	return NULL;
}

/*
 *	数字
 */
inline char *Digit(
	char *str	//	数式文字列
){
	if('0'<=*str && *str<='9') return str+1;
	return NULL;
}

/*
 *	数字
 */
inline char *Hexadecimal(
	char *str	//	数式文字列
){
	if('A'<=*str && *str<='F' || 'a'<=*str && *str<='f'
		|| '0'<=*str && *str<='9') return str+1;
	return NULL;
}

/*
 *	空白およびコメント
 */
char *Space(
	char *str	//	対象文字列
){
	while(str && *str){
		char *tmp, *eee;
		if(tmp = Character(str, ' ')){
			str = tmp;
		}else if(tmp = Character(str, '\t')){
			str = tmp;
		}else if(tmp = Character(str, '\r')){
			str = tmp;
		}else if(tmp = Character(str, '\n')){
			str = tmp;
		}else if(tmp = String(str, "　")){
			str = tmp;
		}else if(tmp = String(str, "//")){
			str = tmp;
			while(*str){
				if(tmp = Character(str, '\n')){
					str = tmp;
					break;
				}
				str = CharNext(str);
			}
		}else if(tmp = String(eee = str, "/*")){
			char *cmtloc = str;
			str = tmp;
			while(true){
				if(!*str) throw CSynErr(eee, lang(CommentEndNotFound));
				if(tmp = String(str, "*/")){
					str = tmp;
					break;
				}else{
					str = CharNext(str);
				}
			}
		}else{
			break;
		}
	}
	return str;
}

/*
 *	定数値
 */
char *ConstValue(
	char *str,	//	対象文字列
	int *reti,	//	読込先 (int)
	float *retf	//	読込先 (float)
){
	char *tmp;
	bool minus = false;
	if(tmp = Character(str, '-')){
		str = tmp;
		minus = true;
	}
	if(tmp = Digit(str)){
		while(true){
			char *tmp2;
			if(tmp2 = Digit(tmp)){
				tmp = tmp2;
			}else if(tmp2 = Character(tmp, '.')){
				tmp = tmp2;
				goto UNDERDECIMAL;
			}else{
				char save = *tmp;
				*tmp = 0;
				int val;
				sscanf(str, "%d", &val);
				*reti = minus ? -val : val;
				*retf = (float)*reti;
				*tmp = save;
				return Space(tmp);
			}
		}
	}else if(tmp = Character(str, '.')){
UNDERDECIMAL:
		char *tmp2;
		if(tmp2 = Character(tmp, '#')){
			tmp = tmp2;
			while(true){
				if((tmp2 = Digit(tmp)) || (tmp2 = Nondigit(tmp))){
					tmp = tmp2;
				}else{
					*retf = 0.0f;
					*reti = 0;
					return Space(tmp);
				}
			}
		}
		while(true){
			if(tmp2 = Digit(tmp)){
				tmp = tmp2;
			}else{
				char save = *tmp;
				*tmp = 0;
				float val;
				sscanf(str, "%f", &val);
				*retf = minus ? -val : val;
				*reti = (int)*retf;
				*tmp = save;
				return Space(tmp);
			}
		}
	}
	return NULL;
}

/*
 *	整数値
 */
char *ConstInteger(
	char *str,	//	対象文字列
	int *ret	//	読込先
){
	float dummy;
	if(str = ConstValue(str, ret, &dummy)) return str;
	return NULL;
}

/*
 *	実数値
 */
char *ConstFloat(
	char *str,	//	対象文字列
	float *ret	//	読込先
){
	int dummy;
	if(str = ConstValue(str, &dummy, ret)) return str;
	return NULL;
}

/*
 *	色値
 */
char *ColorValue(
	char *str,		//	対象文字列
	D3DCOLOR *ret	//	読込先
){
	char *tmp;
	if(tmp = Character(str, '#')){
		str = tmp;
		while(true){
			char *tmp2;
			if(tmp2 = Hexadecimal(tmp)){
				tmp = tmp2;
			}else{
				char save = *tmp;
				*tmp = 0;
				D3DCOLOR val;
				sscanf(str, "%x", &val);
				*ret = val;
				*tmp = save;
				return Space(tmp);
			}
		}
	}
	return NULL;
}

/*
 *	ポインタ
 */
char *HexPointer(
	char *str,	//	対象文字列
	void **ret	//	読込先
){
	char *tmp;
	if(tmp = Hexadecimal(str)){
		while(true){
			char *tmp2;
			if(tmp2 = Hexadecimal(tmp)){
				tmp = tmp2;
			}else{
				char save = *tmp;
				*tmp = 0;
				void *val;
				sscanf(str, "%p", &val);
				*ret = val;
				*tmp = save;
				return Space(tmp);
			}
		}
	}
	return NULL;
}

/*
 *	2D ベクトル
 */
char *Vector2D(
	char *str,	//	対象文字列
	VEC2 *ret	//	読込先
){
	if(!(str = Character2(str, '('))) return NULL;
	if(!(str = ConstFloat(str, &ret->x))) return NULL;
	if(!(str = Character2(str, ','))) return NULL;
	if(!(str = ConstFloat(str, &ret->y))) return NULL;
	if(!(str = Character2(str, ')'))) return NULL;
	return str;
}

/*
 *	3D ベクトル
 */
char *Vector3D(
	char *str,	//	対象文字列
	VEC3 *ret	//	読込先
){
	if(!(str = Character2(str, '('))) return NULL;
	if(!(str = ConstFloat(str, &ret->x))) return NULL;
	if(!(str = Character2(str, ','))) return NULL;
	if(!(str = ConstFloat(str, &ret->y))) return NULL;
	if(!(str = Character2(str, ','))) return NULL;
	if(!(str = ConstFloat(str, &ret->z))) return NULL;
	if(!(str = Character2(str, ')'))) return NULL;
	return str;
}

/*
 *	識別子
 */
char *Identifier(
	char *str,	//	対象文字列
	string *ret	//	読込先
){
	char *tmp;
	if(!(tmp = Nondigit(str))) return NULL;
	while(true){
		char *tmp2;
		if(tmp2 = Nondigit(tmp)){
			tmp = tmp2;
		}else if(tmp2 = Digit(tmp)){
			tmp = tmp2;
		}else{
			char save = *tmp;
			*tmp = 0;
			*ret = str;
			*tmp = save;
			return Space(tmp);
		}
	}
	return NULL;
}

/*
 *	リテラル文字列
 */
char *StringLiteral(
	char *str,	//	対象文字列
	string *ret	//	読込先
){
	char *tmp, *eee;
	if(tmp = Character(eee = str, '\"')){
		str = tmp;
		while(*tmp){
			char *tmp2;
			if((tmp2 = Character(tmp, '\r'))
				|| (tmp2 = Character(tmp, '\n'))){
				throw CSynErr(eee, lang(StringLiteralExceedLineBreak));
			}else if(tmp2 = String(tmp, "\\\"")){
				tmp = tmp2;
			}else if(tmp2 = String(tmp, "\\\r\n")){
				tmp = tmp2;
			}else if(tmp2 = Character(tmp, '\"')){
				char save = *tmp;
				*tmp = 0;
				*ret = str;
				*tmp = save;
				return Space(tmp2);
			}else{
				tmp = CharNext(tmp);
			}
		}
	}
	return NULL;
}

/*
 *	yes / no
 */
char *BoolYesNo(
	char *str,	//	対象文字列
	bool *ret	//	読込先
){
	string yesno;
	if(!(str = Identifier(str, &yesno))) return NULL;
	*ret = yesno=="yes" ? true : false;
	return str;
}

/*
 *	識別子 + 比較
 */
char *Identifier2(
	char *str,	//	対象文字列
	char *read	//	識別子
){
	string tmp;
	if(!(str = Identifier(str, &tmp))) return NULL;
	return tmp==read ? str : NULL;
}

/*
 *	代入文
 */
char *Assignment(
	char *str,	//	対象文字列
	char *read	//	識別子
){
	if(!(str = Identifier2(str, read))) return NULL;
	return Character2(str, '=');
}

/*
 *	整数代入
 */
char *AsgnInteger(
	char *str,	//	対象文字列
	char *read,	//	識別子
	int *ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = ConstInteger(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = ConstInteger(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	実数代入
 */
char *AsgnFloat(
	char *str,	//	対象文字列
	char *read,	//	識別子
	float *ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = ConstFloat(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = ConstFloat(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	色値代入
 */
char *AsgnColor(
	char *str,		//	対象文字列
	char *read,		//	識別子
	D3DCOLOR *ret,	//	読込先
	int n,			//	読込個数
	bool fill		//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = ColorValue(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = ColorValue(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	色値代入
 */
char *AsgnPointer(
	char *str,	//	対象文字列
	char *read,	//	識別子
	void **ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = HexPointer(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = HexPointer(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	2D ベクトル代入
 */
char *AsgnVector2D(
	char *str,	//	対象文字列
	char *read,	//	識別子
	VEC2 *ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = Vector2D(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = Vector2D(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	3D ベクトル代入
 */
char *AsgnVector3D(
	char *str,	//	対象文字列
	char *read,	//	識別子
	VEC3 *ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = Vector3D(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = Vector3D(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	識別子代入
 */
char *AsgnIdentifier(
	char *str,		//	対象文字列
	char *read,		//	識別子
	string *ret,	//	読込先
	int n,			//	読込個数
	bool fill		//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = Identifier(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = Identifier(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	文字列代入
 */
char *AsgnString(
	char *str,		//	対象文字列
	char *read,		//	識別子
	string *ret,	//	読込先
	int n,			//	読込個数
	bool fill		//	不足自動補完
){
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = StringLiteral(str, ret++))) return NULL;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = StringLiteral(str, ret++))) return NULL;
	}
	return Character2(str, ';');
}

/*
 *	yes / no 代入 (yes: 1, no: 0)
 */
char *AsgnYesNo(
	char *str,	//	対象文字列
	char *read,	//	識別子
	bool *ret,	//	読込先
	int n,		//	読込個数
	bool fill	//	不足自動補完
){
	string yesno;
	if(!(str = Assignment(str, read))) return NULL;
	if(!(str = Identifier(str, &yesno))) return NULL;
	*ret = yesno=="yes" ? true : false; ret++;
	while(--n>0){
		char *tmp;
		if(tmp = Character2(str, ';')){
			if(!fill) return NULL;
			while(n-->0){ *ret = *(ret-1); ret++; }
			return tmp;
		}
		if(!(str = Character2(str, ','))) return NULL;
		if(!(str = Identifier(str, &yesno))) return NULL;
		*ret = yesno=="yes" ? true : false; ret++;
	}
	return Character2(str, ';');
}

/*
 *	ブロック開始
 */
char *BeginBlock(
	char *str,	//	対象文字列
	char *read	//	識別子
){
	if(!(str = Identifier2(str, read))) return NULL;
	return Character2(str, '{');
}

/*
 *	名前付きブロック開始
 */
char *BeginNamedBlock(
	char *str,		//	対象文字列
	char *read,		//	識別子
	string *name	//	名前代入先
){
	if(!(str = Identifier2(str, read))) return NULL;
	if(!(str = StringLiteral(str, name))) return NULL;
	return Character2(str, '{');
}

/*
 *	ブロック終了
 */
char *EndBlock(
	char *str	//	対象文字列
){
	return Character2(str, '}');
}
