#ifndef CRAILDETECTCURVE_H_INCLUDED
#define CRAILDETECTCURVE_H_INCLUDED

#include "CRailWay.h"
#include "CRailTraceCurve.h"

/*
 *	レール検出カーブ (2D)
 */
class CRailDetectCurve2D: public CRailTraceCurve{
private:
	static float ms_MinDist;		//	最小検出距離
	static CRailLinkTemp ms_Detect;	//	検出情報
	int m_DetectMode;				//	検出モード
	VEC3 m_CursorPos1;				//	カーソル座標 1
	VEC3 m_CursorPos2;				//	カーソル座標 2
public:
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool RenderLink();
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static CRailLinkTemp &GetDetect(){ return ms_Detect; }
	CRailDetectCurve2D(CRailWay *,
		CRailPlugin *, CTiePlugin *, CGirderPlugin *, int ,VEC3 &, VEC3 &);
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

/*
 *	レール検出カーブ (3D)
 */
class CRailDetectCurve3D: public CRailTraceCurve{
private:
	static float ms_MinDist;		//	最小検出距離
	static CRailLinkTemp ms_Detect;	//	検出情報
	VEC3 m_ArrowPos;				//	矢印座標
public:
	static void ResetDetect(){ ms_MinDist = -1.0f; }
	static bool RenderLink();
	static bool IsDetected(){ return ms_MinDist>=0.0f; }
	static CRailLinkTemp &GetDetect(){ return ms_Detect; }
	CRailDetectCurve3D(CRailWay *, CRailPlugin *, CTiePlugin *, CGirderPlugin *, VEC3 &);
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

#endif
