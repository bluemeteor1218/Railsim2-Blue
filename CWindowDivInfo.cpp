#include "stdafx.h"
#include "CWindowDivInfo.h"
#include "CConfigMode.h"
#include "CSceneryMode.h"
#include "CSkinPlugin.h"
#include "CScene.h"
#include "CSaveFile.h"

//	内部定数
const int DRAG_BAR_WIDTH = 1;

CWindowInfo::CWindowInfo(){
	m_Scene = NULL;
	m_Div = NULL;
	m_Camera.Init(200.0f, 2.0f, 5000.0f, true);
}

CWindowInfo::~CWindowInfo(){
	Free();
}

void CWindowInfo::Free(){
	m_Scene = NULL;
	CDetectInfo tmp_detect_info;
	m_Camera.SetFocusInfo(tmp_detect_info);
	DELETE_V(m_Div);
}

CWindowInfo *CWindowInfo::GetPointWindow(int x, int y, int w, int h, const POINT& pos){
	if(m_Div){
		return m_Div->GetPointWindow(x, y, w, h, pos);
	}else if(x<=pos.x && pos.x<x+w && y<=pos.y && pos.y<y+h){
		return this;
	}
	return NULL;
}

CWindowInfo *CWindowInfo::GetFirstLeaf(){
	return m_Div ? m_Div->m_ChildWindow[0][0].GetFirstLeaf() : this;
}

void CWindowInfo::ApplyViewportAndCamera(){
	SetViewport(m_PosX, m_PosY, m_Width, m_Height);
	m_Scene->Enter(false);
	m_Camera.Apply(false);
}

void CWindowInfo::RenderScene(int x, int y, int w, int h, CSceneryMode* mode, int opt){
	m_PosX = x; m_PosY = y;
	m_Width = w; m_Height = h;
	if(m_Div){
		m_Div->RenderScene(x, y, w, h, mode, opt);
	}else{
		ApplyViewportAndCamera();
		m_Scene = g_Scene;
		g_SaveFile->RenderScene(opt);
		sv3.pDev->EndScene();
	}
}

void CWindowInfo::OnDeleteScene(CScene *scene){
	if(m_Scene==scene) m_Scene = g_Scene;
	if(m_Div){
		m_Div->OnDeleteScene(scene);
	}
}

/*
 *	読込
 */
char *CWindowInfo::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(eee = str, "WindowInfo"))) throw CSynErr(eee);
	bool is_div;
	if(!(str = AsgnYesNo(eee = str, "Divided", &is_div))) throw CSynErr(eee);
	if(is_div){
		m_Div = new CWindowDivInfo;
		str = m_Div->Read(str);
		m_Scene = GetFirstLeaf()->GetScene();
	}else{
		if(!(str = AsgnPointer(eee = str, "Scene", (void **)&m_Scene))) throw CSynErr(eee);
		m_Scene = (CScene *)ReplaceAdr(m_Scene);
		str = m_Camera.Read(str);
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CWindowInfo::Save(
	FILE *df,		//	ファイル
	string indent	//	インデント
){
	fprintf(df, "%sWindowInfo{\n", indent.c_str());
	fprintf(df, "%s\tDivided = %s;\n", indent.c_str(), YESNO[m_Div!=NULL]);
	string indent2 = indent+"\t";
	if(m_Div){
		m_Div->Save(df, indent2);
	}else{
		fprintf(df, "%s\tScene = %p;\n", indent.c_str(), m_Scene);
		m_Camera.Save(df, indent2);
	}
	fprintf(df, "%s}\n", indent.c_str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CWindowDivInfo **CWindowDivInfo::s_DragDivInfo = NULL;
CWindowDivInfo *CWindowDivInfo::s_MoveDivInfo = NULL;
CCamera *CWindowDivInfo::s_DragDivCamera = NULL;
CScene **CWindowDivInfo::s_DragDivScene = NULL;
int CWindowDivInfo::s_DragState = CWindowDivInfo::DRAG_STATE_NONE;
int CWindowDivInfo::s_ShrinkState = CWindowDivInfo::DRAG_STATE_NONE;
int CWindowDivInfo::s_ShrinkAnim = 0;
int CWindowDivInfo::s_TargetPosX, CWindowDivInfo::s_TargetPosY;
int CWindowDivInfo::s_TargetWidth, CWindowDivInfo::s_TargetHeight;

void CWindowDivInfo::InitState(){
	s_DragDivInfo = NULL;
	s_MoveDivInfo = NULL;
	s_DragDivCamera = NULL;
	s_DragDivScene = NULL;
	s_DragState = CWindowDivInfo::DRAG_STATE_NONE;
	s_ShrinkState = CWindowDivInfo::DRAG_STATE_NONE;
}

CWindowDivInfo::CWindowDivInfo(){
	m_DragState = 0;
	m_HorzRatio = m_VertRatio = 0.f;
}

CWindowDivInfo::~CWindowDivInfo(){
}

void CWindowDivInfo::CalcChildSize(int w, int h){
	m_ChildWidth[0] = m_HorzRatio ? Round(w*m_HorzRatio) : w;
	m_ChildWidth[1] = w-m_ChildWidth[0];
	m_ChildHeight[0] = m_VertRatio ? Round(h*m_VertRatio) : h;
	m_ChildHeight[1] = h-m_ChildHeight[0];
}

int CWindowDivInfo::ScanInputSelf(CWindowDivInfo** info, int x, int y, int w, int h){
	int ret = 1;
	int i;
	if(s_ShrinkState!=DRAG_STATE_NONE){
		if(++s_ShrinkAnim<5){
			if(s_ShrinkState&DRAG_STATE_VERT){
				m_HorzRatio += m_HorzRatio<.5f ? -m_HorzRatio * .5f : (1.f-m_HorzRatio)*.5f;
			}
			if(s_ShrinkState&DRAG_STATE_HORZ){
				m_VertRatio += m_VertRatio<.5f ? -m_VertRatio * .5f : (1.f-m_VertRatio)*.5f;
			}
		}else{
			if(s_ShrinkState&DRAG_STATE_VERT){
				for(i = 0; i<WIN_DIV_MAX; ++i){
					if(m_HorzRatio<.5f){
						m_ChildWindow[i][0].Free();
						m_ChildWindow[i][0] = m_ChildWindow[i][1];
						m_ChildWindow[i][1].m_Div = NULL;
					}else{
						m_ChildWindow[i][1].Free();
					}
				}
				m_HorzRatio = 0.f;
			}
			if(s_ShrinkState&DRAG_STATE_HORZ){
				for(i = 0; i<WIN_DIV_MAX; ++i){
					if(m_VertRatio<.5f){
						m_ChildWindow[0][i].Free();
						m_ChildWindow[0][i] = m_ChildWindow[1][i];
						m_ChildWindow[1][i].m_Div = NULL;
					}else{
						m_ChildWindow[1][i].Free();
					}
				}
				m_VertRatio = 0.f;
			}
			if(!m_HorzRatio && !m_VertRatio){
				CWindowInfo *p_wnd = &m_ChildWindow[0][0];
				if(p_wnd->m_Div){
					*info = p_wnd->m_Div;
					p_wnd->m_Div = NULL;
				}else{
					*info = NULL;
				}
				p_wnd = p_wnd->GetFirstLeaf();
				*s_DragDivCamera = p_wnd->m_Camera;
				*s_DragDivScene = p_wnd->m_Scene;
				delete this;
				g_ConfigMode->SetActiveWindow(NULL);
				ret = 2;
			}
			s_DragDivInfo = NULL;
			s_ShrinkState = DRAG_STATE_NONE;
		}
	}else{
		POINT pos = g_Cursor.GetPos();
		if(s_DragState&DRAG_STATE_HORZ){
			const float min_frac = 1.f/h;
			m_VertRatio = (pos.y-y)*1.f/h;
			ValueArea(&m_VertRatio, min_frac, 1.f-min_frac);
		}
		if(s_DragState&DRAG_STATE_VERT){
			const float min_frac = 1.f/w;
			m_HorzRatio = (pos.x-x)*1.f/w;
			ValueArea(&m_HorzRatio, min_frac, 1.f-min_frac);
		}
		if(GetButton(DIM_LEFT)<S_PUSH){
			CalcChildSize(w, h);
			s_ShrinkState = DRAG_STATE_NONE;
			if(s_DragState&DRAG_STATE_VERT && (m_ChildWidth[0]<WIN_DIV_MIN_SIZE || m_ChildWidth[1]<WIN_DIV_MIN_SIZE)) s_ShrinkState |= DRAG_STATE_VERT;
			if(s_DragState&DRAG_STATE_HORZ && (m_ChildHeight[0]<WIN_DIV_MIN_SIZE || m_ChildHeight[1]<WIN_DIV_MIN_SIZE)) s_ShrinkState |= DRAG_STATE_HORZ;
			if(s_ShrinkState!=DRAG_STATE_NONE){
				s_ShrinkAnim = 0;
			}else{
				s_DragDivInfo = NULL;
			}
		}
	}
	return ret;
}

int CWindowDivInfo::ScanInput(CWindowDivInfo** info, int x, int y, int w, int h, CCamera *cam, CScene **scene){
	s_MoveDivInfo = NULL;
	if(s_DragDivInfo && *s_DragDivInfo){
		return (*s_DragDivInfo)->ScanInputSelf(s_DragDivInfo, s_TargetPosX, s_TargetPosY, s_TargetWidth, s_TargetHeight);
	}else{
		return ScanInputRecursive(info, x, y, w, h, cam, scene);
	}
}

int CWindowDivInfo::ScanInputRecursive(CWindowDivInfo** info, int x, int y, int w, int h, CCamera *cam, CScene **scene){
	int i, j;
	POINT pos = g_Cursor.GetPos();
	if(x<=pos.x && pos.x<x+w && y<=pos.y && pos.y<y+h){
		s_DragState = DRAG_STATE_NONE;
		if(*info){
			(*info)->CalcChildSize(w, h);
			const int tx = x+(*info)->m_ChildWidth[0];
			const int ty = y+(*info)->m_ChildHeight[0];
			bool copy_h = false, swap_h = false;
			bool copy_v = false, swap_v = false;
			if((*info)->m_ChildWidth[1] && tx-WIN_DIV_MOVE_MARGIN<=pos.x && pos.x<=tx+WIN_DIV_MOVE_MARGIN){
				if((*info)->m_ChildHeight[1] && ty-WIN_DIV_MOVE_MARGIN<=pos.y && pos.y<=ty+WIN_DIV_MOVE_MARGIN){
					s_DragState = DRAG_STATE_HORZVERT;
				}else{
					if(!(*info)->m_ChildHeight[1] && (pos.y<y+WIN_DIV_MARGIN || y+h-WIN_DIV_MARGIN<=pos.y)){
						s_DragState = DRAG_STATE_HORZVERT;
						copy_v = true;
						swap_v = pos.y<y+WIN_DIV_MARGIN;
					}else{
						s_DragState = DRAG_STATE_VERT;
					}
				}
			}else{
				if((*info)->m_ChildHeight[1] && ty-WIN_DIV_MOVE_MARGIN<=pos.y && pos.y<=ty+WIN_DIV_MOVE_MARGIN){
					if(!(*info)->m_ChildWidth[1] && (pos.x<x+WIN_DIV_MARGIN || x+w-WIN_DIV_MARGIN<=pos.x)){
						s_DragState = DRAG_STATE_HORZVERT;
						copy_h = true;
						swap_h = pos.x<x+WIN_DIV_MARGIN;
					}else{
						s_DragState = DRAG_STATE_HORZ;
					}
				}else{
					for(i = 0; i<WIN_DIV_MAX; ++i){
						if((*info)->m_ChildHeight[i]<=0) continue;
						const int cy = i ? ty : y;
						const int ch = (*info)->m_ChildHeight[i];
						for(j = 0; j<WIN_DIV_MAX; ++j){
							if((*info)->m_ChildWidth[j]<=0) continue;
							const int cx = j ? tx : x;
							const int cw = (*info)->m_ChildWidth[j];
							CWindowInfo &wnd = (*info)->m_ChildWindow[i][j];
							int ret = ScanInputRecursive(&wnd.m_Div, cx, cy, cw, ch, &wnd.m_Camera, &wnd.m_Scene);
							if(ret) return ret;
						}
					}
					return 0;
				}
			}
			if(s_DragState!=DRAG_STATE_NONE){
				s_TargetPosX = x;
				s_TargetPosY = y;
				s_TargetWidth = w;
				s_TargetHeight = h;
				if(GetButton(DIM_LEFT)==S_PUSH){
					g_Skin->MouseDown();
					s_DragDivInfo = info;
					s_DragDivCamera = cam;
					s_DragDivScene = scene;
					if(copy_h){
						for(i = 0; i<WIN_DIV_MAX; ++i){
							(*info)->m_ChildWindow[i][1] = *(*info)->m_ChildWindow[i][0].GetFirstLeaf();
							if(swap_h){
								CWindowInfo t = (*info)->m_ChildWindow[i][1];
								(*info)->m_ChildWindow[i][1] = (*info)->m_ChildWindow[i][0];
								(*info)->m_ChildWindow[i][0] = t;
							}
						}
						if(swap_h) (*info)->m_HorzRatio = 1.f/w;
					}
					if(copy_v){
						for(i = 0; i<WIN_DIV_MAX; ++i){
							(*info)->m_ChildWindow[1][i] = *(*info)->m_ChildWindow[0][i].GetFirstLeaf();
							if(swap_v){
								CWindowInfo t = (*info)->m_ChildWindow[1][i];
								(*info)->m_ChildWindow[1][i] = (*info)->m_ChildWindow[0][i];
								(*info)->m_ChildWindow[0][i] = t;
							}
						}
						if(swap_v) (*info)->m_VertRatio = 1.f/h;
					}
					return 1;
				}else{
					s_MoveDivInfo = *info;
				}
			}
		}else{
			if(pos.x<x+WIN_DIV_MARGIN || x+w-WIN_DIV_MARGIN<=pos.x){
				if(pos.y<y+WIN_DIV_MARGIN || y+h-WIN_DIV_MARGIN<=pos.y){
					s_DragState = DRAG_STATE_HORZVERT;
				}else{
					s_DragState = DRAG_STATE_VERT;
				}
			}else{
				if(pos.y<y+WIN_DIV_MARGIN || y+h-WIN_DIV_MARGIN<=pos.y){
					s_DragState = DRAG_STATE_HORZ;
				}else{
				}
			}
			if(s_DragState!=DRAG_STATE_NONE){
				s_TargetPosX = x;
				s_TargetPosY = y;
				s_TargetWidth = w;
				s_TargetHeight = h;
				if(GetButton(DIM_LEFT)==S_PUSH){
					g_Skin->MouseDown();
					*info = new CWindowDivInfo;
					(*info)->InitCamera(*cam, *scene);
					s_DragDivInfo = info;
					s_DragDivCamera = cam;
					s_DragDivScene= scene;
					return 1;
				}
			}
		}
	}else{
	}
	return 0;
}

void CWindowDivInfo::RenderInterface(){
	if(s_DragState==DRAG_STATE_NONE) return;
	POINT pos = g_Cursor.GetPos();
	const D3DCOLOR rect_color = 0xffffff00;
	const D3DCOLOR div_color = 0xffff0000;
	const int tx1 = s_TargetPosX+WIN_DIV_MARGIN, tx2 = s_TargetPosX+s_TargetWidth-WIN_DIV_MARGIN;
	const int ty1 = s_TargetPosY+WIN_DIV_MARGIN, ty2 = s_TargetPosY+s_TargetHeight-WIN_DIV_MARGIN;
	Fill2DRect(s_TargetPosX, s_TargetPosY, s_TargetPosX+s_TargetWidth, ty1, rect_color);
	Fill2DRect(s_TargetPosX, ty2, s_TargetPosX+s_TargetWidth, s_TargetPosY+s_TargetHeight, rect_color);
	Fill2DRect(s_TargetPosX, ty1, tx1, ty2, rect_color);
	Fill2DRect(tx2, ty1, s_TargetPosX+s_TargetWidth, ty2, rect_color);
	if(s_MoveDivInfo){
	}else{
		if(s_DragDivInfo){
			ValueArea((int*)&pos.x, s_TargetPosX, s_TargetPosX+s_TargetWidth-1);
			ValueArea((int*)&pos.y, s_TargetPosY, s_TargetPosY+s_TargetHeight-1);
		}else{
			pos.x = pos.x<s_TargetPosX+s_TargetWidth/2 ? s_TargetPosX : s_TargetPosX+s_TargetWidth-1;
			pos.y = pos.y<s_TargetPosY+s_TargetHeight/2 ? s_TargetPosY : s_TargetPosY+s_TargetHeight-1;
			if(s_DragState&DRAG_STATE_HORZ){
				Fill2DRect(s_TargetPosX, pos.y-DRAG_BAR_WIDTH, s_TargetPosX+s_TargetWidth, pos.y+DRAG_BAR_WIDTH+1, div_color);
			}
			if(s_DragState&DRAG_STATE_VERT){
				Fill2DRect(pos.x-DRAG_BAR_WIDTH, s_TargetPosY, pos.x+DRAG_BAR_WIDTH+1, s_TargetPosY+s_TargetHeight, 0xffff0000);
			}
		}
	}
}

void CWindowDivInfo::InitCamera(const CCamera& cam, CScene *scene){
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		for(j = 0; j<WIN_DIV_MAX; ++j){
			CWindowInfo &wnd = m_ChildWindow[i][j];
			wnd.SetCamera(cam);
			wnd.SetScene(scene);
		}
	}
}

void CWindowDivInfo::RenderInterfaceRecursive(int x, int y, int w, int h){
	const bool dragging = s_DragDivInfo && *s_DragDivInfo==this;
	const bool div_horz = (s_DragState&DRAG_STATE_HORZ)!=0;
	const bool div_vert = (s_DragState&DRAG_STATE_VERT)!=0;
	D3DCOLOR div_color_horz = (dragging || s_MoveDivInfo==this) && div_horz ? 0xffff0000 : 0xff000000;
	D3DCOLOR div_color_vert = (dragging || s_MoveDivInfo==this) && div_vert ? 0xffff0000 : 0xff000000;
	CalcChildSize(w, h);
	int i, j;
	for(i = 0; i<2; ++i){
		if(i==!div_vert && m_ChildHeight[0]>0 && m_ChildHeight[1]>0){
			Fill2DRect(x, y+m_ChildHeight[0]-WIN_DIV_PADDING/2, x+w, y+m_ChildHeight[0]-WIN_DIV_PADDING/2+WIN_DIV_PADDING, div_color_horz);
		}
		if(i==!!div_vert && m_ChildWidth[0]>0 && m_ChildWidth[1]>0){
			Fill2DRect(x+m_ChildWidth[0]-WIN_DIV_PADDING/2, y, x+m_ChildWidth[0]-WIN_DIV_PADDING/2+WIN_DIV_PADDING, y+h, div_color_vert);
		}
	}
	for(i = 0; i<WIN_DIV_MAX; ++i){
		if(m_ChildHeight[i]<=0) continue;
		const int cy = i ? y+m_ChildHeight[i-1] : y;
		const int ch = m_ChildHeight[i];
		for(j = 0; j<WIN_DIV_MAX; ++j){
			if(m_ChildWidth[j]<=0) continue;
			const int cx = j ? x+m_ChildWidth[j-1] : x;
			const int cw = m_ChildWidth[j];
			if(m_ChildWindow[i][j].m_Div){
				m_ChildWindow[i][j].m_Div->RenderInterfaceRecursive(cx, cy, cw, ch);
			}
		}
	}
}

void CWindowDivInfo::RenderScene(int x, int y, int w, int h, CSceneryMode* mode, int opt){
	CalcChildSize(w, h);
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		if(m_ChildHeight[i]<=0) continue;
		const int cy = i ? y+m_ChildHeight[i-1] : y;
		const int ch = m_ChildHeight[i];
		for(j = 0; j<WIN_DIV_MAX; ++j){
			if(m_ChildWidth[j]<=0) continue;
			const int cx = j ? x+m_ChildWidth[j-1] : x;
			const int cw = m_ChildWidth[j];
			m_ChildWindow[i][j].RenderScene(cx, cy, cw, ch, mode, opt);
		}
	}
}

CWindowInfo *CWindowDivInfo::GetPointWindow(int x, int y, int w, int h, const POINT& pos){
	CalcChildSize(w, h);
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		if(m_ChildHeight[i]<=0) continue;
		const int cy = i ? y+m_ChildHeight[i-1] : y;
		const int ch = m_ChildHeight[i];
		for(j = 0; j<WIN_DIV_MAX; ++j){
			if(m_ChildWidth[j]<=0) continue;
			const int cx = j ? x+m_ChildWidth[j-1] : x;
			const int cw = m_ChildWidth[j];
			CWindowInfo *ret = m_ChildWindow[i][j].GetPointWindow(cx, cy, cw, ch, pos);
			if(ret) return ret;
		}
	}
	return NULL;
}

void CWindowDivInfo::OnDeleteScene(CScene *scene){
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		for(j = 0; j<WIN_DIV_MAX; ++j){
			m_ChildWindow[i][j].OnDeleteScene(scene);
		}
	}
}

/*
 *	読込
 */
char *CWindowDivInfo::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(eee = str, "WindowDivInfo"))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "HorzRatio", &m_HorzRatio))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "VertRatio", &m_VertRatio))) throw CSynErr(eee);
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		if(i && !m_VertRatio) break;
		for(j = 0; j<WIN_DIV_MAX; ++j){
			if(j && !m_HorzRatio) break;
			str = m_ChildWindow[i][j].Read(str);
		}
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CWindowDivInfo::Save(
	FILE *df,		//	ファイル
	string indent	//	インデント
){
	fprintf(df, "%sWindowDivInfo{\n", indent.c_str());
	fprintf(df, "%s\tHorzRatio = %f;\n", indent.c_str(), m_HorzRatio);
	fprintf(df, "%s\tVertRatio = %f;\n", indent.c_str(), m_VertRatio);
	string indent2 = indent+"\t";
	int i, j;
	for(i = 0; i<WIN_DIV_MAX; ++i){
		if(i && !m_VertRatio) break;
		for(j = 0; j<WIN_DIV_MAX; ++j){
			if(j && !m_HorzRatio) break;
			m_ChildWindow[i][j].Save(df, indent2);
		}
	}
	fprintf(df, "%s}\n", indent.c_str());
}
