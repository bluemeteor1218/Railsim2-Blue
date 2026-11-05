//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "texture.h"
#include "sprite.h"

/*
 *	スプライトの使用開始
 */
void BeginSprite(){
	sv3.pSpr->Begin();
}

/*
 *	スプライトの使用終了
 */
void EndSprite(){
	sv3.pSpr->End();
}

/*
 *	コンストラクタ
 */
CSprite::CSprite(){
	m_pTex = NULL;
	m_fRender = FALSE;
	m_fTest = FALSE;

	SetRect(0, 0, 0, 0);
	SetTestRect(0, 0, 0, 0);
	SetScale(VEC2(1, 1));
	SetCenter(VEC2(0, 0));
	SetPos(VEC2(0, 0));
	SetColor(MAKE_XC(255, 255, 255));
}

/*
 *	デストラクタ
 */
CSprite::~CSprite(){
}

/*
 *	テクスチャーを関連付け
 *
 *	pTex	: テクスチャ
 */
void CSprite::SetTexture(CTexture *pTex){
	int w, h;

	m_pTex = pTex;

	//	テクスチャーサイズを取得（2の乗数になる）
	m_pTex->GetSize(&w, &h);
	SetRect(0, 0, w, h);
	SetTestRect(0, 0, w, h);

	m_center = VEC2((w/2.0f), (h/2.0f));
	m_fRender = TRUE;
	m_fTest = TRUE;
}

/*
 *	レンダリング
 */
void CSprite::Render(){
	if(!m_pTex || !m_fRender) return;

	sv3.pSpr->Draw(
		m_pTex->GetObject(), &m_rect, &m_scale,
		&VEC2(m_center.x*m_scale.x, m_center.y*m_scale.y), m_rot,
		&m_pos, m_color);
}

/*
 *	文字列を描画
 *
 *	str	: 文字列
 *
 *	※テクスチャーが128x128で、ASCIIコード順にフォントが格納されていること。
 */
void CSprite::DrawString(const char *str){
	int i = 0;
	int tx, ty;
	int x = m_pos.x, y = m_pos.y;

	while(1){
		if(!str[i] || i>4096){
			break;
		}else if(str[i]=='\n'){
			x = m_pos.x; y += 16; i++; continue;
		}else if(str[i]=='\t'){
			x += 64-x%64; i++; continue;
		}

		tx = (str[i]%16)*8;
		ty = (str[i]/16)*16;

		SetRect(tx, ty, tx+8, ty+16);
		SetPos(VEC2(x, y));
		Render();

		i++;
		x += 8;
	}
}

/*
 *	当り判定
 *
 *	※回転は考慮していない。
 */
BOOL CSprite::CollisionTest(CSprite *pSpr){
	if(!m_pTex || !m_fTest) return FALSE;
	if(!pSpr->m_pTex || !pSpr->m_fTest) return FALSE;

	int dx1 = (int)(m_pos.x+m_cr.left *m_scale.x);
	int dx2 = (int)(m_pos.x+m_cr.right *m_scale.x);
	int dy1 = (int)(m_pos.y+m_cr.top *m_scale.y);
	int dy2 = (int)(m_pos.y+m_cr.bottom*m_scale.y);

	int sx1 = (int)(pSpr->m_pos.x+pSpr->m_cr.left *pSpr->m_scale.x);
	int sx2 = (int)(pSpr->m_pos.x+pSpr->m_cr.right *pSpr->m_scale.x);
	int sy1 = (int)(pSpr->m_pos.y+pSpr->m_cr.top *pSpr->m_scale.y);
	int sy2 = (int)(pSpr->m_pos.y+pSpr->m_cr.bottom*pSpr->m_scale.y);

	return sx1<dx2 && sx2>dx1 && sy1<dy2 && sy2>dy1 ? TRUE : FALSE;
}
