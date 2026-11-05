#ifndef CLINEPLUGIN_H_INCLUDED
#define CLINEPLUGIN_H_INCLUDED

#include "CProfilePlugin.h"

/*
 *	架線プラグイン
 */
class CLinePlugin: public CProfilePlugin{
	friend class CLine;
	friend class CRailPlugin;
	friend class CLineBuildCurve;
private:
	float m_TrolleyAlt;		//	軌道面－トロリ線間高さ
	float m_Height;			//	トロリ線－支持点間高さ
	float m_MaxInterval;	//	架線柱間隔
	float m_Offset;			//	オフセット
	float m_MaxDeflection;	//	パンタグラフ中心からの最大偏位
	float m_PolePos;		//	架線柱間積算距離
public:
	static void RenderPreview();
	CLinePlugin(char *id): CProfilePlugin(id){}
	char *DirName(){ return "Line"; }
	char *TextName2(){ return "Line2.txt"; }
	float GetTrolleyAlt(){ return m_TrolleyAlt; }
	float GetHeight(){ return m_Height; }
	bool Load();
	void SetPreview();
	float GetPolePos(){ return m_PolePos; }
	void AddPolePos(float);
	void ResetPolePos(){ m_PolePos = m_Offset; }
	void SetPolePos(float pp){ m_PolePos = pp; }
	CPLUGIN_CASTFUNC(CLinePlugin);
};

/*
 *	枕木プラグインリスト
 */
class CLinePluginList: public CProfilePluginList{
private:
public:
	char *DirName(){ return "Line"; }
	char *TextName2(){ return "Line2.txt"; }
	char *Default(){ return "Default_SimpleCatenary"; }
	CPlugin *NewEntry(char *id){ return new CLinePlugin(id); }
	CPLUGINLIST_CASTFUNC(CLinePlugin);
};

//	外部グローバル
extern CLinePlugin *g_Line;
extern CLinePluginList *g_LinePluginList;

#endif
