#ifndef CDIAINST_H_INCLUDED
#define CDIAINST_H_INCLUDED

class CListElement;
class CListView;
class CTrainGroup;

/*
 *	ダイヤ要素基本クラス
 */
class CDiaElementBase{
	friend class CDiaDialogBase;
	friend class CDiaListBase;
protected:
	CListElement *m_ListElement;	//	リスト要素
public:
	CDiaElementBase();
	virtual ~CDiaElementBase(){}
	void UpdateCaption();
	virtual string GetListCaption() = 0;
	virtual char *Label() = 0;
	virtual char *Read(char *) = 0;
	virtual void Save(FILE *, char *) = 0;
};

//	反復子
typedef list<CDiaElementBase *>::iterator IPDiaElementBase;

/*
 *	ダイヤリスト基本クラス
 */
class CDiaListBase{
	friend class CDiaDialogBase;
	friend class CDiaInstBase;
protected:
	bool m_UseDefault;					//	デフォルト設定を使用
	list<CDiaElementBase *> m_DiaList;	//	ダイヤリスト
public:
	CDiaListBase();
	virtual ~CDiaListBase(){}
	void Free();
	int Size(){ return m_DiaList.size(); }
	void ListElement(CListView *, int);
	int AddElement(CDiaElementBase *);
	int DeleteElement(CDiaElementBase *);
	void RotateElement(bool);
	CDiaElementBase *DequeueBase(bool);
	virtual CDiaElementBase *NewEntry() = 0;
	virtual char *Label() = 0;
	char *Read(char *, CTrainGroup **);
	void Save(FILE *, char *, CTrainGroup *);
};

//	反復子
typedef map<CTrainGroup *, CDiaListBase *>::iterator IPDiaListBase;

/*
 *	ダイヤ関連インスタンス基本クラス
 */
class CDiaInstBase{
	friend class CDiaDialogBase;
protected:
	string m_Name;									//	名称
	map<CTrainGroup *, CDiaListBase *> m_DiaMap;	//	ダイヤマップ
public:
	virtual ~CDiaInstBase();
	bool IsInside(CTrainGroup *group){ return !!m_DiaMap.count(group); }
	bool IsDefault(CTrainGroup *group){
		return m_DiaMap.count(group) ? m_DiaMap[group]->m_UseDefault : true;
	}
	CDiaListBase *Search(CTrainGroup *);
	CDiaListBase *SearchEffectBase(CTrainGroup *);
	CDiaElementBase *DequeueBase(CTrainGroup *g){ return SearchEffectBase(g)->DequeueBase(true); }
	void DeleteGroup(CTrainGroup *g){ m_DiaMap.erase(g); }
	virtual CDiaListBase *NewEntry() = 0;
	virtual char *Label() = 0;
	void RestoreAddress();
	char *Read(char *);
	void Save(FILE *, char *);
};

#define CDIAINSTBASE_CASTFUNC(el, li) \
	li *SearchEffect(CTrainGroup *g){ return (li *)SearchEffectBase(g); } \
	el *Dequeue(CTrainGroup *g){ return (el *)DequeueBase(g); }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ポイント切替要素
 */
class CPointElement: public CDiaElementBase{
	friend class CPointDialog;
private:
	int m_PointSwitch;
public:
	CPointElement();
	string GetListCaption();
	int GetValue(){ return m_PointSwitch; }
	void SetValue(int p){ m_PointSwitch = p; }
	int CalcPoint();
	char *Label(){ return "PointElement"; }
	char *Read(char *);
	void Save(FILE *, char *);
};

/*
 *	ポイント切替要素リスト
 */
class CPointList: public CDiaListBase{
private:
public:
	virtual CDiaElementBase *NewEntry(){ return new CPointElement; }
	char *Label(){ return "PointList"; }
};

/*
 *	ポイント切替マップ
 */
class CPointInst: public CDiaInstBase{
private:
public:
	virtual CDiaListBase *NewEntry(){ return new CPointList; }
	char *Label(){ return "PointInst"; }
	CDIAINSTBASE_CASTFUNC(CPointElement, CPointList);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ダイヤ設定要素
 */
class CDiaElement: public CDiaElementBase{
	friend class CDiaDialog;
	friend class CTrainGroup;
private:
	int m_Action;	//	動作
	int m_TimeType;	//	時間設定
	int m_Hour;		//	時
	int m_Minute;	//	分
	int m_Second;	//	秒
//	int m_Joint;	//	連結を受け入れる
	float m_Offset;	//	停車位置
public:
	CDiaElement();
	string GetListCaption();
	double GetTime(){ return (m_Hour+(m_Minute+m_Second/60.0)/60.0)/24.0; }
	char *Label(){ return "DiaElement"; }
	char *Read(char *);
	void Save(FILE *, char *);
};

/*
 *	ダイヤ設定要素リスト
 */
class CDiaList: public CDiaListBase{
private:
public:
	virtual CDiaElementBase *NewEntry(){ return new CDiaElement; }
	char *Label(){ return "DiaList"; }
};

/*
 *	ダイヤ設定マップ
 */
class CDiaInst: public CDiaInstBase{
private:
public:
	virtual CDiaListBase *NewEntry(){ return new CDiaList; }
	char *Label(){ return "DiaInst"; }
	CDIAINSTBASE_CASTFUNC(CDiaElement, CDiaList);
};

//	外部グローバル
extern char *g_DiaDefaultString[];

#endif
