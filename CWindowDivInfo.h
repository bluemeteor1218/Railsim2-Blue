#ifndef CWINDOWDIVINFO_H_INCLUDED
#define CWINDOWDIVINFO_H_INCLUDED

#include "CCamera.h"

class CScene;
class CWindowDivInfo;
class CSceneryMode;

const int WIN_DIV_MAX = 2;			//	画面分割単位
const int WIN_DIV_MARGIN = 5;		//	画面分割余白
const int WIN_DIV_PADDING = 2;		//	画面分割線幅
const int WIN_DIV_MOVE_MARGIN = 1;	//	画面分割移動判定幅
const int WIN_DIV_MIN_SIZE = 50;	//	画面分割最小サイズ

class CWindowInfo {
	friend class CWindowDivInfo;

private:
	CScene *m_Scene;
	CCamera m_Camera;
	CWindowDivInfo *m_Div;
	int m_PosX, m_PosY;
	int m_Width, m_Height;

public:
	CWindowInfo();
	~CWindowInfo();

	CWindowDivInfo *GetDiv(){ return m_Div; }
	CWindowDivInfo **GetDivAdr(){ return &m_Div; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	CScene *GetScene(){ return m_Scene; }
	CCamera *GetCamera(){ return &m_Camera; }
	void SetCamera(const CCamera& cam){ m_Camera = cam; }
	void SetScene(CScene *scene){ m_Scene = scene; }
	void Free();
	CWindowInfo *GetPointWindow(int x, int y, int w, int h, const POINT& pos);
	CWindowInfo *GetFirstLeaf();
	void ApplyViewportAndCamera();
	void RenderScene(int x, int y, int w, int h, CSceneryMode* mode, int opt);
	void OnDeleteScene(CScene *scene);
	char *Read(char *);
	void Save(FILE *, string indent);
};

class CWindowDivInfo {
	friend class CWindowInfo;

private:
	enum {
		DRAG_STATE_NONE = 0,
		DRAG_STATE_HORZ = 1,
		DRAG_STATE_VERT = 2,
		DRAG_STATE_HORZVERT = DRAG_STATE_HORZ|DRAG_STATE_VERT,
	};

public:
	static CWindowDivInfo **s_DragDivInfo;
	static CWindowDivInfo *s_MoveDivInfo;
	static CCamera *s_DragDivCamera;
	static CScene **s_DragDivScene;
	static int s_DragState;
	static int s_ShrinkState;
	static int s_ShrinkAnim;
	static int s_TargetPosX, s_TargetPosY;
	static int s_TargetWidth, s_TargetHeight;

	float m_HorzRatio;
	float m_VertRatio;
	CWindowInfo m_ChildWindow[WIN_DIV_MAX][WIN_DIV_MAX];
	int m_ChildWidth[WIN_DIV_MAX];
	int m_ChildHeight[WIN_DIV_MAX];
	int m_DragState;

public:
	static void InitState();
	void CalcChildSize(int w, int h);
	int ScanInputSelf(CWindowDivInfo** info, int x, int y, int w, int h);
	static int ScanInput(CWindowDivInfo** info, int x, int y, int w, int h, CCamera *cam, CScene **scene);
	static int ScanInputRecursive(CWindowDivInfo** info, int x, int y, int w, int h, CCamera *cam, CScene **scene);
	static void RenderInterface();
	void InitCamera(const CCamera& cam, CScene *scene);
	void RenderInterfaceRecursive(int x, int y, int w, int h);
	void RenderScene(int x, int y, int w, int h, CSceneryMode* mode, int opt);
	CWindowInfo* GetPointWindow(int x, int y, int w, int h, const POINT& pos);
	void OnDeleteScene(CScene *scene);

	CWindowDivInfo();
	~CWindowDivInfo();
	char *Read(char *);
	void Save(FILE *, string indent);
};

#endif
