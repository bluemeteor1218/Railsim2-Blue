#include "stdafx.h"
#include "CCamera.h"
#include "CTrainPlugin.h"
#include "CTrainGroupTemplate.h"
#include "CTrain.h"
#include "CTrainEditMode.h"
#include "CSimulationMode.h"
#include "CConfigMode.h"
#include "CSaveFile.h"

//	外部グローバル
extern bool g_MoverEnabled;

//	内部グローバル
CTrainPlugin *g_Train = NULL;		//	車輌プラグイン

/*
 *	読込
 */
char *CObjectJointZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	string obj;
	if(!(str = BeginNamedBlock(eee = str, "JointZY", &obj))) return NULL;
	if(!(m_Link = FindObjectHybrid(mpi, obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(!(str = AsgnVector2D(eee = str, "AttachCoord", &m_AttachCoord))) throw CSynErr(eee);
	if(!(str = AsgnVector2D(eee = str, "LocalCoord", &m_LocalCoord))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CObjectJointZYX::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	string obj;
	if(!(str = BeginNamedBlock(eee = str, "JointZYX", &obj))) return NULL;
	if(!(m_Link = FindObjectHybrid(mpi, obj)))
		throw CSynErr(eee, "%s: \"%s\"", lang(UndefinedObject), obj.c_str());
	if(tmp = AsgnFloat(str, "AttachX", &m_AttachX)) str = tmp;
	else m_AttachX = 0.0f;
	if(!(str = AsgnVector2D(eee = str, "AttachCoord", &m_AttachCoord))) throw CSynErr(eee);
	if(!(str = AsgnVector2D(eee = str, "LocalCoord", &m_LocalCoord))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CAxleObject::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	if(!(str = BeginNamedBlock(eee = str, "Axle", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Diameter", &m_Diameter))) throw CSynErr(eee);
	if(!(str = AsgnInteger(eee = str, "Symmetric", &m_Symmetric))) throw CSynErr(eee);
	if(m_Symmetric<1) m_Symmetric = 1;
	m_MaxRotation = SYMMETRIC_ROTATION_MAX/m_Symmetric;
	if(!(str = AsgnVector2D(eee = str, "Coord", &m_Coord))) throw CSynErr(eee);
	if(tmp = AsgnYesNo(eee = str, "WheelSound", &m_WheelSound)) str = tmp;
	else m_WheelSound = true;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(1);
	return str;
}

/*
 *	プレビュー用姿勢設定
 */
float CAxleObject::CalcRotation(
	float dist	//	進行距離
){
	if(!m_Diameter) return 0.0f;
	float tmp = m_MaxRotation*(1.0f-expf(-fabsf(dist)/(m_MaxRotation*m_Diameter*D3DX_PI)));
	return dist<0.0f ? -tmp : tmp;
}

/*
 *	プレビュー用姿勢設定
 */
void CAxleObject::SetPreview(
	float offset,	//	オフセット
	bool reverse	//	反転
){
	SetMesh(m_LastObject = &m_PreviewObject);
	m_PreviewObject.SetPos(VEC3(0.0f, m_Coord.y, offset+(reverse ? -m_Coord.x : m_Coord.x)));
	m_PreviewObject.SetDir(reverse ? -V3DIR : V3DIR, m_Turn ? -V3UP : V3UP);
}

/*
 *	姿勢設定
 */
void CAxleObject::SetPosture(
	VEC3 pos,	//	座標
	VEC3 dir,	//	dir
	VEC3 up,	//	up
	float rot	//	回転量
){
	CObject *obj = GetPartsObject();
	SetMesh(obj);
	obj->SetPos(pos+m_Coord.y*up);
	obj->SetDir(dir, m_Turn ? -up : up);
	if(m_Diameter) obj->RotX(2.0f*D3DX_PI*rot);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
VEC3 CBodyObject::ms_TiltDir;

/*
 *	読込
 */
char *CBodyObject::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	if(!(str = BeginNamedBlock(eee = str, "Body", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint1.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint2.Read(eee = str, mpi))) throw CSynErr(eee);

	if(tmp = BeginBlock(str, "Tilt")){
		str = tmp;
		if(!(str = AsgnFloat(eee = str, "TiltRatio", &m_TiltRatio))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "MaxAngle", &m_TiltMaxAngle))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "BaseAlt", &m_TiltBaseAlt))) throw CSynErr(eee);
		m_TiltMaxAngle = D3DXToRadian(m_TiltMaxAngle);
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	}else{
		m_TiltMaxAngle = 0.0f;
	}

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(1);
	return str;
}

/*
 *	姿勢設定
 */
void CBodyObject::SetPosture(){
	CObject *obj = GetPartsObject();
	SetMesh(obj);
	VEC2 sdir = m_Joint2.m_LocalCoord-m_Joint1.m_LocalCoord;
	VEC3 j1 = m_Joint1.GetPos(), j2 = m_Joint2.GetPos(), wdir = j2-j1;
	VEC3 right = m_Joint1.GetRight()+m_Joint2.GetRight();
	float slen = V2Len(&sdir), wlen = V3Len(&wdir);
	V3Norm(&wdir, &wdir);
	VEC3 cpos = 0.5f*(j1+j2), w90;
	V3Norm(&w90, V3Cross(&w90, &wdir, &right));
	VEC3 ldir = sdir.x*wdir-(m_Turn ? -sdir.y : sdir.y)*w90, lup;
	V3Cross(&lup, &ldir, &right);
	obj->SetDir(ldir, m_Turn ? -lup : lup);
	VEC3 dif = 0.5f*VEC2toVEC3(m_Joint1.m_LocalCoord+m_Joint2.m_LocalCoord);
	V3Norm(&ldir, &obj->GetDir());
	V3Norm(&lup, &obj->GetUp());
	VEC3 pos = cpos-dif.x*ldir-dif.y*lup;
	if(m_TiltMaxAngle){
		float tilt = m_TiltRatio*V3Dot(&ms_TiltDir, V3Norm(&right, &right));
		float tmp = m_TiltMaxAngle*(1.0f-expf(-fabsf(tilt)/m_TiltMaxAngle));
		tilt = tilt<0.0f ? -tmp : tmp;
		pos += m_TiltBaseAlt*lup;
		obj->RotZ(tilt);
		V3Norm(&lup, &obj->GetUp());
		obj->SetPos(pos-m_TiltBaseAlt*lup);
	}else{
		obj->SetPos(pos);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CFreeObjectContainer::CFreeObjectContainer(){
	m_FreeObject = NULL;
}

/*
 *	コピーコンストラクタ
 */
CFreeObjectContainer::CFreeObjectContainer(
	const CFreeObjectContainer &src	//	コピー元
){
	m_FreeObject = src.m_FreeObject->Duplicate();
}

/*
 *	コピーコンストラクタ
 */
CFreeObjectContainer::CFreeObjectContainer(
	CFreeObject3D *obj3d	//	obj
){
	m_FreeObject = obj3d;
}

/*
 *	デストラクタ
 */
CFreeObjectContainer::~CFreeObjectContainer(){
	DELETE_V(m_FreeObject);
}

/*
 *	読込
 */
char *CFreeObjectContainer::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp;
	DELETE_V(m_FreeObject);
	CFreeObject3D object3d;
	CFreeObjectZY objectzy;
	CFreeTriangleZY trianglezy;
	CFreeCrankZY crankzy;
	CFreePistonZY pistonzy;
	if(tmp = object3d.Read(str, mpi)){
		str = tmp;
		m_FreeObject = new CFreeObject3D(object3d);
	}else if(tmp = objectzy.Read(str, mpi)){
		str = tmp;
		m_FreeObject = new CFreeObjectZY(objectzy);
	}else if(tmp = trianglezy.Read(str, mpi)){
		str = tmp;
		m_FreeObject = new CFreeTriangleZY(trianglezy);
	}else if(tmp = crankzy.Read(str, mpi)){
		str = tmp;
		m_FreeObject = new CFreeCrankZY(crankzy);
	}else if(tmp = pistonzy.Read(str, mpi)){
		str = tmp;
		m_FreeObject = new CFreePistonZY(pistonzy);
	}else{
		str = NULL;
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CFreeObjectZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *tmp, *eee;
	if(!(str = BeginNamedBlock(eee = str, "ObjectZY", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(tmp = AsgnFloat(str, "FixPosition", &m_FixPosition)) str = tmp;
	else m_FixPosition = 0.5f;
	if(tmp = AsgnFloat(str, "FixRight", &m_FixRight)) str = tmp;
	else m_FixRight = m_FixPosition;
	if(!(str = m_Joint1.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint2.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(1);
	return str;
}

/*
 *	姿勢設定
 */
void CFreeObjectZY::SetPostureFreeObject(){
	CObject *obj = GetPartsObject();
	SetMesh(obj);
	VEC2 sdir = m_Joint2.m_LocalCoord-m_Joint1.m_LocalCoord;
	VEC3 j1 = m_Joint1.GetPos(), j2 = m_Joint2.GetPos(), wdir = j2-j1;
	VEC3 right = (1.0f-m_FixRight)*m_Joint1.GetRight()
		+m_FixRight*m_Joint2.GetRight(), w90;
	float slen = V2Len(&sdir), wlen = V3Len(&wdir);
	V3Norm(&wdir, &wdir);
	V3Norm(&right, &right);
	VEC3 cpos = (1.0f-m_FixPosition)*j1+m_FixPosition*j2;
	V3Norm(&w90, V3Cross(&w90, &wdir, &right));
	VEC3 ldir = sdir.x*wdir-(m_Turn ? -sdir.y : sdir.y)*w90, lup;
	V3Cross(&lup, &ldir, &right);
	obj->SetDir(ldir, m_Turn ? -lup : lup);
	VEC3 dif = VEC2toVEC3((1.0f-m_FixPosition)*m_Joint1.m_LocalCoord
		+m_FixPosition*m_Joint2.m_LocalCoord);
	V3Norm(&ldir, &obj->GetDir());
	V3Norm(&lup, &obj->GetUp());
	obj->SetPos(cpos-dif.x*ldir-dif.y*lup);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CTriangleLinkZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginNamedBlock(eee = str, "Link", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = AsgnVector2D(eee = str, "LinkCoord", &m_LinkCoord))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CFreeTriangleZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginBlock(str, "TriangleZY"))) return NULL;
	if(!(str = m_Link1.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Link2.Read(eee = str, mpi))) throw CSynErr(eee);
	if(m_Link1.Check(m_Link2.GetName()))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedObjectName), m_Link2.GetName());
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(2);
	return str;
}

/*
 *	読込
 */
void CFreeTriangleZY::SetPostureFreeObject(){
	CObject *obj1 = m_Link1.GetPartsObject();
	CObject *obj2 = m_Link2.GetPartsObject();
	m_Link1.SetMesh(obj1);
	m_Link2.SetMesh(obj2);
	VEC2 &lc1 = m_Link1.m_Joint.m_LocalCoord, sdir1 = m_Link1.m_LinkCoord-lc1;
	VEC2 &lc2 = m_Link2.m_Joint.m_LocalCoord, sdir2 = m_Link2.m_LinkCoord-lc2;
	VEC3 j1 = m_Link1.m_Joint.GetPos();
	VEC3 j2 = m_Link2.m_Joint.GetPos(), wdir = j2-j1;
	VEC3 right = m_Link1.m_Joint.GetRight()+m_Link2.m_Joint.GetRight(), w90;
	float slen1 = V2Len(&sdir1), slen2 = V2Len(&sdir2), wlen = V3Len(&wdir);
	V3Norm(&wdir, &wdir);
	V3Norm(&right, &right);
	V3Norm(&w90, V3Cross(&w90, &wdir, &right));
	float px = (wlen*wlen+slen1*slen1-slen2*slen2)*0.5f/wlen;
	float py2 = slen1*slen1-px*px, py = py2<0.0f ? 0.5f*wlen : -sqrtf(py2);
	VEC3 ppos = j1+px*wdir+py*w90, w11, w12, w21, w22;
	V3Norm(&w12, V3Cross(&w12, V3Norm(&w11, &(ppos-j1)), &right));
	V3Norm(&w22, V3Cross(&w22, V3Norm(&w21, &(ppos-j2)), &right));
	VEC3 ldir1 = sdir1.x*w11-(m_Link1.m_Turn ? -sdir1.y : sdir1.y)*w12, lup1;
	VEC3 ldir2 = sdir2.x*w21-(m_Link2.m_Turn ? -sdir2.y : sdir2.y)*w22, lup2;
	V3Cross(&lup1, &ldir1, &right);
	V3Cross(&lup2, &ldir2, &right);
	obj1->SetDir(ldir1, m_Link1.m_Turn ? -lup1 : lup1);
	obj2->SetDir(ldir2, m_Link2.m_Turn ? -lup2 : lup2);
	V3Norm(&ldir1, &obj1->GetDir());
	V3Norm(&ldir2, &obj2->GetDir());
	V3Norm(&lup1, &obj1->GetUp());
	V3Norm(&lup2, &obj2->GetUp());
	obj1->SetPos(j1-lc1.x*ldir1-lc1.y*lup1);
	obj2->SetPos(j2-lc2.x*ldir2-lc2.y*lup2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CCrankSlideZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginNamedBlock(eee = str, "Slide", &m_ObjectName))) return NULL;
	CheckObjectHybrid(eee, mpi, m_ObjectName);
	if(!(str = ReadModelInfo(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Joint.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = AsgnVector2D(eee = str, "Direction", &m_Direction))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CFreeCrankZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginBlock(str, "CrankZY"))) return NULL;
	if(!(str = m_Link.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Slide.Read(eee = str, mpi))) throw CSynErr(eee);
	if(m_Link.Check(m_Slide.GetName()))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedObjectName), m_Slide.GetName());
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(2);
	return str;
}

/*
 *	読込
 */
void CFreeCrankZY::SetPostureFreeObject(){
	CObject *obj1 = m_Link.GetPartsObject();
	CObject *obj2 = m_Slide.GetPartsObject();
	m_Link.SetMesh(obj1);
	m_Slide.SetMesh(obj2);
	VEC3 sldir = m_Slide.m_Joint.GetDir(), slup = m_Slide.m_Joint.GetUp();
	VEC3 direction = m_Slide.m_Direction.x*sldir+m_Slide.m_Direction.y*slup;
	V3Norm(&direction, &direction);
	VEC2 &lc1 = m_Link.m_Joint.m_LocalCoord, sdir1 = m_Link.m_LinkCoord-lc1;
	VEC2 &lc2 = m_Slide.m_Joint.m_LocalCoord;
	VEC3 j1 = m_Link.m_Joint.GetPos();
	VEC3 j2 = m_Slide.m_Joint.GetPos(), wdir = j2-j1;
	VEC3 right = m_Link.m_Joint.GetRight()+m_Slide.m_Joint.GetRight(), w90;
	float lldcos = V3Dot(&wdir, &direction), lldsin = V3Len(&(wdir-lldcos*direction));
	float slen1 = V2Len(&sdir1), llofs = slen1*slen1-lldsin*lldsin;
	VEC3 lljpos = j2+((llofs<0.0f ? 0.0f : sqrtf(llofs))-lldcos)*direction;
	wdir = lljpos-j1;
	float wlen = V3Len(&wdir);
	V3Norm(&wdir, &wdir);
	V3Norm(&right, &right);
	V3Norm(&w90, V3Cross(&w90, &wdir, &right));
	VEC3 ldir1 = sdir1.x*wdir-(m_Link.m_Turn ? -sdir1.y : sdir1.y)*w90, lup1;
	VEC3 lup2 = -m_Slide.m_Direction.y*sldir+m_Slide.m_Direction.x*slup;
	V3Cross(&lup1, &ldir1, &right);
	obj1->SetDir(ldir1, m_Link.m_Turn ? -lup1 : lup1);
	obj2->SetDir(direction, m_Slide.m_Turn ? -lup2 : lup2);
	V3Norm(&ldir1, &obj1->GetDir());
	V3Norm(&lup1, &obj1->GetUp());
	obj1->SetPos(j1-lc1.x*ldir1-lc1.y*lup1);
	obj2->SetPos(lljpos);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CFreePistonZY::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	モデルプラグイン
){
	char *eee;
	if(!(str = BeginBlock(str, "PistonZY"))) return NULL;
	if(!(str = m_Link1.Read(eee = str, mpi))) throw CSynErr(eee);
	if(!(str = m_Link2.Read(eee = str, mpi))) throw CSynErr(eee);
	if(m_Link1.Check(m_Link2.GetName()))
		throw CSynErr(eee, "%s: \"%s\"", lang(OverlappedObjectName), m_Link2.GetName());
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	mpi->AddPartsNum(2);
	return str;
}

/*
 *	読込
 */
void CFreePistonZY::SetPostureFreeObject(){
	CObject *obj1 = m_Link1.GetPartsObject();
	CObject *obj2 = m_Link2.GetPartsObject();
	m_Link1.SetMesh(obj1);
	m_Link2.SetMesh(obj2);
	VEC2 &lc1 = m_Link1.m_Joint.m_LocalCoord, sdir1 = m_Link1.m_LinkCoord-lc1;
	VEC2 &lc2 = m_Link2.m_Joint.m_LocalCoord, sdir2 = m_Link2.m_LinkCoord-lc2;
	VEC3 j1 = m_Link1.m_Joint.GetPos();
	VEC3 j2 = m_Link2.m_Joint.GetPos(), wdir = j2-j1;
	VEC3 right = m_Link1.m_Joint.GetRight()+m_Link2.m_Joint.GetRight(), w90;
	float slen1 = V2Len(&sdir1), slen2 = V2Len(&sdir2), wlen = V3Len(&wdir);
	V3Norm(&wdir, &wdir);
	V3Norm(&right, &right);
	V3Norm(&w90, V3Cross(&w90, &wdir, &right));
	VEC3 ppos = 0.5f*(j1+j2), w11, w12, w21, w22;
	V3Norm(&w12, V3Cross(&w12, V3Norm(&w11, &(ppos-j1)), &right));
	V3Norm(&w22, V3Cross(&w22, V3Norm(&w21, &(ppos-j2)), &right));
	VEC3 ldir1 = sdir1.x*w11-(m_Link1.m_Turn ? -sdir1.y : sdir1.y)*w12, lup1;
	VEC3 ldir2 = sdir2.x*w21-(m_Link2.m_Turn ? -sdir2.y : sdir2.y)*w22, lup2;
	V3Cross(&lup1, &ldir1, &right);
	V3Cross(&lup2, &ldir2, &right);
	obj1->SetDir(ldir1, m_Link1.m_Turn ? -lup1 : lup1);
	obj2->SetDir(ldir2, m_Link2.m_Turn ? -lup2 : lup2);
	V3Norm(&ldir1, &obj1->GetDir());
	V3Norm(&ldir2, &obj2->GetDir());
	V3Norm(&lup1, &obj1->GetUp());
	V3Norm(&lup2, &obj2->GetUp());
	obj1->SetPos(j1-lc1.x*ldir1-lc1.y*lup1);
	obj2->SetPos(j2-lc2.x*ldir2-lc2.y*lup2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	[static]
 *	プレビュー
 */
void CTrainPlugin::RenderPreview(){
	CNamedObjectAfterRenderer::SetCurrentInst(NULL);
	g_SaveFile->ResetSwitch();
	if(ms_PreviewState && g_Train) g_Train->Preview(0.0f, false);
}

/*
 *	デストラクタ
 */
CTrainPlugin::~CTrainPlugin(){
}

/*
 *	ロード
 */
bool CTrainPlugin::Load(){
	char *str = m_Script, *tmp, *eee;
	if(!ChDir() || !m_Script) return false;
	g_NamedObjectMipMap = g_TrainMipMap;
	CNamedObject::SetCastShadowDefault(true);
	try{
		if(!(str = BeginBlock(eee = str, "TrainInfo"))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "FrontLimit", &m_FrontLimit))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "TailLimit", &m_TailLimit))) throw CSynErr(eee);
		if((m_Length = m_FrontLimit-m_TailLimit)<0.0f) throw CSynErr(eee, lang(InvalidTrainLength));
		if(!(str = AsgnFloat(eee = str, "MaxVelocity", &m_MaxVelocity))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "MaxAcceleration", &m_MaxAcceleration))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "MaxDeceleration", &m_MaxDeceleration))) throw CSynErr(eee);
		if(tmp = AsgnFloat(str, "TiltSpeed", &m_TiltSpeed)) str = tmp;
		else m_TiltSpeed = 1.0;
		if(tmp = AsgnFloat(str, "DoorClosingTime", &m_DoorClosingTime)) str = tmp;
		else m_DoorClosingTime = 0.0;
		//	[km/h/sec] to [km/h/frame]
		m_MaxAcceleration /= MAXFPS;
		m_MaxDeceleration /= MAXFPS;
		if(m_MaxVelocity<0.0f) m_MaxVelocity = 0.0f;
		if(m_MaxAcceleration<0.0f) m_MaxVelocity = 0.0f;
		if(m_MaxDeceleration<0.0f) m_MaxDeceleration = 0.0f;
		ValueArea(&m_TiltSpeed, 0.0f, 1.0f);
		if(m_DoorClosingTime<0.0f) m_DoorClosingTime = 0.0f;
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(!(str = ReadModelSwitch(eee = str))) throw CSynErr(eee);

		if(!(str = BeginBlock(eee = str, "PrimaryAssembly"))) throw CSynErr(eee);
		CAxleObject axleobj;
		while(tmp = axleobj.Read(str, this)){
			str = tmp;
			m_AxleObject.push_back(axleobj);
		}
		m_AxleObject.sort();
		CBodyObject bodyobj;
		while(tmp = bodyobj.Read(str, this)){
			str = tmp;
			m_BodyObject.push_back(bodyobj);
		}
		CFreeObjectContainer freeobj;
		while(tmp = freeobj.Read(str, this)){
			str = tmp;
			m_FreeObject.push_back(freeobj);
		}
		if(!(str = ReadEffect(eee = str))) throw CSynErr(eee);
		if(tmp = BeginBlock(eee = str, "FrontCabin")){
			str = tmp;
			if(!(str = m_FrontCabin.Read(eee = str, this))) throw CSynErr(eee);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}
		if(tmp = BeginBlock(eee = str, "TailCabin")){
			str = tmp;
			if(!(str = m_TailCabin.Read(eee = str, this))) throw CSynErr(eee);
			if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
		}
		if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

		if(*(eee = str)) throw CSynErr(eee);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) iao->LoadModel(this);
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) ibo->LoadModel(this);
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->LoadModel(this);
	LoadData();
	DELETE_A(m_Buffer);
	return true;
}

/*
 *	ロード
 */
bool CTrainPlugin::LoadOldForm(){
	if(!ChDir()) return false;
	g_NamedObjectMipMap = g_StructMipMap;
	CNamedObject::SetCastShadowDefault(true);
	FILE *file = fopen(TextName(), "rt");
	char *dummy = FlashOut();
	float sc, itv, len, spd;
	fscanf(file, "%s %s %f %f %f %f", dummy, dummy, &sc, &itv, &len, &spd);
	fclose(file);
	float oldscale = 2.0f/sc;
	itv *= oldscale;
	len *= oldscale;
	m_FrontLimit = len*0.5f+0.3f;
	m_TailLimit = -m_FrontLimit;
	m_Length = m_FrontLimit-m_TailLimit;
	m_MaxVelocity = spd*10.0f;
	m_MaxAcceleration = 3.0f/MAXFPS;
	m_MaxDeceleration = 4.0f/MAXFPS;
	m_TiltSpeed = 1.0f;
	m_DoorClosingTime = 0.0f;
	if(m_MaxVelocity<0.0f) m_MaxVelocity = 0.0f;
	CAxleObject axle;
	axle.m_ObjectName = "Wheel1";
	axle.m_ModelFileName = "";
	axle.m_ModelScale = 1.0f;
	axle.m_Turn = false;
	axle.m_Diameter = 1.0f;
	axle.m_Symmetric = 1;
	axle.m_MaxRotation = 1.0f;
	axle.m_Coord = VEC2(0.5f*itv, 0.0f);
	axle.m_WheelSound = false;
	m_AxleObject.push_back(axle);
	axle.m_ObjectName = "Wheel2";
	axle.m_Coord = VEC2(-0.5f*itv, 0.0f);
	m_AxleObject.push_back(axle);
	CBodyObject body;
	body.m_ObjectName = "MainBody";
	body.m_ModelFileName = "Model.x";
	body.m_ModelScale = oldscale;
	body.m_Turn = true;
	body.m_TiltMaxAngle = 0.0f;
	body.m_Joint1.m_AttachCoord = V2ZERO;
	body.m_Joint1.m_LocalCoord = VEC2(-0.5f*itv, 0.0f);
	body.m_Joint1.m_Link = FindObject("Wheel1");
	body.m_Joint2.m_AttachCoord = V2ZERO;
	body.m_Joint2.m_LocalCoord = VEC2(0.5f*itv, 0.0f);
	body.m_Joint2.m_Link = FindObject("Wheel2");
	m_BodyObject.push_back(body);
	m_BodyObject.begin()->LoadModel(this);
	m_FrontCabin.m_LocalCoord = V3ZERO;
	m_FrontCabin.m_AttachCoord = VEC3(0.0f, 2.5f, -m_FrontLimit);
	m_FrontCabin.m_AttachDir = -V3DIR;
	m_FrontCabin.m_AttachUp = V3UP;
	m_FrontCabin.m_Link = &*m_BodyObject.begin();
	m_FrontCabin.m_DirLink = &*m_BodyObject.begin();
	m_FrontCabin.m_UpLink = &*m_BodyObject.begin();
	m_TailCabin.m_LocalCoord = V3ZERO;
	m_TailCabin.m_AttachCoord = VEC3(0.0f, 2.5f, -m_TailLimit);
	m_TailCabin.m_AttachDir = V3DIR;
	m_TailCabin.m_AttachUp = V3UP;
	m_TailCabin.m_Link = &*m_BodyObject.begin();
	m_TailCabin.m_DirLink = &*m_BodyObject.begin();
	m_TailCabin.m_UpLink = &*m_BodyObject.begin();
	m_PartsNum = 3;
	return true;
}

/*
 *	プレビュー設定
 */
void CTrainPlugin::SetPreview(){
	ms_PreviewState = true;
	g_Train = this;
	g_TrainGroupTemplate = NULL;
	string desc = g_Train->GetBasicInfo();
	desc += FlashIn("\n%s: %.3f [m]\n%s: %.1f [km/h]",
		lang(EntireLength), m_Length, lang(MaxVelocity), m_MaxVelocity);
	desc += "\n"+g_Train->GetDescription();
	g_TrainEditMode->SetProperty((char *)desc.c_str());
}

/*
 *	プレビュー
 */
void CTrainPlugin::Preview(
	float offset,	//	オフセット
	bool reverse	//	反転
){
	CBodyObject::SetTiltDir(R2L(V3ZERO));
	SetPartsInst(NULL);
	SetMoverState(NULL);
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) iao->SetPreview(offset, reverse);
	SetPosture();
	Render(NULL);
}

/*
 *	オブジェクト検索
 */
CNamedObject *CTrainPlugin::FindObject(
	const string &name	//	オブジェクト名
){
	CNamedObject *ret;
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) if(ret = iao->Check(name)) return ret;
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) if(ret = ibo->Check(name)) return ret;
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) if(ret = ifo->Check(name)) return ret;
	return NULL;
}

/*
 *	サウンド有効かどうか
 */
bool CTrainPlugin::IsSoundEnabled(){
	return !!g_ConfigMode->GetTrainSound();
}

/*
 *	車軸リスト設定
 */
void CTrainPlugin::SetAxleList(
	CTrain *train	//	車輌
){
	int i = 0;
	IAxleObject ia = m_AxleObject.begin();
	for(; ia!=m_AxleObject.end(); ia++, i++) train->PushAxle(R2L(CAxlePosture(
		&*ia, train, (!i || i==m_AxleObject.size()-1) && m_AxleObject.size()>1)));
}

/*
 *	姿勢設定
 */
void CTrainPlugin::SetPosture(){
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) ibo->SetPosture();
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->SetPosture();
}

/*
 *	パーツインスタンスのアタッチ
 */
void CTrainPlugin::AttachPartsObject(){
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) iao->GetPartsObject();
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) ibo->GetPartsObject();
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->AttachPartsObject();
}

/*
 *	入力チェック
 */
void CTrainPlugin::ScanInput(
	CTrain *train	//	車輌インスタンス
){
	train->SetLocalAxis();
	SetCamDistSwitch(train->GetPos());
	g_PreSimulationFlag = false;
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) iao->CheckDetect();
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) ibo->CheckDetect();
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->ScanInput();
}

/*
 *	レンダリング
 */
void CTrainPlugin::Render(
	CTrain *train	//	車輌インスタンス
){
	SetAnimation(train);
	if(train){
		train->SetLocalAxis();
		SetCamDistSwitch(train->GetPos());
	}else{
		VEC3 pvpos = m_AxleObject.size()
			? 0.5f*(m_AxleObject.begin()->GetObject()->GetPos()
			+(--m_AxleObject.end())->GetObject()->GetPos()) : V3ZERO;
		g_SystemObject[SYS_OBJ_LOCAL].SetPreviewPosture(pvpos, V3DIR, V3UP);
		SetCamDistSwitch(pvpos);
	}
	g_PreSimulationFlag = false;
	CNamedObject::SetSetMaterial(m_Version>=2.00f);
	if(train && !g_SimulationMode->GetSimSpeed()){
		g_MoverEnabled = false;
		train->ApplyAxle(false);
		SetMoverState(train);
		SetPosture();
		SetPartsInst(train);
		g_MoverEnabled = true;
	}
	IAxleObject iao = m_AxleObject.begin();
	for(; iao!=m_AxleObject.end(); iao++) iao->Render();
	IBodyObject ibo = m_BodyObject.begin();
	for(; ibo!=m_BodyObject.end(); ibo++) ibo->Render();
	IFreeObjectContainer ifo = m_FreeObject.begin();
	for(; ifo!=m_FreeObject.end(); ifo++) ifo->Render();
	SimulateEffect(train);
}

/*
 *	シミュレーション進行
 */
void CTrainPlugin::Simulate(
	CTrain *train	//	車輌インスタンス
){
	SetMoverState(train);
	train->SetLocalAxis();
	SetCamDistSwitch(train->GetPos());
	g_PreSimulationFlag = true;
	SetPosture();
	SimulateEffect(train);
}
