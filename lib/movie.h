//	Copyright (c) 2002 Midikyou

struct SYSVALUE_V{
	IGraphBuilder*	pGraph;	//	フィルタグラフ・オブジェクト
	IMediaControl*	pMCtrl;	//	メディアコントロール・オブジェクト
	IVideoWindow*	pVWin;	//	ビデオウインドウ・オブジェクト
	IMediaEventEx*	pEvent;	//	イベント・オブジェクト
	BOOL fPlay;				//	再生状態フラグ
};
extern SYSVALUE_V svv;

BOOL InitDirectShow();
void FreeDirectShow();

BOOL PlayMovie(char *strFile);
void StopMovie();
BOOL GetMovieState();
void ShowMovieLayer(BOOL f);
void AdjustMovieLayer();
