#ifndef CPIER_H_INCLUDED
#define CPIER_H_INCLUDED

class CPierPlugin;

/*
 *	橋脚インスタンス
 */
class CPier{
private:
	static CPier **ms_Root;		//	接続ルート
	static float ms_MinDist;	//	最小検出距離
	static CPier *ms_Detect;	//	検出情報
	int m_Selected;				//	選択フラグ
	bool m_Valid;				//	建設有効フラグ
	float m_SurfaceAlt;			//	地形検出高度
	float m_PierArea;			//	橋脚部長さ
	VEC3 m_SurfaceHit;			//	地表投影座標
	CObject m_JointObject;		//	ジョイント部オブジェクト
	CObject m_HeadObject;		//	ヘッド部オブジェクト
	CObject m_BaseObject;		//	基礎部オブジェクト
	VEC3 m_PierBegin;			//	橋脚部開始座標
	VEC3 m_PierEnd;				//	橋脚部終了座標
	VEC3 m_PierRight;			//	橋脚部 right
	VEC3 m_PierUp;				//	橋脚部 up
	VEC3 m_PierDir;				//	橋脚部 dir
	CPierPlugin *m_PierPlugin;	//	橋脚プラグイン
	CPier *m_Next;				//	リスト次
public:
	static void SetRoot(CPier **r){ ms_Root = r; }
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static CPier *GetDetect(){ return ms_Detect; }
	CPier();
	CPier(VEC3, VEC3, VEC3, float, CPierPlugin *);
	~CPier();
	void SetMesh();
	bool Init(float *);
	void Delete();
	CPier *Next(){ return m_Next; }
	CPier **NextAdr(){ return &m_Next; }
	bool Confirm();
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int s){ m_Selected |= s; }
	void ScanInput(int, VEC3 &, VEC3 &);
	void Dump(int prev = 4);
	void Render();
	char *Read(char *);
	void Save(FILE *);
};

#endif
