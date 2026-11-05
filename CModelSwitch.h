#ifndef CMODELSWITCH_H_INCLUDED
#define CMODELSWITCH_H_INCLUDED

#include "CCustomizer.h"
#include "CEffector.h"

class CListView;
class CListElement;
class CExpression;
class CModelPlugin;
class CModelInst;

//	システムスイッチ番号
extern enum{
	SYS_SW_FRONT = 0,
	SYS_SW_CONNECT1,
	SYS_SW_CONNECT2,
	SYS_SW_DOOR1,
	SYS_SW_DOOR2,
	SYS_SW_SERIAL,
	SYS_SW_CAMDIST,
	SYS_SW_VELOCITY,
	SYS_SW_ACCEL,
	SYS_SW_CABINVIEW,
	SYS_SW_APPROACH1,
	SYS_SW_APPROACH2,
	SYS_SW_STOPPING,
	//	↑ instance switches (repaired when afterrender)
	SYS_SW_NIGHT,
	SYS_SW_WEATHER,
	SYS_SW_SEASON,
	SYS_SW_SHADOW,
	SYS_SW_ENVMAP,
	SYS_SW_YEAR,
	SYS_SW_MONTH,
	SYS_SW_DAY,
	SYS_SW_DAYOFWEEK,
	SYS_SW_HOUR,
	SYS_SW_MINUTE,
	SYS_SW_SECOND,
	SYS_SW_FORCE_DWORD = 0xffffffff
} SYS_SW_ID;

//	内部定数
const int SYSTEM_SWITCH_NUM = 25;	//	システムスイッチ数
const int INSTANCE_SWITCH_NUM = 13;	//	内、インスタンススイッチ数

/*
 *	モデルスイッチ
 */
class CModelSwitch{
	friend class CModelPlugin;
	friend class CSwitchApplier;
private:
	int m_ID;						//	番号
	int m_Value;					//	値
	string m_SwitchName;			//	スイッチ名
	string m_GroupCommon;			//	編成共通識別子
	list<string> m_Entry;			//	エントリ
	CListElement *m_ListElement;	//	リスト要素
public:
	CModelSwitch();
	CModelSwitch(int);
	void Init(int, char *);
	int GetValue(){ return m_Value; }
	void SetValue(int v){ m_Value = v; }
	char *Read(char *, CModelPlugin *);
	CModelSwitch *Check(const string &name){ return m_SwitchName==name ? this : NULL; }
	bool IsGroupCommon(){ return !!m_GroupCommon.size(); }
	CModelSwitch *CheckGroupCommon(CModelSwitch *rhs){
		return m_GroupCommon==rhs->m_GroupCommon ? this : NULL;
	}
	char *GetCurrentOptionName();
	void ListEntry(CListView *);
	void EditSwitch(CListView *, int *, int);
	void SetListElement(CListElement *le){ m_ListElement = le; }
};

//	反復子
typedef list<CModelSwitch>::iterator IModelSwitch;

/*
 *	ネットワーク同期用スイッチ識別情報
 */
struct CStaticSwitchID{
	CModelInst *inst;
	int index;
	int tmp_opt;
	bool net_sync;
	CStaticSwitchID(CModelInst *ins, int idx)
		: inst(ins), index(idx), tmp_opt(-1), net_sync(false)
	{}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	スイッチ適用エントリ
 */
class CSwitchEntry{
	friend class CSwitchApplier;
protected:
	int m_TypeFlag;			//	タイプフラグ
	int m_ConditionType;	//	0: case, 1: always, 2: true, 3: false, 4: default
	vector<int> m_Value;	//	適用番号
public:
	virtual ~CSwitchEntry(){}
	char *Read(char *, CModelPlugin *, int);
	virtual char *Read2(char *, CModelPlugin *) = 0;
	int GetTypeFlag(){ return m_TypeFlag; }
	bool CheckValue(int, bool);
};

/*
 *	スイッチ適用子
 */
class CSwitchApplier{
protected:
	int m_TypeFlag;				//	タイプフラグ
	CExpression *m_Expression;	//	接続先スイッチ
public:
	CSwitchApplier(){ m_Expression = NULL; }
	CSwitchApplier(const CSwitchApplier *);
	virtual ~CSwitchApplier();
	char *Read(char *, CModelPlugin *);
	virtual char *Read2(char *, CModelPlugin *, int) = 0;
	int GetSwitchValue();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	カスタマイザスイッチ適用エントリ
 */
class CCustomizerSwitchEntry: public CSwitchEntry{
	friend class CSwitchApplier;
private:
	list<CCustomizerContainer> m_Customizer;	//	カスタマイザリスト
public:
	char *Read2(char *, CModelPlugin *);
	void LoadData(CModelPlugin *);
	void SetOffList(CNamedObject *);
	CMesh *GetMesh(float *);
	void SetPosture(CObject *);
	void Apply(CMesh *);
};

//	反復子
typedef list<CCustomizerSwitchEntry>::iterator ICustomizerSwitchEntry;

/*
 *	カスタマイザスイッチ適用子
 */
class CCustomizerSwitchApplier: public CCustomizerBase, public CSwitchApplier{
private:
	list<CCustomizerSwitchEntry> m_Entry;	//	エントリ
public:
	CCustomizerSwitchApplier(){}
	CCustomizerSwitchApplier(const CCustomizerSwitchApplier &rhs):
		CSwitchApplier(&rhs){ m_Entry = rhs.m_Entry; }
	CCustomizerBase *Duplicate(){ return new CCustomizerSwitchApplier(*this); }
	char *Read2(char *, CModelPlugin *, int);
	void LoadDataCustomizer(CModelPlugin *);
	void SetOffListCustomizer(CNamedObject *);
	int GetTypeFlagCustomizer(){ return m_TypeFlag; }
	CMesh *GetMeshCustomizer(float *);
	void SetPostureCustomizer(CObject *);
	void ApplyCustomizer(CMesh *);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	エフェクタスイッチ適用エントリ
 */
class CEffectorSwitchEntry: public CSwitchEntry{
	friend class CSwitchApplier;
private:
	list<CEffectorContainer> m_Effector;	//	カスタマイザリスト
public:
	char *Read2(char *, CModelPlugin *);
	void LoadData(CModelPlugin *);
	void Apply(CScene *);
};

//	反復子
typedef list<CEffectorSwitchEntry>::iterator IEffectorSwitchEntry;

/*
 *	エフェクタスイッチ適用子
 */
class CEffectorSwitchApplier: public CEffectorBase, public CSwitchApplier{
private:
	list<CEffectorSwitchEntry> m_Entry;	//	エントリ
public:
	CEffectorSwitchApplier(){}
	CEffectorSwitchApplier(const CEffectorSwitchApplier &rhs):
		CSwitchApplier(&rhs){ m_Entry = rhs.m_Entry; }
	CEffectorBase *Duplicate(){ return new CEffectorSwitchApplier(*this); }
	char *Read2(char *, CModelPlugin *, int);
	void LoadDataEffector(CModelPlugin *);
	int GetTypeFlagEffector(){ return m_TypeFlag; }
	void ApplyEffector(CScene *);
};

//	関数宣言
void InitSystemSwitch();
void SetCamDistSwitch(VEC3);

//	外部グローバル
extern CModelSwitch g_SystemSwitch[];
extern vector<CStaticSwitchID> g_StaticSwitchTable;

#endif
