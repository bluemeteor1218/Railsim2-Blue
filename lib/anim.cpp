//	Copyright (c) 2002 Midikyou

#include "headers.h"
#include "debug.h"
#include "graphic.h"
#include "texture.h"
#include "mesh.h"
#include "object.h"
#include "anim.h"

/*
 *	コンストラクタ
 */
CAnim::CAnim(){
	m_nMeshes = 0;
	m_nFrames = 0;
	m_playFrame = 0;
	m_speed = 0.05f;
	m_pMeshList = NULL;
	m_pStateTable = NULL;
}

/*
 *	デストラクタ
 */
CAnim::~CAnim(){
	Free();
}

/*
 *	アニメーションを解放
 */
void CAnim::Free(){
	if(m_pMeshList){
		delete [] m_pMeshList;
		m_pMeshList = NULL;
	}
	if(m_pStateTable){
		delete [] m_pStateTable;
		m_pStateTable = NULL;
	}
	m_nMeshes = 0;
	m_nFrames = 0;
	m_playFrame = 0;
}

/*
 *	アニメーションを読み込み
 *
 *	strFile	: ファイル名
 */
BOOL CAnim::Load(LPCSTR strFile){
	Free();
	FILE *fp = fopen(strFile, "r");

	if(!fp){
		Debug("can't open %s.\n", strFile);
		return FALSE;
	}
	int f, i;

	fscanf(fp, "%d", &m_nMeshes);	//	メッシュ数
	m_pMeshList = new CMesh[m_nMeshes];

	char buf[256];

	for(i = 0; i<m_nMeshes; i++){
		fscanf(fp, "%s", buf);
		m_pMeshList[i].Load(FALSE, buf);
	}
	VEC3 pos;
	QUAT qua;

	fscanf(fp, "%d", &m_nFrames);	//	フレーム数
	m_pStateTable = new ANIM_STATE[m_nFrames*m_nMeshes];

	for(f = 0; f<m_nFrames; f++){
		for(i = 0; i<m_nMeshes; i++){
			fscanf(
				fp, "%f, %f, %f %f, %f, %f, %f",
				&pos.x, &pos.y, &pos.z,
				&qua.x, &qua.y, &qua.z, &qua.w);
			m_pStateTable[f*m_nMeshes+i].pos = pos;
			m_pStateTable[f*m_nMeshes+i].qua = qua;
		}
	}
	fclose(fp);
	return TRUE;
}

/*
 *	キーフレームのクォータニオンからマトリクスを計算
 *
 *	pMtx		: マトリクス格納先
 *	s1		: 現在フレームのクォータニオンのインデックス（メッシュ別）
 *	s2		: 次のフレームのクォータニオンのインデックス（メッシュ別）
 *	rate		: キーフレーム間の比率
 */
void CAnim::ComputeMatrix(MTX4 *pMtx, int s1, int s2, float rate){
	QUAT qua;
	VEC3 pos;

	D3DXQuaternionSlerp(
		&qua,
		&m_pStateTable[s1].qua,
		&m_pStateTable[s2].qua,
		rate);
	D3DXMatrixRotationQuaternion(
		pMtx,
		&qua);
	pos = m_pStateTable[s1].pos*(1-rate)
	+m_pStateTable[s2].pos*rate;

	pMtx->_41 = pos.x;
	pMtx->_42 = pos.y;
	pMtx->_43 = pos.z;
}

/*
 *	レンダリング
 *
 *	fLoop		: ループのON/OFF
 */
BOOL CAnim::Render(BOOL fLoop){
	static int f1, f2;
	static float rate;

	if(m_speed>0){
		f1 = (int)m_playFrame;
		f2 = f1+1;
		if(f2==m_nFrames) f2 = fLoop ? 0 : f1;

		rate = m_playFrame-f1;
	}else if(m_speed<0){
		f1 = (int)m_playFrame;
		f2 = f1-1;
		if(f2==-1) f2 = fLoop ? m_nFrames-1 : f1;

		rate = 1.0f-(m_playFrame-f1);
	}
	//	else f1, f2, rateは前回のまま

	CObject obj;
	MTX4 mtx;
	int s1, s2;

	for(int i = 0; i<m_nMeshes; i++){
		s1 = f1*m_nMeshes+i;
		s2 = f2*m_nMeshes+i;

		ComputeMatrix(&mtx, s1, s2, rate);
		mtx = mtx*m_obj.GetMatrix();

		obj.SetMesh(&m_pMeshList[i]);
		obj.SetMatrix(&mtx);
		obj.Render();
	}
	m_playFrame += m_speed;

	if(m_playFrame>=m_nFrames){
		m_playFrame -= m_nFrames;
		if(!fLoop) return TRUE;
	}else if(m_playFrame<0){
		m_playFrame += m_nFrames;
		if(!fLoop) return TRUE;
	}
	return FALSE;
}
