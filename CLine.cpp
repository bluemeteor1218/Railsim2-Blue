#include "stdafx.h"
#include "CLine.h"
#include "CScene.h"
#include "CLinePlugin.h"
#include "CPolePlugin.h"

//	外部グローバル
extern bool g_ShowPoleSelect;
extern bool g_ShowLineSelect;
extern MAT8 g_MatSelect[];
extern MAT8 g_MatSelectA[];

/*
 *	コンストラクタ
 */
CPoleLink::CPoleLink(
	int side,	//	サイド
	int track,	//	軌道番号
	CPole *pole	//	接続架線柱
){
	m_Side = side;
	m_Track = track;
	m_Link = pole;
}

/*
 *	接続点描画
 */
void CPoleLink::Render(
	D3DCOLOR color	//	線色
){
	Draw3DPointAs2DRect(GetPos(), color, 5);
}

/*
 *	アドレス復元
 */
void CPoleLink::RestoreAddress(){
	m_Link = (CPole *)ReplaceAdr(m_Link);
}

/*
 *	読込
 */
char *CPoleLink::Read(
	char *str,	//	対象文字列
	char *pref	//	プレフィックス
){
	char *eee;
	if(!(str = Assignment(eee = str, pref))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Side))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Track))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_Link))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CPoleLink::Save(
	FILE *df,	//	ファイル
	char *pref	//	プレフィックス
){
	fprintf(df, "%s%d, %d, %p;\n", pref, m_Side, m_Track, m_Link);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
CPole **CPole::ms_Root = NULL;
float CPole::ms_MinDist;
CPoleLink CPole::ms_Detect;

/*
 *	[static]
 *	接続点を表示
 */
bool CPole::RenderLink(){
	if(ms_MinDist<0.0f) return false;
	devSetTexture(0, NULL);
	devSetLighting(FALSE);
	devResetMatrix();
	ms_Detect.Render(0xff00ffff);
	return true;
}

/*
 *	コンストラクタ (読込用)
 */
CPole::CPole(){
	m_Selected = 0;
	m_PolePlugin = NULL;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CPole::CPole(
	const VEC3 &pos,		//	座標
	const VEC3 &dir,		//	方向
	CPolePlugin *pp	//	架線柱プラグイン
){
	m_Selected = 0;
	m_Pos = pos;
	m_OrigDir = dir;
	m_Up = V3UP;
	m_Dir = VEC3(m_OrigDir.x, 0.0f, m_OrigDir.z);
	V3NormAxis(&m_Right, &m_Up, &m_Dir);
	if(m_PolePlugin = pp){
		m_Object.SetMesh(m_PolePlugin->m_Mesh, m_Pos, m_PolePlugin->m_ModelScale);
		m_Object.SetDir(m_Dir, m_Up);
	}else{
		m_Object.SetMesh(NULL, m_Pos, 1.0f);
		m_Object.SetDir(m_Dir, m_Up);
	}
	m_Next = *ms_Root;
	*ms_Root = this;
}

/*
 *	デストラクタ
 */
CPole::~CPole(){
	DELETE_V(m_Next);
}

/*
 *	リストから削除
 */
void CPole::Delete(
	CScene *scene	//	シーン
){
	while(m_LineList.size()) scene->DeleteLine(*m_LineList.begin());
	g_Scene->DeletePoleLink(this);
	m_Next = NULL;
	delete this;
}

/*
 *	支持点座標取得
 */
VEC3 CPole::GetJointPos(
	int track	//	軌道番号
){
	return m_PolePlugin ? m_Pos+m_Right*(m_PolePlugin->m_TrackInterval
		*(track-0.5f*(m_PolePlugin->m_TrackNum-1))) : m_Pos;
}

/*
 *	入力チェック
 */
void CPole::ScanInput(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	if(mode==1){	//	scan pole joint link
		int i, n = m_PolePlugin ? m_PolePlugin->GetTrackNum() : 1;
		for(i = 0; i<n; i++){
			VEC3 p = WorldToScreen(GetJointPos(i));
			if(p.z<0.0f) continue;
			float dist = V3Len(&(rect1-p));
			if(dist<DETECT_2D_MAX && (ms_MinDist<0.0f || dist<ms_MinDist)){
				ms_MinDist = dist;
				ms_Detect = CreateLink(0, i);
			}
		}
		return;
	}
	VEC3 center = m_Object.GetCenter();
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
		float th = 500.0f*m_Object.GetRadius()/(V3Len(&(GetVPos()-center))*g_FovRatio);
		if(th<DETECT_2D_MIN) th = DETECT_2D_MIN;
		if(dist<th && (ms_MinDist<0.0f || ms_MinDist>dist)){
			ms_MinDist = dist;
			ms_Detect = CreateLink(0, 0);
		}
		m_Selected &= 2;
		break;
	}
}

/*
 *	コンストラクタ
 */
void CPole::Render(){
	if(!m_PolePlugin) return;
	m_Object.ResetMatFlag();
	if(g_ShowPoleSelect && m_Selected) m_Object.RenderSC(&g_MatSelect[m_Selected]);
	else m_Object.Render();
	CastShadow(&m_Object);
}

/*
 *	アドレス復元
 */
void CPole::RestoreAddress(){
	IPLine ipl = m_LineList.begin();
	for(; ipl!=m_LineList.end(); ipl++) *ipl = (CLine *)ReplaceAdr(*ipl);
}

/*
 *	読込
 */
char *CPole::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "Pole"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "PolePlugin", &pid))) throw CSynErr(eee);
	m_PolePlugin = g_PolePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = AsgnVector3D(eee = str, "Pos", &m_Pos))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "OrigDir", &m_OrigDir))) throw CSynErr(eee);
	m_Up = V3UP;
	m_Dir = VEC3(m_OrigDir.x, 0.0f, m_OrigDir.z);
	V3NormAxis(&m_Right, &m_Up, &m_Dir);
	if(m_PolePlugin){
		m_Object.SetMesh(m_PolePlugin->m_Mesh, m_Pos, m_PolePlugin->m_ModelScale);
		m_Object.SetDir(m_Dir, m_Up);
	}else{
		m_Object.SetMesh(NULL, m_Pos, 1.0f);
		m_Object.SetDir(m_Dir, m_Up);
	}
	if(tmp = Assignment(str, "LineList")){
		str = tmp;
		do{
			if(m_LineList.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
			CLine *line;
			if(!(str = HexPointer(eee = str, (void **)&line))) throw CSynErr(eee);
			m_LineList.push_back(line);
		} while(!(tmp = Character2(str, ';')));
		str = tmp;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CPole::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tPole{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tPolePlugin = \"%s\";\n", CheckPluginID(m_PolePlugin));
	fprintf(df, "\t\t\t\tPos = "); V3Save(df, m_Pos, ";\n");
	fprintf(df, "\t\t\t\tOrigDir = "); V3Save(df, m_OrigDir, ";\n");
	if(m_LineList.size()){
		fprintf(df, "\t\t\t\tLineList = ");
		IPLine ipl = m_LineList.begin();
		for(; ipl!=m_LineList.end(); ipl++) fprintf(df,
			ipl==m_LineList.begin() ? "%p" : ", %p", *ipl);
		fprintf(df, ";\n");
	}
	fprintf(df, "\t\t\t}\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
CLine **CLine::ms_Root = NULL;
float CLine::ms_MinDist;
CLine *CLine::ms_Detect;

/*
 *	コンストラクタ (読込用)
 */
CLine::CLine(){
	m_Selected = 0;
	m_LinePlugin = NULL;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CLine::CLine(
	CPoleLink link1,	//	始点
	CPoleLink link2,	//	終点
	CLinePlugin *lp		//	架線プラグイン
){
	m_Selected = 0;
	m_Link[0] = link1;
	m_Link[1] = link2;
	m_Link[0].Connect(this);
	m_Link[1].Connect(this);
	m_LinePlugin = lp;
	VEC3 up = V3UP;
	m_Dir = link1.GetPos()-link2.GetPos();
	float len = V3Len(&m_Dir);
	m_Dir.y = 0.0f;
	V3NormAxis(&m_Right, &up, &m_Dir);
	if(m_LinePlugin){
		m_LinePlugin->CopyMapTemp(m_LineMapV);
		m_LinePlugin->AddMapTemp(V3Len(&(link1.GetPos()-link2.GetPos())));
	}
	m_Next = *ms_Root;
	*ms_Root = this;
}

/*
 *	デストラクタ
 */
CLine::~CLine(){
	DELETE_V(m_Next);
}

/*
 *	リストから削除
 */
void CLine::Delete(){
	m_Link[0].Disconnect(this);
	m_Link[1].Disconnect(this);
	m_Next = NULL;
	delete this;
}

/*
 *	入力チェック
 */
void CLine::ScanInput(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	VEC3 center = 0.5f*(m_Link[0].GetPos()+m_Link[1].GetPos());
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
		float th = 250.0f*V3Len(&(m_Link[0].GetPos()-m_Link[1].GetPos()))
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
void CLine::Dump(){
	if(!m_LinePlugin) return;
	m_LinePlugin->SetMapTemp(m_LineMapV);
	VEC3 p1 = m_Link[0].GetPos()-m_LinePlugin->m_Height*V3UP;
	VEC3 p2 = m_Link[1].GetPos()-m_LinePlugin->m_Height*V3UP;
	m_LinePlugin->Dump(
		p1, R2L(-m_Link[0].GetRight()), R2L(m_Link[0].GetUp()),
		p1, R2L(-m_Link[0].GetRight()), R2L(m_Link[0].GetUp()),
		p2, R2L(m_Link[1].GetRight()), R2L(m_Link[1].GetUp()),
		p2, R2L(m_Link[1].GetRight()), R2L(m_Link[1].GetUp()), V3Len(&(p2-p1)), 4);
}

/*	
 *	等間隔オブジェクトをレンダリング
 */
void CLine::Render(){
	if(!g_ShadowNeeded && !g_ShowLineSelect
		&& !(m_LinePlugin && m_LinePlugin->HasInterval())) return;
	VEC3 p1 = m_Link[0].GetPos()-m_LinePlugin->m_Height*V3UP;
	VEC3 p2 = m_Link[1].GetPos()-m_LinePlugin->m_Height*V3UP;
	float seglen = V3Len(&(p2-p1));
	MAT8 *altmat = m_Selected ? &g_MatSelect[m_Selected] : NULL;
	MAT8 *altmat2 = m_Selected ? &g_MatSelectA[m_Selected] : NULL;
	if(g_ShowLineSelect && m_Selected){
		devSetTexture(0, NULL);
		devSetMaterial(altmat2);
		devResetMatrix();
		m_LinePlugin->Dump(p1, m_Right, R2L(V3UP), p1, m_Right, R2L(V3UP),
			p2, m_Right, R2L(V3UP), p2, m_Right, R2L(V3UP), seglen, 1);
	}
	m_LinePlugin->SetMapTemp(m_LineMapV);
	m_LinePlugin->Render(p1, m_Right, R2L(V3UP), m_Dir, p1, m_Right, R2L(V3UP),
		p2, m_Right, R2L(V3UP), m_Dir, p2, m_Right, R2L(V3UP), 3, seglen, altmat);
}

/*
 *	アドレス復元
 */
void CLine::RestoreAddress(){
	m_Link[0].RestoreAddress();
	m_Link[1].RestoreAddress();
}

/*
 *	読込
 */
char *CLine::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Line"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "LinePlugin", &pid))) throw CSynErr(eee);
	m_LinePlugin = g_LinePluginList->FindPlugin(pid.c_str(), true);
	if(!(str = ReadMapVector(eee = str, "LineMapV", m_LineMapV))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Right", &m_Right))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Dir", &m_Dir))) throw CSynErr(eee);
	if(!(str = m_Link[0].Read(eee = str, "Link0"))) throw CSynErr(eee);
	if(!(str = m_Link[1].Read(eee = str, "Link1"))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CLine::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tLine{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tLinePlugin = \"%s\";\n", CheckPluginID(m_LinePlugin));
	SaveMapVector(df, "\t\t\t\tLineMapV = ", m_LineMapV);
	fprintf(df, "\t\t\t\tRight = "); V3Save(df, m_Right, ";\n");
	fprintf(df, "\t\t\t\tDir = "); V3Save(df, m_Dir, ";\n");
	m_Link[0].Save(df, "\t\t\t\tLink0 = ");
	m_Link[1].Save(df, "\t\t\t\tLink1 = ");
	fprintf(df, "\t\t\t}\n");
}
