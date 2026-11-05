//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "frame.h"
#include "graphic.h"
#include "vertex.h"
#include "water_mesh.h"

/*
 *	コンストラクタ
 */
CWaterMesh::CWaterMesh(){
	Init(VEC3(0, 0, 0), 10, 10, 0.005f);
	SetColor(0xffffffff);
	SetSpeed(0.05f);
	SetGain(1.0f);
}

/*
 *	デストラクタ
 */
CWaterMesh::~CWaterMesh(){
}

/*
 *	初期化
 *
 *	pos	: 中心座標
 *	w		: 横幅(X)
 *	h		: 縦幅(Y)
 *	d		: UV誤差（水面の荒れ具合）
 */
void CWaterMesh::Init(VEC3 pos, float w, float h, float d){
	for(int y = 0; y<MESH_H+1; y++){
		for(int x = 0; x<MESH_W+1; x++){
			m_cuv[y][x].x = FRand2(-d, d);
			m_cuv[y][x].y = FRand2(-d, d);
			m_vec[y][x].x = FRand2(-d, d);
			m_vec[y][x].y = FRand2(-d, d);
		}
	}
	m_w = w;
	m_h = h;
	m_dw = m_w/MESH_W;
	m_dh = m_h/MESH_H;
	m_pos = pos;
	m_pos.x -= m_w/2;
	m_pos.y -= m_h/2;
}

/*
 *	水面に衝撃を与える
 *
 *	x		: X座標
 *	y		: Y座標
 *	d		: 変位
 */
void CWaterMesh::Impact(float x, float y, float d){
	int ix = (int)(x/m_dw);
	int iy = (int)(y/m_dh);

	m_cuv[iy][ix].x += d;
	m_cuv[iy][ix].y += d;
}

/*
 *	描画、パラメータ更新
 */
void CWaterMesh::Render(){
	/*
	 *	以下のUV変化アルゴリズムは http://	homepage1.nifty.com/kaneko/
	 *	にある「金子さん」のプログラムを参考にさせて頂きました。
	 */

	//	１フレーム前のUV座標を保存
	memcpy(m_ouv, m_cuv, sizeof(m_cuv));

	int x, y;

	//	UV座標の差分をとる
	for(y = 1; y<MESH_H; y++){
		for(x = 1; x<MESH_W; x++){
			VEC2 duv(0, 0);

			//	上下左右と平均する
			duv += m_ouv[y+1][x]*0.1f;
			duv += m_ouv[y][x+1]*0.1f;
			duv += m_ouv[y][x-1]*0.1f;
			duv += m_ouv[y-1][x]*0.1f;
			duv -= m_ouv[y][x]*0.4f;

			//	UVの差だけ速度を加算、速度分だけUVを修正
			m_vec[y][x] += duv;
			m_cuv[y][x] += m_vec[y][x]*m_speed;
			m_vec[y][x] *= m_gain;	//	減衰
		}
	}
	int idx = 0;
	float x1, y1, x2, y2, u1, u2, v1, v2;

	//	頂点情報の作成
	for(y = 0; y<MESH_H; y++){
		for(x = 0; x<MESH_W; x++){
			idx = (y*MESH_W+x)*6;

			x1 = m_pos.x+x*m_dw;
			y1 = m_pos.y+y*m_dh;
			x2 = x1+m_dw;
			y2 = y1+m_dh;
			u1 = (float)x/MESH_W;
			v1 = (float)y/MESH_H;
			u2 = (float)(x+1)/MESH_W;
			v2 = (float)(y+1)/MESH_H;

			SetVTX_LX(
				&m_vt[idx ], x1, y1, 0, m_color,
				u1+m_cuv[y ][x ].x, 1-(v1+m_cuv[y ][x ].y));
			SetVTX_LX(
				&m_vt[idx+1], x1, y2, 0, m_color,
				u1+m_cuv[y+1][x ].x, 1-(v2+m_cuv[y+1][x ].y));
			SetVTX_LX(
				&m_vt[idx+2], x2, y1, 0, m_color,
				u2+m_cuv[y ][x+1].x, 1-(v1+m_cuv[y ][x+1].y));
			SetVTX_LX(
				&m_vt[idx+3], x2, y2, 0, m_color,
				u2+m_cuv[y+1][x+1].x, 1-(v2+m_cuv[y+1][x+1].y));
			m_vt[idx+4] = m_vt[idx+2];
			m_vt[idx+5] = m_vt[idx+1];
		}
	}
	//	頂点バッファの作成、レンダリング
	m_vtx.Create(m_vt, FVF_LX, sizeof(m_vt));
	m_vtx.RenderTL();
}
