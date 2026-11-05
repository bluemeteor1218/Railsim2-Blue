#ifndef CCAMERA_H_INCLUDED
#define CCAMERA_H_INCLUDED

#include "CModelInst.h"

/*
 *	カメラ
 */
class CCamera{
private:
	static CCamera *ms_CurrentCamera;	//	現在のカメラ
	int m_LeftState;					//	左ボタン状態
	int m_MiddleState;					//	中ボタン状態
	int m_RightState;					//	右ボタン状態
	int m_PrintInfoTime;				//	情報表示時間
	float m_Head, m_Pitch;				//	角度
//	float m_Bank;						//	バンク未使用
	float m_Dist;						//	距離
	float m_Wheel;						//	ホイール
	float m_UpDown;						//	上下移動
	float m_DefDist;					//	既定距離
	float m_MinDist, m_MaxDist;			//	最小・最大距離
	float m_FieldOfView;				//	視野角
	float m_FieldOfViewEffect;			//	実効視野角
	float m_AutoZoomBaseDist;			//	自動ズーム基準距離
	float m_FocusSpeed;					//	フォーカス捕捉速度
	bool m_LightSwitch;					//	照明 ON/OFF
	bool m_LinkLight;					//	照明連動
	bool m_ClipRect;					//	クリップ設定
	bool m_LockPos;						//	位置固定
	bool m_AutoZoom;					//	自動ズーム
	int m_LocalFocus;					//	ローカル制御
	VEC3 m_Focus;						//	注視点
	VEC3 m_FocusTarget;					//	フォーカス移動先
	VEC3 m_CameraPos;					//	カメラ座標
	VEC3 m_LightDir;					//	照明角度
	CDetectInfo m_FocusInfo;			//	注目情報
public:
	static CCamera *GetCurrentCamera(){ return ms_CurrentCamera; }
	void Init(float, float, float, bool);
	void ResetCamera(float head = -0.25f*D3DX_PI,
		float pitch = 0.25f*D3DX_PI, float bank = 0.0f, float dist = -1.0f);
	void SetCenter(){ m_Focus = V3ZERO; }
	float GetFieldOfView(){ return m_FieldOfView; }
	VEC3 GetFocus(){ return m_Focus; }
	void SetFocus(VEC3 f){ m_Focus = f; }
	void SetFocusTarget(VEC3, float);
	CNamedObject *GetFocusObject(){ return m_FocusInfo.GetObject(); }
	CModelInst *GetFocusInst(){ return m_FocusInfo.GetModelInst(); }
	CPartsInst *GetFocusParts(){ return m_FocusInfo.GetPartsInst(); }
	void SetFocusInfo(CDetectInfo &);
	bool GetLockPos(){ return m_LockPos; }
	void SetLockPos(bool l){ m_LockPos = l; BeginPrint(); }
	bool GetAutoZoom(){ return m_AutoZoom; }
	void SetAutoZoom(bool);
	int GetLocalFocus(){ return m_LocalFocus; }
	void SetLocalFocus(int, CObject *tlocal = NULL);
	float GetDist(){ return m_Dist; }
	void ResetLight();
	void SwitchLight(bool f){ m_LightSwitch = f; }
	bool IsLightOn(){ return m_LightSwitch; }
	void LinkLight(bool);
	bool IsLightLinked(){ return m_LinkLight; }
	void InvCalcParam(CObject *tlocal = NULL);
	void Select();
	void Apply(bool, CObject *tlocal = NULL);
	void ApplyProjection(float, CObject *tlocal = NULL);
	void BeginPrint(){ m_PrintInfoTime = MAXFPS*2; }
	void PrintInfo(bool ext = false);
	void ControlLocal(CObject *tlocal = NULL);
	int ScanInput(int, CObject *tlocal = NULL);
	void Slide(int, CObject *tlocal = NULL);
	char *Read(char *);
	void Save(FILE *, string indent = "\t\t");
};

//	外部グローバル
extern float g_FovRatio;

#endif
