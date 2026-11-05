//	Copyright (c) 2002 Midikyou

struct SYSVALUE_S{
	LPDIRECTSOUND8	pDS;		//	DirectSoundオブジェクト
	LPSNDBUF		pPB;		//	プライマリバッファ
	LP3DLISTENER	pListener;	//	リスナー
	BOOL f3D;			//	3Dフラグ
	BOOL fFX;			//	FXフラグ
	LONG initVolume;	//	初期ボリューム
};
extern SYSVALUE_S svs;

BOOL InitDirectSound();
void FreeDirectSound();
BOOL CreatePrimaryBuffer();
void PrimaryBufferVerify();

void SetMasterVolume(LONG dB = svs.initVolume);
void SetListenerSens(float f);
void SetListenerPos(VEC3 v);
void SetListenerDir(VEC3 d, VEC3 u);

/*
 *	ウェーブフォーマットを取得
 *
 *	freq	: サンプリング周波数
 *	bit	: 量子化ビット数
 *	ch	: チャンネル数
 */
inline WAVEFORMATEX GetWaveFormat(DWORD freq, WORD bit, WORD ch){
	WAVEFORMATEX wfmt;

	wfmt.wFormatTag = WAVE_FORMAT_PCM;
	wfmt.nChannels = ch;
	wfmt.nSamplesPerSec = freq;
	wfmt.wBitsPerSample = bit;
	wfmt.nBlockAlign = (bit/8)*ch;
	wfmt.nAvgBytesPerSec = freq*wfmt.nBlockAlign;
	wfmt.cbSize = 0;

	return wfmt;
}
