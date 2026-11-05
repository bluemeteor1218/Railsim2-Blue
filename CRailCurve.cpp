#include "stdafx.h"
#include "CRailCurve.h"
#include "CRailTraceCurve.h"

//	内部定数
extern const float RAIL_SEG_MAX = 100.0f;	//	レール分割最大値

/*
 *	分割点計算
 */
bool CRailCurve::CalcSplit(
	VEC3 &pos1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &dir2	//	終点
){
	m_Radius = -1.0f;
	m_CompSplit = false;
	m_SplitDir = pos2-pos1;
	m_SegLen = V3Len(&m_SplitDir);
	V3Norm(&m_SplitDir, &m_SplitDir);
	if(V3Len(&(pos1+m_SegLen*dir1-pos2))+V3Len(&(pos1+m_SegLen*dir2-pos2))<0.2f){
		m_SplitPos = 0.5f*(pos1+pos2);
		return m_SegLen<=RAIL_SEG_MAX;
	}
	VEC3 m1, m2, cctmp, to1, to2;
	float dot1, dot2, tl1, tl2, dif;
	if(LineLineNearest(&m1, &m2, &pos1, &dir1, &pos2, &dir2)){
		//	parallel
		if(V3Dot(&dir1, &dir2)>0.001f){
			cctmp = 0.5f*(pos1+pos2); to1 = cctmp-pos1; to2 = cctmp-pos2;
			dot1 = V3Dot(&to1, &dir1); dot2 = V3Dot(&to2, &dir2);
			if(dot1*dot2<=0.0f){
				m_CompSplit = true;
				m_SplitPos = 0.5f*(pos1+pos2);
				V3Norm(&cctmp, &(dir1+dir2));
				V3Norm(&m_SplitDir, &(-cctmp+2.0f*m_SplitDir*V3Dot(&cctmp, &m_SplitDir)));
				return false;
			}
		}else{
			dif = V3Dot(&dir1, &(pos2-pos1));
			goto LINECOMP;
		}
	}else{
		cctmp = 0.5f*(m1+m2);
		to1 = cctmp-pos1;
		to2 = cctmp-pos2;
		dot1 = V3Dot(&to1, &dir1);
		dot2 = V3Dot(&to2, &dir2);
		tl1 = V3Len(&to1);
		tl2 = V3Len(&to2);
		dif = tl1-tl2;

		if(tl1<0.001f || tl2<0.001f || dot1*dot2>=0.0f){
			m_CompSplit = true;
			dot1 = V3Dot(&m_SplitDir, &dir1);
			dot2 = V3Dot(&m_SplitDir, &dir2);

			if(fabsf(dot1)<fabsf(dot2)){
				V3Norm(&to1, &(m_SplitDir-dir1*dot1));
				V3Norm(&to2, &(dir1-dir2*V3Dot(&dir1, &dir2)));
				if(V3Dot(&dir2, &m_SplitDir)<0.0f) to2 = -to2;
			}else{
				V3Norm(&to2, &(dir2*dot2-m_SplitDir));
				V3Norm(&to1, &(dir1*V3Dot(&dir1, &dir2)-dir2));
				if(V3Dot(&to1, &m_SplitDir)<0.0f) to1 = -to1;
			}
			float r = 0.25f*m_SegLen, lbound = r, ubound = r;
			while(true){
				float dif = V3Len(&((pos1+to1*r)-(pos2+to2*r)))-2.0f*r;
				if(fabsf(dif)<0.001f) break;
				if(dif>0.0f){
					if(r==ubound) ubound = r *= 2.0f;
					else r = 0.5f*((lbound = r)+ubound);
				}else{
					r = 0.5f*((ubound = r)+lbound);
				}
			}
			m_SplitPos = 0.5f*((m1 = pos1+to1*r)+(m2 = pos2+to2*r));
			VEC3 u1, u2, s1, s2;
			VEC3 out1 = m_SplitPos-m1, out2 = m_SplitPos-m2;
			V3Norm(&s1, V3Cross(&s1, V3Cross(&u1, &dir1, &to1), &out1));
			V3Norm(&s2, V3Cross(&s2, V3Cross(&u2, &dir2, &to2), &out2));
			V3Norm(&m_SplitDir, &(s1+s2));
			return false;
		}
		if(dot1<0.0f) dif = -dif;
LINECOMP:;
		if(dif>1.0f){
			m_CompSplit = true;
			m_SplitPos = pos1+dif*dir1;
			m_SplitDir = dir1;
			return false;
		}
		if(dif<-1.0f){
			m_CompSplit = true;
			m_SplitPos = pos2+dif*dir2;
			m_SplitDir = dir2;
			return false;
		}
	}
	VEC3 mid = 0.5f*(pos1+pos2);
	float theta = acosf(V3Dot(&dir1, &dir2));
	m_Radius = 0.5f*m_SegLen/sinf(0.5f*theta);
	VEC3 out = dir1-dir2;
	V3Norm(&out, &out);
	m_SplitDir = pos2-pos1;
	if(V3Dot(&m_SplitDir, &dir2)<0.0f){
		m_SplitPos = (1.0f+cosf(0.5f*theta))*m_Radius*out+mid;
		V3Norm(&out, &(-dir1-dir2));
		VEC3 vert = m_SplitDir-out*V3Dot(&out, &m_SplitDir);
		m_SplitDir = out*(m_Radius*(2.0f*D3DX_PI-theta))+2.0f*vert;
	}else{
		m_SplitPos = (1.0f-cosf(0.5f*theta))*m_Radius*out+mid;
		V3Norm(&out, &(dir1+dir2));
		VEC3 vert = m_SplitDir-out*V3Dot(&out, &m_SplitDir);
		m_SplitDir = out*(m_Radius*theta)+2.0f*vert;
	}
	V3Norm(&m_SplitDir, &m_SplitDir);
	return false;
}

/*
 *	半径計算
 */
bool CRailCurve::CalcRadius(
	VEC3 &pos1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &dir2	//	終点
){
	if(dir1==dir2) return false;
	float theta = acosf(V3Dot(&dir1, &dir2));
	m_Radius = 0.5f*m_SegLen/sinf(0.5f*theta);
	return true;
}

/*
 *	曲線長さ計算
 */
float CRailCurve::CalcLength(
	VEC3 &pos1, VEC3 &dir1,	//	始点
	VEC3 &pos2, VEC3 &dir2,	//	終点
	float sum				//	累計長さ
){
	if(CalcSplit(pos1, dir1, pos2, dir2)) return sum+V3Len(&(pos2-pos1));
	CRailCurve curve;
	return curve.CalcLength(m_SplitPos, m_SplitDir, pos2, dir2,
		curve.CalcLength(pos1, dir1, m_SplitPos, m_SplitDir, sum));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	static メンバ
bool CRailTraceCurve::ms_Terminate1;
bool CRailTraceCurve::ms_Terminate2;
IRailSplitter CRailTraceCurve::ms_SpliceItr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	カントから座標系計算
 */
void CalcCantAxis(
	VEC3 *right, VEC3 *up, VEC3 *dir,	//	代入先
	float cant							//	カント量
){
	*up = V3UP;
	V3NormAxis(right, up, dir);
	*up += *right*cant;
	V3NormAxis(right, up, dir);
}
