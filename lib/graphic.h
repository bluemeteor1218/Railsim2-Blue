//	Copyright (c) 2002 Midikyou

#define SQRT2	1.41421356f	//	√2
//	πはD3DX_PIを使用する

/*
 *	宣言
 */

//	６面体の頂点保持用
struct BOX8{
	VEC3 v[8];
};

struct SYSVALUE_3D{
	LPDIRECT3D8				pD3D;	//	Direct3D本体
	LPDIRECT3DDEVICE8		pDev;	//	3Dデバイス
	D3DPRESENT_PARAMETERS	d3dpp;	//	スクリーンのフォーマット
	LPD3DXSPRITE			pSpr;	//	スプライト操作用

	MTX4 mtxWorld;	//	ワールド変換マトリクス
	MTX4 mtxView;	//	ビュー変換マトリクス
	MTX4 mtxViewInv;//	上記の逆行列
	MTX4 mtxProj;	//	プロジェクション変換マトリクス
	MTX4 mtxVPort;	//	ビューポートマトリクス
	MTX4 mtxWtoS;	//	ワールド→スクリーン変換用
	MTX4 mtxStoW;	//	スクリーン→ワールド変換用

	MTX4 mtxFront;	//	向きマトリクス
	MTX4 mtxRear;	//	
	MTX4 mtxLeft;	//	
	MTX4 mtxRight;	//	
	MTX4 mtxTop;	//	
	MTX4 mtxBottom;	//	

	MTX4 mtxOld;	//	マトリクス保存用
	MTX4 mtxTmp;	//	作業用マトリクス
	VEC3 vecTmp;	//	作業用ベクトル

	float u[2];		//	UV座標
	float v[2];

	UINT iAdapter;	//	アダプターID
	BOOL fWindowed;	//	スクリーンモード
	char type[8];	//	3Dデバイスのタイプ
	int width;		//	スクリーンの横幅（g_winWとは必ずしも一致しない）
	int height;	//	スクリーンの縦幅（g_winHとは必ずしも一致しない）
	D3DFORMAT format;	//	スクリーンのフォーマット

	DWORD capsMaxPrim;	//	同時にレンダリングできるプリミティブ数（頂点／三角？）
	DWORD capsMaxLight;	//	同時にアクティブにできるライト数
	DWORD capsFogVertex;//	頂点フォグのサポート
	DWORD capsFogPixel;	//	テーブル（ピクセル）フォグのサポート
	DWORD capsFogRange;	//	範囲フォグのサポート
	DWORD capsTexMem;	//	テクスチャーメモリー推定値(KB)
	DWORD capsTexWidth;	//	利用可能なテクスチャサイズ
	DWORD capsTexHeight;//	
	DWORD capsTexStage;	//	バインド可能なテクスチャーステージ数
	BOOL capsTexAlpha;	//	アルファのサポート
	BOOL capsTexMipMap;//	ミップマップのサポート
	BOOL capsTexBump;	//	バンプマップのサポート
};
extern SYSVALUE_3D sv3;

BOOL InitDirect3D();
void FreeDirect3D();
BOOL Create3DDevice(int width, int height);
void SelectDisplayAdapter();
BOOL SetPresentParam();
D3DFORMAT FindDepthStencilFormat(D3DFORMAT form);
const char *FormatToString(D3DFORMAT f);

void InitMetrics();
void InitRenderState();
void GetDeviceCaps();

BOOL BeginScene(D3DCOLOR c = 0xff000000);
void EndScene();

D3DCOLOR GetXRGB32(DWORD d, D3DFORMAT fmt);

/*
 *	D3DCOLORの作成
 */
#define MAKE_AC D3DCOLOR_ARGB	//	アルファ指定
#define MAKE_XC D3DCOLOR_XRGB	//	アルファMAXで固定
/*
 *	D3DCOLORVALUEの作成
 *
 *	r		: R値
 *	g		: G値
 *	b		: B値
 *	a		: Alpha値
 */
inline D3DCOLORVALUE MAKE_CV(float r, float g, float b, float a){
	D3DCOLORVALUE cv;
	cv.r = r, cv.g = g, cv.b = b, cv.a = a;
	return cv;
}

/*
 *	座標指定マトリクスを作成
 *
 *	x		: X座標
 *	y		: Y座標
 *	z		: Z座標
 */
inline MTX4 MAKE_MTX_XYZ(float x, float y, float z){
	return MTX4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
}
//	v		:座標
inline MTX4 MAKE_MTX_VEC3(VEC3 v){
	return MTX4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, v.x, v.y, v.z, 1);
}

/*
 *	向き指定マトリクス
 */
#define MTX_FRONT		sv3.mtxFront
#define MTX_REAR		sv3.mtxRear
#define MTX_LEFT		sv3.mtxLeft
#define MTX_RIGHT		sv3.mtxRight
#define MTX_TOP			sv3.mtxTop
#define MTX_BOTTOM		sv3.mtxBottom
#define MTX_VIEW_INV	sv3.mtxViewInv

/*
 *	行列変換の実行
 */
inline void devTransform(MTX4 *pMtx){
	sv3.pDev->SetTransform(D3DTS_WORLD, pMtx);
}

/*
 *	float to DWORD transform
 */
inline DWORD F2DW(float v){
	return *(DWORD *)&v;
}
