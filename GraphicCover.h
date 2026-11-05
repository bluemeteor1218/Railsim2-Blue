#ifndef GRAPHICCOVER_H_INCLUDED
#define GRAPHICCOVER_H_INCLUDED

class CLineDumpL;

void NormalizeMatrix(MTX4 *);
bool LineLineNearest(VEC3 *, VEC3 *, VEC3 *, VEC3 *, VEC3 *, VEC3 *);
float LineLineDistance(VEC3 *, VEC3 *, VEC3 *, VEC3 *);
void LinePointPosition(VEC3 *, VEC3 *, VEC3 *, VEC3 *, VEC3 *, float);

void InitGrid();
void DrawGrid(VEC3);
void DrawTangent(VEC3, VEC3, D3DCOLOR, CLineDumpL *);
void DrawFocus(VEC3);
void Draw3DLineWithShadow(VEC3, VEC3, D3DCOLOR, D3DCOLOR c2 = 0);
void Draw3DPointAs2DRect(VEC3, D3DCOLOR, int);

void InitShadow();
void CastShadow(CObject *);
void RenderShadow();

/*
 *	角度正規化 (0～2PI におさめる)
 */
inline float NormAngle(double a){
	return (float)(a-2.0f*D3DX_PI*ceil(a/(2.0*D3DX_PI)));
}

/*
 *	視界前方水平ベクトル
 */
inline VEC3 GetVFow(){
	VEC3 fow = GetVDir();
	fow.y = 0.0f;
	V3Norm(&fow, &fow);
	return fow;
}

/*
 *	ベクトル正規直交化 (dir → up → right の順で保存)
 */
inline void V3NormAxis(VEC3 *r, VEC3 *u, VEC3 *d){
	V3Norm(r, V3Cross(r, u, V3Norm(d, d)));
	V3Norm(u, V3Cross(u, d, r));
}

/*
 *	ベクトルをローカル系に変換
 */
inline VEC3 V3WorldToLocal(VEC3 *v, VEC3 *r, VEC3 *u, VEC3 *d){
	return VEC3(V3Dot(v, r), V3Dot(v, u), V3Dot(v, d));
}
inline VEC3 V3WorldToLocal(VEC3 *v, CObject *obj){
	return VEC3(V3Dot(v, &obj->GetRight()),
		V3Dot(v, &obj->GetUp()), V3Dot(v, &obj->GetDir()));
}

/*
 *	ベクトルをワールド系に変換
 */
inline VEC3 V3LocalToWorld(VEC3 *v, VEC3 *r, VEC3 *u, VEC3 *d){
	return *r*v->x+*u*v->y+*d*v->z;
}
inline VEC3 V3LocalToWorld(VEC3 *v, CObject *obj){
	return obj->GetRight()*v->x+obj->GetUp()*v->y+obj->GetDir()*v->z;
}

/*
 *	ベクトル成分大小分離
 */
inline void V3MinMax(VEC3 a, VEC3 b, VEC3 *min, VEC3 *max){
	if(a.x>b.x){ min->x = b.x; max->x = a.x; }else{ min->x = a.x; max->x = b.x; }
	if(a.y>b.y){ min->y = b.y; max->y = a.y; }else{ min->y = a.y; max->y = b.y; }
	if(a.z>b.z){ min->z = b.z; max->z = a.z; }else{ min->z = a.z; max->z = b.z; }
}

inline VEC3 VEC2toVEC3( VEC2 vec )
{
	return VEC3( vec.x, vec.y, 0.0f );
}

/*
 *	D3DCOLOR 制御
 */
inline void SplitXC(
	DWORD c,				//	分解する色
	int *r, int *g, int *b	//	代入先
){
	*r = (c&0x00ff0000)>>16;
	*g = (c&0x0000ff00)>>8;
	*b = c&0x000000ff;
}
inline void SplitAC(
	DWORD c,						//	分解する色
	int *a, int *r, int *g, int *b	//	代入先
){
	*a = (c&0xff000000)>>24;
	*r = (c&0x00ff0000)>>16;
	*g = (c&0x0000ff00)>>8;
	*b = c&0x000000ff;
}
inline D3DCOLOR MaxColor(D3DCOLOR c1, D3DCOLOR c2){
	int a1, r1, g1, b1, a2, r2, g2, b2;
	SplitAC(c1, &a1, &r1, &g1, &b1);
	SplitAC(c2, &a2, &r2, &g2, &b2);
	return MAKE_AC(a1>a2 ? a1 : a2, r1>r2 ? r1 : r2, g1>g2 ? g1 : g2, b1>b2 ? b1 : b2);
}
inline D3DCOLOR MultiplyColor(D3DCOLOR c1, D3DCOLOR c2){
	int a1, r1, g1, b1, a2, r2, g2, b2;
	SplitAC(c1, &a1, &r1, &g1, &b1);
	SplitAC(c2, &a2, &r2, &g2, &b2);
	return MAKE_AC(a1*a2/255, r1*r2/255, g1*g2/255, b1*b2/255);
}
inline D3DCOLOR MixColor(D3DCOLOR c1, D3DCOLOR c2, float p1){
	float p2 = 1.0f-p1;
	int a1, r1, g1, b1, a2, r2, g2, b2;
	SplitAC(c1, &a1, &r1, &g1, &b1);
	SplitAC(c2, &a2, &r2, &g2, &b2);
	return MAKE_AC(
		Round(p1*a1+p2*a2), Round(p1*r1+p2*r2), Round(p1*g1+p2*g2), Round(p1*b1+p2*b2));
}
inline D3DCOLOR ScaleColor(D3DCOLOR c, float s){
	ValueArea(&s, 0.0f, 1.0f);
	return (Round((c>>24)*s)<<24)|(c&0x00ffffff);
}
inline D3DCOLORVALUE ACtoCV(D3DCOLOR c){
	int a, r, g, b;
	SplitAC(c, &a, &r, &g, &b);
	return MAKE_CV(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

//	外部グローバル
extern bool g_ShadowNeeded;
extern float g_BlinkAlpha;

#endif
