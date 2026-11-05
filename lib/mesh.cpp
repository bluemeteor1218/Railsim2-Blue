//	Copyright (c) 2002 Midikyou

#include "..\stdafx.h"

#define UDX_MESH_MEASURE (0)

#if UDX_MESH_MEASURE
#include <vector>
#include <string>
#include "..\CJobTimer.h"
#define UDX_MESH_TIMER_RAII(name) TIMER_RAII(name)
#else
#define UDX_MESH_TIMER_RAII(name)
#endif

#include <rmxfguid.h>
#include <rmxftmpl.h>

#include "..\CModelPlugin.h"
#include "..\CEnvPlugin.h"
#include "..\CConfigMode.h"

//	外部グローバル
extern CTexList g_TexList;
extern float g_BlinkAlpha;
extern CEnvPlugin *g_Env;

//	内部グローバル
int g_AncientNightFlag;		//	旧バージョン対応用・夜間発光フラグ
CMeshList g_MeshList;		//	テクスチャリスト
MAT8 *g_AltMaterial = NULL;	//	代替マテリアル

/*
 *	コンストラクタ
 */
CXFile::CXFile(){
	m_pXF = NULL;
	m_pPtr = NULL;
}

/*
 *	デストラクタ
 */
CXFile::~CXFile(){
	Close();
}

/*
 *	ファイル／リソースのオープン
 */
BOOL CXFile::Open(LPCSTR strSrc, BOOL fRes){
	HRESULT hr;

	DirectXFileCreate(&m_pXF);
	m_pXF->RegisterTemplates((LPVOID)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);

	if(fRes){
		DXFILELOADRESOURCE res;

		res.hModule = NULL;
		res.lpName = strSrc;
		res.lpType = "X";

		hr = m_pXF->CreateEnumObject(
			(LPVOID)&res, DXFILELOAD_FROMRESOURCE, &m_pPtr);
	}else{
		hr = m_pXF->CreateEnumObject(
			(LPVOID)strSrc,	DXFILELOAD_FROMFILE, &m_pPtr);
	}
	return hr==DXFILE_OK;
}

/*
 *	次のデータを取得
 */
BOOL CXFile::GetNextData(LPDIRECTXFILEDATA *ppDat){
	HRESULT hr;

	hr = m_pPtr->GetNextDataObject(ppDat);

	return hr==DXFILE_OK;
}

/*
 *	最上階層のメッシュを取得
 */
BOOL CXFile::GetTopMesh(LPDIRECTXFILEDATA *ppDat){
	const GUID *type;

	while(1){
		if(!GetNextData(ppDat)) return FALSE;

		(*ppDat)->GetType(&type);

		if(*type==TID_D3DRMMesh) break;
		else RELEASE(*ppDat);	//	メッシュ以外は解放
	}
	return TRUE;
}

/*
 *	クローズ
 */
void CXFile::Close(){
	RELEASE(m_pPtr);
	RELEASE(m_pXF);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CMesh::CMesh(){
	m_pMesh = NULL;
	m_pMatFlag = NULL;
	m_pMatOrder = NULL;
	m_pMat = NULL;
	m_pCustomMat = NULL;
	m_pTex = NULL;
	m_pCustomTex = NULL;
	m_pTexTrans = NULL;
}

/*
 *	デストラクタ
 */
CMesh::~CMesh(){
	Free();
}

/*
 *	X ファイルの読込み
 */
BOOL CMesh::Load(
	BOOL fRes,			//	リソースか？
	char *strName,		//	ファイル・リソース名
	D3DCOLOR cTrans,	//	テクスチャーの透過色
	int nMipLv			//	ミップマップ LV
){
	//	既存なら解放
	if(m_pMesh) Free();

	Debug("load(%s) ... ", strName);
	m_strName = strName;

	LPD3DXBUFFER pBuf = 0;
	LPD3DXBUFFER pAdj = 0;
	HRESULT hr;

	if(fRes){
		//	リソース読込み
		CXFile xfile;
		if(!xfile.Open(strName, TRUE)){
			Debug("open failed.\n");
			return FALSE;
		}
		LPDIRECTXFILEDATA pDat;
		if(!xfile.GetTopMesh(&pDat)){
			Debug("mesh is not found.\n");
			return FALSE;
		}
		hr = D3DXLoadMeshFromXof(pDat, D3DXMESH_SYSTEMMEM,
			sv3.pDev, &pAdj, &pBuf, &m_dwNumMat, &m_pMesh);
		RELEASE(pDat);
		xfile.Close();
	}else{
		//	ファイル読込み
		hr = D3DXLoadMeshFromX(
			strName, D3DXMESH_SYSTEMMEM/*Lock等を考えるとこれがベスト*/,
			sv3.pDev, &pAdj, &pBuf, &m_dwNumMat, &m_pMesh);
	}

	if(FAILED(hr)){
		Debug("failed.\n");
		return FALSE;
	}
	Debug("ok.\n");

	//	マテリアルの取得、テクスチャーのロード
	D3DXMATERIAL *pMat = (D3DXMATERIAL *)pBuf->GetBufferPointer();

	m_pMatFlag = new DWORD[m_dwNumMat];
	m_pMat = new MAT8[m_dwNumMat];
	m_pCustomMat = new MAT8[m_dwNumMat];
	m_pTex = new LPTEX8[m_dwNumMat];
	m_pCustomTex = new LPTEX8[m_dwNumMat];
	m_pTexTrans = new TTMTX[m_dwNumMat];

	DWORD i, j;
	for(i = 0; i<m_dwNumMat; i++){
		m_pMat[i] = pMat[i].MatD3D;
		m_pMat[i].Ambient = m_pMat[i].Diffuse;
		m_pTex[i] = NULL;

		if(!pMat[i].pTextureFilename) continue;

		//	リストにあれば参照、なければロードして追加
		m_pTex[i] = g_TexList.Get(fRes, pMat[i].pTextureFilename, cTrans, nMipLv);
	}
	RELEASE(pBuf);

	m_pMatOrder = new DWORD[m_dwNumMat];
	for(i = 0; i<m_dwNumMat; i++) m_pMatOrder[i] = i;
	for(i = 1; i<m_dwNumMat; i++){
		for(j = i; j>0; j--){
			DWORD &o1 = m_pMatOrder[j-1], &o2 = m_pMatOrder[j];
			if(m_pMat[o1].Diffuse.a<m_pMat[o2].Diffuse.a){
				int tmp = o1; o1 = o2; o2 = tmp;
			}else{
				break;
			}
		}
	}

	//	境界の計算
	ComputeBoundary();

	//	メッシュの最適化
	LPD3DXMESH pMeshOpt = NULL;
	DWORD *pAdjBuf = (DWORD *)pAdj->GetBufferPointer();
	hr = m_pMesh->Optimize(
		D3DXMESHOPT_ATTRSORT|D3DXMESHOPT_COMPACT|D3DXMESHOPT_VERTEXCACHE,
		pAdjBuf, NULL, NULL, NULL, &pMeshOpt);
	if(SUCCEEDED(hr)){
		m_pMesh->Release();
		m_pMesh = pMeshOpt;
	}else{
		Debug("optimization failed (%x).\n", hr);
	}
	RELEASE(pAdj);
	return TRUE;
}

/*
 *	球の作成
 *
 *	sl	: スライス数
 *	st	: スタック数
 *	cv	: 色
 */
BOOL CMesh::CreateSphere(float r, UINT sl, UINT st, D3DCOLORVALUE cv){
	//	既存なら解放
	if(m_pMesh) Free();

	if(FAILED(D3DXCreateSphere(sv3.pDev, r, sl, st, &m_pMesh, NULL))){
		Debug("D3DXCreateSphere\n");
		return FALSE;
	}
	m_strName = "";

	m_dwNumMat = 1;
	m_pMatFlag = new DWORD[m_dwNumMat];
	m_pMat = new MAT8[m_dwNumMat];
	m_pCustomMat = new MAT8[m_dwNumMat];
	m_pTex = new LPTEX8[m_dwNumMat];
	m_pCustomTex = new LPTEX8[m_dwNumMat];
	m_pTexTrans = new TTMTX[m_dwNumMat];

	m_pMat[0].Diffuse = m_pMat[0].Ambient = cv;
	m_pMat[0].Specular = m_pMat[0].Emissive = MAKE_CV(0, 0, 0, 0);
	m_pMat[0].Power = 0.0f;
	m_pTex[0] = NULL;

	ComputeBoundary();
	return TRUE;
}

/*
 *	直方体の作成
 *
 *	x		: Xサイズ
 *	y		: Yサイズ
 *	z		: Zサイズ
 *	cv	: 色
 */
BOOL CMesh::CreateBox(float x, float y, float z, D3DCOLORVALUE cv){
	//	既存なら解放
	if(m_pMesh) Free();

	if(FAILED(D3DXCreateBox(sv3.pDev, x, y, z, &m_pMesh, NULL))){
		Debug("D3DXCreateSphere\n");
		return FALSE;
	}
	m_strName = "";

	m_dwNumMat = 1;
	m_pMatFlag = new DWORD[m_dwNumMat];
	m_pMat = new MAT8[m_dwNumMat];
	m_pCustomMat = new MAT8[m_dwNumMat];
	m_pTex = new LPTEX8[m_dwNumMat];
	m_pCustomTex = new LPTEX8[m_dwNumMat];
	m_pTexTrans = new TTMTX[m_dwNumMat];

	m_pMat[0].Diffuse = m_pMat[0].Ambient = cv;
	m_pMat[0].Specular = m_pMat[0].Emissive = MAKE_CV(0, 0, 0, 0);
	m_pMat[0].Power = 0.0f;
	m_pTex[0] = NULL;

	ComputeBoundary();
	return TRUE;
}

/*
 *	直方体の作成
 *
 *	x		: Xサイズ
 *	y		: Yサイズ
 *	z		: Zサイズ
 *	cv	: 色
 */
BOOL CMesh::CreateTeapot(D3DCOLORVALUE cv){
	//	既存なら解放
	if(m_pMesh) Free();

	if(FAILED(D3DXCreateTeapot(sv3.pDev, &m_pMesh, NULL))){
		Debug("D3DXCreateTeapot\n");
		return FALSE;
	}
	m_strName = "";

	m_dwNumMat = 1;
	m_pMatFlag = new DWORD[m_dwNumMat];
	m_pMat = new MAT8[m_dwNumMat];
	m_pCustomMat = new MAT8[m_dwNumMat];
	m_pTex = new LPTEX8[m_dwNumMat];
	m_pCustomTex = new LPTEX8[m_dwNumMat];
	m_pTexTrans = new TTMTX[m_dwNumMat];

	m_pMat[0].Diffuse = m_pMat[0].Ambient = cv;
	m_pMat[0].Specular = m_pMat[0].Emissive = MAKE_CV(0, 0, 0, 0);
	m_pMat[0].Power = 0.0f;
	m_pTex[0] = NULL;

	ComputeBoundary();
	return TRUE;
}

/*
 *	メッシュの解放
 */
void CMesh::Free(){
	if(m_strName!=""){
		Debug("release(%s)\n", m_strName.c_str());
		m_strName = "";
	}

	if(m_pTex) for(DWORD i = 0;i<m_dwNumMat;i++) g_TexList.Release(m_pTex[i]);
	DELETE_A(m_pMatFlag);
	DELETE_A(m_pMatOrder);
	DELETE_A(m_pMat);
	DELETE_A(m_pCustomMat);
	DELETE_A(m_pTex);
	DELETE_A(m_pCustomTex);
	DELETE_A(m_pTexTrans);
	RELEASE(m_pMesh);
}

/*
 *	境界ボックス／球の計算
 */
void CMesh::ComputeBoundary(){
	LPBYTE	pVertex;
	DWORD	nVertex, fvf;

	m_pMesh->LockVertexBuffer(D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK, &pVertex);
	nVertex = m_pMesh->GetNumVertices();
	fvf = m_pMesh->GetFVF();

	D3DXComputeBoundingBox((VOID *)pVertex, nVertex, fvf, &m_min, &m_max);
	//D3DXComputeBoundingSphere((VOID *)pVertex, nVertex, fvf, &m_center, &m_radius);
	m_center = 0.5f*(m_min+m_max);
	m_radius = 0.5f*V3Len(&(m_max-m_min));
	m_pMesh->UnlockVertexBuffer();
}

/*
 *	マテリアルフラグのリセット
 */
void CMesh::ResetMatFlag(
	DWORD def	//	デフォルト値
){
	for(DWORD i = 0; i<m_dwNumMat; i++) m_pMatFlag[i] = def;
}

/*
 *	マテリアルフラグをマスク
 */
void CMesh::MaskMatFlag(
	DWORD mask	//	マスク値
){
	for(DWORD i = 0; i<m_dwNumMat; i++) m_pMatFlag[i] &= mask;
}

/*
 *	カスタムレンダリング
 *
 *	pMtx	: 座標変換行列
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CMesh::RenderCustom(MTX4 *pMtx, CNamedObject *nobj){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	/*
	 *	MatFlag
	 *	0x01 ( 1): NoCastShadow
	 *	0x02 ( 2): AfterShadow phase
	 *	0x04 ( 4): AfterShadow enable
	 *	0x08 ( 8): EnvMap
	 *	0x10 (16): TexTrans
	 *	0x20 (32): AlphaZeroTest
	 */

	memcpy(m_pCustomMat, m_pMat, m_dwNumMat*sizeof(MAT8));
	memcpy(m_pCustomTex, m_pTex, m_dwNumMat*sizeof(LPTEX8));
	nobj->SetMaterial(this);
	for(DWORD i = 0; i<m_dwNumMat; i++){
		DWORD order = m_pMatOrder[i];
		float alpha = m_pCustomMat[order].Diffuse.a;
		int flag = m_pMatFlag[order];
		if((flag&6)==2 || !alpha) continue;
		if(g_AltMaterial){
			sv3.pDev->SetMaterial(g_AltMaterial);
			devSetTexture(0, NULL);
			{
				UDX_MESH_TIMER_RAII("DrawSubset");
				m_pMesh->DrawSubset(order);
			}
		}else{
			if(flag&8){
				devSetEnvMap(1, TRUE);
				g_Env->SetEnvMapTexture();
				devSetTexColor(1, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
				//devSetTexAlpha(1, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
			}
			if(flag&16){
				devSetTexTrans(0, TRUE);
				devTexTransform(0, &m_pTexTrans[order].GetMTX4());
			}
			if(flag&32){
				devTEX_POINT(0);
				devTEX_POINT(1);
				devSetState(D3DRS_ALPHATESTENABLE, TRUE);
				devSetState(D3DRS_ALPHAREF, 0x00);
				devSetState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
			}
			if(g_RenderBlink) m_pCustomMat[order].Diffuse.a *= g_BlinkAlpha;
			sv3.pDev->SetMaterial(&m_pCustomMat[order]);
			devSetTexture(0, m_pCustomTex[order]);
			{
				UDX_MESH_TIMER_RAII("DrawSubset");
				m_pMesh->DrawSubset(order);
			}
			if(flag&8){
				devSetEnvMap(1, FALSE);
				devSetTexture(1, NULL);
				devSetTexColor(1, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT);
				//devSetTexAlpha(1, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT);
			}
			if(flag&16) devSetTexTrans(0, FALSE);
			if(flag&32){
				g_ConfigMode->SetTexFilter();
				devSetState(D3DRS_ALPHATESTENABLE, FALSE);
			}
		}
	}
}

/*
 *	レンダリング
 *
 *	pMtx	: 座標変換行列
 *
 *	更新するデバイスパラメータ	: ワールドマトリクス、マテリアル、テクスチャ
 */
void CMesh::Render(MTX4 *pMtx){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	for(DWORD i = 0; i<m_dwNumMat; i++){
		DWORD order = m_pMatOrder[i];
		float alpha = m_pMat[order].Diffuse.a;
		if(g_AltMaterial){
			sv3.pDev->SetMaterial(g_AltMaterial);
			devSetTexture(0, NULL);
			{
				UDX_MESH_TIMER_RAII("DrawSubset");
				m_pMesh->DrawSubset(order);
			}
		}else{
			if(g_AncientNightFlag){
				if(!alpha){
					MAT8 matTmp = m_pMat[order];
					matTmp.Diffuse.a = g_RenderBlink ? g_BlinkAlpha : 1.0f;
					sv3.pDev->SetMaterial(&matTmp);
				}else if(alpha==0.5f){
					static MAT8 matLight = {
						{1.0f, 0.8f, 0.5f, 0.7f},
						{1.0f, 0.8f, 0.5f, 0.7f},
						{1.0f, 0.8f, 0.5f, 1.0f},
						{1.0f, 0.8f, 0.5f, 1.0f},
						1.0f};
					matLight.Diffuse.a = g_RenderBlink ? g_BlinkAlpha*0.7f : 0.7f;
					sv3.pDev->SetMaterial(&matLight);
				}else{
					if(g_RenderBlink) m_pMat[order].Diffuse.a *= g_BlinkAlpha;
					sv3.pDev->SetMaterial(&m_pMat[order]);
				}
			}else{
				if(!alpha) continue;
				if(g_RenderBlink) m_pMat[order].Diffuse.a *= g_BlinkAlpha;
				sv3.pDev->SetMaterial(&m_pMat[order]);
			}
			devSetTexture(0, m_pTex[order]);
			{
				UDX_MESH_TIMER_RAII("DrawSubset");
				m_pMesh->DrawSubset(order);
			}
			m_pMat[order].Diffuse.a = alpha;
		}
	}
}

/*
 *	レンダリング (アンビエントのみ)
 *
 *	pMtx	: 座標変換行列
 */
void CMesh::RenderAmb(MTX4 *pMtx){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	for(DWORD i = 0;i<m_dwNumMat;i++){
		DWORD order = m_pMatOrder[i];
		D3DCOLORVALUE &dif = m_pMat[order].Diffuse, tdif = dif;
		dif.r = dif.g = dif.b = 0.0f;
		sv3.pDev->SetMaterial(&m_pMat[order]);
		devSetTexture(0, m_pTex[order]);
		{
			UDX_MESH_TIMER_RAII("DrawSubset");
			m_pMesh->DrawSubset(order);
		}
		dif = tdif;
	}
}

/*
 *	レンダリング (テクスチャ指定)
 *
 *	pMtx	: 座標変換行列
 *	pTex	: テクスチャ
 */
void CMesh::RenderT(MTX4 *pMtx, LPTEX8 pTex){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	for(DWORD i = 0;i<m_dwNumMat;i++){
		DWORD order = m_pMatOrder[i];
		sv3.pDev->SetMaterial(&m_pMat[order]);
		devSetTexture(0, pTex);
		{
			UDX_MESH_TIMER_RAII("DrawSubset");
			m_pMesh->DrawSubset(order);
		}
	}
}

/*
 *	レンダリング (アルファ値指定)
 *
 *	pMtx	: 座標変換行列
 *	alpha : アルファ値
 */
void CMesh::RenderA(MTX4 *pMtx, float altalpha){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	for(DWORD i = 0;i<m_dwNumMat;i++){
		DWORD order = m_pMatOrder[i];
		float alpha = m_pMat[order].Diffuse.a;
		if(g_AncientNightFlag){
			if(!alpha){
				MAT8 matTmp = m_pMat[order];
				matTmp.Diffuse.a = altalpha*(g_RenderBlink ? g_BlinkAlpha : 1.0f);
				sv3.pDev->SetMaterial(&matTmp);
			}else if(alpha==0.5f){
				static MAT8 matLight = {
					{1.0f, 0.8f, 0.5f, 0.7f},
					{1.0f, 0.8f, 0.5f, 0.7f},
					{1.0f, 0.8f, 0.5f, 1.0f},
					{1.0f, 0.8f, 0.5f, 1.0f},
					1.0f};
				matLight.Diffuse.a = altalpha*(g_RenderBlink ? g_BlinkAlpha*0.7f : 0.7f);
				sv3.pDev->SetMaterial(&matLight);
			}else{
				m_pMat[order].Diffuse.a *= altalpha*(g_RenderBlink ? g_BlinkAlpha : 1.0f);
				sv3.pDev->SetMaterial(&m_pMat[order]);
			}
		}else{
			if(!alpha) continue;
			m_pMat[order].Diffuse.a *= altalpha*(g_RenderBlink ? g_BlinkAlpha : 1.0f);
			sv3.pDev->SetMaterial(&m_pMat[order]);
		}
		devSetTexture(0, m_pTex[order]);
		{
			UDX_MESH_TIMER_RAII("DrawSubset");
			m_pMesh->DrawSubset(order);
		}
		m_pMat[order].Diffuse.a = alpha;
	}
}

/*
 *	レンダリング (α加算)
 *
 *	pMtx	: 座標変換行列
 *	aplus	: α加算値
 */
void CMesh::RenderAP(MTX4 *pMtx, float aplus){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	D3DCOLORVALUE c1, c2;

	for(DWORD i = 0;i<m_dwNumMat;i++){
		DWORD order = m_pMatOrder[i];
		//	α値を変更
		c2 = c1 = m_pMat[0].Ambient;	//	= m_pMat[0].Diffuse
		c2.a = max(-1.0f, min(1.0f, c2.a*aplus));
		m_pMat[order].Diffuse = m_pMat[order].Ambient = c2;

		sv3.pDev->SetMaterial(&m_pMat[order]);
		devSetTexture(0, m_pTex[order]);
		{
			UDX_MESH_TIMER_RAII("DrawSubset");
			m_pMesh->DrawSubset(order);
		}

		//	α値を復元
		m_pMat[order].Diffuse = m_pMat[order].Ambient = c1;
	}
}

/*
 *	レンダリング (マテリアル指定、影などで使用)
 *
 *	pMtx	: 座標変換行列
 *	mat	: マテリアル
 */
void CMesh::RenderSC(MTX4 *pMtx, MAT8 *pMat){
	if(!m_pMesh) return;
	UDX_MESH_TIMER_RAII("CMesh::Render");

	devTransform(pMtx);

	for(DWORD i = 0;i<m_dwNumMat;i++){
		DWORD order = m_pMatOrder[i];
		sv3.pDev->SetMaterial(pMat);
		devSetTexture(0, NULL);
		{
			UDX_MESH_TIMER_RAII("DrawSubset");
			m_pMesh->DrawSubset(order);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CMeshList::CMeshList(){
	m_pList = NULL;
}

/*
 *	デストラクタ
 */
CMeshList::~CMeshList(){
	while(m_pList) Release(&m_pList->m_Mesh);
}

/*
 *	メッシュをリストから検索し、なければロード
 */
CMesh *CMeshList::Get(BOOL fRes, LPCSTR strName, D3DCOLOR cTrans, int nMipLv){
	if(!strName || !*strName) return NULL;

	//	リストからテクスチャーを検索
	MESHINFO *p = m_pList;

	//	ファイルならフルパスに変換
	char full[_MAX_PATH];
	if(!fRes){
		_fullpath(full, strName, _MAX_PATH);
		strName = full;
		extern void MoveToFile(char *);
		MoveToFile(full);
	}

	while(p){
		//	見つかれば参照カウンタを増やし、メッシュを返す
		if(!_mbsicmp((PUCHAR)p->strName.c_str(), (PUCHAR)strName)
			&& p->cTrans==cTrans && p->nMipLv==nMipLv){
			//	Debug("[%s] is in mesh-list.\n", strName); /*デバッグ*/
			p->nRef++;
			return &p->m_Mesh;
		}
		p = p->pNext;
	}
	//	見つからなければロード
	Debug("load(%s) ... ", strName);

	p = new MESHINFO;

	if(!p->m_Mesh.Load(fRes, (char *)strName, cTrans, nMipLv)){
		Debug("failed.\n");
		return NULL;
	}
	Debug("ok.\n");

	//	リストの先頭に追加、参照カウンタを設定する
	MESHINFO *q = m_pList;
	m_pList = p;
	p->pNext = q;
	p->strName = strName;
	p->nRef = 1;
	p->nMipLv = nMipLv;

	return &p->m_Mesh;
}

/*
 *	可能ならメッシュを解放する
 */
void CMeshList::Release(CMesh *pMesh){
	//	メッシュを検索
	MESHINFO *p = m_pList;
	MESHINFO *q = NULL;

	while(p){
		//	見つかれば参照カウンタを減らす
		if(&p->m_Mesh==pMesh){
			p->nRef--;

			//	参照がなくなればメッシュを解放、リストから外す
			if(p->nRef==0){
				Debug("release(%s)\n", p->strName.c_str());
				p->m_Mesh.Free();

				if(p==m_pList){
					//	リストの先頭
					m_pList = p->pNext;
					delete p;
				}else{
					//	途中
					q->pNext = p->pNext;
					delete p;
				}
			}
			break;
		}
		q = p;
		p = p->pNext;
	}
}
