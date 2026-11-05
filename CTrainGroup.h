#ifndef CTRAINGROUP_H_INCLUDED
#define CTRAINGROUP_H_INCLUDED

#include "CTrainSetBuffer.h"
#include "CDiaInst.h"

class CRailConnector;
class CRailLinkTemp;
class CModelInst;
class CModelSwitch;
class CTrain;
class CPlatformInst;
class CTrainPlugin;
class CListView;
class CListElement;
class CPluginTree;
class CTrainGroupTemplate;

/*
 *	編成インスタンス
 */
class CTrainGroup{
private:
	void *m_OldAdr;						//	旧アドレス
	int m_ControlState;					//	操作状態
	int m_Serial;						//	シリアルナンバー
	int m_State;						//	状態
	int m_DoorWait;						//	ドア待ちカウンタ
	float m_Length;						//	長さ
	float m_MaxVelocity;				//	最高速度
	float m_MaxAcceleration;			//	最高加速度
	float m_MaxDeceleration;			//	最高減速度
	float m_DoorClosingTime;			//	ドア閉め所要時間
	float m_TargetSpeed;				//	設定速度
	float m_EffectTargetSpeed;			//	設定速度
	float m_CurrentSpeed;				//	現在速度
	float m_OldSpeed;					//	旧速度
	int m_SpeedLimit;					//	制限速度
	float m_StopTarget;					//	停止目標距離
	float m_PreviewOffset;				//	プレビューオフセット
	double m_DepartureTime;				//	出発時刻
	bool m_OpenDoor[2];					//	ドア開
	bool m_Reverse;						//	後退モード
	bool m_Enabled;						//	配置フラグ
	bool m_NotifyFlag;					//	プラットフォーム通知フラグ
	int m_SplitPos;						//	分割予定位置
	VEC3 m_CabinPos[2];					//	運転席位置オフセット
	VEC3 m_CabinDir[2];					//	運転席視点方向
	string m_Name;						//	編成名
	CListElement *m_ListElement;		//	リスト要素
	list<CTrainSetBuffer> m_SetBuffer;	//	配置バッファ
	CGroupEndLocator m_Location[2];		//	終端位置
	CGroupEndLocator m_Seeker;			//	経路探索子
	set<CRailConnector *> m_PointList;	//	通過中ポイントリスト
	set<CRailConnector *> m_SeekList;	//	探索中ポイントリスト
	set<CRailConnector *> m_OldSeek;	//	旧探索中ポイントリスト
	CDiaElement m_DiaElement;			//	ダイヤ要素
	CPlatformInst *m_Platform;			//	プラットフォーム
	CTrain *m_TrainList;				//	車輌リスト
	CTrain *m_SelectTrain;				//	選択中車輌
	CTrainGroup *m_Next;				//	次
public:
	CTrainGroup(char *);
	~CTrainGroup();
	CTrainGroup *Next(){ return m_Next; }
	CTrainGroup **NextAdr(){ return &m_Next; }
	void *OldAdr(){ return m_OldAdr; }
	char *GetName(){ return (char *)m_Name.c_str(); }
	void SetName(char *name){ m_Name = name; }
	void SetListElement(CListElement *le){ m_ListElement = le; }
	CListElement *GetListElement(){ return m_ListElement; }
	CTrain *GetSelectedTrain(){ return m_SelectTrain; }
	void MakeTemplate(CTrainGroupTemplate *);
	bool IsSet(){ return m_Enabled; }
	int GetSerial(){ return m_Serial; }
	void SetSerial(int s){ m_Serial = s; }
	int GetTrainNum();
	void OnTrainNumChanged();
	vector<CTrain *> GetTrainByVector();
	void SetTrainByVector(vector<CTrain *>);
	int GetState(){ return m_State; }
	bool IsReverse(){ return m_Reverse; }
	void CalcSpec();
	void CheckTargetSpeed();
	void PrintInfo();
	CModelInst *Control(CTrain *);
	float GetTargetSpeed(){ return m_TargetSpeed; }
	void SetTargetSpeed(float ts){ m_TargetSpeed = ts; }
	float GetEffectTargetSpeed(){ return m_EffectTargetSpeed; }
	void SetEffectTargetSpeed(float ets){ m_EffectTargetSpeed = ets; }
	void SetSpeedLimit(int);
	float CalcSignedSpeedLimit();
	bool CalcViewAxis(CObject *);
	void SetCabinView();
	void AddTrain(CTrainPlugin *, bool);
	void DeleteTrain(CTrain *);
	void ListTrain(CListView *);
	CTrain *EditTrain(CListView *, CPluginTree *);
	void SetGroupCommonSwitch(CModelSwitch *, int);
	void Remove();
	void SetSetBuffer();
	void SetError(const char *);
	bool Set(CRailLinkTemp *, int);
	bool Trail(bool, bool);
	void NotifyWarp();
	void NotifyPlatform(CPlatformInst *, float, bool);
	void DeletePlatform(CPlatformInst *pf){ if(m_Platform==pf) m_Platform = NULL; }
	void ClearPoint();
	void ClearSeek();
	void AddPoint(CRailConnector *pcon){ m_PointList.insert(pcon); }
	bool AddSeek(CRailConnector *);
	void SetConnectSwitch(CTrain *);
	void ScanInput(int, VEC3 &, VEC3 &);
	void Preview();
	void Render();
	void Simulate();
	void MergeTrain(CTrainGroup *, int, int);
	void SplitTrain(int);
	void RestoreSet();
	void RestoreAddress();
	char *Read(char *, CTrainGroup ***);
	void Save(FILE *);
};

//	外部グローバル
extern CTrainGroup *g_TrainGroup;

CTrainGroup *GetTrainGroupBySerial(int);

#endif
