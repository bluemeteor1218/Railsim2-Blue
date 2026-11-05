#ifndef CTRAINLISTVIEW_H_INCLUDED
#define CTRAINLISTVIEW_H_INCLUDED

#include "CListView.h"

class CTrainGroup;

/*
 *	編成リストビュー
 */
class CGroupListView: public CListView{
private:
public:
	bool IsRenamable(CListElement *le){ return !g_NetworkInitialized && !!le->GetData(); }
	void EndRename(CListElement *);
};

/*
 *	車輌リストビュー
 */
class CTrainListView: public CListView{
private:
public:
	void DoubleClick();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	リネーマ
 */
class CGroupRenamer: public CMenuCommand{
private:
	CTrainGroup *m_Group;	//	編成
public:
	CGroupRenamer(CTrainGroup *gr){ m_Group = gr; }
	void Exec();
};

#endif
