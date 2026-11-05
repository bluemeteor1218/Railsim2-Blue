#include "stdafx.h"
#include "CCamera.h"
#include "CInterface.h"
#include "CScene.h"
#include "CSurfacePlugin.h"
#include "CSkinPlugin.h"
#include "CNeutralMode.h"

//	外部定数
extern const float CLIP_PLANE_NEAR;
extern const float CLIP_PLANE_FAR;
extern const float FOV_DEF;

//	内部定数
extern const float CAM_PAN = 0.001f;		//	カメラパン
const float CAM_HEAD = 0.001f;				//	ヘッド感度
const float CAM_PITCH = 0.001f;				//	ピッチ感度
//const float CAM_BANK = 0.001f;			//	バンク感度
const float CAM_FOWARD = 0.0001f;			//	前後感度
const float CAM_ZOOM = 0.00002f;			//	ズーム感度
const float FOV_MIN = D3DXToRadian(0.1f);	//	最小視野角
const float FOV_MAX = D3DXToRadian(179.0f);	//	最大視野角
const int CAM_EDGE_MOVE = 20;				//	画面端移動速度

//	外部グローバル
extern bool g_HidefCaptureFlag;
extern int g_HidefBufferSize;
extern int g_HidefQuality;
extern float g_HidefLeft, g_HidefRight;
extern float g_HidefBottom, g_HidefTop;

//	内部グローバル
float g_FovRatio;	//	視野角率
float g_StereoInterval = 0.0f;
D3DCOLORVALUE g_LightColor = {1.0f, 1.0f, 1.0f, 0.0f};	//	光源色
MTX4 g_BoldLineMtx[8][8];

//	static メンバ
CCamera *CCamera::ms_CurrentCamera = NULL;

/*
 *	初期化
 */
void CCamera::Init(
	float defdist,	//	既定距離
	float mindist,	//	最小距離
	float maxdist,	//	最大距離
	bool clip		//	クリップ設定
){
	m_LeftState = m_MiddleState = m_RightState = 0;
	m_DefDist = defdist;
	m_MinDist = mindist;
	m_MaxDist = maxdist;
	m_ClipRect = clip;
	m_LockPos = false;
	m_AutoZoom = false;
	m_LocalFocus = false;
	m_FocusSpeed = 0.0f;
	ResetCamera();
	ResetLight();
}

/*
 *	カメラをリセット
 */
void CCamera::ResetCamera(
	float head, float pitch, float bank,	//	角度
	float dist								//	距離
){
	m_PrintInfoTime = 0;
	m_Head = head;
	m_Pitch = pitch;
//	m_Bank = bank;
	m_Dist = dist<0.0f ? m_DefDist : dist;
	m_FieldOfView = 0.25f*D3DX_PI;
	m_Wheel = 0.0f;
	SetCenter();
}

/*
 *	フォーカス移動先設定
 */
void CCamera::SetFocusTarget(
	VEC3 f, float s
){
	m_FocusTarget = f;
	m_FocusSpeed = s;
}

/*
 *	フォーカスインスタンス設定
 */
void CCamera::SetFocusInfo(
	CDetectInfo &info	//	フォーカス情報
){
	bool flag;
	if(flag = !GetFocusInst() && info.GetModelInst()){
		m_AutoZoomBaseDist = V3Len(&(GetVPos()-info.GetPartsInst()->GetCenter()));
	}else if(flag = GetFocusInst() && !info.GetModelInst()){
		m_FieldOfView = m_FieldOfViewEffect;
	}
	m_FocusInfo = info;
	if(flag || GetFocusInst()) InvCalcParam();
}

/*
 *	オートズーム設定
 */
void CCamera::SetAutoZoom(
	bool z	//	フラグ
){
	if(m_AutoZoom = z) m_AutoZoomBaseDist = m_Dist;
	else m_FieldOfView = m_FieldOfViewEffect;
	BeginPrint();
}

/*
 *	ローカル制御設定
 */
void CCamera::SetLocalFocus(
	int l,			//	制御モード
	CObject *tlocal	//	ローカル系直接指定
){
	m_LocalFocus = l;
	InvCalcParam(tlocal);
	BeginPrint();
}

/*
 *	ライトをリセット
 */
void CCamera::ResetLight(){
	m_LightSwitch = true;
	m_LinkLight = false;
	m_LightDir = VEC3(1.5f, -2.0f, -1.0f);
}

/*
 *	ライトを連動
 */
void CCamera::LinkLight(
	bool link	//	リンクフラグ
){
	if(link==m_LinkLight) return;
	if(link){
		m_LinkLight = true;
		m_LightDir = V3WorldToLocal(
			&m_LightDir, &GetVRight(), &GetVUp(), &GetVDir());
	}else{
		m_LinkLight = false;
		m_LightDir = V3LocalToWorld(
			&m_LightDir, &GetVRight(), &GetVUp(), &GetVDir());
	}
}

/*
 *	パラメータを逆計算
 */
void CCamera::InvCalcParam(
	CObject *tlocal	//	ローカル系直接指定
){
	VEC3 dir = m_Focus-m_CameraPos;
	V3Norm(&dir, &dir);
	if((GetFocusInst() || tlocal) && m_LocalFocus){
		VEC3 tr, tu = V3UP, td;
		CObject *fobj = tlocal ? tlocal : GetFocusParts()->GetObject();
		if(!tlocal && GetFocusObject()->GetType()==1){
			tr = fobj->GetRight(); td = VEC3(-tr.z, 0.0f, tr.x);
			V3NormAxis(&tr, &tu, &td);
			V3Norm(&dir, &V3WorldToLocal(&dir, &tr, &tu, &td));
		}else{
			switch(m_LocalFocus){
			case 1:
				td = fobj->GetDir();
				td.y = 0.0f;
				V3NormAxis(&tr, &tu, &td);
				V3Norm(&dir, &V3WorldToLocal(&dir, &tr, &tu, &td));
				break;
			case 2:
				V3Norm(&dir, &V3WorldToLocal(&dir, fobj));
				break;
			}
		}
	}
	m_Pitch = -asinf(dir.y);
	dir.y = 0.0f;
	V3Norm(&dir, &dir);
	m_Head = acosf(dir.x);
	if(dir.z<0.0f) m_Head = 2.0f*D3DX_PI-m_Head;
	m_Dist = V3Len(&(m_Focus-m_CameraPos));
}

/*
 *	選択
 */
void CCamera::Select(){
	ms_CurrentCamera = this;
	m_LeftState = m_MiddleState = m_RightState = 0;
	m_Wheel = m_UpDown = 0.0f;
	g_Cursor.Release();
	Apply(true);
}

/*
 *	カメラ設定
 */
void CCamera::Apply(
	bool light,		//	ライト設定フラグ
	CObject *tlocal	//	ローカル系直接指定
){
	if(GetFocusInst()){
		if(GetFocusInst()->GetScene()){
			if(GetFocusInst()->GetScene()!=g_Scene){
				g_Scene = GetFocusInst()->GetScene();
				*g_Scene->GetCamera() = *this;
				g_Scene->Enter(true);
				*g_Scene->GetCamera() = *this;
				g_NeutralMode->SetFocusInfo(m_FocusInfo);
				return;
			}
			g_NeutralMode->SetFocusInfo(m_FocusInfo);
		}else{
			CDetectInfo tmp_detect_info;
			g_NeutralMode->SetFocusInfo(tmp_detect_info);
		}
		if(GetFocusInst()->IsWarping()) return;
	}else{
		CDetectInfo tmp_detect_info;
		g_NeutralMode->SetFocusInfo(tmp_detect_info);
	}
	float sinpitch = sinf(m_Pitch), cospitch = cosf(m_Pitch);
	VEC3 pos, up = V3UP;
	VEC3 dir = VEC3(cos(m_Head)*cospitch, -sinpitch, sin(m_Head)*cospitch);
	if(m_LocalFocus){
		VEC3 tr, tu = V3UP, td;
		if(GetFocusInst() || tlocal){
			CObject *fobj = tlocal ? tlocal : GetFocusParts()->GetObject();
			if(!tlocal && GetFocusObject()->GetType()==1){
				tr = fobj->GetRight(); td = VEC3(-tr.z, 0.0f, tr.x);
				V3NormAxis(&tr, &tu, &td);
				V3Norm(&dir, &V3LocalToWorld(&dir, &tr, &tu, &td));
			}else{
				switch(m_LocalFocus){
				case 1:
					td = fobj->GetDir();
					td.y = 0.0f;
					V3NormAxis(&tr, &tu, &td);
					V3Norm(&dir, &V3LocalToWorld(&dir, &tr, &tu, &td));
					break;
				case 2:
					up = fobj->GetUp();
					if(up.y<0.0f) up = -up;
					V3Norm(&dir, &V3LocalToWorld(&dir, fobj));
					break;
				}
			}
		}
	}
	if(m_LockPos && (GetFocusInst() || tlocal)){
		pos = m_CameraPos;
		m_Focus = tlocal ? tlocal->GetPos() : GetFocusParts()->GetCenter();
		SetView(pos, m_Focus, V3UP);
		if(g_StereoInterval) MoveVW(g_StereoInterval*GetVRight());
		InvCalcParam(tlocal);
		dir = GetVDir();
	}else{
		if(GetFocusInst()) m_Focus = GetFocusParts()->GetCenter();
		else if(tlocal) m_Focus = tlocal->GetPos();
		else if(m_FocusSpeed)
			m_Focus = (1.0f-m_FocusSpeed)*m_Focus+m_FocusSpeed*m_FocusTarget;
		pos = m_Focus-m_Dist*dir;
		SetView(pos, m_Focus, up);
		if(g_StereoInterval) MoveVW(g_StereoInterval*GetVRight());
		m_CameraPos = GetVPos();
	}
	ValueArea(&m_FieldOfViewEffect, FOV_MIN, FOV_MAX);
//	RotVZ(m_Bank);
	if(light){
		if(m_LightSwitch){
			devSetAmbient(0xff808080);
			SetDirLight(m_LinkLight ? V3LocalToWorld(
				&m_LightDir, &GetVRight(), &GetVUp(), &GetVDir()) : m_LightDir,
				g_LightColor = MAKE_CV(1.0f, 1.0f, 1.0f, 0.0f));
		}else{
			devSetAmbient(0xff202020);
			SetDirLight(V3DIR, g_LightColor = MAKE_CV(0.0f, 0.0f, 0.0f, 1.0f));
		}
	}
	ApplyProjection(CLIP_PLANE_NEAR, tlocal);
}

/*
 *	プロジェクション設定
 */
void CCamera::ApplyProjection(
	float nearclip,	//	近距離クリップ
	CObject *tlocal	//	ローカル系直接指定
){
	D3DVIEWPORT8 vp;
	sv3.pDev->GetViewport(&vp);

	m_FieldOfViewEffect = (GetFocusInst() || tlocal) && m_AutoZoom
		? 2.0f*atanf(m_AutoZoomBaseDist*tanf(0.5f*m_FieldOfView)
		/V3Len(&(m_CameraPos-m_Focus))) : m_FieldOfView;
	g_FovRatio = tanf(0.5f*m_FieldOfViewEffect)/tanf(0.5f*FOV_DEF);
	float cliptmp = 1.0f/g_FovRatio;
	ValueArea(&cliptmp, 0.1f, 1.0f);
	float zn = nearclip*cliptmp, zf = CLIP_PLANE_FAR*cliptmp;
	if(g_HidefCaptureFlag){
		float vph = 2.0f*zn*tanf(0.5f*m_FieldOfViewEffect);
		float vpw = (vph*vp.Width)/vp.Height;
		D3DXMatrixPerspectiveOffCenterLH(&sv3.mtxProj,
			g_HidefLeft*vpw, g_HidefRight*vpw, g_HidefBottom*vph, g_HidefTop*vph, zn, zf);
		int bx, by;
		for(by = 0; by<g_HidefQuality; ++by){
			float ofsy = (g_HidefTop-g_HidefBottom)
				/g_HidefBufferSize*(by-(g_HidefQuality-1)*0.5f);
			for(bx = 0; bx<g_HidefQuality; ++bx){
				float ofsx = (g_HidefRight-g_HidefLeft)
					/g_HidefBufferSize*(bx-(g_HidefQuality-1)*0.5f);
				D3DXMatrixPerspectiveOffCenterLH(&g_BoldLineMtx[by][bx],
					(g_HidefLeft+ofsx)*vpw, (g_HidefRight+ofsx)*vpw,
					(g_HidefBottom+ofsy)*vph, (g_HidefTop+ofsy)*vph, zn, zf);
			}
		}
	}else{
		D3DXMatrixPerspectiveFovLH(&sv3.mtxProj, m_FieldOfViewEffect,
			(float)vp.Width/vp.Height, zn, zf);
	}
	sv3.pDev->SetTransform(D3DTS_PROJECTION, &sv3.mtxProj);
}

/*
 *	情報表示
 */
void CCamera::PrintInfo(
	bool ext	//	拡張制御情報表示
){
	if(m_PrintInfoTime<=0) return;
	static char *onoff[2] = {"OFF", "ON"};
	static char *local[3] = {"OFF", "HORZ", "ALL"};
	const int ERASE = MAXFPS/3;
	float alpha = m_PrintInfoTime>=ERASE ? 1.0f : (float)m_PrintInfoTime/ERASE;
	m_PrintInfoTime--;
	int ix = g_DispWidth*60/100, iy = g_DispHeight;
	int cx = ix+50, cy = iy-TILE_UNIT*2-TILE_HALF;
	int fx = Round(50*sinf(0.5f*m_FieldOfViewEffect));
	int fy = Round(50*cosf(0.5f*m_FieldOfViewEffect));
	devSetTexture(0, NULL);
	D3DCOLOR white = ScaleColor(0xffffffff, alpha), black = ScaleColor(0xff000000, alpha);
	D3DCOLOR white2 = ScaleColor(0x80ffffff, alpha), black2 = ScaleColor(0x80000000, alpha);
	int deg;
	for(deg = 0; deg<=180; deg += 15){
		float rad = D3DXToRadian(deg);
		int dx1 = Round(50*cosf(rad)), dy1 = Round(50*sinf(rad));
		int dx2 = Round(40*cosf(rad)), dy2 = Round(40*sinf(rad));
		Draw2DLine(cx+dx1+1, cy-dy1+1, cx+dx2+1, cy-dy2+1, black2, 0x01000000);
		Draw2DLine(cx+dx1, cy-dy1, cx+dx2, cy-dy2, white2, 0x01ffffff);
	}
	Draw2DLine(cx+1, cy+1, cx+fx+1, cy-fy+1, black, 0x01000000);
	Draw2DLine(cx+1, cy+1, cx-fx+1, cy-fy+1, black, 0x01000000);
	Draw2DLine(cx, cy, cx+fx, cy-fy, white, 0x01ffffff);
	Draw2DLine(cx, cy, cx-fx, cy-fy, white, 0x01ffffff);
	g_StrTex->RenderLeft(ix, iy-TILE_UNIT*2, white, black,
		FlashIn("%s: %.1f [deg]", lang(FieldOfView), D3DXToDegree(m_FieldOfViewEffect)));
	g_StrTex->RenderLeft(ix, iy-TILE_UNIT, white, black,
		FlashIn("%s: %.1f [m]", lang(Distance), m_Dist));
	if(ext){
		g_StrTex->RenderLeft(ix+110, iy-TILE_UNIT*3, white, black,
			FlashIn("%s: %s", lang(FixViewpoint), onoff[m_LockPos]));
		g_StrTex->RenderLeft(ix+110, iy-TILE_UNIT*2, white, black,
			FlashIn("%s: %s", lang(AutoZoom), onoff[m_AutoZoom]));
		g_StrTex->RenderLeft(ix+110, iy-TILE_UNIT, white, black,
			FlashIn("%s: %s", lang(Local), local[m_LocalFocus]));
	}
}

/*
 *	ローカル切替入力チェック
 */
void CCamera::ControlLocal(
	CObject *tlocal	//	ローカル系直接指定
){
	if(GetKey(DIK_HOME)==S_PUSH){
		if(CheckCtrl()) SetLockPos(!GetLockPos());
		else SetLocalFocus((GetLocalFocus()+(CheckShift() ? 2 : 1))%3, tlocal);
	}
	if(GetKey(DIK_END)==S_PUSH) SetAutoZoom(!GetAutoZoom());
}

/*
 *	入力チェック
 */
int CCamera::ScanInput(
	int mode,		//	モード (0: pan, 1: slide, 2: arrow, 3: slide(middle), 4: zoom only)
	CObject *tlocal	//	ローカル系直接指定
){
	bool clipping = false;
	int ret = 0, wh = CInterface::GetFocus() ? 0 : GetWheel();
	if(wh){
		if(wh*m_Wheel<0.0f) m_Wheel = 0.0f;
		m_Wheel = m_Wheel*1.5f+wh;
		ret = 1;
	}else{
		if(GetKey(DIK_PRIOR)>=S_PUSH) wh += 5;
		if(GetKey(DIK_NEXT)>=S_PUSH) wh -= 5;
		if(wh){
			if(wh*m_Wheel<0.0f) m_Wheel = 0.0f;
			m_Wheel = m_Wheel*1.1f+wh;
			ret = 1;
		}else{
			m_Wheel *= 0.5f;
		}
	}
	float ratio = CheckFast();
	if(wh) BeginPrint();
	if(CheckShift()){
		if(CheckCtrl()){
			m_FieldOfView = 5.f*Round(D3DXToDegree(m_FieldOfView/5));
			if(wh) m_FieldOfView += wh>0 ? -5.0f : 5.0f;
			m_FieldOfView = D3DXToRadian(m_FieldOfView);
		}else{
			m_FieldOfView -= m_Wheel*CAM_ZOOM*ratio*sqrtf(m_FieldOfView);
		}
		ValueArea(&m_FieldOfView, FOV_MIN, FOV_MAX);
	}else if((!m_LockPos || !GetFocusInst() && !tlocal) && mode!=4){
		m_Dist -= m_Dist*m_Wheel*CAM_FOWARD*ratio;
		ValueArea(&m_Dist, m_MinDist/g_FovRatio, m_MaxDist/g_FovRatio);
	}
	if(mode==4) return 0;
	POINT delta = g_Cursor.GetDelta();
	bool is_delta = delta.x || delta.y;
	switch(GetButton(DIM_LEFT)){
	case S_PUSH:
		if(g_Cursor.IsLock()) break;
		if(mode!=2){
			m_LeftState = 2;
			if(mode!=3) g_Cursor.Lock();
			CInterface::SetFocus(NULL);
		}
		ret = 10;
		break;
	case S_HOLD:
		if(m_MiddleState || m_RightState) break;
		switch(m_LeftState){
		case 2:
			if(!g_Cursor.CheckDrag()){
				ret = 11;
				break;
			}
			m_LeftState = 1;
		case 1:
			ret = 11;
			if(mode==3){
				POINT pos = g_Cursor.GetPos(), tmp = {0, 0};
				if(pos.x<=0) tmp.x = -CAM_EDGE_MOVE;
				if(pos.x>=g_DispWidth-1) tmp.x = CAM_EDGE_MOVE;
				if(pos.y<=0) tmp.y = -CAM_EDGE_MOVE;
				if(pos.y>=g_DispHeight-1) tmp.y = CAM_EDGE_MOVE;
				VEC3 fow = GetVFow();
				fow = (GetVRight()*tmp.x-fow*tmp.y)*(ratio*m_Dist*CAM_PAN);
				m_Focus += fow;
			}else{
				Slide(mode, tlocal);
			}
			break;
		}
		if(is_delta) clipping = true;
		break;
	default:
		switch(m_LeftState){
		case 2:
			ret = 12;
		case 1:
			m_LeftState = 0;
			if(!m_MiddleState && !m_RightState) g_Cursor.Release();
			break;
		}
		break;
	}
	switch(GetButton(DIM_MIDDLE)){
	case S_PUSH:
		if(g_Cursor.IsLock()) break;
		if(mode!=2){
			m_MiddleState = 2;
			g_Cursor.Lock();
			CInterface::SetFocus(NULL);
		}
		ret = 20;
		break;
	case S_HOLD:
		if(m_LeftState || m_RightState) break;
		switch(m_MiddleState){
		case 2:
			if(!g_Cursor.CheckDrag()){
				ret = 21;
				break;
			}
			m_MiddleState = 1;
		case 1:
			ret = 21;
			Slide(mode, tlocal);
			break;
		}
		if(is_delta) clipping = true;
		break;
	default:
		switch(m_MiddleState){
		case 2:
			ret = 22;
		case 1:
			m_MiddleState = 0;
			if(!m_LeftState && !m_RightState) g_Cursor.Release();
			break;
		}
		break;
	}
	switch(GetButton(DIM_RIGHT)){
	case S_PUSH:
	//	if(g_Cursor.IsLock()) break;
		m_RightState = 2;
		g_Cursor.Lock();
		CInterface::SetFocus(NULL);
		ret = 30;
		break;
	case S_HOLD:
		switch(m_RightState){
		case 2:
			if(!g_Cursor.CheckDrag()){
				ret = 31;
				break;
			}
			m_RightState = 1;
		case 1:
			ret = 31;
			if(!m_LockPos || !GetFocusInst() && !tlocal){
				m_Head -= delta.x*CAM_HEAD;
				m_Pitch += delta.y*CAM_PITCH;
				ValueCircular(&m_Head, 0.0f, 2.0f*D3DX_PI);
				ValueArea(&m_Pitch, -0.49f*D3DX_PI, 0.49f*D3DX_PI);
			}
			break;
		}
		break;
	default:
		switch(m_RightState){
		case 2:
			ret = 32;
		case 1:
			m_RightState = 0;
			if(!m_LeftState && !m_MiddleState) g_Cursor.Release();
			break;
		}
		break;
	}
	if(m_ClipRect && clipping) Surface()->ClipRect(&m_Focus);
	return ret;
}

/*
 *	スライド操作
 */
void CCamera::Slide(
	int mode,		//	モード (0: pan, 1: slide, 2: arrow, 3: slide(middle))
	CObject *tlocal	//	ローカル系直接指定
){
	float ratio = CheckFast();
	POINT delta = g_Cursor.GetDelta();
	VEC3 *target = GetFocusInst() || tlocal ? &m_CameraPos : &m_Focus;
	if(delta.x || delta.y) m_FocusSpeed = 0.0f;
	switch(mode){
	case 0: {
		VEC3 mov = CheckShift() ? GetVDir()*(ratio*delta.y*m_Dist*CAM_PAN)
			: -(GetVRight()*delta.x-GetVUp()*delta.y)*(ratio*m_Dist*CAM_PAN);
		*target += mov;
		break; }
	case 1:
	case 3:
		if(CheckShift()){
			float h = ratio*delta.y*m_Dist*CAM_PAN;
			target->y -= h;
		}else{
			VEC3 fow = GetVFow();
			fow = (GetVRight()*delta.x-fow*delta.y)*(ratio*m_Dist*CAM_PAN);
			*target += fow;
		}
		break;
	}
	if((GetFocusInst() || tlocal) && !m_LockPos) InvCalcParam(tlocal);
}

/*
 *	読込
 */
char *CCamera::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Camera"))) return NULL;
	if(!(str = AsgnFloat(eee = str, "Head", &m_Head))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Pitch", &m_Pitch))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "Dist", &m_Dist))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "FieldOfView", &m_FieldOfView))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "Focus", &m_Focus))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	保存
 */
void CCamera::Save(
	FILE *df,		//	ファイル
	string indent	//	インデント
){
	fprintf(df, "%sCamera{\n", indent.c_str());
	fprintf(df, "%s\tHead = %f;\n", indent.c_str(), m_Head);
	fprintf(df, "%s\tPitch = %f;\n", indent.c_str(), m_Pitch);
	fprintf(df, "%s\tDist = %f;\n", indent.c_str(), m_Dist);
	fprintf(df, "%s\tFieldOfView = %f;\n", indent.c_str(), m_FieldOfView);
	fprintf(df, "%s\tFocus = ", indent.c_str()); V3Save(df, m_Focus, ";\n");
	fprintf(df, "%s}\n", indent.c_str());
}
