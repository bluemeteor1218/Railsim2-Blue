#include "stdafx.h"
#include "CDiaDialog.h"
#include "CRailConnector.h"
#include "CStation.h"
#include "CTrainGroup.h"
#include "CSaveFile.h"
#include "CSkinPlugin.h"

//	内部グローバル
char *g_DiaDefaultString[2] = {lang(IndividualSetting), lang(Default)};

/*
 *	コンストラクタ
 */
CDiaDialogBase::CDiaDialogBase(){
	m_GroupMenu = NULL;
}

/*
 *	デストラクタ
 */
CDiaDialogBase::~CDiaDialogBase(){
	DELETE_V(m_GroupMenu);
	DELETE_V(m_RoutineMenu);
}

/*
 *	初期化
 */
void CDiaDialogBase::Init(
	int x, int y,	//	位置
	int w, int h,	//	サイズ
	char *title,	//	タイトル
	CInterface *p,	//	親
	char *dialabel,	//	グループラベル
	int exth		//	拡張領域高さ
){
	m_ExtHeight = exth;
	m_DiaInst = NULL;
	char *groupcol[3] = {lang(Name), lang(CarNum), lang(Setting)};
	CWindowCtrl::Init(x, y, w, h, title, p, true);
	SetResize(TILE_UNIT*14, m_ExtHeight+TILE_UNIT*13, g_DispWidth, g_DispHeight, this);
	m_NameLabel.Init(TILE_HALF, TILE_UNIT+TILE_QUAD,
		TILE_UNIT*2, TILE_UNIT, lang(Name), this, 0, 1);
	m_NameEdit.Init(TILE_HALF+TILE_UNIT*2, TILE_UNIT+TILE_QUAD, 1, 1, "", this, 64);
	m_GroupListView.Init(TILE_HALF, TILE_UNIT*2+TILE_QUAD*2, w-TILE_UNIT, 1,
		this, 3, groupcol, DRAG_NONE, LISTVIEW_RENAMABLE, this, CMD_GROUP);
	m_GroupMenu = new CPopMenu("", NULL);
	new CPopMenu(lang(ChangeName), m_GroupMenu);
	m_UseDefault.Init(0, 0, 1, 1, lang(UseDefaultSetting), this);
	m_DefaultLabel.Init(0, 0, 1, 1, lang(DefaultSetting), this, 0, 1);
	char *routinecol[1] = {lang(Routine)};
	m_RoutineListView.Init(0, 0, w-TILE_UNIT, 1,
		this, 1, routinecol, DRAG_NONE, 0, this, CMD_ROUTINE);
	m_RoutineAdd.Init(0, 0, 1, 1, lang(Add), this);
	m_RoutineDelete.Init(0, 0, 1, 1, lang(Delete), this);
	m_RoutinePrev.Init(0, 0, 1, 1, lang(Rew), this);
	m_RoutineNext.Init(0, 0, 1, 1, lang(Fow), this);
	m_DiaGroup.Init(0, 0, 1, 1, dialabel, this);
	class CRoutineAdder: public CMenuCommand{
	private:
		CDiaDialogBase *m_Owner;	//	ダイアログ
	public:
		CRoutineAdder(CDiaDialogBase *o){ m_Owner = o; }
		void Exec(){ m_Owner->AddRoutine(); }
	};
	class CRoutineDeleter: public CMenuCommand{
	private:
		CDiaDialogBase *m_Owner;	//	ダイアログ
	public:
		CRoutineDeleter(CDiaDialogBase *o){ m_Owner = o; }
		void Exec(){ m_Owner->DeleteRoutine(); }
	};
	class CRoutineRotater: public CMenuCommand{
	private:
		CDiaDialogBase *m_Owner;	//	ダイアログ
		bool m_Prev;				//	逆回転
	public:
		CRoutineRotater(CDiaDialogBase *o, bool p){ m_Owner = o; m_Prev = p; }
		void Exec(){ m_Owner->RotateRoutine(m_Prev); }
	};
	m_RoutineMenu = new CPopMenu("", NULL);
	(new CPopMenu(lang(AddRoutine), m_RoutineMenu))
		->SetCommand(new CRoutineAdder(this));
	(new CPopMenu(lang(DeleteRoutine), m_RoutineMenu))
		->SetCommand(new CRoutineDeleter(this));
	(new CPopMenu(lang(RewindOne), m_RoutineMenu))
		->SetCommand(new CRoutineRotater(this, true));
	(new CPopMenu(lang(ForwardOne), m_RoutineMenu))
		->SetCommand(new CRoutineRotater(this, false));
}

/*
 *	初期化 2
 */
void CDiaDialogBase::InitFoot(){
	m_ApplyButton.Init(0, 0, 1, 1, lang(Close), this);
	WindowResized(m_Width, m_Height, this);
}

/*
 *	ウィンドウリサイズ
 */
void CDiaDialogBase::WindowResized(
	int w, int h,		//	新規サイズ
	CWindowCtrl *wnd	//	ウィドウコントロール
){
	int varh = h-m_ExtHeight-TILE_UNIT*5-TILE_QUAD*5;
	int lvh1 = varh/2, lvh2 = varh-lvh1;
	int y = TILE_UNIT*2+TILE_QUAD*2, tw = w-TILE_UNIT;
	m_NameEdit.SetSize(w-TILE_UNIT*3, TILE_UNIT);
	m_GroupListView.SetSize(tw, lvh1);
	m_UseDefault.SetPos(TILE_HALF, y += lvh1+TILE_QUAD);
	m_UseDefault.SetSize(tw, TILE_UNIT);
	m_DefaultLabel.SetPos(TILE_HALF, y);
	m_DefaultLabel.SetSize(tw, TILE_UNIT);
	m_RoutineListView.SetPos(TILE_HALF, y += TILE_UNIT+TILE_QUAD);
	m_RoutineListView.SetSize(tw, lvh2-TILE_UNIT);
	m_RoutineAdd.SetPos(TILE_HALF, y += lvh2-TILE_UNIT);
	m_RoutineAdd.SetSize(tw/4, TILE_UNIT);
	m_RoutineDelete.SetPos(TILE_HALF+tw/4, y);
	m_RoutineDelete.SetSize(tw*2/4-tw/4, TILE_UNIT);
	m_RoutinePrev.SetPos(TILE_HALF+tw*2/4, y);
	m_RoutinePrev.SetSize(tw*3/4-tw*2/4, TILE_UNIT);
	m_RoutineNext.SetPos(TILE_HALF+tw*3/4, y);
	m_RoutineNext.SetSize(tw-tw*3/4, TILE_UNIT);
	m_DiaGroup.SetPos(TILE_HALF, y += TILE_UNIT+TILE_QUAD);
	m_DiaGroup.SetSize(tw, m_ExtHeight);
	m_ApplyButton.SetPos(w-TILE_UNIT*4-TILE_HALF, y += m_ExtHeight+TILE_QUAD);
	m_ApplyButton.SetSize(TILE_UNIT*4, TILE_UNIT);
	ResizeDiaDialogBase(tw, m_ExtHeight);
}

/*
 *	メニュー発行
 */
CPopMenu *CDiaDialogBase::Dispatch(
	CMDTYPE type,	//	コマンドタイプ
	DWORD data		//	データ
){
	switch(type){
	case CMD_GROUP: {
		if(data){
			CTrainGroup *group = (CTrainGroup *)((CListElement *)data)->GetData();
			if(group){
				m_GroupMenu->GetMenu(0)->Enable(true);
				m_GroupMenu->GetMenu(0)->SetCommand(new CGroupRenamer(group));
			}else{
				m_GroupMenu->GetMenu(0)->Enable(false);
			}
		}else{
			m_GroupMenu->GetMenu(0)->Enable(false);
		}
		return m_GroupMenu; }
	case CMD_ROUTINE: {
		return m_RoutineMenu; }
	}
	return NULL;
}

/*
 *	ダイアログ有効化
 */
void CDiaDialogBase::Enter(
	CDiaInstBase *dinst	//	ダイヤインスト
){
	m_DiaInst = dinst;
	m_DiaElement = NULL;
	m_NameEdit.SetText((char *)m_DiaInst->m_Name.c_str());
	g_SaveFile->ListGroupDia(&m_GroupListView, m_DiaInst);
	CListElement *le = m_GroupListView.InsertItem(0, lang(Default));
	le->SetString(1, "-");
	le->SetString(2, "-");
	if(g_TrainGroup){
		int i = g_TrainGroup->GetSerial()+1;
		m_GroupListView.SetSelectionMark(i, 0);
		m_GroupListView.EnsureVisible(i);
	}else{
		m_GroupListView.SetSelectionMark(0, 0);
		m_GroupListView.EnsureVisible(0);
	}
	SetTrainGroup(g_TrainGroup);
	m_GroupListView.GiveFocus(true);
	Show(true);
}

/*
 *	編成をセット
 */
void CDiaDialogBase::SetTrainGroup(
	CTrainGroup *group	//	編成
){
	g_TrainGroup = group;
	m_DiaList = NULL;
	if(g_TrainGroup){
		m_UseDefault.Show(true);
		m_DefaultLabel.Show(false);
		if(m_DiaInst->IsInside(g_TrainGroup)){
			m_DiaList = m_DiaInst->Search(g_TrainGroup);
			m_DiaList->ListElement(&m_RoutineListView, -1);
			m_UseDefault.SetCheck(m_DiaList->m_UseDefault);
		}else{
			m_RoutineListView.DeleteAllItems();
			m_UseDefault.SetCheck(true);
		}
	}else{
		m_UseDefault.SetCheck(false);
		m_UseDefault.Show(false);
		m_DefaultLabel.Show(true);
		m_DiaList = m_DiaInst->Search(NULL);
		m_DiaList->ListElement(&m_RoutineListView, 0);
	}
	m_DiaElement = NULL;
}

/*
 *	ダイヤ要素をセット
 */
void CDiaDialogBase::SetDiaElementBase(
	CDiaElementBase *el	//	要素
){
	m_DiaElement = el;
	m_DiaGroup.Show(!!m_DiaElement);
	SetElementValue(m_DiaElement);
}

/*
 *	入力チェック
 */
void CDiaDialogBase::ScanInputDia(){
	if(!m_Visible){
		m_ApplyButton.SetPush(false);
		return;
	}
	if(m_ApplyButton.IsPushed()){
		m_CloseButton->SetPush(true);
		return;
	}
	m_DiaInst->m_Name = m_NameEdit.GetRealtimeText();
	if(m_DiaList){
		bool chk = !!m_UseDefault.GetCheck();
		if(m_DiaList->m_UseDefault!=chk){
			m_DiaList->m_UseDefault = chk;
			if(chk){
				m_RoutineListView.SetSelectionMark(-1, 0);
			}else{
				m_RoutineListView.SetSelectionMark(0, 0);
				m_RoutineListView.EnsureVisible(0);
			}
			if(g_TrainGroup) g_TrainGroup->GetListElement()
				->SetString(2, g_DiaDefaultString[chk]);
		}
	}else if(!m_UseDefault.GetCheck()){
		m_DiaList = m_DiaInst->Search(g_TrainGroup);
		m_DiaList->ListElement(&m_RoutineListView, 0);
	}
	if(m_RoutineAdd.IsPushed()) AddRoutine();
	else if(m_RoutineDelete.IsPushed()) DeleteRoutine();
	else if(m_RoutinePrev.IsPushed()) RotateRoutine(true);
	else if(m_RoutineNext.IsPushed()) RotateRoutine(false);
 	CListElement *gle = m_GroupListView.GetFocusItem();
	if(gle && gle->IsSelected()){
		CTrainGroup *tg = (CTrainGroup *)gle->GetData();
		if(tg!=g_TrainGroup) SetTrainGroup(tg);
	}else{
		m_GroupListView.SetSelectionMark(0, 0);
		m_GroupListView.EnsureVisible(0);
		SetTrainGroup(NULL);
	}
 	CListElement *rle = m_RoutineListView.GetFocusItem();
	if(rle && rle->IsSelected()){
		CDiaElementBase *de = (CDiaElementBase *)rle->GetData();
		if(de!=m_DiaElement) SetDiaElementBase(de);
	}else{
		SetDiaElementBase(NULL);
	}
	ScanInputDiaDialogBase();
}

/*
 *	ルーチン追加
 */
void CDiaDialogBase::AddRoutine(){
	if(!m_DiaList) m_DiaList = m_DiaInst->Search(g_TrainGroup);
	int ret = m_DiaList->AddElement(m_DiaElement);
	m_DiaList->ListElement(&m_RoutineListView, ret);
	if(m_DiaList->Size()==1) m_UseDefault.SetCheck(0);
}

/*
 *	ルーチン削除
 */
void CDiaDialogBase::DeleteRoutine(){
	if(!m_DiaList || !m_DiaElement) return;
	int ret = m_DiaList->DeleteElement(m_DiaElement);
	if(m_DiaList->Size()){
		m_DiaList->ListElement(&m_RoutineListView, ret);
	}else if(g_TrainGroup){
		m_DiaInst->m_DiaMap.erase(g_TrainGroup);
		g_TrainGroup->GetListElement()->SetString(2, g_DiaDefaultString[1]);
		SetTrainGroup(g_TrainGroup);
	}else{
		AddRoutine();
	}
}

/*
 *	ルーチン回転
 */
void CDiaDialogBase::RotateRoutine(
	bool prev	//	逆回転
){
	if(!m_DiaList) return;
	m_DiaList->RotateElement(prev);
	int i = m_RoutineListView.GetSelectionMark()+(prev ? 1 : -1);
	if(i<0) i = m_DiaList->Size()-1;
	else if(i>=m_DiaList->Size()) i = 0;
	m_DiaList->ListElement(&m_RoutineListView, i);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	初期化
 */
void CPointDialog::Init(
	CInterface *p	//	親
){
	int ww = TILE_UNIT*16, wh = TILE_UNIT*20;
	CDiaDialogBase::Init(TILE_UNIT, g_DispHeight-wh-TILE_UNIT,
		ww, wh, lang(PointSetting), p, lang(Action), TILE_UNIT*3);
	char *plabel[3] = {lang(Rnd), lang(Left), lang(Right)};
	int i;
	for(i = 0; i<3; i++) m_Point[i].Init(0, 0, 1, 1,
		plabel[i], &m_DiaGroup, i ? &m_Point[i-1] : NULL);
	InitFoot();
}

/*
 *	ウィンドウリサイズ
 */
void CPointDialog::ResizeDiaDialogBase(
	int w, int h	//	新規サイズ
){
	int i, tw = w-TILE_UNIT*2+TILE_QUAD;
	for(i = 0; i<3; i++){
		m_Point[i].SetPos(TILE_UNIT+tw*i/3, TILE_UNIT+TILE_QUAD);
		m_Point[i].SetSize(tw*(i+1)/3-tw*i/3-TILE_QUAD, TILE_UNIT);
	}
}

/*
 *	入力チェック
 */
void CPointDialog::ScanInputDiaDialogBase(){
	CPointElement *pe = GetElement();
	if(!pe) return;
	int ps = m_Point->GetNumber();
	if(pe->m_PointSwitch!=ps){
		pe->m_PointSwitch = ps;
		pe->UpdateCaption();
	}
}

/*
 *	要素値をセット
 */
void CPointDialog::SetElementValue(
	CDiaElementBase *el	//	ダイヤ要素
){
	if(el){
		CPointElement *pe = (CPointElement *)el;
		m_Point[pe->m_PointSwitch].SetCheck();
	}else{
		m_Point[0].SetCheck();
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	初期化
 */
void CDiaDialog::Init(
	CInterface *p	//	親
){
	int ww = TILE_UNIT*16, wh = TILE_UNIT*25;
	CDiaDialogBase::Init(TILE_UNIT, g_DispHeight-wh-TILE_UNIT,
		ww, wh, lang(DiaSetting), p, lang(Action), TILE_UNIT*8);
	char *alabel[3] = {lang(Stop), lang(Return), lang(Pass)};
	char *tlabel[2] = {lang(StopDuration), lang(DeptTime)};
	int i;
	for(i = 0; i<3; i++) m_Action[i].Init(0, 0, 1, 1,
		alabel[i], &m_DiaGroup, i ? &m_Action[i-1] : NULL);
	for(i = 0; i<2; i++) m_TimeType[i].Init(0, 0, 1, 1,
		tlabel[i], &m_DiaGroup, i ? &m_TimeType[i-1] : NULL);
	m_HourEdit.Init(0, 0, TILE_UNIT*2, TILE_UNIT, "", &m_DiaGroup, 6);
	m_HourLabel.Init(0, 0, TILE_UNIT, TILE_UNIT, lang(Hr), &m_DiaGroup, 0, 1);
	m_MinuteEdit.Init(0, 0, TILE_UNIT*2, TILE_UNIT, "", &m_DiaGroup, 2);
	m_MinuteLabel.Init(0, 0, TILE_UNIT, TILE_UNIT, lang(Min), &m_DiaGroup, 0, 1);
	m_SecondEdit.Init(0, 0, TILE_UNIT*2, TILE_UNIT, "", &m_DiaGroup, 2);
	m_SecondLabel.Init(0, 0, TILE_UNIT, TILE_UNIT, lang(Sec), &m_DiaGroup, 0, 1);
//	m_JointCheck.Init(0, 0, 1, 1, "他編成との連結を許可", &m_DiaGroup);
	m_StopPosEdit.Init(0, 0, TILE_UNIT*8-TILE_HALF, TILE_UNIT, "", &m_DiaGroup, 9);
	m_StopPosButton.Init(0, 0, TILE_UNIT*3, TILE_UNIT, lang(Apply), &m_DiaGroup);
	InitFoot();
	m_OffsetSlide = 0;
}

/*
 *	ウィンドウリサイズ
 */
void CDiaDialog::ResizeDiaDialogBase(
	int w, int h	//	新規サイズ
){
	int i, tw = w-TILE_UNIT*2+TILE_QUAD;
	for(i = 0; i<3; i++){
		m_Action[i].SetPos(TILE_UNIT+tw*i/3, TILE_UNIT+TILE_QUAD);
		m_Action[i].SetSize(tw*(i+1)/3-tw*i/3-TILE_QUAD, TILE_UNIT);
	}
	for(i = 0; i<2; i++){
		m_TimeType[i].SetPos(TILE_UNIT+tw*i/2, TILE_UNIT*2+TILE_QUAD);
		m_TimeType[i].SetSize(tw*(i+1)/2-tw*i/2-TILE_QUAD, TILE_UNIT);
	}
	int tx = (w-TILE_UNIT*(2+9)-TILE_HALF*5)/2;
	m_HourEdit.SetPos(tx+TILE_UNIT, TILE_UNIT*3+TILE_QUAD);
	m_HourLabel.SetPos(tx+TILE_UNIT*3+TILE_HALF, TILE_UNIT*3+TILE_QUAD);
	m_MinuteEdit.SetPos(tx+TILE_UNIT*5, TILE_UNIT*3+TILE_QUAD);
	m_MinuteLabel.SetPos(tx+TILE_UNIT*7+TILE_HALF, TILE_UNIT*3+TILE_QUAD);
	m_SecondEdit.SetPos(tx+TILE_UNIT*9, TILE_UNIT*3+TILE_QUAD);
	m_SecondLabel.SetPos(tx+TILE_UNIT*11+TILE_HALF, TILE_UNIT*3+TILE_QUAD);
//	m_JointCheck.SetPos(TILE_UNIT, TILE_UNIT*4+TILE_QUAD);
//	m_JointCheck.SetSize(w-TILE_UNIT*2, TILE_UNIT);
	m_StopPosEdit.SetPos(tx+TILE_UNIT, TILE_UNIT*6+TILE_QUAD);
	m_StopPosButton.SetPos(tx+TILE_UNIT*9, TILE_UNIT*6+TILE_QUAD);
}

/*
 *	入力チェック
 */
void CDiaDialog::ScanInputDiaDialogBase(){
	CDiaElement *de = GetElement();
	if(!de) return;
	bool update = false;
	int act = m_Action->GetNumber();
	if(de->m_Action!=act){
		de->m_Action = act;
		update = true;
	}
	int tt = m_TimeType->GetNumber();
	if(de->m_TimeType!=tt){
		de->m_TimeType = tt;
		update = true;
	}
	int hour = 0, minute = 0, second = 0;
	sscanf(m_HourEdit.GetRealtimeText().c_str(), "%d", &hour);
	sscanf(m_MinuteEdit.GetRealtimeText().c_str(), "%d", &minute);
	sscanf(m_SecondEdit.GetRealtimeText().c_str(), "%d", &second);
	ValueArea(&hour, 0, tt ? 23 : 999999);
	ValueArea(&minute, 0, 59);
	ValueArea(&second, 0, 59);
	if(de->m_Hour!=hour || de->m_Minute!=minute || de->m_Second!=second){
		de->m_Hour = hour;
		de->m_Minute = minute;
		de->m_Second = second;
		update = true;
	}
//	int jt = m_JointCheck.GetCheck();
//	if(de->m_Joint!=jt){
//		de->m_Joint = jt;
//		update = true;
//	}
	if(m_StopPosEdit.IsFocus() && !m_StopPosEdit.IsComp()
		&& (GetKey(DIK_RETURN)|GetKey(DIK_NUMPADENTER))==S_PUSH){
		m_StopPosEdit.FinishInput();
		m_StopPosButton.SetPush(true);
	}
	if(m_StopPosButton.IsPushed()){
		de->m_Offset = 0.0f;
		sscanf(m_StopPosEdit.GetText(), "%f", &de->m_Offset);
		ValueArea(&de->m_Offset, -1.0f, 1.0f);
		m_StopPosEdit.SetText(FlashIn("%f", de->m_Offset));
		m_StopPosEdit.GiveFocus(false);
		return;
	}
	if(GetButton(DIM_LEFT)>=S_PUSH){
		int px, py, tw = m_DiaGroup.GetWidth()-TILE_UNIT*2;
		m_DiaGroup.GetAbsPos(&px, &py);
		px += TILE_UNIT;
		py += TILE_UNIT*4+TILE_QUAD;
		int hx1 = px+tw/3, hx2 = px+tw*2/3;
		POINT pos = g_Cursor.GetPos();
		if(m_OffsetSlide || GetButton(DIM_LEFT)==S_PUSH &&
			px<=pos.x && pos.x<px+tw && py+TILE_QUAD<=pos.y && pos.y<py+TILE_UNIT*2){
			m_OffsetSlide = 1;
			de->m_Offset = (pos.x-px-tw*0.5f)*3.0f/tw;
			if(py+TILE_UNIT<pos.y) de->m_Offset = Round(de->m_Offset*10.0f)*0.1f;
			ValueArea(&de->m_Offset, -1.0f, 1.0f);
			m_StopPosEdit.FinishInput();
			m_StopPosEdit.SetText(FlashIn("%f", de->m_Offset));
			update = true;
		}
	}else{
		m_OffsetSlide = 0;
	}
	if(update) de->UpdateCaption();
}

/*
 *	レンダリング
 */
void CDiaDialog::RenderDiaDialogBase(){
	CDiaElement *de = GetElement();
	if(!de) return;
	devSetTexture(0, NULL);
	D3DCOLOR col = g_Skin->m_InterfaceData.m_StaticFontColor;
	D3DCOLOR col2 = col&0x00ffffff;
	if(!col2) col2 = 0x01000000;
	int px, py, tw = m_DiaGroup.GetWidth()-TILE_UNIT*2;
	m_DiaGroup.GetAbsPos(&px, &py);
	px += TILE_UNIT;
	py += TILE_UNIT*4+TILE_QUAD;
	int hx1 = px+tw/3, hx2 = px+tw*2/3;
	Draw2DLine(hx1, py+TILE_UNIT+1, hx2-1, py+TILE_UNIT+1, col);
	Draw2DLine(hx1, py+TILE_UNIT+1, hx1, py+TILE_UNIT*2, col, col2);
	Draw2DLine(hx2-1, py+TILE_UNIT+1, hx2-1, py+TILE_UNIT*2, col, col2);
	int i, tpn = 3, ofs = Round(de->m_Offset*tw/3);
	int trl = hx2+2-hx1;
	for(i = 0; i<tpn; i++) Fill2DRect(
		hx1+trl*i/tpn+ofs, py+TILE_QUAD, hx1+trl*(i+1)/tpn-2+ofs, py+TILE_UNIT, col);
	g_StrTex->RenderCenter(px+tw/2, py+TILE_UNIT+FontY(TILE_UNIT), col, 0, lang(Platform));
	g_StrTex->RenderLeft(px, py+TILE_UNIT+FontY(TILE_UNIT), col, 0, lang(Back_A));
	g_StrTex->RenderRight(px+tw, py+TILE_UNIT+FontY(TILE_UNIT), col, 0, lang(Front_A));
}

/*
 *	要素値をセット
 */
void CDiaDialog::SetElementValue(
	CDiaElementBase *el	//	ダイヤ要素
){
	if(el){
		CDiaElement *de = (CDiaElement *)el;
		m_Action[de->m_Action].SetCheck();
		m_TimeType[de->m_TimeType].SetCheck();
		m_HourEdit.FinishInput();
		m_MinuteEdit.FinishInput();
		m_SecondEdit.FinishInput();
		m_HourEdit.SetText(FlashIn("%02d", de->m_Hour));
		m_MinuteEdit.SetText(FlashIn("%02d", de->m_Minute));
		m_SecondEdit.SetText(FlashIn("%02d", de->m_Second));
	//	m_JointCheck.SetCheck(de->m_Joint);
		m_StopPosEdit.SetText(FlashIn("%f", de->m_Offset));
	}else{
		m_Action[0].SetCheck();
		m_TimeType[0].SetCheck();
		m_HourEdit.SetText("");
		m_MinuteEdit.SetText("");
		m_SecondEdit.SetText("");
		m_StopPosEdit.SetText("");
	//	m_JointCheck.SetCheck(0);
	}
}
