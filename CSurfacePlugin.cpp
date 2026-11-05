#include "stdafx.h"
#include "CCamera.h"
#include "CSurfacePlugin.h"
#include "CConfigMode.h"
#include "CSceneEditMode.h"
#include "CSaveFile.h"

//	static メンバ
CObject CSurfacePlugin::ms_PreviewObject;

/*
 *	[static]
 *	プレビュー
 */
void CSurfacePlugin::RenderPreview(){
	CNamedObjectAfterRenderer::SetCurrentInst(NULL);
	g_SaveFile->ResetSwitch();
	if(ms_PreviewState && g_Surface){
		g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(V3ZERO, V3DIR, V3UP);
		SetCamDistSwitch(V3ZERO);
		g_Surface->Preview();
	}
}

/*
 *	デストラクタ
 */
CSurfacePlugin::~CSurfacePlugin(){
}

/*
 *	ロード
 */
char *CSurfacePlugin::LoadStructBefore(
	char *str	//	対象文字列
){
	g_NamedObjectMipMap = g_SurfaceMipMap;
	CNamedObject::SetCastShadowDefault(false);
	char *eee;
	if(!(str = BeginBlock(eee = str, "SurfaceInfo"))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "SizeX", &m_SizeX))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "SizeZ", &m_SizeZ))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	ロード
 */
bool CSurfacePlugin::LoadOldForm(){
	if(!ChDir()) return false;
	g_NamedObjectMipMap = g_SurfaceMipMap;
	CNamedObject::SetCastShadowDefault(false);
	FILE *file = fopen(TextName(), "rt");
	char *dummy = FlashOut();
	float sc, sizex, sizez;
	fscanf(file, "%s %s %f %f %f %f", dummy, dummy, &sc, &sizex, &sizez);
	fclose(file);
	float oldscale = 2.0f/sc;
	m_SizeX = sizex*oldscale;
	m_SizeZ = sizez*oldscale;
	m_FreeObject.push_back(CFreeObjectContainer(new CFreeObject3D("MainObject", "Model.x", oldscale)));
	m_FreeObject.begin()->LoadModel(this);
	m_PartsNum = 1;
	return true;
}

/*
 *	プレビュー設定
 */
void CSurfacePlugin::SetPreview(){
	ms_PreviewState = true;
	g_Surface = this;
	string desc = g_Surface->GetBasicInfo();
	desc += FlashIn("\n%s: %.1f%s%.1f [m]", lang(Size), m_SizeX, lang(MulOp), m_SizeZ);
	desc += "\n"+g_Surface->GetDescription();
	g_SceneEditMode->SetProperty((char *)desc.c_str());
}

/*
 *	サウンド有効かどうか
 */
bool CSurfacePlugin::IsSoundEnabled(){
	return !!g_ConfigMode->GetSurfaceSound();
}

/*
 *	地形ピック
 */
bool CSurfacePlugin::PickSurface(
	VEC3 pos,	//	座標
	VEC3 dir,	//	方向
	VEC3 *hit,	//	ヒット座標格納先
	VEC3 *tri,	//	三角形頂点格納先
	int inv		//	裏面フラグ
){
	VEC3 hit2, tri2[3];
	IFreeObjectContainer ifo = m_FreeObject.begin();
	int i, j;
	float mindist;
	bool flag = false;
	int ccc = 0;
	for(; ifo!=m_FreeObject.end(); ifo++){
		ccc++;
		for(j = 0; j<ifo->GetNamedObjectNum(); ++j){
			if(ifo->GetNamedObject(j)->GetPartsObject()->Pick(pos, dir, &hit2, tri2, inv)){
				float tmpdist = V3Len(&(pos-hit2));
				if(!flag || mindist>tmpdist){
					mindist = tmpdist;
					if(hit) *hit = hit2;
					if(tri) for(i = 0; i<3; i++) tri[i] = tri2[i];
				}
				flag = true;
			}
		}
	}
	return flag;
}

/*
 *	矩形範囲に収める
 */
bool CSurfacePlugin::ClipRect(
	VEC3 *pos	//	対象座標
){
	float hsx = 0.5f*m_SizeX, hsz = 0.5f*m_SizeZ;
	bool ret = true;
	if(pos->x<-hsx){ pos->x = -hsx; ret = false; }
	else if(pos->x>hsx){ pos->x = hsx; ret = false; }
	if(pos->z<-hsz){ pos->z = -hsz; ret = false; }
	else if(pos->z>hsz){ pos->z = hsz; ret = false; }
	return ret;
}
