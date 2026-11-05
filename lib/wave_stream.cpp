//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "window.h"
#include "sound.h"
#include "wave_stream.h"

/*
 *	コンストラクタ
 */
CWaveStream::CWaveStream(){
	m_hEvent = NULL;
	m_pBuf = NULL;
	m_pNotify = NULL;
}

/*
 *	デストラクタ
 */
CWaveStream::~CWaveStream(){
	End();
}

/*
 *	ストリーミング開始
 *
 *	format	: ウェーブフォーマット
 *	size		: ストリームバッファの１ブロックのサイズ
 *	count		: ブロック数
 *
 *	※DirectMusicと併用する場合は、FreeMusic()でパフォーマンスを解放すること。
 */
BOOL CWaveStream::Begin(LPWAVEFORMATEX format, DWORD size, int count){
	if(!svs.pDS) return FALSE;
	End();	//	実行中なら終了

	//	サウンドバッファの作成
	DSBUFFERDESC dsbdesc;

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	m_size = size*count;
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwBufferBytes = m_size;
	dsbdesc.lpwfxFormat = format;
	dsbdesc.dwFlags = DSBCAPS_LOCDEFER
							| DSBCAPS_CTRLPOSITIONNOTIFY
							| DSBCAPS_GETCURRENTPOSITION2;

	HRESULT hr;
	hr = svs.pDS->CreateSoundBuffer(&dsbdesc, &m_pBuf, NULL);

	if(FAILED(hr)){
		Debug("CWaveStream::CreateSoundBuffer\n");
		return FALSE;
	}
	//	サウンドバッファの初期化
	LPBYTE	pBuf1, pBuf2;
	DWORD	size1, size2;

	hr = m_pBuf->Lock(
		0, m_size,
		(void **)&pBuf1, &size1,
		(void **)&pBuf2, &size2, 0);
	if(FAILED(hr)) return FALSE;

	ZeroMemory(pBuf1, size1);

	if(pBuf2!=NULL)
		ZeroMemory(pBuf2, size2);

	m_pBuf->Unlock(pBuf1, size1, pBuf2, size2);

	//	通知オブジェクトの作成
	m_pBuf->QueryInterface(IID_IDirectSoundNotify, (LPVOID *)&m_pNotify);

	if(FAILED(hr)){
		Debug("IDirectSoundNotify\n");
		return FALSE;
	}
	//	通知イベントの作成
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	DSBPOSITIONNOTIFY *pn = new DSBPOSITIONNOTIFY[count];

	for(int i = 0; i<count; i++){
		pn[i].dwOffset = (size*i)+size-1;	//	size*iじゃダメ？
		pn[i].hEventNotify = m_hEvent;
	}
	hr = m_pNotify->SetNotificationPositions(count, pn);
	delete [] pn;

	if(FAILED(hr)){
		Debug("SetNotificationPositions\n");
		return FALSE;
	}
	//	バッファをリングバッファとみなしてループ再生
	m_wpos = 0;
	m_pBuf->Play(0, 0, DSBPLAY_LOOPING);

	return TRUE;
}

/*
 *	ストリーミング終了
 */
void CWaveStream::End(){
	if(m_pBuf) m_pBuf->Stop();

	RELEASE(m_pBuf);
	RELEASE(m_pNotify);

	if(m_hEvent){
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
}

/*
 *	ストリームバッファにデータを補充する
 *
 *	pData	: データ
 *	size	: データサイズ（Begin()で指定した値と同じ）
 *
 *	※ストリームバッファの再生に同期します。
 */
BOOL CWaveStream::Enqueue(LPBYTE pData, DWORD size){
	//	通知イベントを待つ
	DWORD ret = WaitForSingleObject(m_hEvent, STREAM_TIMEOUT);

	if(ret==WAIT_TIMEOUT){
		Debug("time out CWaveStream::Enqueue\n");
		return FALSE;
	}
	//	書込みカーソルの安全な位置に修正
	DWORD ppos, wpos;

	m_pBuf->GetCurrentPosition(&ppos, &wpos);

	if((ppos<wpos && (m_wpos>ppos && m_wpos<wpos))	//	 |ooPxxxWoo|
	|| (ppos>wpos && (m_wpos>ppos || m_wpos<wpos)))	//	 |xxWoooPxx|
		m_wpos = wpos;

	//	バッファをロック
	LPBYTE	pBuf1, pBuf2;
	DWORD	size1, size2;
	HRESULT hr;

	hr = m_pBuf->Lock(
		m_wpos, size,
		(void **)&pBuf1, &size1,
		(void **)&pBuf2, &size2, 0);
	if(hr==DSERR_BUFFERLOST)	m_pBuf->Restore();
	else if(FAILED(hr))		return FALSE;

	//	データをコピー
	CopyMemory(pBuf1, pData, size1);

	if(pBuf2)
		CopyMemory(pBuf2, pData+size1, size2);

	//	バッファをアンロック
	m_pBuf->Unlock(pBuf1, size1, pBuf2, size2);

	//	書込み位置を更新
	m_wpos += size1;
	m_wpos %= m_size;	//	バッファサイズを超えたら一周

	return TRUE;
}
