//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "frame.h"
#include "window.h"
#include "graphic.h"
#include "draw.h"
#include "texture.h"
#include "font.h"
#include "input.h"
#include "sound.h"
#include "wave.h"
#include "mesh.h"
#include "object.h"
#include "editbox.h"
#include "..\Const.h"
#include "..\Macro.h"
#include "..\SystemCover.h"
#include "..\CSkinPlugin.h"

//	関数宣言
string EliminateTabAndCRLF(string);

//	内部定数
const int BLINK_FRAME = MAXFPS/2;	//	点滅間隔
const int BASE_MARGINX = 1;			//	編集領域余白

//	static メンバ
bool CEditBox::ms_Active = false;

/*
 *	コンストラクタ
 */
CEditBox::CEditBox(){
	m_hImc = NULL;
//	m_pList = NULL;
	m_str = "";
	m_x = 0;
	m_y = 0;
	m_pos = m_selpos = 0;
	m_max = 80;
}

/*
 *	デストラクタ
 */
CEditBox::~CEditBox(){
	Release();
}

/*
 *	エディットボックス作成
 *
 *	x	: 表示開始 X 座標
 *	y	: 表示開始 Y 座標
 *	max	: 最大文字数
 *	imm : 0:不変, 1:ON, -1:OFF
 */
void CEditBox::Create(int x, int y, int w, int max, string pre, int imm){
	Release(imm);
	ms_Active = true;
	m_hImc = ImmGetContext(svw.hWnd);
	if(imm) ImmSetOpenStatus(m_hImc, imm>0);
	m_comp = "";
	pre = EliminateTabAndCRLF(pre);
	m_x = x;
	m_y = y;
	m_width = w;
	m_pos = pre.size();
	m_selpos = 0;
	m_max = max;
	m_str = m_show = pre;
	m_frame = 0;
	m_oldsize = 0;
	m_old_iscomp = FALSE;
	ClearCharQueue();
}

/*
 *	エディットボックス解放
 */
void CEditBox::Release(int imm){
	ms_Active = false;
	if(m_hImc){
		if(imm) ImmSetOpenStatus(m_hImc, imm>0);
		ImmReleaseContext(svw.hWnd, m_hImc);
		m_hImc = 0;
	}
}

/*
 *	入力チェック
 */
int CEditBox::ScanInput(){
	//	キーが何も押されていないならスキップ
	if(CheckKeyDown()<0) return EDIT_PROC;

	BOOL is_comp = FALSE;
	if(ImmGetCompositionString(m_hImc, GCS_COMPCLAUSE, NULL, 0)>0){
		char attr = -1;
		ImmGetCompositionString(m_hImc, GCS_COMPATTR, &attr, 1);
		if(attr!=ATTR_INPUT) is_comp = TRUE;
	}
	BOOL comp_end = m_old_iscomp && !is_comp;
	m_old_iscomp = is_comp;
	//	エンターキー
	bool enter = (GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH;
	if(enter || comp_end){
		PeekAllMessage();
		//	変換後の確定で無い場合は入力終了と見なす
		if(!IsComp()) return EDIT_OK;

		//	確定した文字列を連結
		CompEnd(enter);
		return EDIT_PROC;
	}
	//	エスケープキー
	if(GetKey(DIK_ESCAPE)==S_PUSH){
		//	変換後の取り消しで無い場合は入力終了と見なす
		if(!IsComp()) return EDIT_CANCEL;
		else return EDIT_PROC;
	}
	//	FEPが起動中なら変換中の文字列を取得
	int oldsize = m_oldsize;
	if(IsFEPOpen()){
		GetCompStr(m_comp);
		m_oldsize = m_comp.size();
	}else{
		m_oldsize = 0;
	}
	static int rep;
	if(IsComp()){
		//	変換中なら
		rep = 0;
	}else if(m_oldsize==oldsize){
		//	変換中でないなら (FEPの起動の有無にかかわらず)
		int s;
		if((s = GetKey(DIK_BACK))>=S_PUSH){
			if(PickRepeat(&rep, s)) BackSpace();
		}else if((s = GetKey(DIK_DELETE))>=S_PUSH){
			if(PickRepeat(&rep, s)) DeleteChar();
		}else if((s = GetKey(DIK_LEFT))>=S_PUSH){
			if(PickRepeat(&rep, s)) MoveLeft();
		}else if((s = GetKey(DIK_RIGHT))>=S_PUSH){
			if(PickRepeat(&rep, s)) MoveRight();
		}else if(IsFEPOpen() && (s = GetKey(DIK_SPACE))>=S_PUSH){
			if(PickRepeat(&rep, s)) AddString("　");
		}else if(GetKey(DIK_HOME)==S_PUSH){
			MoveHead();
		}else if(GetKey(DIK_END)==S_PUSH){
			MoveLast();
		}else if(CheckCtrl() && GetKey(DIK_A)==S_PUSH){
			SelectAll();
		}else if(CheckCtrl() && GetKey(DIK_X)==S_PUSH){
			ClipCut();
		}else if(CheckCtrl() && GetKey(DIK_C)==S_PUSH){
			ClipCopy();
		}else if(CheckCtrl() && GetKey(DIK_V)==S_PUSH){
			ClipPaste();
		}else{ 
			int c;
			while(c = DequeueChar())
				if(IsPrintChar(c) && (m_selpos!=m_pos || m_str.size()<m_max)) AddChar(c);
		}
	}else{
		m_show = m_str;
	}
	return EDIT_PROC;
}

/*
 *	レンダリング
 */
void CEditBox::Render(){
	devSetTexture(0, NULL);
	ms_Active = true;
	int i, sel1, sel2;
	if(m_pos<m_selpos){ sel1 = m_pos; sel2 = m_selpos; }
	else{ sel1 = m_selpos; sel2 = m_pos; }
	m_show = m_str;
	if(IsComp()){
		//	変換中なら
		int size = ImmGetCompositionString(m_hImc, GCS_COMPCLAUSE, NULL, 0);
		int n = size/sizeof(int), *pClause = new int[n];
		ImmGetCompositionString(m_hImc, GCS_COMPCLAUSE, pClause, size);
		size = ImmGetCompositionString(m_hImc, GCS_COMPATTR, NULL, 0);
		char *pAttr = new char[size];
		ImmGetCompositionString(m_hImc, GCS_COMPATTR, pAttr, size);

		string tmpstr(&m_str[0], &m_str[m_pos]);
		m_show.insert(m_pos, m_comp);
		int tbw = (m_show.size()+1)*svf.size/2;
		if(tbw<m_width) tbw = m_width;
		if(m_pos<m_selpos){ sel1 += m_comp.size(); sel2 += m_comp.size(); }
		Grad2DRect(m_x-BASE_MARGINX, m_y, m_x+tbw+BASE_MARGINX, m_y+svf.size,
			g_Skin->m_EditCtrlData.m_EditBaseColor);
		if(sel1!=sel2) Grad2DRect(m_x+sel1*svf.size/2, m_y,
			m_x+sel2*svf.size/2, m_y+svf.size, g_Skin->m_EditCtrlData.m_SelectedBaseColor);

		for(i = 0; i<n-1; i++){
			D3DCOLOR col = g_Skin->m_EditCtrlData.
				m_ConvertClauseColor[pAttr[pClause[i]]==ATTR_TARGET_CONVERTED];
			int tx = m_x+((m_pos+pClause[i])*svf.size/2)+1, ty = m_y+svf.size-2;
			int tx2 = m_x+((m_pos+pClause[i+1])*svf.size/2)-1;
			Draw2DLine(tx, ty, tx2, ty, col);
			Draw2DLine(tx, ty+1, tx2, ty+1, col);
		}

		Text(m_x, m_y, g_Skin->m_EditCtrlData.m_EditFontColor, tmpstr.c_str());
		Text(m_x+m_pos*svf.size/2, m_y,
			g_Skin->m_EditCtrlData.m_ConvertFontColor, m_comp.c_str());
		Text(m_x+(m_pos+m_comp.size())*svf.size/2, m_y,
			g_Skin->m_EditCtrlData.m_EditFontColor, &m_str[m_pos]);

		delete [] pClause;
		delete [] pAttr;
	}else{
		//	文字列を表示
		int tbw = (m_show.size()+1)*svf.size/2;
		if(tbw<m_width) tbw = m_width;
		Grad2DRect(m_x-BASE_MARGINX, m_y, m_x+tbw+BASE_MARGINX, m_y+svf.size,
			g_Skin->m_EditCtrlData.m_EditBaseColor);
		if(sel1!=sel2) Grad2DRect(m_x+sel1*svf.size/2, m_y,
			m_x+sel2*svf.size/2, m_y+svf.size, g_Skin->m_EditCtrlData.m_SelectedBaseColor);
		Text(m_x, m_y, g_Skin->m_EditCtrlData.m_EditFontColor, m_show.c_str());
		//	キャレットを表示
		DrawCaret();
	}
}

/*
 *	キャレット表示
 */
void CEditBox::DrawCaret(){
	m_frame = (m_frame+1)%BLINK_FRAME;
	if(m_frame<BLINK_FRAME/2){
		int tx = m_x+(m_pos*svf.size/2), ty = m_y+svf.size-2;
		Draw2DLine(tx, ty, tx+svf.size/2, ty, g_Skin->m_EditCtrlData.m_ConvertFontColor);
		Draw2DLine(tx, ty+1, tx+svf.size/2, ty+1, g_Skin->m_EditCtrlData.m_ConvertFontColor);
	}
}

/*
 *	キャレット位置から1文字前を消す
 */
void CEditBox::BackSpace(){
	if(!DeleteSelected()){
		//	現在位置が 2 文字目以降
		if(m_pos>0){
			char *s = (char *)m_str.c_str();
			int j = CharPrev(s, s+m_pos)-s;
			m_str.erase(j, m_pos-j);
			m_selpos = m_pos = j;
		}
		m_show = m_str;
		m_frame = BLINK_FRAME/5;
	}else{
		DeleteSelected();
	}
}

/*
 *	キャレット位置の文字を消す
 */
void CEditBox::DeleteChar(){
	if(!DeleteSelected()){
		if(m_pos<m_str.size()){
			char *s = (char *)m_str.c_str();
			int j = CharNext(s+m_pos)-s;
			m_str.erase(m_pos, j-m_pos);
		}
	}
	m_show = m_str;
	m_frame = BLINK_FRAME/5;
}

/*
 *	選択された文字列を消す
 */
bool CEditBox::DeleteSelected(){
	if(m_pos<m_selpos){
		m_str.erase(m_pos, m_selpos-m_pos);
		m_selpos = m_pos;
	}else if(m_selpos<m_pos){
		m_str.erase(m_selpos, m_pos-m_selpos);
		m_pos = m_selpos;
	}else{
		return false;
	}
	return true;
}

/*
 *	選択された文字列を取得
 */
string CEditBox::GetSelected(){
	const char *s = m_str.c_str();
	if(m_pos<m_selpos) return string(s+m_pos, s+m_selpos);
	else if(m_selpos<m_pos) return string(s+m_selpos, s+m_pos);
	else return "";
}

/*
 *	キャレット位置に文字を追加
 *
 *	c		: 文字
 */
void CEditBox::AddChar(char c){
	DeleteSelected();
	m_str.insert(m_str.begin()+m_pos, c);
	m_selpos = ++m_pos;
	m_show = m_str;
	m_frame = BLINK_FRAME/5;
}

/*
 *	キャレット位置に文字列を追加
 *
 *	s		: 文字列
 */
void CEditBox::AddString(string s){
	DeleteSelected();
	ClearCharQueue();
	m_str.insert(m_str.begin()+m_pos, s.begin(), s.end());
	m_selpos = m_pos += s.size();
	Clip();
	m_show = m_str;
	m_frame = BLINK_FRAME/5;
}

/*
 *	クリップボードに切り取り
 */
void CEditBox::ClipCut(){
	if(m_selpos==m_pos) return;
	ClipCopy();
	DeleteSelected();
	m_show = m_str;
	m_frame = BLINK_FRAME/5;
}

/*
 *	クリップボードにコピー
 */
void CEditBox::ClipCopy(){
	if(m_selpos==m_pos) return;
	HGLOBAL hText;
	string sel = GetSelected();
	hText = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sel.size()+1);
	lstrcpy((char *)GlobalLock(hText), sel.c_str());
	GlobalUnlock(hText);
	OpenClipboard(svw.hWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hText);
	CloseClipboard();
}

/*
 *	クリップボードから貼り付け
 */
void CEditBox::ClipPaste(){
	OpenClipboard(svw.hWnd);
	HANDLE hText = GetClipboardData(CF_TEXT);
	if(!hText) return;
	string s = EliminateTabAndCRLF((char *)GlobalLock(hText));
	GlobalUnlock(hText);
	CloseClipboard();
	DeleteSelected();
	ClearCharQueue();
	m_str.insert(m_str.begin()+m_pos, s.begin(), s.end());
	m_selpos = m_pos += s.size();
	Clip();
	m_show = m_str;
	m_frame = BLINK_FRAME/5;
}

/*
 *	キャレットを左に移動
 */
void CEditBox::MoveLeft(){
	char *s = (char *)m_str.c_str();
	if(m_pos>0) m_pos = CharPrev(s, s+m_pos)-s;
	if(!CheckShift()) m_selpos = m_pos;
	m_frame = BLINK_FRAME/5;
}

/*
 *	キャレットを右に移動
 */
void CEditBox::MoveRight(){
	char *s = (char *)m_str.c_str();
	if(m_pos<m_str.size()) m_pos = CharNext(s+m_pos)-s;
	if(!CheckShift()) m_selpos = m_pos;
	m_frame = BLINK_FRAME/5;
}

/*
 *	キャレットを先頭に移動
 */
void CEditBox::MoveHead(){
	m_pos = 0;
	if(!CheckShift()) m_selpos = m_pos;
	m_frame = BLINK_FRAME/5;
}

/*
 *	キャレットを最後尾に移動
 */
void CEditBox::MoveLast(){
	m_pos = m_str.size();
	if(!CheckShift()) m_selpos = m_pos;
	m_frame = BLINK_FRAME/5;
}

/*
 *	全て選択
 */
void CEditBox::SelectAll(){
	m_selpos = 0;
	m_pos = m_str.size();
	m_frame = BLINK_FRAME/5;
}

/*
 *	変換確定
 */
void CEditBox::CompEnd(bool get_result){
	DeleteSelected();
	if(get_result) GetResultStr(m_comp);
	m_str = m_str.insert(m_pos, m_comp);
	m_selpos = m_pos += m_comp.size();
	Clip();			//	最大文字数でクリップ
	m_show = m_str;	//	表示文字列を更新
	m_comp = "";	//	変換中文字列のクリア
	ClearCharQueue();
}

/*
 *	文字数制限
 */
void CEditBox::Clip(){
	int size = m_str.size(), i;
	if(size<=m_max) return;
	char *s = (char *)m_str.c_str();

	//	全角文字の整合性をチェックしながら消していく
	for(i = size; i>m_max;){
		int j = CharPrev(s, s+i)-s;
		m_str.erase(j, i-j);
		i = j;
	}
	size = m_str.size();
	if(m_pos>size) m_selpos = m_pos = size;
}

/*
 *	変換中の文字列を取得
 *
 *	str	: 文字列の格納先
 */
void CEditBox::GetCompStr(string &str){
	LONG size = ImmGetCompositionString(m_hImc, GCS_COMPSTR, NULL, 0);
	char *pBuf = new char[size+1];
	ImmGetCompositionString(m_hImc, GCS_COMPSTR, pBuf, size);
	pBuf[size] = 0;
	str = pBuf;
	delete pBuf;
}

/*
 *	変換が確定した文字列を取得
 *
 *	str	: 文字列の格納先
 */
void CEditBox::GetResultStr(string &str){
	LONG size = ImmGetCompositionString(m_hImc, GCS_RESULTSTR, NULL, 0);
	char *pBuf = new char[size+1];
	ImmGetCompositionString(m_hImc, GCS_RESULTSTR, pBuf, size);
	pBuf[size] = 0;
	str = pBuf;
	delete pBuf;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	タブと改行を削除
 */
string EliminateTabAndCRLF(string s){
	char *ptr = (char *)s.c_str();
	for(; *ptr; ptr = CharNext(ptr)){
		if(*ptr=='\t') *ptr = ' ';
		else if(*ptr=='\n' || *ptr=='\r') return string(s.begin(), s.begin()+(ptr-s.c_str()));
	}
	return s;
}

/*
 *	ファイル選択ダイアログ
 *
 *	file	: ファイル名格納先
 *	len		: ファイル名の最大文字数
 *	def		: 定義("text file" 等、任意）
 *	ext		: 拡張子
 *	flag	: TRUE = 書込み、FALSE = 読込み
 *
 *	戻り値: TRUE = 選択成功, FALSE = キャンセル、失敗
 */
BOOL SelectFile(char *file, int len, const char *def, const char *ext, int flag){
	OPENFILENAME ofn;
	char filter[256];

	memset(&ofn, 0, sizeof(OPENFILENAME));
	memset(file, 0, sizeof(file));
	wsprintf(filter, "%s (*.%s)\t*.%s\t\t", def, ext, ext);

	for(int i = 0;i<256;i++){
		if(filter[i]=='\t') filter[i] = '\0';
	}
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = svw.hWnd;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 0;
	ofn.lpstrDefExt = ext;
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
