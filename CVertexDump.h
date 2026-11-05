#ifndef CVERTEXDUMP_H_INCLUDED
#define CVERTEXDUMP_H_INCLUDED

#define LINE_DUMP_MAX (sv3.capsMaxPrim/2)	//	直線ダンプ最大値
#define QUAD_DUMP_MAX (sv3.capsMaxPrim/6)	//	四角形ダンプ最大値

/*
 *	ディフューズポインタ
 */
class CDiffusePointer{
private:
	D3DCOLOR *m_Pointer;	//	ポインタ
	D3DCOLOR m_Default;		//	デフォルト値
public:
	CDiffusePointer(D3DCOLOR *p){ m_Default = *(m_Pointer = p); }
	void Restore(){ *m_Pointer = m_Default; }
	void Set(D3DCOLOR c){ *m_Pointer = c; }
};

//	反復子
typedef list<CDiffusePointer>::iterator IDiffusePointer;

/*
 *	バーテックスダンパ
 */
class CVertexDump{
protected:
public:
};

/*
 *	直線ダンプ (ライティング済・座標変換済み)
 */
class CLineDumpTL: public CVertexDump{
private:
	int m_LineNum;		//	直線数
	int m_Count;		//	カウンタ
	VTX_TL *m_Buffer;	//	バッファ
	CVertex m_Vertex;	//	バーテックス
	CLineDumpTL *m_Next;	//	次
public:
	CLineDumpTL(int);
	CLineDumpTL(CLineDumpTL *);
	~CLineDumpTL();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(VEC2, D3DCOLOR, VEC2, D3DCOLOR);
	void Preview(VEC2, D3DCOLOR, VEC2, D3DCOLOR);
	void PrepareVertex();
	void Render(bool drawup);
};

/*
 *	直線ダンプ (ライティング済)
 */
class CLineDumpL: public CVertexDump{
private:
	int m_LineNum;		//	直線数
	int m_Count;		//	カウンタ
	VTX_L *m_Buffer;	//	バッファ
	CVertex m_Vertex;	//	バーテックス
	CLineDumpL *m_Next;	//	次
public:
	CLineDumpL(int);
	CLineDumpL(CLineDumpL *);
	~CLineDumpL();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(VEC3, D3DCOLOR, VEC3, D3DCOLOR);
	void Preview(VEC3, D3DCOLOR, VEC3, D3DCOLOR);
	void PrepareVertex();
	void Render(bool drawup);
};

/*
 *	直線ダンプ (未ライティング)
 */
class CLineDumpN: public CVertexDump{
private:
	int m_LineNum;		//	直線数
	int m_Count;		//	カウンタ
	VTX_N *m_Buffer;	//	バッファ
	CVertex m_Vertex;	//	バーテックス
	CLineDumpN *m_Next;	//	次
public:
	CLineDumpN(int);
	CLineDumpN(CLineDumpN *);
	~CLineDumpN();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(VEC3, D3DCOLOR, VEC3, D3DCOLOR);
	void Preview(VEC3, D3DCOLOR, VEC3, D3DCOLOR);
	void PrepareVertex();
	void Render(bool drawup);
};

/*
 *	四角形ダンプ (テクスチャなし)
 */
class CQuadDumpN: public CVertexDump{
private:
	int m_QuadNum;		//	四角形数
	int m_Count;		//	カウンタ
	VTX_N *m_Buffer;	//	バッファ
	CVertex m_Vertex;	//	バーテックス
	CQuadDumpN *m_Next;	//	次
public:
	CQuadDumpN(int);
	CQuadDumpN(CQuadDumpN *);
	~CQuadDumpN();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR,
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR);
	void Preview(
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR,
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR);
	void PrepareVertex();
	void Render(bool drawup);
};

/*
 *	四角形ダンプ (テクスチャあり)
 */
class CQuadDumpNX: public CVertexDump{
private:
	int m_QuadNum;			//	四角形数
	int m_Count;			//	カウンタ
	VTX_NX *m_Buffer;		//	バッファ
	CVertex m_Vertex;		//	バーテックス
	LPTEX8 m_Texture;		//	テクスチャポインタ
	CQuadDumpNX *m_Next;	//	次
public:
	CQuadDumpNX(int, LPTEX8);
	CQuadDumpNX(CQuadDumpNX *);
	~CQuadDumpNX();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float,
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float);
	void Preview(
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float,
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float);
	void PrepareVertex();
	void Render(bool drawup);
};

#endif
