#include "stdafx.h"
#include "CScene.h"
#include "CPier.h"
#include "CPierPlugin.h"
#include "CSurfacePlugin.h"

//	外部定数
extern const float RAIL_SEG_MAX;

//	内部定数
extern const float TAPER_DIV_RATIO = 0.2f;	//	テーパ分割係数

//	外部グローバル
extern bool g_ShowPierSelect;
extern MAT8 g_MatSelect[];
extern MAT8 g_MatSelectA[];

//	static メンバ
CPier **CPier::ms_Root = NULL;
float CPier::ms_MinDist;
CPier *CPier::ms_Detect;

/*
 *	コンストラクタ
 */
CPier::CPier(){
	m_Selected = 0;
	m_Valid = true;
	m_Next = NULL;
	m_PierPlugin = NULL;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CPier::CPier(
	VEC3 jpos,			//	ジョイント部座標
	VEC3 jdir,			//	ジョイント部 right
	VEC3 jup,			//	ジョイント部 up
	float pickalt,		//	地形 pick 開始高度
	CPierPlugin *ipi	//	レールプラグイン
){
	m_Selected = 0;
	m_Valid = false;
	m_Next = NULL;
	m_PierPlugin = ipi;
	SetMesh();
	m_JointObject.SetPos(jpos);
	m_JointObject.SetDir(jdir, jup);
	if(!Init(&pickalt)) return;
	m_Valid = true;
	m_Next = *ms_Root;
	*ms_Root = this;
}

/*
 *	デストラクタ
 */
CPier::~CPier(){
	DELETE_V(m_Next);
}

/*
 *	メッシュ設定
 */
void CPier::SetMesh(){
	if(m_PierPlugin){
		m_JointObject.SetMesh(m_PierPlugin->m_JointMesh, V3ZERO, m_PierPlugin->m_JointScale);
		m_HeadObject.SetMesh(m_PierPlugin->m_HeadMesh, V3ZERO, m_PierPlugin->m_HeadScale);
		m_BaseObject.SetMesh(m_PierPlugin->m_BaseMesh, V3ZERO, m_PierPlugin->m_BaseScale);
	}
}

/*
 *	パーツ初期化
 */
bool CPier::Init(
	float *pickalt	//	地形検出
){
	VEC3 jpos = m_JointObject.GetPos();
	VEC3 jright, jup = m_JointObject.GetUp(), jdir = m_JointObject.GetDir();
	V3NormAxis(&jright, &jup, &jdir);
	VEC3 hpos = jpos+V3LocalToWorld(
		&m_PierPlugin->m_JointToHeadLocal, &jright, &jup, &jdir);
	VEC3 hright, hup = V3UP, hdir = VEC3(jdir.x, 0.0f, jdir.z);
	V3NormAxis(&hright, &hup, &hdir);
	m_HeadObject.SetPos(hpos);
	m_HeadObject.SetDir(hdir, hup);
	VEC3 top = hpos+V3LocalToWorld(
		&m_PierPlugin->m_HeadToPierLocal, &hright, &hup, &hdir);
	if(pickalt){
		if(!g_Scene->PickScene(
			VEC3(top.x, *pickalt, top.z), -V3UP, &m_SurfaceHit)) return false;
		if(jpos.y-m_SurfaceHit.y<m_PierPlugin->m_BuildMinAlt) return false;
	}else{
		m_SurfaceHit = VEC3(top.x, m_SurfaceAlt, top.z);
	}
	m_SurfaceAlt = m_SurfaceHit.y;
	VEC3 bottom = m_SurfaceHit+m_PierPlugin->m_BaseToPierLocal.y*V3UP;
	m_PierArea = top.y-bottom.y;
	m_PierUp = hdir;
	if(m_PierPlugin->m_Direction){
		m_PierBegin = bottom;
		m_PierEnd = top;
		m_PierRight = -hright;
		m_PierDir = V3UP;
	}else{
		m_PierBegin = top;
		m_PierEnd = bottom;
		m_PierRight = hright;
		m_PierDir = -V3UP;
	}
	m_BaseObject.SetPos(m_SurfaceHit+hright*m_PierPlugin->m_BaseToPierLocal.x
		+hdir*m_PierPlugin->m_BaseToPierLocal.z);
	m_BaseObject.SetDir(hdir, hup);
	return true;
}

/*
 *	リストから削除
 */
void CPier::Delete(){
	g_Scene->DeletePierLink(this);
	m_Next = NULL;
	delete this;
}

/*
 *	有効確認
 *
 *	!m_Valid ならばリストは連結されていないので、メモリリーク等は発生しない
 */
bool CPier::Confirm(){
	if(m_Valid) return true;
	delete this;
	return false;
}

/*
 *	入力チェック
 */
void CPier::ScanInput(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	VEC3 center = 0.5f*(m_PierBegin+m_PierEnd);
	VEC3 sc = WorldToScreen(center);
	if(sc.z<0.0f) return;
	switch(mode){
	case 2:
		if(rect1.x<=sc.x && sc.x<=rect2.x && rect1.y<=sc.y && sc.y<=rect2.y)
			m_Selected |= 1;
		else m_Selected &= 2;
		break;
	case 3:
		if(CheckCtrl()){
			m_Selected &= ~m_Selected<<1;
		}else{
			if(CheckShift()) m_Selected |= m_Selected<<1;
			else m_Selected = m_Selected<<1;
		}
		m_Selected &= 2;
		break;
	case 4:
		float dist = V3Len(&(rect1-sc));
		float th = 250.0f*V3Len(&(m_PierBegin-m_PierEnd))
			/(V3Len(&(GetVPos()-center))*g_FovRatio);
		if(th<DETECT_2D_MIN) th = DETECT_2D_MIN;
		if(dist<th && (ms_MinDist<0.0f || ms_MinDist>dist)){
			ms_MinDist = dist;
			ms_Detect = this;
		}
		m_Selected &= 2;
		break;
	}
}

/*
 *	断面をダンプ
 */
void CPier::Dump(
	int prev	//	プレビューフラグ
){
	if(!m_PierPlugin || m_PierArea<=0.0f) return;
	float tsx = 1.0f+m_PierArea*m_PierPlugin->m_TaperX;
	float tsy = 1.0f+m_PierArea*m_PierPlugin->m_TaperY;
	m_PierPlugin->ResetMapTemp();
	int i, div = 1+Round(((tsx>tsy ? tsx : tsy)-1.0f)/TAPER_DIV_RATIO);
	int smax = Round(2.0f*m_PierArea/RAIL_SEG_MAX);
	if(div<smax) div = smax;
	VEC3 tr1, tu1, tr2, tu2;
	if(m_PierPlugin->m_Direction){
		tr1 = m_PierRight*tsx; tu1 = m_PierUp*tsy;
		tr2 = m_PierRight; tu2 = m_PierUp;
	}else{
		tr1 = m_PierRight; tu1 = m_PierUp;
		tr2 = m_PierRight*tsx; tu2 = m_PierUp*tsy;
	}
	float divarea = m_PierArea/div;
	for(i = 0; i<div; i++){
		float q12 = (float)i/div, q11 = 1.0f-q12;
		float q22 = (float)(i+1)/div, q21 = 1.0f-q22;
		VEC3 p1 = q11*m_PierBegin+q12*m_PierEnd, r1 = q11*tr1+q12*tr2, u1 = q11*tu1+q12*tu2;
		VEC3 p2 = q21*m_PierBegin+q22*m_PierEnd, r2 = q21*tr1+q22*tr2, u2 = q21*tu1+q22*tu2;
		m_PierPlugin->Dump(p1, r1, u1, p1, r1, u1, p2, r2, u2, p2, r2, u2, divarea, prev);
	}
}

/*
 *	等間隔オブジェクトをレンダリング
 */
void CPier::Render(){
	if(!m_PierPlugin) return;
	MAT8 *altmat = m_Selected ? &g_MatSelect[m_Selected] : NULL;
	if(m_JointObject.IsMeshValid()){
		if(g_ShowPierSelect && m_Selected) m_JointObject.RenderSC(altmat);
		else m_JointObject.Render();
		CastShadow(&m_JointObject);
	}
	if(m_PierPlugin->m_HeadObject.IsMeshValid()){
		if(g_ShowPierSelect && m_Selected) m_HeadObject.RenderSC(altmat);
		else m_HeadObject.Render();
		CastShadow(&m_HeadObject);
	}
	float tsx, tsy;
	if(m_PierArea<=0.0f){
		tsx = tsy = 1.0f;
	}else{
		tsx = 1.0f+m_PierArea*m_PierPlugin->m_TaperX;
		tsy = 1.0f+m_PierArea*m_PierPlugin->m_TaperY;
	}
	if(m_PierPlugin->m_BaseObject.IsMeshValid()){
		if(g_ShowPierSelect && m_Selected) m_BaseObject.RenderSC(altmat);
		else m_BaseObject.Render();
		CastShadow(&m_BaseObject);
	}
	if(!g_ShowPierSelect && !g_ShadowNeeded
		&& (m_PierArea<=0.0f || !m_PierPlugin->HasInterval())) return;
	m_PierPlugin->ResetMapTemp();
	if(g_ShowPierSelect && m_Selected){
		devSetTexture(0, NULL);
		devSetMaterial(altmat);
		devResetMatrix();
		Dump(1);
	}
	if(m_PierPlugin->m_Direction) m_PierPlugin->Render(
		m_PierBegin, R2L(m_PierRight*tsx), R2L(m_PierUp*tsy), m_PierDir,
		m_PierBegin, R2L(m_PierRight*tsx), m_PierUp,
		m_PierEnd, m_PierRight, m_PierUp, m_PierDir,
		m_PierEnd, m_PierRight, m_PierUp, 3, m_PierArea, altmat);
	else m_PierPlugin->Render(
		m_PierBegin, m_PierRight, m_PierUp, m_PierDir,
		m_PierBegin, m_PierRight, m_PierUp,
		m_PierEnd, R2L(m_PierRight*tsx), R2L(m_PierUp*tsy), m_PierDir,
		m_PierEnd, m_PierRight, m_PierUp, 3, m_PierArea, altmat);
}

/*
 *	読込
 */
char *CPier::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Pier"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "PierPlugin", &pid))) throw CSynErr(eee);
	m_PierPlugin = g_PierPluginList->FindPlugin(pid.c_str(), true);
	VEC3 jpos, jdir, jup;
	if(!(str = AsgnVector3D(eee = str, "JointPos", &jpos))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "JointDir", &jdir))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "JointUp", &jup))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "SurfaceAlt", &m_SurfaceAlt))) throw CSynErr(eee);
	SetMesh();
	m_JointObject.SetPos(jpos);
	m_JointObject.SetDir(jdir, jup);
	if(m_PierPlugin) Init(NULL);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CPier::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tPier{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tPierPlugin = \"%s\";\n", CheckPluginID(m_PierPlugin));
	fprintf(df, "\t\t\t\tJointPos = "); V3Save(df, R2L(m_JointObject.GetPos()), ";\n");
	fprintf(df, "\t\t\t\tJointDir = "); V3Save(df, R2L(m_JointObject.GetDir()), ";\n");
	fprintf(df, "\t\t\t\tJointUp = "); V3Save(df, R2L(m_JointObject.GetUp()), ";\n");
	fprintf(df, "\t\t\t\tSurfaceAlt = %f;\n", m_SurfaceAlt);
	fprintf(df, "\t\t\t}\n");
}
