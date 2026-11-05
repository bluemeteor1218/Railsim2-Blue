#ifndef CPIERPLUGIN_H_INCLUDED
#define CPIERPLUGIN_H_INCLUDED

#include "CProfilePlugin.h"

/*
 *	橋脚プラグイン
 */
class CPierPlugin: public CProfilePlugin{
	friend class CPier;
	friend class CLineBuildCurve;
private:
	int m_TrackNum;			//	軌道数
	float m_TrackInterval;	//	軌道間隔
	float m_Interval;			//	間隔
	float m_Offset;				//	オフセット
	float m_BuildMinAlt;		//	建設最小高度
	float m_TaperX, m_TaperY;	//	XY テーパ
	float m_TaperZ;				//	Z テーパ (等間隔配置用)
	float m_PierPos;			//	架線柱間積算距離
	bool m_Direction;			//	延長方向 (false: down, true: up)
	VEC3 m_JointToHeadLocal;	//	ジョイント部・ヘッド配置ローカル座標
	VEC3 m_HeadToPierLocal;		//	ヘッド部・橋脚配置ローカル座標
	VEC3 m_BaseToPierLocal;		//	基礎部・橋脚配置ローカル座標
	CMesh *m_JointMesh;			//	ジョイント部メッシュ
	CMesh *m_HeadMesh;			//	ヘッド部メッシュ
	CMesh *m_BaseMesh;			//	基礎部メッシュ
	string m_JointFile;			//	ジョイント部ファイル名
	string m_HeadFile;			//	ヘッド部ファイル名
	string m_BaseFile;			//	基礎部ファイル名
	float m_JointScale;			//	ジョイント部スケール
	float m_HeadScale;			//	ヘッド部スケール
	float m_BaseScale;			//	基礎部スケール
	CObject m_JointObject;		//	ジョイント部オブジェクト
	CObject m_HeadObject;		//	ヘッド部オブジェクト
	CObject m_BaseObject;		//	基礎部オブジェクト
public:
	static void RenderPreview();
	CPierPlugin(char *id): CProfilePlugin(id){}
	char *DirName(){ return "Pier"; }
	char *TextName2(){ return "Pier2.txt"; }
	bool IsMultiTrack(){ return m_TrackNum>1; }
	int ConfirmMultiTrack(int tn, float ti){
		return tn!=m_TrackNum ? 1 : (tn>1 && ti!=m_TrackInterval ? 2 : 0);
	}
	bool UseTaper(){ return true; }
	float GetTaperZ(){ return m_Direction ? -m_TaperZ : m_TaperZ; }
	bool Load();
	void SetPreview();
	void Preview(VEC3, float);
	float GetPierPos(){ return m_PierPos; }
	void AddPierPos(float);
	void ResetPierPos(){ m_PierPos = m_Offset; }
	void SetPierPos(float pp){ m_PierPos = pp; }
	CPLUGIN_CASTFUNC(CPierPlugin);
};

/*
 *	橋脚プラグインリスト
 */
class CPierPluginList: public CProfilePluginList{
private:
public:
	char *DirName(){ return "Pier"; }
	char *TextName2(){ return "Pier2.txt"; }
	char *Default(){ return "Default_SinglePC"; }
	CPlugin *NewEntry(char *id){ return new CPierPlugin(id); }
	CPLUGINLIST_CASTFUNC(CPierPlugin);
};

//	外部グローバル
extern CPierPlugin *g_Pier;
extern CPierPluginList *g_PierPluginList;

#endif
