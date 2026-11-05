#include "stdafx.h"
#include "md5.h"
#include "Network.h"
#include "CListView.h"
#include "CSimpleDialog.h"
#include "CCheckBox.h"
#include "CEditCtrl.h"
#include "CPushButton.h"
#include "CSimulationMode.h"
#include "CTrainGroup.h"
#include "CSaveFile.h"

//	内部定数
extern const int RSN_SCRNAME_MAX = 32;
extern const int RSN_SYNC_INTERVAL_MIN = MAXFPS/10;
extern const int RSN_SYNC_INTERVAL_MAX = MAXFPS*5;
const int RSN_MEMBER_MAX = 12;
const int RSN_TRANSFER_STRIDE = 16000;

//	内部グローバル
CRITICAL_SECTION g_NetworkCS;

bool g_NetworkCloseRequest = false;
bool g_NetworkClosedByHost = false;
bool g_NetworkInitialized = false;
bool g_NetworkStarted = false;
int g_NetworkLastSimSpeed = 0;
int g_NetworkSyncLimitReceived = 0;
int g_NetworkSyncLimitSent = 0;
string g_NetworkScreenName;

DPNID g_NetworkHostID;
int g_NetworkHostPort;
int g_NetworkHostIP[4];
int g_NetworkLocalPort;
int g_NetworkSyncInterval;
char *g_NetworkFileCopy;
int g_NetworkFileCopySize;
bool g_NetworkJoinWithFile = false;
unsigned char g_NetworkFileDigest[16];

DPNID g_NetworkSendTo = NULL;
char *g_NetworkSendData = 0;
int g_NetworkSendSize = 0;
int g_NetworkSendRestSize = 0;
RSNTransferState g_NetworkSendState = RSN_TRANS_NONE;
bool g_NetworkSending = false;
bool g_NetworkSendAbort = false;

DPNID g_NetworkTransferFrom = NULL;
RSNTransferState g_NetworkTransferState = RSN_TRANS_NONE;
int g_NetworkTransferSize = 0;
int g_NetworkTransferRestSize = 0;
int g_NetworkTransferParts = 0;
char *g_NetworkTransferData = NULL;
char *g_NetworkTransferFlag = NULL;

int g_LastJoinedHostIP[4] = {0, 0, 0, 0};
int g_LastJoinedHostPort = 51111;
int g_LastJoinedLocalPort = 51112;
int g_LastCreatedHostPort = 51111;
string g_LastScreenName;

int g_NetworkDummyMapAddress = 1;

const GUID RSN_GUID_APP =
{0x937ce2dd, 0x5120, 0x40da, {0x8d, 0xe6, 0xa, 0x29, 0x86, 0x47, 0x4f, 0xad}};

const D3DCOLOR RSN_MEMBER_COLORS[RSN_MEMBER_MAX] =
{
	0xffff8080, 0xff80ff80, 0xff8080ff, 0xff80ffff, 0xffff80ff, 0xffffff80,
	0xffffc040, 0xff40ffc0, 0xffc040ff, 0xffff40c0, 0xffc0ff40, 0xff40c0ff,
};
bool g_RSNMemberColorUsed[RSN_MEMBER_MAX];

int RSNAllocateColor(){
	int i;
	for(i = 0; i<RSN_MEMBER_MAX; i++){
		if(!g_RSNMemberColorUsed[i]){
			g_RSNMemberColorUsed[i] = true;
			return i;
		}
	}
	return 0;
}

void RSNReleaseColor(int color_id){
	g_RSNMemberColorUsed[color_id] = false;
}

//	train control sync
struct RSNTrainControlInfo{
	int sync_limit;
	void *train_group;
	float target_speed;
};

//	switch control sync
struct RSNSwitchControlInfo{
	int sync_limit;
	int switch_id;
	int switch_opt;
};
list<RSNSwitchControlInfo> g_SwitchControlQueue;

void EnqueueSwitchControl(int switch_id, int switch_opt){
	RSNSwitchControlInfo switch_ctrl;
	switch_ctrl.sync_limit = -1;
	switch_ctrl.switch_id = switch_id;
	switch_ctrl.switch_opt = switch_opt;
	list<RSNSwitchControlInfo>::iterator itr = g_SwitchControlQueue.begin();
	for(; itr!=g_SwitchControlQueue.end(); itr++){
		if(itr->switch_id==switch_id){
			*itr = switch_ctrl;
			return;
		}
	}
	g_SwitchControlQueue.push_back(switch_ctrl);
}

//	point control sync
struct RSNPointControlInfo{
	int sync_limit;
	void *point_id;
	int point_opt;
};
list<RSNPointControlInfo> g_PointControlQueue;

void EnqueuePointControl(void *point_id, int point_opt){
	RSNPointControlInfo point_ctrl;
	point_ctrl.sync_limit = -1;
	point_ctrl.point_id = point_id;
	point_ctrl.point_opt = point_opt;
	list<RSNPointControlInfo>::iterator itr = g_PointControlQueue.begin();
	for(; itr!=g_PointControlQueue.end(); itr++){
		if(itr->point_id==point_id){
			*itr = point_ctrl;
			return;
		}
	}
	g_PointControlQueue.push_back(point_ctrl);
}

//	set train control sync
struct RSNSetTrainControlInfo{
	int sync_limit;
	void *set_group;
	void *set_rail;
	int set_side;
	float set_sumlen;
	int set_flag;
};
list<RSNSetTrainControlInfo> g_SetTrainControlQueue;

void EnqueueSetTrainControl(
	void *set_group, void *set_rail, int set_side, float set_sumlen, int set_flag
){
	RSNSetTrainControlInfo set_train_ctrl;
	set_train_ctrl.sync_limit = -1;
	set_train_ctrl.set_group = set_group;
	set_train_ctrl.set_rail = set_rail;
	set_train_ctrl.set_side = set_side;
	set_train_ctrl.set_sumlen = set_sumlen;
	set_train_ctrl.set_flag = set_flag;
	list<RSNSetTrainControlInfo>::iterator itr = g_SetTrainControlQueue.begin();
	for(; itr!=g_SetTrainControlQueue.end(); itr++){
		if(itr->set_group==set_group){
			*itr = set_train_ctrl;
			return;
		}
	}
	g_SetTrainControlQueue.push_back(set_train_ctrl);
}

//	merge train control sync
struct RSNMergeTrainControlInfo{
	int sync_limit;
	void *merge_group1;
	void *merge_group2;
	int merge_side1;
	int merge_side2;
};
list<RSNMergeTrainControlInfo> g_MergeTrainControlQueue;

void EnqueueMergeTrainControl(
	void *merge_group1, void *merge_group2, int merge_side1, int merge_side2
){
	RSNMergeTrainControlInfo merge_train_ctrl;
	merge_train_ctrl.sync_limit = -1;
	merge_train_ctrl.merge_group1 = merge_group1;
	merge_train_ctrl.merge_group2 = merge_group2;
	merge_train_ctrl.merge_side1 = merge_side1;
	merge_train_ctrl.merge_side2 = merge_side2;
	list<RSNMergeTrainControlInfo>::iterator itr = g_MergeTrainControlQueue.begin();
	for(; itr!=g_MergeTrainControlQueue.end(); itr++){
		if(itr->merge_group1==merge_group1 || itr->merge_group1==merge_group2
			|| itr->merge_group2==merge_group1 || itr->merge_group2==merge_group2){
			*itr = merge_train_ctrl;
			return;
		}
	}
	g_MergeTrainControlQueue.push_back(merge_train_ctrl);
}

struct RSNMemberInfo{
	DPNID id;
	string name;
	int color_id;
	D3DCOLOR color;
	int sim_speed;
	int sync_limit;
	vector<RSNTrainControlInfo> train_ctrl;
	vector<RSNSwitchControlInfo> switch_ctrl;
	vector<RSNPointControlInfo> point_ctrl;
	vector<RSNSetTrainControlInfo> set_train_ctrl;
	vector<RSNMergeTrainControlInfo> merge_train_ctrl;
	RSNMemberInfo(): sync_limit(0){}
};
vector<RSNMemberInfo> g_NetworkMembers; // 使用時はクリティカルセクションに入れること
bool g_NetworkMemberUpdated = false;

struct RSNChatLog{
	string chat;
	D3DCOLOR color;
};
static list<RSNChatLog> g_ChatLog;

void PushChatLog(char *, D3DCOLOR color = 0xffffffff);

vector<DPNID> g_LayoutTransferQueue;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

enum RSNMessageType{
	RSN_MSG_UNKNOWN,
	RSN_MSG_NOTIFY_JOIN,
	RSN_MSG_ACCEPT_CLIENT,
	RSN_MSG_GENERAL_INFO,
	RSN_MSG_REQUEST_LAYOUT,
	RSN_MSG_TRANSFER_REQUEST,
	RSN_MSG_TRANSFER_RESPONSE,
	RSN_MSG_TRANSFER_DATA,
	RSN_MSG_TRANSFER_COMPLETE,
	RSN_MSG_EXCEED_SYNC_LIMIT,
	RSN_MSG_INVALID_VERSION,
	RSN_MSG_MAXIMUM_MEMBERS,
	RSN_MSG_MEMBER_LIST,
	RSN_MSG_CHAT,
	RSN_MSG_LEAVE_SESSION,
	RSN_MSG_CLOSE_SESSION,
	RSN_MSG_START_SIMULATION,
	RSN_MSG_ENTRY_CLOSED,
	RSN_MSG_SIMULATION_PAUSED,
	RSN_MSG_FORCE_DWORD = 0x7fffffff,
};

struct RSNNotifyID{
	RSNMessageType type;
	DPNID id;
};

struct RSNNotifyDigest{
	RSNMessageType type;
	DPNID id;
	unsigned char digest[16];
};

struct RSNNotifyString{
	RSNMessageType type;
	DPNID id;
	int len;
	char str[16];
};

struct RSNNotifyJoin{
	RSNMessageType type;
	DPNID id;
	float version;
	char name[RSN_SCRNAME_MAX+1];
};

struct RSNNotifyGeneralInfo{
	RSNMessageType type;
	DPNID id;
	int sync_interval;
};

struct RSNNotifyMemberList{
	RSNMessageType type;
	DPNID id;
	int num;
	struct{
		DPNID id;
		char name[RSN_SCRNAME_MAX+1];
		D3DCOLOR color;
		int sync_limit;
	} data[RSN_MEMBER_MAX];
};

struct RSNTransferRequest{
	RSNMessageType type;
	DPNID id;
	RSNTransferState state;
	int size;
};

struct RSNTransferResponse{
	RSNMessageType type;
	DPNID id;
	RSNTransferState state;
	int accept;
};

struct RSNTransferData{
	RSNMessageType type;
	DPNID id;
	RSNTransferState state;
	int partid;
	int partsize;
	char data[RSN_TRANSFER_STRIDE];
};

struct RSNTransferComplete{
	RSNMessageType type;
	DPNID id;
	RSNTransferState state;
};

struct RSNExceedSyncLimit{
	RSNMessageType type;
	DPNID id;
	int sim_speed;
	int limit;
	void *train_group;
	float target_speed;
	int switch_id;
	int switch_opt;
	void *point_id;
	int point_opt;
	void *set_group;
	void *set_rail;
	int set_side;
	float set_sumlen;
	int set_flag;
	void *merge_group1;
	void *merge_group2;
	int merge_side1;
	int merge_side2;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void RSNSendNotifyID(RSNMessageType type, DPNID dest){
	RSNNotifyID send;
	send.type = type;
	send.id = GetLocalPlayerID();
	SendTo(dest, &send, sizeof(send));
}

void RSNSendNotifyDigest(RSNMessageType type, DPNID dest, const unsigned char *digest){
	RSNNotifyDigest send;
	send.type = type;
	send.id = GetLocalPlayerID();
	memcpy(send.digest, digest, 16);
	SendTo(dest, &send, sizeof(send));
}

void RSNSendNotifyString(RSNMessageType type, DPNID dest, const char *str){
	int len = strlen(str), size = len+13;
	char *data = new char[size];
	RSNNotifyString *send = (RSNNotifyString *)data;
	send->type = type;
	send->id = GetLocalPlayerID();
	send->len = len;
	strcpy(send->str, str);
	SendTo(dest, send, size);
	delete [] data;
}

void RSNSendNotifyMemberList(RSNMessageType type, DPNID dest){
	EnterCriticalSection(&g_NetworkCS);
	RSNNotifyMemberList send;
	send.type = type;
	send.id = GetLocalPlayerID();
	send.num = g_NetworkMembers.size();
	int i;
	for(i = 0; i<g_NetworkMembers.size(); i++){
		RSNMemberInfo &info = g_NetworkMembers[i];
		send.data[i].id = info.id;
		strcpy(send.data[i].name, info.name.c_str());
		send.data[i].color = RSN_MEMBER_COLORS[info.color_id];
		send.data[i].sync_limit = info.sync_limit;
	}
	LeaveCriticalSection(&g_NetworkCS);
	SendTo(dest, &send, sizeof(send));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool RSNBeginTransferData(DPNID dest, const void *data, int size, RSNTransferState state){
	bool ret = false;
	EnterCriticalSection(&g_NetworkCS);
	if(!g_NetworkSendState && size>0){
		g_NetworkSendTo = dest;
		g_NetworkSendData = (char *)data;
		g_NetworkSendSize = size;
		g_NetworkSendRestSize = g_NetworkSendSize;
		g_NetworkSendState = state;
		g_NetworkSending = false;
		g_NetworkSendAbort = false;
		RSNTransferRequest send;
		send.type = RSN_MSG_TRANSFER_REQUEST;
		send.id = GetLocalPlayerID();
		send.state = state;
		send.size = size;
		SendTo(dest, &send, sizeof(send));
		ret = true;
	}
	LeaveCriticalSection(&g_NetworkCS);
	return ret;
}

void RSNEndTransferData(){
	EnterCriticalSection(&g_NetworkCS);
	g_NetworkTransferFrom = NULL;
	g_NetworkTransferState = RSN_TRANS_NONE;
	g_NetworkTransferSize = 0;
	g_NetworkTransferRestSize = 0;
	g_NetworkTransferParts = 0;
	DELETE_A(g_NetworkTransferData);
	DELETE_A(g_NetworkTransferFlag);
	LeaveCriticalSection(&g_NetworkCS);
}

RSNTransferState GetNetworkTransferState(){
	EnterCriticalSection(&g_NetworkCS);
	RSNTransferState ret = g_NetworkTransferState;
	LeaveCriticalSection(&g_NetworkCS);
	return ret;
}

bool IsNetworkTransferComplete(){
	EnterCriticalSection(&g_NetworkCS);
	bool complete = g_NetworkTransferRestSize == 0;
	LeaveCriticalSection(&g_NetworkCS);
	return complete;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool CheckLayoutDigest(const unsigned char *digest){
	int i;
	for(i = 0; i<16; i++) if(g_NetworkFileDigest[i]!=digest[i]) break;
	if(i<16){
		ErrorDialog(lang(LayoutDataHashMismatch));
		return false;
	}
	return true;
}

void ExceedNetworkSyncLimit(int count){
	if(!g_NetworkInitialized || !g_NetworkStarted) return;
	if(g_NetworkSyncLimitSent<=count+g_NetworkSyncInterval){
		g_NetworkSyncLimitSent += g_NetworkSyncInterval;
		RSNExceedSyncLimit send;
		send.type = RSN_MSG_EXCEED_SYNC_LIMIT;
		send.id = GetLocalPlayerID();
		send.sim_speed = g_SimulationMode->GetSimSpeed();
		send.limit = g_NetworkSyncLimitSent;
		send.train_group = NULL;
		send.target_speed = 0.0f;
		int gid = 0;
		CTrainGroup *group = g_SaveFile->GetTrainGroup();
		while(group){
			if(group->GetTargetSpeed()!=group->GetEffectTargetSpeed()){
				send.train_group = group->OldAdr();
				send.target_speed = group->GetTargetSpeed();
			}
			gid++;
			group = group->Next();
		}
		send.switch_id = -1;
		if(g_SwitchControlQueue.size()){
			RSNSwitchControlInfo switch_ctrl = g_SwitchControlQueue.front();
			g_SwitchControlQueue.pop_front();
			send.switch_id = switch_ctrl.switch_id;
			send.switch_opt = switch_ctrl.switch_opt;
		}
		send.point_id = NULL;
		if(g_PointControlQueue.size()){
			RSNPointControlInfo point_ctrl = g_PointControlQueue.front();
			g_PointControlQueue.pop_front();
			send.point_id = point_ctrl.point_id;
			send.point_opt = point_ctrl.point_opt;
		}
		send.set_group = NULL;
		if(g_SetTrainControlQueue.size()){
			RSNSetTrainControlInfo set_train_ctrl = g_SetTrainControlQueue.front();
			g_SetTrainControlQueue.pop_front();
			send.set_group = set_train_ctrl.set_group;
			send.set_rail = set_train_ctrl.set_rail;
			send.set_side = set_train_ctrl.set_side;
			send.set_sumlen = set_train_ctrl.set_sumlen;
			send.set_flag = set_train_ctrl.set_flag;
		}
		send.merge_group1 = send.merge_group2 = NULL;
		if(g_MergeTrainControlQueue.size()){
			RSNMergeTrainControlInfo merge_train_ctrl = g_MergeTrainControlQueue.front();
			g_MergeTrainControlQueue.pop_front();
			send.merge_group1 = merge_train_ctrl.merge_group1;
			send.merge_group2 = merge_train_ctrl.merge_group2;
			send.merge_side1 = merge_train_ctrl.merge_side1;
			send.merge_side2 = merge_train_ctrl.merge_side2;
		}
		SendToAll(&send, sizeof(send));
	}
}

void UpdateSyncLimit(){
	if(!g_NetworkInitialized || !g_NetworkStarted) return;
	EnterCriticalSection(&g_NetworkCS);
	int i, min_limit = -1;
	for(i = 0; i<g_NetworkMembers.size(); i++){
		RSNMemberInfo &info = g_NetworkMembers[i];
		if(min_limit<0 || info.sync_limit<min_limit) min_limit = info.sync_limit;
	}
	if(g_NetworkSyncLimitReceived<min_limit) g_NetworkSyncLimitReceived = min_limit;
	LeaveCriticalSection(&g_NetworkCS);
	if(g_NetworkLastSimSpeed && g_SimulationMode->GetSimSpeed()==0)
		RSNSendNotifyID(RSN_MSG_SIMULATION_PAUSED, DPNID_ALL_PLAYERS_GROUP);
	g_NetworkLastSimSpeed = g_SimulationMode->GetSimSpeed();
}

void SyncTrainControls(int count){
	EnterCriticalSection(&g_NetworkCS);
	int i;
	for(i = 0; i<g_NetworkMembers.size(); i++){
		RSNMemberInfo &info = g_NetworkMembers[i];
		while(info.train_ctrl.size() && info.train_ctrl[0].sync_limit<count)
			info.train_ctrl.erase(info.train_ctrl.begin());
		while(info.switch_ctrl.size() && info.switch_ctrl[0].sync_limit<count)
			info.switch_ctrl.erase(info.switch_ctrl.begin());
		while(info.point_ctrl.size() && info.point_ctrl[0].sync_limit<count)
			info.point_ctrl.erase(info.point_ctrl.begin());
		while(info.set_train_ctrl.size() && info.set_train_ctrl[0].sync_limit<count)
			info.set_train_ctrl.erase(info.set_train_ctrl.begin());
		while(info.merge_train_ctrl.size() && info.merge_train_ctrl[0].sync_limit<count)
			info.merge_train_ctrl.erase(info.merge_train_ctrl.begin());
	}
	for(i = g_NetworkMembers.size()-1; i>=0; i--){
		RSNMemberInfo &info = g_NetworkMembers[i];
		if(info.train_ctrl.size()){
			RSNTrainControlInfo *train_ctrl = &info.train_ctrl[0];
			if(train_ctrl->sync_limit==count){
				CTrainGroup *group = (CTrainGroup *)g_AddressMap[train_ctrl->train_group];
				if(info.id!=GetLocalPlayerID()) group->SetTargetSpeed(train_ctrl->target_speed);
				group->SetEffectTargetSpeed(train_ctrl->target_speed);
			}
		}
		if(info.switch_ctrl.size()){
			RSNSwitchControlInfo *switch_ctrl = &info.switch_ctrl[0];
			if(switch_ctrl->sync_limit==count){
				void SetStaticSwitchOption(int, int);
				SetStaticSwitchOption(switch_ctrl->switch_id, switch_ctrl->switch_opt);
			}
		}
		if(info.point_ctrl.size()){
			RSNPointControlInfo *point_ctrl = &info.point_ctrl[0];
			if(point_ctrl->sync_limit==count){
				void SetStaticPointOption(void *, int);
				SetStaticPointOption(point_ctrl->point_id, point_ctrl->point_opt);
			}
		}
		if(info.set_train_ctrl.size()){
			RSNSetTrainControlInfo *set_train_ctrl = &info.set_train_ctrl[0];
			if(set_train_ctrl->sync_limit==count){
				void NetSyncSetTrain(void *, void *, int, float, int);
				NetSyncSetTrain(set_train_ctrl->set_group, set_train_ctrl->set_rail,
					set_train_ctrl->set_side, set_train_ctrl->set_sumlen, set_train_ctrl->set_flag);
			}
		}
		if(info.merge_train_ctrl.size()){
			RSNMergeTrainControlInfo *merge_train_ctrl = &info.merge_train_ctrl[0];
			if(merge_train_ctrl->sync_limit==count){
				void NetSyncMergeTrain(void *, void *, int, int);
				NetSyncMergeTrain(
					merge_train_ctrl->merge_group1, merge_train_ctrl->merge_group2,
					merge_train_ctrl->merge_side1, merge_train_ctrl->merge_side2);
			}
		}
	}
	LeaveCriticalSection(&g_NetworkCS);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void RSNReceiveCommon(RECEIVE_DATA *ptr, DWORD size, LPARAM lp){
	int i;
	RSNMessageType type = *(RSNMessageType *)ptr;
	char *datahead = (char *)ptr+sizeof(RSNMessageType);
	switch(type){
	case RSN_MSG_CHAT: {
		RSNNotifyString *recv = (RSNNotifyString *)ptr;
		D3DCOLOR color = 0xffffffff;
		EnterCriticalSection(&g_NetworkCS);
		string name = "???";
		for(i = 0; i<g_NetworkMembers.size(); i++){
			RSNMemberInfo &info = g_NetworkMembers[i];
			if(info.id==recv->id){
				name = info.name;
				color = info.color;
				break;
			}
		}
		LeaveCriticalSection(&g_NetworkCS);
		PushChatLog(FlashIn("%s : %s", name.c_str(), recv->str), color);
		return; }
	case RSN_MSG_TRANSFER_REQUEST: {
		EnterCriticalSection(&g_NetworkCS);
		RSNTransferRequest *recv = (RSNTransferRequest *)ptr;
		RSNTransferResponse send;
		send.type = RSN_MSG_TRANSFER_RESPONSE;
		send.id = GetLocalPlayerID();
		send.state = recv->state;
		if(g_NetworkTransferState){
			send.accept = 0;
		}else{
			send.accept = 1;
			g_NetworkTransferFrom = recv->id;
			g_NetworkTransferState = recv->state;
			g_NetworkTransferSize = recv->size;
			g_NetworkTransferRestSize = g_NetworkTransferSize;
			g_NetworkTransferParts =
				(g_NetworkTransferSize+RSN_TRANSFER_STRIDE-1)/RSN_TRANSFER_STRIDE;
			DELETE_A(g_NetworkTransferData);
			DELETE_A(g_NetworkTransferFlag);
			g_NetworkTransferData = new char[recv->size];
			g_NetworkTransferFlag = new char[g_NetworkTransferParts];
			for(i = 0; i<g_NetworkTransferParts; i++) g_NetworkTransferFlag[i] = 0;
		}
		SendTo(recv->id, &send, sizeof(send));
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	case RSN_MSG_TRANSFER_RESPONSE: {
		RSNTransferResponse *recv = (RSNTransferResponse *)ptr;
		if(recv->accept && recv->id==g_NetworkSendTo && recv->state==g_NetworkSendState){
			int cur = 0;
			i = 0;
			g_NetworkSending = true;
			RSNTransferData send;
			send.type = RSN_MSG_TRANSFER_DATA;
			send.id = GetLocalPlayerID();
			send.state = g_NetworkSendState;
			while(cur<g_NetworkSendSize){
				send.partid = i;
				send.partsize = g_NetworkSendSize - cur;
				if(send.partsize>RSN_TRANSFER_STRIDE) send.partsize = RSN_TRANSFER_STRIDE;
				memcpy(send.data, g_NetworkSendData+cur, send.partsize);
				//Dialog("sending %d-%d/%d", cur, cur+send.partsize, g_NetworkSendSize);
				bool abort = false;
				EnterCriticalSection(&g_NetworkCS);
				if(!g_NetworkSendAbort) SendTo(g_NetworkSendTo, &send, 20+send.partsize);
				LeaveCriticalSection(&g_NetworkCS);
				if(g_NetworkSendAbort) break;
				i++;
				g_NetworkSendRestSize -= send.partsize;
				cur += send.partsize;
			}
			g_NetworkSending = false;
			g_NetworkSendState = RSN_TRANS_NONE;
		}
		return; }
	case RSN_MSG_TRANSFER_DATA: {
		RSNTransferData *recv = (RSNTransferData *)ptr;
		if(recv->id==g_NetworkTransferFrom && recv->state==g_NetworkTransferState){
			if(g_NetworkTransferFlag[recv->partid]){
				ErrorDialog("double transfer %d", recv->partid);
			}else{
				memcpy(g_NetworkTransferData+recv->partid*RSN_TRANSFER_STRIDE,
					recv->data, recv->partsize);
				g_NetworkTransferFlag[recv->partid] = 1;
				g_NetworkTransferRestSize -= recv->partsize;
				if(g_NetworkTransferRestSize<0){
					ErrorDialog("rest size error %d", g_NetworkTransferRestSize);
				}else{
					RSNTransferComplete send;
					send.type = RSN_MSG_TRANSFER_COMPLETE;
					send.id = GetLocalPlayerID();
					send.state = g_NetworkTransferState;
					SendTo(g_NetworkTransferFrom, &send, sizeof(send));
				}
			}
		}
		return; }
	case RSN_MSG_EXCEED_SYNC_LIMIT: {
		RSNExceedSyncLimit *recv = (RSNExceedSyncLimit *)ptr;
		//if(recv->id==GetLocalPlayerID()) return;
		EnterCriticalSection(&g_NetworkCS);
		for(i = 0; i<g_NetworkMembers.size(); i++){
			RSNMemberInfo &info = g_NetworkMembers[i];
			if(info.id==recv->id){
				info.sim_speed = recv->sim_speed;
				info.sync_limit = recv->limit;
				if(recv->train_group!=NULL){
					RSNTrainControlInfo train_ctrl;
					train_ctrl.sync_limit = recv->limit;
					train_ctrl.train_group = recv->train_group;
					train_ctrl.target_speed = recv->target_speed;
					info.train_ctrl.push_back(train_ctrl);
				}
				if(recv->switch_id>=0){
					RSNSwitchControlInfo switch_ctrl;
					switch_ctrl.sync_limit = recv->limit;
					switch_ctrl.switch_id = recv->switch_id;
					switch_ctrl.switch_opt = recv->switch_opt;
					info.switch_ctrl.push_back(switch_ctrl);
				}
				if(recv->point_id!=NULL){
					RSNPointControlInfo point_ctrl;
					point_ctrl.sync_limit = recv->limit;
					point_ctrl.point_id = recv->point_id;
					point_ctrl.point_opt = recv->point_opt;
					info.point_ctrl.push_back(point_ctrl);
				}
				if(recv->set_group!=NULL){
					RSNSetTrainControlInfo set_train_ctrl;
					set_train_ctrl.sync_limit = recv->limit;
					set_train_ctrl.set_group = recv->set_group;
					set_train_ctrl.set_rail = recv->set_rail;
					set_train_ctrl.set_side = recv->set_side;
					set_train_ctrl.set_sumlen = recv->set_sumlen;
					set_train_ctrl.set_flag = recv->set_flag;
					info.set_train_ctrl.push_back(set_train_ctrl);
				}
				if(recv->merge_group1!=NULL){
					RSNMergeTrainControlInfo merge_train_ctrl;
					merge_train_ctrl.sync_limit = recv->limit;
					merge_train_ctrl.merge_group1 = recv->merge_group1;
					merge_train_ctrl.merge_group2 = recv->merge_group2;
					merge_train_ctrl.merge_side1 = recv->merge_side1;
					merge_train_ctrl.merge_side2 = recv->merge_side2;
					info.merge_train_ctrl.push_back(merge_train_ctrl);
				}
			}
		}
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	case RSN_MSG_SIMULATION_PAUSED: {
		RSNNotifyID *recv = (RSNNotifyID *)ptr;
		EnterCriticalSection(&g_NetworkCS);
		for(i = 0; i<g_NetworkMembers.size(); i++){
			RSNMemberInfo &info = g_NetworkMembers[i];
			if(info.id==recv->id){
				info.sim_speed = 0;
				break;
			}
		}
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	}
}

void RSNReceiveHost(RECEIVE_DATA *ptr, DWORD size, LPARAM lp){
	int i;
	RSNMessageType type = *(RSNMessageType *)ptr;
	char *datahead = (char *)ptr+sizeof(RSNMessageType);
	switch(type){
	case RSN_MSG_NOTIFY_JOIN: {
		RSNNotifyJoin *recv = (RSNNotifyJoin *)ptr;
		if(recv->version!=RAILSIM_VERSION){
			RSNSendNotifyID(RSN_MSG_INVALID_VERSION, recv->id);
			return;
		}
		if(g_NetworkStarted){
			RSNSendNotifyID(RSN_MSG_ENTRY_CLOSED, recv->id);
			return;
		}
		EnterCriticalSection(&g_NetworkCS);
		if(g_NetworkMembers.size()>=RSN_MEMBER_MAX){
			LeaveCriticalSection(&g_NetworkCS);
			RSNSendNotifyID(RSN_MSG_MAXIMUM_MEMBERS, recv->id);
			return;
		}
		RSNMemberInfo new_user;
		new_user.id = recv->id;
		new_user.name = recv->name;
		new_user.color_id = RSNAllocateColor();
		new_user.color = RSN_MEMBER_COLORS[new_user.color_id];
		new_user.sim_speed = 0;
		g_NetworkMembers.push_back(new_user);
		g_NetworkMemberUpdated = true;
		PushChatLog(FlashIn("%s %s", recv->name, lang(HasJoined)));
		RSNSendNotifyDigest(RSN_MSG_ACCEPT_CLIENT, recv->id, g_NetworkFileDigest);
		RSNSendNotifyMemberList(RSN_MSG_MEMBER_LIST, DPNID_ALL_PLAYERS_GROUP);
		RSNNotifyGeneralInfo ginfo;
		ginfo.type = RSN_MSG_GENERAL_INFO;
		ginfo.id = GetLocalPlayerID();
		ginfo.sync_interval = g_NetworkSyncInterval;
		SendTo(recv->id, &ginfo, sizeof(ginfo));
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	case RSN_MSG_REQUEST_LAYOUT: {
		RSNNotifyID *recv = (RSNNotifyID *)ptr;
		EnterCriticalSection(&g_NetworkCS);
		if(find(g_LayoutTransferQueue.begin(), g_LayoutTransferQueue.end(), recv->id)
			==g_LayoutTransferQueue.end()) g_LayoutTransferQueue.push_back(recv->id);
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	case RSN_MSG_LEAVE_SESSION: {
		RSNNotifyID *recv = (RSNNotifyID *)ptr;
		EnterCriticalSection(&g_NetworkCS);
		for(i = 0; i<g_NetworkMembers.size(); i++){
			RSNMemberInfo &info = g_NetworkMembers[i];
			if(info.id==recv->id){
				PushChatLog(FlashIn("%s %s", info.name.c_str(), lang(HasLeft)));
				RSNReleaseColor(info.color_id);
				g_NetworkMembers.erase(g_NetworkMembers.begin()+i);
				RSNSendNotifyMemberList(RSN_MSG_MEMBER_LIST, DPNID_ALL_PLAYERS_GROUP);
				g_NetworkMemberUpdated = true;
				break;
			}
		}
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	}
	RSNReceiveCommon(ptr, size, lp);
}

void RSNReceiveClient(RECEIVE_DATA *ptr, DWORD size, LPARAM lp){
	int i, j;
	RSNMessageType type = *(RSNMessageType *)ptr;
	char *datahead = (char *)ptr+sizeof(RSNMessageType);
	switch(type){
	case RSN_MSG_MEMBER_LIST: {
		RSNNotifyMemberList *recv = (RSNNotifyMemberList *)ptr;
		EnterCriticalSection(&g_NetworkCS);
		for(j = 0; j<g_NetworkMembers.size(); j++){
			RSNMemberInfo &info = g_NetworkMembers[j];
			bool leave = true;
			for(i = 0; i<recv->num; i++){
				if(info.id==recv->data[i].id && info.name==recv->data[i].name){
					leave = false;
					break;
				}
			}
			if(leave){
				if(!IsHost() && info.id==GetLocalPlayerID()){
					if(!g_NetworkCloseRequest){
						g_NetworkCloseRequest = true;
						g_NetworkClosedByHost = true;
						EnqueueCommonDialog(new CSimpleDialog(lang(DisconnectedByHost), lang(NetworkError)));
					}
					LeaveCriticalSection(&g_NetworkCS);
					return;
				}else{
					PushChatLog(FlashIn("%s %s", info.name.c_str(), lang(HasLeft)));
				}
			}
		}
		for(i = 0; i<recv->num; i++){
			bool join = true;
			for(j = 0; j<g_NetworkMembers.size(); j++){
				RSNMemberInfo &info = g_NetworkMembers[j];
				if(info.id==recv->data[i].id && info.name==recv->data[i].name){
					join = false;
					break;
				}
			}
			if(join) PushChatLog(FlashIn("%s %s", recv->data[i].name, lang(HasJoined)));
		}
		g_NetworkMembers.clear();
		for(i = 0; i<recv->num; i++){
			RSNMemberInfo new_user;
			new_user.id = recv->data[i].id;
			new_user.name = recv->data[i].name;
			new_user.color_id = 0;
			new_user.color = recv->data[i].color;
			new_user.sim_speed = 0;
			new_user.sync_limit = recv->data[i].sync_limit;
			g_NetworkMembers.push_back(new_user);
		}
		g_NetworkMemberUpdated = true;
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	case RSN_MSG_INVALID_VERSION:
		g_NetworkCloseRequest = true;
		EnqueueCommonDialog(new CSimpleDialog(
			lang(ProgramVersionMismatch), lang(NetworkError)));
		return;
	case RSN_MSG_MAXIMUM_MEMBERS:
		g_NetworkCloseRequest = true;
		EnqueueCommonDialog(new CSimpleDialog(
			lang(MaximumMembersReached), lang(NetworkError)));
		return;
	case RSN_MSG_ENTRY_CLOSED:
		g_NetworkCloseRequest = true;
		EnqueueCommonDialog(new CSimpleDialog(
			lang(EntryAlreadyClosed), lang(NetworkError)));
		return;
	case RSN_MSG_ACCEPT_CLIENT: {
		RSNNotifyDigest *recv = (RSNNotifyDigest *)ptr;
		g_NetworkHostID = recv->id;
		memcpy(g_NetworkFileDigest, recv->digest, 16);
		if(!g_NetworkJoinWithFile) RSNSendNotifyID(RSN_MSG_REQUEST_LAYOUT, g_NetworkHostID);
		return; }
	case RSN_MSG_GENERAL_INFO: {
		RSNNotifyGeneralInfo *recv = (RSNNotifyGeneralInfo *)ptr;
		g_NetworkSyncInterval = recv->sync_interval;
		return; }
	case RSN_MSG_CLOSE_SESSION: {
		if(!g_NetworkCloseRequest){
			g_NetworkCloseRequest = true;
			g_NetworkClosedByHost = true;
			EnqueueCommonDialog(new CSimpleDialog(lang(HostClosedSession), lang(NetworkError)));
		}
		return; }
	case RSN_MSG_START_SIMULATION: {
		EnterCriticalSection(&g_NetworkCS);
		if(!g_NetworkStarted){
			g_NetworkStarted = true;
			CGameMode::SetNeutral();
			PushChatLog(lang(StartSimulation));
		}
		LeaveCriticalSection(&g_NetworkCS);
		return; }
	}
	RSNReceiveCommon(ptr, size, lp);
}

void RSNReceiveDummy(RECEIVE_DATA *, DWORD, LPARAM){}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void InitializeNetwork(){
	static bool first_init = true;
	if(first_init){
		first_init = false;
		InitializeCriticalSection(&g_NetworkCS);
	}
	g_NetworkDummyMapAddress = 1;
	g_NetworkHostID = NULL;
	g_NetworkCloseRequest = false;
	g_NetworkClosedByHost = false;
	g_NetworkInitialized = true;
	g_NetworkStarted = false;
	g_NetworkLastSimSpeed = 0;
	g_NetworkSyncLimitReceived = 0;
	g_NetworkSyncLimitSent = 0;
	int i;
	for(i = 0; i<RSN_MEMBER_MAX; i++) g_RSNMemberColorUsed[i] = false;
	g_NetworkMembers.clear();
	g_NetworkMemberUpdated = false;
	g_LayoutTransferQueue.clear();
	g_SwitchControlQueue.clear();
	g_PointControlQueue.clear();
	g_SetTrainControlQueue.clear();
	g_MergeTrainControlQueue.clear();
	g_ChatLog.clear();
}

int RSNCreateSession(int host_port, int sync_interval, const char *screen_name){
	if(g_NetworkInitialized) return 1;
	if(strlen(screen_name)<1) return 2;
	g_NetworkHostPort = host_port;
	g_NetworkScreenName = screen_name;
	g_LastCreatedHostPort = g_NetworkHostPort;
	g_LastScreenName = g_NetworkScreenName;
	SetReceiveFunc(&RSNReceiveHost, NULL);
	if(!CreateSession(&RSN_GUID_APP, "RailSimNetPlay", g_NetworkHostPort)){
		CloseSession();
		return 3;
	}
	InitializeNetwork();
	g_NetworkHostID = GetLocalPlayerID();
	MD5 hash;
	hash.update((unsigned char *)g_NetworkFileCopy, g_NetworkFileCopySize);
	hash.finalize();
	memcpy(g_NetworkFileDigest, hash.raw_digest(), 16);
	RSNMemberInfo new_user;
	new_user.id = GetLocalPlayerID();
	new_user.name = g_NetworkScreenName;
	new_user.color_id = RSNAllocateColor();
	new_user.color = RSN_MEMBER_COLORS[new_user.color_id];
	new_user.sim_speed = 0;
	g_NetworkMembers.push_back(new_user);
	g_NetworkMemberUpdated = true;
	PushChatLog(FlashIn("%s %s", screen_name, lang(HasJoined)));
	return 0;
}

int RSNJoinSession(const char *ip_port, int local_port,
	bool join_with_file, const char *screen_name){
	if(g_NetworkInitialized) return 1;
	if(strlen(screen_name)<1) return 2;
	g_NetworkSyncInterval = 10;
	g_NetworkLocalPort = local_port;
	g_NetworkJoinWithFile = join_with_file;
	g_NetworkScreenName = screen_name;
	if(sscanf(ip_port, "%d.%d.%d.%d:%d",
		&g_NetworkHostIP[0], &g_NetworkHostIP[1],
		&g_NetworkHostIP[2], &g_NetworkHostIP[3], &g_NetworkHostPort)!=5){
		return 4;
	}
	int i;
	for(i = 0; i<4; i++) g_LastJoinedHostIP[i] = g_NetworkHostIP[i];
	g_LastJoinedHostPort = g_NetworkHostPort;
	g_LastJoinedLocalPort = g_NetworkLocalPort;
	g_LastScreenName = g_NetworkScreenName;
	//Dialog("host port = %d\nlocal port = %d", g_NetworkHostPort, g_NetworkLocalPort);
	string ipadr = FlashIn("%d.%d.%d.%d",
		g_NetworkHostIP[0], g_NetworkHostIP[1],
		g_NetworkHostIP[2], g_NetworkHostIP[3]);
	SetReceiveFunc(&RSNReceiveClient, NULL);
	if(!JoinSession(&RSN_GUID_APP, ipadr.c_str(),
		g_NetworkHostPort, g_NetworkLocalPort)){
		CloseSession();
		return 3;
	}
	InitializeNetwork();
	RSNNotifyJoin send;
	send.type = RSN_MSG_NOTIFY_JOIN;
	send.id = GetLocalPlayerID();
	send.version = RAILSIM_VERSION;
	strcpy(send.name, g_NetworkScreenName.c_str());
	SendToAll(&send, sizeof(send));
	return 0;
}

void RSNDeleteMember(DPNID id){
	if(!IsHost()) return;
	int i;
	EnterCriticalSection(&g_NetworkCS);
	for(i = 0; i<g_NetworkMembers.size(); i++){
		RSNMemberInfo &info = g_NetworkMembers[i];
		if(info.id==id){
			PushChatLog(FlashIn("%s %s", info.name.c_str(), lang(HasLeft)));
			RSNReleaseColor(info.color_id);
			g_NetworkMembers.erase(g_NetworkMembers.begin()+i);
			RSNSendNotifyMemberList(RSN_MSG_MEMBER_LIST, DPNID_ALL_PLAYERS_GROUP);
			g_NetworkMemberUpdated = true;
			break;
		}
	}
	LeaveCriticalSection(&g_NetworkCS);
}

bool RSNCloseSession(){
	if(!g_NetworkInitialized) return false;
	EnterCriticalSection(&g_NetworkCS);
	while(g_NetworkSending){
		g_NetworkSendAbort = true;
		LeaveCriticalSection(&g_NetworkCS);
		Sleep(100);
		EnterCriticalSection(&g_NetworkCS);
	}
	if(IsHost()) RSNSendNotifyID(RSN_MSG_CLOSE_SESSION, DPNID_ALL_PLAYERS_GROUP);
	else if(g_NetworkHostID && !g_NetworkClosedByHost) RSNSendNotifyID(RSN_MSG_LEAVE_SESSION, g_NetworkHostID);
	RSNEndTransferData();
	SetReceiveFunc(&RSNReceiveDummy, NULL);
	if(CloseSession()){
		g_NetworkMembers.clear();
		g_NetworkInitialized = false;
		g_NetworkCloseRequest = false;
	}
	LeaveCriticalSection(&g_NetworkCS);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CheckNetworkState(){
	if(!g_NetworkInitialized) return;
	if(g_NetworkCloseRequest){
		RSNCloseSession();
		return;
	}
	EnterCriticalSection(&g_NetworkCS);
	if(g_LayoutTransferQueue.size()){
		if(RSNBeginTransferData(g_LayoutTransferQueue[0],
			g_NetworkFileCopy, g_NetworkFileCopySize+1, RSN_TRANS_LAYOUT)){
			g_LayoutTransferQueue.erase(g_LayoutTransferQueue.begin());
		}
	}
	LeaveCriticalSection(&g_NetworkCS);
}

void ListNetworkMember(CListView *lv){
	EnterCriticalSection(&g_NetworkCS);
	int i, j, n = g_NetworkMembers.size();
	if(g_NetworkMemberUpdated){
		g_NetworkMemberUpdated = false;
		lv->DeleteAllItems();
		for(i = 0; i<n; i++){
			RSNMemberInfo &info = g_NetworkMembers[i];
			lv->InsertItem(-1, (char *)info.name.c_str());
			lv->GetElement(i)->SetData(info.id);
		}
	}
	for(i = 0; i<n; i++){
		RSNMemberInfo &info = g_NetworkMembers[i];
		bool ready = true;
		if(info.id==g_NetworkHostID){
			lv->GetElement(i)->SetString(1, lang(Host));
			continue;
		}
		if(!IsHost()){
			lv->GetElement(i)->SetString(1, lang(Client));
			continue;
		}
		if(g_NetworkSendState==RSN_TRANS_LAYOUT && info.id==g_NetworkSendTo){
			lv->GetElement(i)->SetString(1, FlashIn("%s %d%%", lang(Sending),
				(g_NetworkSendSize-g_NetworkSendRestSize)*100/g_NetworkSendSize));
			continue;
		}
		for(j = 0; j<g_LayoutTransferQueue.size(); j++){
			if(info.id==g_LayoutTransferQueue[j]){
				lv->GetElement(i)->SetString(1, lang(WaitingTransfer));
				ready = false;
				break;
			}
		}
		if(ready) lv->GetElement(i)->SetString(1, lang(Ready));
	}
	LeaveCriticalSection(&g_NetworkCS);
}

void StartNetworkSimulation(){
	if(!IsHost()) return;
	EnterCriticalSection(&g_NetworkCS);
	g_NetworkStarted = true;
	RSNSendNotifyID(RSN_MSG_START_SIMULATION, DPNID_ALL_PLAYERS_GROUP);
	LeaveCriticalSection(&g_NetworkCS);
	PushChatLog(lang(StartSimulation));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const int CHAT_MSG_MAX = 16;

static CInterface g_NetworkInterface;	//	ネットワークインターフェイス
static CInterface g_ChatInterface;		//	ネットワークインターフェイス
static CCheckBox g_ChatCheckBox;		//	チャット表示
static CEditCtrl g_ChatEditCtrl;		//	チャット入力
static CPushButton g_ChatSendButton;	//	チャット送信

bool g_ChatBackground = true;

void InitNetworkInterface(){
	int ty = g_DispHeight-TILE_UNIT*3-TILE_QUAD;
	int tw = 420;
	g_NetworkInterface.Init(0, 0, 1, 1, "", NULL);
	g_ChatInterface.Init(0, 0, 1, 1, "", &g_NetworkInterface);
	g_ChatCheckBox.Init(TILE_QUAD, ty,
		TILE_UNIT, TILE_UNIT, "", &g_NetworkInterface);
	g_ChatCheckBox.SetCheck(1);
	g_ChatEditCtrl.Init(TILE_QUAD*2+TILE_UNIT, ty,
		tw, TILE_UNIT, "", &g_ChatInterface, 64);
	g_ChatEditCtrl.SetKeepIMM(true);
	g_ChatSendButton.Init(TILE_QUAD*3+TILE_UNIT+tw, ty,
		TILE_UNIT*3, TILE_UNIT, lang(Send), &g_ChatInterface);
}

bool ScanInputNetworkInterface(){
	if(!g_NetworkInitialized) return false;
	bool ret = g_NetworkInterface.ScanInput();
	bool show = !!g_ChatCheckBox.GetCheck();
	g_ChatInterface.Show(show);
	if(show){
		if(GetKey(DIK_F)==S_PUSH && CheckCtrl()){
			if(g_ChatEditCtrl.IsFocus()){
				g_ChatEditCtrl.FinishInput();
				g_ChatCheckBox.SetCheck(0);
			}else{
				g_ChatEditCtrl.GiveFocus(false);
			}
		}
		if(g_ChatEditCtrl.IsFocus() && !g_ChatEditCtrl.IsComp()
			&& (GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH)
			g_ChatSendButton.SetPush(true);
		if(g_ChatSendButton.IsPushed()){
			g_ChatEditCtrl.FinishInput();
			char *chat = g_ChatEditCtrl.GetText();
			if(*chat){
				if(!strcmp(chat, "/help") || !strcmp(chat, "/h")){
					PushChatLog(FlashIn("===== %s =====", lang(ChatCommandList)));
					PushChatLog(FlashIn("/h or /help : %s", lang(PrintChatCommandList)));
					PushChatLog(FlashIn("/m or /members : %s", lang(PrintMemberList)));
					PushChatLog(FlashIn("/b or /background : %s", lang(ToggleChatBackground)));
					PushChatLog(FlashIn("/c or /clear : %s", lang(ClearChatLog)));
					PushChatLog(lang(ChatShortcutExp));
				}else if(!strcmp(chat, "/members") || !strcmp(chat, "/m")){
					EnterCriticalSection(&g_NetworkCS);
					int i, n = g_NetworkMembers.size();
					PushChatLog(FlashIn("===== %s =====", lang(MemberList)));
					for(i = 0; i<n; i++){
						RSNMemberInfo &info = g_NetworkMembers[i];
						PushChatLog(FlashIn("%02d> %s (x%d)",
							i, info.name.c_str(), info.sim_speed), info.color);
					}
					LeaveCriticalSection(&g_NetworkCS);
				}else if(!strcmp(chat, "/b") || !strcmp(chat, "/background")){
					g_ChatBackground = !g_ChatBackground;
				}else if(!strcmp(chat, "/c") || !strcmp(chat, "/clear")){
					g_ChatLog.clear();
				}else{
					RSNSendNotifyString(RSN_MSG_CHAT, DPNID_ALL_PLAYERS_GROUP, chat);
				}
				g_ChatEditCtrl.SetText("");
			}
			g_ChatEditCtrl.GiveFocus(false);
		}
	}else{
		if(GetKey(DIK_F)==S_PUSH && CheckCtrl()){
			g_ChatCheckBox.SetCheck(1);
			g_ChatEditCtrl.GiveFocus(false);
		}
	}
	return ret;
}

void RenderNetworkInterface(){
	if(!g_NetworkInitialized) return;
	g_NetworkInterface.Render();
	if(g_ChatInterface.IsVisible()){
		int i, n = g_ChatLog.size();
		list<RSNChatLog>::iterator itr = g_ChatLog.begin();
		for(i = 0; i<n; i++){
			CStringDrawer *sd = g_StrTex->DrawString(itr->chat.c_str(), 0xff000000);
			int tx = TILE_QUAD, tw = sd->GetWidth();
			int ty = g_DispHeight-TILE_UNIT*3-TILE_QUAD*2-FONT_HEIGHT*(n-i);
			if(g_ChatBackground){
				devSetTexture(0, NULL);
				Fill2DRect(tx, ty, tx+tw, ty+FONT_HEIGHT, 0x80000000);
			}
			sd->RenderLeft(tx, ty, itr->color);
			itr++;
		}
	}
}

void PushChatLog(char *chat, D3DCOLOR color){
	if(!g_NetworkInitialized) return;
	EnterCriticalSection(&g_NetworkCS);
	while(g_ChatLog.size()>=CHAT_MSG_MAX) g_ChatLog.pop_front();
	RSNChatLog log;
	log.chat = chat;
	log.color = color;
	g_ChatLog.push_back(log);
	LeaveCriticalSection(&g_NetworkCS);
}
