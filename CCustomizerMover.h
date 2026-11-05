#ifndef CCUSTOMIZERMOVER_H_INCLUDED
#define CCUSTOMIZERMOVER_H_INCLUDED

#include "CCustomizer.h"

/*
 *	移動子基本クラス
 */
class CMoverBase: public CCustomizerBase{
private:
	CMoverState **m_State;		//	状態変数
	CMoverState m_PreviewState;	//	プレビュー用状態変数
public:
	CMoverBase(){ m_State = NULL; }
	virtual CCustomizerBase *Duplicate() = 0;
	virtual void SetOffListCustomizer(CNamedObject *){}
	virtual void ResetOffCustomizer(int){}
	int GetTypeFlagCustomizer(){ return CSTM_MODELMOVER; }
	virtual void SetPostureCustomizer(CObject *) = 0;
	void InitMover(CModelPlugin *);
	CMoverState *GetState(){ return *m_State ? *m_State : &m_PreviewState; }
};

/*
 *	移動子基本クラス
 */
class CDynamicMoverBase: public CMoverBase{
protected:
	int m_ApplyFlag;	//	適用フラグ
public:
	virtual CCustomizerBase *Duplicate() = 0;
	virtual void SetOffListCustomizer(CNamedObject *);
	void ResetOffCustomizer(int);
	virtual void SetPostureCustomizer(CObject *) = 0;
};

/*
 *	静的移動子基本クラス
 */
class CStaticMoverBase: public CDynamicMoverBase{
protected:
	float m_AnimationTime[6];	//	アニメーション時間
public:
	virtual CCustomizerBase *Duplicate() = 0;
	char *ReadTimingInfo(char *);
	virtual void SetPostureCustomizer(CObject *) = 0;
	float ProcDelay();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	静的回転子
 */
class CStaticRotator: public CStaticMoverBase{
private:
	float m_RotationAngle;	//	回転角度
	VEC3 m_RotationAxis;	//	回転軸
public:
	CCustomizerBase *Duplicate(){ return new CStaticRotator(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureCustomizer(CObject *);
};

/*
 *	静的移動子
 */
class CStaticMover: public CStaticMoverBase{
private:
	VEC3 m_Displacement;	//	変位
public:
	CCustomizerBase *Duplicate(){ return new CStaticMover(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureCustomizer(CObject *);
};

/*
 *	動的回転子
 */
class CDynamicRotator: public CDynamicMoverBase{
private:
	float m_RotationSpeed;	//	回転角度
	float m_Acceleration;	//	加速度
	float m_Deceleration;	//	減速度
	VEC3 m_RotationAxis;	//	回転軸
public:
	CCustomizerBase *Duplicate(){ return new CDynamicRotator(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureCustomizer(CObject *);
};

/*
 *	風トラッカ
 */
class CWindTracker: public CMoverBase{
private:
	float m_TrackSpeed;		//	追跡速度
	bool m_FixAxisFlag;		//	軸固定フラグ
	VEC3 m_FixAxis;			//	固定軸
public:
	CCustomizerBase *Duplicate(){ return new CWindTracker(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureCustomizer(CObject *);
};

/*
 *	風車
 */
class CWindmill: public CMoverBase{
private:
	int m_Symmetric;		//	対称カウント
	float m_RotationSpeed;	//	回転速度
	float m_MaxRotation;	//	フレーム当たり回転量最大値
	bool m_Directional;		//	方向性
	VEC3 m_RotationAxis;	//	回転軸
public:
	CCustomizerBase *Duplicate(){ return new CWindmill(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureCustomizer(CObject *);
};

/*
 *	アナログ時計
 */
class CAnalogClock: public CCustomizerBase{
private:
	int m_HandType;			//	針タイプ
public:
	CCustomizerBase *Duplicate(){ return new CAnalogClock(*this); }
	char *Read(char *);
	int GetTypeFlagCustomizer(){ return CSTM_MODELMOVER; }
	void SetPostureCustomizer(CObject *);
};

#endif
