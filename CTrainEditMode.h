#ifndef CTRAINEDITMODE_H_INCLUDED
#define CTRAINEDITMODE_H_INCLUDED

#include "CTrainListView.h"
#include "C3DPluginMode.h"

class CTrain;
class CTrainPlugin;
class CTrainGroupTemplate;

/*
 *	車輌編成モード
 */
class CTrainEditMode: public CModelPluginMode{
private:
	bool m_GroupPreview;			//	編成プレビュー
	CPushButton m_AddButton;		//	車輌追加ボタン
	CWindowCtrl m_TemplateWindow;	//	テンプレートプレビュー窓
	CListView m_TemplateListView;	//	テンプレートリストビュー
	CWindowCtrl m_GroupWindow;		//	編成窓
	CGroupListView m_GroupListView;	//	編成リスト
	CWindowCtrl m_TrainWindow;		//	車輌窓
	CTrainListView m_TrainListView;	//	車輌リスト
	CPopMenu *m_GroupMenu;			//	編成メニュー
	CPopMenu *m_TrainMenu;			//	車輌メニュー
	CPopMenu *m_TemplateMenu;		//	編成テンプレートメニュー
public:
	CTrainEditMode();
	~CTrainEditMode();
	void WindowResized(int, int, CWindowCtrl *);
	CPopMenu *Dispatch(CMDTYPE, DWORD);
	CListView *GetTemplateListView(){ return &m_TemplateListView; }
	void DoubleClick(CMDTYPE, DWORD);
	void SwitchPreviewMode();
	void AddGroup();
	void DeleteGroup(CTrainGroup *);
	void NewFromTemplate(CTrainGroupTemplate *);
	void AddFromTemplate(CTrainGroupTemplate *);
	void AddTrain(CTrainPlugin *);
	void DeleteTrain(CTrain *);
	CModelPlugin *GetModelPlugin();
	char *PluginDirName(){ return "Train"; }
	CPluginList *GetPluginList();
	CPlugin *FindModelPlugin(char *, char *);
	char *LoadModelPluginSetting(char *);
	void EnterModelPlugin();
	void ModalFuncModelPlugin();
	CModelInst *ScanInputModelPlugin();
	void RenderModelPlugin();
};

//	外部グローバル
extern CTrainEditMode *g_TrainEditMode;

#endif
