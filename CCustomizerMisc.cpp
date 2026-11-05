#include "stdafx.h"
#include "CCustomizerMisc.h"
#include "CNamedObject.h"
#include "CEnvPlugin.h"
#include "CConfigMode.h"

/*
 *	コンストラクタ
 */
CModelChanger::CModelChanger(){
	m_AltModel = NULL;
	m_AltModelScale = -1.0f;
}

/*
 *	読込
 */
char *CModelChanger::Read(
	char *str	//	対象文字列
){
	char *eee;
	m_AltModel = NULL;
	if(!(str = Assignment(str, "ChangeModel"))) return NULL;
	if(!(str = StringLiteral(eee = str, &m_AltModelName))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = ConstFloat(eee = str, &m_AltModelScale))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	if(m_AltModelScale<0.0f) m_AltModelScale = 0.0f;
	return str;
}

/*
 *	データ読込
 */
void CModelChanger::LoadDataCustomizer(
	CModelPlugin *mpi	//	呼び出し元
){
	m_AltModel = g_MeshList.Get(FALSE, m_AltModelName.c_str(), 0, !g_NamedObjectMipMap);
	mpi->ChDir();
}

/*
 *	変更子適用
 */
CMesh *CModelChanger::GetMeshCustomizer(
	float *asc	//	スケール格納先
){
	*asc = m_AltModelScale;
	return m_AltModel;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CShadowInhibitor::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	int id;
	m_NoCastShadow = m_NoReceiveShadow = m_Transparent = false;
	if(tmp = Assignment(str, "NoCastShadow")){
		str = tmp;
		m_NoCastShadow = true;
	}else if(tmp = Assignment(str, "NoReceiveShadow")){
		str = tmp;
		m_NoReceiveShadow = true;
	}else if(tmp = Assignment(str, "NoShadow")){
		str = tmp;
		m_NoCastShadow = m_NoReceiveShadow = true;
	}else if(tmp = Assignment(str, "Transparent")){
		str = tmp;
		m_NoCastShadow = m_NoReceiveShadow = m_Transparent = true;
	}else{
		return NULL;
	}
	m_MaterialID = vector<int>();
	if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
	m_MaterialID.push_back(id);
	while(tmp = Character2(str, ',')){
		str = tmp;
		if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
		m_MaterialID.push_back(id);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	変更子適用
 */
void CShadowInhibitor::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	int i, mn = m_MaterialID.size();
	switch(CNamedObject::GetAfterRender()){
	case 0:
		if(m_Transparent){
			CNamedObject::SetRenderAfter(2);
		}else{
			if(!g_ShadowNeeded) return;
			if(m_NoReceiveShadow) CNamedObject::SetRenderAfter(1);
		}
		for(i = 0; i<mn; i++){
			int tmp = m_MaterialID[i];
			if(!mesh->CheckMatNum(tmp)) continue;
			if(m_NoCastShadow) mesh->SetMatFlag(tmp, 1);
			if(m_NoReceiveShadow) mesh->SetMatFlag(tmp, 2);
		}
		break;
	case 1:
		if(!g_ShadowNeeded) return;
		if(m_NoReceiveShadow && !m_Transparent)
			for(i = 0; i<mn; i++){
				int tmp = m_MaterialID[i];
				if(!mesh->CheckMatNum(tmp)) continue;
				mesh->SetMatFlag(tmp, 4);
			}
		break;
	case 2:
		if(m_Transparent){
			for(i = 0; i<mn; i++){
				int tmp = m_MaterialID[i];
				if(!mesh->CheckMatNum(tmp)) continue;
				mesh->SetMatFlag(tmp, 4);
			}
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CEnvMapper::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	int id;
	if(!(str = Assignment(str, "EnvMap"))) return NULL;
	m_MaterialID = vector<int>();
	if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
	m_MaterialID.push_back(id);
	while(tmp = Character2(str, ',')){
		str = tmp;
		if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
		m_MaterialID.push_back(id);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	変更子適用
 */
void CEnvMapper::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!g_ConfigMode->GetEnvMap()) return;
	int i, mn = m_MaterialID.size();
	for(i = 0; i<mn; i++){
		int tmp = m_MaterialID[i];
		if(!mesh->CheckMatNum(tmp)) continue;
		mesh->SetMatFlag(tmp, 8);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CAlphaTester::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	int id;
	if(!(str = Assignment(str, "AlphaZeroTest"))) return NULL;
	m_MaterialID = vector<int>();
	if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
	m_MaterialID.push_back(id);
	while(tmp = Character2(str, ',')){
		str = tmp;
		if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
		m_MaterialID.push_back(id);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	変更子適用
 */
void CAlphaTester::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	int i, mn = m_MaterialID.size();
	for(i = 0; i<mn; i++){
		int tmp = m_MaterialID[i];
		if(!mesh->CheckMatNum(tmp)) continue;
		mesh->SetMatFlag(tmp, 32);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CTextureChanger::Read(
	char *str	//	対象文字列
){
	char *eee;
	m_AltTexture = NULL;
	if(!(str = Assignment(str, "ChangeTexture"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_MaterialID))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = StringLiteral(eee = str, &m_AltTextureName))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	データ読込
 */
void CTextureChanger::LoadDataCustomizer(
	CModelPlugin *mpi	//	呼び出し元
){
	m_AltTexture = g_TexList.Get(FALSE, m_AltTextureName.c_str(), 0, !g_NamedObjectMipMap);
}

/*
 *	変更子適用
 */
void CTextureChanger::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!mesh->CheckMatNum(m_MaterialID)) return;
	mesh->SetCustomTexture(m_MaterialID, m_AltTexture);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CTextureTransformer::Read(
	char *str,	//	対象文字列
	bool matid	//	マテリアル番号読込フラグ
){
	char *tmp, *eee;
	int i, num, type;
	float a[6];
	if(tmp = Assignment(str, "ShiftTexture")){
		type = 1; num = 2;
	}else if(tmp = Assignment(str, "ScaleTexture")){
		type = 2; num = 4;
	}else if(tmp = Assignment(str, "RotateTexture")){
		type = 3; num = 3;
	}else if(tmp = Assignment(str, "TransformTexture")){
		type = 4; num = 6;
	}else{
		return NULL;
	}
	str = tmp;
	if(matid){
		if(!(str = ConstInteger(eee = str, &m_MaterialID))) throw CSynErr(eee);
	}else{
		m_MaterialID = -1;
	}
	for(i = 0; i<num; i++){
		if((matid || i) && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
		if(!(str = ConstFloat(eee = str, &a[i]))) throw CSynErr(eee);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	SetUpMatrix(type, a);
	return str;
}

/*
 *	変換行列セットアップ
 */
void CTextureTransformer::SetUpMatrix(
	int type,	//	タイプ
	float *a	//	パラメタ配列
){
	int i;
	switch(m_TransformType = type){
	case 1: m_Matrix = TTMTX(1.0f, 0.0f, a[0], 0.0f, 1.0f, a[1]); break;
	case 2: m_Matrix = TTMTX(a[0], 0.0f, a[2], 0.0f, a[1], a[3]); break;
	case 3: {
		float rad = D3DXToRadian(a[0]), c = cosf(rad), s = sinf(rad);
		m_Matrix = TTMTX(c, -s, (1.0f-c)*a[1]+s*a[2], s, c, -s*a[1]+(1.0f-c)*a[2]);
		break; }
	case 4: for(i = 0; i<6; i++) m_Matrix.a[i] = a[i]; break;
	}
}

/*
 *	変更子適用
 */
void CTextureTransformer::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!mesh->CheckMatNum(m_MaterialID)) return;
	mesh->SetTexTrans(m_MaterialID, m_Matrix);
	mesh->SetMatFlag(m_MaterialID, 16);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CAnimationApplier::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *eee;
	string name;
	m_Animation = NULL;
	if(!(str = Assignment(str, "SetAnimation"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_MaterialID))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = StringLiteral(eee = str, &name))) throw CSynErr(eee);
	if(!(m_Animation = mpi->FindAnimation(name)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedAnimation), name.c_str());
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	変更子適用
 */
void CAnimationApplier::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!mesh->CheckMatNum(m_MaterialID)) return;
	m_Animation->Apply(mesh, m_MaterialID);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CAlphaChanger::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	string special;
	if(!(str = Assignment(str, "ChangeAlpha"))) return NULL;
	if(!(str = ConstInteger(eee = str, &m_MaterialID))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(tmp = ConstFloat(eee = str, &m_AltAlpha)){
		str = tmp;
		if(m_AltAlpha<0.0f) m_AltAlpha = 0.0f;
	}else if(tmp = Identifier(eee = str, &special)){
		str = tmp;
		if(special=="DayAlpha") m_AltAlpha = -1.0f;
		else if(special=="NightAlpha") m_AltAlpha = -2.0f;
		else throw CSynErr(eee, "%s: \"%s\"", lang(InvalidAlphaValue), special.c_str());
	}else{
		throw CSynErr(eee);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	変更子適用
 */
void CAlphaChanger::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	if(!mesh->CheckMatNum(m_MaterialID)) return;
	float altalpha = m_AltAlpha;
	MAT8 mat = mesh->GetDefaultMaterial(m_MaterialID);
	if(m_AltAlpha==-1.0f) mat.Diffuse.a = g_DayAlpha;
	else if(m_AltAlpha==-2.0f) mat.Diffuse.a = g_NightAlpha;
	else mat.Diffuse.a = m_AltAlpha;
	mesh->SetCustomMaterial(m_MaterialID, mat);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CMaterialChanger::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	string special;
	if(!(str = BeginBlock(str, "ChangeMaterial"))) return NULL;
	if(!(str = Assignment(str, "MaterialID"))) return NULL;
	int id;
	m_MaterialID = vector<int>();
	if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
	m_MaterialID.push_back(id);
	while(tmp = Character2(str, ',')){
		str = tmp;
		if(!(str = ConstInteger(eee = str, &id))) throw CSynErr(eee);
		m_MaterialID.push_back(id);
	}
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	ZeroMemory(&m_Material, sizeof(MAT8));
	m_Material.Diffuse.a = m_Material.Ambient.a =
		m_Material.Specular.a = m_Material.Emissive.a = 1.0f;
	if(tmp = AsgnFloat(str, "Diffuse", (float *)&m_Material.Diffuse, 4, false)) str = tmp;
	else m_Material.Diffuse.a = -1.0f;
	if(tmp = AsgnFloat(str, "Ambient", (float *)&m_Material.Ambient, 3, false)) str = tmp;
	else m_Material.Ambient.a = -1.0f;
	if(tmp = AsgnFloat(str, "Specular", (float *)&m_Material.Specular, 3, false)) str = tmp;
	else m_Material.Specular.a = -1.0f;
	if(tmp = AsgnFloat(str, "Emissive", (float *)&m_Material.Emissive, 3, false)) str = tmp;
	else m_Material.Emissive.a = -1.0f;
	if(tmp = AsgnFloat(str, "Power", &m_Material.Power)) str = tmp;
	else m_Material.Power = -1.0f;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	変更子適用
 */
void CMaterialChanger::ApplyCustomizer(
	CMesh *mesh	//	メッシュ
){
	int i, mn = m_MaterialID.size();
	for(i = 0; i<mn; i++){
		int tmp = m_MaterialID[i];
		if(!mesh->CheckMatNum(tmp)) continue;
		MAT8 mat = mesh->GetDefaultMaterial(tmp);
		if(m_Material.Diffuse.a>=0.0f) mat.Diffuse = m_Material.Diffuse;
		if(m_Material.Ambient.a>=0.0f) mat.Ambient = m_Material.Ambient;
		if(m_Material.Specular.a>=0.0f) mat.Specular = m_Material.Specular;
		if(m_Material.Emissive.a>=0.0f) mat.Emissive = m_Material.Emissive;
		if(m_Material.Power>=0.0f) mat.Power = m_Material.Power;
		mesh->SetCustomMaterial(tmp, mat);
	}
}
