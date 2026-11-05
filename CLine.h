#ifndef CLINE_H_INCLUDED
#define CLINE_H_INCLUDED

class CPole;
class CLine;
class CScene;
class CLinePlugin;
class CPolePlugin;

/*
 *	架線柱リンク
 */
class CPoleLink{
public:
	int m_Side;		//	サイド
	int m_Track;	//	軌道番号
	CPole *m_Link;	//	リンク
public:
	CPoleLink(){ m_Side = m_Track = NULL; m_Link = NULL; }
	CPoleLink(int, int, CPole *);
	bool IsValid(){ return !!m_Link; }
	void Connect(CLine *);
	void Disconnect(CLine *);
	VEC3 GetPos();
	VEC3 GetOrigPos();
	VEC3 GetRight();
	VEC3 GetUp();
	VEC3 GetDir();
	VEC3 GetOrigDir();
	void Render(D3DCOLOR);
	void RestoreAddress();
	char *Read(char *, char *);
	void Save(FILE *, char *);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	架線柱インスタンス
 */
class CPole{
	friend class CPoleLink;
private:
	static CPole **ms_Root;		//	接続ルート
	static float ms_MinDist;	//	最小検出距離
	static CPoleLink ms_Detect;	//	検出情報
	int m_Selected;				//	選択フラグ
	VEC3 m_Pos;					//	位置
	VEC3 m_Right;				//	右
	VEC3 m_Up;					//	上
	VEC3 m_Dir;					//	方向
	VEC3 m_OrigDir;				//	旧 dir
	CObject m_Object;			//	オブジェクト
	list<CLine *> m_LineList;	//	架線リスト
	CPolePlugin *m_PolePlugin;	//	架線柱プラグイン
	CPole *m_Next;				//	リスト次
public:
	static void SetRoot(CPole **r){ ms_Root = r; }
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool RenderLink();
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static CPoleLink &GetDetect(){ return ms_Detect; }
	CPole();
	CPole(const VEC3 &, const VEC3 &, CPolePlugin *);
	~CPole();
	void Delete(CScene *);
	CPole *Next(){ return m_Next; }
	CPole **NextAdr(){ return &m_Next; }
	CPoleLink CreateLink(int s, int t){ return CPoleLink(s, t, this); }
	void AddLine(CLine *line){ m_LineList.push_back(line); }
	void DeleteLine(CLine *line){ m_LineList.remove(line); }
	VEC3 GetJointPos(int);
	VEC3 GetOrigPos(){ return m_Pos; }
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int s){ m_Selected |= s; }
	void ScanInput(int, VEC3 &, VEC3 &);
	void Render();
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

/*
 *	架線インスタンス
 */
class CLine{
private:
	static CLine **ms_Root;		//	接続ルート
	static float ms_MinDist;	//	最小検出距離
	static CLine *ms_Detect;	//	検出情報
	int m_Selected;				//	選択フラグ
	vector<float> m_LineMapV;	//	架線マッピング V 座標
	VEC3 m_Right;				//	right
	VEC3 m_Dir;					//	dir
	CPoleLink m_Link[2];		//	接続架線柱
	CLinePlugin *m_LinePlugin;	//	架線プラグイン
	CLine *m_Next;				//	リスト次
public:
	static void SetRoot(CLine **r){ ms_Root = r; }
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static CLine *GetDetect(){ return ms_Detect; }
	CLine();
	CLine(CPoleLink, CPoleLink, CLinePlugin *);
	~CLine();
	void Delete();
	CLine *Next(){ return m_Next; }
	CLine **NextAdr(){ return &m_Next; }
	CPoleLink GetLink(int s){ return m_Link[s]; }
	int GetSelectFlag(){ return m_Selected; }
	void AddSelectFlag(int s){ m_Selected |= s; }
	void ScanInput(int, VEC3 &, VEC3 &);
	void Dump();
	void Render();
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *);
};

//	反復子
typedef list<CLine *>::iterator IPLine;

/*
 *	架線柱に接続
 */
inline void CPoleLink::Connect(
	CLine *line	//	架線インスタンス
){
	m_Link->AddLine(line);
}

/*
 *	架線柱の接続解除
 */
inline void CPoleLink::Disconnect(
	CLine *line	//	架線インスタンス
){
	m_Link->DeleteLine(line);
}

/*
 *	接続先座標を求める
 */
inline VEC3 CPoleLink::GetPos(){
	return m_Link->GetJointPos(m_Track);
}

/*
 *	接続先座標を求める
 */
inline VEC3 CPoleLink::GetOrigPos(){
	return m_Link->GetOrigPos();
}

/*
 *	接続先 right ベクトルを求める
 */
inline VEC3 CPoleLink::GetRight(){
	return m_Side ? -m_Link->m_Right : m_Link->m_Right;
}

/*
 *	接続先 up ベクトルを求める
 */
inline VEC3 CPoleLink::GetUp(){
	return m_Link->m_Up;
}

/*
 *	接続先 dir ベクトルを求める
 */
inline VEC3 CPoleLink::GetDir(){
	return m_Side ? -m_Link->m_Dir : m_Link->m_Dir;
}

/*
 *	接続先 dir ベクトルを求める
 */
inline VEC3 CPoleLink::GetOrigDir(){
	return m_Side ? -m_Link->m_OrigDir : m_Link->m_OrigDir;
}

//	外部グローバル
extern vector<CPoleLink> g_LastPole;
extern vector<CPoleLink> g_FinishPole;

#endif
