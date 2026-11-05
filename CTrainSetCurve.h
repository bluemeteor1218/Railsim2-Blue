#ifndef CTRAINSETCURVE_H_INCLUDED
#define CTRAINSETCURVE_H_INCLUDED

#include "CRailTraceCurve.h"
#include "CTrainSetBuffer.h"

/*
 *	車輌配置カーブ
 */
class CTrainSetCurve: public CRailTraceCurve{
private:
	bool m_Reverse;					//	後退フラグ
	CGroupEndLocator m_Location;	//	設置位置
	CGroupEndLocator *m_Tail;		//	終端格納先
	ITrainSetBuffer *m_Current;		//	現在位置
	ITrainSetBuffer *m_End;			//	終了位置
public:
	CTrainSetCurve(CRailPlugin *, CTiePlugin *, CGirderPlugin *,
		bool, CGroupEndLocator, CGroupEndLocator *, ITrainSetBuffer *, ITrainSetBuffer *);
	bool Confirm(VEC3 &, VEC3 &){ return *m_Current!=*m_End; }
	void FinishTrace(
		VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &, VEC3 &,
		float, float, CRailSplitter &);
};

#endif
