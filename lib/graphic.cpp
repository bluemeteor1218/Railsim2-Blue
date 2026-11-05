//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "graphic.h"
#include "render.h"
#include "light.h"
#include "texture.h"
#include "font.h"
#include "frame.h"
#include "..\Const.h"

//	内部定数
extern const float CLIP_PLANE_NEAR = 0.5f;		//	前方クリップ面
extern const float CLIP_PLANE_FAR = 10000.0f;	//	後方クリップ面
extern const float FOV_DEF = 0.25f*D3DX_PI;		//	デフォルト視野角

//	外部グローバル
extern char *g_PluginViewArg;
extern int g_DispWidth;
extern int g_DispHeight;
extern bool g_FullScreen;

//	内部グローバル
bool g_StencilEnabled = false;
DWORD g_BufferClearMode;

/*
 *	Direct3Dの初期化
 */
BOOL InitDirect3D(){
	DebugHL();
	Debug("InitDirect3D\n");

	//	Direct3Dの作成
	sv3.pD3D = Direct3DCreate8(D3D_SDK_VERSION);

	ASSERT("Direct3Dの初期化に失敗しました.", sv3.pD3D); 

	//	デバイスの作成
	if(!Create3DDevice(g_DispWidth, g_DispHeight)) return FALSE;

	GetDeviceCaps();	//	デバイス能力の取得

	//	環境設定
	InitMetrics();
	InitRenderState();

	//	関連オブジェクトの作成
	SetDirLight(VEC3(1, -1, 1), MAKE_CV(0.5f, 0.5f, 0.5f, 0.0f));
	D3DXCreateSprite(sv3.pDev, &sv3.pSpr);
	CreateFont(FONT_HEIGHT, 0xffffffff, FW_NORMAL);

	return TRUE;
}

/*
 *	Direct3Dの解放
 */
void FreeDirect3D(){
	DebugHL();
	Debug("FreeDirect3D\n");

	FreeFont();
	RELEASE(sv3.pSpr);
	RELEASE(sv3.pDev);
	RELEASE(sv3.pD3D);
}

/*
 *	3Dデバイスの作成
 *
 *	width	: ビューポート横幅
 *	height: ビューポート縦幅
 */
BOOL Create3DDevice(int width, int height){
	sv3.width = width;
	sv3.height = height;

	SelectDisplayAdapter();	//	ディスプレイアダプタの選択(sv3.iAdapter)
	SetPresentParam();		//	デバイスパラメータの指定(sv3.d3dpp)

	//	ウインドウモードならウインドウを表示する
	if(sv3.fWindowed){
		ShowWindow(svw.hWnd, SW_SHOW);
		UpdateWindow(svw.hWnd);
	}
	HRESULT hr;

	//	TnL HAL Device
	strcpy(sv3.type, "TnLHAL");

	hr = sv3.pD3D->CreateDevice(
		sv3.iAdapter, D3DDEVTYPE_HAL, svw.hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &sv3.d3dpp, &sv3.pDev);

	if(FAILED(hr)){
		//	HAL Device
		strcpy(sv3.type, "HAL");

		hr = sv3.pD3D->CreateDevice(
			sv3.iAdapter, D3DDEVTYPE_HAL, svw.hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &sv3.d3dpp, &sv3.pDev);

		if(FAILED(hr)){
			//	REF Device
			strcpy(sv3.type, "REF");

			hr = sv3.pD3D->CreateDevice(
				sv3.iAdapter, D3DDEVTYPE_REF, svw.hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, &sv3.d3dpp, &sv3.pDev);
			FAILED_ASSERT("3Dデバイスが作成できません.", hr);
		}
	}
	Debug("デバイスタイプ = %s\n", sv3.type);
	return TRUE;
}

/*
 *	ディスプレイアダプタの選択
 */
void SelectDisplayAdapter(){
	//	アダプタ数の取得
	int num = sv3.pD3D->GetAdapterCount();
	Debug("アダプタ数 = %d\n", num);

	//	アダプタの選択
	Debug("アダプタID = ");

	if(num>=2 && CheckArguments("-2nd")){
		Debug("1\n");
		sv3.iAdapter = 1;
	}else{
		Debug("D3DADAPTER_DEFAULT\n");
		sv3.iAdapter = D3DADAPTER_DEFAULT;
	}
	//	アダプタ情報の取得
	D3DADAPTER_IDENTIFIER8 id;

	sv3.pD3D->GetAdapterIdentifier(sv3.iAdapter, 0, &id);
	Debug("アダプタ名 = %s\n", id.Description);
}

/*
 *	デバイスパラメータの初期化
 */
BOOL SetPresentParam(){
	//	ディスプレイモードをカウント(リフレッシュレートの違いも考慮)
	UINT num = sv3.pD3D->GetAdapterModeCount(sv3.iAdapter);
	Debug("アダプタのモード数 = %d\n", num);

	//	現在のサーフェイスフォーマットを取得
	D3DDISPLAYMODE mode;
	D3DFORMAT formatAlt = D3DFMT_UNKNOWN;

	sv3.pD3D->GetAdapterDisplayMode(sv3.iAdapter, &mode);
	sv3.format = mode.Format;

	Debug("現在のモード = %d x %d %s\n",
		mode.Width, mode.Height, FormatToString(mode.Format));

	//	ディスプレイモードを列挙
	for(int i = 0; i<num; i++){
		sv3.pD3D->EnumAdapterModes(sv3.iAdapter, i, &mode);

		//	解像度が一致するか？
		if(mode.Width==sv3.width && mode.Height==sv3.height){
			//	フォーマットが一致するか？
			if(mode.Format==sv3.format)
				break;	//	列挙終了
			else
				formatAlt = mode.Format;	//	代替フォーマットを保存
		}
	}
	Debug("ウインドウ化 = ");

	if(mode.Format!=sv3.format){
		//	一致するモードがないので代替フォーマットを使用する
		sv3.format = formatAlt;

		Debug("OFF(フォーマット変更)\n");
		sv3.fWindowed = FALSE;
	}else{
		//	スクリーンモードの決定
		if(!g_FullScreen || g_PluginViewArg || CheckArguments("-win")){
			Debug("ON\n");
			sv3.fWindowed = TRUE;
		}else{
			Debug("OFF\n");
			sv3.fWindowed = FALSE;
		}
	}
	Debug("選択されたモード = %d x %d %s ",
		sv3.width, sv3.height, FormatToString(sv3.format));

	//	パラメータ設定
	ZeroMemory(&sv3.d3dpp, sizeof(sv3.d3dpp));

	sv3.d3dpp.BackBufferCount = 1;
	sv3.d3dpp.BackBufferWidth = sv3.width;
	sv3.d3dpp.BackBufferHeight = sv3.height;
	sv3.d3dpp.BackBufferFormat = sv3.format;

	sv3.d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	sv3.d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	sv3.d3dpp.Windowed = sv3.fWindowed;
	sv3.d3dpp.hDeviceWindow = svw.hWnd;

	sv3.d3dpp.EnableAutoDepthStencil = TRUE;
	sv3.d3dpp.AutoDepthStencilFormat = FindDepthStencilFormat(sv3.format);

	Debug("(%s)\n", FormatToString(sv3.d3dpp.AutoDepthStencilFormat));

	return TRUE;
}

/*
 *	使用可能なデプス／ステンシルバッファのフォーマットを検索
 */
D3DFORMAT FindDepthStencilFormat(D3DFORMAT form){
	HRESULT hr;

#define TEST_DSFORMAT(dev, dsf) \
	hr = sv3.pD3D->CheckDeviceFormat( /* サポートしているか？ */ \
		sv3.iAdapter, dev, \
		form, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, dsf); \
	if(SUCCEEDED(hr)) { \
		hr = sv3.pD3D->CheckDepthStencilMatch( /* 互換性テスト */ \
			sv3.iAdapter, dev, \
			form, form, dsf); \
		if(SUCCEEDED(hr)) return dsf; \
	}

	g_StencilEnabled = true;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D24S8 );
	g_StencilEnabled = true;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D24X4S4 );
	g_StencilEnabled = false;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D32 );
	g_StencilEnabled = false;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D24X8 );
	g_StencilEnabled = false;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D16 );
	g_StencilEnabled = true;
	TEST_DSFORMAT( D3DDEVTYPE_HAL, D3DFMT_D15S1 );
	g_StencilEnabled = false;
	return D3DFMT_UNKNOWN;
}

/*
 *	サーフェイスフォーマットを文字列に変換
 */
const char *FormatToString(D3DFORMAT f){
	switch(f){
	case D3DFMT_R8G8B8:			return "D3DFMT_R8G8B8";
	case D3DFMT_A8R8G8B8:		return "D3DFMT_A8R8G8B8";
	case D3DFMT_X8R8G8B8:		return "D3DFMT_X8R8G8B8";
	case D3DFMT_R5G6B5:			return "D3DFMT_R5G6B5";
	case D3DFMT_X1R5G5B5:		return "D3DFMT_X1R5G5B5";
	case D3DFMT_A1R5G5B5:		return "D3DFMT_A1R5G5B5";
	case D3DFMT_A4R4G4B4:		return "D3DFMT_A4R4G4B4";
	case D3DFMT_R3G3B2:			return "D3DFMT_R3G3B2";
	case D3DFMT_A8:				return "D3DFMT_A8";
	case D3DFMT_A8R3G3B2:		return "D3DFMT_A8R3G3B2";
	case D3DFMT_X4R4G4B4:		return "D3DFMT_X4R4G4B4";
	case D3DFMT_D16_LOCKABLE:	return "D3DFMT_D16_LOCKABLE";
	case D3DFMT_D32:			return "D3DFMT_D32";
	case D3DFMT_D15S1:			return "D3DFMT_D15S1";
	case D3DFMT_D24S8:			return "D3DFMT_D24S8";
	case D3DFMT_D16:			return "D3DFMT_D16";
	case D3DFMT_D24X8:			return "D3DFMT_D24X8";
	case D3DFMT_D24X4S4:		return "D3DFMT_D24X4S4";
	default:					return "D3DFMT_UNKNOWN";
	}
}

/*
 *	バッファサイズ適用
 */
void AffectWindowSize()
{
	if(!sv3.pDev) return;

	//int width = sv3.width, height = sv3.height;
	int width = svw.winW, height = svw.winH;

	D3DXMatrixPerspectiveFovLH(
		&sv3.mtxProj,
		D3DX_PI/4,
		(float)width / height,
		CLIP_PLANE_NEAR,
		CLIP_PLANE_FAR
	);
	sv3.pDev->SetTransform(D3DTS_PROJECTION, &sv3.mtxProj);

	//ビューポート行列の作成(ワールド→スクリーン座標変換用)
	sv3.mtxVPort = MTX4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	sv3.mtxVPort._11 =  width *0.5f;
	sv3.mtxVPort._22 = -height*0.5f;
	sv3.mtxVPort._41 =  width *0.5f;
	sv3.mtxVPort._42 =  height*0.5f;
}

/*
 *	座標系の初期化
 */
void InitMetrics(){
#if 0
	// viewport
	D3DVIEWPORT8 vp;
    vp.X = sv3.width / 2;
    vp.Y = 0;
    vp.Width = sv3.width / 2;
    vp.Height = sv3.height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
	sv3.pDev->SetViewport(&vp);
#endif

	//	各座標変換行列の指定
	sv3.mtxWorld = MTX4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	sv3.pDev->SetTransform(D3DTS_WORLD, &sv3.mtxWorld);

	D3DXMatrixLookAtLH(
		&sv3.mtxView,
		&D3DXVECTOR3(0.0f, 0.0f, -1.0f),
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f),
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));
	sv3.pDev->SetTransform(D3DTS_VIEW, &sv3.mtxView);

	//	クリッピング設定
	D3DCLIPSTATUS8 cs;

	cs.ClipUnion = cs.ClipIntersection = D3DCS_ALL;
	sv3.pDev->SetClipStatus(&cs);

	//	向き行列の作成
	sv3.mtxFront = MTX4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	sv3.mtxRear = MTX4(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);
	sv3.mtxLeft = MTX4(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1);
	sv3.mtxRight = MTX4(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);
	sv3.mtxTop = MTX4(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1);
	sv3.mtxBottom = MTX4(1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 1);

	//	UV座標の初期化
	sv3.u[0] = 0, sv3.u[1] = 1;
	sv3.v[0] = 0, sv3.v[1] = 1;

	AffectWindowSize();
}

/*
 *	レンダリング・ステートの初期化
 */
void InitRenderState(){
	devSetLighting(TRUE);
	devSetAmbient(0xff808080);
	devSetSpecular(TRUE);
	devSetShading(D3DSHADE_GOURAUD);
	devSetCulling(TRUE);
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetFog(FALSE, 0, 0, 0);
	devSetPixelFog(FALSE, 0, 0, 0);
	devSetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	devSetNormalize(TRUE);

	devSetTexColor(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	devSetTexAlpha(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	devSetTexFilter(0, D3DTEXF_POINT);

	g_BufferClearMode = D3DCLEAR_ZBUFFER | (g_StencilEnabled ? D3DCLEAR_STENCIL : 0);
	sv3.pDev->Clear(0, NULL, D3DCLEAR_TARGET|g_BufferClearMode, 0, 1.0f, 0);
}

/*
 *	デバイス能力の取得
 */
void GetDeviceCaps(){
	D3DCAPS8 caps;

	sv3.pDev->GetDeviceCaps(&caps);

	sv3.capsMaxPrim = caps.MaxPrimitiveCount;
	sv3.capsMaxLight = caps.MaxActiveLights;
	sv3.capsFogVertex = (caps.RasterCaps&D3DPRASTERCAPS_FOGVERTEX)!=0;
	sv3.capsFogPixel = (caps.RasterCaps&D3DPRASTERCAPS_FOGTABLE)!=0;
	sv3.capsFogRange = (caps.RasterCaps&D3DPRASTERCAPS_FOGRANGE)!=0;
	sv3.capsTexMem = sv3.pDev->GetAvailableTextureMem()/1024;
	sv3.capsTexWidth = caps.MaxTextureWidth;
	sv3.capsTexHeight = caps.MaxTextureHeight;
	sv3.capsTexStage = caps.MaxSimultaneousTextures;
	sv3.capsTexAlpha = (caps.TextureCaps&D3DPTEXTURECAPS_ALPHA)!=0;
	sv3.capsTexMipMap = (caps.TextureCaps&D3DPTEXTURECAPS_MIPMAP)!=0;
	sv3.capsTexBump = (caps.TextureOpCaps&D3DTEXOPCAPS_BUMPENVMAP)!=0;
	Debug(
		"最大プリミティブ = %d\n" 
		"最大ライト = %d\n"
		"フォグ{\n"
		"\t頂点 = %d\n"
		"\tピクセル = %d\n"
		"\t範囲 = %d\n"
		"}\n"
		"テクスチャー{\n"
		"\t利用可能メモリ = %d KB\n"
		"\t最大サイズ = %d x %d\n"
		"\t最大ステージ = %d\n"
		"\tアルファブレンド = %d\n"
		"\tミップマップ = %d\n"
		"\tバンプマップ = %d\n"
		"}\n",
		sv3.capsMaxPrim,
		sv3.capsMaxLight,
		sv3.capsFogVertex,
		sv3.capsFogPixel,
		sv3.capsFogRange,
		sv3.capsTexMem,
		sv3.capsTexWidth, sv3.capsTexHeight,
		sv3.capsTexStage,
		sv3.capsTexAlpha,
		sv3.capsTexMipMap,
		sv3.capsTexBump);
}

/*
 *	reset
 */
BOOL ResetD3DDevice()
{
	FreeFont();
	HRESULT hr = sv3.pDev->Reset(&sv3.d3dpp);
	if(FAILED(hr))
	{
		Debug("error D3DDevice Reset\n", hr);
		SendWM_CLOSE();
		return FALSE;
	}
	CreateFont();
	InitMetrics();
	InitRenderState();
	return TRUE;
}

/*
 *	シーンの開始
 *
 *	c	: クリアカラー、c = 0でクリアしない。
 */
BOOL BeginScene(D3DCOLOR c){
	//	デバイスのテスト
	HRESULT hr = sv3.pDev->TestCooperativeLevel();

	if(hr==D3DERR_DEVICELOST){
		Debug("3Dデバイスがロストしています.\n");
		return FALSE;
	}else if(hr==D3DERR_DEVICENOTRESET){
		Debug("3Dデバイスのリセットが必要です.\n");
		/*
		 *	本来ならここで sv3.pDev->Reset()を呼び出してデバイスの再設定
		 *	を試みるべきだが、その前にデバイスに関連するオブジェクトを解放
		 *	しておかなくてはならない。
		 *
		 *	そのタイミングをプログラマに通知したり、解放の義務をおしつける
		 *	のは当ライブラリのコンセプトに反する。　
		 *
		 *	従って、ここでプログラム終了させることにする。
		 */
	//	SendWM_CLOSE();
		if(!ResetD3DDevice()) return FALSE;
	}
	else if(sv3.d3dpp.BackBufferWidth!=svw.winW || sv3.d3dpp.BackBufferHeight!=svw.winH)
	{
		Debug("バッファサイズを変更します.\n");
		sv3.d3dpp.BackBufferWidth	= svw.winW;
		sv3.d3dpp.BackBufferHeight	= svw.winH;
		sv3.width  = svw.winW;
		sv3.height = svw.winH;
		AffectWindowSize();
		if(!ResetD3DDevice()) return FALSE;
	}
	//	シーン開始
	if(FAILED(sv3.pDev->BeginScene())) return FALSE;

	//	クリア
	sv3.pDev->Clear(0, NULL, (c!=0 ? D3DCLEAR_TARGET : 0)|g_BufferClearMode, c, 1.0f, 0);
	//	ビュートランスフォーム
	sv3.pDev->SetTransform(D3DTS_VIEW, &sv3.mtxView);

	//	各種変換行列の計算
	float tmp;
	D3DXMatrixInverse(&sv3.mtxViewInv, &tmp, &sv3.mtxView);

	sv3.mtxWtoS = /*sv3.mtxWorld**/ sv3.mtxView*sv3.mtxProj*sv3.mtxVPort;
	D3DXMatrixInverse(&sv3.mtxStoW, &tmp, &sv3.mtxWtoS);

	return TRUE;
}

/*
 *	シーンの終了
 */
void EndScene(){
	sv3.pDev->EndScene();
	sv3.pDev->Present(NULL, NULL, NULL, NULL);
}

/*
 *	カラー値をX8R8G8B8フォーマットに変換する
 *
 *	d		: 任意フォーマットの色
 *	fmt	: フォーマット
 */
D3DCOLOR GetXRGB32(DWORD d, D3DFORMAT fmt){
	D3DCOLOR c;
	DWORD r, g, b;

	switch(fmt){
	case D3DFMT_R8G8B8:
		c = 0xff000000|(d>>8);
		break;

	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		c = 0xff000000|d;
		break;

	case D3DFMT_R5G6B5:
		r = min((DWORD)0xff, ((d&0xf800)>>11)*8);
		g = min((DWORD)0xff, ((d&0x07e0)>> 5)*4);
		b = min((DWORD)0xff, (d&0x001f)*8);
		c = 0xff000000|r|g|b;
		break;

	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
		r = min((DWORD)0xff, ((d&0x7e00)>>11)*8);
		g = min((DWORD)0xff, ((d&0x03e0)>>5)*8);
		b = min((DWORD)0xff, (d&0x001f)*8);
		c = 0xff000000|r|g|b;
		break;

	case D3DFMT_A4R4G4B4:
		r = min((DWORD)0xff, ((d&0x0f00)>>8)*16);
		g = min((DWORD)0xff, ((d&0x00f0)>>4)*16);
		b = min((DWORD)0xff, (d&0x000f)*16);
		c = 0xff000000|r|g|b;
		break;

	default:
		c = 0;
	}
	return c;
}

/*
 *	マテリアルの初期化
 */
void devResetMaterial(){
	MAT8 mat;

	mat.Diffuse = MAKE_CV(0.8f, 0.8f, 0.8f, 1.0f);
	mat.Ambient = MAKE_CV(0.8f, 0.8f, 0.8f, 1.0f);
	mat.Specular = MAKE_CV(0.0f, 0.0f, 0.0f, 0.0f);
	mat.Emissive = MAKE_CV(0.0f, 0.0f, 0.0f, 0.0f);
	mat.Power = 0.0f;

	devSetMaterial(&mat);
}

/*
 *	ワイヤフレーム用マテリアル
 */
void devSetLineMaterial(){
	MAT8 mat;

	mat.Diffuse = MAKE_CV(1.0f, 1.0f, 1.0f, 1.0f);
	mat.Ambient = MAKE_CV(0.4f, 0.4f, 0.4f, 1.0f);
	mat.Specular = MAKE_CV(0.0f, 0.0f, 0.0f, 0.0f);
	mat.Emissive = MAKE_CV(0.0f, 0.0f, 0.0f, 0.0f);
	mat.Power = 0.0f;

	devSetMaterial(&mat);
}
