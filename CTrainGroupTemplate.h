#ifndef CTRAINGROUPTEMPLATE_H_INCLUDED
#define CTRAINGROUPTEMPLATE_H_INCLUDED

#include "CWindowCtrl.h"
#include "CListView.h"
#include "CPlugin.h"

class CTrainPlugin;
class CTrainGroup;

/*
 *	車輌テンプレート
 *
 *	スイッチ付き車輌プラグインポインタ
 */
class CTrainTemplate{
	friend class CTrainGroupTemplate;
private:
	bool m_Reverse;					//	反転フラグ
	vector<int> m_SwitchOption;		//	スイッチ設定値
	string m_TrainID;				//	車輌 ID
	CTrainPlugin *m_TrainPlugin;	//	車輌プラグイン
public:
	CTrainTemplate(){
		Init(NULL, false, R2L(vector<int>()));
	}
	CTrainTemplate(const CTrainTemplate &rhs){
		Init(rhs.m_TrainPlugin, rhs.m_Reverse, R2L(vector<int>(rhs.m_SwitchOption)));
		m_TrainID = rhs.m_TrainID;
	}
	CTrainTemplate(CTrainPlugin *tpi, bool turn, vector<int> &sopt){
		Init(tpi, turn, sopt);
	}
	void Init(CTrainPlugin *, bool, vector<int> &);
	char *Read(char *);
	void Save(FILE *);
	void LoadTrainPlugin();
	void AddToGroup(CTrainGroup *);
};

//	反復子
typedef list<CTrainTemplate>::iterator ITrainTemplate;

/*
 *	編成テンプレート
 */
class CTrainGroupTemplate: public CPlugin{
private:
	list<CTrainTemplate> m_TrainList;	//	車輌リスト
	CTreeFileElement *m_TreeElement;	//	ツリー要素
public:
	CTrainGroupTemplate(char *);
	void PushTrain(CTrainTemplate &t){ m_TrainList.push_back(t); }
	void SetLoaded(){ m_State = 2; }
	char *DirName(){ return "TrainGroupTemplate"; }
	char *TextName2(){ return FlashIn("%s.txt", m_Name.c_str()); }
	char *SaveString(){ return GetName(); }
	bool PreLoadTGT(FILE *);
	bool Load();
	void Save(FILE *);
	string &GetNameRef(){ return m_Name; }
	CTreeFileElement *GetTreeElement(){ return m_TreeElement; }
	void SetTreeElement(CTreeFileElement *);
	bool IsRenamable(){ return true; }
	bool ConfirmRename(string &);
	bool IsDeletable();
	void DeleteFromTree();
	bool DeleteFromDisk();
	bool Rename(string &);
	void AddToGroup(CTrainGroup *);
	void SetPreview();
};

//	反復子
typedef list<CTrainGroupTemplate>::iterator ITrainGroupTemplate;

//	関数宣言
void LoadTrainGroupTemplateList();
ITrainGroupTemplate FindTrainGroupTemplate(CTrainGroupTemplate *);
ITrainGroupTemplate FindTrainGroupTemplate(string);
void SetTemplateMenu(CPopMenu *, CTrainGroupTemplate *, CListElement *);
CMenuCommand *MakeGroupSaver(CTrainGroup *);
void AddTrainGroupTemplate();
void ListTrainGroupTemplate();
void ModalFuncTrainGroupTemplate();

//	外部グローバル
extern CTrainGroupTemplate *g_TrainGroupTemplate;
extern list<CTrainGroupTemplate> g_TrainGroupTemplateList;

#endif
