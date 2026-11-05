//	Copyright (c) 2002 Midikyou

class CNamedObject;

BOOL BoxTest(BOX8 *pDst, BOX8 *pSrc);
BOOL IntersectTriangle(
	const VEC3& orig, const VEC3& dir,
	VEC3& v0, VEC3& v1, VEC3& v2,
	FLOAT *t, FLOAT *u, FLOAT *v);

class CObject{
	CMesh		*m_pMesh;		//	メッシュ・クラスのオブジェクト
	CObject		*m_pParent;		//	親オブジェクト
	MTX4		m_mtx;			//	マトリクス
	float		m_scale;		//	スケール
	BOX8		m_box;			//	境界ボックス（トランスフォーム前）

public:
	static MAT8 matShadow;

	CObject();
	virtual ~CObject();

	void SetMesh(CMesh *pM, VEC3 p = VEC3(0, 0, 0), float s = 1.0f);
	void SetShadowColor(D3DCOLORVALUE cv);
	void Scale(float s);
	void SetScale(float s);
	void SetScale(float sx, float sy, float sz);

	void SetPosS(VEC3 v);
	void Move(VEC3 v);
	void Move2(VEC3 v);
	void MoveS(VEC3 v);
	void RotX(float v);
	void RotX2(float v);
	void RotY(float v);
	void RotY2(float v);
	void RotZ(float v);
	void RotZ2(float v);
	void RotAxis(VEC3 ax, float v);
	void Billboard();
	void Billboard2();
	void LookAt(VEC3 at);
	void LookAt2(VEC3 at);
	void SetDir(VEC3 dir, VEC3 up = VEC3(0, 1, 0));

	float GetWScale();
	VEC3 GetPosS();
	VEC3 GetWPos();
	VEC3 GetWRight();
	VEC3 GetWUp();
	VEC3 GetWDir();
	MTX4 GetWMatrix();

	void RenderCustom(CNamedObject *nobj);
	void Render();
	void RenderAmb();
	void RenderT(LPTEX8);
	void RenderA(float);
	void RenderAP(float);
	void RenderSC(MAT8 *);
	void RenderShadow(MTX4 *pMtx);

	float GetRadius();
	VEC3 GetCenter();
	BOX8 GetBox();
	void GetSphere(VEC3 *c, float *r);
	BOOL IntersectB(VEC3 pos, VEC3 dir);
	BOOL IntersectS(VEC3 pos, VEC3 dir);
	BOOL Pick(VEC3 pos, VEC3 dir, VEC3 *hit, VEC3 tri[] = NULL, BOOL inv = FALSE);
	BOOL IsVisible();

	void ResetMatFlag(DWORD def = 0){ if(m_pMesh) m_pMesh->ResetMatFlag(def); }
	BOOL IsMeshValid(){ return m_pMesh && m_pMesh->GetObject(); }
	void SetPos(VEC3 v){ m_mtx._41 = v.x, m_mtx._42 = v.y, m_mtx._43 = v.z; }
	VEC3 GetPos()		{ return VEC3(m_mtx._41, m_mtx._42, m_mtx._43); }
	VEC3 GetRight()		{ return VEC3(m_mtx._11, m_mtx._12, m_mtx._13); }
	VEC3 GetUp()		{ return VEC3(m_mtx._21, m_mtx._22, m_mtx._23); }
	VEC3 GetDir()		{ return VEC3(m_mtx._31, m_mtx._32, m_mtx._33); }
	float GetScale()	{ return m_scale; }

	void SetMatrix(MTX4 *pMtx){ m_mtx = *pMtx; m_scale = 1; }
	MTX4 GetMatrix(){ return m_mtx; }
	void SetParent(CObject *p){m_pParent = p; }
	CObject *GetParent(){ return m_pParent; }
	CMesh *GetMesh(){ return m_pMesh; }
};

/*
 *	境界ボックスをセット
 */
inline void SetBoxMinMax(BOX8 *pB, VEC3 min, VEC3 max){
	pB->v[0].x = max.x, pB->v[0].y = max.y, pB->v[0].z = max.z;
	pB->v[1].x = max.x, pB->v[1].y = max.y, pB->v[1].z = min.z;
	pB->v[2].x = max.x, pB->v[2].y = min.y, pB->v[2].z = max.z;
	pB->v[3].x = max.x, pB->v[3].y = min.y, pB->v[3].z = min.z;
	pB->v[4].x = min.x, pB->v[4].y = max.y, pB->v[4].z = max.z;
	pB->v[5].x = min.x, pB->v[5].y = max.y, pB->v[5].z = min.z;
	pB->v[6].x = min.x, pB->v[6].y = min.y, pB->v[6].z = max.z;
	pB->v[7].x = min.x, pB->v[7].y = min.y, pB->v[7].z = min.z;
}

/*
 *	境界球テスト
 *
 *	c1, r1		: オブジェクトAの中心と半径
 *	c2, r2		: オブジェクトBの中心と半径
 */
inline BOOL SphereTest(VEC3 c1, float r1, VEC3 c2, float r2){
	if(D3DXVec3Length(&VEC3(c1-c2))<r1+r2) return TRUE;
	else return FALSE;
}

//	手抜き用マクロ(^^;
#define OBJ_CTRL(o, v) \{\
	ON_HOLD(DIK_LEFT) o.Move(VEC3(-v, 0, 0));\
	ON_HOLD(DIK_RIGHT) o.Move(VEC3(v, 0, 0));\
	ON_HOLD(DIK_UP)	o.Move(VEC3(0, 0, v));\
	ON_HOLD(DIK_DOWN) o.Move(VEC3(0, 0, -v));\
	ON_HOLD(DIK_PRIOR) o.Move(VEC3(0, v, 0));\
	ON_HOLD(DIK_NEXT) o.Move(VEC3(0, -v, 0));\
	ON_HOLD(DIK_V)	o.RotY(-0.05f);\
	ON_HOLD(DIK_B)	o.RotY(0.05f);\
}
