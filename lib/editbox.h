//	Copyright (c) 2002 Midikyou

#define EDIT_PROC	0	//	入力中である
#define EDIT_OK		1	//	入力は確定された
#define EDIT_CANCEL	-1	//	入力はキャンセルされた

BOOL SelectFile(char *file, int len, const char *def, const char *ext, int flag);

class CEditBox{
	static bool ms_Active;	//	エディットボックス有効フラグ

	HIMC m_hImc;		//	FEPのハンドラ
	//	LPCANDIDATELIST m_pList;
	string m_str;		//	保存用文字列
	string m_show;		//	表示する文字列
	string m_comp;		//	変換中の文字列
	int m_x;			//	表示開始X位置
	int m_y;			//	表示開始Y位置
	int m_width;		//	編集領域最小幅
	int m_pos;			//	キャレット位置
	int m_selpos;		//	選択開始位置
	int m_max;			//	最大文字数
	int m_frame;		//	点滅カウンタ
	int m_oldsize;		//	変換文字列サイズ
	BOOL m_old_iscomp;	//	変換中かどうか (IsComp とは異なる)

	void DrawCaret();
	void BackSpace();
	void DeleteChar();
	bool DeleteSelected();
	string GetSelected();
	void AddChar(char c);
	void AddString(string s);
	void ClipCut();
	void ClipCopy();
	void ClipPaste();
	void MoveLeft();
	void MoveRight();
	void MoveHead();
	void MoveLast();
	void SelectAll();
	void CompEnd(bool);
	void Clip();
	void GetCompStr(string &str);
	void GetResultStr(string &str);

public:
	static bool IsActive(){ return ms_Active; }
	static void Disable(){ ms_Active = false; }

	CEditBox();
	~CEditBox();
	void Create(int x, int y, int w, int max = 80, string pre = "", int imm=-1);
	void Release(int imm=-1);
	int ScanInput();
	void Render();

	/*
	 *	位置設定
	 */
	void SetPos(int x, int y){
		m_x = x; m_y = y;
	}
	/*
	 *	エディットボックス内の文字列を取得
	 *
	 *	str	: 文字列の格納先
	 */
	void GetText(string &str){
		str = m_str;
	}
	/*
	 *	FEPの状態を取得
	 *
	 *	戻り値：TRUE＝FEP起動中、FALSE＝非起動中
	 */
	BOOL IsFEPOpen(){
		return ImmGetOpenStatus(m_hImc);
	}
	/*
	 *	変換中であるかを取得
	 *
	 *	戻り値：TRUE＝変換中、FALSE＝非変換中
	 */
	BOOL IsComp(){
		if(m_comp!="") return TRUE;
		else return FALSE;
	}
	/*
	 *	変換中のカーソル位置の取得
	 */
	int CEditBox::GetFEPCursorPos(){
		return ImmGetCompositionString(m_hImc, GCS_CURSORPOS, NULL, 0);
	}
};

/*
 *	全半角の判定
 */
inline BOOL Is2ByteChar(BYTE c){
	if((c>=0x81 && c<=0x9f) || (c>=0xe0 && c<=0xfc))
		return TRUE;
	else
		return FALSE;
}

/*
 *	表示可能文字の判定
 */
inline BOOL IsPrintChar(int c){
	if(__isascii(c) && isprint(c)) return TRUE;
	else return FALSE;
}
