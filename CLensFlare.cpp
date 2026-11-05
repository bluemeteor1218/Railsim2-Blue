#include "stdafx.h"
#include "CLensFlare.h"
#include "CModelPlugin.h"
#include "CConfigMode.h"

/*
 *	読込
 */
char *CFlareElement::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	m_Texture = NULL;
	if(tmp = BeginBlock(str, "Circle")){
		str = tmp;
		m_Type = 0;
	}else if(tmp = BeginBlock(str, "Hexagon")){
		str = tmp;
		m_Type = 1;
	}else if(tmp = BeginBlock(str, "Texture")){
		str = tmp;
		m_Type = 2;
	}else{
		return NULL;
	}
	if(!(str = AsgnFloat(eee = str, "Distance", &m_Distance))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Radius", &m_Radius))) throw CSynErr(eee);
	if(m_Type<2){
		if(!(str = AsgnColor(eee = str, "InnerColor", &m_InnerColor))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "OuterColor", &m_OuterColor))) throw CSynErr(eee);
	}else{
		if(!(str = AsgnString(eee = str, "TexFileName", &m_TexFileName))) throw CSynErr(eee);
		if(tmp = AsgnColor(eee = str, "Color", &m_InnerColor)) str = tmp;
		else m_InnerColor = 0xffffffff;
		m_OuterColor = m_InnerColor;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CFlareElement::LoadData(){
	if(m_Type==2) m_Texture = g_TexList.Get(
		FALSE, m_TexFileName.c_str(), 0, !g_NamedObjectMipMap);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CLensFlare::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "LensFlare"))) return NULL;
	if(!(str = AsgnFloat(eee = str, "StartAngle", &m_StartAngle))) throw CSynErr(eee);
	if(tmp = AsgnFloat(eee = str, "Twinkle", &m_Twinkle)) str = tmp;
	else m_Twinkle = 0.0f;
	if(tmp = AsgnFloat(eee = str, "Inclination", &m_Inclination)) str = tmp;
	else m_Inclination = 0.0f;
	m_StartAngle = cosf(D3DXToRadian(m_StartAngle));
	m_Flare.clear();
	CFlareElement flare;
	while(tmp = flare.Read(str)){
		str = tmp;
		m_Flare.push_back(flare);
	}
	m_Flare.sort();
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CLensFlare::LoadData(){
	IFlareElement ifl = m_Flare.begin();
	for(; ifl!=m_Flare.end(); ifl++) ifl->LoadData();
}

/*
 *	レンダリング
 */
void CLensFlare::Render(
	VEC3 pos,		//	光源座標
	VEC3 dir,		//	光源方向
	VEC3 toward,	//	基準方向
	float vol,		//	マスタアルファ値
	float maxdist	//	最大距離
){
	if(!m_Flare.size()) return;
	float range = V3Len(&(GetVPos()-pos));
	VEC3 vCamera = GetVDir();
	V3Norm(&dir, &dir);
	V3Norm(&toward, &toward);
	V3Norm(&vCamera, &vCamera);
	float angle = V3Dot(&dir, &-toward);
	if(angle<=m_StartAngle) return;
	devBLEND_ADD2();	//	加算モードに設定
	float alpha = vol*(angle-m_StartAngle)/(1.0f-m_StartAngle);
	float shift = 0.0f, bl = alpha*FRand2(1.0f-m_Twinkle, 1.0f);
	IFlareElement ifl = m_Flare.begin();
	VEC3 tpos = pos, approach = dir+m_Inclination*vCamera;
	//V3Norm(&approach, &approach);
	for(; ifl!=m_Flare.end(); ifl++){
		float dist = ifl->m_Distance;
		float atn = bl*(maxdist<0.0f ? 1.0f : (maxdist-V3Len(&(tpos-pos)))/maxdist);
		tpos += range*(dist-shift)*approach;
		if(atn<=0.0f) break;
		devTransBillboard(tpos);
		switch(ifl->m_Type){
		case 0:
			devSetTexture(0, NULL);
			Fill3DCircle(V3ZERO, ifl->m_Radius,
				ScaleColor(ifl->m_InnerColor, atn), ScaleColor(ifl->m_OuterColor, atn));
			break;
		case 1:
			devSetTexture(0, NULL);
			Fill3DHex(V3ZERO, ifl->m_Radius,
				ScaleColor(ifl->m_InnerColor, atn), ScaleColor(ifl->m_OuterColor, atn));
			break;
		case 2:
			SetUVMap(0.0f, 0.0f, 1.0f, 1.0f);
			devSetTexture(0, ifl->m_Texture);
			TexMap3DRect(
				VEC3(-ifl->m_Radius, ifl->m_Radius, 0.0f),
				VEC3(ifl->m_Radius, ifl->m_Radius, 0.0f),
				VEC3(ifl->m_Radius, -ifl->m_Radius, 0.0f),
				VEC3(-ifl->m_Radius, -ifl->m_Radius, 0.0f),
				ScaleColor(ifl->m_InnerColor, atn));
			break;
		}
		shift = dist;
	}
	devBLEND_ALPHA();	//	半透明モードに戻す
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CWhiteout::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Whiteout"))) return NULL;
	if(!(str = AsgnFloat(eee = str, "StartAngle", &m_StartAngle))) throw CSynErr(eee);
	if(!(str = AsgnColor(eee = str, "Color", &m_Color))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	m_StartAngle = cosf(D3DXToRadian(m_StartAngle));
	return str;
}

/*
 *	レンダリング
 */
void CWhiteout::Render(
	VEC3 dir,	//	光源方向
	float vol	//	マスタアルファ値
){
	if(!m_Color) return;
	VEC3 vLight = -dir;			//	カメラから光源へのベクトル
	VEC3 vCamera = GetVDir();	//	カメラの向き
	V3Norm(&vCamera, &vCamera);
	V3Norm(&vLight, &vLight);
	float angle = V3Dot(&vLight, &vCamera);
	if(angle<=m_StartAngle) return;
	float alpha = vol*(angle-m_StartAngle)/(1.0f-m_StartAngle);
	Fill2DRect(0, 0, g_DispWidth, g_DispHeight, ScaleColor(m_Color, alpha));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CHeadlightInst::CHeadlightInst(
	VEC3 pos,		//	光源座標
	VEC3 dir,		//	光源方向
	CHeadlight *hl	//	ヘッドライト
){
	m_RenderPos = pos;
	m_RenderDir = dir;
	m_Headlight = hl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
list<CHeadlightInst> CHeadlight::ms_RenderList;

/*
 *	[static]
 *	リスト初期化
 */
void CHeadlight::InitRenderList(){
	ms_RenderList.clear();
}

/*
 *	[static]
 *	全てレンダリング
 */
void CHeadlight::RenderAll(){
	devSetTexture(0, NULL);
	//devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	IHeadlightInst ihi = ms_RenderList.begin();
	for(; ihi!=ms_RenderList.end(); ihi++) ihi->m_Headlight->Render(&*ihi);
	ms_RenderList.clear();
	//devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
}

/*
 *	読込
 */
char *CHeadlight::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	車輌プラグイン
){
	char *tmp, *eee;
	string obj;
	if(!(str = BeginBlock(str, "Headlight"))) return NULL;
	if(!(str = AsgnString(eee = str, "AttachObject", &obj))) throw CSynErr(eee);
	if(!(m_Link = mpi->FindObject(obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(!(str = AsgnVector3D(eee = str, "SourceCoord", &m_SourceCoord))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Direction", &m_Direction))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "MaxDistance", &m_MaxDistance))) throw CSynErr(eee);
	if(tmp = m_LensFlare.Read(eee = str)) str = tmp;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CHeadlight::LoadData(){
	m_LensFlare.LoadData();
}

/*
 *	レンダリングリストに登録
 */
bool CHeadlight::Register(){
	CObject *obj = m_Link->GetObject();
	VEC3 renderpos = obj->GetPos(), renderdir;
	VEC3 right, up, dir, ldir, tdir;
	V3Norm(&right, &obj->GetRight());
	V3Norm(&up, &obj->GetUp());
	V3Norm(&dir, &obj->GetDir());
	renderpos += m_SourceCoord.x*right+m_SourceCoord.y*up+m_SourceCoord.z*dir;
	V3Norm(&renderdir, &(m_Direction.x*right+m_Direction.y*up+m_Direction.z*dir));
	ms_RenderList.push_back(CHeadlightInst(renderpos, renderdir, this));
	return true;
}

/*
 *	レンダリング
 */
void CHeadlight::Render(
	CHeadlightInst *inst	//	インスタンス
){
	m_LensFlare.Render(inst->m_RenderPos, inst->m_RenderDir,
		inst->m_RenderPos-GetVPos(), 1.0f, m_MaxDistance);
}
