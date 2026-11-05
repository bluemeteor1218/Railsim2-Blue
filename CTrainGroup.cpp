#include "stdafx.h"
#include "RailMap.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CSaveFile.h"
#include "CRailConnector.h"
#include "CRailWay.h"
#include "CTrain.h"
#include "CTrainGroup.h"
#include "CStation.h"
#include "CTrainPlugin.h"
#include "CSkinPlugin.h"
#include "CListView.h"
#include "CPluginTree.h"
#include "CTrainGroupTemplate.h"
#include "CSimulationMode.h"

//	関数宣言
void PushUndoStack();
void EnqueueMergeTrainControl(void *, void *, int, int);

//	外部グローバル
extern bool g_IgnoreAcceleration;
extern CGroupEndLocator g_HitTrainGroup;

//	内部定数
const float CABIN_POS_MOVE = 0.005f;				//	運転席位置調整速度
const float CABIN_POS_MAX = 10.0f;					//	運転席位置最大変位
const float CABIN_DIR_MOVE = 0.005f;				//	運転席視点方向調整速度
const float CABIN_DIR_MAX = 2.0f;					//	運転席視点方向最大変位
const float SPEED_CONTROL_DELTA = 1.0f;				//	速度設定幅
const float FINAL_ACCELERATION = 0.5f;				//	最終加速度率
const float STOP_LIMIT_LINE = 1.0f;					//	停止余裕距離
extern const float GROUP_PREVIEW_SPEED = 0.1f;		//	編成プレビュー移動速度
const double FRAME_PER_DAY = 24.0*60.0*60.0*30.0;	//	1 日当たりフレーム数

//	内部グローバル
CTrain *g_CabinViewTrain = NULL;	//	運転席適用車輌
bool g_UpdateTrainGroupList = false;

void NetSyncSetTrain(
	void *set_group, void *set_rail, int set_side, float set_sumlen, int set_flag
){
	if(!set_group || !g_AddressMap.count(set_group))
		ErrorDialog("NetSyncSetTrain set_group error.");
	if(set_rail && !g_AddressMap.count(set_rail))
		ErrorDialog("NetSyncSetTrain set_rail error.");
	CTrainGroup *group = (CTrainGroup *)g_AddressMap[set_group];
	if(set_rail){
		CRailWay *rail = (CRailWay *)g_AddressMap[set_rail];
		CRailLinkTemp link(set_side, set_sumlen, R2L(V3ZERO), R2L(V3ZERO), R2L(V3ZERO), R2L(V3ZERO), rail, IRailSplitter());
		group->Set(&link, set_flag);
	}else{
		group->Remove();
	}
}

void NetSyncMergeTrain(
	void *merge_group1, void *merge_group2, int merge_side1, int merge_side2
){
	if(!merge_group1 || !g_AddressMap.count(merge_group1))
		ErrorDialog("NetSyncMergeTrain merge_group1 error.");
	if(merge_group2 && !g_AddressMap.count(merge_group2))
		ErrorDialog("NetSyncMergeTrain merge_group2 error.");
	CTrainGroup *group1 = (CTrainGroup *)g_AddressMap[merge_group1];
	CTrainGroup *group2 = (CTrainGroup *)g_AddressMap[merge_group2];
	if(group2) group1->MergeTrain(group2, merge_side1, merge_side2);
	else group1->SplitTrain(merge_side1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CTrainGroup::CTrainGroup(
	char *name	//	名称
){
	m_OldAdr = NULL;
	m_Name = name;
	m_ListElement = NULL;
	m_Enabled = false;
	m_Reverse = 0;
	m_State = 0;
	m_ControlState = 0;
	m_DoorWait = 0;
	m_OpenDoor[0] = m_OpenDoor[1] = false;
	m_Platform = NULL;
	m_TrainList = NULL;
	m_Length = 0.0f;
	m_MaxVelocity = m_MaxAcceleration = m_MaxDeceleration = 0.0f;
	m_DoorClosingTime = 0.0f;
	m_TargetSpeed = m_EffectTargetSpeed = m_CurrentSpeed = m_OldSpeed = 0.0f;
	m_StopTarget = 0.0f;
	m_PreviewOffset = 0.0f;
	m_SplitPos = -1;
	m_Seeker.m_Type = 0;
	m_Seeker.m_Group = this;
	int i;
	for(i = 0; i<2; i++){
		m_Location[i].m_Group = this;
		m_CabinPos[i] = V3ZERO;
		m_CabinDir[i] = V3DIR;
	}
	m_Next = NULL;
}

/*
 *	デストラクタ
 */
CTrainGroup::~CTrainGroup(){
	DELETE_V(m_TrainList);
	DELETE_V(m_Next);
}

/*
 *	テンプレート作成
 */
void CTrainGroup::MakeTemplate(
	CTrainGroupTemplate *tgt	//	テンプレート
){
	CTrain *ptr = m_TrainList;
	while(ptr){
		tgt->PushTrain(R2L(CTrainTemplate(
			ptr->m_TrainPlugin, ptr->m_Reverse, ptr->m_SwitchOption)));
		ptr = ptr->m_Next;
	}
	tgt->SetLoaded();
}

/*
 *	車輌数取得
 */
int CTrainGroup::GetTrainNum(){
	CTrain *ptr = m_TrainList;
	int num = 0;
	while(ptr){
		num++;
		ptr = ptr->m_Next;
	}
	return num;
}

/*
 *	車輌数変更時処理
 */
void CTrainGroup::OnTrainNumChanged(){
	if(m_ListElement) m_ListElement->SetString(1, FlashIn("%d", GetTrainNum()));
	CalcSpec();
}

/*
 *	順序取得
 */
vector<CTrain *> CTrainGroup::GetTrainByVector(){
	vector<CTrain *> train_list;
	CTrain *ptr = m_TrainList;
	while(ptr){
		train_list.push_back(ptr);
		ptr = ptr->Next();
	}
	return train_list;
}

/*
 *	編成順序変更
 */
void CTrainGroup::SetTrainByVector(vector<CTrain *> train_list){
	int i;
	m_TrainList = train_list.size() ? train_list[0] : NULL;
	for(i = 0; i<train_list.size(); ++i)
		*train_list[i]->NextAdr() = i<train_list.size()-1 ? train_list[i+1] : NULL;
	OnTrainNumChanged();
}

/*
 *	性能計算
 */
void CTrainGroup::CalcSpec(){
	m_MaxVelocity = m_MaxAcceleration = m_MaxDeceleration = 0.0f;
	m_DoorClosingTime = 0.0f;
	CTrain *ptr = m_TrainList;
	while(ptr){
		CTrainPlugin *tpi = ptr->m_TrainPlugin;
		if(tpi){
			if(m_MaxVelocity<tpi->m_MaxVelocity) m_MaxVelocity = tpi->m_MaxVelocity;
			if(m_MaxAcceleration<tpi->m_MaxAcceleration)
				m_MaxAcceleration = tpi->m_MaxAcceleration;
			if(m_MaxDeceleration<tpi->m_MaxDeceleration)
				m_MaxDeceleration = tpi->m_MaxDeceleration;
			if(m_DoorClosingTime<tpi->m_DoorClosingTime)
				m_DoorClosingTime = tpi->m_DoorClosingTime;
		}
		ptr = ptr->m_Next;
	}
	if(m_MaxDeceleration<0.1f) m_MaxDeceleration = 0.1f;
}

/*
 *	速度設定確認＆修正
 */
void CTrainGroup::CheckTargetSpeed(){
	ValueArea(&m_TargetSpeed, -m_MaxVelocity, m_MaxVelocity);
	ValueArea(&m_EffectTargetSpeed, -m_MaxVelocity, m_MaxVelocity);
}

/*
 *	速度情報表示
 */
void CTrainGroup::PrintInfo(){
//	g_StrTex->RenderLeft(TILE_QUAD, g_DispHeight-FONT_HEIGHT*3-TILE_QUAD,
//		0xffff8000, 0xff000000, FlashIn("SEEK %f / %f", m_SeekResult, m_SeekDist));
	g_StrTex->RenderLeft(TILE_QUAD, g_DispHeight-FONT_HEIGHT*2-TILE_QUAD,
		0xffffffff, 0xff000000, FlashIn("%s (%d/%d): %s", lang(Consist),
		m_Serial+1, g_SaveFile->GetGroupNum(), m_Name.c_str()));
	if(m_Enabled) g_StrTex->RenderLeft(TILE_QUAD, g_DispHeight-FONT_HEIGHT-TILE_QUAD,
		0xffffffff, 0xff000000, FlashIn("%s: %.1f / %.1f [km/h]", lang(Velocity),
		m_CurrentSpeed, m_TargetSpeed));
	else  g_StrTex->RenderLeft(TILE_QUAD, g_DispHeight-FONT_HEIGHT-TILE_QUAD,
		0xffffffff, 0xff000000, lang(NotSet));
}

/*
 *	速度操作
 */
CModelInst *CTrainGroup::Control(
	CTrain *src	//	呼び出し元
){
	if(!m_Enabled) return NULL;
	if(CEditBox::IsActive()) return src;
	if(GetKey(DIK_DELETE)==S_PUSH){
		if(g_NetworkInitialized){
			void EnqueueSetTrainControl(void *, void *, int, float, int);
			EnqueueSetTrainControl(m_OldAdr, NULL, 0, 0.0f, 0);
		}else{
			PushUndoStack();
			Remove();
		}
		return NULL;
	}
	if(GetKey(DIK_BACK)==S_PUSH){
		m_TargetSpeed = -m_TargetSpeed;
		return src;
	}
	const int SPD_CTRL = 20;
	float delta = CheckCtrl() ? 50.0f*SPEED_CONTROL_DELTA : SPEED_CONTROL_DELTA;
	if(m_ControlState<=0){
		if(GetKey(DIK_LEFT)>=S_PUSH){
			if(!m_ControlState) m_ControlState = m_TargetSpeed>0.0f ? -SPD_CTRL : -2*SPD_CTRL;
			if(m_ControlState>-2*SPD_CTRL){
				if(m_ControlState>-2*SPD_CTRL+1){
					m_ControlState--;
					m_TargetSpeed -= (m_ControlState&1)*delta;
				}else{
					m_TargetSpeed -= 2*delta;
				}
				if(m_TargetSpeed<0.0f) m_TargetSpeed = 0.0f;
			}else{
				if(m_ControlState>-3*SPD_CTRL+1){
					m_ControlState--;
					m_TargetSpeed -= (m_ControlState&1)*delta;
				}else{
					m_TargetSpeed -= 2*delta;
				}
				if(m_TargetSpeed<-m_MaxVelocity) m_TargetSpeed = -m_MaxVelocity;
			}
		}else{
			m_ControlState = 0;
		}
	}
	if(m_ControlState>=0){
		if(GetKey(DIK_RIGHT)>=S_PUSH){
			if(!m_ControlState) m_ControlState = m_TargetSpeed<0.0f ? SPD_CTRL : 2*SPD_CTRL;
			if(m_ControlState<2*SPD_CTRL){
				if(m_ControlState<2*SPD_CTRL-1){
					m_ControlState++;
					m_TargetSpeed += (m_ControlState&1)*delta;
				}else{
					m_TargetSpeed += 2*delta;
				}
				if(m_TargetSpeed>0.0f) m_TargetSpeed = 0.0f;
			}else{
				if(m_ControlState<3*SPD_CTRL-1){
					m_ControlState++;
					m_TargetSpeed += (m_ControlState&1)*delta;
				}else{
					m_TargetSpeed += 2*delta;
				}
				if(m_TargetSpeed>m_MaxVelocity) m_TargetSpeed = m_MaxVelocity;
			}
		}else{
			m_ControlState = 0;
		}
	}
	int idx = 0;
	float nearest = 200.0f, split_pos = -1;
	CTrain *train = m_TrainList;
	VEC2 mouse_pos(g_Cursor.GetPos().x, g_Cursor.GetPos().y);
	while(train){
		if(idx>0 && train->GetScene()==g_Scene){
			VEC3 hlen = train->GetDir()*train->GetLength()*(train->m_Reverse ? -0.5f : 0.5f);
			VEC3 pos1 = WorldToScreen(train->GetPos()+hlen);
			float dist = V2Len(&(VEC2(pos1.x, pos1.y)-mouse_pos));
			if(dist<nearest){
				nearest = dist;
				split_pos = idx;
			}
		}
		train = train->m_Next;
		idx++;
	}
	if(CheckCtrl() && CheckShift() && split_pos>0){
		m_SplitPos = split_pos;
		if(GetButton(DIM_LEFT)==S_PUSH){
			if(g_NetworkInitialized) EnqueueMergeTrainControl(m_OldAdr, NULL, m_SplitPos, 0);
			else if(g_ManualControl) SplitTrain(m_SplitPos);
		}
	}
	return src;
}

/*
 *	制限速度設定
 */
void CTrainGroup::SetSpeedLimit(int sl){
	if(sl<0 || g_ManualControl || g_IgnoreAcceleration) return;
	if(m_SpeedLimit<0 || m_SpeedLimit>sl) m_SpeedLimit = sl;
}

/*
 *	制限速度付き実効
 */
float CTrainGroup::CalcSignedSpeedLimit(){
	if(m_SpeedLimit<0) return m_EffectTargetSpeed;
	float abs_limited_speed = fabsf(m_EffectTargetSpeed);
	if(abs_limited_speed>m_SpeedLimit) abs_limited_speed = (float)m_SpeedLimit;
	return m_EffectTargetSpeed<0.0f ? -abs_limited_speed : abs_limited_speed;
}

/*
 *	編成視点計算
 */
bool CTrainGroup::CalcViewAxis(
	CObject *obj	//	dir ベクトル
){
	int n = 0;
	VEC3 front, tail;
	ITrainSetBuffer itsb;
	int inscene = 0;
	bool warping = false;
	CScene *firstscene = NULL, *lastscene = NULL;
	for(itsb = m_SetBuffer.begin(); itsb!=m_SetBuffer.end(); itsb++){
		if(itsb->m_Posture){
			if(!firstscene){
				front = itsb->m_Posture->m_Pos;
				firstscene = itsb->m_Posture->m_Rail->GetScene();
			}
			tail = itsb->m_Posture->m_Pos;
			lastscene = itsb->m_Posture->m_Rail->GetScene();
			if(itsb->m_Posture->m_Rail->GetScene()==g_Scene) inscene++;
			else warping = true;
		}
	}
	if(!inscene && firstscene){
		CCamera cam = *CCamera::GetCurrentCamera();
		g_Scene = m_Reverse ? lastscene : firstscene;
		*g_Scene->GetCamera() = cam;
		g_Scene->Enter(true);
		*g_Scene->GetCamera() = cam;
		obj->SetPos(m_Reverse ? tail : front);
		return CalcViewAxis(obj);
	}
	if(warping) return false;
	VEC3 tright, tdir, tdir0 = front-tail, center = 0.5f*(front+tail);
	V3Norm(&tdir, &VEC3(tdir0.x, 0.0f, tdir0.z));
	V3Norm(&tdir0, &tdir0);
	V3Norm(&tright, V3Cross(&tright, &V3UP, &tdir));
	VEC3 pmin = VEC3(0.0f, center.y, 0.0f), pmax = pmin;
	for(itsb = m_SetBuffer.begin(); itsb!=m_SetBuffer.end(); itsb++){
		if(!itsb->m_Posture) continue;
		VEC3 to = itsb->m_Posture->m_Pos-center;
		float x = V3Dot(&tright, &to);
		if(x<pmin.x) pmin.x = x;
		if(x>pmax.x) pmax.x = x;
		float y = itsb->m_Posture->m_Pos.y;
		if(y<pmin.y) pmin.y = y;
		if(y>pmax.y) pmax.y = y;
		float z = V3Dot(&tdir, &to);
		if(z<pmin.z) pmin.z = z;
		if(z>pmax.z) pmax.z = z;
	}
	VEC3 pos = center+tright*(0.5f*(pmin.x+pmax.x))+tdir*(0.5f*(pmin.z+pmax.z));
	pos.y = 0.5f*(pmin.y+pmax.y);
	obj->SetPos(pos);
	obj->SetDir(tdir0, V3UP);
	return true;
}

/*
 *	運転席視点設定
 */
void CTrainGroup::SetCabinView(){
	POINT delta = g_Cursor.GetDelta();
	if(GetKey(DIK_HOME)==S_PUSH){
		m_CabinPos[m_Reverse] = V3ZERO;
	}else if(GetKey(DIK_END)==S_PUSH){
		m_CabinDir[m_Reverse] = V3DIR;
	}else if((GetButton(DIM_LEFT)|GetButton(DIM_MIDDLE))>=S_PUSH){
		if(CheckShift()){
			m_CabinPos[m_Reverse].y -= CABIN_POS_MOVE*delta.y;
			ValueArea(&m_CabinPos[m_Reverse].y, -CABIN_POS_MAX, CABIN_POS_MAX);
		}else{
			VEC3 td = m_CabinDir[m_Reverse];
			V3Norm(&td, &VEC3(td.x, 0.0f, td.z));
			VEC3 tr(td.z, 0.0f, -td.x);
			m_CabinPos[m_Reverse] += CABIN_POS_MOVE*(delta.x*tr-delta.y*td);
			ValueArea(&m_CabinPos[m_Reverse].x, -CABIN_POS_MAX, CABIN_POS_MAX);
			ValueArea(&m_CabinPos[m_Reverse].z, -CABIN_POS_MAX, CABIN_POS_MAX);
		}
	}else if(GetButton(DIM_RIGHT)>=S_PUSH){
		m_CabinDir[m_Reverse].x += CABIN_DIR_MOVE*delta.x;
		m_CabinDir[m_Reverse].y -= CABIN_DIR_MOVE*delta.y;
		ValueArea(&m_CabinDir[m_Reverse].x, -CABIN_DIR_MAX, CABIN_DIR_MAX);
		ValueArea(&m_CabinDir[m_Reverse].y, -CABIN_DIR_MAX, CABIN_DIR_MAX);
	}
	CTrain *train = m_TrainList;
	if(m_Reverse) while(train->Next()) train = train->Next();
	g_CabinViewTrain = train;
	train->SetCabinView(!!m_Reverse, m_CabinPos[m_Reverse], m_CabinDir[m_Reverse]);
}

/*
 *	車輌追加
 */
void CTrainGroup::AddTrain(
	CTrainPlugin *tpi,	//	車輌プラグイン
	bool rev			//	反転フラグ
){
	if(m_Enabled){
		EnqueueCommonDialog(new CSimpleDialog(lang(CannotAddWhileTrainSet), lang(Error)));
		g_Skin->Error();
		return;
	}
	CTrain **adr = &m_TrainList;
	while(*adr) adr = &(*adr)->m_Next;
	CTrain *newtrain = *adr = new CTrain(tpi, this, rev);
	CTrain *ptr = m_TrainList;
	while(ptr){
		if(ptr!=newtrain) newtrain->GetModelPlugin()
			->CheckGroupCommonSwitchAll(newtrain, ptr);
		ptr = ptr->m_Next;
	}
	OnTrainNumChanged();
}

/*
 *	車輌削除
 */
void CTrainGroup::DeleteTrain(
	CTrain *tr	//	車輌
){
	if(m_Enabled){
		EnqueueCommonDialog(new CSimpleDialog(lang(CannotDeleteWhileTrainSet), lang(Error)));
		g_Skin->Error();
		return;
	}
	CTrain **adr = &m_TrainList;
	while(*adr){
		if(*adr==tr){
			CTrain *tmp = *adr;
			*adr = tmp->m_Next;
			tmp->m_Next = NULL;
			delete tmp;
			break;
		}
		adr = &(*adr)->m_Next;
	}
	OnTrainNumChanged();
}

/*
 *	車輌リスト作成
 */
void CTrainGroup::ListTrain(
	CListView *lv	//	リストビュー
){
	CTrain *ptr = m_TrainList;
	lv->DeleteAllItems();
	while(ptr){
		CListElement *le = lv->InsertItem(-1, ptr->m_TrainPlugin->GetName());
		le->SetString(1, ptr->IsReverse() ? lang(Yes) : lang(No));
		le->SetData((DWORD)ptr);
		ptr->SetListElement(le);
		ptr = ptr->m_Next;
	}
	m_SelectTrain = NULL;
}

/*
 *	車輌編集
 */
CTrain *CTrainGroup::EditTrain(
	CListView *tlv,		//	車輌リストビュー
	CPluginTree *ttv	//	車輌プラグインツリー
){
	if(!m_TrainList) return NULL;
	CListElement *tle = tlv->GetFocusItem();
	if(tle && tle->IsSelected()){
		CTrain *tr = (CTrain *)tle->GetData();
		if(tr!=m_SelectTrain){
			m_SelectTrain = tr;
			ttv->SelectPlugin(m_SelectTrain->m_TrainPlugin);
			m_SelectTrain->m_TrainPlugin->SetSwitch(m_SelectTrain);
			return m_SelectTrain;
		}else if(!g_Train || !CPlugin::IsPreview()
			|| g_Train->GetLinkInst()!=m_SelectTrain){
			tlv->SetSelectionMark(-1, 0);
		}
	}else if(m_SelectTrain){
		m_SelectTrain = NULL;
		if(g_Train) g_Train->FreeInst();
	}
	return NULL;
}

/*
 *	編成共通スイッチの設定
 */
void CTrainGroup::SetGroupCommonSwitch(
	CModelSwitch *sw,	//	対象スイッチ (固定側)
	int val				//	対象値 (固定側)
){
	CTrain *ptr = m_TrainList;
	while(ptr){
		ptr->GetModelPlugin()->CheckGroupCommonSwitch(ptr, sw, val);
		ptr = ptr->m_Next;
	}
}

/*
 *	車輌撤去
 */
void CTrainGroup::Remove(){
	if(m_Enabled){
		m_Enabled = false;
		if(m_ListElement) m_ListElement->SetString(2, lang(NotSet));
		m_Location[0].Detach();
		m_Location[1].Detach();
		m_Seeker.Detach();
		CTrain *train = m_TrainList;
		while(train){
			train->StopSound();
			train = train->Next();
		}
	}
	ClearPoint();
	ClearSeek();
	m_Platform = NULL;
}

/*
 *	車軸バッファ準備
 */
void CTrainGroup::SetSetBuffer(){
	m_SetBuffer.clear();
	m_SetBuffer.push_back(CTrainSetBuffer(0.0f, false, NULL));
	m_Length = m_TrainList->SetSetBuffer(0.0f, &m_SetBuffer);
	m_SetBuffer.push_back(CTrainSetBuffer(m_Length, false, NULL));
}

/*
 *	ネットワーク制御エラーをチャットに投げる
 */
void CTrainGroup::SetError(const char *msg){
	if(g_NetworkInitialized){
		void PushChatLog(char *, D3DCOLOR c);
		PushChatLog(FlashIn("%s (%s): %s", lang(TrainOperation), m_Name.c_str(), msg), 0xffffffff);
	}else{
		EnqueueCommonDialog(new CSimpleDialog((char *)msg, lang(Error)));
		g_Skin->Error();
	}
}

/*
 *	車輌配置 (初回)
 */
bool CTrainGroup::Set(
	CRailLinkTemp *link,	//	配置先データ
	int flag				//	0: 配置のみ, 1: 自動前進, 2: 自動後進, -1: キーボード取得
){
	if(!m_TrainList){
		SetError(lang(NoCarInserted));
		return false;
	}
	if(m_Enabled){
		SetError(lang(AlreadySet));
		return false;
	}
	PushUndoStack();
	m_Reverse = false;
	m_State = 0;
	m_Platform = NULL;
	m_Location[0] = CGroupEndLocator(link->m_Side, link->m_SumLen, link->m_Link, this);
	SetSetBuffer();
	m_ControlState = 0;
	m_CurrentSpeed = m_OldSpeed = 0.0f;
	if(flag<0) flag = CheckCtrl() ? (CheckShift() ? 2 : 1) : 0;
	switch(flag){
	case 1: m_EffectTargetSpeed = m_TargetSpeed = m_MaxVelocity; break;
	case 2: m_EffectTargetSpeed = m_TargetSpeed = -m_MaxVelocity; break;
	default: m_EffectTargetSpeed = m_TargetSpeed = 0.0f; break;
	}
	if(!link->m_Link->IsInsideGroup(!link->m_Side, link->m_SumLen) && Trail(false, true)){
		m_Seeker = m_Location[m_Reverse];
		m_Seeker.Attach(2);
		m_Enabled = true;
		if(m_ListElement) m_ListElement->SetString(2, lang(Set));
		CTrain *train = m_TrainList;
		while(train){
			train->ResetState();
			CTrainPlugin *tpi = train->m_TrainPlugin;
			tpi->SetSwitch(train);
			tpi->SetPartsInst(train);
			tpi->SetMoverState(train);
			train->ApplyAxle(false);
			train->ResetTilt();	//	must be after ApplyAxle
			tpi->SetPosture();
			train = train->Next();
		}
		return true;
	}
	Remove();
	SetError(lang(NonSuitablePlace));
	return false;
}

/*
 *	車輌配置
 */
bool CTrainGroup::Trail(
	bool extend,	//	拡張トレール
	bool hittest	//	衝突判定
){
	CTrain *train = m_TrainList;
	while(train){
		train->m_Setting = train->m_Warping = false;
		train = train->Next();
	}
	m_SetBuffer.sort();
	ITrainSetBuffer ibegin, iend;
	if(m_Reverse){
		ibegin = --m_SetBuffer.end();
		iend = --m_SetBuffer.begin();
		// 本来 std::list の --begin() は許可されないが、実質動くのでとりあえずこのまま。
		// VC2010 だと怒られるので /D "_HAS_ITERATOR_DEBUGGING=0" してなんとか回避。
		// ただし libcpmtd.lib の代わりに libcpmt.lib をリンクする必要がある。
	}else{
		ibegin = m_SetBuffer.begin();
		iend = m_SetBuffer.end();
	}
	ITrainSetBuffer ib = m_SetBuffer.begin();
	float min = ib->m_SumLen;
	for(; ib!=m_SetBuffer.end(); ib++) ib->m_SumLen -= min;
	CGroupEndLocator &head = m_Location[m_Reverse], *tail = &m_Location[!m_Reverse];
	bool ret;
	if(ret = m_Reverse
		? head.m_SetRail->SetTrain(!head.m_Side, head.m_Offset+ibegin->m_SumLen,
			tail, 1, &ibegin, &iend, this, extend, hittest)
		: head.m_SetRail->SetTrain(!head.m_Side, head.m_Offset,
			tail, 0, &ibegin, &iend, this, extend, hittest)){
		tail->m_Side = !tail->m_Side;
		tail->m_Offset = tail->m_SetRail->GetSegLen()-tail->m_Offset;
	}else{
		Remove();
		return false;
	}
	m_Location[0].Attach(0);
	m_Location[1].Attach(1);
	return ret;
}

/*
 *	ワープ位置通知
 */
void CTrainGroup::NotifyWarp(){
	CTrain *train = m_TrainList;
	while(train){
		if(train->m_Setting) train->NotifyWarp();
		train = train->Next();
	}
}

/*
 *	プラットフォーム位置通知
 */
void CTrainGroup::NotifyPlatform(
	CPlatformInst *pf,	//	プラットフォーム
	float dist,			//	距離
	bool pfrev			//	方向逆フラグ
){
	if(!pf->IsStoppable()) return;
	m_NotifyFlag = true;
	switch(m_State){
	case 0:
		if(pf==m_Platform) return;
		m_Platform = pf;
		m_DiaElement = *m_Platform->GetStation()->GetDiaInst()->Dequeue(this);
		if(m_DiaElement.m_Action<2){
			m_State = 1;
			m_StopTarget = 0.5f*(m_DiaElement.m_Offset+1.0f)
				*(m_Platform->GetLength()+m_Length);
			if(m_StopTarget<0.001f) m_StopTarget = 0.001f;
			m_StopTarget += dist;
			m_OpenDoor[!pfrev] = m_Platform->GetOpenDoor(0);
			m_OpenDoor[pfrev] = m_Platform->GetOpenDoor(1);
		}
		break;
	}
}

/*
 *	ポイント情報消去
 */
void CTrainGroup::ClearPoint(){
	ISPRailConnector ip = m_PointList.begin();
	for(; ip!=m_PointList.end(); ip++) (*ip)->SetUser(NULL);
	m_PointList.clear();
	ClearRailBlockUser(this);
}

/*
 *	シーカ消去
 */
void CTrainGroup::ClearSeek(){
	m_OldSeek = m_SeekList;
	ISPRailConnector ip = m_SeekList.begin();
	for(; ip!=m_SeekList.end(); ip++) (*ip)->SetUser(NULL);
	m_SeekList.clear();
	ClearRailBlockUser(this);
}

/*
 *	シーカ追加
 *
 *	m_OldSeek に入っていれば true を返す
 */
bool CTrainGroup::AddSeek(
	CRailConnector *pcon	//	ポイント
){
	m_SeekList.insert(pcon);
	return !!m_OldSeek.count(pcon);
}

/*
 *	連結スイッチの設定
 */
void CTrainGroup::SetConnectSwitch(
	CTrain *train	//	車輌インスタンス
){
	int mrev = m_Reverse==train->m_Reverse;
	g_SystemSwitch[SYS_SW_FRONT].SetValue(!mrev);
	if(train->m_Reverse){
		g_SystemSwitch[SYS_SW_CONNECT1].SetValue(!!train->Next());
		g_SystemSwitch[SYS_SW_CONNECT2].SetValue(train!=m_TrainList);
	}else{
		g_SystemSwitch[SYS_SW_CONNECT1].SetValue(train!=m_TrainList);
		g_SystemSwitch[SYS_SW_CONNECT2].SetValue(!!train->Next());
	}
	if(m_State==2){
		g_SystemSwitch[SYS_SW_DOOR1].SetValue(m_OpenDoor[!mrev]);
		g_SystemSwitch[SYS_SW_DOOR2].SetValue(m_OpenDoor[mrev]);
	}else{
		g_SystemSwitch[SYS_SW_DOOR1].SetValue(0);
		g_SystemSwitch[SYS_SW_DOOR2].SetValue(0);
	}
	g_SystemSwitch[SYS_SW_VELOCITY].SetValue(Round(fabsf(m_CurrentSpeed)));
	g_SystemSwitch[SYS_SW_ACCEL].SetValue(Round(
		(fabsf(m_CurrentSpeed)-fabsf(m_OldSpeed))*(1000*MAXFPS)));
	g_SystemSwitch[SYS_SW_CABINVIEW].SetValue(g_CabinViewTrain==train);
}

/*
 *	入力チェック
 */
void CTrainGroup::ScanInput(
	int mode,		//	モード (0: link)
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	if(!m_Enabled) return;
	CGroupEndLocator &head = m_Location[m_Reverse];
//	if(g_Scene!=head.m_SetRail->GetScene()) return;
	CTrain *train = m_TrainList;
	while(train){
		train->ScanInput(mode, rect1, rect2);
		train = train->m_Next;
	}
}

/*
 *	プレビュー
 */
void CTrainGroup::Preview(){
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(m_Serial);
	float offset = 0.0f, tmp = 0.0f;
	CTrain *train = m_TrainList;
	bool rev = m_Reverse;
	m_Reverse = false;
	while(train){
		CTrainPlugin *tpi = train->m_TrainPlugin;
		SetConnectSwitch(train);
		tpi->SetSwitch(train);
		CNamedObjectAfterRenderer::SetCurrentInst(NULL);
		tpi->Preview(offset-m_PreviewOffset, train->m_Reverse);
		if(train==m_SelectTrain) tmp = offset;
		offset += train->m_Reverse ? -tpi->m_FrontLimit : tpi->m_TailLimit;
		if(train = train->Next()){
			tpi = train->m_TrainPlugin;
			offset -= train->m_Reverse ? -tpi->m_TailLimit : tpi->m_FrontLimit;
		}
	}
	m_Reverse = rev;
	m_PreviewOffset = GROUP_PREVIEW_SPEED*tmp+(1.0f-GROUP_PREVIEW_SPEED)*m_PreviewOffset;
}

/*
 *	レンダリング
 */
void CTrainGroup::Render(){
	if(!m_Enabled) return;
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(m_Serial);
//	CGroupEndLocator &head = m_Location[m_Reverse];
//	if(g_Scene!=head.m_SetRail->GetScene()) return;
	CTrain *train = m_TrainList;
	CTrain *labelcar = NULL;
	int idx = 0;
	while(train){
		SetConnectSwitch(train);
		train->Render();
		if(g_MapDrawNeeded && (m_Reverse || !labelcar)
			&& !train->IsWarping() && train->CheckScene()) labelcar = train;
		if(idx++==m_SplitPos){
			VEC3 spos = train->GetPos()+train->GetDir()*train->GetLength()*(train->m_Reverse ? -0.5f : 0.5f);
			devResetMatrix();
			devSetLighting(FALSE);
			devSetTexture(0, NULL);
			Draw3DPointAs2DRect(spos, 0xff00ffff, 5);
			devSetLighting(TRUE);
		}
		train = train->Next();
	}
	m_SplitPos = -1;
	if(labelcar) RailMapText(labelcar->GetPos(), GetName(), 0xffff0000);
}

/*
 *	シミュレーション進行
 */
void CTrainGroup::Simulate(){
	if(!m_Enabled) return;
	if(!g_NetworkInitialized) m_EffectTargetSpeed = m_TargetSpeed;
	m_SpeedLimit = -1;
	g_SystemSwitch[SYS_SW_SERIAL].SetValue(m_Serial);
	m_OldSpeed = m_CurrentSpeed;
	double abstime = g_SaveFile->GetAbsTime();
	float speed = m_CurrentSpeed/(3.6f*MAXFPS);	//	[km/h] to [m/frame]
	m_Location[0].Detach();
	m_Location[1].Detach();
	m_Seeker.Detach();
	ClearSeek();
	CGroupEndLocator &head = m_Location[m_Reverse];
	head.m_Offset = head.m_SetRail->GetSegLen()-head.m_Offset;
	float sptmp = fabsf(speed);
	g_HitTrainGroup.m_Group = NULL;
	int hit = head.m_SetRail->MarchTrain(&sptmp, &head, NULL, this);
	ClearPoint();
	m_StopTarget -= sptmp;
	speed = speed<0.0f ? -sptmp : sptmp;
	ITrainSetBuffer ib = m_SetBuffer.begin();
	for(; ib!=m_SetBuffer.end(); ib++) ib->Rotate(speed);
	if(hit) m_CurrentSpeed = speed*3.6f*MAXFPS;
	m_Seeker = head;
	head.m_Offset = head.m_SetRail->GetSegLen()-head.m_Offset;
	Trail(true, false);
	static bool connect_input = false;
	if(CheckAlt()){
		if(g_HitTrainGroup.m_Group && g_TrainGroup==this && g_ManualControl && !connect_input){
			connect_input = true;
			//ErrorDialog("%p: hit with %p type %d\n%p / %p %p", this, g_HitTrainGroup.m_Group, g_HitTrainGroup.m_Type,
			//	g_HitTrainGroup.m_SetRail, m_Location[0].m_SetRail, m_Location[1].m_SetRail);
			int merge_side1 = m_Reverse==true, merge_side2 = g_HitTrainGroup.m_Type==1;
			if(g_NetworkInitialized){
				EnqueueMergeTrainControl(
					m_OldAdr, g_HitTrainGroup.m_Group->m_OldAdr, merge_side1, merge_side2);
			}else if(g_ManualControl){
				MergeTrain(g_HitTrainGroup.m_Group, merge_side1, merge_side2);
			}
		}
	}else{
		connect_input = false;
	}
	g_HitTrainGroup.m_Group = NULL;
	float seekdist, dccmpff;
	if(g_ManualControl) goto NORMAL;
	seekdist = m_CurrentSpeed/(3.6f*MAXFPS);	//	[km/h] to [m/frame]
	dccmpff = (g_IgnoreAcceleration ? 1000.0f : m_MaxDeceleration)/(3.6f*MAXFPS);	//	[km/h/frame] to [m/frame/frame]
	seekdist = STOP_LIMIT_LINE+0.5f*seekdist*seekdist/dccmpff;
	m_NotifyFlag = false;
	hit = head.m_SetRail->MarchTrain(&seekdist, &m_Seeker, this, this);
	if(hit || m_State || m_CurrentSpeed){
		float signed_speed_limit = CalcSignedSpeedLimit();
		float newspeed, tmpdccspeed;
		//	減速停止・発車待ち
		if(hit){
			if(hit==2) seekdist -= STOP_LIMIT_LINE;
			if(m_State==1 && m_StopTarget<seekdist){
				hit = 0;
				seekdist = m_StopTarget;
			}
		}else{
			switch(m_State){
			case 1: seekdist = m_StopTarget; break;
			case 2: case 3: goto SUSPEND;
			default: goto NORMAL;
			}
		}
		if(seekdist<0.0f) seekdist = 0.0f;
		sptmp = sqrtf(2.0f*seekdist*dccmpff);	//	[m/frame]
		if(sptmp>seekdist) sptmp = seekdist;
		sptmp *= 3.6f*MAXFPS;	//	[m/frame] to [km/h]
		newspeed = m_CurrentSpeed<0.0f ? -sptmp : sptmp;

		tmpdccspeed = m_CurrentSpeed;
		if(fabsf(tmpdccspeed)>fabsf(signed_speed_limit)){
			if(tmpdccspeed<0.0f){
				tmpdccspeed += m_MaxDeceleration;
				if(tmpdccspeed>signed_speed_limit) tmpdccspeed = signed_speed_limit;
			}else{
				tmpdccspeed -= m_MaxDeceleration;
				if(tmpdccspeed<signed_speed_limit) tmpdccspeed = signed_speed_limit;
			}
			if(fabsf(tmpdccspeed)<fabsf(newspeed)) newspeed = tmpdccspeed;
		}

		if(fabsf(newspeed)<=fabsf(m_CurrentSpeed)) m_CurrentSpeed = newspeed;
		else if(!hit) goto NORMAL;
		if(hit==1 && !m_CurrentSpeed) m_EffectTargetSpeed = -m_EffectTargetSpeed;
SUSPEND:
		switch(m_State){
		case 1:
			if(!m_CurrentSpeed && (m_StopTarget<STOP_LIMIT_LINE || hit==1)){
				m_State = 2;
				m_DoorWait = m_OpenDoor[0] || m_OpenDoor[1]
					? Round(m_DoorClosingTime*MAXFPS) : 0;
				if(hit==1 || m_DiaElement.m_Action==1
					|| !head.m_SetRail->CheckPlatformExtend(head.m_Side, m_Platform))
					m_EffectTargetSpeed = -m_EffectTargetSpeed;
				switch(m_DiaElement.m_TimeType){
				case 0:
					m_DepartureTime = abstime+m_DiaElement.GetTime();
					break;
				case 1:
					m_DepartureTime = g_SaveFile->GetSumDays()+m_DiaElement.GetTime();
					if(m_DepartureTime<abstime) m_DepartureTime += 1.0;
					break;
				}
			}
			break;
		case 2:
			if(abstime+m_DoorWait*g_SimulationMode->GetTimeScale()/FRAME_PER_DAY
				>=m_DepartureTime) m_State = 3;
		case 3:
			if(abstime>=m_DepartureTime) m_State = 0;
			break;
		}
		if(hit && !m_CurrentSpeed && signed_speed_limit) m_Reverse = m_EffectTargetSpeed<0.0f;
	}else{
NORMAL:
		//	定常状態
		float signed_speed_limit = CalcSignedSpeedLimit();
		if(g_IgnoreAcceleration){
			m_CurrentSpeed = m_EffectTargetSpeed;
		}else{
			float acs = fabsf(m_CurrentSpeed), ats = fabsf(signed_speed_limit);
			if(m_CurrentSpeed*signed_speed_limit<0.0f) ats = 0.0f;
			if(acs>ats){
				float dcc = m_MaxDeceleration;
				if(dcc>acs-ats) dcc = acs-ats;
				if(m_CurrentSpeed<0.0f) dcc = -dcc;
				m_CurrentSpeed -= dcc;
			}else if(acs<ats){
				float acc = m_MaxAcceleration*(FINAL_ACCELERATION+
					(1.0f-FINAL_ACCELERATION)*(m_MaxVelocity-acs)/m_MaxVelocity);
				if(acc<0.0f) acc = 0.0f;
				if(acc>ats-acs) acc = ats-acs;
				if(signed_speed_limit<0.0f) acc = -acc;
				m_CurrentSpeed += acc;
			}
		}
		if(g_ManualControl) goto SKIP_ATS;
	}
	if(!m_NotifyFlag) m_Platform = NULL;
	m_Seeker.m_Offset = m_Seeker.m_SetRail->GetSegLen()-m_Seeker.m_Offset;
	m_Seeker.Attach(2);
SKIP_ATS:
	if(m_CurrentSpeed) m_Reverse = m_CurrentSpeed<0.0f;
	CTrain *train = m_TrainList;
	while(train){
		SetConnectSwitch(train);
		train->Simulate();
		train = train->Next();
	}
	if(!g_NetworkInitialized) m_TargetSpeed = m_EffectTargetSpeed;
}

/*
 *	編成併合
 */
void CTrainGroup::MergeTrain(CTrainGroup *target, int my_side, int tgt_side){
	//Dialog("my_side = %d\ntgt_side = %d", my_side, tgt_side);
	m_Location[0].Detach();
	m_Location[1].Detach();
	target->m_Location[0].Detach();
	target->m_Location[1].Detach();
	m_CurrentSpeed = (m_CurrentSpeed*m_Length+target->m_CurrentSpeed*target->m_Length)/(m_Length+target->m_Length);
	int i;
	vector<CTrainSetBuffer *> vsb;
	ITrainSetBuffer isb; 
	for(isb = target->m_SetBuffer.begin(); isb!=target->m_SetBuffer.end(); isb++){
		vsb.push_back(&*isb);
		if(!!my_side^!tgt_side){
			isb->m_SumLen = target->m_Length-isb->m_SumLen;
			isb->m_Reverse = !isb->m_Reverse;
		}
		if(my_side) isb->m_SumLen += m_Length;
	}
	if(!my_side){
		for(isb = m_SetBuffer.begin(); isb!=m_SetBuffer.end(); isb++) isb->m_SumLen += target->m_Length;
	}
	m_Length += target->m_Length;
	//Dialog("moving %d axle", vsb.size());
	if(my_side) m_SetBuffer.pop_back(); else m_SetBuffer.pop_front();
	for(i = 1; i<vsb.size(); ++i){
		int idx = tgt_side ? vsb.size()-1-i : i;
		if(my_side) m_SetBuffer.push_back(*vsb[idx]); else m_SetBuffer.push_front(*vsb[idx]);
	}
	//for(isb = m_SetBuffer.begin(); isb!=m_SetBuffer.end(); isb++) Dialog("C %f", isb->m_SumLen);
	target->m_SetBuffer.clear();
	vector<CTrain *> vtr;
	CTrain *train = target->m_TrainList;
	for(; train; train = train->m_Next) vtr.push_back(train);
	if(my_side){
		CTrain **carrier = &m_TrainList;
		while(*carrier) carrier = &(*carrier)->m_Next;
		for(i = 0; i<vtr.size(); i++){
			int idx = tgt_side ? vtr.size()-1-i : i;
			vtr[idx]->m_Reverse = (vtr[idx]->m_Reverse^!tgt_side^1)&1;
			vtr[idx]->m_Group = this;
			vtr[idx]->m_Next = NULL;
			*carrier = vtr[idx];
			carrier = &vtr[idx]->m_Next;
		}
	}else{
		for(i = 0; i<vtr.size(); i++){
			int idx = tgt_side ? vtr.size()-1-i : i;
			vtr[idx]->m_Reverse = (vtr[idx]->m_Reverse^!tgt_side)&1;
			vtr[idx]->m_Group = this;
			vtr[idx]->m_Next = m_TrainList;
			m_TrainList = vtr[idx];
		}
	}
	target->m_TrainList = NULL;
	target->Remove();
	CalcSpec();
	ClearSeek();
	target->CalcSpec();
	target->ClearSeek();
	m_Location[my_side] = target->m_Location[!tgt_side];
	m_Location[my_side].m_Group = this;
	m_Location[my_side].Attach(my_side);
	ClearPoint();
	Trail(true, false);
	g_SaveFile->DeleteGroup(target);
	if(!g_NetworkInitialized) g_TrainGroup = this;
	g_UpdateTrainGroupList = true;
}

/*
 *	編成分割
 */
void CTrainGroup::SplitTrain(int split_pos){
	if(split_pos<=0 || GetTrainNum()<=split_pos) return;
	m_Location[0].Detach();
	m_Location[1].Detach();
	bool old_reverse = m_Reverse;
	m_Reverse = false;
	CTrainGroup *new_group = new CTrainGroup((char *)m_Name.c_str());
	new_group->m_Enabled = true;
	new_group->m_Reverse = false;
	new_group->m_OldAdr = RegisterNewMapAddress(new_group);
	new_group->m_Next = m_Next;
	m_Next = new_group;
	vector<CTrain *> vtr;
	CTrain *train = m_TrainList;
	for(; train; train = train->m_Next) vtr.push_back(train);
	set<CAxlePosture *> axle_set2;
	m_Length = new_group->m_Length = 0.0f;
	int i;
	for(i = vtr.size()-1; i>=split_pos; i--){
		vtr[i]->m_Group = new_group;
		vtr[i]->m_Next = new_group->m_TrainList;
		new_group->m_Length += vtr[i]->GetLength();
		new_group->m_TrainList = vtr[i];
		IAxlePosture iap = vtr[i]->m_AxleList.begin();
		for(; iap!=vtr[i]->m_AxleList.end(); iap++) axle_set2.insert(&*iap);
	}
	for(; i>=0; i--) m_Length += vtr[i]->GetLength();
	vtr[split_pos-1]->m_Next = NULL;
	ITrainSetBuffer isb = m_SetBuffer.begin();
	vector<ITrainSetBuffer> dummy_bufs;
	for(; isb!=m_SetBuffer.end(); isb++){
		if(!isb->m_Posture){
			if(dummy_bufs.size()) isb->m_SumLen = m_Length;
			dummy_bufs.push_back(isb);
		}else if(axle_set2.count(isb->m_Posture)){
			ITrainSetBuffer isb2 = isb--;
			isb2->m_SumLen -= m_Length;
			new_group->m_SetBuffer.splice(new_group->m_SetBuffer.end(), m_SetBuffer, isb2, ++ITrainSetBuffer(isb2));
		}
	}
	if(dummy_bufs.size()!=2) ErrorDialog("ERROR: dummy_bufs.size() = %d", dummy_bufs.size());
	g_SaveFile->NumberGroup();
	CalcSpec();
	new_group->CalcSpec();
	new_group->m_SetBuffer.push_front(CTrainSetBuffer(0.0f, false, NULL));
	new_group->m_SetBuffer.push_back(CTrainSetBuffer(new_group->m_Length, false, NULL));
	new_group->m_Location[1] = m_Location[1];
	new_group->m_Location[1].m_Group = new_group;
	ClearSeek();
	ClearPoint();
	Trail(true, false);
	new_group->CalcSpec();
	CGroupEndLocator &head2 = new_group->m_Location[0];
	head2 = m_Location[1];
	head2.m_Group = new_group;
	head2.m_Side = !head2.m_Side;
	head2.m_Offset = head2.m_SetRail->GetSegLen()-head2.m_Offset;
	head2.Attach(0);
	new_group->Trail(true, false);
	m_Reverse = old_reverse;
	new_group->m_Reverse = old_reverse;
	new_group->m_CurrentSpeed = m_CurrentSpeed;
	new_group->m_OldSpeed = m_OldSpeed;
	if(m_Reverse){
		new_group->m_TargetSpeed = m_TargetSpeed;
		new_group->m_EffectTargetSpeed = m_EffectTargetSpeed;
		m_TargetSpeed = m_EffectTargetSpeed = 0.0f;
		g_TrainGroup = new_group;
	}else{
		new_group->m_TargetSpeed = 0.0f;
		new_group->m_EffectTargetSpeed = 0.0f;
	}
	CheckTargetSpeed();
	new_group->CheckTargetSpeed();
	g_UpdateTrainGroupList = true;
}

/*
 *	設置状態復元
 */
void CTrainGroup::RestoreSet(){
	if(!m_Enabled) return;
	SetSetBuffer();
	m_Location[0].Detach();
	m_Location[1].Detach();
	CGroupEndLocator &head = m_Location[m_Reverse];
	head.m_Offset = head.m_SetRail->GetSegLen()-head.m_Offset;
	ClearPoint();
	head.m_Offset = head.m_SetRail->GetSegLen()-head.m_Offset;
	Trail(true, false);
}

/*
 *	アドレス復元
 */
void CTrainGroup::RestoreAddress(){
	m_Location[0].RestoreAddress();
	m_Location[1].RestoreAddress();
	m_Seeker.RestoreAddress();
	set<CRailConnector *> pointlist, seeklist;
	ISPRailConnector ipr;
	for(ipr = m_PointList.begin(); ipr!=m_PointList.end(); ipr++)
		pointlist.insert((CRailConnector *)ReplaceAdr(*ipr));
	m_PointList = pointlist;
	for(ipr = m_SeekList.begin(); ipr!=m_SeekList.end(); ipr++)
		seeklist.insert((CRailConnector *)ReplaceAdr(*ipr));
	m_SeekList = seeklist;
	m_Platform = (CPlatformInst *)ReplaceAdr(m_Platform);
	CalcSpec();
}

/*
 *	読込
 */
char *CTrainGroup::Read(
	char *str,			//	対象文字列
	CTrainGroup ***root	//	格納先
){
	char *eee, *tmp;
	if(!(str = BeginBlock(str, "TrainGroup"))){
		delete this;
		return NULL;
	}
	if(!(str = AsgnPointer(eee = str, "Address", &m_OldAdr))) throw CSynErr(eee);
	g_AddressMap[m_OldAdr] = this;
	if(!(str = AsgnString(eee = str, "Name", &m_Name))) throw CSynErr(eee);
	m_Name = RestoreDoubleQuote(m_Name);
	if(!(str = AsgnVector3D(eee = str, "CabinPos", m_CabinPos, 2, false))) throw CSynErr(eee);
	if(!(str = AsgnVector3D(eee = str, "CabinDir", m_CabinDir, 2, false))) throw CSynErr(eee);
	if(!(str = AsgnYesNo(eee = str, "Enabled", &m_Enabled))) throw CSynErr(eee);
	if(m_Enabled){
		if(!(str = AsgnInteger(eee = str, "State", &m_State))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "TargetSpeed", &m_TargetSpeed))) throw CSynErr(eee);
		m_EffectTargetSpeed = m_TargetSpeed;
		if(!(str = AsgnFloat(eee = str, "CurrentSpeed", &m_CurrentSpeed))) throw CSynErr(eee);
		if(!(str = AsgnFloat(eee = str, "StopTarget", &m_StopTarget))) throw CSynErr(eee);
		if(!(str = AsgnPointer(eee = str, "DepartureTime",
			(void **)&m_DepartureTime, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnInteger(eee = str, "DoorWait", &m_DoorWait))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "OpenDoor", m_OpenDoor, 2, false))) throw CSynErr(eee);
		if(!(str = AsgnYesNo(eee = str, "Reverse", &m_Reverse))) throw CSynErr(eee);
		if(!(str = m_Location[0].Read(str, "GroupEnd0"))) throw CSynErr(eee);
		if(!(str = m_Location[1].Read(str, "GroupEnd1"))) throw CSynErr(eee);
		if(!(str = m_Seeker.Read(str, "Seeker"))) throw CSynErr(eee);
		if(tmp = Assignment(str, "PointList")){
			str = tmp;
			do{
				if(m_PointList.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
				CRailConnector *con;
				if(!(str = HexPointer(eee = str, (void **)&con))) throw CSynErr(eee);
				m_PointList.insert(con);
			} while(!(tmp = Character2(str, ';')));
			str = tmp;
		}
		if(tmp = Assignment(str, "SeekList")){
			str = tmp;
			do{
				if(m_SeekList.size() && !(str = Character2(eee = str, ','))) throw CSynErr(eee);
				CRailConnector *con;
				if(!(str = HexPointer(eee = str, (void **)&con))) throw CSynErr(eee);
				m_SeekList.insert(con);
			} while(!(tmp = Character2(str, ';')));
			str = tmp;
		}
		if(!(str = m_DiaElement.Read(str))) throw CSynErr(eee);
		if(!(str = AsgnPointer(eee = str, "Platform", (void **)&m_Platform))) throw CSynErr(eee);
	}
	CTrain **adr = &m_TrainList;
	while(true){
		if(tmp = (new CTrain(this))->Read(str, &adr)) str = tmp;
		else break;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	**root = this;
	*root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CTrainGroup::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\tTrainGroup{\n");
	fprintf(df, "\t\tAddress = %p;\n", this);
	fprintf(df, "\t\tName = \"%s\";\n", ExpandDoubleQuote(m_Name).c_str());
	fprintf(df, "\t\tCabinPos = ");
	V3Save(df, m_CabinPos[0], ", "); V3Save(df, m_CabinPos[1], ";\n");
	fprintf(df, "\t\tCabinDir = ");
	V3Save(df, m_CabinDir[0], ", "); V3Save(df, m_CabinDir[1], ";\n");
	fprintf(df, "\t\tEnabled = %s;\n", YESNO[m_Enabled]);
	if(m_Enabled){
		fprintf(df, "\t\tState = %d;\n", m_State);
		fprintf(df, "\t\tTargetSpeed = %f;\n", m_TargetSpeed);
		fprintf(df, "\t\tCurrentSpeed = %f;\n", m_CurrentSpeed);
		fprintf(df, "\t\tStopTarget = %f;\n", m_StopTarget);
		fprintf(df, "\t\tDepartureTime = %p, %p;\n",
			*(PDWORD)&m_DepartureTime, *((PDWORD)&m_DepartureTime+1));
		fprintf(df, "\t\tDoorWait = %d;\n", m_DoorWait);
		fprintf(df, "\t\tOpenDoor = %s, %s;\n", YESNO[m_OpenDoor[0]], YESNO[m_OpenDoor[1]]);
		fprintf(df, "\t\tReverse = %s;\n", YESNO[m_Reverse]);
		m_Location[0].Save(df, "\t\t", "GroupEnd0");
		m_Location[1].Save(df, "\t\t", "GroupEnd1");
		m_Seeker.Save(df, "\t\t", "Seeker");
		ISPRailConnector ipr;
		if(m_PointList.size()){
			fprintf(df, "\t\tPointList = ");
			for(ipr = m_PointList.begin(); ipr!=m_PointList.end(); ipr++)
				fprintf(df, ipr==m_PointList.begin() ? "%p" : ", %p", *ipr);
			fprintf(df, ";\n");
		}
		if(m_SeekList.size()){
			fprintf(df, "\t\tSeekList = ");
			for(ipr = m_SeekList.begin(); ipr!=m_SeekList.end(); ipr++)
				fprintf(df, ipr==m_SeekList.begin() ? "%p" : ", %p", *ipr);
			fprintf(df, ";\n");
		}
		m_DiaElement.Save(df, "\t\t");
		fprintf(df, "\t\tPlatform = %p;\n", m_Platform);
	}
	CTrain *train = m_TrainList;
	while(train){
		train->Save(df);
		train = train->Next();
	}
	fprintf(df, "\t}\n");
}

CTrainGroup *GetTrainGroupBySerial(int serial){
	int i = 0;
	CTrainGroup *group = g_SaveFile->GetTrainGroup();
	while(group){
		if(i==serial) return group;
		i++;
		group = group->Next();
	}
	return NULL;
}
