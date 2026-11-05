/**
 * @file comm.cpp
 * @brief UDX extension : Comm の実装
 */


#include "headers.h"
#include "debug.h"
#include "comm.h"

// Comm 内部で使われる関数
static BOOL InitPeer();
static BOOL InitDeviceAddress(DWORD dwPort);
static BOOL InitHostAddress(LPCTSTR lpszIP, DWORD dwPort);
static void ReceiveFunc(RECEIVE_DATA* pData, DWORD dwSize, LPARAM lParam);
static HRESULT CALLBACK DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage);


/**
 * @brief UDX専用 : DirectPlay の初期化
 * @return 成功したとき TRUE，失敗したとき FALSE
 */
BOOL InitDirectPlay()
{
	DebugHL();
	Debug("InitDirectPlay\n");

	// DirectPlay の初期化
	FAILED_ASSERT(
		"DirectPlay が初期化できませんでした.",
		CoCreateInstance(
			CLSID_DirectPlay8Peer,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IDirectPlay8Peer,
			(LPVOID*)&svc.pDP)
	);

	svc.pReceiveFunc = &ReceiveFunc;

	return TRUE;
}

BOOL InitPeer()
{
	// DirectPlay オブジェクトの初期化
	FAILED_ASSERT(
		"DirectPlay オブジェクトの初期化に失敗しました．",
		svc.pDP->Initialize(NULL, &DirectPlayMessageHandler, 0)
	);
	return TRUE;
}

/**
 * @brief UDX専用 : DirectPlay の解放
 */
void FreeDirectPlay()
{
	if(!svc.pDP) return;

	DebugHL();
	Debug("FreeDirectPlay\n");
	
	svc.pDP->Close(0);			  // セッション終了
	RELEASE(svc.pHostAddress);
	RELEASE(svc.pDeviceAddress);
	RELEASE(svc.pDP);
}

/**
 * @brief デバイスアドレスの初期化
 * @param dwPort デバイスのポート
 * @return 成功したとき TRUE，失敗したとき FALSE
 */
static BOOL InitDeviceAddress(DWORD dwPort)
{
	// デバイスアドレスの生成
	FAILED_ASSERT(
		"デバイスアドレスの生成に失敗しました．",
		CoCreateInstance(
			CLSID_DirectPlay8Address,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IDirectPlay8Address,
			(void**)&svc.pDeviceAddress)
	);

	// サービスプロバイダの設定
	svc.pDeviceAddress->SetSP(&CLSID_DP8SP_TCPIP);

	// ポートの設定
	if(dwPort) svc.pDeviceAddress->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(dwPort), DPNA_DATATYPE_DWORD);

	return TRUE;
}

/**
 * @brief ホストアドレスの初期化
 * @param lpszIP ホストの IP
 * @param dwPort ホストの ポート
 * @return 成功したとき TRUE，失敗したとき FALSE
 */
static BOOL InitHostAddress(LPCTSTR lpszIP, DWORD dwPort)
{
	// ホストアドレスの生成
	FAILED_ASSERT(
		"ホストアドレスの生成に失敗しました",
		CoCreateInstance(
			CLSID_DirectPlay8Address,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IDirectPlay8Address,
			(void**)&svc.pHostAddress)
	);

	// サービスプロバイダの設定
	svc.pHostAddress->SetSP(&CLSID_DP8SP_TCPIP);

	// ホストの IP の設定
	if(lstrlen(lpszIP) > 0) svc.pHostAddress->AddComponent(DPNA_KEY_HOSTNAME, lpszIP, lstrlen(lpszIP)+1, DPNA_DATATYPE_STRING_ANSI);

	// ホストの ポート の設定
	if(dwPort) svc.pHostAddress->AddComponent(DPNA_KEY_PORT, &dwPort, sizeof(dwPort), DPNA_DATATYPE_DWORD);

	return TRUE;
}

/**
 * @brief データ受信時に呼ばれる受信処理関数を設定
 *
 * データ受信が発生したときに呼ばれる受信処理関数を設定する．
 * 受信処理関数は #PFN_RECEIVE 型と同じ引数と戻り値の形式を持つ関数である必要がある．
 * @param pReceiveFunc 受信処理関数へのポインタ
 * @param lParam 受信処理関数に渡される32bit値．
 */
void SetReceiveFunc(PFN_RECEIVE pReceiveFunc, LPARAM lParam)
{
	svc.pReceiveFunc = pReceiveFunc;
	svc.lReceiveParam = lParam;
}

/**
 * @brief セッションを作成する
 * @param pguidApp アプリケーションの GUID へのポインタ
 * @param lpszSessionName セッション名
 * @param dwPort 使用するポート番号
 * @return 成功したとき TRUE，失敗したとき FALSE
 */
BOOL CreateSession(const GUID* pguidApp, LPCTSTR lpszSessionName, DWORD dwPort)
{
 	InitPeer();

	svc.bHost = TRUE;

   // デバイスアドレスの初期化
	if(FAILED(InitDeviceAddress(dwPort))) return FALSE;

	// セッション情報の設定
	DPN_APPLICATION_DESC dpnAppDesc;
	ZeroMemory(&dpnAppDesc, sizeof(DPN_APPLICATION_DESC));
	WCHAR wszSessionName[MAX_SESSIONNAME];
	mbstowcs(wszSessionName, lpszSessionName, MAX_SESSIONNAME);
	dpnAppDesc.dwSize		   = sizeof(DPN_APPLICATION_DESC);
	dpnAppDesc.dwFlags		  = 0;//DPNSESSION_MIGRATE_HOST;
	dpnAppDesc.guidApplication  = *pguidApp;
	dpnAppDesc.pwszSessionName  = wszSessionName;

	// セッションの作成
	if(FAILED(svc.pDP->Host(
		&dpnAppDesc,
		&svc.pDeviceAddress,
		1,
		NULL,
		NULL,
		NULL,
		DPNHOST_OKTOQUERYFORADDRESSING))
	) return FALSE;

	return TRUE;
}

/**
 * @brief セッションに参加する
 * @param pguidApp アプリケーションの GUID へのポインタ
 * @param lpszHostIP ホストの IP
 * @param dwHostPort ホストのポート
 * @param dwLocalPort ローカルデバイスのポート
 * @return セッションに参加することができたとき TRUE，できなかったとき FALSE
 */
BOOL JoinSession(const GUID* pguidApp, LPCTSTR lpszHostIP, DWORD dwHostPort, DWORD dwLocalPort)
{
	InitPeer();

	svc.bHost = FALSE;

	// デバイスアドレスの初期化
	if(FAILED(InitDeviceAddress(dwLocalPort))) return FALSE;

	// ホストアドレスの初期化
	if(FAILED(InitHostAddress(lpszHostIP, dwHostPort))) return FALSE;

	// セッション情報
	DPN_APPLICATION_DESC dpnAppDesc;
	ZeroMemory(&dpnAppDesc, sizeof(DPN_APPLICATION_DESC));
	dpnAppDesc.dwSize		   = sizeof(DPN_APPLICATION_DESC);
	dpnAppDesc.guidApplication  = *pguidApp;

	// 汎用パラメータの設定
	DPN_CAPS dpnCaps;
	ZeroMemory(&dpnCaps, sizeof(DPN_CAPS));
	dpnCaps.dwSize				  = sizeof(DPN_CAPS);
	dpnCaps.dwConnectTimeout		= 1000;	 //接続要求を再送信するまでの時間（ms秒）
	dpnCaps.dwConnectRetries		= 1;		//接続要求を送る回数
	dpnCaps.dwTimeoutUntilKeepAlive = 0;
	svc.pDP->SetCaps(&dpnCaps, 0);

	// セッションの参加
	if(FAILED(svc.pDP->Connect(
		&dpnAppDesc,
		svc.pHostAddress,
		svc.pDeviceAddress,
		NULL, NULL, NULL, 0, NULL, NULL, NULL,
		DPNCONNECT_OKTOQUERYFORADDRESSING | DPNCONNECT_SYNC))
	) return FALSE;

	return TRUE;
}

BOOL CloseSession()
{
	if(svc.pDP->Close(0)!=S_OK) return FALSE;
	return TRUE;
}

/**
 * @brief ホストを列挙する（未実装）
 * @param pguidApp アプリケーションの GUID へのポインタ
 * @param lpszHostIP ホストの IP
 * @return 成功したとき TRUE，失敗したとき FALSE
 */
BOOL EnumHosts(const GUID* pguidApp, LPCTSTR lpszIP)
{
	// デバイスアドレスの初期化
	if(FAILED(InitDeviceAddress(NULL))) return FALSE;

	// ホストアドレスの初期化
	if(FAILED(InitHostAddress(lpszIP, NULL))) return FALSE;

	// セッション情報
	DPN_APPLICATION_DESC dpnAppDesc;
	ZeroMemory(&dpnAppDesc, sizeof(DPN_APPLICATION_DESC));
	dpnAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
	dpnAppDesc.guidApplication = *pguidApp;

	// ホストの列挙
	FAILED_ASSERT(
		"ホストの列挙に失敗しました．",
		svc.pDP->EnumHosts(
			&dpnAppDesc,							// application description
			svc.pHostAddress,					   // host address
			svc.pDeviceAddress,					 // device address
			NULL,								   // pointer to user data
			0,									  // user data size
			0,									  // retry count (0=default)
			0,									  // retry interval (0=default)
			0,									  // time out (0=default)
			NULL,								   // user context
			NULL,								   // async handle
			DPNENUMHOSTS_OKTOQUERYFORADDRESSING | DPNENUMHOSTS_SYNC)  // flags
	);

	return TRUE;
}

/**
 * @brief セッションに参加している全員にデータを送信する
 * @param pData 送信するデータへのポインタ
 * @param dwSize 送信するデータのサイズ（単位：Byte）
 * @return 送信に成功したとき TRUE，失敗したとき FALSE を返す
 */
BOOL SendToAll(const PVOID pData, DWORD dwSize)
{
	return SendTo(DPNID_ALL_PLAYERS_GROUP, pData, dwSize);
}

/**
 * @brief セッション内の特定の相手にデータを送信する
 * @param dest 送信先 ID
 * @param pData 送信するデータへのポインタ
 * @param dwSize 送信するデータのサイズ（単位：Byte）
 * @return 送信に成功したとき TRUE，失敗したとき FALSE を返す
 */
BOOL SendTo(DPNID dest, const PVOID pData, DWORD dwSize)
{
	DPN_BUFFER_DESC dpnBuffer;

	dpnBuffer.pBufferData = (BYTE*)pData;
	dpnBuffer.dwBufferSize = dwSize;

	FAILED_ASSERT(
		"データの送信に失敗しました．",
		svc.pDP->SendTo(dest, &dpnBuffer, 1, 0, NULL, NULL, DPNSEND_SYNC|DPNSEND_GUARANTEED)
	);

	return TRUE;
}

/**
 * @brief 自分の ID を取得する
 * @return 自分の ID
 */
DPNID GetLocalPlayerID()
{
	return svc.idLocalPlayer;
}

/**
 * @brief 自分がホストであるかどうかを調べる
 * @return 自分がホストのとき TRUE，ホストではないとき FALSE
 */
BOOL IsHost()
{
	return svc.bHost;
}

/** 
 * @brief デフォルトの受信処理関数
 *
 * この関数はプレースホルダであり，アプリケーションで独自の #PFN_RECEIVE 型の関数を定義し，
 * SetReceiveFunc() により受信処理関数として設定する必要がある．
 * @param pData 受信データへのポインタ
 * @param dwSize 受信データのサイズ（単位：Byte）
 * @param lParam SetReceiveFunc 関数により設定された 32bit値
 */
void ReceiveFunc(RECEIVE_DATA* pData, DWORD dwSize, LPARAM lParam)
{
	// 何もしない
	Debug("ReceiveFunc()\n");
}

/**
 * メッセージ処理
 * @param pvUserContext
 * @param dwMessageType
 * @param pMessage
 * @return 正常にメッセージを処理したとき S_OK を返す
 */
static HRESULT CALLBACK DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{

	switch(dwMessageType){

	case DPN_MSGID_CREATE_PLAYER:
		{
			PDPNMSG_CREATE_PLAYER pCreatePlayerMsg;
			PDPN_PLAYER_INFO pdpnPlayerInfo = NULL;
			DWORD dwSize = 0;

			pCreatePlayerMsg = (PDPNMSG_CREATE_PLAYER)pMessage;

			// DPN_PLAYER_INFO のサイズを取得する
			HRESULT hr;
			hr = svc.pDP->GetPeerInfo(pCreatePlayerMsg->dpnidPlayer, pdpnPlayerInfo, &dwSize, 0);
			if(hr != DPNERR_BUFFERTOOSMALL) return FALSE;
			pdpnPlayerInfo = (DPN_PLAYER_INFO*) new BYTE[dwSize];
			ZeroMemory(pdpnPlayerInfo, dwSize);
			pdpnPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

			// DPN_PLAYER_INFO を取得する
			FAILED_ASSERT(
				"プレイヤ情報の取得に失敗しました．",
				svc.pDP->GetPeerInfo(pCreatePlayerMsg->dpnidPlayer, pdpnPlayerInfo, &dwSize, 0)
			);

			// 作成されたプレイヤ情報をデバッグ出力
			Debug("PlayerInfo: dpnidPlayer = %x, dwPlayerFlags = %d\n",
				pCreatePlayerMsg->dpnidPlayer, pdpnPlayerInfo->dwPlayerFlags);

			// プレイヤーの情報を取得
			if(pdpnPlayerInfo->dwPlayerFlags & DPNPLAYER_LOCAL){
				// 自分の DPNID を取得
				svc.idLocalPlayer = pCreatePlayerMsg->dpnidPlayer;
				if(pdpnPlayerInfo->dwPlayerFlags & DPNPLAYER_HOST){
					// 自分がホストのとき
					svc.bHost = TRUE;
				}
			}
		}

	case DPN_MSGID_DESTROY_PLAYER:
		break;

		/*
	case DPN_MSGID_HOST_MIGRATE:
		{
			PDPNMSG_HOST_MIGRATE pHostMigrateMsg;
			pHostMigrateMsg = (PDPNMSG_HOST_MIGRATE)pMessage;

			if(pHostMigrateMsg->dpnidNewHost == svc.idLocalPlayer){
				// 新しいホストの ID と自分の ID が同じとき
				svc.bHost = TRUE;
			}

			break;
		}
		*/

	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		{
			// 未実装

			//PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg;
			//pEnumHostsResponseMsg = (PDPNMSG_ENUM_HOSTS_RESPONSE)pMessage;
			break;
		}

	case DPN_MSGID_RECEIVE:
		{
			PDPNMSG_RECEIVE pReceiveMsg;
			pReceiveMsg = (PDPNMSG_RECEIVE)pMessage;

			// 受信処理関数に受信データを渡す
			svc.pReceiveFunc(
				pReceiveMsg->pReceiveData,
				pReceiveMsg->dwReceiveDataSize,
				svc.lReceiveParam
			);

		}

	}

	return S_OK;
}
