//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "frame.h"
#include "graphic.h"
#include "view.h"
#include "render.h"
#include "texture.h"
#include "vertex.h"
#include "draw.h"
#include "effect.h"

//	外部グローバル
extern int g_DispWidth;
extern int g_DispHeight;

/*
 *	バックバッファ上のピクセル色を取得
 *
 *	x, y	: ピクセル位置
 *	戻り値: X8R8G8B8フォーマットの色
 *
 *	※負荷が高いため頻繁に使用しない事
 */
D3DCOLOR GetPixelColor(int x, int y){
	//	画面外
	if(x<0 || x>=sv3.width || y<0 || y>=sv3.height)
		return 0;

	//	バックバッファをサーフェイスのコピー
	LPSURF8		pSrc, pDst;
	D3DFORMAT	fmt = sv3.d3dpp.BackBufferFormat;
	RECT		rect = {x, y, x+3, y+3};
	POINT		point = {0, 0};

	sv3.pDev->GetRenderTarget(&pSrc);
	sv3.pDev->CreateImageSurface(4, 4, fmt, &pDst);
	sv3.pDev->CopyRects(pSrc, &rect, 1, pDst, &point);

	//	サーフェイスをロック
	D3DLOCKED_RECT	lrect;
	DWORD			pixel;
	D3DCOLOR		c;

	pDst->LockRect(&lrect, NULL, D3DLOCK_READONLY);
	pixel = *((DWORD *)lrect.pBits);	//	最初のピクセルを取得
	c = GetXRGB32(pixel, fmt);	//	X8R8G8B8にフォーマット変換
	pDst->UnlockRect();

	RELEASE(pSrc);
	RELEASE(pDst);
	return c;
}

/*
 *	レンズフレアを描画
 *
 *	pos	: 光源の位置
 *	size	: フレアサイズ
 *	fWhite: ホワイトアウトのON/OFF
 *
 *	更新するデバイスパラメータ：ワールド変換行列、ブレンドモード、テクスチャー
 */
void RenderLensFlare(VEC3 pos, float size, BOOL fWhite){
	devBLEND_ADD2();	//	加算モード

	//	光源のレンダリング
	//devTransBillboard(pos);
	//TexMap3DRect(VEC3(0, 0, 0), size, size, 0xc0ffe080+(Rand(0x40)<<24));

	VEC3 vLight = pos-GetVPos();		//	カメラから光源へのベクトル
	VEC3 vCamera = GetVDir();			//	カメラの向き
	VEC3 vDist = vLight/5-vCamera*5;	//	フレアの間隔（適当です）

	//	光の入射角を計算
	D3DXVec3Normalize(&vCamera, &vCamera);
	D3DXVec3Normalize(&vLight, &vLight);

	float angle = D3DXVec3Dot(&vLight, &vCamera);

	if(angle>0.9f){
		//	フレアの描画
		DWORD aplus = (DWORD)(max(0.0f, (angle-0.9f)*FRand2(1400.0f, 2200.0f)))<<24;
		devSetTexture(0, NULL);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DCircle(VEC3(0, 0, 0), size/10, 0x00000000+aplus, 0x00804000+aplus);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DHex( VEC3(0, 0, 0), size/8, 0x00000000+aplus, 0x00806000+aplus);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DCircle(VEC3(0, 0, 0), size/13, 0x00000000+aplus, 0x00001040+aplus);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DCircle(VEC3(0, 0, 0), size/7, 0x00000000+aplus, 0x00006020+aplus);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DCircle(VEC3(0, 0, 0), size/20, 0x00008040+aplus, 0x00000000+aplus);

		pos -= vDist;
		devTransBillboard(pos);
		Fill3DCircle(VEC3(0, 0, 0), size/5, 0x00000000, 0x00002000+aplus);

		devBLEND_ALPHA();	//	半透明モード

		//	ホワイトアウト
		if(fWhite && angle>0.9f){
			aplus = (DWORD)(max(0.0f, (angle-0.9f)*1000.0f))<<24;
			Fill2DRect(0, 0, g_DispWidth, g_DispHeight, aplus|0x00ffffff);
		}
	}else{
		devBLEND_ALPHA();	//	半透明モード
	}
}
