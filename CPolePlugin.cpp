#include "stdafx.h"
#include "CCamera.h"
#include "CPolePlugin.h"
#include "CPoleSelectMode.h"

//	外部グローバル
extern bool g_RailMipMap;

//	static メンバ
CObject CPolePlugin::ms_PreviewObject;

/*
 *	ロード
 */
bool CPolePlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "PoleInfo"))) throw CSynErr(eee);
		if(tmp = AsgnInteger(eee = str, "TrackNum", &m_TrackNum)) str = tmp;
		else m_TrackNum = 1;
		if(m_TrackNum<1){
			throw CSynErr(eee, lang(InvalidTrackNum));
		}else if(m_TrackNum>1){
			if(!(str = AsgnFloat(eee = str, "TrackInterval", &m_TrackInterval))) throw CSynErr(eee);
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Model"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_ModelFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "ModelScale", &m_ModelScale))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	m_Mesh = g_MeshList.Get(FALSE, (char *)m_ModelFileName.c_str(), 0, !g_RailMipMap);
	ChDir();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CPolePlugin::SetPreview(){
	ms_PreviewState = true;
	g_Pole = this;
	string desc = g_Pole->GetBasicInfo();
	if(m_TrackNum>1) desc += FlashIn("\n%s: %d\n%s: %.3f [m]",
		lang(TrackNum), m_TrackNum, lang(Interval), m_TrackInterval);
	desc += "\n"+g_Pole->GetDescription();
	g_PoleSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	プレビュー
 */
void CPolePlugin::Preview(
	VEC3 pos	//	支持点座標
){
	ms_PreviewObject.SetMesh(m_Mesh, V3ZERO, m_ModelScale);
	ms_PreviewObject.SetPos(pos);
	ms_PreviewObject.SetDir(V3DIR, V3UP);
	ms_PreviewObject.ResetMatFlag();
	ms_PreviewObject.Render();
	CastShadow(&ms_PreviewObject);
}
