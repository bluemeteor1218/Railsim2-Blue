#include "stdafx.h"
#include "CRailPlugin.h"
#include "CTiePlugin.h"
#include "CTieSelectMode.h"

/*
 *	ロード
 */
bool CTiePlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "TieInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Height", &m_Height))) throw CSynErr(eee);
		if(tmp = AsgnYesNo(eee = str, "FlattenCant", &m_FlattenCant)) str = tmp;
		else m_FlattenCant = false;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = ReadProfile(eee = str))) throw CSynErr(eee);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	LoadData();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CTiePlugin::SetPreview(){
	ms_PreviewState = true;
	g_Tie = this;
	string desc = g_Tie->GetBasicInfo();
	desc += "\n"+g_Tie->GetDescription();
	g_TieSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	ダンプ後処理
 */
void CTiePlugin::AfterDump(
	VEC3 &p1, VEC3 &u1,		//	始点 (正規化済)
	VEC3 &ip1, VEC3 &iu1,	//	非カント始点 (正規化済)
	VEC3 &p2, VEC3 &u2,		//	終点 (正規化済)
	VEC3 &ip2, VEC3 &iu2	//	非カント終点 (正規化済)
){
	ip1 -= m_Height*iu1;
	ip2 -= m_Height*iu2;
	if(m_FlattenCant){
		p1 = ip1; u1 = iu1;
		p2 = ip2; u2 = iu2;
	}else{
		p1 -= m_Height*u1;
		p2 -= m_Height*u2;
	}
}

/*
 *	橋脚設置位置計算
 */
void CTiePlugin::CalcPierPos(
	VEC3 *pos,		//	位置
	VEC3 *right,	//	right (正規化済)
	VEC3 *up,		//	up (正規化済)
	VEC3 *dir		//	dir (正規化済)
){
	if(m_FlattenCant){
		right->y = 0.0f;
		V3Norm(up, V3Cross(up, dir, V3Norm(right, right)));
	}
	*pos -= *up*m_Height;
}
