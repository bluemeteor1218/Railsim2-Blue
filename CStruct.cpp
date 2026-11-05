#include "stdafx.h"
#include "CListView.h"
#include "CSimpleDialog.h"
#include "CScene.h"
#include "CStructPlugin.h"
#include "CSkinPlugin.h"
#include "CStructEditMode.h"

//	外部グローバル
extern MAT8 *g_AltMaterial;
extern MAT8 g_MatSelect[];

//	static メンバ
CStruct **CStruct::ms_Root = NULL;

/*
 *	コンストラクタ (読込用)
 */
CStruct::CStruct(){
	m_StructPlugin = NULL;
	FixPosture(V3ZERO, V3DIR, V3UP);
	m_Scene = g_Scene;
	m_Next = NULL;
}

/*
 *	コンストラクタ (地形用)
 */
CStruct::CStruct(
	CStructPlugin *bpi	//	施設プラグイン
):
	CModelInst(bpi)	//	基本クラス
{
	m_StructPlugin = bpi;
	FixPosture(V3ZERO, V3DIR, V3UP);
	m_Scene = NULL;
	m_Next = NULL;
}

/*
 *	コンストラクタ
 */
CStruct::CStruct(
	CStructPlugin *bpi,	//	施設プラグイン
	VEC3 pos,			//	位置
	VEC3 dir,			//	dir
	VEC3 up,			//	up
	bool link			//	リスト連結フラグ
):
	CModelInst(bpi)	//	基本クラス
{
	m_StructPlugin = bpi;
	FixPosture(pos, dir, up);
	bpi->SetSwitch(this);
	bpi->SetPartsInst(this);
	bpi->SetMoverState(this);
	bpi->SetPosture();
	m_Scene = g_Scene;
	if(link){
		m_Next = *ms_Root;
		*ms_Root = this;
	}else{
		m_Next = NULL;
	}
}

/*
 *	デストラクタ
 */
CStruct::~CStruct(){
	DELETE_V(m_Next);
}

/*
 *	プラグイン選択モード取得
 */
bool CStruct::IsSelectVisible(){
	return g_StructEditMode->IsModeActive();
}

/*
 *	プラグイン選択モード取得
 */
CPartsInst *CStruct::FindParts(CNamedObject *nobj){
	list<CFreeObjectContainer>::iterator n_itr = m_StructPlugin->m_FreeObject.begin();
	list<CPartsInst>::iterator p_itr = m_PartsInst.begin();
	while(true){
		if(n_itr==m_StructPlugin->m_FreeObject.end()) break;
		if(p_itr==m_PartsInst.end()) break;
		int i;
		for(i = 0; i<n_itr->GetNamedObjectNum(); ++i){
			if(n_itr->GetNamedObject(i)==nobj) return &*p_itr;
			++p_itr;
		}
		++n_itr;
	}
	return NULL;
}

/*
 *	撤去
 */
void CStruct::Remove(){
	m_Scene->DeleteStruct(this);
}

/*
 *	リストから削除
 */
void CStruct::Delete(){
	m_Next = NULL;
	delete this;
}

/*
 *	速度情報表示
 */
void CStruct::PrintInfo(){
}

/*
 *	操作
 */
CModelInst *CStruct::Control(){
	if(CEditBox::IsActive()) return this;
	if(GetKey(DIK_DELETE)==S_PUSH){
		class CStructDeleter: public CMenuCommand{
		private:
			CStruct *m_Struct;	//	施設
		public:
			CStructDeleter(CStruct *s){ m_Struct = s; }
			void Exec(){
				void PushUndoStack();
				PushUndoStack();
				m_Struct->Remove();
			}
		};
		CYesNoDialog *dlg = new CYesNoDialog(
			lang(DeleteStructCfmMessage), lang(Confirmation), false);
		dlg->SetYesCommand(new CStructDeleter(this));
		EnqueueCommonDialog(dlg);
	}
	return this;
}

/*
 *	入力チェック
 */
void CStruct::ScanInput(
	int mode,		//	モード (0: link)
	VEC3 &rect1,	//	領域始点
	VEC3 &rect2		//	領域終点
){
	ms_CurrentInst = this;
	ms_DetectTemp = 0;
	SetSwitchStruct();
	m_StructPlugin->SetSwitch(this);
	m_StructPlugin->SetPartsInst(this);
	m_StructPlugin->ScanInput(this);
	switch(mode){
	case 2:
		if(ms_DetectTemp) m_Selected |= 1; else m_Selected &= 2;
		break;
	case 3:
		if(CheckCtrl()){
			m_Selected &= ~m_Selected<<1;
		}else{
			if(CheckShift()) m_Selected |= m_Selected<<1;
			else m_Selected = m_Selected<<1;
		}
		m_Selected &= 2;
		break;
	case 4:
		m_Selected &= 2;
		break;
	case 5:
		m_Selected = 0;
		break;
	case 6:
		m_Selected = m_Selected<<1;
		m_Selected &= 2;
		break;
	}
}

/*
 *	レンダリング
 */
void CStruct::Render(){
	if(m_Selected && IsSelectVisible()) g_AltMaterial = &g_MatSelect[m_Selected];
	SetSwitchStruct();
	m_StructPlugin->SetSwitch(this);
	m_StructPlugin->SetPartsInst(this);
	m_StructPlugin->Render(this);
	g_AltMaterial = NULL;
}

/*
 *	シミュレート進行
 */
void CStruct::SimulateModelInst(){
	SetSwitchStruct();
	m_StructPlugin->SetSwitch(this);
	m_StructPlugin->SetPartsInst(this);
	m_StructPlugin->Simulate(this);
	SimulateStruct();
}

/*
 *	読込
 */
char *CStruct::Read(
	char *str	//	対象文字列
){
	char *eee;
	if(!(str = BeginBlock(str, "Struct"))){
		delete this;
		return NULL;
	}
	void *oldadr;
	if(!(str = AsgnPointer(eee = str, "Address", &oldadr))) throw CSynErr(eee);
	g_AddressMap[oldadr] = this;
	string pid;
	if(!(str = AsgnString(eee = str, "StructPlugin", &pid))) throw CSynErr(eee);
	m_ModelPlugin = m_StructPlugin = g_StructPluginList->FindPlugin(pid.c_str(), true);
	if(!(str = ReadModelInst(eee = str, true))) throw CSynErr(eee);
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	*ms_Root = this;
	ms_Root = &m_Next;
	return str;
}

/*
 *	保存
 */
void CStruct::Save(
	FILE *df	//	ファイル
){
	fprintf(df, "\t\t\tStruct{\n");
	fprintf(df, "\t\t\t\tAddress = %p;\n", this);
	fprintf(df, "\t\t\t\tStructPlugin = \"%s\";\n", CheckPluginID(m_StructPlugin));
	SaveModelInst(df, "\t\t\t\t", true);
	fprintf(df, "\t\t\t}\n");
}
