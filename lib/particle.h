//	Copyright (c) 2002 Midikyou

#if 0

/*
 *	生成用パラメータ
 */
struct PARTICLE_PARAM{
	float minLife;	//	寿命（最小、最大）
	float maxLife;
	BYTE minR;		//	色（R, G, B, Alpha）
	BYTE maxR;
	BYTE minG;
	BYTE maxG;
	BYTE minB;
	BYTE maxB;
	BYTE minA;
	BYTE maxA;
	float minAP;	//	α値変化量
	float maxAP;
	float minSize;	//	サイズ（半径）
	float maxSize;
	float minSP;	//	サイズ変化量
	float maxSP;
	float minV;		//	速度
	float maxV;
};
/*
 *	保持用パラメータ
 */
struct PARTICLE{
	float		lifeTime;	//	寿命（秒）
	float		age;		//	年齢（秒）
	BYTE		r;			//	R 成分
	BYTE		g;			//	G 成分
	BYTE		b;			//	B 成分
	float		alpha;		//	α値（0-255）
	float		aplus;		//	α値加算量／秒
	float		size;		//	サイズ（半径）
	float		splus;		//	サイズ加算量／秒
	VEC3		pos;		//	位置
	VEC3		vec;		//	移動方向／秒
};
/*
 *	パーティクルエミッタ・クラス
 */
class CParticle{
	PARTICLE *m_pb;	//	粒子データ
	VTX_LX*	m_vt;	//	頂点テーブル
	int		m_num;	//	粒子数
	VEC3	m_pos;	//	粒子発生位置
	VEC3	m_gravity;	//	重力
	float	m_rx[2];//	放出方向の範囲
	float	m_ry[2];
	float	m_rz[2];

	//	コピーコンストラクタ封印
	CParticle& operator = (const CParticle&){return *this;}
public:

	CParticle();
	~CParticle();
	void Generate(int num, VEC3 pos, PARTICLE_PARAM *pp);
	void Regenerate(PARTICLE_PARAM *pp);
	void Delete();
	BOOL Render();

	/*
	 *	重力の設定
	 */
	void SetGravity(VEC3 g){m_gravity = g;}
	/*
	 *	放出方向の範囲設定(X)
	 */
	void SetXRange(float min, float max){m_rx[0] = min, m_rx[1] = max;}
	/*
	 *	放出方向の範囲設定(Y)
	 */
	void SetYRange(float min, float max){m_ry[0] = min, m_ry[1] = max;}
	/*
	 *	放出方向の範囲設定(Z)
	 */
	void SetZRange(float min, float max){m_rz[0] = min, m_rz[1] = max;}
};

#endif
