/**
 * @file    comm.h
 * @brief   UDX extension : Comm - DirectPlay による通信機能を提供する．
 */

/**
 * @mainpage
 *
 * @section s01 はじめに
 * UDX extension : Comm は，
 * UDX Library に DirectPlay による通信機能を提供するための
 * UDX 拡張プログラムで，
 * comm.h と comm.cpp の二つのファイルで構成される．
 * 
 * @section s02 必要環境
 * Comm を使用するために次の環境が必要となる．
 * - UDX Library 1.46 final
 * - DirectX 8.0 SDK or later
 *
 * @section s03 関連ページ
 * - @ref p01
 * - @ref p02
 *
 * @section s04 その他
 * @author  T.Tsuruoka <tsuruoka@macrobilis.com>
 * @since   2004-08-09
 * @version 0.1
 */

/**
 * @page p01 Install
 *
 * @section s11 導入の手順
 * -# UDX Library と同一のディレクトリ内に comm.h と comm.cpp を配置する．
 * -# UDX Library に対して次のようにプログラムを追加する．
 *
 * - udx.h
 * @code
 * #include"movie.h"
 * 
 * #include"comm.h"        // ■ 追加 (UDX extension)
 * 
 * #endif _UDX_H
 * @endcode
 *
 * - sysvalue.h
 * @code
 * SYSVALUE_M svm;     //Music
 * SYSVALUE_V svv;     //Video
 * 
 * SYSVALUE_C svc;     //Comm      ■ 追加 (UDX extension)
 * 
 * CFrame g_frame;
 * @endcode
 *
 * - main.cpp \c CApp::Init 内
 * @code
 * #ifndef NO_MOVIE
 *     if(!InitDirectShow())   return FALSE;
 * #endif
 * 
 *     // ■ 以下 3 行追加 (UDX extension)
 * #ifndef NO_COMM
 *     if(!InitDirectPlay())   return FALSE;
 * #endif
 * 
 *     InitFrame();        //FPS管理機構の初期化
 * @endcode
 *
 * - main.cpp \c CApp::~CApp 内
 * @code
 *     //DirectX関連の解放
 * 
 *     // ■ 以下 3 行追加 (UDX extension)
 * #ifndef NO_COMM
 *     FreeDirectPlay();
 * #endif
 * 
 * #ifndef NO_MOVIE
 *     FreeDirectShow();
 * #endif
 * @endcode
 */

/**
 * @page p02 How to Use
 *
 * @section s20 通信のためのプログラム作成の流れ
 * - @ref s21
 * - @ref s22
 * - @ref s23
 * - @ref s24
 * - @ref s25
 *
 * @section s21 アプリケーションの GUID を設定
 * guidgen.exe などで GUID を生成することができる．
 * @code
 * // アプリケーションを識別するための GUID
 * // {4C7B0386-6137-402e-BB12-84A3FD080481}
 * const GUID g_guidApp = 
 * { 0x4c7b0386, 0x6137, 0x402e, { 0xbb, 0x12, 0x84, 0xa3, 0xfd, 0x8, 0x4, 0x81 } };
 * @endcode
 * 
 * @section s22 セッションを作ってホストになる
 * CreateSession() によりセッションを作る．
 * セッションを作ったプレイヤは自動的にホストとなる．
 * CreateSession() を使ってセッションを作る具体的な例を示す．
 * -# GUID，セッション名，ホストのポートを指定する方法
 * @code
 * CreateSession(&g_guidApp, "sample_session", 2501);
 * @endcode
 * -# GUID，セッション名のみを指定する方法
 * @code
 * CreateSession(&g_guidApp, "sample_session", NULL);
 * @endcode
 * ホストのポート番号をNULLにすると，自動的にポート番号が割り当てられる．
 * 
 * @section s23 セッションに参加する
 * セッションに参加するためにはセッションを管理しているホストに接続しなければならない．
 * そのため，セッションに参加するプレイヤは，
 * ホストの IP アドレスとポート番号を知っている必要がある．
 * JoinSession() を使って，セッションに参加する具体的な例を示す．
 * -# GUID，ホストの IP，ホストのポート番号，自分が使う（ローカルの）ポート番号を指定する方法
 * @code
 * JoinSession(&g_guidApp, "192.168.1.1", 2501, 2502);
 * @endcode
 * ホストのポート番号は CreateSession() で指定したポート番号と同じ番号を指定し，
 * ホストのポート番号と自分が使うポート番号は異なった番号を指定しなければならない．
 * -# GUID，自分が使うポート番号のみを指定する方法
 * @code
 * JoinSession(&g_guidApp, NULL, NULL, 2502);
 * @endcode
 * ホストの情報が不足しているとき，ホストの情報を入力するためのダイアログが表示される．
 * "192.168.1.1:2501" のようにコロンの前にホストの IP アドレス，
 * コロンの後ろにホストのポート番号を指定する．
 * 
 * @section s24 データを送信する
 * データを送信するには， SendToAll() を使う．
 * SendToAll() は自分を含むセッションに参加しているプレイヤ全員にデータを送信する．
 * @code
 * struct COMM_INFO    // 送受信データを格納する構造体
 * {
 *     ....
 * };
 *     ....
 * COMM_INFO commInfo;
 *     ....
 * SendToAll(&commInfo, sizeof(commInfo));
 * @endcode
 *
 * @section s25 受信用の関数を作成して登録する
 * 受信用の関数は #PFN_RECEIVE 型の関数で，自分宛てにデータが届いたときに呼ばれる．
 * ただし関数は SetReceiveFunc() を使って登録しておく必要がある．
 * @code 
 * void Receive(RECEIVE_DATA* pData, DWORD dwSize, LPARAM lParam)
 * {
 *     COMM_INFO* pCommInfo = (COMM_INFO*)pData;
 *     ...
 * }
 *     ...
 * SetReceiveFunc(&Receive, NULL);
 * @endcode
 * SetReceiveFunc() の 2番目の引数は，
 * 受信用の関数の 3番目の引数 \a lParam として渡される．
 * 
 */

/**
 * @example commtest.cpp
 *
 * @section e01 はじめに
 * Comm を使用するプログラムの例を示す．
 * このプログラムはローカル環境（1台のPC上）で，
 * 2つ以上起動させることで，通信の動作を確認することができる．
 * 
 * @section e02 使い方
 * -# プログラムを1つ起動する．
 * -# [F1]キー を押して，セッションを作成する．このプログラムはホストとなる．
 * -# 同じプログラムをもう1つ起動する．
 * -# [F2]キー を押して，セッションに参加する．
 * -# 2つあるプログラムの一方のウィンドウ上で左ドラッグを行なうと，
 *    自分ともう一方のウィンドウ上にデータが送られる．
 */


#pragma once

#include <dplay8.h>
#pragma comment(lib, "dplayx.lib")

#include <dxerr8.h>
#pragma comment(lib, "dxerr8.lib")

#define MAX_SESSIONNAME 512


typedef BYTE RECEIVE_DATA;

/** 
 * @brief 受信処理関数
 * @param pData 受信データへのポインタ
 * @param dwSize 受信データのサイズ（単位：Byte）
 * @param lParam SetReceiveFunc 関数により設定された 32bit値
 */
typedef void (*PFN_RECEIVE)(RECEIVE_DATA* pData, DWORD dwSize, LPARAM lParam);


/**
 * @brief グローバルとして保持される DirectPlay 関連の構造体
 */
struct SYSVALUE_C
{
	IDirectPlay8Peer*       pDP;
	IDirectPlay8Address*    pDeviceAddress;
	IDirectPlay8Address*    pHostAddress;
	DPNID                   idLocalPlayer;      // 自分自身の ID
	BOOL                    bHost;              // ホストかどうか
	PFN_RECEIVE             pReceiveFunc;       // 受信が発生したときに呼ばれる関数へのポインタ
	LPARAM                  lReceiveParam;      // 受信処理関数に渡す32bit値
};

extern SYSVALUE_C svc;  // 実体は sysvalue.h

BOOL InitDirectPlay();
void FreeDirectPlay();

void SetReceiveFunc(PFN_RECEIVE pReceiveFunc, LPARAM lParam);
BOOL CreateSession(const GUID* pguidApp, LPCTSTR lpszSessionName, DWORD dwPort);
BOOL JoinSession(const GUID* pguidApp, LPCTSTR lpszHostIP, DWORD dwHostPort, DWORD dwLocalPort);
BOOL CloseSession();
BOOL EnumHosts(const GUID* pguidApp, LPCTSTR lpszIP);
BOOL SendToAll(const PVOID pData, DWORD dwSize);
BOOL SendTo(DPNID dest, const PVOID pData, DWORD dwSize);
DPNID GetLocalPlayerID();
BOOL IsHost();

