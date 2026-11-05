#ifndef CPROFILEPLUGIN_H_INCLUDED
#define CPROFILEPLUGIN_H_INCLUDED

#include "CPlugin.h"

class CScene;
class CQuadDumpNX;
class CLineDumpN;

/*
 *	デフォルト NULL で初期化されるポインタ
 */
template< typename TYPE >
class MapPtrValue {
private:
	TYPE *m_Ptr;
public:
	MapPtrValue(): m_Ptr(NULL){}
	operator TYPE*(){ return m_Ptr; }
	TYPE* operator->(){ return m_Ptr; }
	TYPE*& operator=(TYPE* p){ return m_Ptr = p; }
	void SafeDelete(){ if(m_Ptr){ delete m_Ptr; m_Ptr = NULL; } }
};

/*
 *	断面頂点データ
 */
class CProfileVertex{
	friend class CProfileFace;
	friend class CProfilePlugin;
private:
	bool m_IgnoreCant;		//	カント無視
	bool m_ReadNormal;		//	法線読込フラグ
	bool m_ShadowDrawed;	//	影描画フラグ
	VEC2 m_Coord;			//	座標
	VEC2 m_Normal;			//	法線
	VEC3 m_TransCoord[2];	//	一時座標
	D3DCOLOR m_Diffuse;		//	頂点色
	float m_TexU;			//	テクスチャ U 座標
public:
	char *Read(char *, bool);
	bool IsSame(CProfileVertex *rhs){
		return m_IgnoreCant==rhs->m_IgnoreCant && m_Coord==rhs->m_Coord;
	}
};

//	反復子
typedef list<CProfileVertex>::iterator IProfileVertex;
typedef list<CProfileVertex *>::iterator IPProfileVertex;

/*
 *	断面辺データ
 */
class CProfileFace{
	friend class CProfilePlugin;
private:
	list<CProfileVertex> m_Vertex;	//	頂点
public:
	char *Read(char *, bool);
};

//	反復子
typedef list<CProfileFace>::iterator IProfileFace;

/*
 *	断面データ
 */
class CProfile{
	friend class CProfilePlugin;
private:
	float m_TexMapVTemp;		//	マッピング位置
	float m_TexVPerMeter;		//	長さ当たり V 座標
	bool m_UseTexture;			//	テクスチャ使用フラグ
	string m_TexFileName;		//	テクスチャファイル名
	LPTEX8 m_Texture;			//	テクスチャ
	list<CProfileFace> m_Face;	//	断面
	map<CScene *, MapPtrValue<CQuadDumpN> > m_DumpN;	//	ダンパ (テクスチャなし)
	map<CScene *, MapPtrValue<CQuadDumpNX> > m_DumpNX;	//	ダンパ (テクスチャあり)
public:
	CProfile();
	~CProfile();
	void ClearAll();
	char *Read(char *);
	void LoadTexture();
	void PrepareDump();
};

//	反復子
typedef list<CProfile>::iterator IProfile;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ワイヤフレーム頂点データ
 */
class CWireframeVertex{
	friend class CProfilePlugin;
private:
	bool m_IgnoreCant;	//	カント無視
	VEC3 m_Coord;		//	座標
	D3DCOLOR m_Diffuse;	//	頂点色
public:
	char *Read(char *);
};

//	反復子
typedef list<CWireframeVertex>::iterator IWireframeVertex;

/*
 *	ワイヤフレーム直線データ
 */
class CWireframeLine{
	friend class CProfilePlugin;
private:
	list<CWireframeVertex> m_Vertex;	//	頂点
public:
	char *Read(char *);
};

//	反復子
typedef list<CWireframeLine>::iterator IWireframeLine;

/*
 *	ワイヤフレームデータ
 */
class CWireframe{
	friend class CProfilePlugin;
private:
	float m_MinInterval;			//	最小間隔
	float m_MaxInterval;			//	最大間隔
	list<CWireframeLine> m_Line;	//	断面
	map<CScene *, MapPtrValue<CLineDumpN> > m_DumpN;	//	ダンパ
public:
	CWireframe();
	~CWireframe();
	void ClearAll();
	char *Read(char *);
	void PrepareDump();
	bool CheckInterval(float l){
		return (m_MinInterval<0.0f || m_MinInterval<=l)
			&& (m_MaxInterval<0.0f || l<m_MaxInterval);
	}
};

//	反復子
typedef list<CWireframe>::iterator IWireframe;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	等間隔配置データ
 */
class CInterval{
	friend class CProfilePlugin;
private:
	bool m_IgnoreCant;		//	カント無視
	float m_IntervalTemp;	//	現在位置
	float m_Interval;		//	間隔
	float m_Offset;			//	オフセット
	float m_ModelScale;		//	モデルスケール
	string m_ModelFileName;	//	モデルファイル名
	CMesh *m_Mesh;			//	メッシュ
	CObject m_Object;		//	オブジェクト
public:
	char *Read(char *);
	void LoadModel();
};

//	反復子
typedef list<CInterval>::iterator IInterval;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	断面プラグイン
 */
class CProfilePlugin: public CPlugin{
protected:
	static vector<CProfileVertex *> ms_TempIndex;	//	作業用インデックスバッファ
	list<CProfileVertex *> m_ProfileVertex;	//	断面頂点リスト
	list<CProfile> m_Profile;				//	断面
	list<CWireframe> m_Wireframe;			//	ワイヤフレーム
	list<CInterval> m_Interval;				//	等間隔配置
public:
	CProfilePlugin(char *id): CPlugin(id){}
	virtual ~CProfilePlugin(){}
	virtual char *DirName() = 0;
	virtual char *TextName2() = 0;
	virtual bool Load() = 0;
	virtual void SetPreview() = 0;
	virtual bool UseTaper(){ return false; }
	virtual float GetTaperZ(){ return 0.0f; }
	bool HasInterval(){ return !!m_Interval.size(); }
	char *ReadProfile(char *);
	void LoadData();
	void Dump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, float, int);
	void Render(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		int, float, MAT8 *altmat = NULL);
	virtual void BeforeDump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &){}
	virtual void AfterDump(VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &){}
	void ResetMapTemp();
	void CopyMapTemp(vector<float> &);
	void AddMapTemp(float);
	void SetMapTemp(vector<float> &);
	void ClearDump();
	void ClearDumpAll();
	void PrepareVertex();
	void RenderAll();
	CPLUGIN_CASTFUNC(CProfilePlugin);
};

/*
 *	断面プラグインリスト
 */
class CProfilePluginList: public CPluginList{
protected:
public:
	virtual char *DirName() = 0;
	virtual char *TextName2() = 0;
	void ClearDump();
	void ClearDumpAll();
	void PrepareVertex();
	void RenderAll();
	virtual CPlugin *NewEntry(char *) = 0;
	CPLUGINLIST_CASTFUNC(CProfilePlugin);
};

//	関数宣言
char *ReadMapVector(char *, char *, vector<float> &);
void SaveMapVector(FILE *, char *, vector<float> &);

#endif
