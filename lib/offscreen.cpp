//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "texture.h"
#include "offscreen.h"

/*
 *	コンストラクタ
 */
COffScreen::COffScreen(){
	m_pTex = NULL;
	m_pRT = NULL;
	m_pZB = NULL;
	m_fRender = FALSE;
}

/*
 *	デストラクタ
 */
COffScreen::~COffScreen(){
	Free();
}

/*
 *	サーフェイスの作成
 *
 *	w, h	: サイズ　※2の乗数のみ
 */
BOOL COffScreen::Create(int w, int h){
	Free();	//	既存なら解放

	HRESULT hr;

	hr = sv3.pDev->CreateTexture(
		w, h, 1, D3DUSAGE_RENDERTARGET,
		sv3.d3dpp.BackBufferFormat,
		D3DPOOL_DEFAULT, &m_pTex);
	if(FAILED(hr)) return FALSE;

	//	何故かD3DUSAGE_DEPTHSTENCILでテクスチャが作成できないので・・・
	hr = sv3.pDev->CreateDepthStencilSurface(
		w, h,
		sv3.d3dpp.AutoDepthStencilFormat,
		sv3.d3dpp.MultiSampleType,
		&m_pZB
	); 
	if(FAILED(hr)) return FALSE;

	m_fRender = TRUE;
	return TRUE;
}

/*
 *	サーフェイスの解放
 */
void COffScreen::Free(){
	m_fRender = FALSE;

	RELEASE(m_pTex);
	RELEASE(m_pZB);
}

/*
 *	レンダリング開始
 */
BOOL COffScreen::Begin(D3DCOLOR c){
	if(!m_fRender) return FALSE;

	//	現在のスクリーン設定を退避
	sv3.pDev->GetRenderTarget(&m_pOldRT);
	sv3.pDev->GetDepthStencilSurface(&m_pOldZB);

	//	新しいスクリーン設定をセット
	m_pTex->GetSurfaceLevel(0, &m_pRT);

	if(FAILED(sv3.pDev->SetRenderTarget(m_pRT, m_pZB)))
		return FALSE;

	//	シーンの開始
	if(!BeginScene(c)) return FALSE;
	return TRUE;
}

/*
 *	レンダリング終了
 */
void COffScreen::End(){
	if(!m_fRender) return;

	//	シーンの終了、スクリーン設定の復元
	sv3.pDev->EndScene();
	RELEASE(m_pRT);

	sv3.pDev->SetRenderTarget(m_pOldRT, m_pOldZB);
	RELEASE(m_pOldRT);
	RELEASE(m_pOldZB);
}
