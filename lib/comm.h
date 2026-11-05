// Network layer placeholder. The original used DirectPlay 8, which no longer ships with the Windows SDK and
// was only used by an optional sample extension; the interface is kept so callers keep compiling.
#pragma once

typedef BYTE RECEIVE_DATA;
typedef DWORD DPNID;
#define DPNID_ALL_PLAYERS_GROUP 0

typedef void (*PFN_RECEIVE)(RECEIVE_DATA* pData, DWORD dwSize, LPARAM lParam);

struct SYSVALUE_C {
	DPNID       idLocalPlayer;
	BOOL        bHost;
	PFN_RECEIVE pReceiveFunc;
	LPARAM      lReceiveParam;
};

extern SYSVALUE_C svc;

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
