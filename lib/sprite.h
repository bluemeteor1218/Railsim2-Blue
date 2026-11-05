//	Copyright (c) 2002 Midikyou

class CSprite{
	CTexture*	m_pTex;		//	テクスチャ・クラスのオブジェクト
	RECT		m_rect;		//	転送元の矩形
	RECT		m_cr;		//	当り判定矩形
	VEC2		m_scale;	//	スケール
	VEC2		m_center;	//	中心
	VEC2		m_pos;		//	位置
	float		m_rot;		//	回転角
	D3DCOLOR	m_color;	//	乗算色
	BOOL		m_fRender;	//	描画フラグ
	BOOL		m_fTest;	//	当り判定フラグ

public:
	CSprite();
	virtual ~CSprite();
	void SetTexture(CTexture *pTex);
	virtual void Render();
	void DrawString(const char *str);
	BOOL CollisionTest(CSprite *pSpr);

	/*
	 *	　描画フラグの設定
	 */
	void EnableRender(BOOL f){m_fRender = f;}
	/*
	 *	　描画フラグの取得
	 */
	BOOL IsEnableRender(){return m_fRender;}
	/*
	 *	　当り判定フラグの設定
	 */
	void EnableTest(BOOL f){m_fTest = f;}
	/*
	 *	　当り判定フラグの取得
	 */
	BOOL IsEnableTest(){return m_fTest;}
	/*
	 *	　スケールの設定
	 */
	void SetScale(VEC2 s){m_scale = s;}
	/*
	 *	　スケールの取得
	 */
	VEC2 GetScale(){return m_scale;}
	/*
	 *	　位置の設定
	 */
	void SetPos(VEC2 p){m_pos = p;}
	/*
	 *	　位置の取得
	 */
	VEC2 GetPos(){return m_pos;}
	/*
	 *	　移動
	 */
	void Move(VEC2 v){m_pos += v;}
	/*
	 *	　中心座標の設定
	 */
	void SetCenter(VEC2 c){m_center = c;}
	/*
	 *	　中心座標の取得
	 */
	VEC2 GetCenter(){return m_center;}
	/*
	 *	　回転角の設定
	 */
	void SetRot(float r){m_rot = r;}
	/*
	 *	　回転角の取得
	 */
	float GetRot(){return m_rot;}
	/*
	 *	　乗算色の設定
	 */
	void SetColor(D3DCOLOR c){m_color = c;}
	/*
	 *	　乗算色の取得
	 */
	D3DCOLOR GetColor(){return m_color;}
	/*
	 *	　転送元矩形の設定
	 */
	void SetRect(int x1, int y1, int x2, int y2){::SetRect(&m_rect, x1, y1, x2, y2);}
	/*
	 *	　転送元矩形の取得
	 */
	RECT GetRect(){return m_rect;}
	/*
	 *	　当り判定矩形の設定
	 */
	void SetTestRect(int x1, int y1, int x2, int y2){::SetRect(&m_cr, x1, y1, x2, y2);}
	/*
	 *	　当り判定矩形の取得
	 */
	RECT GetTestRect(){return m_cr;}
};

void BeginSprite();
void EndSprite();