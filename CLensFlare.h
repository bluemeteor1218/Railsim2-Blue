#ifndef CLENSFLARE_H_INCLUDED
#define CLENSFLARE_H_INCLUDED

class CNamedObject;
class CModelPlugin;
class CHeadlight;

/*
 *	レンズフレア要素
 */
class CFlareElement{
	friend class CLensFlare;
private:
	int m_Type;				//	タイプ
	float m_Distance;		//	距離
	float m_Radius;			//	半径
	D3DCOLOR m_InnerColor;	//	内側の色
	D3DCOLOR m_OuterColor;	//	外側の色
	string m_TexFileName;	//	テクスチャファイル名
	LPTEX8 m_Texture;		//	テクスチャ
public:
	char *Read(char *);
	void LoadData();
	bool operator<(const CFlareElement &rhs){
		return m_Distance<rhs.m_Distance;
	}
};

//	反復子
typedef list<CFlareElement>::iterator IFlareElement;

/*
 *	レンズフレア
 */
class CLensFlare{
private:
	float m_StartAngle;				//	開始角度 (cos)
	float m_Twinkle;				//	点滅度合い
	float m_Inclination;			//	角度の傾き
	list<CFlareElement> m_Flare;	//	フレアリスト
public:
	char *Read(char *);
	void LoadData();
	void Render(VEC3, VEC3, VEC3, float, float);
};

/*
 *	ホワイトアウト
 */
class CWhiteout{
private:
	float m_StartAngle;	//	開始角度 (cos)
	D3DCOLOR m_Color;	//	色
public:
	CWhiteout(){ m_Color = 0; }
	char *Read(char *);
	void Render(VEC3, float);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ヘッドライトインスタンス
 */
class CHeadlightInst{
	friend class CHeadlight;
private:
	VEC3 m_RenderPos;			//	レンダリング用光源座標
	VEC3 m_RenderDir;			//	レンダリング用光源方向
	CHeadlight *m_Headlight;	//	ヘッドライト
public:
	CHeadlightInst(VEC3, VEC3, CHeadlight *);
};

//	反復子
typedef list<CHeadlightInst>::iterator IHeadlightInst;

/*
 *	ヘッドライト
 */
class CHeadlight{
private:
	static list<CHeadlightInst> ms_RenderList;	//	レンダリングリスト
	float m_MaxDistance;	//	最大距離
	VEC3 m_SourceCoord;		//	位置
	VEC3 m_Direction;		//	方向
	CNamedObject *m_Link;	//	接続先オブジェクト
	CLensFlare m_LensFlare;	//	レンズフレア
public:
	static void InitRenderList();
	static void RenderAll();
	char *Read(char *, CModelPlugin *);
	void LoadData();
	bool Register();
	void Render(CHeadlightInst *);
};

//	反復子
typedef list<CHeadlight>::iterator IHeadlight;

#endif
