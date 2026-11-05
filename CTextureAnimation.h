#ifndef CTEXTUREANIMATION_H_INCLUDED
#define CTEXTUREANIMATION_H_INCLUDED

class CModelPlugin;
class CTextureTransformer;

/*
 *	アニメーションフレーム
 */
class CTexAnimFrame{
	friend class CTextureAnimation;
private:
	int m_FrameLength;					//	フレームの長さ
	string m_TextureFileName;			//	テクスチャファイル名
	LPTEX8 m_FrameTexture;				//	フレームテクスチャ
	CTextureTransformer *m_TexTrans;	//	UV 変換
public:
	CTexAnimFrame();
	CTexAnimFrame(const CTexAnimFrame &);
	CTexAnimFrame(string, int);
	~CTexAnimFrame();
	char *Read(char *);
	void LoadData();
	void Apply(CMesh *, int);
};

/*
 *	テクスチャアニメーション状態
 */
class CTexAnimState{
	friend class CTextureAnimation;
private:
	int m_Frame;	//	フレーム
	int m_Count;	//	カウント
public:
	CTexAnimState(){ Reset(); }
	void Reset(){ m_Frame = m_Count = 0; }
	void Step(){ m_Count++; }
	char *Read(char *);
	void Save(FILE *, char *);
};

//	反復子
typedef list<CTexAnimState>::iterator ITexAnimState;

/*
 *	テクスチャアニメーション
 */
class CTextureAnimation{
private:
	int m_CurrentFrame;					//	現在のフレーム
	CTexAnimState m_PreviewState;		//	プレビュー用状態変数
	string m_AnimationName;				//	アニメーション名
	vector<CTexAnimFrame> m_FrameList;	//	フレームリスト
public:
	char *Read(char *, CModelPlugin *);
	void LoadData();
	CTextureAnimation *Check(const string &name){
		return m_AnimationName==name ? this : NULL;
	}
	void SetState(CTexAnimState *);
	void Apply(CMesh *, int);
	LPTEX8 GetFrameTexture(){
		return m_FrameList.size() ? m_FrameList[m_CurrentFrame].m_FrameTexture : NULL;
	}
};

//	反復子
typedef list<CTextureAnimation>::iterator ITextureAnimation;

#endif
