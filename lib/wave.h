//	Copyright (c) 2002 Midikyou

using namespace std;

#define FX_DRY			-1
#define FX_REVERB		0
#define FX_REVERB3D		1	//	不調のため未サポート
#define FX_ECHO			2
#define FX_CHORUS		3
#define FX_FLANGER		4
#define FX_GARGLE		5
#define FX_COMP			6
#define FX_PARAMEQ		7
#define FX_DISTORTION	8

class CWave{
	//	コピーコンストラクタ封印
	CWave& operator = (const CWave&){return *this;}
	BOOL CreateBuffer(HMMIO hMMI, LPWAVEFORMATEX pFmt, DWORD len);
public:
	LPSNDBUF	m_pSB;			//	セカンダリバッファ
	LPSNDBUF8	m_pFX;			//	エフェクト使用のため
	LP3DBUF		m_p3D;			//	3Dバッファ
	string		m_strName;		//	ファイル名 (リロード用)
	DWORD		m_BytesPerSec;	//	秒当たりバイト数

	CWave();
	~CWave();
	BOOL Load(char *strFile);
	BOOL Duplicate(CWave *pWav);
	BOOL Query();
	void Free();
	void Play(int ms = 0);
	BOOL GetStatus();
	void SetFX(int fx = FX_DRY);

/*
 *	停止
 */
	void Stop(){if(m_pSB) m_pSB->Stop();}
/*
 *	音量の設定 
 *
 *	dB	: DSBVOLUME_MIN-DSBVOLUME_MAX(1/100dB単位)
 */
	void SetVolume(LONG dB = DSBVOLUME_MAX){if(m_pSB) m_pSB->SetVolume(dB);}
/*
 *	3D定位の設定
 *
 *	v		: 位置(メートル)
 */
	void SetPos(VEC3 v){if(svs.f3D && m_p3D) m_p3D->SetPosition(v.x, v.y, v.z, DS3D_IMMEDIATE);}
/*
 *	音源の移動速度の設定
 *
 *	v		: 速度(メートル/秒)
 */
	void SetVelocity(VEC3 v){if(svs.f3D && m_p3D) m_p3D->SetVelocity(v.x, v.y, v.z, DS3D_IMMEDIATE);}
};
