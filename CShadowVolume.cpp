#include "stdafx.h"
#include "CJobTimer.h"
#include "CShadowVolume.h"
#include "CScene.h"
#include "CEnvPlugin.h"

//	内部定数
const int TRI_DUMP_MAX_MAX = 21845;	//	三角形ダンプ最大値の最大値
const float SHADOW_INF_DIST = 1.0e10f;	//	シャドウボリューム無限遠点

//	外部グローバル
extern bool g_HidefCaptureFlag;
extern int g_HidefBufferSize;

/*
 *	コンストラクタ
 */
CTriDumpS::CTriDumpS(
	int trinum	//	四角形数
){
	m_TriNum = trinum;
	if(m_TriNum>TRI_DUMP_MAX_MAX) m_TriNum = TRI_DUMP_MAX_MAX;
	m_Count = 0;
	m_Buffer = new VTX_S[m_TriNum*3];
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CTriDumpS::CTriDumpS(
	CTriDumpS *src	//	引き継ぎ元
){
	m_TriNum = src->m_TriNum;
	m_Count = src->m_Count;
	m_Buffer = src->m_Buffer;
	m_Next = src->m_Next;
}

/*
 *	デストラクタ
 */
CTriDumpS::~CTriDumpS(){
	DELETE_A(m_Buffer);
	DELETE_V(m_Next);
}

/*
 *	バッファ追加
 */
void CTriDumpS::Feed(){
	m_Next = new CTriDumpS(this);
	m_Count = 0;
	m_Buffer = new VTX_S[m_TriNum*3];
}

/*
 *	プリミティブ追加
 */
void CTriDumpS::Add(
	VEC3 p1, VEC3 p2, VEC3 p3	//	頂点
){
	if(m_Count==m_TriNum) Feed();
	VTX_S *buf = &m_Buffer[m_Count*3];
	*buf = p1; buf++;
	*buf = p2; buf++;
	*buf = p3; buf++;
	m_Count++;
}

/*
 *	即描画
 */
void CTriDumpS::Preview(
	VEC3 p1, VEC3 p2, VEC3 p3	//	頂点
){
	VTX_S prev[3], *buf = prev;
	*buf = p1; buf++;
	*buf = p2; buf++;
	*buf = p3; buf++;
	sv3.pDev->SetVertexShader(FVF_S);
	sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, prev, sizeof(VTX_S));
}

/*
 *	バーテックス準備
 */
void CTriDumpS::PrepareVertex(){
	m_Vertex.Create(m_Buffer, FVF_S, m_Count*3*sizeof(VTX_S));
	if(m_Next) m_Next->PrepareVertex();
}

/*
 *	レンダリング
 */
void CTriDumpS::Render(
	bool drawup	//	DrawPrimitiveUp を使用
){
	devSetTexture(0, NULL);
	if(drawup){
		sv3.pDev->SetVertexShader(FVF_S);
		sv3.pDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_Count, m_Buffer, sizeof(VTX_S));
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
CShadowVolume::CShadowVolume(){
	m_FaceVolume = NULL;
}

/*
 *	デストラクタ
 */
CShadowVolume::~CShadowVolume(){
	DELETE_V(m_FaceVolume);
}

/*
 *	リセット
 */
void CShadowVolume::Reset(){
	DELETE_V(m_FaceVolume);
	m_FaceVolume = new CTriDumpS(TRI_DUMP_MAX);
}

/*
 *	メッシュから生成
 */
void CShadowVolume::BuildFromMesh(
	CObject *obj,	//	オブジェクト
	VEC3 vLight		//	ライト方向
){
	TIMER_RAII("CShadowVolume::BuildFromMesh()");

	CMesh *udxMesh = obj->GetMesh();
	if(!udxMesh) return;
	udxMesh->MaskMatFlag(1);
	LPD3DXMESH pMesh = udxMesh->GetObject();

	VEC3 vLocal = V3WorldToLocal(&vLight, obj);
	MTX4 mtxWorld = obj->GetWMatrix();

	UINT fvfSize = D3DXGetFVFVertexSize(pMesh->GetFVF());

	BYTE* pVertices = NULL;
	WORD* pIndices = NULL;
	DWORD* pAttributes = NULL;

	//	バッファをロック
	if(FAILED(pMesh->LockVertexBuffer(D3DLOCK_READONLY, (BYTE **)&pVertices))){
		return;
	}
	if(FAILED(pMesh->LockIndexBuffer(D3DLOCK_READONLY, (BYTE **)&pIndices))){
		pMesh->UnlockVertexBuffer();
		return;
	}
	if(FAILED(pMesh->LockAttributeBuffer(D3DLOCK_READONLY, &pAttributes))){
		pMesh->UnlockVertexBuffer();
		pMesh->UnlockIndexBuffer();
		return;
	}
	DWORD dwNumVertices = pMesh->GetNumVertices();
	DWORD dwNumFaces = pMesh->GetNumFaces();

	DWORD i, j;
	m_TempIndex.clear();
	for(i = 0; i<dwNumFaces; i++){
		if(udxMesh->GetMatFlag(pAttributes[i])) continue;
		WORD wFace0 = pIndices[3*i+0];
		WORD wFace1 = pIndices[3*i+1];
		WORD wFace2 = pIndices[3*i+2];
		VEC3 v0 = *(VEC3 *)(pVertices+fvfSize*wFace0);
		VEC3 v1 = *(VEC3 *)(pVertices+fvfSize*wFace1);
		VEC3 v2 = *(VEC3 *)(pVertices+fvfSize*wFace2);

		VEC3 vNormal;
		V3Cross(&vNormal, &(v2-v1), &(v1-v0));
		if(V3Dot(&vNormal, &vLocal)>=0.0f){
			m_TempIndex.push_back(CEdgeIndex(wFace0, wFace1));
			m_TempIndex.push_back(CEdgeIndex(wFace1, wFace2));
			m_TempIndex.push_back(CEdgeIndex(wFace2, wFace0));
		}
	}

	sort(m_TempIndex.begin(), m_TempIndex.end());

	DWORD dwNumEdges = m_TempIndex.size();
	for(i = 0; i<dwNumEdges; i++){
		CEdgeIndex edge = m_TempIndex[i];
		int cnt = 1;
		for(; i<dwNumEdges-1; i++){
			int cmp = edge.Compare(m_TempIndex[i+1]);
			if(!cmp) break;
			cnt += cmp;
		}
		if(!cnt) continue;
		VEC3 v1, tv1 = *(VEC3 *)(pVertices+fvfSize*edge.m_Index1);
		VEC3 v2, tv2 = *(VEC3 *)(pVertices+fvfSize*edge.m_Index2);
		if(cnt<0){
			cnt = -cnt;
			D3DXVec3TransformCoord(&v2, &tv1, &mtxWorld);
			D3DXVec3TransformCoord(&v1, &tv2, &mtxWorld);
		}else{
			D3DXVec3TransformCoord(&v1, &tv1, &mtxWorld);
			D3DXVec3TransformCoord(&v2, &tv2, &mtxWorld);
		}
		for(j = 0; j<cnt; j++) m_FaceVolume->Add(v1, v2, vLight*SHADOW_INF_DIST);
	}

	//	バッファのロックを解除
	pMesh->UnlockVertexBuffer();
	pMesh->UnlockIndexBuffer();
	pMesh->UnlockAttributeBuffer();
}

/*
 *	境界辺を追加
 */
void CShadowVolume::AddFaceEdge(
	VEC3 &v1, VEC3 &v2,	//	頂点
	VEC3 &vLight		//	光源方向
){
	m_FaceVolume->Add(v1, v2, vLight*SHADOW_INF_DIST);
}

/*
 *	レンダリング
 */
void CShadowVolume::Render(){
	//	いろいろ設定
	devSetState( D3DRS_ZENABLE, TRUE );
	devSetState( D3DRS_ZWRITEENABLE, FALSE );
	devSetState( D3DRS_STENCILENABLE, TRUE );
	devSetState( D3DRS_SHADEMODE, D3DSHADE_FLAT );

	//	ステンシルテストは常にパス
	devSetState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
	devSetState( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
	devSetState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );

	//	Z テストがパスするところだけインクリメント
	devSetState( D3DRS_STENCILREF, 0x1 );
	devSetState( D3DRS_STENCILMASK, 0xffffffff );
	devSetState( D3DRS_STENCILWRITEMASK, 0xffffffff );
	devSetState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );

	//	フレームバッファには描かない（ステンシルのみ描く）
	devSetState( D3DRS_ALPHABLENDENABLE, TRUE );
	devSetState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
	devSetState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	//	シャドウボリュームの手前面を描画
	devTransform( &sv3.mtxFront );
	m_FaceVolume->Render( true );

	//	Z テストがパスするところだけデクリメント
	devSetState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );

	//	カリングを逆にして奥面を描画
	devSetState( D3DRS_CULLMODE, D3DCULL_CW );
	devTransform( &sv3.mtxFront );
	m_FaceVolume->Render( true );

	//	設定を戻す
	devSetState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	devSetState( D3DRS_CULLMODE, D3DCULL_CCW );
	devSetState( D3DRS_ZWRITEENABLE, TRUE );
	devSetState( D3DRS_STENCILENABLE, FALSE );
	devSetState( D3DRS_ALPHABLENDENABLE, FALSE );
}

/*
 *	描画
 */
void CShadowVolume::Draw(
	D3DCOLOR color	//	shadow color
){
	//	いろいろ設定
	devSetState( D3DRS_ZENABLE, FALSE );
	devSetState( D3DRS_STENCILENABLE, TRUE );
	devSetState( D3DRS_FOGENABLE, FALSE );
	devSetState( D3DRS_ALPHABLENDENABLE, TRUE );
	devSetState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	devSetState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	devSetTexState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	devSetTexState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	devSetTexState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	devSetTexState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	devSetTexState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	devSetTexState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );

	//	ステンシルバッファの値が 1 以上のところは影
	devSetState( D3DRS_STENCILREF, 0x1 );
	devSetState( D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL );
	devSetState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );

	//	影部分を暗くする
	if(g_HidefCaptureFlag) Fill2DRect(0, 0, g_HidefBufferSize, g_HidefBufferSize, color);
	else Fill2DRect(0, 0, g_DispWidth, g_DispHeight, color);

	//	設定を戻す
	devSetState( D3DRS_ZENABLE, TRUE );
	devSetState( D3DRS_STENCILENABLE, FALSE );
	//devSetState( D3DRS_FOGENABLE, TRUE );
	devSetState( D3DRS_ALPHABLENDENABLE, FALSE );
}
