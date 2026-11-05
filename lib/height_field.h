//	Copyright (c) 2002 Midikyou

class CHeightField{
	CVertex m_vtx;
	float m_scale;
	int	m_width;
	int	m_depth;
	float *m_map;
	BOOL m_fRender;

	//	コピーコンストラクタ封印
	CHeightField& operator = (const CHeightField&){return *this;}
public:
	CHeightField();
	~CHeightField();

	BOOL Generate(
		LPCSTR strFile, VEC3 pos = VEC3(0, 0, 0), float s = 1, float h = 1, D3DCOLOR c = 0xffffffff);

	void Free();
	void Render();
	float GetHeight(float x, float z);
};

VEC3 CalcPlaneNormal(VEC3 v0, VEC3 v1, VEC3 v2);