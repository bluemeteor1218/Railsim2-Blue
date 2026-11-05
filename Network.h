#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

class CListView;

enum RSNTransferState{
	RSN_TRANS_NONE = 0,
	RSN_TRANS_LAYOUT = 10,
	RSN_TRANS_FORCE_DWORD = 0x7fffffff,
};

//	ネットワーク制御関数
int RSNCreateSession(int, int, const char *);
int RSNJoinSession(const char *, int, bool, const char *);
void RSNDeleteMember(DPNID id);
bool RSNCloseSession();

void RSNReceiveHost(RECEIVE_DATA *, DWORD, LPARAM);
void RSNReceiveClient(RECEIVE_DATA *, DWORD, LPARAM);

bool RSNBeginTransferData(DPNID, const void *, int, RSNTransferState);
void RSNEndTransferData();
RSNTransferState GetNetworkTransferState();
bool IsNetworkTransferComplete();

bool CheckLayoutDigest(const unsigned char *);
inline bool CheckPortArea(int port){ return 0<=port && port<65536; }

void ExceedNetworkSyncLimit(int);
void ListNetworkMember(CListView *);

//	外部グローバル
extern bool g_NetworkCloseRequest;
extern bool g_NetworkInitialized;
extern int g_NetworkSyncLimitReceived;
extern int g_NetworkSyncLimitSent;
extern int g_NetworkHostPort;
extern int g_NetworkHostIP[4];
extern int g_NetworkLocalPort;
extern int g_NetworkSyncInterval;
extern char *g_NetworkFileCopy;
extern int g_NetworkFileCopySize;

extern int g_NetworkTransferSize;
extern int g_NetworkTransferRestSize;
extern char* g_NetworkTransferData;

#endif
