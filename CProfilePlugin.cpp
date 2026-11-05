#include "stdafx.h"
#include "CShadowVolume.h"
#include "CCamera.h"
#include "CProfilePlugin.h"

//	外部グローバル
extern CScene *g_Scene;
extern bool g_RailMipMap;
extern CShadowVolume g_ShadowVolume;

/*
 *	断面中間点座標の計算
 */
inline VEC3 CalcMidProfile(
	VEC3 &p1, VEC3 &r1, VEC3 &u1,	//	境界 1
	VEC3 &p2, VEC3 &r2, VEC3 &u2,	//	境界 2
	VEC3 &mid						//	中間点
){
	return (1.0f-mid.z)*(p1+r1*mid.x+u1*mid.y)+mid.z*(p2+r2*mid.x+u2*mid.y);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CProfileVertex::Read(
	char *str,	//	対象文字列
	bool tex	//	テクスチャ
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Vertex"))) return NULL;
	if(tmp = AsgnYesNo(str, "IgnoreCant", &m_IgnoreCant)) str = tmp;
	else m_IgnoreCant = false;
	if(!(str = AsgnVector2D(eee = str, "Coord", &m_Coord))) throw CSynErr(eee);
	if(tmp = AsgnVector2D(str, "Normal", &m_Normal)){
		str = tmp;
		m_ReadNormal = true;
	}else{
		m_ReadNormal = false;
	}
	V2Norm(&m_Normal, &m_Normal);
	if(tmp = AsgnColor(str, "Diffuse", &m_Diffuse)) str = tmp;
	else m_Diffuse = 0xffffffff;
	if(tex){
		if(!(str = AsgnFloat(eee = str, "TexU", &m_TexU))) throw CSynErr(eee);
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CProfileFace::Read(
	char *str,	//	対象文字列
	bool tex	//	テクスチャ
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "Face"))) return NULL;

	m_Vertex.clear();
	CProfileVertex vertex;
	while(tmp = vertex.Read(str, tex)){
		str = tmp;
		m_Vertex.push_back(vertex);
	}
	IProfileVertex iv = m_Vertex.begin(), prev, next = iv;
	for(; iv!=m_Vertex.end(); iv++){
		next++;
		if(!iv->m_ReadNormal){
			if(iv==m_Vertex.begin()){
				VEC2 tmp = next->m_Coord-iv->m_Coord;
				V2Norm(&iv->m_Normal, &VEC2(-tmp.y, tmp.x));
			}else if(next==m_Vertex.end()){
				VEC2 tmp = iv->m_Coord-prev->m_Coord;
				V2Norm(&iv->m_Normal, &VEC2(-tmp.y, tmp.x));
			}else{
				VEC2 tmp1 = iv->m_Coord-prev->m_Coord;
				VEC2 tmp2 = next->m_Coord-iv->m_Coord;
				V2Norm(&tmp2, &VEC2(-tmp2.y, tmp2.x));
				V2Norm(&tmp1, &VEC2(-tmp1.y, tmp1.x));
				V2Norm(&iv->m_Normal, &(tmp1+tmp2));
			}
		}
		prev = iv;
	}
	if(m_Vertex.size()<2) throw CSynErr(eee, lang(TwoVertexNeeded));

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CProfile::CProfile(){
	m_Texture = NULL;
}

/*
 *	デストラクタ
 */
CProfile::~CProfile(){
	ClearAll();
}

/*
 *	すべて解放
 */
void CProfile::ClearAll(){
	map<CScene *, MapPtrValue<CQuadDumpN> >::iterator itr_n;
	for(itr_n = m_DumpN.begin(); itr_n!=m_DumpN.end(); ++itr_n){
		itr_n->second.SafeDelete();
	}
	map<CScene *, MapPtrValue<CQuadDumpNX> >::iterator itr_nx;
	for(itr_nx = m_DumpNX.begin(); itr_nx!=m_DumpNX.end(); ++itr_nx){
		itr_nx->second.SafeDelete();
	}
}

/*
 *	読込
 */
char *CProfile::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Profile"))) return NULL;

	if(!(str = BeginBlock(eee = str, "Material"))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "UseTexture", &m_UseTexture))) throw CSynErr(eee);
	if(m_UseTexture){
		if(!(str = AsgnString(eee = str, "TexFileName", &m_TexFileName))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "TexVPerMeter", &m_TexVPerMeter))) throw CSynErr(eee);
	}else{
		m_TexFileName = "";
		m_TexVPerMeter = 1.0f;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);

	m_Face.clear();
	CProfileFace face;
	while(tmp = face.Read(str, m_UseTexture)){
		str = tmp;
		m_Face.push_back(face);
	}

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	テクスチャ読込
 */
void CProfile::LoadTexture(){
	if(m_UseTexture) m_Texture = g_TexList.Get(FALSE, m_TexFileName.c_str(), 0, !g_RailMipMap);
}

/*
 *	ダンパ準備
 */
void CProfile::PrepareDump(){
	if(m_UseTexture){
		MapPtrValue<CQuadDumpNX> &dump_nx = m_DumpNX[g_Scene];
		if(!dump_nx) dump_nx = new CQuadDumpNX(QUAD_DUMP_MAX, m_Texture);
	}else{
		MapPtrValue<CQuadDumpN> &dump_n = m_DumpN[g_Scene];
		if(!dump_n) dump_n = new CQuadDumpN(QUAD_DUMP_MAX);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/*
 *	読込
 */
char *CWireframeVertex::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Vertex"))) return NULL;
	if(tmp = AsgnYesNo(str, "IgnoreCant", &m_IgnoreCant)) str = tmp;
	else m_IgnoreCant = false;
	if(!(str = AsgnVector3D(eee = str, "Coord", &m_Coord))) throw CSynErr(eee);
	if(tmp = AsgnColor(str, "Diffuse", &m_Diffuse)) str = tmp;
	else m_Diffuse = 0xff000000;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CWireframeLine::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "Line"))) return NULL;

	m_Vertex.clear();
	CWireframeVertex vertex;
	while(tmp = vertex.Read(str)){
		str = tmp;
		m_Vertex.push_back(vertex);
	}
	if(m_Vertex.size()<2) throw CSynErr(eee, lang(TwoVertexNeeded));

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CWireframe::CWireframe(){
}

/*
 *	デストラクタ
 */
CWireframe::~CWireframe(){
	ClearAll();
}

/*
 *	すべて解放
 */
void CWireframe::ClearAll(){
	map<CScene *, MapPtrValue<CLineDumpN> >::iterator itr_n;
	for(itr_n = m_DumpN.begin(); itr_n!=m_DumpN.end(); ++itr_n){
		itr_n->second.SafeDelete();
	}
}

/*
 *	読込
 */
char *CWireframe::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Wireframe"))) return NULL;

	if(tmp = AsgnFloat(str, "MinInterval", &m_MinInterval)) str = tmp;
	else m_MinInterval = -1.0f;
	if(tmp = AsgnFloat(str, "MaxInterval", &m_MaxInterval)) str = tmp;
	else m_MaxInterval = -1.0f;

	m_Line.clear();
	CWireframeLine line;
	while(tmp = line.Read(str)){
		str = tmp;
		m_Line.push_back(line);
	}

	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	ダンパ準備
 */
void CWireframe::PrepareDump(){
	MapPtrValue<CLineDumpN> &dump_n = m_DumpN[g_Scene];
	if(!dump_n) dump_n = new CLineDumpN(LINE_DUMP_MAX);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CInterval::Read(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	if(!(str = BeginBlock(str, "Interval"))) return NULL;
	if(tmp = AsgnYesNo(str, "IgnoreCant", &m_IgnoreCant)) str = tmp;
	else m_IgnoreCant = false;
	if(!(str = AsgnString(eee = str, "ModelFileName", &m_ModelFileName))) throw CSynErr(eee);
	if(tmp = AsgnFloat(str, "ModelScale", &m_ModelScale)) str = tmp;
	else m_ModelScale = 1.0f;
	if(!(str = AsgnFloat(eee = str, "Interval", &m_Interval))) throw CSynErr(eee);
	if(tmp = AsgnFloat(str, "Offset", &m_Offset)) str = tmp;
	else m_Offset = 0.0f;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	モデル読込
 */
void CInterval::LoadModel(){
	m_Mesh = g_MeshList.Get(FALSE, (char *)m_ModelFileName.c_str(), 0, !g_RailMipMap);
	m_Object.SetMesh(m_Mesh, V3ZERO, m_ModelScale);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
vector<CProfileVertex *>CProfilePlugin::ms_TempIndex;

/*
 *	読込
 */
char *CProfilePlugin::ReadProfile(
	char *str	//	対象文字列
){
	char *tmp;
	CProfile profile;
	CWireframe wireframe;
	CInterval interval;
	while(true){
		if(tmp = profile.Read(str)){
			str = tmp;
			m_Profile.push_back(profile);
		}else if(tmp = wireframe.Read(str)){
			str = tmp;
			m_Wireframe.push_back(wireframe);
		}else if(tmp = interval.Read(str)){
			str = tmp;
			m_Interval.push_back(interval);
		}else{
			break;
		}
	}
	return str;
}

/*
 *	データ読込
 */
void CProfilePlugin::LoadData(){
	IProfile ip = m_Profile.begin();
	IInterval ii = m_Interval.begin();
	for(; ip!=m_Profile.end(); ip++) ip->LoadTexture();
	for(; ii!=m_Interval.end(); ii++){
		ii->LoadModel();
		ChDir();
	}
}

/*
 *	ダンプ
 */
void CProfilePlugin::Dump(
	VEC3 &p1, VEC3 &r1, VEC3 &u1,		//	始点 (正規化済)
	VEC3 &ip1, VEC3 &ir1, VEC3 &iu1,	//	非カント始点 (正規化済)
	VEC3 &p2, VEC3 &r2, VEC3 &u2,		//	終点 (正規化済)
	VEC3 &ip2, VEC3 &ir2, VEC3 &iu2,	//	非カント終点 (正規化済)
	float len,	//	テクスチャ延長長さ
	int prev	//	プレビュー (1: render, 2: settex, 4: mapscroll)
){
	void (CLineDumpN::*f_ll)(VEC3, D3DCOLOR, VEC3, D3DCOLOR);
	void (CQuadDumpN::*f_qn)(
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR,
		VEC3, VEC3, D3DCOLOR, VEC3, VEC3, D3DCOLOR);
	void (CQuadDumpNX::*f_qnx)(
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float,
		VEC3, VEC3, D3DCOLOR, float, float, VEC3, VEC3, D3DCOLOR, float, float);
	if(prev&1){
		f_ll = &CLineDumpN::Preview;
		f_qn = &CQuadDumpN::Preview;
		f_qnx = &CQuadDumpNX::Preview;
	}else{
		f_ll = &CLineDumpN::Add;
		f_qn = &CQuadDumpN::Add;
		f_qnx = &CQuadDumpNX::Add;
	}
	BeforeDump(p1, r1, u1, p2, r2, u2);
	IProfile ip = m_Profile.begin();
	if(prev&2){
		devSetLighting(TRUE);
		devResetMaterial();
	}
	for(; ip!=m_Profile.end(); ip++){
		MapPtrValue<CQuadDumpNX> &dump_nx = ip->m_DumpNX[g_Scene];
		MapPtrValue<CQuadDumpN> &dump_n = ip->m_DumpN[g_Scene];
		float v1 = ip->m_TexMapVTemp, v2 = v1+ip->m_TexVPerMeter*len;
		if(prev&2) devSetTexture(0, ip->m_Texture);
		else ip->PrepareDump();
		IProfileFace ir = ip->m_Face.begin();
		for(; ir!=ip->m_Face.end(); ir++){
			IProfileVertex iv1 = ir->m_Vertex.begin(), iv2 = iv1;
			iv2++;
			for(; iv2!=ir->m_Vertex.end(); iv1++, iv2++){
				VEC2 &c1 = iv1->m_Coord, &c2 = iv2->m_Coord;
				VEC2 &n1 = iv1->m_Normal, &n2 = iv2->m_Normal;
				D3DCOLOR &df1 = iv1->m_Diffuse, &df2 = iv2->m_Diffuse;
				if(ip->m_UseTexture){
					float &tu1 = iv1->m_TexU, &tu2 = iv2->m_TexU;
					if(iv1->m_IgnoreCant){
						if(iv2->m_IgnoreCant) (dump_nx->*f_qnx)(
							ip1+ir1*c1.x+iu1*c1.y, ir1*n1.x+iu1*n1.y, df1, tu1, v1,
							ip2+ir2*c1.x+iu2*c1.y, ir2*n1.x+iu2*n1.y, df1, tu1, v2,
							ip2+ir2*c2.x+iu2*c2.y, ir2*n2.x+iu2*n2.y, df2, tu2, v2,
							ip1+ir1*c2.x+iu1*c2.y, ir1*n2.x+iu1*n2.y, df2, tu2, v1);
						else (dump_nx->*f_qnx)(
							ip1+ir1*c1.x+iu1*c1.y, ir1*n1.x+iu1*n1.y, df1, tu1, v1,
							ip2+ir2*c1.x+iu2*c1.y, ir2*n1.x+iu2*n1.y, df1, tu1, v2,
							p2+r2*c2.x+u2*c2.y, r2*n2.x+u2*n2.y, df2, tu2, v2,
							p1+r1*c2.x+u1*c2.y, r1*n2.x+u1*n2.y, df2, tu2, v1);
					}else{
						if(iv2->m_IgnoreCant) (dump_nx->*f_qnx)(
							p1+r1*c1.x+u1*c1.y, r1*n1.x+u1*n1.y, df1, tu1, v1,
							p2+r2*c1.x+u2*c1.y, r2*n1.x+u2*n1.y, df1, tu1, v2,
							ip2+ir2*c2.x+iu2*c2.y, ir2*n2.x+iu2*n2.y, df2, tu2, v2,
							ip1+ir1*c2.x+iu1*c2.y, ir1*n2.x+iu1*n2.y, df2, tu2, v1);
						else (dump_nx->*f_qnx)(
							p1+r1*c1.x+u1*c1.y, r1*n1.x+u1*n1.y, df1, tu1, v1,
							p2+r2*c1.x+u2*c1.y, r2*n1.x+u2*n1.y, df1, tu1, v2,
							p2+r2*c2.x+u2*c2.y, r2*n2.x+u2*n2.y, df2, tu2, v2,
							p1+r1*c2.x+u1*c2.y, r1*n2.x+u1*n2.y, df2, tu2, v1);
					}
				}else{
					if(iv1->m_IgnoreCant){
						if(iv2->m_IgnoreCant) (dump_n->*f_qn)(
							ip1+ir1*c1.x+iu1*c1.y, ir1*n1.x+iu1*n1.y, df1,
							ip2+ir2*c1.x+iu2*c1.y, ir2*n1.x+iu2*n1.y, df1,
							ip2+ir2*c2.x+iu2*c2.y, ir2*n2.x+iu2*n2.y, df2,
							ip1+ir1*c2.x+iu1*c2.y, ir1*n2.x+iu1*n2.y, df2);
						else (dump_n->*f_qn)(
							ip1+ir1*c1.x+iu1*c1.y, ir1*n1.x+iu1*n1.y, df1,
							ip2+ir2*c1.x+iu2*c1.y, ir2*n1.x+iu2*n1.y, df1,
							p2+r2*c2.x+u2*c2.y, r2*n2.x+u2*n2.y, df2,
							p1+r1*c2.x+u1*c2.y, r1*n2.x+u1*n2.y, df2);
					}else{
						if(iv2->m_IgnoreCant) (dump_n->*f_qn)(
							p1+r1*c1.x+u1*c1.y, r1*n1.x+u1*n1.y, df1,
							p2+r2*c1.x+u2*c1.y, r2*n1.x+u2*n1.y, df1,
							ip2+ir2*c2.x+iu2*c2.y, ir2*n2.x+iu2*n2.y, df2,
							ip1+ir1*c2.x+iu1*c2.y, ir1*n2.x+iu1*n2.y, df2);
						else (dump_n->*f_qn)(
							p1+r1*c1.x+u1*c1.y, r1*n1.x+u1*n1.y, df1,
							p2+r2*c1.x+u2*c1.y, r2*n1.x+u2*n1.y, df1,
							p2+r2*c2.x+u2*c2.y, r2*n2.x+u2*n2.y, df2,
							p1+r1*c2.x+u1*c2.y, r1*n2.x+u1*n2.y, df2);
					}
				}
			}
		}
		if(prev&4) ip->m_TexMapVTemp = v2-(int)v2;
	}
	if(prev&2){
		devSetTexture(0, NULL);
		devSetLineMaterial();
	}
	IWireframe iw = m_Wireframe.begin();
	for(; iw!=m_Wireframe.end(); iw++){
		if(!iw->CheckInterval(len)) continue;
		MapPtrValue<CLineDumpN> &dump_n = iw->m_DumpN[g_Scene];
		if(!(prev&2)) iw->PrepareDump();
		IWireframeLine ir = iw->m_Line.begin();
		for(; ir!=iw->m_Line.end(); ir++){
			IWireframeVertex iv1 = ir->m_Vertex.begin(), iv2 = iv1;
			iv2++;
			for(; iv2!=ir->m_Vertex.end(); iv1++, iv2++){
				VEC3 &c1 = iv1->m_Coord, &c2 = iv2->m_Coord;
				D3DCOLOR &df1 = iv1->m_Diffuse, &df2 = iv2->m_Diffuse;
				if(iv1->m_IgnoreCant){
					if(iv2->m_IgnoreCant) (dump_n->*f_ll)(
						CalcMidProfile(ip1, ir1, iu1, ip2, ir2, iu2, c1), df1,
						CalcMidProfile(ip1, ir1, iu1, ip2, ir2, iu2, c2), df2);
					else (dump_n->*f_ll)(
						CalcMidProfile(ip1, ir1, iu1, ip2, ir2, iu2, c1), df1,
						CalcMidProfile(p1, r1, u1, p2, r2, u2, c2), df2);
				}else{
					if(iv2->m_IgnoreCant) (dump_n->*f_ll)(
						CalcMidProfile(p1, r1, u1, p2, r2, u2, c1), df1,
						CalcMidProfile(ip1, ir1, iu1, ip2, ir2, iu2, c2), df2);
					else (dump_n->*f_ll)(
						CalcMidProfile(p1, r1, u1, p2, r2, u2, c1), df1,
						CalcMidProfile(p1, r1, u1, p2, r2, u2, c2), df2);
				}
			}
		}
	}
	AfterDump(p1, u1, ip1, iu1, p2, u2, ip2, iu2);
}

/*
 *	レンダリング
 */
void CProfilePlugin::Render(
	VEC3 &p1, VEC3 &r1, VEC3 &u1, VEC3 &d1,	//	始点 (正規化済)
	VEC3 &ip1, VEC3 &ir1, VEC3 &iu1,		//	非カント始点 (正規化済)
	VEC3 &p2, VEC3 &r2, VEC3 &u2, VEC3 &d2,	//	終点 (正規化済)
	VEC3 &ip2, VEC3 &ir2, VEC3 &iu2,		//	非カント終点 (正規化済)
	int close,		//	閉鎖 (1: begin, 2: end)
	float len,		//	テクスチャ延長長さ
	MAT8 *altmat	//	代替マテリアル
){
	float sx1, sy1, sx2, sy2;
	float taperz = GetTaperZ(), sz = 1.0;
	bool ftaper = UseTaper(), close1 = !!(close&1), close2 = !!(close&2);
	if(ftaper){
		sx1 = V3Len(&r1); sy1 = V3Len(&u1);
		sx2 = V3Len(&r2); sy2 = V3Len(&u2);
		if(taperz<0.0f) sz -= len*taperz;
	}
	BeforeDump(p1, r1, u1, p2, r2, u2);
	VEC3 dir = p2-p1, idir = ip2-ip1, td1, td2;
	V3Norm(&dir, &dir);
	V3Norm(&idir, &idir);
	V3Norm(&td1, V3Cross(&td1, &r1, &u1));
	V3Norm(&td2, V3Cross(&td2, &r2, &u2));
	if(g_ShadowNeeded){
		ms_TempIndex.clear();
		VEC3 vLight = svl.dir.Direction;
		V3Norm(&vLight, &vLight);
		IProfile ip = m_Profile.begin();
		for(; ip!=m_Profile.end(); ip++){
			IProfileFace ir = ip->m_Face.begin();
			for(; ir!=ip->m_Face.end(); ir++){
				IProfileVertex iv1 = ir->m_Vertex.begin(), iv2 = iv1;
				iv2++;
				for(; iv2!=ir->m_Vertex.end(); iv1++, iv2++){
					VEC2 &c1 = iv1->m_Coord, &c2 = iv2->m_Coord;
					VEC3 sp1, sp2, sp3, sp4;
					if(iv1->m_IgnoreCant){
						sp1 = ip1+ir1*c1.x+iu1*c1.y;
						sp2 = ip2+ir2*c1.x+iu2*c1.y;
					}else{
						sp1 = p1+r1*c1.x+u1*c1.y;
						sp2 = p2+r2*c1.x+u2*c1.y;
					}
					if(iv2->m_IgnoreCant){
						sp3 = ip2+ir2*c2.x+iu2*c2.y;
						sp4 = ip1+ir1*c2.x+iu1*c2.y;
					}else{
						sp3 = p2+r2*c2.x+u2*c2.y;
						sp4 = p1+r1*c2.x+u1*c2.y;
					}
					VEC3 vNormal;
					V3Cross(&vNormal, &(sp1-sp2), &(sp4-sp2));
					if(V3Dot(&vNormal, &vLight)>=0.0f){
						iv1->m_TransCoord[0] = sp1;
						iv1->m_TransCoord[1] = sp2;
						iv2->m_TransCoord[0] = sp3;
						iv2->m_TransCoord[1] = sp4;
						ms_TempIndex.push_back(&*iv1);
						ms_TempIndex.push_back(&*iv2);
						if(close1 || !iv2->m_ShadowDrawed)
							g_ShadowVolume.AddFaceEdge(sp4, sp1, vLight);
						if(close2) g_ShadowVolume.AddFaceEdge(sp2, sp3, vLight);
						iv2->m_ShadowDrawed = true;
					}else{
						if(!close1 && iv2->m_ShadowDrawed)
							g_ShadowVolume.AddFaceEdge(sp1, sp4, vLight);
						iv2->m_ShadowDrawed = false;
					}
				}
			}
		}
		sort(ms_TempIndex.begin(), ms_TempIndex.end());
		DWORD i, dwNumEdges = ms_TempIndex.size();
		for(i = 0; i<dwNumEdges; i++){
			CProfileVertex *edge = ms_TempIndex[i];
			while(i<dwNumEdges-1 && edge->IsSame(ms_TempIndex[i+1])){
				i += 2;
				while(i<dwNumEdges && edge->IsSame(ms_TempIndex[i])) i++;
				if(i>=dwNumEdges) goto ENDDUMP;
				edge = ms_TempIndex[i];
			}
			g_ShadowVolume.AddFaceEdge(edge->m_TransCoord[0], edge->m_TransCoord[1], vLight);
		}
ENDDUMP:;
	}
	IInterval ii = m_Interval.begin();
	for(; ii!=m_Interval.end(); ii++){
		float v1 = ii->m_IntervalTemp;
		while(v1<len){
			if(v1>=0.0f){
				float q2 = v1/len, q1 = 1.0f-q2;
				if(ii->m_IgnoreCant){
					ii->m_Object.SetPos(ip1+idir*v1);
					ii->m_Object.SetDir(q1*d1+q2*d2, q1*iu1+q2*iu2);
				}else{
					ii->m_Object.SetPos(p1+dir*v1);
					ii->m_Object.SetDir(q1*td1+q2*td2, q1*u1+q2*u2);
				}
				if(ftaper) ii->m_Object.SetScale(q1*sx1+q2*sx2, q1*sy1+q2*sy2, sz);
				ii->m_Object.ResetMatFlag();
				if(altmat) ii->m_Object.RenderSC(altmat); else ii->m_Object.Render();
				CastShadow(&ii->m_Object);
			}
			v1 += ii->m_Interval*sz;
			sz += ii->m_Interval*sz*taperz;
		}
		ii->m_IntervalTemp = v1-len;
	}
	AfterDump(p1, u1, ip1, iu1, p2, u2, ip2, iu2);
}

/*
 *	マッピング位置リセット
 */
void CProfilePlugin::ResetMapTemp(){
	IProfile ip = m_Profile.begin();
	IInterval ii = m_Interval.begin();
	for(; ip!=m_Profile.end(); ip++) ip->m_TexMapVTemp = 0.0f;
	for(; ii!=m_Interval.end(); ii++) ii->m_IntervalTemp = ii->m_Offset;
}

/*
 *	マッピング位置コピー
 */
void CProfilePlugin::CopyMapTemp(
	vector<float> &mapv	//	代入先
){
	int pn = m_Profile.size()+m_Interval.size();
	if(!pn) return;
	mapv.resize(pn);
	vector<float>::iterator ptr = mapv.begin();
	IProfile ip = m_Profile.begin();
	IInterval ii = m_Interval.begin();
	for(; ip!=m_Profile.end(); ip++, ptr++) *ptr = ip->m_TexMapVTemp;
	for(; ii!=m_Interval.end(); ii++, ptr++) *ptr = ii->m_IntervalTemp;
}

/*
 *	マッピング位置加算
 */
void CProfilePlugin::AddMapTemp(
	float dist	//	距離
){
	IProfile ip = m_Profile.begin();
	IInterval ii = m_Interval.begin();
	for(; ip!=m_Profile.end(); ip++){
		ip->m_TexMapVTemp += dist*ip->m_TexVPerMeter;
		ip->m_TexMapVTemp -= (int)ip->m_TexMapVTemp;
	}
	for(; ii!=m_Interval.end(); ii++){
		ii->m_IntervalTemp += ii->m_Interval*(int)(dist/ii->m_Interval)-dist;
		if(ii->m_IntervalTemp<0.0f) ii->m_IntervalTemp += ii->m_Interval;
	}
}

/*
 *	マッピング位置セット
 */
void CProfilePlugin::SetMapTemp(
	vector<float> &mapv	//	現在位置
){
	vector<float>::iterator ptr = mapv.begin();
	IProfile ip = m_Profile.begin();
	IInterval ii = m_Interval.begin();
	for(; ip!=m_Profile.end(); ip++, ptr++) ip->m_TexMapVTemp = *ptr;
	for(; ii!=m_Interval.end(); ii++, ptr++) ii->m_IntervalTemp = *ptr;
}

/*
 *	ダンパ解放
 */
void CProfilePlugin::ClearDump(){
	IProfile ip = m_Profile.begin();
	IWireframe iw = m_Wireframe.begin();
	for(; ip!=m_Profile.end(); ip++){
		ip->m_DumpN[g_Scene].SafeDelete();
		ip->m_DumpNX[g_Scene].SafeDelete();
	}
	for(; iw!=m_Wireframe.end(); iw++) iw->m_DumpN[g_Scene].SafeDelete();
}

/*
 *	ダンパすべて解放
 */
void CProfilePlugin::ClearDumpAll(){
	IProfile ip = m_Profile.begin();
	IWireframe iw = m_Wireframe.begin();
	for(; ip!=m_Profile.end(); ip++) ip->ClearAll();
	for(; iw!=m_Wireframe.end(); iw++) iw->ClearAll();
}

/*
 *	バーテックス準備
 */
void CProfilePlugin::PrepareVertex(){
	IProfile ip = m_Profile.begin();
	IWireframe iw = m_Wireframe.begin();
	for(; ip!=m_Profile.end(); ip++){
		MapPtrValue<CQuadDumpN> &dump_n = ip->m_DumpN[g_Scene];
		MapPtrValue<CQuadDumpNX> &dump_nx = ip->m_DumpNX[g_Scene];
		if(dump_n) dump_n->PrepareVertex();
		if(dump_nx) dump_nx->PrepareVertex();
	}
	for(; iw!=m_Wireframe.end(); iw++){
		MapPtrValue<CLineDumpN> &dump_n = iw->m_DumpN[g_Scene];
		if(dump_n) dump_n->PrepareVertex();
	}
}

/*
 *	全てレンダリング
 */
void CProfilePlugin::RenderAll(){
	IProfile ip = m_Profile.begin();
	IWireframe iw = m_Wireframe.begin();
	devSetLighting(TRUE);
	devResetMaterial();
	for(; ip!=m_Profile.end(); ip++){
		MapPtrValue<CQuadDumpN> &dump_n = ip->m_DumpN[g_Scene];
		MapPtrValue<CQuadDumpNX> &dump_nx = ip->m_DumpNX[g_Scene];
		if(dump_n) dump_n->Render(false);
		if(dump_nx) dump_nx->Render(false);
	}
	devSetLineMaterial();
	for(; iw!=m_Wireframe.end(); iw++){
		MapPtrValue<CLineDumpN> &dump_n = iw->m_DumpN[g_Scene];
		if(dump_n) dump_n->Render(false);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	ダンパ解放
 */
void CProfilePluginList::ClearDump(){
	CProfilePlugin *ptr = Root();
	while(ptr){
		ptr->ClearDump();
		ptr = ptr->Next();
	}
}

/*
 *	ダンパすべて解放
 */
void CProfilePluginList::ClearDumpAll(){
	CProfilePlugin *ptr = Root();
	while(ptr){
		ptr->ClearDumpAll();
		ptr = ptr->Next();
	}
}

/*
 *	ダンパ準備
 */
void CProfilePluginList::PrepareVertex(){
	CProfilePlugin *ptr = Root();
	while(ptr){
		ptr->PrepareVertex();
		ptr = ptr->Next();
	}
}

/*
 *	全てレンダリング
 */
void CProfilePluginList::RenderAll(){
	devResetMatrix();
	devResetMaterial();
	CProfilePlugin *ptr = Root();
	while(ptr){
		ptr->RenderAll();
		ptr = ptr->Next();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	マッピングベクトル読込
 */
char *ReadMapVector(
	char *str,			//	対象文字列
	char *pref,			//	プレフィックス
	vector<float> &mapv	//	マッピングベクトル
){
	char *eee, *tmp;
	if(tmp = Assignment(str, pref)){
		str = tmp;
		do{
			if(mapv.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
			float texv;
			if(!(str = ConstFloat(eee = str, &texv))) throw CSynErr(eee);
			mapv.push_back(texv);
		} while(!(tmp = Character2(str, ';')));
		str = tmp;
	}
	return str;
}

/*
 *	マッピングベクトル保存
 */
void SaveMapVector(
	FILE *df,			//	ファイル
	char *pref,			//	プレフィックス
	vector<float> &mapv	//	マッピングベクトル
){
	if(!mapv.size()) return;
	fprintf(df, pref);
	int i;
	for(i = 0; i<mapv.size(); i++) fprintf(df, i ? ", %f" : "%f", mapv[i]);
	fprintf(df, ";\n");
}
