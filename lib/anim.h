//	Copyright (c) 2002 Midikyou

struct ANIM_STATE{
	VEC3 pos;
	QUAT qua;
};

class CAnim{
	CMesh		*m_pMeshList;	//	メッシュリスト
	ANIM_STATE	*m_pStateTable;	//	メッシュ別の状態(クォータニオン、座標)
	CObject		m_obj;			//	アニメーション全体の座標管理
	int			m_nMeshes;		//	メッシュ数
	int			m_nFrames;		//	フレーム数
	float		m_playFrame;	//	再生中のフレーム
	float		m_speed;		//	ループ毎のフレーム加算値

	void ComputeMatrix(MTX4 *pMtx, int s1, int s2, float rate);
public:
	CAnim();
	~CAnim();

	BOOL Load(LPCSTR strFile);
	void Free();
	BOOL Render(BOOL fLoop);

	/*
	 *	再生速度を指定
	 */
	void SetSpeed(float s){m_speed = s;}
	/*
	 *	再生中のフレームを設定
	 */
	void SetFrame(float f){m_playFrame = max(0.0f, min(f, m_nFrames-1.0f));}
	/*
	 *	再生中のフレームを取得
	 */
	float GetFrame(){return m_playFrame;}
	/*
	 *	フレーム数を取得
	 */
	int GetNumFrames(){return m_nFrames;}
	/*
	座標管操作オブジェクトを取得

	 *	※当り判定用の情報は含まれていません。
	 */
	CObject *GetObject(){return &m_obj;}
	/*
	 *	メッシュリストを取得
	 */
	void GetMeshList(CMesh **ppMesh, int *pNum){
		*ppMesh = m_pMeshList;
		*pNum = m_nMeshes;
	}
};
