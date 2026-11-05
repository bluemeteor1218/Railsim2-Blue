//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "graphic.h"
#include "movie.h"

/*
 *	DirectShowの初期化
 */
BOOL InitDirectShow(){
	DebugHL();
	Debug("InitDirectShow\n");

	//	フィルタグラフ構築用オブジェクト
	FAILED_ASSERT(
		"グラフィックフィルタが初期化できません.",
		CoCreateInstance(
			CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
			IID_IGraphBuilder, (void **)&svv.pGraph
		));
	//	メディア再生コントロール
	FAILED_ASSERT(
		"メディアコントロールが初期化できません.",
		svv.pGraph->QueryInterface(
			IID_IMediaControl, (void **)&svv.pMCtrl
		));
	//	通知イベント
	FAILED_ASSERT(
		"メディアイベントが初期化できません.",
		svv.pGraph->QueryInterface(
			IID_IMediaEventEx, (void **)&svv.pEvent
		));
	return TRUE;
}

/*
 *	DirectShowの解放
 */
void FreeDirectShow(){
	DebugHL();
	Debug("FreeDirectShow\n");

	StopMovie();
	RELEASE(svv.pEvent);
	RELEASE(svv.pMCtrl);
	RELEASE(svv.pGraph);
}

/*
 *	ムービーイベントに対するハンドラ
 */
void OnGrapNotify(){
	if(!svv.pEvent) return;	//	ムービー機能未使用時も呼ばれるためチェック

	long evCode, param1, param2;

	//	全イベントを取り出す
	while(SUCCEEDED(svv.pEvent->GetEvent(&evCode, &param1, &param2, 0))){ 
		//	イベントに関連するリソースを解放する
		svv.pEvent->FreeEventParams(evCode, param1, param2);

		if((EC_COMPLETE==evCode) || (EC_USERABORT==evCode)){
			StopMovie();
			break;
		} 
	} 
}

/*
 *	ムービー再生
 *
 *	strFile	: ファイル名
 *
 *	※再生中はシーンのレンダリングを止めること。
 */
BOOL PlayMovie(char *strFile){
	Debug("play(%s) ... ", strFile);

	WCHAR wstr[MAX_PATH];

	MultiByteToWideChar(CP_ACP, 0, strFile, -1, wstr, MAX_PATH);

	if(FAILED(svv.pGraph->RenderFile(wstr, NULL))){
		Debug("can't render.\n");
		return FALSE;
	}
	if(FAILED(svv.pGraph->QueryInterface(IID_IVideoWindow, (void **)&svv.pVWin))){
		Debug("can't query interface.\n");
		return FALSE;
	}
	if(FAILED(svv.pEvent->SetNotifyWindow((OAHWND)svw.hWnd, WM_GRAPHNOTIFY, 0))){
		Debug("can't set notify window.\n");
		return FALSE;
	}
	AdjustMovieLayer();

	if(FAILED(svv.pMCtrl->Run())){
		Debug("can't run media control.\n");
		return FALSE;
	}
	Debug("ok.\n");
	svv.fPlay = TRUE;

	return TRUE;
}

/*
 *	ムービー停止
 */
void StopMovie(){
	svv.fPlay = FALSE;
	svv.pMCtrl->Stop();
	RELEASE(svv.pVWin);
}

/*
 *	再生状態の取得
 */
BOOL GetMovieState(){
	return svv.fPlay;
}

/*
 *	ムービーレイヤーのサイズをクライアント領域に合わせる
 */
void AdjustMovieLayer(){
	if(!svv.pVWin) return;	//	ムービー機能未使用時も呼ばれるためチェック

	svv.pVWin->put_Owner((OAHWND)svw.hWnd);
	svv.pVWin->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS);
	svv.pVWin->SetWindowPosition(0, 0, svw.winW, svw.winH);
}
