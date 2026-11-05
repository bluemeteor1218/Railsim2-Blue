//	Copyright (c) 2002 Midikyou

//	※このファイルを書き換えた場合はmain.cppをリビルドして下さい。

const int MAXFPS = 30;	//	FPSの目標値

class CFrame{
	DWORD frame;		//	フレームをカウント
	DWORD frameWait;	//	フレーム毎のウエイト
	DWORD fineWait;		//	MAXFPS/10毎のウエイト
	DWORD cnt;			//	MAXFPS/10までフレームをカウント
	DWORD start;		//	開始時間
	DWORD old;			//	MAXFPS/2フレーム前の時間

public:
	float fps;		//	FPSの実測値
	DWORD framecnt;	//	起動後のフレームカウント

	void Init();
	void Sync();
};

extern CFrame g_frame;

/*
 *	FPS の取得
 */
inline float GetFPS(){ return g_frame.fps; }

/*
 *	フレームカウントの取得
 */
inline DWORD GetFrameCount(){ return g_frame.framecnt; }

/*
 *	フレーム同期
 */
inline void SyncFrame(){ g_frame.Sync(); }

/*
 *	乱数初期化
 */
inline void Randomize(){ srand((unsigned int)time(NULL)); }

/*
 *	0～x-1の乱数
 */
inline int Rand(int x){ return rand()%x; }
inline float FRand(float x){ return rand()*x/RAND_MAX; }

/*
 *	範囲指定の乱数
 */
inline int Rand2(int a, int b){ return Rand(b-a)+a; }
inline float FRand2(float a, float b){ return FRand(b-a)+a; }

inline VEC3 V3Rand(VEC3 v){ return VEC3(FRand(v.x), FRand(v.y), FRand(v.z)); }
inline VEC3 V3Rand2(VEC3 a, VEC3 b){ return V3Rand(b-a)+a; }
inline VEC3 V3RandS(float s){ return V3Rand2(VEC3(-s, -s, -s), VEC3(s, s, s)); }
