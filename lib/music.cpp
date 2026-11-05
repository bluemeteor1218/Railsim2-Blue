//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "sound.h"
#include "music.h"

/*
 *	DirectMusicの初期化
 */
BOOL InitDirectMusic(){
	DebugHL();
	Debug("InitDirectMusic\n");

	//	ローダの作成
	FAILED_ASSERT(
		"DirectMusicローダーが作成できません.",
		CoCreateInstance(
			CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC,
			IID_IDirectMusicLoader8, (void **)&svm.pLoader
		));
	return TRUE;
}

/*
 *	DirectMusicの解放
 */
void FreeDirectMusic(){
	DebugHL();
	Debug("FreeDirectMusic\n");

	FreeMusic();
	RELEASE(svm.pLoader);
}

/*
 *	音楽データのロード
 *
 *	strFile	: ファイル名
 */
BOOL LoadMusic(char *strFile){
	Debug("load(%s) ... ", strFile);

	//	既存なら解放
	if(svm.pSeg) FreeMusic();

	//	カレントパスを取得しUNICODEに変換
	char dir[MAX_PATH];
	WCHAR tmp[MAX_PATH];

	GetCurrentDirectory(MAX_PATH, dir);
	MultiByteToWideChar(CP_ACP, 0, dir, -1, tmp, MAX_PATH);

	//	検索ディレクトリを指定
	svm.pLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, tmp, FALSE);
	MultiByteToWideChar(CP_ACP, 0, strFile, -1, tmp, MAX_PATH);

	HRESULT hr;

	//	ファイルをセグメントにロード
	hr = svm.pLoader->LoadObjectFromFile(
			CLSID_DirectMusicSegment, IID_IDirectMusicSegment8, tmp, (void **)&svm.pSeg);

	if(FAILED(hr)){
		Debug("can't load object.\n");
		return FALSE;
	}
	Debug("ok.\n");

	//	パフォーマンスの作成
	if(!CreatePerformance()) return FALSE;

	//	セグメントをパフォーマンスにロード
	svm.pSeg->Download(svm.pPerf);

	return TRUE;
}

/*
 *	音楽データの解放
 *
 *	n		: チャンネル
 */
void FreeMusic(){
	StopMusic();
	if(svm.pPerf) svm.pPerf->CloseDown();

	RELEASE(svm.pPerf);
	RELEASE(svm.pSeg);
}

/*
 *	パフォーマンスの作成
 */
BOOL CreatePerformance(){
	if(!svs.pDS) return FALSE;
	//	パフォーマンスの作成
	FAILED_ASSERT(
		"パフォーマンスが作成できません.",
		CoCreateInstance(
			CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC,
			IID_IDirectMusicPerformance8, (void **)&svm.pPerf
		));
	//	DirectSoundの旧インターフェイスを取得
	IDirectSound *pDSound;

	svs.pDS->QueryInterface(IID_IDirectSound, (void **)&pDSound);

	//	DirectSoundでパフォーマンスを初期化
	FAILED_ASSERT(
		"パフォーマンスが初期化できません.",
		svm.pPerf->InitAudio(
			NULL, &pDSound, NULL,
			DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL, NULL));
	RELEASE(pDSound);

	return TRUE;
}

/*
 *	音楽の再生
 *
 *	f		: ループフラグ（TRUE＝ループON、FALSE＝OFF）
 */
void PlayMusic(BOOL f){
	if(!svm.pSeg || !svm.pPerf) return;

	svm.pSeg->SetStartPoint(svm.time);
	svm.pSeg->SetRepeats(f ? DMUS_SEG_REPEAT_INFINITE : 0);

	svm.pPerf->PlaySegmentEx(
		svm.pSeg, NULL, NULL, 0, 0,
		&svm.pState, NULL, NULL);
}

/*
 *	音楽の一時停止
 *
 */
void PauseMusic(){
	if(!svm.pSeg || !svm.pPerf) return;

	svm.pPerf->Stop(NULL, NULL, 0, 0);
	svm.pState->GetSeek(&svm.time);	//	停止時間を取得

	RELEASE(svm.pState);
}

/*
 *	音楽の停止
 */
void StopMusic(){
	if(!svm.pSeg || !svm.pPerf) return;

	svm.pPerf->Stop(NULL, NULL, 0, 0);
	svm.time = 0;	//	頭出しを行う

	RELEASE(svm.pState);
}

/*
 *	音楽の再生状態を取得
 */
BOOL GetMusicState(){
	if(svm.pSeg && svm.pPerf->IsPlaying(svm.pSeg, NULL)==S_OK)
		return TRUE;
	else
		return FALSE;
}
