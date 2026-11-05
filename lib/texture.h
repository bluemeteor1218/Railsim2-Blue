//	Copyright (c) 2002 Midikyou


using namespace std;

#define UDX_TEXTURE_MEASURE (0)

#if UDX_TEXTURE_MEASURE
#include <vector>
#include <string>
#include "..\CJobTimer.h"
#endif

/*
 *	テクスチャーの読込み
 *
 *	ppTex		: 読込み先
 *	strFile	: ファイル名
 *	cTrans	: 透過色
 *	nMipLv	: ミップマップ・レベル
 *
 *	※サイズが２の乗数でない場合は自動的に透明な領域が追加される。
 */
inline HRESULT LOAD_TEXTURE(
	LPTEX8 *ppTex, LPCSTR strFile, D3DCOLOR cTrans = 0, int nMipLv = 1){
	HRESULT hr;

	hr = D3DXCreateTextureFromFileExA(
		sv3.pDev, strFile, 0, 0, nMipLv, 0,
		D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
		D3DX_DEFAULT, D3DX_DEFAULT,
		cTrans, NULL, NULL, ppTex);
	return hr;
}
//	システムメモリへ
inline HRESULT LOAD_TEXTURE_SYS(
	LPTEX8 *ppTex, LPCSTR strFile, D3DCOLOR cTrans = 0, int nMipLv = 1){
	HRESULT hr;

	hr = D3DXCreateTextureFromFileExA(
		sv3.pDev, strFile, 0, 0, nMipLv, 0,
		D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
		D3DX_DEFAULT, D3DX_DEFAULT,
		cTrans, NULL, NULL, ppTex);
	return hr;
}
//	リソースから
inline HRESULT LOAD_TEXTURE_RES(
	LPTEX8 *ppTex, LPCSTR strRes, D3DCOLOR cTrans = 0, int nMipLv = 1){
	HRESULT hr;

	hr = D3DXCreateTextureFromResourceExA(
		sv3.pDev, NULL, strRes, 0, 0, nMipLv, 0,
		D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
		D3DTEXF_POINT, D3DTEXF_POINT,
		cTrans, NULL, NULL, ppTex);
	return hr;
}

/*
 *	テクスチャ・クラス
 */
class CTexture{
	BOOL m_fCreate;				//	自作フラグ
	LPTEX8			m_pTex;		//	テクスチャ・オブジェクト
	D3DSURFACE_DESC	m_desc;		//	サイズ等の情報

	//	コピーコンストラクタ封印
	CTexture& operator = (const CTexture&){return *this;}
public:
	CTexture();
	~CTexture();

	BOOL Load(LPCSTR strFile, D3DCOLOR cTrans = 0, int nMipLv = 1);
	BOOL LoadResource(LPCSTR strRes, D3DCOLOR cTrans = 0, int nMipLv = 1);
	void Free();

	BOOL Create(int w, int h);
	BOOL DrawInText(int x, int y, LPCSTR str, HFONT hFont,
		D3DCOLOR col = 0xffffffff, D3DCOLOR sdw = 0, int w = -1, int h = -1);
	void Render(int x, int y);

	/*
	 *	サイズの取得（２の乗数サイズに揃えられる）
	 *
	 *	pSize	: サイズの格納先
	 */
	void GetSize(int *pW, int *pH){*pW = m_desc.Width, *pH = m_desc.Height;}
	/*
	 *	テクスチャ・オブジェクトの取得
	 */
	LPTEX8 GetObject(){ return m_pTex; }
	/*
	 *	サーフェイスの取得
	 */
	void GetSurface(LPSURF8 *ppSur){m_pTex->GetSurfaceLevel(0, ppSur);}
};

//	テクスチャリストの要素
struct TEXINFO{
	LPTEX8 pTex;
	string strName;
	int nRef;
	D3DCOLOR cTrans;
	int nMipLv;
	TEXINFO *pNext;
};

/*
 *	テクスチャリスト・クラス (CMeshクラスで使用)
 *	・同じテクスチャーが多重にロードされないようにする。
 *	・参照数を監視し、使用されていないテクスチャーを解放する。
 */
class CTexList{
	TEXINFO *m_pList;	//	リストの先頭
public:
	CTexList();
	~CTexList();
	LPTEX8 Get(BOOL fRes, LPCSTR strName, D3DCOLOR cTrans = 0, int nMipLv = 1);
	void Release(LPTEX8 pTex);
};

/*
 *	テクスチャステージ・ステートの設定
 *
 *	stage	：ステージ
 *	param	：パラメータ
 *	state	：状態
 */
inline void devSetTexState(DWORD stage, D3DTEXTURESTAGESTATETYPE pram, DWORD state){
	sv3.pDev->SetTextureStageState(stage, pram, state);
}

/*
 *	テクスチャの設定
 *
 *	n		: ステージ
 *	pTex	: テクスチャ
 */
inline void devSetTexture(DWORD n, LPTEX8 pTex){
#if UDX_TEXTURE_MEASURE
	TIMER_RAII("devSetTexture");
#endif
	sv3.pDev->SetTexture(n, pTex);
}

/*
 *	テクスチャの取得
 *
 *	n		: ステージ
 */
inline LPTEX8 devGetTexture(DWORD n){
	IDirect3DBaseTexture8 *pTex;
	sv3.pDev->GetTexture(n, &pTex);
	return (LPTEX8)pTex;
}

/*
 *	テクスチャーカラーの設定
 *
 *	stage	: テクスチャーステージ
 *	op	: 演算タイプ
 *	arg1	: 引数１
 *	arg2	: 引数２
 */
inline void devSetTexColor(
	DWORD stage,
	DWORD op = D3DTOP_MODULATE,
	DWORD arg1 = D3DTA_TEXTURE,
	DWORD arg2 = D3DTA_DIFFUSE	){
	devSetTexState(stage, D3DTSS_COLOROP, op);
	devSetTexState(stage, D3DTSS_COLORARG1, arg1);
	devSetTexState(stage, D3DTSS_COLORARG2, arg2);
}

//	標準のマッピング
#define devTEX_SINGLE() devSetTexState(1, D3DTSS_COLOROP, D3DTOP_DISABLE)
//	ライトマッピング（シングルパス）
#define devTEX_LIGHTMAP() devSetTexColor(1, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT)
//	環境マッピング（シングルパス）
#define devTEX_ENVMAP() devSetTexColor(1, D3DTOP_ADDSMOOTH, D3DTA_TEXTURE, D3DTA_CURRENT)

/*
 *	テクスチャーαの設定
 *
 *	stage	: テクスチャーステージ
 *	op	: 演算タイプ
 *	arg1	: 引数１
 *	arg2	: 引数２
 */
inline void devSetTexAlpha(
	DWORD stage,
	DWORD op = D3DTOP_MODULATE,
	DWORD arg1 = D3DTA_TEXTURE,
	DWORD arg2 = D3DTA_DIFFUSE	){
	devSetTexState(stage, D3DTSS_ALPHAOP, op);
	devSetTexState(stage, D3DTSS_ALPHAARG1, arg1);
	devSetTexState(stage, D3DTSS_ALPHAARG2, arg2);
}

/*
 *	テクスチャーフィルターの設定
 *
 *	stage	: テクスチャーステージ
 *	type	: フィルタータイプ
 *
 *	D3DTEXF_NONE	: フィルタなし
 *	D3DTEXF_POINT	: 最近点補完
 *	D3DTEXF_LINEAR	: 線形補完
 *	etc...
 */
inline void devSetTexFilter(DWORD stage, DWORD type){
	devSetTexState(stage, D3DTSS_MAGFILTER, type);
	devSetTexState(stage, D3DTSS_MINFILTER, type);
	devSetTexState(stage, D3DTSS_MIPFILTER, type);
}

#define devTEX_NONE(stage) devSetTexFilter(stage, D3DTEXF_NONE)
#define devTEX_POINT(stage) devSetTexFilter(stage, D3DTEXF_POINT)
#define devTEX_LINEAR(stage) devSetTexFilter(stage, D3DTEXF_LINEAR)

/*
 *	テクスチャーアドレッシングモードの設定
 *
 *	stage	: テクスチャーステージ
 *	mode	: アドレッシングモード
 *
 *	D3DTADDRESS_WRAP		: ラップ（デフォルト）
 *	D3DTADDRESS_MIRROR		: ミラー
 *	D3DTADDRESS_CLAMP		: クランプ
 *	D3DTADDRESS_BORDER		: 境界色
 *	D3DTADDRESS_MIRRORONCE	: ミラー２
 */
inline void devSetTexAddress(DWORD stage, D3DTEXTUREADDRESS mode){
	devSetTexState(stage, D3DTSS_ADDRESSU, mode);
	devSetTexState(stage, D3DTSS_ADDRESSV, mode);
}

#define devTEX_WRAP()		devSetTexAddress(0, D3DTADDRESS_WRAP)
#define devTEX_MIRROR()		devSetTexAddress(0, D3DTADDRESS_MIRROR)
#define devTEX_CLAMP()		devSetTexAddress(0, D3DTADDRESS_CLAMP)
#define devTEX_BORDER()		devSetTexAddress(0, D3DTADDRESS_BORDER)
#define devTEX_MIRROR2()	devSetTexAddress(0, D3DTADDRESS_MIRRORONCE)

/*
 *	テクスチャートランスフォームのON/OFF
 *
 *	stage	: テクスチャーステージ
 *	f		: フラグ
 */
inline void devSetTexTrans(DWORD stage, BOOL f){
	devSetTexState(
		stage,
		D3DTSS_TEXTURETRANSFORMFLAGS, f ? D3DTTFF_COUNT2 : D3DTTFF_DISABLE);
}

/*
 *	テクスチャートランスフォーム
 *
 *	stage	: テクスチャーステージ
 *	pMtx	: 変換行列
 *
 *	※事前にdevSetTexTrans()でTUREを設定すること。
 */
inline void devTexTransform(DWORD stage, MTX4 *pMtx){
	sv3.pDev->SetTransform(
		(D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage), pMtx);
}

/*
 *	環境マップの設定
 *
 *	stage	: テクスチャーステージ
 *	f		: フラグ
 */
inline void devSetEnvMap(DWORD stage, BOOL f){
	devSetTexTrans(stage, f);
	devTexTransform(stage, f
		? &MTX4(
			0.5f,  0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.5f,  0.5f, 1.0f, 0.0f,
			0.0f,  0.0f, 0.0f, 1.0f)
		: &MTX_FRONT);
	devSetTexState(stage, D3DTSS_TEXCOORDINDEX,
		f ? D3DTSS_TCI_CAMERASPACENORMAL : D3DTSS_TCI_PASSTHRU);
}

//	関数宣言
D3DCOLOR CheckTexTrans(LPCSTR str);
void CalcTextRect(int *w, int *h, LPCSTR str, HFONT hFont);
