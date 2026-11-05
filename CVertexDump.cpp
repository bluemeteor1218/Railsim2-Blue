#include "stdafx.h"

//	内部定数
const int LINE_DUMP_MAX_MAX = 32767;	//	直線ダンプ最大値の最大値
const int QUAD_DUMP_MAX_MAX = 10922;	//	四角形ダンプ最大値の最大値

//	外部グローバル
extern bool g_HidefCaptureFlag;
extern int g_HidefQuality;
extern MTX4 g_BoldLineMtx[8][8];

/*
 *	コンストラクタ
 */
CLineDumpTL::CLineDumpTL(
	int linenum	//	直線数
){
	m_LineNum = linenum;
	if(m_LineNum>LINE_DUMP_MAX_MAX) m_LineNum = LINE_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_TL[m_LineNum*2];
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CLineDumpTL::CLineDumpTL(
	CLineDumpTL *src	//	引き継ぎ元
){
	m_LineNum = src->m_LineNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CLineDumpTL::~CLineDumpTL(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CLineDumpTL::Feed(){
	m_Next = new CLineDumpTL(this);
	m_Count = 0;
	m_Buffer = new VTX_TL[m_LineNum*2];
}

/*
 *	プリミティブ追加
 */
void CLineDumpTL::Add(
	VEC2 p1, D3DCOLOR c1,	//	頂点 1
	VEC2 p2, D3DCOLOR c2	//	頂点 2
){
	if(m_Count==m_LineNum) Feed();
	VTX_TL *buf = &m_Buffer[m_Count*2];
	buf->x = p1.x; buf->y = p1.y; buf->z = 0.0f; buf->rhw = 1.0f; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = 0.0f; buf->rhw = 1.0f; buf->d = c2;
	m_Count++;
}

/*
 *	即描画
 */
void CLineDumpTL::Preview(
	VEC2 p1, D3DCOLOR c1,	//	頂点 1
	VEC2 p2, D3DCOLOR c2	//	頂点 2
){
	VTX_TL prev[2], *buf = prev;
	buf->x = p1.x; buf->y = p1.y; buf->z = 0.0f; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = 0.0f; buf->d = c2;
	sv3.pDev->SetVertexShader(FVF_TL);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, prev, sizeof(VTX_TL));
}

/*
 *	バーテックス準備
 */
void CLineDumpTL::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_TL, m_Count*2*sizeof(VTX_TL));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CLineDumpTL::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, NULL);
	if(m_Next) m_Next->Render(drawup);
	if(drawup){
		sv3.pDev->SetVertexShader(FVF_TL);
		sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, m_Count, m_Buffer, sizeof(VTX_TL));
	}else{
		m_Vertex.RenderLL();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CLineDumpL::CLineDumpL(
	int linenum	//	直線数
){
	m_LineNum = linenum;
	if(m_LineNum>LINE_DUMP_MAX_MAX) m_LineNum = LINE_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_L[m_LineNum*2];
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CLineDumpL::CLineDumpL(
	CLineDumpL *src	//	引き継ぎ元
){
	m_LineNum = src->m_LineNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CLineDumpL::~CLineDumpL(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CLineDumpL::Feed(){
	m_Next = new CLineDumpL(this);
	m_Count = 0;
	m_Buffer = new VTX_L[m_LineNum*2];
}

/*
 *	プリミティブ追加
 */
void CLineDumpL::Add(
	VEC3 p1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, D3DCOLOR c2	//	頂点 2
){
	if(m_Count==m_LineNum) Feed();
	VTX_L *buf = &m_Buffer[m_Count*2];
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z; buf->d = c2;
	m_Count++;
}

/*
 *	即描画
 */
void CLineDumpL::Preview(
	VEC3 p1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, D3DCOLOR c2	//	頂点 2
){
	VTX_L prev[2], *buf = prev;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z; buf->d = c2;
	sv3.pDev->SetVertexShader(FVF_L);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, prev, sizeof(VTX_L));
}

/*
 *	バーテックス準備
 */
void CLineDumpL::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_L, m_Count*2*sizeof(VTX_L));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CLineDumpL::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, NULL);
	if(drawup){
		sv3.pDev->SetVertexShader(FVF_L);
		sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, m_Count, m_Buffer, sizeof(VTX_L));
	}else{
		m_Vertex.RenderLL();
	}
	if(m_Next) m_Next->Render(drawup);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CLineDumpN::CLineDumpN(
	int linenum	//	直線数
){
	m_LineNum = linenum;
	if(m_LineNum>LINE_DUMP_MAX_MAX) m_LineNum = LINE_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_N[m_LineNum*2];
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CLineDumpN::CLineDumpN(
	CLineDumpN *src	//	引き継ぎ元
){
	m_LineNum = src->m_LineNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CLineDumpN::~CLineDumpN(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CLineDumpN::Feed(){
	m_Next = new CLineDumpN(this);
	m_Count = 0;
	m_Buffer = new VTX_N[m_LineNum*2];
}

/*
 *	プリミティブ追加
 */
void CLineDumpN::Add(
	VEC3 p1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, D3DCOLOR c2	//	頂点 2
){
	if(m_Count==m_LineNum) Feed();
	VTX_N *buf = &m_Buffer[m_Count*2];
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z; buf->d = c1; buf->n = V3UP;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z; buf->d = c2; buf->n = V3UP;
	m_Count++;
}

/*
 *	即描画
 */
void CLineDumpN::Preview(
	VEC3 p1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, D3DCOLOR c2	//	頂点 2
){
	VTX_N prev[2], *buf = prev;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z; buf->d = c1; buf->n = V3UP;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z; buf->d = c2; buf->n = V3UP;
	sv3.pDev->SetVertexShader(FVF_N);
	sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, prev, sizeof(VTX_N));
}

/*
 *	バーテックス準備
 */
void CLineDumpN::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_N, m_Count*2*sizeof(VTX_N));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CLineDumpN::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, NULL);
	if(g_HidefCaptureFlag){
		int bx, by;
		for(by = 0; by<g_HidefQuality; ++by){
			for(bx = 0; bx<g_HidefQuality; ++bx){
				sv3.pDev->SetTransform(D3DTS_PROJECTION, &g_BoldLineMtx[by][bx]);
				if(drawup){
					sv3.pDev->SetVertexShader(FVF_N);
					sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, m_Count, m_Buffer, sizeof(VTX_N));
				}else{
					m_Vertex.RenderLL();
				}
			}
		}
		sv3.pDev->SetTransform(D3DTS_PROJECTION, &sv3.mtxProj);
	}else{
		if(drawup){
			sv3.pDev->SetVertexShader(FVF_N);
			sv3.pDev->DrawPrimitiveUP(D3DPT_LINELIST, m_Count, m_Buffer, sizeof(VTX_N));
		}else{
			m_Vertex.RenderLL();
		}
	}
	if(m_Next) m_Next->Render(drawup);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CQuadDumpN::CQuadDumpN(
	int quadnum	//	四角形数
){
	m_QuadNum = quadnum;
	if(m_QuadNum>QUAD_DUMP_MAX_MAX) m_QuadNum = QUAD_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_N[m_QuadNum*6];
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CQuadDumpN::CQuadDumpN(
	CQuadDumpN *src	//	引き継ぎ元
){
	m_QuadNum = src->m_QuadNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CQuadDumpN::~CQuadDumpN(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CQuadDumpN::Feed(){
	m_Next = new CQuadDumpN(this);
	m_Count = 0;
	m_Buffer = new VTX_N[m_QuadNum*6];
}

/*
 *	プリミティブ追加
 */
void CQuadDumpN::Add(
	VEC3 p1, VEC3 n1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, VEC3 n2, D3DCOLOR c2,	//	頂点 2
	VEC3 p3, VEC3 n3, D3DCOLOR c3,	//	頂点 3
	VEC3 p4, VEC3 n4, D3DCOLOR c4	//	頂点 4
){
	if(m_Count==m_QuadNum) Feed();
	VTX_N *buf = &m_Buffer[m_Count*6];
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z;
	buf->n = n2; buf->d = c2;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3;
	buf++;
	buf->x = p4.x; buf->y = p4.y; buf->z = p4.z;
	buf->n = n4; buf->d = c4;
	buf++;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1;
	m_Count++;
}

/*
 *	即描画
 */
void CQuadDumpN::Preview(
	VEC3 p1, VEC3 n1, D3DCOLOR c1,	//	頂点 1
	VEC3 p2, VEC3 n2, D3DCOLOR c2,	//	頂点 2
	VEC3 p3, VEC3 n3, D3DCOLOR c3,	//	頂点 3
	VEC3 p4, VEC3 n4, D3DCOLOR c4	//	頂点 4
){
	VTX_N prev[6], *buf = prev;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z;
	buf->n = n2; buf->d = c2;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3;
	buf++;
	buf->x = p4.x; buf->y = p4.y; buf->z = p4.z;
	buf->n = n4; buf->d = c4;
	buf++;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1;
	sv3.pDev->SetVertexShader(FVF_N);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, prev, sizeof(VTX_N));
}

/*
 *	バーテックス準備
 */
void CQuadDumpN::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_N, m_Count*6*sizeof(VTX_N));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CQuadDumpN::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, NULL);
	if(drawup){
		sv3.pDev->SetVertexShader(FVF_N);
		sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_Count*2, m_Buffer, sizeof(VTX_N));
	}else{
		m_Vertex.RenderTL();
	}
	if(m_Next) m_Next->Render(drawup);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CQuadDumpNX::CQuadDumpNX(
	int quadnum,	//	四角形数
	LPTEX8 tex		//	テクスチャ
){
	m_QuadNum = quadnum;
	if(m_QuadNum>QUAD_DUMP_MAX_MAX) m_QuadNum = QUAD_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_NX[m_QuadNum*6];
	m_Texture = tex;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CQuadDumpNX::CQuadDumpNX(
	CQuadDumpNX *src	//	引き継ぎ元
){
	m_QuadNum = src->m_QuadNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Texture = src->m_Texture;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CQuadDumpNX::~CQuadDumpNX(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CQuadDumpNX::Feed(){
	m_Next = new CQuadDumpNX(this);
	m_Count = 0;
	m_Buffer = new VTX_NX[m_QuadNum*6];
}

/*
 *	プリミティブ追加
 */
void CQuadDumpNX::Add(
	VEC3 p1, VEC3 n1, D3DCOLOR c1, float u1, float v1,	//	頂点 1
	VEC3 p2, VEC3 n2, D3DCOLOR c2, float u2, float v2,	//	頂点 2
	VEC3 p3, VEC3 n3, D3DCOLOR c3, float u3, float v3,	//	頂点 3
	VEC3 p4, VEC3 n4, D3DCOLOR c4, float u4, float v4	//	頂点 4
){
	if(m_Count==m_QuadNum) Feed();
	VTX_NX *buf = &m_Buffer[m_Count*6];
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1; buf->u = u1; buf->v = v1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z;
	buf->n = n2; buf->d = c2; buf->u = u2; buf->v = v2;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3; buf->u = u3; buf->v = v3;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3; buf->u = u3; buf->v = v3;
	buf++;
	buf->x = p4.x; buf->y = p4.y; buf->z = p4.z;
	buf->n = n4; buf->d = c4; buf->u = u4; buf->v = v4;
	buf++;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1; buf->u = u1; buf->v = v1;
	m_Count++;
}

/*
 *	即描画
 */
void CQuadDumpNX::Preview(
	VEC3 p1, VEC3 n1, D3DCOLOR c1, float u1, float v1,	//	頂点 1
	VEC3 p2, VEC3 n2, D3DCOLOR c2, float u2, float v2,	//	頂点 2
	VEC3 p3, VEC3 n3, D3DCOLOR c3, float u3, float v3,	//	頂点 3
	VEC3 p4, VEC3 n4, D3DCOLOR c4, float u4, float v4	//	頂点 4
){
	VTX_NX prev[6], *buf = prev;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1; buf->u = u1; buf->v = v1;
	buf++;
	buf->x = p2.x; buf->y = p2.y; buf->z = p2.z;
	buf->n = n2; buf->d = c2; buf->u = u2; buf->v = v2;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3; buf->u = u3; buf->v = v3;
	buf++;
	buf->x = p3.x; buf->y = p3.y; buf->z = p3.z;
	buf->n = n3; buf->d = c3; buf->u = u3; buf->v = v3;
	buf++;
	buf->x = p4.x; buf->y = p4.y; buf->z = p4.z;
	buf->n = n4; buf->d = c4; buf->u = u4; buf->v = v4;
	buf++;
	buf->x = p1.x; buf->y = p1.y; buf->z = p1.z;
	buf->n = n1; buf->d = c1; buf->u = u1; buf->v = v1;
	sv3.pDev->SetVertexShader(FVF_NX);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, prev, sizeof(VTX_NX));
}

/*
 *	バーテックス準備
 */
void CQuadDumpNX::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_NX, m_Count*6*sizeof(VTX_NX));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CQuadDumpNX::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, m_Texture);
	if(drawup){
		sv3.pDev->SetVertexShader(FVF_NX);
		sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_Count*2, m_Buffer, sizeof(VTX_NX));
	}else{
		m_Vertex.RenderTL();
	}
	if(m_Next) m_Next->Render(drawup);
}
