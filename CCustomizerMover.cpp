#include "stdafx.h"
#include "CCustomizerMover.h"
#include "CSaveFile.h"
#include "CConfigMode.h"

//	内部グローバル
bool g_PreviewAnimation = true;	//	プレビュー時アニメーションフラグ (編成プレビューでは false)
bool g_MoverEnabled = true;		//	ムーバー有効フラグ (ポーズ時スイッチ適用処理では false)

/*
 *	読込
 */
char *CMoverState::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = Assignment(eee = str, "MoverState"))) return NULL;
	if(!(str = Vector3D(eee = str, &m_Pos))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ','))) throw CSynErr(eee);
	if(!(str = Vector3D(eee = str, &m_Dir))) throw CSynErr(eee);
	if(!(str = Character2(eee = str, ';'))) throw CSynErr(eee);
	return str;
}

/*
 *	保存
 */
void CMoverState::Save(
	FILE *df,	//	ファイル
	char *ind	//	インデント
){
	fprintf(df, "%sMoverState = ", ind);
	V3Save(df, m_Pos, ", ");
	V3Save(df, m_Dir, ";\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
void CMoverBase::InitMover(
	CModelPlugin *mpi	//	呼び出し元
){
	m_State = mpi->AddMover();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	適用フラグリセット
 */
void CDynamicMoverBase::ResetOffCustomizer(
	int v	//	設定値
){
	/*
	 *	m_ApplyFlag の意味
	 *	0: 未適用
	 *	1: スイッチ条件真で適用済み
	 *	2: スイッチ条件偽で未適用
	 */
	m_ApplyFlag = v ? (m_ApplyFlag ? m_ApplyFlag : v) : 0;
}

/*
 *	常時適用リスト作成
 */
void CDynamicMoverBase::SetOffListCustomizer(
	CNamedObject *nobj	//	呼び出し元
){
	nobj->AddOffList(this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CStaticMoverBase::ReadTimingInfo(
	char *str	//	対象文字列
){
	char *tmp, *eee;
	char *timinglabel[6] = {
		"PreAnimationDelay", "AnimationTime", "PostAnimationDelay",
		"PreReverseDelay", "ReverseTime", "PostReverseDelay"};
	int i;
	for(i = 0; i<6; i++){
		if(tmp = AsgnFloat(eee = str, timinglabel[i], &m_AnimationTime[i])) str = tmp;
		else m_AnimationTime[i] = i<3 ? 0.0f : m_AnimationTime[5-i];
		if(m_AnimationTime[i]<0.0f) m_AnimationTime[i] = 0.0f;
	}
	return str;
}

/*
 *	前遅延処理
 *
 *	戻り値: クレジット
 */
float CStaticMoverBase::ProcDelay(){
	if(!g_PreviewAnimation) return m_ApplyFlag ? 0.0f : 1.0f;
	CMoverState *mstate = GetState();
	VEC3 pos = mstate->GetPos();
	float *timing = m_AnimationTime, credit = 1.0f;
	if(m_ApplyFlag){	//	逆方向
		pos.x = 3.0f-pos.x;
		timing += 3;
	}
	int i;
	for(i = 0; i<3; i++){
		float th = i+1.0f;
		if(pos.x<th){
			if(timing[i]){
				float tmp = 1.0f/(timing[i]*MAXFPS);
				if(g_MoverEnabled) pos.x += credit*tmp;
				credit = (pos.x-th)/tmp;
				ValueArea(&pos.x, i, th);
				if(credit<=0.0f) break;
				if(credit>1.0f) credit = 1.0f;
			}else{
				pos.x = th;
			}
		}
	}
	if(m_ApplyFlag) pos.x = 3.0f-pos.x;
	if(g_MoverEnabled) mstate->SetPos(pos);
	pos.x -= 1.0f;
	ValueArea(&pos.x, 0.0f, 1.0f);
	return pos.x;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CStaticRotator::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "StaticRotation"))) return NULL;
	if(tmp = AsgnVector3D(eee = str, "RotationAxis", &m_RotationAxis)) str = tmp;
	else m_RotationAxis = V3DIR;
	if(!(str = AsgnFloat(eee = str, "RotationAngle", &m_RotationAngle))) throw CSynErr(eee);
	m_RotationAngle = D3DXToRadian(m_RotationAngle);
	if(!(str = ReadTimingInfo(str))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	InitMover(mpi);
	return str;
}

/*
 *	姿勢適用
 */
void CStaticRotator::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	if(m_ApplyFlag==1) return;
	float ratio = ProcDelay();
	if(ratio) obj->RotAxis(m_RotationAxis, ratio*m_RotationAngle);
	if(!m_ApplyFlag) m_ApplyFlag = 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CStaticMover::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *eee;
	if(!(str = BeginBlock(eee = str, "StaticMove"))) return NULL;
	if(!(str = AsgnVector3D(eee = str, "Displacement", &m_Displacement))) throw CSynErr(eee);
	if(!(str = ReadTimingInfo(str))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	InitMover(mpi);
	return str;
}

/*
 *	姿勢適用
 */
void CStaticMover::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	if(m_ApplyFlag==1) return;
	float ratio = ProcDelay();
	if(ratio) obj->Move(ratio*m_Displacement);
	if(!m_ApplyFlag) m_ApplyFlag = 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CDynamicRotator::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "DynamicRotation"))) return NULL;
	if(tmp = AsgnVector3D(eee = str, "RotationAxis", &m_RotationAxis)) str = tmp;
	else m_RotationAxis = V3DIR;
	if(!(str = AsgnFloat(eee = str, "RotationSpeed", &m_RotationSpeed))) throw CSynErr(eee);
	m_RotationSpeed *= 2.0f*D3DX_PI/MAXFPS;
	if(tmp = AsgnFloat(eee = str, "Acceleration", &m_Acceleration)){
		str = tmp;
		if(m_Acceleration<0.0f) m_Acceleration = m_RotationSpeed;
		else m_Acceleration *= 2.0f*D3DX_PI/(MAXFPS*MAXFPS);
	}else{
		m_Acceleration = m_RotationSpeed;
	}
	if(tmp = AsgnFloat(eee = str, "Deceleration", &m_Deceleration)){
		str = tmp;
		if(m_Deceleration<0.0f) m_Deceleration = m_Deceleration;
		else m_Deceleration *= 2.0f*D3DX_PI/(MAXFPS*MAXFPS);
	}else{
		m_Deceleration = m_RotationSpeed;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	InitMover(mpi);
	return str;
}

/*
 *	姿勢適用
 */
void CDynamicRotator::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	if(m_ApplyFlag==1 || !g_PreviewAnimation) return;
	CMoverState *mstate = GetState();
	VEC3 pos = mstate->GetPos();	//	x: phase, y: speed
	if(g_MoverEnabled){
		if(m_RotationSpeed<0.0f){
			pos.y -= (m_ApplyFlag ? -m_Deceleration : m_Acceleration);
			ValueArea(&pos.y, m_RotationSpeed, 0.0f);
		}else{
			pos.y += (m_ApplyFlag ? -m_Deceleration : m_Acceleration);
			ValueArea(&pos.y, 0.0f, m_RotationSpeed);
		}
		pos.x += pos.y;
	}
	ValueCircular(&pos.x, 0.0f, 2.0f*D3DX_PI);
	mstate->SetPos(pos);
	obj->RotAxis(m_RotationAxis, pos.x);
	if(!m_ApplyFlag) m_ApplyFlag = 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CWindTracker::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "TrackWind"))) return NULL;
	if(!(str = AsgnFloat(eee = str, "TrackSpeed", &m_TrackSpeed))) throw CSynErr(eee);
	if(tmp = AsgnVector3D(eee = str, "FixAxis", &m_FixAxis)){
		str = tmp;
		V3Norm(&m_FixAxis, &m_FixAxis);
		m_FixAxisFlag = true;
	}else{
		m_FixAxisFlag = false;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	InitMover(mpi);
	return str;
}

/*
 *	姿勢適用
 */
void CWindTracker::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	CMoverState *mstate = GetState();
	VEC3 tr = obj->GetRight(), tu = obj->GetUp(), td = obj->GetDir();
	VEC3 odir = V3LocalToWorld(&mstate->GetDir(), &tr, &tu, &td);
	if(g_MoverEnabled){
		VEC3 wind = g_ConfigMode->GetWind() ? g_WindDir : V3ZERO;
		VEC3 pos = obj->GetPos();
		wind += mstate->GetPos()-pos;
		mstate->SetPos(pos);
		VEC3 ndir = odir+m_TrackSpeed*wind;
		V3Norm(&ndir, &ndir);
		if(m_FixAxisFlag){
			VEC3 axis = V3LocalToWorld(&m_FixAxis, obj);
			V3Norm(&ndir, &(ndir-V3Dot(&ndir, &axis)*axis));
		}
		obj->SetDir(ndir, obj->GetUp());
		mstate->SetDir(*V3Norm(&ndir, &V3WorldToLocal(&obj->GetDir(), &tr, &tu, &td)));
	}else{
		obj->SetDir(odir, obj->GetUp());
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CWindmill::Read(
	char *str,			//	対象文字列
	CModelPlugin *mpi	//	呼び出し元
){
	char *tmp, *eee;
	if(!(str = BeginBlock(eee = str, "Windmill"))) return NULL;
	if(tmp = AsgnYesNo(eee = str, "Directional", &m_Directional)) str = tmp;
	else m_Directional = true;
	if(tmp = AsgnVector3D(eee = str, "RotationAxis", &m_RotationAxis)) str = tmp;
	else m_RotationAxis = V3DIR;
	if(!(str = AsgnFloat(eee = str, "RotationSpeed", &m_RotationSpeed))) throw CSynErr(eee);
	m_RotationSpeed *= 2.0f*D3DX_PI;
	if(!(str = AsgnInteger(eee = str, "Symmetric", &m_Symmetric))) throw CSynErr(eee);
	if(m_Symmetric<1) m_Symmetric = 1;
	m_MaxRotation = SYMMETRIC_ROTATION_MAX/m_Symmetric;
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	InitMover(mpi);
	return str;
}

/*
 *	姿勢適用
 */
void CWindmill::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	CMoverState *mstate = GetState();
	VEC3 odir = mstate->GetDir();
	if(g_MoverEnabled){
		VEC3 wind = g_ConfigMode->GetWind() ? g_WindDir : V3ZERO;
		VEC3 pos = obj->GetPos();
		wind += mstate->GetPos()-pos;
		mstate->SetPos(pos);
		float rot = m_RotationSpeed;
		if(m_Directional){
			VEC3 axis = V3LocalToWorld(&m_RotationAxis, obj);
			rot *= V3Dot(V3Norm(&axis, &axis), &wind);
		}else{
			rot *= V3Len(&wind);
		}
		float tmp = m_MaxRotation*(1.0f-expf(-fabsf(rot)/m_MaxRotation));
		odir.x += rot<0.0f ? -tmp : tmp;
		ValueCircular(&odir.x, 0.0f, 2.0f*D3DX_PI);
		obj->RotAxis(m_RotationAxis, odir.x);
		mstate->SetDir(odir);
	}else{
		obj->RotAxis(m_RotationAxis, odir.x);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	読込
 */
char *CAnalogClock::Read(
	char *str	//	対象文字列
){
	char *eee;
//	if(!(str = BeginBlock(eee = str, "AnalogClock"))) return NULL;
	string handtype;
	if(!(str = AsgnIdentifier(eee = str, "AnalogClock", &handtype))) return NULL;
	if(handtype=="Hour") m_HandType = 0;
	else if(handtype=="Minute") m_HandType = 1;
	else if(handtype=="Second") m_HandType = 2;
	else throw CSynErr(eee, "%s: \"%s\"", lang(InvalidHandType), handtype.c_str());
//	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	姿勢適用
 */
void CAnalogClock::SetPostureCustomizer(
	CObject *obj	//	オブジェクト
){
	double theta = 2.0f*D3DX_PI;
	double time = g_RSPV ? 0.0 : g_SaveFile->GetAbsTime();
	switch(m_HandType){
	case 0: theta *= 2.0*fmod(time, 1.0/2.0); break;
	case 1: theta *= 24.0*fmod(time, 1.0/24.0); break;
	case 2: theta *= 24.0*60.0*fmod(time, 1.0/(24.0*60.0)); break;
	}
	obj->RotZ((float)theta);
}
