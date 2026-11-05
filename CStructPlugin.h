#ifndef CSTRUCTPLUGIN_H_INCLUDED
#define CSTRUCTPLUGIN_H_INCLUDED

#include "CTrainPlugin.h"

/*
 *	施設プラグイン
 */
class CStructPlugin: public CModelPlugin{
	friend class CStruct;
protected:
	list<CFreeObjectContainer> m_FreeObject;	//	フリーオブジェクト
public:
	static void RenderPreview(VEC3, VEC3, VEC3);
	CStructPlugin(char *id): CModelPlugin(id){}
	virtual ~CStructPlugin();
	char *DirName(){ return "Struct"; }
	char *TextName(){ return "Struct.txt"; }
	char *TextName2(){ return "Struct2.txt"; }
	bool Load();
	virtual char *LoadStructBefore(char *);
	virtual char *LoadStructAfter(char *str){ return str; }
	virtual bool LoadOldForm();
	virtual void SetPreview();
	void Preview();
	virtual void PreviewStruct(){}
	CNamedObject *FindObject(const string &);
	virtual bool IsSoundEnabled();
	void SetPosture();
	void ScanInput(CStruct *);
	void Render(CStruct *);
	void Simulate(CStruct *);
	CPLUGIN_CASTFUNC(CStructPlugin);
};

/*
 *	施設プラグインリスト
 */
class CStructPluginList: public CModelPluginList{
protected:
public:
	virtual char *DirName(){ return "Struct"; }
	virtual char *TextName(){ return "Struct.txt"; }
	virtual char *TextName2(){ return "Struct2.txt"; }
	char *Default(){ return "Ship"; }
	CPlugin *NewEntry(char *id){ return new CStructPlugin(id); }
	CPLUGINLIST_CASTFUNC(CStructPlugin);
};

//	外部グローバル
extern CStructPlugin *g_Struct;
extern CStructPluginList *g_StructPluginList;

#endif
