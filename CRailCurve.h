#ifndef CRAILCURVE_H_INCLUDED
#define CRAILCURVE_H_INCLUDED

class CAxleObject;

/*
 *	レールカーブ
 */
class CRailCurve{
protected:
	float m_Radius;		//	曲率半径
	float m_SegLen;		//	セグメント長
	bool m_CompSplit;	//	補完制御点
	VEC3 m_SplitPos;	//	中間点座標
	VEC3 m_SplitDir;	//	中間点接線 (正規化済)
public:
	virtual void Curve(VEC3 &, VEC3 &, VEC3 &, VEC3 &, bool, bool){}
	virtual void ShowCompSplit(VEC3 &, VEC3 &){}
	bool CalcSplit(VEC3 &, VEC3 &, VEC3 &, VEC3 &);
	bool CalcRadius(VEC3 &, VEC3 &, VEC3 &, VEC3 &);
	float CalcLength(VEC3 &, VEC3 &, VEC3 &, VEC3 &, float sum = 0.0f);
};

//	関数宣言
void CalcCantAxis(VEC3 *, VEC3 *, VEC3 *, float);

#endif
