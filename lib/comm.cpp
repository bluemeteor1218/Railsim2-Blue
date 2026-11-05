#include "headers.h"
#include "comm.h"

BOOL InitDirectPlay(){
	ZeroMemory(&svc, sizeof(svc));
	return TRUE;
}

void FreeDirectPlay(){
}

void SetReceiveFunc(PFN_RECEIVE pReceiveFunc, LPARAM lParam){
	svc.pReceiveFunc = pReceiveFunc;
	svc.lReceiveParam = lParam;
}

BOOL CreateSession(const GUID*, LPCTSTR, DWORD){ return FALSE; }
BOOL JoinSession(const GUID*, LPCTSTR, DWORD, DWORD){ return FALSE; }
BOOL CloseSession(){ return TRUE; }
BOOL EnumHosts(const GUID*, LPCTSTR){ return FALSE; }
BOOL SendToAll(const PVOID, DWORD){ return FALSE; }
BOOL SendTo(DPNID, const PVOID, DWORD){ return FALSE; }
DPNID GetLocalPlayerID(){ return svc.idLocalPlayer; }
BOOL IsHost(){ return svc.bHost; }
