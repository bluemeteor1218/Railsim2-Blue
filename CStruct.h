#ifndef CSTRUCT_H_INCLUDED
#define CSTRUCT_H_INCLUDED

class CListElement;
class CStructPlugin;
class CScene;

#include "CModelInst.h"

/*
 *	施設インスタンス
 */
class CStruct: public CModelInst{
	friend class CStructGroup;
protected:
	static CStruct **ms_Root;		//	接続ルート
	CScene *m_Scene;				//	シーン
	CStructPlugin *m_StructPlugin;	//	施設プラグイン
	CStruct *m_Next;				//	次
public:
	static void SetRoot(CStruct **r){ ms_Root = r; }
	CStruct();
	CStruct(CStructPlugin *);
	CStruct(CStructPlugin *, VEC3, VEC3, VEC3, bool);
	virtual ~CStruct();
	virtual bool IsSelectVisible();
	virtual CPartsInst *FindParts(CNamedObject *);
	void Remove();
	void Delete();
	void PrintInfo();
	virtual CModelInst *Control();
	void ScanInput(int, VEC3 &, VEC3 &);
	void Render();
	void SimulateModelInst();
	virtual void SimulateStruct(){}
	virtual void SetSwitchStruct(){}
	virtual CScene *GetScene(){ return m_Scene; }
	CMODELINST_CASTFUNC(CStruct);
	char *Read(char *);
	void Save(FILE *);
};

#endif
