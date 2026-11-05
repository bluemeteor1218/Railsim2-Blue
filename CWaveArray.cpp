#include "stdafx.h"
#include "CWaveArray.h"

/*
 *	コンストラクタ
 */
CWaveArray::CWaveArray(){
	m_Wave = NULL;
}

/*
 *	デストラクタ
 */
CWaveArray::~CWaveArray(){
	Free();
}

/*
 *	サウンドのロード
 */
void CWaveArray::Load(
	char *f,	//	ファイル名
	int n,		//	同時再生最大数
	bool f3d	//	3D フラグ
){
	int i;
	BOOL f3dold = svs.f3D;
	svs.f3D = f3d;
	m_Wave = new (CWave[m_Number = n]);
#if 0 // duplicate?
	m_Wave[0].Load(f);
	for(i = 1; i<m_Number; i++) if(!m_Wave[i].Duplicate(&m_Wave[0])) m_Wave[i].Load(f);
#else
	for(i = 0; i<m_Number; i++) m_Wave[i].Load(f);
#endif
	svs.f3D = f3dold;
}

/*
 *	再生
 */
void CWaveArray::Add(
	VEC3 p,	//	音源の座標
	int v,	//	音量
	int ms	//	再生開始位置
){
	int i;
	for(i = 0; i<m_Number; i++){
		if(!m_Wave[i].GetStatus()){
			m_Wave[i].SetPos(p);
			m_Wave[i].SetVolume(v);
			m_Wave[i].Play(ms);
			return;
		}
	}
}
