#ifndef CPLUGIN_H_INCLUDED
#define CPLUGIN_H_INCLUDED

class CSynErr;
class CTreeDirElement;
class CTreeFileElement;
class CPluginTree;

/*
 *	プラグイン基本クラス
 */
class CPlugin{
	friend class CTreeFileElement;
	friend class CPluginList;
protected:
	static bool ms_PreviewState;	//	プレビュー状態
	char *m_Buffer;			//	定義スクリプト (最初から)
	char *m_Script;			//	定義スクリプト (ヘッダ以降)
	int m_State;			//	状態 (0: init, 1: preloaded, 2: loaded)
	bool m_InsertTreeFlag;	//	ツリー挿入フラグ
	float m_Version;		//	対応バージョン
	LPTEX8 m_IconTex;		//	アイコンテクスチャ
	float m_IconRect[4];	//	アイコン位置
	string m_ID;			//	ID
	string m_Name;			//	名称
	string m_Author;		//	作者
	string m_IconFileName;	//	作者
	string m_Description;	//	説明
	CPlugin *m_Next;		//	次ポインタ
public:
	static bool IsPreview(){ return ms_PreviewState; }
	static void ResetPreview(){ ms_PreviewState = false; }
	CPlugin(char *);
	virtual ~CPlugin();
	virtual char *DirName() = 0;
	virtual char *TextName(){ return NULL; }
	virtual char *TextName2() = 0;
	virtual char *SaveString(){ return GetID(); }
	char *GetID(){ return (char *)m_ID.c_str(); }
	char *GetName(){ return (char *)m_Name.c_str(); }
	char *GetAuthor(){ return (char *)m_Author.c_str(); }
	bool IsInserted(){ return m_InsertTreeFlag; }
	int Compare(CPlugin *);
	bool ChDir();
	void HandleError(CSynErr *);
	char *LoadHeader(char *);
	bool PreLoad(FILE *);
	bool PreLoadOldForm(FILE *);
	virtual bool Load() = 0;
	virtual bool LoadOldForm(){ return false; }
	CPlugin *LoadAndGet();
	string GetBasicInfo();
	string GetDescription(){ return m_Description; }
	CTreeFileElement *InsertItem(CTreeDirElement *, CPluginTree *);
	virtual void SetTreeElement(CTreeFileElement *){}
	virtual bool IsRenamable(){ return false; }
	virtual bool ConfirmRename(string &){ return false; }
	virtual bool IsDeletable(){ return false; }
	void SetIconTexture();
	LPTEX8 GetIconTexture(){ return m_IconTex; }
	float *GetIconRect(){ return m_IconRect; }
	virtual void SetPreview(){}
};

#define CPLUGIN_CASTFUNC(type) \
	type *Next(){ return (type *)m_Next; }

/*
 *	プラグインリスト
 */
class CPluginList{
protected:
	int m_PluginNum;	//	プラグイン数
	CPlugin *m_List;	//	リスト先頭
public:
	CPluginList();
	virtual ~CPluginList();
	virtual char *DirName() = 0;
	virtual char *TextName(){ return NULL; }
	virtual char *TextName2() = 0;
	virtual char *Default(){ return NULL; }
	virtual CPlugin *NewEntry(char *) = 0;
	bool List();
	bool LoadOne(char *, char *, bool);
	CPlugin *FindPlugin(const char *, bool load = true);
	CPlugin *FindAvailable();
	void BuildTree(CPluginTree *);
};

//	キャスト関数
#define CPLUGINLIST_CASTFUNC(type) \
	type *Root(){ return (type *)m_List; } \
	type *FindPlugin(const char *id, bool load = true){ \
		CPlugin *pi = CPluginList::FindPlugin(id, load); \
		return pi ? (type *)pi : NULL; \
	} \
	type *FindAvailable(){ \
		CPlugin *pi = CPluginList::FindAvailable(); \
		return pi ? (type *)pi : NULL; \
	}

/*
 *	NULL でなければプラグイン ID、NULL なら空文字列を返す
 */
inline char *CheckPluginID(CPlugin *pi){ return (char *)(pi ? pi->GetID() : ""); }

//	関数宣言
char *LoadBinaryText(FILE *, int maxbyte = -1);
char *LoadBinaryText(char *, int maxbyte = -1);

#endif
