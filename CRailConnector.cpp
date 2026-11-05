#include "stdafx.h"
#include "CRailConnector.h"
#include "CRailWay.h"
#include "CScene.h"
#include "CTrainGroup.h"
#include "CConfigMode.h"

//	関数宣言
void CalcCantAxis(VEC3 *, VEC3 *, VEC3 *, float);

//	外部グローバル
extern CScene *g_Scene;
extern D3DCOLOR g_ColorSelect[];

void SetStaticPointOption(void *point_id, int point_opt){
	if(!point_id || !g_AddressMap.count(point_id)) ErrorDialog("SetStaticPointOption point_id error.");
	CRailConnector *rc = (CRailConnector *)g_AddressMap[point_id];
	rc->SetNetPoint(point_opt);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	カント計算
 */
void CRailSplitter::CalcCantAxis(
	float cant	//	カント量
){
	::CalcCantAxis(&m_Right, &m_Up, &m_Dir, cant);
}

/*
 *	中間点計算
 */
CRailSplitter CRailSplitter::CalcMid(
	CRailSplitter *rhs,	//	右辺
	float q1			//	混合率
){
	float q2 = 1.0f-q1;
	VEC3 tr, tu = q1*m_Up+rhs->m_Up, td = q1*m_Dir+rhs->m_Dir;
	V3NormAxis(&tr, &tu, &td);
	return CRailSplitter(q1*m_Pos+q2*rhs->m_Pos, tr, tu, td);
}

/*
 *	読込
 */
char *CRailSplitter::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Splitter"))) return NULL;
	if(!(str = AsgnVector3D(eee = str, "Pos", &m_Pos))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Dir", &m_Dir))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Up", &m_Up))) throw CSynErr(eee);
	V3NormAxis(&m_Right, &m_Up, &m_Dir);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CRailSplitter::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sSplitter{\n", ind);
	fprintf(df, "%s\tPos = ", ind); V3Save(df, m_Pos, ";\n");
	fprintf(df, "%s\tDir = ", ind); V3Save(df, m_Dir, ";\n");
	fprintf(df, "%s\tUp = ", ind); V3Save(df, m_Up, ";\n");
	fprintf(df, "%s}\n", ind);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	影終端フラグ計算
 */
bool CRailConnectorLink::GetShadowTerminate(){
	CRailWayLink *come = m_Link->m_Link[m_Side];
	CRailWayLink *fow = m_Link->m_Link[!m_Side];
	if(fow[0].m_Link && fow[1].m_Link) return false;
	if(fow[0].m_Link || fow[1].m_Link) return !m_Point || !come[!m_Point].m_Link;
	return true;
}

/*
 *	コネクタリンク数計算
 */
int CRailConnectorLink::GetLinkCount(){
	return m_Link->GetLinkCount();
}

/*
 *	アドレス復元
 */
void CRailConnectorLink::RestoreAddress(){
	m_Link = (CRailConnector *)ReplaceAdr(m_Link);
}

/*
 *	読込
 */
char *CRailConnectorLink::Read(
	char *str,	//	対象文字列
	char *pref	//	プレフィックス
){
	char *eee;
	if(!(str = Assignment(eee = str, pref))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Side))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Point))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_Link))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CRailConnectorLink::Save(
	FILE *df,	//	ファイル
	char *pref	//	プレフィックス
){
	fprintf(df, "%s%d, %d, %p;\n", pref, m_Side, m_Point, m_Link);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	反対側へワープ
 */
void CRailWayLink::WarpToOppositeEnd(){
	CScene *scene = m_Link->m_Link[!m_Side].m_Link->GetScene();
	CCamera *camera = NULL;
	if(scene!=g_Scene){
		scene->Enter(true);
		CWindowInfo *active_wnd = g_ConfigMode->GetActiveWindow();
		if(g_ConfigMode->IsWindowDiv() && active_wnd){
			active_wnd->SetScene(g_Scene);
			camera = active_wnd->GetCamera();
		}
	}
	if(!camera) camera = CCamera::GetCurrentCamera();
	camera->SetFocus(m_Link->m_Link[!m_Side].GetPos());
}

/*
 *	アドレス復元
 */
void CRailWayLink::RestoreAddress(){
	m_Link = (CRailWay *)ReplaceAdr(m_Link);
}

/*
 *	読込
 */
char *CRailWayLink::Read(
	char *str,	//	対象文字列
	char *pref	//	プレフィックス
){
	char *eee;
	if(!(str = Assignment(eee = str, pref))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Side))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_Link))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CRailWayLink::Save(
	FILE *df,	//	ファイル
	char *pref	//	プレフィックス
){
	fprintf(df, "%s%d, %p;\n", pref, m_Side, m_Link);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	アドレス復元
 */
void CPierPos::RestoreAddress(){
	m_Link = (CPier *)ReplaceAdr(m_Link);
}

/*
 *	読込
 */
char *CPierPos::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = Assignment(eee = str, "PierLink"))) return NULL;
	if(!(str = ConstFloat(eee = str, &m_Pos))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_Link))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CPierPos::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\t\t\tPierLink = %f, %p;\n", m_Pos, m_Link);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	アドレス復元
 */
void CPolePos::RestoreAddress(){
	m_Link = (CPole *)ReplaceAdr(m_Link);
}

/*
 *	読込
 */
char *CPolePos::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = Assignment(eee = str, "PoleLink"))) return NULL;
	if(!(str = ConstFloat(eee = str, &m_Pos))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Track))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = BoolYesNo(eee = str, &m_Multi))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_Link))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CPolePos::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\t\t\tPoleLink = %f, %d, %s, %p;\n",
		m_Pos, m_Track, YESNO[m_Multi], m_Link);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
CRailConnector **CRailConnector::ms_Root = NULL;
float CRailConnector::ms_MinDist;
CRailConnector *CRailConnector::ms_Detect;

/*
 *	コンストラクタ (読込用)
 */
CRailConnector::CRailConnector(){
	m_OldAdr = NULL;
	m_Selected = 0;
	m_Side = 0;
	m_TrailPoint[0] = m_TrailPoint[1] = 0;
	m_Cant = 0.0f;
	m_CantProc = true;
	m_NetPoint = 0;
	m_User = NULL;
	m_Scene = g_Scene;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CRailConnector::CRailConnector(
	VEC3 &pos,	//	座標
	VEC3 &dir	//	dir
):
	m_Splitter(pos, dir)	//	分割子
{
	m_OldAdr = NULL;
	m_Selected = 0;
	m_TrailPoint[0] = m_TrailPoint[1] = 0;
	m_Cant = 0.0f;
	m_CantProc = false;
	m_NetPoint = 0;
	m_User = NULL;
	m_Scene = g_Scene;
	m_Next = *ms_Root;
	*ms_Root = this;
}

/*
 *	デストラクタ
 */
CRailConnector::~CRailConnector(){
	DELETE_V(m_Next);
}

/*
 *	手動ポイント設定
 */
void CRailConnector::SetNetPoint(int np){
	m_NetPoint = np;
	((CPointElement *)m_PointInst.SearchEffect(NULL)->DequeueBase(false))->SetValue(m_NetPoint ? 2 : 1);
}

/*
 *	ポイント設定ネットワーク同期
 */
void CRailConnector::SwitchNetPoint(){
	if(g_NetworkInitialized){
		if(!m_OldAdr) return;
		void EnqueuePointControl(void *, int);
		EnqueuePointControl(m_OldAdr, !m_NetPoint);
	}else{
		SetNetPoint(!m_NetPoint);
	}
}

/*
 *	未使用ならリストから削除
 */
CRailConnector *CRailConnector::Delete(){
	int i, j;
	for(i = 0; i<2; i++) for(j = 0; j<2; j++) if(m_Link[i][j].m_Link) return this;
	CRailConnector *next = m_Next;
	m_Next = NULL;
	delete this;
	return next;
}

/*
 *	編成撤去
 */
void CRailConnector::RemoveGroup(){
	if(m_User) m_User->Remove();
}

/*
 *	確定
 */
void CRailConnector::Stabilize(
	VEC3 up	//	up
){
	m_CantProc = true;
	VEC3 fr, fu = V3UP, fd = m_Splitter.m_Dir;
	V3NormAxis(&fr, &fu, &fd);
	m_Cant = V3Dot(&up, &fr)/V3Dot(&up, &fu);
	m_Splitter.CalcCantAxis(m_Cant);
}

/*
 *	コネクタリンク数計算
 */
int CRailConnector::GetLinkCount(){
	int i, j, n = 0;
	for(i = 0; i<2; i++) for(j = 0; j<2; j++) if(m_Link[i][j].m_Link) n++;
	return n;
}

/*
 *	進入可能性チェック
 */
bool CRailConnector::CheckRailBlock(CTrainGroup *group){
	int i, j;
	for(i = 0; i<2; i++){
		for(j = 0; j<2; j++){
			CRailWay *way = m_Link[i][j].m_Link;
			if(way && !way->CheckRailBlock(group)) return false;
		}
	}
	return true;
}

/*
 *	入力チェック
 */
void CRailConnector::ScanInput(
	int mode,		//	モード
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	if(GetLinkCount()<3) return;
	VEC3 center = m_Splitter.m_Pos;
	VEC3 sc = WorldToScreen(center);
	if(sc.z<0.0f) return;
	switch(mode){
	case 4: {
		float dist = V3Len(&(rect1-sc));
		if(dist<DETECT_2D_MAX && (ms_MinDist<0.0f || ms_MinDist>dist)){
			ms_MinDist = dist;
			ms_Detect = this;
		}
		m_Selected &= 2;
		break; }
	case 5:
		m_Selected = 0;
		break;
	case 6:
		m_Selected = m_Selected<<1;
		m_Selected &= 2;
		return;
	}
}

/*
 *	ポリゴンをダンプ
 */
bool CRailConnector::Dump(){
	if(m_CantProc) return false;
	int i, j, n = 0;
	bool left = false, right = false;
	m_Cant = 0.0f;
	for(i = 0; i<2; i++){
		for(j = 0; j<2; j++){
			if(m_Link[i][j].m_Link){
				n++;
				float tmp = m_Link[i][j].GetCant();
				if(!i) tmp = -tmp;
				m_Cant += tmp;
				if(tmp<0.0f) left = true;
				if(tmp>0.0f) right = true;
			}
		}
	}
	if(left && right) m_Cant = 0.0f;
	else m_Cant /= n;
	m_Splitter.CalcCantAxis(m_Cant);
	return m_CantProc = true;
}

/*
 *	コネクタ描画
 */
void CRailConnector::Render(
	D3DCOLOR color,	//	線色
	bool net_point	//	ネットポイント表示
){
	if(!color) color = g_ColorSelect[m_Selected];
	Draw3DPointAs2DRect(m_Splitter.m_Pos, color, 5);
	if(!net_point) return;
	VEC3 p = WorldToScreen(m_Splitter.m_Pos);
	int ix = (int)p.x+10, iy = (int)p.y-FONT_HEIGHT/2;
	CStringDrawer *sd = g_StrTex->DrawString(m_NetPoint ? lang(Right) : lang(Left), 0xff000000);
	sd->RenderLeft(ix, iy, color);
	int w = sd->GetWidth();
	devSetTexture(0, NULL);
	Fill2DRect(ix-2, iy-2, ix+w+2, iy+FONT_HEIGHT+2, 0x80ffffff);
	sd->RenderLeft(ix, iy, color, 0xff000000);
	devSetTexture(0, NULL);
}

/*
 *	アドレス復元
 */
void CRailConnector::RestoreAddress(){
	int i, j;
	for(i = 0; i<2; i++) for(j = 0; j<2; j++) m_Link[i][j].RestoreAddress();
	m_PointInst.RestoreAddress();
	m_User = (CTrainGroup *)ReplaceAdr(m_User);

}

/*
 *	読込
 */
char *CRailConnector::Read(
	char *str	//	対象文字列
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "RailConnector"))){
		delete this;
		return NULL;
	}
	if(!(str = AsgnPointer(eee = str, "Address", &m_OldAdr))) throw CSynErr(eee);
	g_AddressMap[m_OldAdr] = this;
	if(!(str = m_Splitter.Read(eee = str))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Cant", &m_Cant))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "Side", &m_Side))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "TrailPoint", m_TrailPoint, 2, false))) throw CSynErr(eee);
	if(!(str = AsgnPointer(eee = str, "User", (void **)&m_User))) throw CSynErr(eee);
	int i, j;
	for(i = 0; i<2; i++) for(j = 0; j<2; j++)
		if(!(str = m_Link[i][j].Read(eee = str, FlashIn("Link%d%d", i, j)))) throw CSynErr(eee);
	if(tmp = m_PointInst.Read(str)) str = tmp;
	m_NetPoint = ((CPointElement *)m_PointInst.SearchEffect(NULL)->DequeueBase(false))->GetValue();
	if(m_NetPoint>0) m_NetPoint = m_NetPoint==2;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CRailConnector::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tRailConnector{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	m_Splitter.Save(df, "\t\t\t\t");
	fprintf(df, "\t\t\t\tCant = %f;\n", m_Cant);
	fprintf(df, "\t\t\t\tSide = %d;\n", m_Side);
	fprintf(df, "\t\t\t\tTrailPoint = %d, %d;\n", m_TrailPoint[0], m_TrailPoint[1]);
	fprintf(df, "\t\t\t\tUser = %p;\n", m_User);
	int i, j;
	for(i = 0; i<2; i++) for(j = 0; j<2; j++)
		m_Link[i][j].Save(df, FlashIn("\t\t\t\tLink%d%d = ", i, j));
	if(GetLinkCount()>2) m_PointInst.Save(df, "\t\t\t\t");
	fprintf(df, "\t\t\t}\n");
}
