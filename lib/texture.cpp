//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "vertex.h"
#include "draw.h"
#include "texture.h"

//	内部グローバル
CTexList g_TexList;		//	テクスチャリスト

/*
 *	コンストラクタ
 */
CTexture::CTexture(){
	m_pTex = NULL;
	m_fCreate = FALSE;
}

/*
 *	デストラクタ
 */
CTexture::~CTexture(){
	Free();
}

/*
 *	読込み
 *
 *	strFile	: ファイル名（BMP, PNGなど）
 *	cTrans	: 透過色
 *	nMipLv	: ミップマップＬＶ
 */
BOOL CTexture::Load(LPCSTR strFile, D3DCOLOR cTrans, int nMipLv){
	Free();	//	既存なら解放
	if(!(m_pTex = g_TexList.Get(FALSE, strFile, cTrans, nMipLv))) return FALSE;
	m_pTex->GetLevelDesc(0, &m_desc);
	return TRUE;
}
//	リソース（ビットマップのみ）
BOOL CTexture::LoadResource(LPCSTR strRes, D3DCOLOR cTrans, int nMipLv){
	Free();	//	既存なら解放
	if(!(m_pTex = g_TexList.Get(TRUE, strRes, cTrans, nMipLv))) return FALSE;
	m_pTex->GetLevelDesc(0, &m_desc);
	return TRUE;
}

/*
 *	解放
 */
void CTexture::Free(){
	if(!m_pTex) return;
	if(m_fCreate){
		RELEASE(m_pTex);
	}else{
		m_fCreate = FALSE;
		g_TexList.Release(m_pTex);
		m_pTex = NULL;
	}
}

/*
 *	テクスチャの作成
 */
BOOL CTexture::Create(int w, int h){
	Free();	//	既存なら解放
	m_fCreate = TRUE;

	//	テクスチャの作成
	HRESULT hr;
	int texW = (int)powf(2, ceilf(logf(w)/logf(2)));
	int texH = (int)powf(2, ceilf(logf(h)/logf(2)));

	hr = sv3.pDev->CreateTexture(
		texW, texH, 1, 0, D3DFMT_A4R4G4B4,
		D3DPOOL_MANAGED, &m_pTex);
	if(FAILED(hr)) goto error;

	m_pTex->GetLevelDesc(0, &m_desc);	//	実際のテクスチャサイズを取得
#if 0
	Debug("Allocated Texture %d x %d.\n", m_desc.Width, m_desc.Height);
#endif

error:
	return SUCCEEDED(hr);
}

/*
 *	文字列を書き込む
 *
 *	str		: 文字列（TAB無効）
 *	font	: フォントハンドル
 */
BOOL CTexture::DrawInText(int x, int y, LPCSTR str,
	HFONT hFont, D3DCOLOR col, D3DCOLOR sdw, int w, int h){
	if(!m_pTex) return FALSE;

	HDC hDC = CreateCompatibleDC(NULL);

	HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

	SetMapMode(hDC, MM_TEXT);
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, 0x00ffffff);

	if(w<0){
		//	描画領域の取得
		RECT drawRect = {0, 0, 1, 1};

		DrawText(hDC, str, -1, &drawRect, DT_CALCRECT|DT_LEFT|DT_EXPANDTABS|DT_NOPREFIX);

		w = drawRect.right;
		h = drawRect.bottom;
	}

	//	DIBの作成
	BITMAPINFO bmi;

	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;

	BYTE *pDIB;
	DWORD dibSize = 4*w*h;
	RECT rect = {0, 0, w, h};

	HBITMAP hBM = CreateDIBSection(
		hDC, &bmi, DIB_RGB_COLORS,
		(VOID **)&pDIB, NULL, 0);
	if(!pDIB) return FALSE;

	//	DIBに文字列を描画
	HBITMAP hOldBM = (HBITMAP)SelectObject(hDC, hBM);
	memset(pDIB, 0, dibSize);
	DrawText(hDC, str, -1, &rect, DT_LEFT|DT_EXPANDTABS|DT_NOCLIP|DT_NOPREFIX);

	//	DIB(32bit)をテクスチャ(16bit)へ転送
	D3DLOCKED_RECT lockRect;

	HRESULT hr = m_pTex->LockRect(0, &lockRect, NULL, 0);	//	ロック

	if(SUCCEEDED(hr)){
		int i, j;
		BYTE a = (col&0xff000000)>>24;
		BYTE r = (col&0x00ff0000)>>16;
		BYTE g = (col&0x0000ff00)>>8;
		BYTE b = col&0x000000ff;
		WORD color = ((a&0xf0)<<8)|((r&0xf0)<<4)|(g&0xf0)|((b&0xf0)>>4);	//	16bit化

		WORD *pDst, *pSdw, *pSdw2 /* , *pSdw3 */ ;
		DWORD *pSrc = (DWORD *)pDIB;

		int cx = 0, cy = 0;
		if(x<0){ cx = -x; x = 0; }
		if(y<0){ cy = -y; y = 0; }
		int cw = w-cx, ch = h-cy;
		if(x+cw>m_desc.Width) cw = m_desc.Width-x;
		if(y+ch>m_desc.Height) ch = m_desc.Height-y;

		if(sdw){
			a = (sdw&0xff000000)>>24;
			r = (sdw&0x00ff0000)>>16;
			g = (sdw&0x0000ff00)>>8;
			b = sdw&0x000000ff;
			WORD shadow = ((a&0xf0)<<8)|((r&0xf0)<<4)|(g&0xf0)|((b&0xf0)>>4);	//	16bit化
			for(i = 0; i<ch; i++){
				pDst = (WORD *)lockRect.pBits+(lockRect.Pitch/2)*(y+i)+x;
				pSdw = (WORD *)lockRect.pBits+(lockRect.Pitch/2)*(y+i+1)+x+1;
				pSdw2 = pSdw-1;
				//pSdw3 = pDst+1;
				pSrc = (DWORD *)pDIB+w*(i+cy)+cx;
				if(i){
					*pDst = 0x00000000;
				}else{
					WORD *tmp = pDst;
					for(j = 0; j<=cw; j++) *tmp++ = 0x00000000;
				}
				for(int j = 0; j<cw; j++){
					if(*pSrc){
						*pDst = color; *pSdw = *pSdw2 = /* *pSdw3 = */ shadow;
					}else{
						*pSdw = /* *pSdw2 = *pSdw3 = */ 0x00000000;
					}
					pDst++; pSdw++; pSdw2++; /* pSdw3++; */ pSrc++;
				}
			}
		}else{
			for(i = 0; i<ch; i++){
				pDst = (WORD *)lockRect.pBits+(lockRect.Pitch/2)*(y+i)+x;
				pSrc = (DWORD *)pDIB+w*(i+cy)+cx;
				for(j = 0; j<cw; j++){
					*pDst = *pSrc ? color : 0x00000000;
					pDst++; pSrc++;
				}
			}
		}
		m_pTex->UnlockRect(0);	//	ロック解除
	}
	//	後始末
	SelectObject(hDC, hOldBM);
	SelectObject(hDC, hOldFont);
	DeleteObject(hBM);
	DeleteDC(hDC);

	return TRUE;
}

/*
 *	2D画像としてレンダリング
 *
 *	x, y	: 描画位置
 *	更新するデバイスパラメータ	: テクスチャ
 *
 *	※事前にライティングOFFにすること。
 */
void CTexture::Render(int x, int y){
	if(m_pTex){
		devSetTexture(0, m_pTex);
		TexMap2DRect(x, y, x+m_desc.Width, y+m_desc.Height);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 *
 *	※CMeshクラスからRelease()が呼ばれるのでデストラクタは不要
 */
CTexList::CTexList(){
	m_pList = NULL;
}
/*
 *	デストラクタ
 */
CTexList::~CTexList(){
	while(m_pList) Release(m_pList->pTex);
}

/*
 *	テクスチャをリストから検索し、なければロード
 */
LPTEX8 CTexList::Get(BOOL fRes, LPCSTR strName, D3DCOLOR cTrans, int nMipLv){
	//	リストからテクスチャーを検索
	TEXINFO *p = m_pList;

	//	ファイルならフルパスに変換
	char full[_MAX_PATH];
	if(!fRes){
		_fullpath(full, strName, _MAX_PATH);
		strName = full;
	}

	if(!cTrans) cTrans = CheckTexTrans(strName);

	while(p){
		//	見つかれば参照カウンタを増やし、テクスチャを返す
		if(!_mbsicmp((PUCHAR)p->strName.c_str(), (PUCHAR)strName)
			&& p->cTrans==cTrans && p->nMipLv==nMipLv){
			//	Debug("[%s] is in texture-list.\n", strName); /*デバッグ*/
			p->nRef++;
			return p->pTex;
		}
		p = p->pNext;
	}
	//	見つからなければロード
	Debug("load(%s) ... ", strName);

	p = new TEXINFO;
	p->pTex = NULL;

	if(fRes ? FAILED(LOAD_TEXTURE_RES(&p->pTex, strName, cTrans, nMipLv))
		: FAILED(LOAD_TEXTURE(&p->pTex, strName, cTrans, nMipLv))){
		Debug("failed.\n");
		return NULL;
	}
	Debug("ok.\n");

	//	リストの先頭に追加、参照カウンタを設定する
	TEXINFO *q = m_pList;
	m_pList = p;
	p->pNext = q;
	p->strName = strName;
	p->nRef = 1;
	p->cTrans = cTrans;
	p->nMipLv = nMipLv;

	return p->pTex;
}

/*
 *	可能ならテクスチャを解放する
 */
void CTexList::Release(LPTEX8 pTex){
	//	テクスチャを検索
	TEXINFO *p = m_pList;
	TEXINFO *q = NULL;

	while(p){
		//	見つかれば参照カウンタを減らす
		if(p->pTex==pTex){
			p->nRef--;

			//	参照がなくなればテクスチャを解放、リストから外す
			if(p->nRef==0){
				Debug("release(%s)\n", p->strName.c_str());
				RELEASE(p->pTex);

				if(p==m_pList){
					//	リストの先頭
					m_pList = p->pNext;
					delete p;
				}else{
					//	途中
					q->pNext = p->pNext;
					delete p;
				}
			}
			break;
		}
		q = p;
		p = p->pNext;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	テクスチャファイル名から透過色を求める
 */
D3DCOLOR CheckTexTrans(LPCSTR str){
	char *ptr = (char *)str, *sharp = NULL, *dot = NULL;
	while(*ptr){
		switch(*ptr){
		case '#':
			sharp = ptr;
			break;
		case '.':
			dot = ptr;
			break;
		case '\\':
			sharp = dot = NULL;
			break;
		}
		ptr = CharNext(ptr);
	}
	if(!sharp) return 0x00000000;
	if(!dot || dot<sharp) dot = ptr;
	int len = dot-sharp;
	if(len==2){
		switch(sharp[1]){
		case 'B': case 'b': return 0xff000000;
		case 'W': case 'w': return 0xffffffff;
		}
	}else if(len==9){
		D3DCOLOR trans;
		if(sscanf(sharp+1, "%x", &trans)==1) return trans;
	}
	return 0x00000000;
}

/*
 *	文字列描画サイズの計算
 */
void CalcTextRect(int *w, int *h, LPCSTR str, HFONT hFont){
	if(!*str){
		*w = *h = 0;
		return;
	}
	HDC hDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
	SetMapMode(hDC, MM_TEXT);
	RECT drawRect = {0, 0, 1, 1};
	DrawText(hDC, str, -1, &drawRect, DT_CALCRECT|DT_LEFT|DT_EXPANDTABS|DT_NOPREFIX);
	*w = drawRect.right;
	*h = drawRect.bottom;
	SelectObject(hDC, hOldFont);
	DeleteDC(hDC);
}
