#include "stdafx.h"
#include "CCamera.h"
#include "CSaveFile.h"
#include "CModelPlugin.h"
#include "CEnvPlugin.h"
#include "CEnvEditMode.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"

//	内部定数
const float AXIAL_INCLINATION = D3DXToRadian(23.44f);	//	地軸の傾き
const float DAYS_PER_YEAR = 365.2425f;					//	年当たり平均日数
const float REV_PER_DAY = 2.0f*D3DX_PI/DAYS_PER_YEAR;	//	1 日当たり公転角度
const float ROT_PER_DAY = 2.0f*D3DX_PI+REV_PER_DAY;		//	1 日当たり自転角度

//	外部グローバル
extern int g_AncientNightFlag;
extern COffScreen g_HidefCapture;
extern bool g_HidefCaptureFlag;

//	内部グローバル
float g_DayAlpha;
float g_NightAlpha;
D3DCOLOR g_NoLightColor;

/*
 *	読込
 */
char *CLightSetting::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Set"))) return NULL;
	if(!(str = AsgnFloat(eee = str, "SunAlt", &m_SunAlt))) throw CSynErr(eee);
	if(!(str = AsgnColor(eee = str, "Directional", &m_Directional))) throw CSynErr(eee);
	if(!(str = AsgnColor(eee = str, "Ambient", &m_Ambient))) throw CSynErr(eee);
	if(!(str = AsgnColor(eee = str, "SkyColor", &m_SkyColor))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CMoon::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Moon"))) return NULL;
	if(!(str = AsgnString(eee = str, "ModelFileName", &m_MoonFile))) throw CSynErr(eee);
	if(tmp = AsgnFloat(str, "ModelScale", &m_MoonScale)) str = tmp;
	else m_MoonScale = 1.0f;
	if(!(str = AsgnFloat(eee = str, "AxialInclination", &m_AxialInclination))) throw CSynErr(eee);
	m_AxialInclination = D3DXToRadian(m_AxialInclination);
	if(!(str = AsgnFloat(eee = str, "RevolutionPeriod", &m_RevolutionPeriod))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "InitialPhase", &m_InitialPhase))) throw CSynErr(eee);
	m_RevolutionPerDay = 2.0f*D3DX_PI/m_RevolutionPeriod;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	データ読込
 */
void CMoon::LoadData(){
	m_MoonMesh = g_MeshList.Get(FALSE, (char *)m_MoonFile.c_str(), 0, 1);
	m_MoonObject.SetMesh(m_MoonMesh, V3ZERO, m_MoonScale);
}

/*
 *	レンダリング
 */
void CMoon::Render(
	double abstime,	//	絶対時間
	float latitude,	//	緯度
	VEC3 sdir,		//	太陽の方向
	float rot		//	地球の自転角度
){
	float rev = NormAngle((abstime+m_InitialPhase)*m_RevolutionPerDay);
	m_MoonObject.SetDir(-V3UP, V3DIR);
	m_MoonObject.RotX(m_AxialInclination-latitude);
	m_MoonObject.RotY(-rev);
	VEC3 rotaxis;
	V3Norm(&rotaxis, &V3WorldToLocal(
		&VEC3(0.0f, sin(latitude), cos(latitude)), &m_MoonObject));
	m_MoonObject.RotAxis(rotaxis, rot);
	VEC3 mdir = m_MoonObject.GetDir();
	V3Norm(&mdir, &mdir);
	m_MoonObject.SetPos(GetVPos()+10.0f*mdir);
	m_MoonObject.SetDir(-sdir, VEC3(sdir.y, sdir.z, sdir.x));
	m_MoonObject.RenderAP(1.0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ロード
 */
bool CEnvPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	try{
		if(!(str = BeginBlock(eee = str, "EnvInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "Latitude", &m_Latitude))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "EnvMapTexFileName", &m_EnvMapTexFile))) throw CSynErr(eee);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		m_Latitude = D3DXToRadian(m_Latitude);

		if(!(str = BeginBlock(eee = str, "Landscape"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_LandscapeFile))) throw CSynErr(eee);
		if(tmp = AsgnFloat(eee = str, "ModelScale", &m_LandscapeScale)) str = tmp;
		else m_LandscapeScale = 1.0f;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = BeginBlock(eee = str, "Sun"))) throw CSynErr(eee);
		if(!(str = AsgnString(eee = str, "ModelFileName", &m_SunFile))) throw CSynErr(eee);
		if(tmp = AsgnFloat(str, "ModelScale", &m_SunScale)) str = tmp;
		else m_SunScale = 1.0f;
		if(!(str = AsgnFloat(eee = str, "AxialInclination", &m_SunAxialInclination))) throw CSynErr(eee);
		m_SunAxialInclination = D3DXToRadian(m_SunAxialInclination);
		if(tmp = m_SunLensFlare.Read(str)) str = tmp;
		if(tmp = m_SunWhiteout.Read(str)) str = tmp;

		if(!(str = BeginBlock(eee = str, "Lighting"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "NightThreshold", &m_NightThreshold))) throw CSynErr(eee);
		if(!(str = AsgnColor(eee = str, "ShadowColor", &m_ShadowColor))) throw CSynErr(eee);
		m_Light.clear();
		CLightSetting light;
		while(tmp = light.Read(str)){
			str = tmp;
			m_Light.push_back(light);
		}
		m_Light.sort();
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		m_Moon.clear();
		CMoon moon;
		while(tmp = moon.Read(str)){
			str = tmp;
			m_Moon.push_back(moon);
		}

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	if(m_EnvMapTexFile.size())
		m_EnvMapTexture = g_TexList.Get(FALSE, m_EnvMapTexFile.c_str());
	else m_EnvMapTexture = NULL;
	m_LandscapeMesh = g_MeshList.Get(FALSE, (char *)m_LandscapeFile.c_str(), 0, 1);
	m_LandscapeObject.SetMesh(m_LandscapeMesh, V3ZERO, m_LandscapeScale);
	ChDir();
	m_SunMesh = g_MeshList.Get(FALSE, (char *)m_SunFile.c_str(), 0, 1);
	m_SunObject.SetMesh(m_SunMesh, V3ZERO, m_SunScale);
	ChDir();
	m_SunLensFlare.LoadData();
	ChDir();
	IMoon im = m_Moon.begin();
	for(; im!=m_Moon.end(); im++){
		im->LoadData();
		ChDir();
	}
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	プレビュー設定
 */
void CEnvPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Env = this;
	string desc = g_Env->GetBasicInfo();
	desc += "\n"+g_Env->GetDescription();
	g_EnvEditMode->SetProperty((char *)desc.c_str());
}

/*
 *	環境描画
 */
void CEnvPlugin::Render(
	double abstime	//	絶対時間
){
	LoadAndGet();
	if(abstime<0.0) abstime = g_SaveFile->GetAbsTime();
	int rotmode = g_SimulationMode->GetEarthRevolution();
	double rottime = abstime;
	if(rotmode){
		abstime = 365*(rotmode%4)/4-10.0;
		rottime += abstime-(int)rottime;
	}
	//	冬至: 12/22 よって 1/1 から 10 日分ずらす
	float rev = NormAngle((abstime+10.0)*REV_PER_DAY);	//	公転角度
	m_SunObject.SetDir(-V3UP, V3DIR);
	m_SunObject.RotX(m_SunAxialInclination-m_Latitude);
	m_SunObject.RotY(-rev);
	VEC3 rotaxis;	//	自転軸
	float rot;		//	自転角度
	V3Norm(&rotaxis, &V3WorldToLocal(
		&VEC3(0.0f, sin(m_Latitude), cos(m_Latitude)), &m_SunObject));
	switch(g_SimulationMode->GetEarthRotation()){
	case 0: rot = NormAngle(rottime*ROT_PER_DAY); break;
	case 1: rot = rev+0.5f*ROT_PER_DAY; break;
	case 2: rot = rev; break;
	}
	m_SunObject.RotAxis(rotaxis, rot);
	VEC3 sdir = m_SunObject.GetDir();
	V3Norm(&sdir, &sdir);
	g_DayAlpha = (sdir.y-m_NightThreshold)*10.0f;
	ValueArea(&g_DayAlpha, 0.0f, 1.0f);
	g_NightAlpha = 1.0f-g_DayAlpha;
	m_SunObject.SetPos(GetVPos()+10.0f*sdir);
	D3DCOLOR directional, ambient, skycolor;
	float sunalt = sdir.y;
	g_SystemSwitch[SYS_SW_NIGHT].SetValue(g_AncientNightFlag = sunalt<m_NightThreshold);
	if(m_Light.size()){
		ILightSetting il1 = m_Light.begin(), il2 = il1;
		il2++;
		for(; il1!=m_Light.end(); il1 = il2, il2++){
			if(sunalt<=il1->m_SunAlt || il2==m_Light.end()){
				directional = il1->m_Directional;
				ambient = il1->m_Ambient;
				skycolor = il1->m_SkyColor;
				break;
			}else if(sunalt<=il2->m_SunAlt){
				float p1 = (il2->m_SunAlt-sunalt)/(il2->m_SunAlt-il1->m_SunAlt);
				directional = MixColor(il1->m_Directional, il2->m_Directional, p1);
				ambient = MixColor(il1->m_Ambient, il2->m_Ambient, p1);
				skycolor = MixColor(il1->m_SkyColor, il2->m_SkyColor, p1);
				break;
			}
		}
	}else{
		directional = 0xffffffff;
		ambient = 0xff808080;
		skycolor = 0xff000000;
	}
	SetDirLight(-sdir, ACtoCV(directional));
	if(g_HidefCaptureFlag) g_HidefCapture.Begin(skycolor);
	else BeginScene(skycolor);
	devSetState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	devBLEND_ADD2();
	m_SunObject.RenderAP(1.0f);
	devSetAmbient(0xffffffff);
	IMoon im = m_Moon.begin();
	for(; im!=m_Moon.end(); im++) im->Render(abstime, m_Latitude, sdir, rot);
	devSetLighting(TRUE);
	devBLEND_ALPHA();
	devSetAmbient(g_NoLightColor = MaxColor(directional, ambient));
	m_LandscapeObject.SetPos(GetVPos());
	m_LandscapeObject.RenderAmb();
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetAmbient(ambient);
}

/*
 *	環境仕上げ
 */
void CEnvPlugin::RenderAfter(){
	VEC3 vp = GetVPos(), sd = m_SunObject.GetDir();
	float flarealpha = (sd.y-m_NightThreshold)*10.0f;
	ValueArea(&flarealpha, 0.0f, 1.0f);
	if(!flarealpha) return;
	V3Norm(&sd, &sd);
	VEC3 sunpos = vp+sd*100.0f;
	devSetTexture(0, NULL);
	devSetZRead(FALSE);
	devSetZWrite(FALSE);
	devSetLighting(FALSE);
	if(g_ConfigMode->GetSunLensFlare())
		m_SunLensFlare.Render(sunpos, -sd, GetVDir(), flarealpha, -1.0f);
	if(g_ConfigMode->GetSunWhiteout()) m_SunWhiteout.Render(-sd, flarealpha);
	devSetZRead(TRUE);
	devSetZWrite(TRUE);
	devSetLighting(TRUE);
}
