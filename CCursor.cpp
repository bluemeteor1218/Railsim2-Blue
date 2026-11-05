#include "stdafx.h"
#include "CCursor.h"
#include "CSkinPlugin.h"

//	内部定数
extern const int CSR_MOVE_TSD = 3;		//	カーソル移動判定閾値
const float CURSOR_HIDE_RATIO = 0.3f;	//	カーソル消去速度

/*
 *	読込
 */
char *CSkinPlugin::_CURSORDATA::Read(
	char *str,	//	対象文字列
	char *name	//	カーソル名
){
	char *tmp, *eee;
	int i;
	if(!(str = BeginBlock(eee = str, name))) throw CSynErr(eee);
	if(!(str = AsgnString(eee = str, "TexFileName",
		&m_TexFileName))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "ImageSize",
		m_ImageSize, 2, false))) throw CSynErr(eee);
	if(tmp = AsgnInteger(eee = str, "Cursor2DSize", m_Cursor2DSize, 2, false)) str = tmp;
	else for(i = 0; i<2; i++) m_Cursor2DSize[i] = m_ImageSize[i];
	if(!(str = AsgnInteger(eee = str, "Cursor2DHotSpot",
		m_Cursor2DHotSpot, 2, false))) throw CSynErr(eee);
	if(tmp = AsgnInteger(eee = str, "Cursor2DAnimNumber", &m_Cursor2DAnimNumber)) str = tmp;
	else m_Cursor2DAnimNumber = 1;
	m_Cursor2DAnimFrame = new int[m_Cursor2DAnimNumber];
	if(tmp = AsgnInteger(eee = str, "Cursor2DAnimFrame",
		m_Cursor2DAnimFrame, m_Cursor2DAnimNumber, true)) str = tmp;
	else for(i = 0; i<m_Cursor2DAnimNumber; i++) m_Cursor2DAnimFrame[i] = 1;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CSkinPlugin::_CURSORDATA::LoadData(){
	m_CursorTexture = g_TexList.Get(FALSE, m_TexFileName.c_str(), 0, 1);
}

/*
 *	描画
 */
void CSkinPlugin::_CURSORDATA::Render(
	POINT pos,	//	座標
	float alpha	//	α値
){
	int tw = m_ImageSize[0], th = m_ImageSize[1];
	int cw = m_Cursor2DSize[0], ch = m_Cursor2DSize[1];
	int cols = tw/cw;
	int px = pos.x-m_Cursor2DHotSpot[0], py = pos.y-m_Cursor2DHotSpot[1];
	devSetTexture(0, m_CursorTexture);
	float fw = (float)cw/tw, fh = (float)ch/th;
	float u = (m_Anim%cols)*fw;
	float v = (m_Anim/cols)*fh;
	SetUVMap(u, v, u+fw, v+fh);
	TexMap2DRect(px, py, px+cw, py+ch, ScaleColor(0xffffffff, alpha));
	m_Frame--;
	if(m_Frame<=0){
		m_Anim = (m_Anim+1)%m_Cursor2DAnimNumber;
		m_Frame = m_Cursor2DAnimFrame[m_Anim];
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	初期化
 */
void CCursor::Init(){
	m_Alpha = 1.0f;
	m_Resizing = -2;
	if(!sv3.fWindowed || svw.fActive){
		Center();
		ShowCursor(FALSE);
		Clip();
	}
}

/*
 *	ウィンドウ内にクリップ
 */
void CCursor::Clip(){
	POINT wp = {0, 0};
	ClientToScreen(svw.hWnd, &wp);
	RECT crc = {wp.x, wp.y, wp.x+g_DispWidth, wp.y+g_DispHeight};
	ClipCursor(&crc);
}

/*
 *	カーソルを中央へ
 */
void CCursor::Center(){
	SetCursor(m_Pos.x = g_DispWidth/2, m_Pos.y = g_DispHeight/2);
}

/*
 *	描画
 */
void CCursor::Render(){
	if(m_State) m_Alpha = m_Alpha*CURSOR_HIDE_RATIO;
	else m_Alpha = m_Alpha*CURSOR_HIDE_RATIO+(1.0f-CURSOR_HIDE_RATIO);
	if(m_Resizing<0) g_Skin->m_NormalCursorData.Render(m_Pos, m_Alpha);
	else g_Skin->m_ResizeCursorData[m_Resizing].Render(m_Pos, m_Alpha);
	m_Resizing = -2;
}

/*
 *	入力チェック
 */
void CCursor::ScanInput(
	bool forcelock	//	強制ロック
){
	ScanInputDevice();
	g_Cursor.FixCursor();

	int cx = g_DispWidth/2, cy = g_DispHeight/2;
	m_Delta = GetCursorXY();
	m_Delta.x -= cx;
	m_Delta.y -= cy;
	if(!m_State && !forcelock){
		m_Pos.x += m_Delta.x;
		m_Pos.y += m_Delta.y;
		ValueArea((int *)&m_Pos.x, 0, g_DispWidth-1);
		ValueArea((int *)&m_Pos.y, 0, g_DispHeight-1);
	}
}

/*
 *	カーソル固定
 */
void CCursor::FixCursor(){
	int cx = g_DispWidth/2, cy = g_DispHeight/2;
	SetCursor(cx, cy);
}

/*
 *	ドラッグ開始判定
 */
bool CCursor::CheckDrag(){
	return abs(m_Delta.x)>=CSR_MOVE_TSD || abs(m_Delta.y)>=CSR_MOVE_TSD;
}

/*
 *	ロック
 */
void CCursor::Lock(){
	m_State = 1;
	m_Delta.x = m_Delta.y = 0;
	SetCursor(g_DispWidth/2, g_DispHeight/2);
}

/*
 *	解放
 */
void CCursor::Release(){
	m_State = 0;
}
