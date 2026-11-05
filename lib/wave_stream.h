//	Copyright (c) 2002 Midikyou

#define STREAM_TIMEOUT	INFINITE
#define THREAD_TIMEOUT	1000

//	ストリーミング再生を管理
class CWaveStream{
	LPSNDBUF	m_pBuf;		//	ストリームバッファ
	LPSNDNOTIFY	m_pNotify;	//	再生進行度の通知オブジェクト
	HANDLE		m_hEvent;	//	再生進行度の通知イベント
	DWORD		m_size;		//	バッファサイズ
	DWORD		m_wpos;		//	バッファ書込み位置

	//	コピーコンストラクタ封印
	CWaveStream& operator = (const CWaveStream&){return *this;}
public:
	CWaveStream();
	~CWaveStream();

	BOOL Begin(LPWAVEFORMATEX format, DWORD size, int count);
	void End();
	BOOL Enqueue(LPBYTE pData, DWORD size);
};

//	スレッド管理
class CThread{
public:
	DWORD	m_idThread;
	HANDLE	m_hThread;

	/*
	 *	コンストラクタ
	 */
	CThread(){
		m_hThread = NULL;
	}
	/*
	 *	デストラクタ
	 */
	~CThread(){
		End();
	}
	/*
	 *	スレッド作成
	 */
	BOOL Begin(LPTHREAD_START_ROUTINE func, void *pData){
		m_hThread = CreateThread(NULL, 0, func, pData, 0, &m_idThread);

		if(!m_hThread){
			Debug("CreateThread\n");
			return FALSE;
		}
		return TRUE;
	}
	/*
	 *	スレッド解放
	 */
	void End(DWORD timeout = THREAD_TIMEOUT){
		if(m_hThread){
			if(WaitForSingleObject(m_hThread, timeout)==WAIT_TIMEOUT){
				//	タイムアウトしたら強制終了
				Debug("time out CThread::End\n");
				TerminateThread(m_hThread, 0);
			}
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
};

/*
 *	Win32API CreateThread ではなく、
 *	ランタイムライブラリ _beginthreadex を使用するスレッドクラス
 *	プロジェクト設定のコード生成をマルチスレッドライブラリに設定すること
 */
class CCrtThread
{
	unsigned int _thread_id;
	HANDLE _thread_handle;

public:

	CCrtThread( )
		: _thread_id( 0 )
		, _thread_handle( 0 )
	{
	}

	unsigned int getId( ) { return _thread_id; }
	HANDLE getHandle( ) { return _thread_handle; }

	bool begin( unsigned int ( __stdcall *_Proc )( void* ), void* _Param )
	{
		_thread_handle = reinterpret_cast< HANDLE >(
			_beginthreadex( 0, 0, _Proc, _Param, 0, &_thread_id ) );
		return _thread_handle ? TRUE : FALSE;
	}

	void end( unsigned int _Timeout = 1000 )
	{
		if ( _thread_handle )
		{
			if ( WaitForSingleObject( _thread_handle, _Timeout ) == WAIT_TIMEOUT )
			{
				TerminateThread( _thread_handle, 0 );
			}
			CloseHandle( _thread_handle );
			_thread_id = 0;
			_thread_handle = 0;
		}
	}
};
