#ifndef CSHADOWVOLUME_H_INCLUDED
#define CSHADOWVOLUME_H_INCLUDED

#define TRI_DUMP_MAX (sv3.capsMaxPrim/3)	//	三角形ダンプ最大値

//	シャドウボリューム用 FVF
#define FVF_S (D3DFVF_XYZ)
typedef VEC3 VTX_S;

/*
 *	四角形ダンプ (座標のみ)
 */
class CTriDumpS{
private:
	int m_TriNum;		//	四角形数
	int m_Count;		//	カウンタ
	VTX_S *m_Buffer;	//	バッファ
	CVertex m_Vertex;	//	バーテックス
	CTriDumpS *m_Next;	//	次
public:
	CTriDumpS(int);
	CTriDumpS(CTriDumpS *);
	~CTriDumpS();
	int GetCount(){ return m_Count; }
	void Feed();
	void Add(VEC3, VEC3, VEC3);
	void Preview(VEC3, VEC3, VEC3);
	void PrepareVertex();
	void Render(bool drawup);
};

/*
 *	エッジインデックス
 */
class CEdgeIndex{
	friend class CShadowVolume;
private:
	WORD m_Index1, m_Index2;	//	インデックス
	WORD m_Large, m_Small;		//	大小値
public:
	CEdgeIndex(WORD i1, WORD i2){
		m_Index1 = i1; m_Index2 = i2;
		if(m_Index1<m_Index2){
			m_Large = m_Index2; m_Small = m_Index1;
		}else{
			m_Large = m_Index1; m_Small = m_Index2;
		}
	}
	int Compare(const CEdgeIndex &rhs) const{
		return m_Large==rhs.m_Large && m_Small==rhs.m_Small
			? (m_Index1==rhs.m_Index1 ? 1 : -1) : 0;
	}
	bool operator<(const CEdgeIndex &rhs) const{
		return m_Large<rhs.m_Large || m_Large==rhs.m_Large && m_Small<rhs.m_Small;
	}
};

/*
 *	シャドウボリューム
 */
class CShadowVolume{
private:
	vector<CEdgeIndex> m_TempIndex;	//	作業用インデックスバッファ
	CTriDumpS *m_FaceVolume;		//	面用ボリュームダンパ
public:
	CShadowVolume();
	~CShadowVolume();
    void Reset();
    void BuildFromMesh(CObject *, VEC3);
	void AddFaceEdge(VEC3 &, VEC3 &, VEC3 &);
    void Render();
	void Draw(D3DCOLOR);
};

#endif
