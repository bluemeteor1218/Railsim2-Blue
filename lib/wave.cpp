//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "sound.h"
#include "wave.h"

/*
 *	コンストラクタ
 *
 *	※プログラム終了までにFree()またはdelete(newで確保した場合)して下さい。
 */
CWave::CWave(){
	m_pSB = NULL;
	m_p3D = NULL;
	m_pFX = NULL;
}

/*
 *	デストラクタ
 */
CWave::~CWave(){
	Free();
}

/*
 *	音声データの読込み
 *
 *	strFile	: ファイル名
 */
BOOL CWave::Load(char *strFile){
	//	既存なら解放
	if(m_pSB) Free();

	Debug("load(%s) ... ", strFile);
	char full[_MAX_PATH];
	_fullpath(full, strFile, _MAX_PATH);
	m_strName = full;

	HMMIO			hMMI;
	MMCKINFO		parent, child;
	WAVEFORMATEX	wfmtx;
	DWORD			len;
	BOOL			ret = FALSE;

	parent.ckid = (FOURCC)0;
	parent.cksize = 0;
	parent.fccType = (FOURCC)0;
	parent.dwDataOffset = 0;
	parent.dwFlags = 0;
	child = parent;

	//	オープン
	hMMI = mmioOpen(strFile, NULL, MMIO_READ|MMIO_ALLOCBUF);

	if(!hMMI){
		Debug("open error.\n");
		return FALSE;
	}
	//	RIFFチャンクの読み込み＆チェック
	parent.fccType = mmioFOURCC('W', 'A', 'V', 'E');

	if(mmioDescend(hMMI, &parent, NULL, MMIO_FINDRIFF)!=0){
		Debug("RIFF is not found.\n"); goto error;
	}
	//	fmtチャンクの読み込み＆チェック
	child.ckid = mmioFOURCC('f', 'm', 't', ' ');

	if(mmioDescend(hMMI, &child, &parent, 0)!=0){
		Debug("fmt is not found.\n"); goto error;
	}
	//	フォーマットの取得
	mmioRead(hMMI, (char *)&wfmtx, sizeof(wfmtx));
	m_BytesPerSec = wfmtx.nAvgBytesPerSec;

	//	PCMフォーマットか？
	if(wfmtx.wFormatTag!=WAVE_FORMAT_PCM){
		Debug("is not PCM format.\n"); goto error;
	}

	if(mmioAscend(hMMI, &child, 0)!=0){
		Debug("ascend error.\n"); goto error;
	}
	//	dataチャンクの読み込み＆チェック
	child.ckid = mmioFOURCC('d', 'a', 't', 'a');

	if(mmioDescend(hMMI, &child, &parent, MMIO_FINDCHUNK)!=0){
		Debug("chunk is not found.\n"); goto error;
	}
	//	dataチャンク長を取得
	len = child.cksize;

	//	バッファを作成、ロード
	if(!CreateBuffer(hMMI, &wfmtx, len)) goto error;

	//	クローズ
	mmioClose(hMMI, 0);
	Debug("ok.\n");
	return TRUE;

error:
	mmioClose(hMMI, 0);
	return FALSE;
}

/*
 *	別のCWaveオブジェクトからデータを複製する
 *
 *	pWav	: コピー元のオブジェクト
 */
BOOL CWave::Duplicate(CWave *pWav){
	if(!svs.pDS || m_pSB) Free();

	int err;
	//	バッファの複製
	if(FAILED(err = svs.pDS->DuplicateSoundBuffer(pWav->m_pSB, &m_pSB))) return FALSE;

	return Query();
}

/*
 *	サウンドバッファの作成
 *
 *	hMMMI	: ファイルハンドラ
 *	pFmt	: ウエーブフォーマット
 *	len	: ウエーブサイズ
 */
BOOL CWave::CreateBuffer(HMMIO hMMI, LPWAVEFORMATEX pFmt, DWORD len){
	if(!svs.pDS) return FALSE;
	//	バッファの作成
	DSBUFFERDESC desc;

	memset(&desc, 0, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = len;
	desc.lpwfxFormat = pFmt;

	if(svs.f3D) desc.dwFlags |= DSBCAPS_CTRL3D;
//	if(svs.fFX) desc.dwFlags |= DSBCAPS_CTRLFX;

	HRESULT hr = svs.pDS->CreateSoundBuffer(&desc, &m_pSB, NULL);

	if(FAILED(hr)){
		if(hr==DSERR_BUFFERTOOSMALL){
			//	データが短くてエフェクトを使用できない
			Debug("data is too small.\n");
			return TRUE;
		}else if(hr = DSERR_OUTOFMEMORY){
			//	メモリ不足（サウンドカード内の？）
			Debug("out of memory.\n");
			return FALSE;
		}else{
			Debug("can't create buffer.\n");
			return FALSE;
		}
	}
	//	バッファのロック
	LPVOID write1, write2;
	DWORD length1, length2;

	if(m_pSB->Lock(0, len, &write1, &length1, &write2, &length2, 0)==DSERR_BUFFERLOST){
		m_pSB->Restore(); return FALSE;
	}
	//	dataチャンクをバッファにロード
	if((DWORD)mmioRead(hMMI, (char *)write1, length1)==length1){
		if(length2!=0) mmioRead(hMMI, (char *)write2, length2);
	}
	//	バッファのアンロック
	if(m_pSB->Unlock(write1, length1, write2, length2)!=DS_OK)
		return FALSE;

	return Query();
}

/*
 *	インターフェイスの取得
 */
BOOL CWave::Query(){
	//	FXバッファの取得
	if(svs.fFX){
		if(FAILED(m_pSB->QueryInterface(IID_IDirectSoundBuffer8, (void **)&m_pFX))){
			Debug("DirectSoundBuffer8\n");
			return FALSE;
		}
	}
	//	3Dバッファの取得
	if(svs.f3D){
		if(FAILED(m_pSB->QueryInterface(IID_IDirectSound3DBuffer, (void **)&m_p3D))){
			Debug("DirectSound3DBuffer\n");
			return FALSE;
		}
	}
	return TRUE;
}

/*
 *	バッファの解放
 */
void CWave::Free(){
	if(m_strName!=""){
		Debug("release(%s)\n", m_strName.c_str());
		m_strName = "";
	}
	Stop();
	RELEASE(m_p3D);
	RELEASE(m_pFX);
	RELEASE(m_pSB);
}

/*
 *	再生
 *
 *	ms	: 再生開始位置 [ms] (<0 でループ)
 */
void CWave::Play(int ms){
	if(!m_pSB) return;

	m_pSB->Stop();
	m_pSB->SetCurrentPosition(ms<0 ? 0 : m_BytesPerSec*ms/1000);
	HRESULT hr = m_pSB->Play(0, 0, ms<0 ? DSBPLAY_LOOPING : 0);
	//	リロード処理
	if(hr==DSERR_BUFFERLOST){
		Debug("DSERR_BUFFERLOST:%s", m_strName.c_str());

		PrimaryBufferVerify();	//	先にプライマリバッファを検証する
		m_pSB->Restore();	//	どの道Free()するが念のため
		Load((char *)m_strName.c_str());
	}
}

/*
 *	再生状態の取得
 */
BOOL CWave::GetStatus(){
	if(!m_pSB) return FALSE;

	DWORD dw;
	m_pSB->GetStatus(&dw);

	if(dw&DSBSTATUS_PLAYING) return TRUE;
	else return FALSE;
}

/*
 *	エフェクトの設定
 *
 *	fx	: エフェクトタイプ（FX_～）
 *
 *	※16bit以外のWAVEファイルはエフェクトが正常にかからない場合がある。
 */
void CWave::SetFX(int fx){
	if(!svs.fFX || !m_pFX) return;

	//	エフェクトのリセット
	if(fx==FX_DRY){
		m_pFX->SetFX(0, NULL, NULL);
		return;
	}

	DSEFFECTDESC desc;
	DWORD rc = 0;

	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = 0;

	switch(fx){
	case FX_REVERB		: desc.guidDSFXClass = GUID_DSFX_WAVES_REVERB;			break;
	case FX_REVERB3D	: desc.guidDSFXClass = GUID_DSFX_STANDARD_I3DL2REVERB;	break;
	case FX_ECHO		: desc.guidDSFXClass = GUID_DSFX_STANDARD_ECHO;			break;
	case FX_CHORUS		: desc.guidDSFXClass = GUID_DSFX_STANDARD_CHORUS;		break;
	case FX_FLANGER		: desc.guidDSFXClass = GUID_DSFX_STANDARD_FLANGER;		break;
	case FX_GARGLE		: desc.guidDSFXClass = GUID_DSFX_STANDARD_GARGLE;		break;
	case FX_COMP		: desc.guidDSFXClass = GUID_DSFX_STANDARD_COMPRESSOR;	break;
	case FX_PARAMEQ		: desc.guidDSFXClass = GUID_DSFX_STANDARD_PARAMEQ;		break;
	case FX_DISTORTION	: desc.guidDSFXClass = GUID_DSFX_STANDARD_DISTORTION;	break;
	default				: SetFX(FX_DRY); return;
	}
	m_pFX->SetFX(1, &desc, &rc);
}
