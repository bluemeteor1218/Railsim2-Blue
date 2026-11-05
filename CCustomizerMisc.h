#ifndef CCUSTOMIZERMISC_H_INCLUDED
#define CCUSTOMIZERMISC_H_INCLUDED

#include "CCustomizer.h"

/*
 *	モデル変更子
 */
class CModelChanger: public CCustomizerBase{
private:
	string m_AltModelName;	//	代替モデルファイル名
	CMesh *m_AltModel;		//	代替モデル
	float m_AltModelScale;	//	代替スケール
public:
	CModelChanger();
	CCustomizerBase *Duplicate(){ return new CModelChanger(*this); }
	char *Read(char *);
	void LoadDataCustomizer(CModelPlugin *);
	int GetTypeFlagCustomizer(){ return CSTM_MODELCHANGER; }
	CMesh *GetMeshCustomizer(float *);
};

/*
 *	影抑制指定子
 */
class CShadowInhibitor: public CCustomizerBase{
private:
	bool m_NoCastShadow;		//	影を落とさない
	bool m_NoReceiveShadow;		//	影を受けない
	bool m_Transparent;			//	透過指定
	vector<int> m_MaterialID;	//	マテリアル番号
public:
	CCustomizerBase *Duplicate(){ return new CShadowInhibitor(*this); }
	char *Read(char *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	環境マップ指定子
 */
class CEnvMapper: public CCustomizerBase{
private:
	vector<int> m_MaterialID;	//	マテリアル番号
public:
	CCustomizerBase *Duplicate(){ return new CEnvMapper(*this); }
	char *Read(char *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	αテスト指定子
 */
class CAlphaTester: public CCustomizerBase{
private:
	vector<int> m_MaterialID;	//	マテリアル番号
public:
	CCustomizerBase *Duplicate(){ return new CAlphaTester(*this); }
	char *Read(char *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	テクスチャ変更子
 */
class CTextureChanger: public CCustomizerBase{
private:
	int m_MaterialID;			//	マテリアル番号
	string m_AltTextureName;	//	代替テクスチャファイル名
	LPTEX8 m_AltTexture;		//	代替テクスチャ
public:
	CCustomizerBase *Duplicate(){ return new CTextureChanger(*this); }
	char *Read(char *);
	void LoadDataCustomizer(CModelPlugin *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	テクスチャトランスフォーマ
 */
class CTextureTransformer: public CCustomizerBase{
private:
	int m_TransformType;	//	変換タイプ
	int m_MaterialID;		//	マテリアル番号
	TTMTX m_Matrix;			//	変換行列
public:
	CCustomizerBase *Duplicate(){ return new CTextureTransformer(*this); }
	char *Read(char *, bool);
	void SetUpMatrix(int, float *);
	void ApplyCustomizer(CMesh *);
	void SetMaterialID(int matid){ m_MaterialID = matid; }
};

/*
 *	アニメーション適用子
 */
class CAnimationApplier: public CCustomizerBase{
private:
	int m_MaterialID;				//	マテリアル番号
	CTextureAnimation *m_Animation;	//	アニメーション
public:
	CCustomizerBase *Duplicate(){ return new CAnimationApplier(*this); }
	char *Read(char *, CModelPlugin *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	アルファ変更子
 */
class CAlphaChanger: public CCustomizerBase{
private:
	int m_MaterialID;	//	マテリアル番号
	float m_AltAlpha;	//	代替アルファ
public:
	CCustomizerBase *Duplicate(){ return new CAlphaChanger(*this); }
	char *Read(char *);
	void ApplyCustomizer(CMesh *);
};

/*
 *	マテリアル変更子
 */
class CMaterialChanger: public CCustomizerBase{
private:
	vector<int> m_MaterialID;	//	マテリアル番号
	MAT8 m_Material;			//	マテリアル
public:
	CCustomizerBase *Duplicate(){ return new CMaterialChanger(*this); }
	char *Read(char *);
	void ApplyCustomizer(CMesh *);
};

#endif
