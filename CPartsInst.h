#ifndef CPARTSINST_H_INCLUDED
#define CPARTSINST_H_INCLUDED

/*
 *	パーツインスタンス
 */
class CPartsInst{
protected:
	CObject m_Object;	//	オブジェクト
public:
	CObject *GetObject(){ return &m_Object; }
	VEC3 GetPos(){ return m_Object.GetPos(); }
	VEC3 GetCenter(){ return m_Object.GetCenter(); }
	void DrawBox();
};

//	反復子
typedef list<CPartsInst>::iterator IPartsInst;

#endif
