#include "stdafx.h"
#include "CPluginTree.h"
#include "CSkinPlugin.h"

//	外部グローバル
extern set<string> g_LackPlugin;

//	static メンバ
bool CPlugin::ms_PreviewState = false;

/*
 *	コンストラクタ
 */
CPlugin::CPlugin(
	char *id	//	ID
){
	m_Script = m_Buffer = NULL;
	m_State = 0;
	m_InsertTreeFlag = false;
	m_ID = id;
	m_Version = 0.0f;
	m_IconTex = NULL;
	m_IconRect[0] = m_IconRect[1] = 0.0f; m_IconRect[2] = m_IconRect[3] = 1.0f;
	m_Next = NULL;
}

/*
 *	デストラクタ
 */
CPlugin::~CPlugin(){
	DELETE_A(m_Buffer);
}

/*
 *	ソート用比較関数
 */
int CPlugin::Compare(
	CPlugin *rhs	//	右辺
){
	int ret = _mbsicmp((PUCHAR)m_Name.c_str(), (PUCHAR)rhs->m_Name.c_str());
	if(!ret) ret = _mbsicmp((PUCHAR)m_ID.c_str(), (PUCHAR)rhs->m_ID.c_str());
	if(!ret) ret = _mbsicmp((PUCHAR)m_Author.c_str(), (PUCHAR)rhs->m_Author.c_str());
	return ret;
}

/*
 *	ディレクトリ移動
 */
bool CPlugin::ChDir(){
	return !chdir(g_BaseDir) && !chdir(DirName()) && !chdir(m_ID.c_str());
}

/*
 *	エラーをハンドル
 */
void CPlugin::HandleError(
	CSynErr *err	//	エラーハンドラ
){
	err->Handle(FlashIn("%s <%s>", DirName(), m_ID.c_str()), m_Buffer);
	DELETE_A(m_Buffer);
}

/*
 *	定義ファイルヘッダのロード
 */
char *CPlugin::LoadHeader(
	char *str	//	定義ファイル
){
	char *tmp, *eee;
	string type;
	m_IconTex = NULL;
	if(!(str = Space(eee = str))) throw CSynErr(eee);
	if(!(str = BeginBlock(eee = str, "PluginHeader"))) throw CSynErr(eee);
	if(!(str = AsgnFloat(eee = str, "RailSimVersion", &m_Version))) throw CSynErr(eee);
	if(m_Version<2.00f) throw CSynErr(eee, "%s: %.2f", lang(InvalidVersion), m_Version);
	if(RAILSIM_VERSION<m_Version) throw CSynErr(eee, "%s: %.2f", lang(UnsupportedVersion), m_Version);
	if(!(str = AsgnIdentifier(eee = str, "PluginType", &type))) throw CSynErr(eee);
	if(type!=DirName()) throw CSynErr(eee, "%s: %s", lang(InvalidPluginType), type.c_str());
	if(!(str = AsgnString(eee = str, "PluginName", &m_Name))) throw CSynErr(eee);
	if(!(str = AsgnString(eee = str, "PluginAuthor", &m_Author))) throw CSynErr(eee);
	if(tmp = AsgnString(eee = str, "IconTexture", &m_IconFileName)) str = tmp;
	if(tmp = AsgnFloat(eee = str, "IconRect", m_IconRect, 4, false)) str = tmp;
	int i = 0;
	string desc;
	while(tmp = AsgnString(eee = str, "Description", &desc)){
		str = tmp;
		m_Description += i++ ? "\n"+desc : desc;
	}
	if(!(str = EndBlock(eee = str))) throw CSynErr(eee, ERR_ENDBLOCK);
	return str;
}

/*
 *	予備ロード
 */
bool CPlugin::PreLoad(
	FILE *file	//	ファイル
){
	char *str = m_Buffer = LoadBinaryText(file);
	try{
		str = LoadHeader(str);
	}
	catch(CSynErr err){
		HandleError(&err);
		return false;
	}
	m_Script = str;
	return true;
}

/*
 *	予備ロード
 */
bool CPlugin::PreLoadOldForm(
	FILE *file	//	ファイル
){
	try{
		char *name = FlashOut(0), *auth = FlashOut(1);
		if(fscanf(file, "%s %s", name, auth)<2) throw CSynErr(NULL);
		m_Name = name;
		m_Author = auth;
		m_Description = lang(RS1Plugin);
	}
	catch(CSynErr err){
		err.Handle(FlashIn("%s <%s>\n%s", DirName(), m_ID.c_str()), NULL);
		return false;
	}
	fclose(file);
	m_Version = 1.41f;
	return true;
}

/*
 *	基本情報テキストを取得
 */
string CPlugin::GetBasicInfo(){
	return "ID: "+m_ID+"\n"+lang(Name)+": "+m_Name+"\n"+lang(Author)+": "+m_Author;
}

/*
 *	必要ならロード
 */
CPlugin *CPlugin::LoadAndGet(){
	switch(m_State){
	case 1:	//	予備ロード済み
		if(m_Version<2.0f){
			if(!LoadOldForm()) break;
		}else{
			if(!Load()) break;
		}
		m_State = 2;
	case 2:	//	ロード済み
		return this;
	}
	return NULL;
}

/*
 *	ツリーアイテム挿入
 */
CTreeFileElement *CPlugin::InsertItem(
	CTreeDirElement *p,	//	挿入先
	CPluginTree *o		//	ツリービュー
){
	CTreeFileElement *fe = new CTreeFileElement((char *)m_Name.c_str(), p, o, this);
	p->InsertItem(fe);
	m_InsertTreeFlag = true;
	SetTreeElement(fe);
	return fe;
}

/*
 *	アイコンテクスチャ設定
 */
void CPlugin::SetIconTexture(){
	if(m_IconFileName.size()){
		ChDir();
		m_IconTex = g_TexList.Get(FALSE, m_IconFileName.c_str());
		m_IconFileName = "";
	}
	if(m_IconTex){
		devSetTexture(0, m_IconTex);
		SetUVMap(m_IconRect[0], m_IconRect[1],
			m_IconRect[0]+m_IconRect[2], m_IconRect[1]+m_IconRect[3]);
	}else{
		g_Skin->SetInterfaceTexture();
		SetUVMap(0.875f, 0.75f, 1.0f, 0.875f);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	コンストラクタ
 */
CPluginList::CPluginList(){
	m_List = NULL;
	m_PluginNum = 0;
}

/*
 *	デストラクタ
 */
CPluginList::~CPluginList(){
	CPlugin *ptr = m_List;
	while(ptr){
		CPlugin *next = ptr->m_Next;
		delete ptr;
		ptr = next;
	}
	m_List = NULL;
}

/*
 *	定義ファイルのロード
 */
bool CPluginList::List(){
	long filelist;
	_finddata_t data;
	CPlugin **adr = &m_List;
	if(chdir(g_BaseDir) || chdir(DirName())) return false;
	if((filelist = _findfirst("*", &data))>=0){
		do{
			FILE *file;
			if(!(data.attrib&_A_SUBDIR)) continue;
			if(chdir(g_BaseDir) || chdir(DirName()) || chdir(data.name)) continue;
			CPlugin *newpi = NewEntry(data.name);
			if(file = fopen(TextName2(), "rb")){
				if(!newpi->PreLoad(file)){
					delete newpi;
					continue;
				}
			}else if(TextName() && (file = fopen(TextName(), "rt"))){
				if(!newpi->PreLoadOldForm(file)){
					delete newpi;
					continue;
				}
			}else{
				delete newpi;
				continue;
			}
			newpi->m_State = 1;
			*adr = newpi;
			adr = &newpi->m_Next;
			m_PluginNum++;
		} while(!_findnext(filelist, &data));
		_findclose(filelist);
	}
	return true;
}

/*
 *	定義ファイルのロード
 */
bool CPluginList::LoadOne(
	char *defpath,	//	定義ファイルパス
	char *piid,		//	プラグイン ID
	bool oldform	//	RS1PI
){
	CPlugin **adr = &m_List;
	while(*adr) adr = &(*adr)->m_Next;
	if(chdir(g_BaseDir)) return false;
	FILE *file;
	CPlugin *newpi = NewEntry(piid);
	bool success = false;
	if(oldform){
		if(file = fopen(defpath, "rt")){
			if(newpi->PreLoadOldForm(file)) success = true;
			else fclose(file);
		}
	}else{
		if(file = fopen(defpath, "rb")){
			if(newpi->PreLoad(file)) success = true;
			else fclose(file);
		}
	}
	if(!success){
		delete newpi;
		return false;
	}
	newpi->m_State = 1;
	*adr = newpi;
	m_PluginNum++;
	return true;
}

/*
 *	ID からプラグインを探す
 */
CPlugin *CPluginList::FindPlugin(
	const char *id,	//	ID
	bool load		//	読込フラグ
){
	if(!*id) return NULL;
	CPlugin *ptr = m_List;
	while(ptr){
		if(!_mbsicmp((PUCHAR)id, (PUCHAR)ptr->m_ID.c_str()))
			return load ? ptr->LoadAndGet() : ptr;
		ptr = ptr->m_Next;
	}
	if(Default() && _mbsicmp((PUCHAR)id, (PUCHAR)Default()))
		return FindPlugin(Default(), load);
	g_LackPlugin.insert(FlashIn("%s\\%s", DirName(), id));
	return NULL;
}

/*
 *	利用可能なプラグインを探す
 */
CPlugin *CPluginList::FindAvailable(){
	CPlugin *ptr = m_List;
	while(ptr){
		if(ptr->LoadAndGet()) return ptr;
		ptr = ptr->m_Next;
	}
	return NULL;
}

/*
 *	プラグインツリー構築
 */
void CPluginList::BuildTree(
	CPluginTree *tree	//	ツリービュー
){
	CPlugin *ptr = m_List;
	CTreeDirElement *root = tree->GetRoot();
	while(ptr){
		if(!ptr->m_InsertTreeFlag) ptr->InsertItem(root, tree);
		ptr = ptr->m_Next;
	}
	tree->GetRoot()->Sort(true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 *	テキストファイルをバイナリでロード
 */
char *LoadBinaryText(
	FILE *file,	//	ファイル
	int maxbyte	//	読込最大サイズ
){
	if(!file) return NULL;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	if(0<=maxbyte && maxbyte<size) size = maxbyte;
	fseek(file, 0, SEEK_SET);
	char *buf = new char[size+1];
	if(fread(buf, size, 1, file)!=1){
		fclose(file);
		delete [] buf;
		return NULL;
	}
	buf[size] = 0;
	fclose(file);
	return buf;
}

/*
 *	テキストファイルをバイナリでロード
 */
char *LoadBinaryText(
	char *fname,	//	ファイル名
	int maxbyte		//	読込最大サイズ
){
	return LoadBinaryText(fopen(fname, "rb"), maxbyte);
}
