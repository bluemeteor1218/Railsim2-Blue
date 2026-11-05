//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "sound.h"

/*
 *	DirectSoundの初期化
 */
BOOL InitDirectSound(){
	DebugHL();
	Debug("InitDirectSound\n");

//	FAILED_ASSERT(
//		"DirectSoundの初期化に失敗しました.",
//		DirectSoundCreate8(NULL, &svs.pDS, NULL));
	if(FAILED(DirectSoundCreate8(NULL, &svs.pDS, NULL))){
		svs.pDS = NULL;
		return TRUE;
	}

	FAILED_ASSERT(
		"サウンドの協調レベルが設定できません.",
		svs.pDS->SetCooperativeLevel(svw.hWnd, DSSCL_PRIORITY));

	//	プライマリバッファの作成
	return CreatePrimaryBuffer();
}

/*
 *	DirectSoundの解放
 */
void FreeDirectSound(){
	if(!svs.pDS || !svs.pPB) return;
	DebugHL();
	Debug("FreeDirectSound\n");

	SetMasterVolume();	//	初期ボリュームに戻す
	RELEASE(svs.pListener);
	RELEASE(svs.pPB);
	RELEASE(svs.pDS);
}

/*
 *	プライマリバッファの作成
 */
BOOL CreatePrimaryBuffer(){
	//	3Dサウンドを使用するかどうか？
	Debug("3Dサウンド = ");

	if(CheckArguments("/3ds")){
		svs.f3D = FALSE;
		Debug("OFF\n");
	}else{
		svs.f3D = TRUE;
		Debug("ON\n");
	}
	//	FXを使用するかどうか？
	Debug("エフェクト = ");

	if(CheckArguments("/fx")){
		svs.fFX = FALSE;
		Debug("OFF\n");
	}else{
		//svs.fFX = TRUE;
		svs.fFX = FALSE;
		Debug("ON\n");
	}
	DSBUFFERDESC desc;

	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME|DSBCAPS_PRIMARYBUFFER;

	if(svs.f3D) desc.dwFlags |= DSBCAPS_CTRL3D;

	FAILED_ASSERT(//	※プライマリバッファにはIDirectSoundBuffer8を使用できない
		"プライマリ・サウンドバッファが作成できません.",
		svs.pDS->CreateSoundBuffer(&desc, &svs.pPB, NULL));
	//	ウェーブフォーマットの設定
	WAVEFORMATEX wfmt = GetWaveFormat(44100, 16, 2);

	FAILED_ASSERT(
		"サウンドフォーマットが指定できません.",
		svs.pPB->SetFormat(&wfmt));
	//	初期ボリュームの取得
	svs.pPB->GetVolume(&svs.initVolume);

	if(svs.f3D){
		//	リスナーの作成
		FAILED_ASSERT(
			"3Dリスナーが作成できません.",
			svs.pPB->QueryInterface(IID_IDirectSound3DListener, (void **)&svs.pListener));

		SetListenerSens(1.0f);
		SetListenerPos(VEC3(0, 0, 0));
		SetListenerDir(VEC3(0, 0, 1), VEC3(0, 1, 0));
	}
	return TRUE;
}

/*
 *	プライマリバッファの検証
 */
void PrimaryBufferVerify(){
	if(!svs.pPB) return;
	DWORD dw;

	svs.pPB->GetStatus(&dw);

	//	ロスト時は復元
	if(dw&DSBSTATUS_BUFFERLOST){
		Debug("PB:DSBSTATUS_BUFFERLOST");
		svs.pPB->Restore();
	}
}

/*
 *	マスターボリュームの設定
 *
 *	dB	: DSBVOLUME_MIN-DSBVOLUME_MAX(1/100dB単位)
 *
 *	※DirectMusicと連動する。
 */
void SetMasterVolume(LONG dB){
	if(!svs.pPB) return;
	svs.pPB->SetVolume(dB);
}

/*
 *	リスナーの感度を設定
 */
void SetListenerSens(float f){
	if(svs.f3D)
		svs.pListener->SetDistanceFactor(f, DS3D_IMMEDIATE);
}

/*
 *	リスナーの位置を設定
 */
void SetListenerPos(VEC3 v){
	if(svs.f3D)
		svs.pListener->SetPosition(v.x, v.y, v.z, DS3D_IMMEDIATE);
}

/*
 *	リスナーの方向を設定
 *
 *	d		: Z軸
 *	u		: Y軸
 */
void SetListenerDir(VEC3 d, VEC3 u){
	if(svs.f3D)
		svs.pListener->SetOrientation(d.x, d.y, d.z, u.x, u.y, u.z, DS3D_IMMEDIATE);
}
