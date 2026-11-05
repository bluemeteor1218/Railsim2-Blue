#ifndef CSAVEFILE_H_INCLUDED
#define CSAVEFILE_H_INCLUDED

class MD5;
class CListView;
class CRailWay;
class CTrainGroup;
class CDiaInstBase;
class CScene;
class CSurfacePlugin;
class CPlatformInst;

/*
 *	セーブファイル
 */
class CSaveFile{
	friend class CFileMode;
private:
	string m_FileName;			//	ファイル名
	string m_FileDate;			//	更新日時
	string m_FileNote;			//	備考
	float m_Version;		//	バージョン
	int m_Year;					//	年
	int m_Month;				//	月
	int m_Day;					//	日
	int m_Hour;					//	時
	int m_Minute;				//	分
	int m_Second;				//	秒
	int m_Frame;				//	フレーム
	int m_DayOfWeek;			//	曜日
	int m_SumDays;				//	日数
	int m_GroupNum;				//	編成数
	int m_SceneNum;				//	シーン数
	VEC3 m_WindDir1;			//	風方向変化前
	VEC3 m_WindDir2;			//	風方向変化後
	int m_WindCount;			//	風方向カウンタ
	int m_WindTime;				//	風方向変化時間
	CRailWay *m_WarpList;		//	ワープ
	CTrainGroup *m_GroupList;	//	編成
	CScene *m_SceneList;		//	シーン
	int m_NetworkSyncCount;		//	ネットワーク同期カウンタ
public:
	CSaveFile(bool);
	~CSaveFile();
	char *GetFileName(){ return (char *)m_FileName.c_str(); }
	void SetFileName(char *fname){ m_FileName = fname; }
	int GetMonth(){ return m_Month; }
	void UpdateWind();
	void SetWarpRoot();
	//bool SetRailBlock(char *);
	bool DeleteWarp();
	void AddGroup();
	void DeleteGroup(CTrainGroup *);
	void ListGroup(CListView *);
	void ListGroupDia(CListView *, CDiaInstBase *);
	CTrainGroup *GetTrainGroup(){ return m_GroupList; }
	void DeletePlatform(CPlatformInst *);
	void NumberGroup();
	void NextGroup(bool);
	vector<CTrainGroup *> GetTrainGroupByVector();
	void SetTrainGroupByVector(vector<CTrainGroup *>);
	void ScanInputWarp(int, VEC3, VEC3);
	void ScanInputInst(int, VEC3, VEC3);
	int GetGroupNum(){ return m_GroupNum; }
	CScene *GetSceneList(){ return m_SceneList; }
	void AddScene(CSurfacePlugin *);
	void DeleteScene(CScene *);
	void ListScene(CListView *);
	void NumberScene();
	void NextScene(bool);
	vector<CScene *> GetSceneByVector();
	void SetSceneByVector(vector<CScene *>);
	void RenderScene(int);
	void Simulate(int);
	void ResetSwitch();
	int GetSceneNum(){ return m_SceneNum; }
	int GetSumDays(){ return m_SumDays; }
	double GetDayTime(){
		return (m_Hour+(m_Minute+(m_Second+m_Frame/30.0)/60.0)/60.0)/24.0;
	}
	double GetAbsTime(){ return m_SumDays+GetDayTime(); }
	char *GetTimeText();
	bool Load(const char *, const char *, bool, bool, char **, int *, bool, char *);
	int Save(const char *, const char *, bool, bool);
};

//	外部グローバル
extern CSaveFile *g_SaveFile;
extern map<void *, void *> g_AddressMap;
extern void *RegisterNewMapAddress(void *);

//	関数宣言
void *ReplaceAdr(void *);

#endif
