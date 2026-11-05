#ifndef CPOPMENU_H_INCLUDED
#define CPOPMENU_H_INCLUDED

/*
 *	メニューコマンドオブジェクト
 */
class CMenuCommand{
public:
	virtual ~CMenuCommand(){}
	virtual void Exec() = 0;
};

/*
 *	ポップアップメニュー
 */
class CPopMenu{
private:
	static CPopMenu *ms_ActiveMenu;	//	使用中のメニュー
	int m_ChildNum;				//	子数
	int m_PosX, m_PosY;			//	座標
	int m_Width, m_Height;		//	メニューサイズ
	int m_Expand;				//	展開方向
	bool m_Enabled;				//	有効フラグ
	string m_String;			//	文字列
	CMenuCommand *m_Command;	//	コマンド
	CPopMenu *m_Pointed;		//	ポイントした ID
	CPopMenu *m_Parent;			//	親
	CPopMenu *m_Brother;		//	兄弟
	CPopMenu *m_Child;			//	子
public:
	static int ScanInputAll();
	static void RenderAll();
	static void ResetCurrentMenu(){ ms_ActiveMenu = NULL; }
	CPopMenu(char *, CPopMenu *);
	~CPopMenu();
	void SetString(char *str){ m_String = str; }
	void InsertMenu(CPopMenu *);
	bool IsExpand(){ return m_Expand>=0; }
	bool IsEnabled(){ return m_Enabled; }
	void Enable(bool en){ m_Enabled = en; }
	bool IsInside(int, int);
	int HitTest(int, int);
	CPopMenu *GetMenu(int);
	void ClearCommand(){ DELETE_V(m_Command); }
	void SetCommand(CMenuCommand *cmd){ ClearCommand(); m_Command = cmd; }
	void Popup(int, int, int tw = 0, int th = 0, int dir = 3);
	int ScanInput();
	void Render();
};

//	コマンドタイプ
typedef enum{
	CMD_NONE = 0,	//	不明
	CMD_PITVELEM,	//	プラグインツリー
	CMD_PILVELEM,	//	プラグインリスト
	CMD_GROUP,		//	編成
	CMD_TRAIN,		//	車輌
	CMD_SCENE,		//	シーン
	CMD_GROUPTMP,	//	編成テンプレート
	CMD_ROUTINE,	//	ダイヤルーチン
	CMD_FILE,		//	ファイル
	CMD_NETWORK,	//	ネットワーク
} CMDTYPE;

/*
 *	メニューコマンダ
 */
class CMenuCommander{
public:
	virtual CPopMenu *Dispatch(CMDTYPE, DWORD) = 0;
	virtual void DoubleClick(CMDTYPE, DWORD){}
};

#endif
