#ifndef CTRAINPLUGIN_H_INCLUDED
#define CTRAINPLUGIN_H_INCLUDED

#include "CModelPlugin.h"
#include "CTrainSetBuffer.h"

/*
 *	オブジェクトジョイント ZY 指定
 */
class CObjectJointZY: public CObjectJoint{
public:
	VEC2 m_AttachCoord;	//	接続先ローカル ZY 座標
	VEC2 m_LocalCoord;	//	自分側ローカル ZY 座標
public:
	char *Read(char *, CModelPlugin *);
	VEC3 GetPos(){
		VEC3 ret(0.0f, m_AttachCoord.y, m_AttachCoord.x);
		CObject *obj = m_Link->GetObject();
		return *D3DXVec3TransformCoord(
			&ret, &(ret/obj->GetScale()), &obj->GetMatrix());
	}
	VEC3 GetRight(){
		CObject *obj = m_Link->GetObject();
		return m_Link->GetTurn() ? -obj->GetRight() : obj->GetRight();
	}
};

/*
 *	オブジェクトジョイント ZY + X 指定
 */
class CObjectJointZYX: public CObjectJointZY{
public:
	float m_AttachX;	//	接続先ローカル X 座標
public:
	char *Read(char *, CModelPlugin *);
	VEC3 GetPos(){
		VEC3 ret(m_AttachX, m_AttachCoord.y, m_AttachCoord.x);
		CObject *obj = m_Link->GetObject();
		return *D3DXVec3TransformCoord(
			&ret, &(ret/obj->GetScale()), &obj->GetMatrix());
	}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	車軸
 */
class CAxleObject: public CNamedObject{
	friend class CAxlePosture;
	friend class CTrainPlugin;
private:
	int m_Symmetric;		//	対称カウント
	float m_Diameter;		//	直径
	float m_MaxRotation;	//	フレーム当たり回転量最大値
	bool m_WheelSound;		//	車輪音フラグ
	VEC2 m_Coord;			//	座標
public:
	bool operator<(const CAxleObject &rhs) const{ return m_Coord.x<rhs.m_Coord.x; }
	virtual int GetType(){ return 1; }
	char *Read(char *, CModelPlugin *);
	float CalcRotation(float);
	void SetPreview(float, bool);
	void SetPosture(VEC3, VEC3, VEC3, float);
};

//	反復子
typedef list<CAxleObject>::iterator IAxleObject;

/*
 *	車体
 */
class CBodyObject: public CNamedObject{
	friend class CTrainPlugin;
private:
	static VEC3 ms_TiltDir;				//	振り子ベクトル
	float m_TiltRatio;					//	振り子係数
	float m_TiltMaxAngle;				//	振り子最大角度
	float m_TiltBaseAlt;				//	振り子基準高さ
	CObjectJointZY m_Joint1, m_Joint2;	//	接続情報
public:
	static void SetTiltDir(VEC3 &td){ ms_TiltDir = td; }
	char *Read(char *, CModelPlugin *);
	void SetPosture();
};

//	反復子
typedef list<CBodyObject>::iterator IBodyObject;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	フリーオブジェクトコンテナ
 */
class CFreeObjectContainer{
private:
	CFreeObjectBase *m_FreeObject;	//	フリーオブジェクト
public:
	CFreeObjectContainer();
	CFreeObjectContainer(const CFreeObjectContainer &);
	CFreeObjectContainer(CFreeObject3D *);
	~CFreeObjectContainer();
	char *Read(char *, CModelPlugin *);
	void SetPosture(){ m_FreeObject->SetPostureFreeObject(); }
	CNamedObject *Check(const string &name){ return m_FreeObject->CheckFreeObject(name); }
	int GetNamedObjectNum(){ return m_FreeObject->GetNamedObjectNum(); }
	CNamedObject *GetNamedObject(int i){ return m_FreeObject->GetNamedObject(i); }
	void LoadModel(CModelPlugin *mpi){ m_FreeObject->LoadModelFreeObject(mpi); }
	void AttachPartsObject(){ m_FreeObject->AttachPartsFreeObject(); }
	void ScanInput(){ m_FreeObject->ScanInputFreeObject(); }
	void Render(){ m_FreeObject->RenderFreeObject(); }
};

//	反復子
typedef list<CFreeObjectContainer>::iterator IFreeObjectContainer;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	2 点配置
 */
class CFreeObjectZY: public CNamedObject, public CFreeObjectBase{
	friend class CTrainPlugin;
private:
	float m_FixPosition;				//	pos 固定位置
	float m_FixRight;					//	right 固定位置
	CObjectJointZYX m_Joint1, m_Joint2;	//	接続情報
public:
	CFreeObjectBase *Duplicate(){ return new CFreeObjectZY(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureFreeObject();
	CNamedObject *CheckFreeObject(const string &name){ return Check(name); }
	int GetNamedObjectNum(){ return 1; }
	CNamedObject *GetNamedObject(int i){ return this; }
	void LoadModelFreeObject(CModelPlugin *mpi){ LoadModel(mpi); }
	void AttachPartsFreeObject(){ GetPartsObject(); }
	void ScanInputFreeObject(){ CheckDetect(); }
	void RenderFreeObject(){ Render(); }
};

/*
 *	三角形辺
 */
class CTriangleLinkZY: public CNamedObject{
	friend class CFreeTriangleZY;
	friend class CFreeCrankZY;
	friend class CFreePistonZY;
private:
	VEC2 m_LinkCoord;			//	三角形リンク座標
	CObjectJointZYX m_Joint;	//	接続情報
public:
	char *Read(char *, CModelPlugin *);
};

/*
 *	三角形オブジェクト
 */
class CFreeTriangleZY: public CFreeObjectBase{
	friend class CTrainPlugin;
private:
	CTriangleLinkZY m_Link1, m_Link2;	//	辺
public:
	CFreeObjectBase *Duplicate(){ return new CFreeTriangleZY(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureFreeObject();
	CNamedObject *CheckFreeObject(const string &name){
		return m_Link1.Check(name) ? &m_Link1
			: (m_Link2.Check(name) ? &m_Link2 : NULL);
	}
	int GetNamedObjectNum(){ return 2; }
	CNamedObject *GetNamedObject(int i){ return i ? &m_Link2 : &m_Link1; }
	void LoadModelFreeObject(CModelPlugin *mpi){
		m_Link1.LoadModel(mpi); m_Link2.LoadModel(mpi);
	}
	void AttachPartsFreeObject(){ m_Link1.GetPartsObject(); m_Link2.GetPartsObject(); }
	void ScanInputFreeObject(){ m_Link1.CheckDetect(); m_Link2.CheckDetect(); }
	void RenderFreeObject(){ m_Link1.Render(); m_Link2.Render(); }
};

/*
 *	クランクスライダ
 */
class CCrankSlideZY: public CNamedObject{
	friend class CFreeCrankZY;
private:
	VEC2 m_Direction;			//	三角形リンク座標
	CObjectJointZYX m_Joint;	//	接続情報
public:
	char *Read(char *, CModelPlugin *);
};

/*
 *	クランクオブジェクト
 */
class CFreeCrankZY: public CFreeObjectBase{
	friend class CTrainPlugin;
private:
	CTriangleLinkZY m_Link;	//	辺
	CCrankSlideZY m_Slide;	//	スライダ
public:
	CFreeObjectBase *Duplicate(){ return new CFreeCrankZY(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureFreeObject();
	CNamedObject *CheckFreeObject(const string &name){
		return m_Link.Check(name) ? (CNamedObject *)&m_Link
			: (m_Slide.Check(name) ? (CNamedObject *)&m_Slide : NULL);
	}
	int GetNamedObjectNum(){ return 2; }
	CNamedObject *GetNamedObject(int i){ return i ? (CNamedObject*)&m_Slide : (CNamedObject*)&m_Link; }
	void LoadModelFreeObject(CModelPlugin *mpi){
		m_Link.LoadModel(mpi); m_Slide.LoadModel(mpi);
	}
	void AttachPartsFreeObject(){ m_Link.GetPartsObject(); m_Slide.GetPartsObject(); }
	void ScanInputFreeObject(){ m_Link.CheckDetect(); m_Slide.CheckDetect(); }
	void RenderFreeObject(){ m_Link.Render(); m_Slide.Render(); }
};

/*
 *	ピストンオブジェクト
 */
class CFreePistonZY: public CFreeObjectBase{
	friend class CTrainPlugin;
private:
	CTriangleLinkZY m_Link1, m_Link2;	//	辺
public:
	CFreeObjectBase *Duplicate(){ return new CFreePistonZY(*this); }
	char *Read(char *, CModelPlugin *);
	void SetPostureFreeObject();
	CNamedObject *CheckFreeObject(const string &name){
		return m_Link1.Check(name) ? &m_Link1
			: (m_Link2.Check(name) ? &m_Link2 : NULL);
	}
	int GetNamedObjectNum(){ return 2; }
	CNamedObject *GetNamedObject(int i){ return i ? &m_Link2 : &m_Link1; }
	void LoadModelFreeObject(CModelPlugin *mpi){
		m_Link1.LoadModel(mpi); m_Link2.LoadModel(mpi);
	}
	void AttachPartsFreeObject(){ m_Link1.GetPartsObject(); m_Link2.GetPartsObject(); }
	void ScanInputFreeObject(){ m_Link1.CheckDetect(); m_Link2.CheckDetect(); }
	void RenderFreeObject(){ m_Link1.Render(); m_Link2.Render(); }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	車輌プラグイン
 */
class CTrainPlugin: public CModelPlugin{
	friend class CTrain;
	friend class CTrainGroup;
private:
	float m_FrontLimit;							//	前部連結位置
	float m_TailLimit;							//	後部連結位置
	float m_Length;								//	長さ
	float m_MaxVelocity;						//	最高速度
	float m_MaxAcceleration;					//	最高加速度
	float m_MaxDeceleration;					//	最高減速度
	float m_TiltSpeed;							//	振り子動作速度
	float m_DoorClosingTime;					//	ドア閉め所要時間
	list<CAxleObject> m_AxleObject;				//	車軸
	list<CBodyObject> m_BodyObject;				//	車体
	list<CFreeObjectContainer> m_FreeObject;	//	フリーオブジェクト
	CObjectJoint3D m_FrontCabin;				//	前部運転席位置
	CObjectJoint3D m_TailCabin;					//	後部運転席位置
public:
	static void RenderPreview();
	CTrainPlugin(char *id): CModelPlugin(id){}
	~CTrainPlugin();
	char *DirName(){ return "Train"; }
	char *TextName(){ return "Train.txt"; }
	char *TextName2(){ return "Train2.txt"; }
	bool Load();
	bool LoadOldForm();
	void SetPreview();
	void Preview(float, bool);
	CNamedObject *FindObject(const string &);
	bool IsSoundEnabled();
	void SetAxleList(CTrain *);
	void SetPosture();
	void AttachPartsObject();
	void ScanInput(CTrain *);
	void Render(CTrain *);
	void Simulate(CTrain *);
	CPLUGIN_CASTFUNC(CTrainPlugin);
};

/*
 *	車輌プラグインリスト
 */
class CTrainPluginList: public CModelPluginList{
private:
public:
	char *DirName(){ return "Train"; }
	char *TextName(){ return "Train.txt"; }
	char *TextName2(){ return "Train2.txt"; }
	char *Default(){ return "Aizentranza01"; }
	CPlugin *NewEntry(char *id){ return new CTrainPlugin(id); }
	CPLUGINLIST_CASTFUNC(CTrainPlugin);
};

//	外部グローバル
extern CTrainPlugin *g_Train;
extern CTrainPluginList *g_TrainPluginList;

#endif
