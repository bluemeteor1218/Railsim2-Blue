//	Copyright (c) 2002 Midikyou

typedef LPDIRECT3DVERTEXBUFFER8 LPVB8;
typedef D3DPRIMITIVETYPE PRIMTYPE;

void CalcNormal(VEC3 t[3], VEC3 *n);

/*
 *	頂点フォーマット
 *
 *	※ライティング済み頂点を使用する場合はライティングをOFFする。
 */

//	座標3D変換済み、ライティング済み
#define FVF_TL (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

struct VTX_TL{
	FLOAT x, y, z;
	FLOAT rhw;
	DWORD d;
};

//	座標3D変換済み、ライティング済み、テクスチャ有り
#define FVF_TLX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)

struct VTX_TLX{
	FLOAT x, y, z;
	FLOAT rhw;
	DWORD d;
	FLOAT u;
	FLOAT v;
};

//	ライティング済み
#define FVF_L (D3DFVF_XYZ|D3DFVF_DIFFUSE)

struct VTX_L{
	FLOAT x, y, z;
	DWORD d;
};

//	ライティング済み、テクスチャ有り
#define FVF_LX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

struct VTX_LX{
	FLOAT x, y, z;
	DWORD d;
	FLOAT u, v;
};

inline void SetVTX_LX(VTX_LX *vt, float x, float y, float z, D3DCOLOR d, float u, float v){
	vt->x = x, vt->y = y, vt->z = z, vt->d = d, vt->u = u, vt->v = v;
}

void SetRectTL_LX(VTX_LX *vt, int x1, int y1, int x2, int y2, int z, D3DCOLOR c);

//	ライティング済み、テクスチャ×２
#define FVF_LX2 (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2)

struct VTX_LX2{
	FLOAT x, y, z;
	DWORD d;
	FLOAT u1, v1;
	FLOAT u2, v2;
};

//	未ライティング
#define FVF_N (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE)

struct VTX_N{
	FLOAT x, y, z;
	VEC3 n;
	DWORD d;
};

inline void SetVTX_N(
	VTX_N *vt, float x, float y, float z, VEC3 n, D3DCOLOR d){
	vt->x = x, vt->y = y, vt->z = z, vt->n = n, vt->d = d;
}

//	未ライティング、テクスチャ有り
#define FVF_NX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1)

struct VTX_NX{
	FLOAT x, y, z;
	VEC3 n;
	DWORD d;
	FLOAT u, v;
};

inline void SetVTX_NX(
	VTX_NX *vt, float x, float y, float z, VEC3 n, D3DCOLOR d, float u, float v){
	vt->x = x, vt->y = y, vt->z = z, vt->n = n, vt->d = d, vt->u = u, vt->v = v;
}

//	未ライティング、テクスチャ×２
#define FVF_NX2 (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX2)

struct VTX_NX2{
	FLOAT x, y, z;
	VEC3 n;
	DWORD d;
	FLOAT u1, v1;
	FLOAT u2, v2;
};

//	頂点クラス
class CVertex{
	LPVB8 m_pVB;	//	頂点バッファ
	DWORD m_fvf;	//	頂点フォーマット
	UINT m_stride;	//	次の頂点データまでのバイト数
	UINT m_num;		//	頂点数

	//	コピーコンストラクタ封印
	CVertex& operator = (const CVertex&){return *this;}
public:
	CVertex();
	~CVertex();

	BOOL Create(LPVOID pSrc, DWORD fvf, UINT size);
	void Free();
	BOOL Lock(LPVOID *ppBuf);
	void Unlock();

	void RenderPL();
	void RenderLL();
	void RenderLS(UINT count);
	void RenderTL();
	void RenderTS(UINT count);
	void RenderTF(UINT count);
	void Render(PRIMTYPE type, UINT count);

	/*
	 *	頂点フォーマットの取得
	 */
	DWORD GetFVF(){return m_fvf;}
	/*
	 *	頂点数の取得
	 */
	UINT Count(){return m_num;}
	/*
	 *	頂点バッファの取得
	 */
	LPVB8 GetObject(){return m_pVB;}
};
