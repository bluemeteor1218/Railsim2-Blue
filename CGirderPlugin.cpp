#include "stdafx.h"
#include "CRailPlugin.h"
#include "CGirderPlugin.h"
#include "CGirderSelectMode.h"

/*
 *	ÉçÅ[Éh
 */
bool CGirderPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "GirderInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Height", &m_Height))) throw CSynErr(eee);
		if(tmp = AsgnInteger(eee = str, "TrackNum", &m_TrackNum)) str = tmp;
		else m_TrackNum = 1;
		if(m_TrackNum<1){
			throw CSynErr(eee, lang(InvalidTrackNum));
		}else if(m_TrackNum>1){
			if(!(str = AsgnFloat(eee = str, "TrackInterval", &m_TrackInterval))) throw CSynErr(eee);
		}
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
 *	ÉvÉåÉrÉÖÅ[ê›íË
 */
void CGirderPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Girder = this;
	string desc = g_Girder->GetBasicInfo();
	if(m_TrackNum>1) desc += FlashIn("\n%s: %d\n%s: %.3f [m]",
		lang(TrackNum), m_TrackNum, lang(Interval), m_TrackInterval);
	desc += "\n"+g_Girder->GetDescription();
	g_GirderSelectMode->SetProperty((char *)desc.c_str());
}

/*
 *	ã¥ãrê›íuà íuåvéZ
 */
void CGirderPlugin::CalcPierPos(
	VEC3 *pos,		//	à íu
	VEC3 *right,	//	right (ê≥ãKâªçœ)
	VEC3 *up,		//	up (ê≥ãKâªçœ)
	VEC3 *dir		//	dir (ê≥ãKâªçœ)
){
	if(m_FlattenCant){
		right->y = 0.0f;
		V3Norm(up, V3Cross(up, dir, V3Norm(right, right)));
	}
	*pos -= *up*m_Height;
}
