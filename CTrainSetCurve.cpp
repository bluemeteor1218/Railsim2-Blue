#include "stdafx.h"
#include "CRailWay.h"
#include "CTrain.h"
#include "CTrainGroup.h"
#include "CTrainSetCurve.h"
#include "CRailPlugin.h"
#include "CTrainPlugin.h"

//	関数宣言
void *ReplaceAdr(void *);

//	外部グローバル
extern CScene *g_Scene;
extern map<void *, void *> g_AddressMap;

/*
 *	コンストラクタ
 */
CAxlePosture::CAxlePosture(
	CAxleObject *axle,	//	車軸
	CTrain *train,		//	車輌
	bool term			//	端フラグ
){
	m_Axle = axle;
	m_Train = train;
	m_Rotation = m_Distance = 0.0f;
	m_Terminate = term;
	m_Rail = NULL;
}

/*
 *	姿勢設定
 */
void CAxlePosture::SetPosture(
	VEC3 pos,		//	座標
	VEC3 dir,		//	dir
	VEC3 up,		//	up
	CRailWay *way	//	レール
){
	m_Pos = pos;
	m_Dir = dir;
	m_Up = up;
	V3NormAxis(&m_Right, &m_Up, &m_Dir);
	m_Rail = way;
	if(m_Terminate) m_Train->ToggleSetting();
}

/*
 *	前後位置取得
 */
float CAxlePosture::GetZPos(){
	return m_Axle->m_Coord.x;
}

/*
 *	姿勢適用
 */
void CAxlePosture::Apply(){
	m_Axle->SetPosture(m_Pos, m_Dir, m_Up, m_Rotation);
}

/*
 *	車軸回転
 */
void CAxlePosture::Rotate(
	float dist,	//	回転数
	bool rev	//	逆転フラグ
){
	m_Rotation += m_Axle->CalcRotation(rev ? -dist : dist);
	m_Rotation -= floor(m_Rotation);
	float tmp = m_Distance;
	m_Distance += dist;
	if(m_Axle && m_Axle->m_WheelSound && m_Rail->GetScene()==g_Scene){
		CRailPlugin *rpi = m_Rail->GetRailPlugin();
		if(rpi) rpi->PlayWheelSound(tmp, m_Distance, m_Pos);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CTrainSetBuffer::CTrainSetBuffer(
	float sumlen,		//	積算距離
	bool rev,			//	反転フラグ
	CAxlePosture *ap	//	姿勢バッファ
){
	m_SumLen = sumlen;
	m_Reverse = rev;
	m_Posture = ap;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CGroupEndLocator::CGroupEndLocator(){
	m_Side = 0;
	m_Type = 0;
	m_Offset = 0.0f;
	m_SetRail = NULL;
	m_Group = NULL;
}

/*
 *	コンストラクタ
 */
CGroupEndLocator::CGroupEndLocator(
	int side,			//	サイド
	float ofs,			//	オフセット
	CRailWay *way,		//	設置レール
	CTrainGroup *group	//	編成
){
	m_Side = side;
	m_Type = 0;
	m_Offset = ofs;
	m_SetRail = way;
	m_Group = group;
}

/*
 *	レールにセット
 */
void CGroupEndLocator::Attach(
	int type	//	探索子フラグ
){
	m_Type = type;
	if(m_SetRail) m_SetRail->m_GroupEnd.push_back(this);
//	else Dialog("ERROR@CGroupEndLocator::Attach");
}

/*
 *	レールから解除
 */
void CGroupEndLocator::Detach(){
	if(m_SetRail){
		int before = m_SetRail->m_GroupEnd.size();
		m_SetRail->m_GroupEnd.remove(this);
		int after = m_SetRail->m_GroupEnd.size();
	//	if(before==after) Dialog("ERROR@CGroupEndLocator::Detach");
	}
}

/*
 *	進行方向取得 (m_Side || !m_Side)
 */
int CGroupEndLocator::GetDirection(){
	switch(m_Type){
	case 0: return m_Group->IsReverse() ? !m_Side : m_Side;
	case 1: return m_Group->IsReverse() ? m_Side : !m_Side;
	case 2: return m_Side;
	}
	return 0;
}

/*
 *	アドレス復元
 */
void CGroupEndLocator::RestoreAddress(){
	m_SetRail = (CRailWay *)ReplaceAdr(m_SetRail);
}

/*
 *	読込
 */
char *CGroupEndLocator::Read(
	char *str,	//	対象文字列
	char *pref	//	プレフィックス
){
	char *eee;
	if(!(str = BeginBlock(str, pref))) return NULL;
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	if(!(str = Assignment(eee = str, "Location"))) throw CSynErr(eee);
	if(!(str = ConstInteger(eee = str, &m_Side))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstFloat(eee = str, &m_Offset))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = HexPointer(eee = str, (void **)&m_SetRail))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CGroupEndLocator::Save(
	FILE *df,	//	ファイル
	char *ind,	//	インデント
	char *pref	//	プレフィックス
){
	fprintf(df, "%s%s{\n", ind, pref);
	fprintf(df, "%s\tAddress = %p;\n", ind, this);
	fprintf(df, "%s\tLocation = %d, %f, %p;\n", ind, m_Side, m_Offset, m_SetRail);
	fprintf(df, "%s}\n", ind);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CTrainSetCurve::CTrainSetCurve(
	CRailPlugin *rpi,		//	レールプラグイン
	CTiePlugin *tpi,		//	枕木プラグイン
	CGirderPlugin *gpi,		//	橋桁プラグイン
	bool rev,				//	逆方向フラグ
	CGroupEndLocator loc,	//	設置位置
	CGroupEndLocator *tail,	//	終端格納先
	ITrainSetBuffer *icur,	//	現在位置
	ITrainSetBuffer *iend	//	終了位置
):
	CRailTraceCurve(rpi, tpi, gpi, NULL)	//	基本クラス
{
	m_Reverse = rev;
	m_Location = loc;
	m_Tail = tail;
	m_Current = icur;
	m_End = iend;
}

/*
 *	トレース完了
 */
void CTrainSetCurve::FinishTrace(
	VEC3 &pos1, VEC3 &right1, VEC3 &up1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &right2, VEC3 &up2, VEC3 &dir2,	//	終点
	float sumlen, float seglen,	//	積算距離・セグメント距離
	CRailSplitter &splitter		//	分割子
){
	ITrainSetBuffer &cur = *m_Current;
	if(cur==*m_End) return;
	while(true){
		float tmp = (m_Reverse ? -cur->m_SumLen : cur->m_SumLen)+m_Location.m_Offset;
		float tmp2 = tmp-sumlen;
		if(seglen<tmp2) return;
	//	if(tmp2<0.0f) Dialog("Calculation Error");
		float q2 = tmp2/seglen, q1 = 1.0f-q2;
		VEC3 td = q1*dir1+q2*dir2;
		cur->SetPosture(q1*pos1+q2*pos2+0.5f*seglen*q1*q2*(dir1-dir2),
			m_Reverse ? td : -td, q1*up1+q2*up2, m_Location.m_SetRail);
		if(m_Reverse) cur--; else cur++;
		if(cur==*m_End){
			m_Tail->m_Side = m_Location.m_Side;
			m_Tail->m_Offset = tmp;
			m_Tail->m_SetRail = m_Location.m_SetRail;
			return;
		}
	}
}
