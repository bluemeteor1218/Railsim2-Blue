//	Copyright (c) 2002 Midikyou

struct SYSVALUE_M{
	IDirectMusicLoader8			*pLoader;	//	ローダー
	IDirectMusicPerformance8	*pPerf;		//	パフォーマンス
	IDirectMusicSegment8		*pSeg;		//	セグメント
	IDirectMusicSegmentState	*pState;	//	セグメント状態
	MUSIC_TIME					time;		//	停止時間
};
extern SYSVALUE_M svm;

BOOL InitDirectMusic();
void FreeDirectMusic();
BOOL CreatePerformance();

BOOL LoadMusic(char *strFile);
void FreeMusic();
void PlayMusic(BOOL f = TRUE);
void PauseMusic();
void StopMusic();
BOOL GetMusicState();
