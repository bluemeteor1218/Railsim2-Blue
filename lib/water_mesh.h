//	Copyright (c) 2002 Midikyou

#define MESH_W 20	//	メッシュの分割数
#define MESH_H 20

class CWaterMesh{
	CVertex m_vtx;	//	頂点バッファ
	VTX_LX m_vt[MESH_H*MESH_W*6];	//	頂点テーブル

	VEC2 m_cuv[MESH_H+1][MESH_W+1];	//	UVオフセット量
	VEC2 m_ouv[MESH_H+1][MESH_W+1];	//	１フレーム前のUVオフセット量
	VEC2 m_vec[MESH_H+1][MESH_W+1];	//	UV変化速度

	VEC3 m_pos;	//	位置
	float m_w;		//	横幅
	float m_h;		//	縦幅
	float m_dw;		//	１マスの横幅
	float m_dh;		//	１マスの縦幅
	float m_speed;	//	UV変化速度の乗算量
	float m_gain;	//	減衰（利得）率
	D3DCOLOR m_color;	//	色

	//	コピーコンストラクタ封印
	CWaterMesh& operator = (const CWaterMesh&){return *this;}
public:
	CWaterMesh();
	~CWaterMesh();
	void Init(VEC3 pos, float w, float h, float d);
	void Render();
	void Impact(float x, float y, float d);

	/*
	 *	色の設定
	 */
	void SetColor(D3DCOLOR c){m_color = c;}
	/*
	 *	色の取得
	 */
	D3DCOLOR GetColor(){return m_color;}
	/*
	 *	速度の設定
	 */
	void SetSpeed(float speed){m_speed = speed; if(m_speed<0) m_speed = 0;}
	/*
	 *	速度の取得
	 */
	float GetSpeed(){return m_speed;}
	/*
	 *	減衰（利得）率の設定
	 */
	void SetGain(float b){m_gain = b;}
	/*
	 *	減衰（利得）率の取得
	 */
	float GetGain(){return m_gain;}
};