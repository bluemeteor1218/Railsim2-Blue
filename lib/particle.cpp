//	Copyright (c) 2002 Midikyou

#if 0

#include "headers.h"
#include "debug.h"
#include "frame.h"
#include "graphic.h"
#include "view.h"
#include "vertex.h"
#include "particle.h"

/*
 *	コンストラクタ
 */
CParticle::CParticle(){
	m_vt = NULL;
	m_pb = NULL;
	m_num = 0;
	m_gravity = VEC3(0, 0, 0);

	SetXRange(-1, 1);
	SetYRange(-1, 1);
	SetZRange(-1, 1);
}

/*
 *	デストラクタ
 */
CParticle::~CParticle(){
	Delete();
}

/*
 *	パーティクルの生成
 *
 *	num	: 粒子数
 *	pos	: 発生位置
 *	pp	: パラメータ
 */
void CParticle::Generate(int num, VEC3 pos, PARTICLE_PARAM *pp){
	Delete();	//	既存なら消去

	m_num = num;
	m_pos = pos;

	m_vt = new VTX_LX[m_num*6];
	m_pb = new PARTICLE[m_num];

	//	各粒子のパラメータを設定
	for(int i = 0; i<m_num; i++){
		m_pb[i].lifeTime = FRand2(pp->minLife, pp->maxLife);
		m_pb[i].age = 0;
		m_pb[i].r = Rand2(pp->minR, pp->maxR);
		m_pb[i].g = Rand2(pp->minG, pp->maxG);
		m_pb[i].b = Rand2(pp->minB, pp->maxB);
		m_pb[i].alpha = Rand2(pp->minA, pp->maxA);
		m_pb[i].aplus = FRand2(pp->minAP, pp->maxAP);
		m_pb[i].size = FRand2(pp->minSize, pp->maxSize);
		m_pb[i].splus = FRand2(pp->minSP, pp->maxSP);

		m_pb[i].pos = VEC3(0, 0, 0);
		m_pb[i].vec = VEC3(
			FRand2(m_rx[0], m_rx[1]),
			FRand2(m_ry[0], m_ry[1]),
			FRand2(m_rz[0], m_rz[1]))*FRand2(pp->minV, pp->maxV);
	}
}

/*
 *	パーティクルの再生成
 *
 *	pp	: パラメータ
 */
void CParticle::Regenerate(PARTICLE_PARAM *pp){
	if(!m_pb) return;

	for(int i = 0; i<m_num; i++){
		//	期限切れの粒子を復活させる
		if(m_pb[i].age<m_pb[i].lifeTime) continue;

		m_pb[i].lifeTime = FRand2(pp->minLife, pp->maxLife);
		m_pb[i].age = 0;
		m_pb[i].r = Rand2(pp->minR, pp->maxR);
		m_pb[i].g = Rand2(pp->minG, pp->maxG);
		m_pb[i].b = Rand2(pp->minB, pp->maxB);
		m_pb[i].alpha = Rand2(pp->minA, pp->maxA);
		m_pb[i].aplus = FRand2(pp->minAP, pp->maxAP);
		m_pb[i].size = FRand2(pp->minSize, pp->maxSize);
		m_pb[i].splus = FRand2(pp->minSP, pp->maxSP);

		m_pb[i].pos = VEC3(0, 0, 0);
		m_pb[i].vec = VEC3(
			FRand2(m_rx[0], m_rx[1]),
			FRand2(m_ry[0], m_ry[1]),
			FRand2(m_rz[0], m_rz[1]))*FRand2(pp->minV, pp->maxV);
	}
}

/*
 *	強制的な消滅
 */
void CParticle::Delete(){
	if(m_vt) delete [] m_vt;
	if(m_pb) delete [] m_pb;
	m_num = 0;
}

/*
 *	レンダリング
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス
 *	※事前にテクスチャが指定されている必要があります。
 */
BOOL CParticle::Render(){
	if(!m_pb) return FALSE;

	CVertex v;
	D3DCOLOR c;
	float r, x1, x2, y1, y2, z;
	float sec, age;
	int j = 0;

	//	前フレームからの経過時間の計算
	sec = 1.0f/GetFPS();

	//	全粒子について
	for(int i = 0; i<m_num; i++){
		//	期限切れの粒子はスキップ
		if(m_pb[i].age>=m_pb[i].lifeTime) continue;

		//	正方形を作成
		r = m_pb[i].size;
		x1 = m_pb[i].pos.x-r;
		x2 = m_pb[i].pos.x+r;
		y1 = m_pb[i].pos.y-r;
		y2 = m_pb[i].pos.y+r;
		z = m_pb[i].pos.z;
		c = MAKE_AC((int)m_pb[i].alpha, m_pb[i].r, m_pb[i].g, m_pb[i].b);

		SetVTX_LX(&m_vt[j ], x1, y1, z, c, 0, 1);
		SetVTX_LX(&m_vt[j+1], x1, y2, z, c, 0, 0);
		SetVTX_LX(&m_vt[j+2], x2, y1, z, c, 1, 1);
		SetVTX_LX(&m_vt[j+3], x2, y2, z, c, 1, 0);
		SetVTX_LX(&m_vt[j+4], x2, y1, z, c, 1, 1);
		SetVTX_LX(&m_vt[j+5], x1, y2, z, c, 0, 0);
		j += 6;

		//	時間の経過に伴うパラメータの更新
		m_pb[i].age		+= sec;
		m_pb[i].alpha	+= m_pb[i].aplus*sec;
		m_pb[i].size	+= m_pb[i].splus*sec;

		if(m_pb[i].alpha<0) m_pb[i].alpha = 0;
		if(m_pb[i].size <0) m_pb[i].size = 0;

		age = m_pb[i].age;

		m_pb[i].pos = m_pb[i].vec*age+m_gravity*age*age;
	}
	//	粒子が1つでも生き残っている
	if(j!=0){
		//	常にカメラの方を向かせる
		MTX4 mtx = sv3.mtxViewInv;
		mtx._41 = m_pos.x;
		mtx._42 = m_pos.y;
		mtx._43 = m_pos.z;
		devTransform(&mtx);

		//	レンダリング
		v.Create(m_vt, FVF_LX, sizeof(VTX_LX)*j);
		v.RenderTL();
		return FALSE;
	}
	return TRUE;
}

#endif
