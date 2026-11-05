#ifndef CSURFACEPLUGIN_H_INCLUDED
#define CSURFACEPLUGIN_H_INCLUDED

#include "CStructPlugin.h"

/*
 *	地形プラグイン
 */
class CSurfacePlugin: public CStructPlugin{
private:
	static CObject ms_PreviewObject;	//	プレビュー用オブジェクト
	float m_SizeX, m_SizeZ;				//	矩形サイズ
public:
	static void RenderPreview();
	CSurfacePlugin(char *id): CStructPlugin(id){}
	~CSurfacePlugin();
	char *DirName(){ return "Surface"; }
	char *TextName(){ return "Surface.txt"; }
	char *TextName2(){ return "Surface2.txt"; }
	char *LoadStructBefore(char *);
	bool LoadOldForm();
	void SetPreview();
	float GetSizeX(){ return m_SizeX; }
	float GetSizeZ(){ return m_SizeZ; }
	bool IsSoundEnabled();
	bool PickSurface(VEC3, VEC3, VEC3 *, VEC3 *tri = NULL, int inv = 1);
	bool ClipRect(VEC3 *);
	CPLUGIN_CASTFUNC(CSurfacePlugin);
};

/*
 *	地形プラグインリスト
 */
class CSurfacePluginList: public CStructPluginList{
private:
public:
	char *DirName(){ return "Surface"; }
	char *TextName(){ return "Surface.txt"; }
	char *TextName2(){ return "Surface2.txt"; }
	char *Default(){ return "Default"; }
	CPlugin *NewEntry(char *id){ return new CSurfacePlugin(id); }
	CPLUGINLIST_CASTFUNC(CSurfacePlugin);
};

//	外部グローバル
extern CSurfacePlugin *g_Surface;
extern CSurfacePlugin *g_DefaultSurface;
extern CSurfacePluginList *g_SurfacePluginList;

#endif
