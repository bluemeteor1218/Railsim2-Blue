//	Copyright (c) 2002 Midikyou

/*
 *	レンダリング・ステートの設定
 *
 *	pram	: パラメータ
 *	state	: 状態
 */
inline void devSetState(D3DRENDERSTATETYPE pram, DWORD state){
	sv3.pDev->SetRenderState(pram, state);
}

/*
 *	レンダリング・ステートの取得
 *
 *	pram	: パラメータ
 */
inline DWORD devGetState(D3DRENDERSTATETYPE pram){
	DWORD state;
	sv3.pDev->GetRenderState(pram, &state);
	return state;
}

/*
 *	変換行列リセット
 */
inline void devResetMatrix(){
	devTransform(&sv3.mtxFront);
}

/*
 *	ライトの使用
 *
 *	f	: TRUE＝ライティングを有効、FALSE＝無効
 */
inline void devSetLighting(BOOL f){
	devSetState(D3DRS_LIGHTING, f);
}
inline BOOL devGetLighting(){
	return (BOOL)devGetState(D3DRS_LIGHTING);
}

/*
 *	ライト（個別）のＯＮ／ＯＦＦ
 *
 *	n	: インデックス
 *	f	: 真理値
 */
inline void devSetLight(int n, BOOL f){
	sv3.pDev->LightEnable(n, f);
}

/*
 *	アンビエントカラー
 */
inline void devSetAmbient(D3DCOLOR c){
	devSetState(D3DRS_AMBIENT, c);
}

/*
 *	マテリアルの設定
 */
inline void devSetMaterial(MAT8 *pMat){
	sv3.pDev->SetMaterial(pMat);
}

void devResetMaterial();
void devSetLineMaterial();

/*
 *	スペキュラーハイライトの使用
 *
 *	f	: 真理値
 */
inline void devSetSpecular(BOOL f){
	devSetState(D3DRS_SPECULARENABLE, f);
}

/*
 *	カリングの使用
 *
 *	f	: 真理値
 */
inline void devSetCulling(BOOL f){
	devSetState(D3DRS_CULLMODE, f ? D3DCULL_CCW : D3DCULL_NONE);
}

/*
 *	シェーディング方法
 *
 *	m	: D3DSHADE_GOURAUD, D3DSHADE_FLAT
 */
inline void devSetShading(DWORD m){
	devSetState(D3DRS_SHADEMODE, m);
}

/*
 *	法線の自動正規化（スケーリングによるシェーディングの減衰を回避）
 *
 *	f	: 真理値
 */
inline void devSetNormalize(BOOL f){
	devSetState(D3DRS_NORMALIZENORMALS, f);
}

/*
 *	Zバッファのクリア
 */
inline void devClearZ(){
	sv3.pDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
}

/*
 *	Zバッファの参照
 *
 *	f	: TRUE＝読込み有効、FALSE＝無効
 */
inline void devSetZRead(BOOL f){
	devSetState(D3DRS_ZENABLE, f);
}
inline BOOL devGetZRead(){
	return (BOOL)devGetState(D3DRS_ZENABLE);
}

/*
 *	Zバッファの書込み
 *
 *	f	: TRUE＝書込み有効、FALSE＝無効
 *
 *	※半透明物体をレンダリングするときはOFFにしておく
 */
inline void devSetZWrite(BOOL f){
	devSetState(D3DRS_ZWRITEENABLE, f);
}
inline BOOL devGetZWrite(){
	return (BOOL)devGetState(D3DRS_ZWRITEENABLE);
}

/*
 *	アルファテストの設定
 *
 *	f	: 真理値
 *	ref	: 基準とするα値
 */
inline void devSetAlphaTest(BOOL f, D3DCMPFUNC func, DWORD ref){
	devSetState(D3DRS_ALPHATESTENABLE, f);

	if(f){
		devSetState(D3DRS_ALPHAFUNC, func);
		devSetState(D3DRS_ALPHAREF, ref);
	}
}

#define devALPHA_PASS()	devSetAlphaTest(FALSE, D3DCMP_ALWAYS, 0)
#define devALPHA_GREAT(ref) devSetAlphaTest(TRUE, D3DCMP_GREATEREQUAL, ref)
/*
 *	ブレンディングファクターの設定
 *
 *	flag	: 真理値
 *	src		: ソース値
 *	dst		: デスティネーション値
 *
 *	D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA	: αブレンド
 *	D3DBLEND_ONE, D3DBLEND_ONE				: 加算ブレンド
 */
inline void devSetBlend(BOOL flag, DWORD src, DWORD dst){
	devSetState(D3DRS_ALPHABLENDENABLE, flag);

	if(flag){
		devSetState(D3DRS_SRCBLEND, src);
		devSetState(D3DRS_DESTBLEND, dst);
	}
}

//	合成なし
#define devBLEND_NONE() devSetBlend(FALSE, 0, 0)
//	αブレンド（標準の半透明合成）
#define devBLEND_ALPHA() devSetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA)
//	明度ブレンド（ソースの色が明るいほど透明）
#define devBLEND_RGB() devSetBlend(TRUE, D3DBLEND_SRCCOLOR, D3DBLEND_INVSRCCOLOR)
//	加算ブレンド（両者の色の足し算）
#define devBLEND_ADD() devSetBlend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE)
//	α加算ブレンド（ソースのα値が高いほど加算される）
#define devBLEND_ADD2() devSetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_ONE)
//	乗算ブレンド（マルチパス・ライトマップなどに使用）
#define devBLEND_MOD() devSetBlend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO)
/*
 *	フォグの設定
 *
 *	flag	: 真理値
 *	c		: 色
 *	start	: 可視距離
 *	end	: 飽和距離
 */
inline void devSetFog(BOOL flag, DWORD c, float start, float end){
	devSetState(D3DRS_FOGENABLE, flag);

	if(flag){
		devSetState(D3DRS_FOGCOLOR,	c);
		devSetState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
		devSetState(D3DRS_FOGSTART,	*(DWORD *)(&start));
		devSetState(D3DRS_FOGEND,		*(DWORD *)(&end));
	}
}

/*
 *	ピクセルフォグの設定
 *
 *	flag	: 真理値
 *	c		: 色
 *	start	: 可視距離
 *	end	: 飽和距離
 */
inline void devSetPixelFog(BOOL flag, DWORD c, float start, float end){
	devSetState(D3DRS_FOGENABLE, flag);

	if(flag){
		devSetState(D3DRS_FOGCOLOR,	c);
		devSetState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
		devSetState(D3DRS_FOGSTART,	*(DWORD *)(&start));
		devSetState(D3DRS_FOGEND,	*(DWORD *)(&end));
	}
}
