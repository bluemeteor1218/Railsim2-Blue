//	Copyright (c) 2002 Midikyou

class COffScreen{
	LPTEX8	m_pTex;		//	テクスチャー
	LPSURF8	m_pRT;		//	レンダリングターゲット
	LPSURF8	m_pZB;		//	Ｚバッファ
	LPSURF8 m_pOldRT;	//	レンダリングターゲット（退避用）
	LPSURF8 m_pOldZB;	//	Ｚバッファ（退避用）
	BOOL	m_fRender;	//	レンダリング可能か？

public:
	COffScreen();
	~COffScreen();

	BOOL Create(int w, int h);
	void Free();
	BOOL Begin(D3DCOLOR c = 0xff000000);
	void End();

	/*
	 *	テクスチャーの取得
	 */
	LPTEX8 GetTexture(){return m_pTex;}
};
