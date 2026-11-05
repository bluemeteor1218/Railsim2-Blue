#ifndef CSKINPLUGIN_H_INCLUDED
#define CSKINPLUGIN_H_INCLUDED

#include "CWaveArray.h"
#include "CPlugin.h"

/*
 *	スキンプラグイン
 */
class CSkinPlugin: public CPlugin{
public:
	struct _CURSORDATA{		//	カーソル設定
		int m_Frame, m_Anim;		//	アニメーションカウンタ
		string m_TexFileName;		//	テクスチャファイル名
		int m_ImageSize[2];			//	画像サイズ
		int m_Cursor2DSize[2];		//	カーソルサイズ
		int m_Cursor2DHotSpot[2];	//	ホットスポット座標
		int m_Cursor2DAnimNumber;	//	アニメーション枚数
		int *m_Cursor2DAnimFrame;	//	アニメフレーム長
		LPTEX8 m_CursorTexture;		//	カーソルテクスチャ
	public:
		_CURSORDATA(){ m_Frame = m_Anim = 0; }
		char *Read(char *, char *);
		void LoadData();
		void Render(POINT, float);
	} m_NormalCursorData, m_ResizeCursorData[4];
	struct _INTERFACEDATA{	//	インターフェイス設定
		string m_TexFileName;			//	テクスチャファイル名
		string m_FontName;				//	フォント名
		HFONT m_hFont;					//	フォントハンドル
		D3DCOLOR m_TitleBarFontColor;	//	タイトルバーフォント色
		D3DCOLOR m_ButtonFontColor;		//	ボタンフォント色
		D3DCOLOR m_StaticFontColor;		//	スタティックフォント色
		D3DCOLOR m_FocusFrameColor;		//	フォーカス枠色
	} m_InterfaceData;
	struct _BACKGROUNDDATA{	//	背景設定
		string m_TexFileName;		//	テクスチャファイル名
		D3DCOLOR m_BackgroundColor;	//	背景色
		int m_ImageSize[2];			//	画像サイズ
	} m_BackgroundData;
	struct _FRAMEDATA{	//	フレーム設定
		string m_FrameTexFileName;			//	テクスチャファイル名
		string m_IconTexFileName[MODE_NUM];	//	アイコンファイル名
		D3DCOLOR m_LabelFontColor;			//	タイトルバーフォント色
		D3DCOLOR m_InfoFontColor;			//	情報フォント色
		D3DCOLOR m_FloatFontColor;			//	フロートフォント色
	} m_FrameData;
	struct _EDITCTRLDATA{	//	エディットコントロール設定
		D3DCOLOR m_DefaultFontColor;		//	既定フォント色
		D3DCOLOR m_EditBaseColor[4];		//	偶数列背景色
		D3DCOLOR m_EditFontColor;			//	編集中フォント色
		D3DCOLOR m_ConvertFontColor;		//	変換中フォント色
		D3DCOLOR m_ConvertClauseColor[2];	//	変換中文節下線色
		D3DCOLOR m_SelectedBaseColor[4];	//	選択中背景色
	} m_EditCtrlData;
	struct _LISTVIEWDATA{	//	リストボックス設定
		D3DCOLOR m_DefaultBaseColorOdd[4];	//	奇数列背景色
		D3DCOLOR m_DefaultBaseColorEven[4];	//	偶数列背景色
		D3DCOLOR m_DefaultFontColor;		//	既定フォント色
		D3DCOLOR m_SelectedBaseColor[4];	//	選択背景色
		D3DCOLOR m_SelectedFontColor;		//	選択フォント色
		D3DCOLOR m_FocusFrameColor;			//	フォーカス枠色
	} m_ListViewData;
	struct _PLUGINTREEDATA{	//	プラグインリスト設定
		D3DCOLOR m_DefaultBaseColor[4];		//	既定背景色
		D3DCOLOR m_DefaultFontColor;		//	既定フォント色
		D3DCOLOR m_SelectedBaseColor[4];	//	選択背景色
		D3DCOLOR m_SelectedFontColor;		//	選択フォント色
		D3DCOLOR m_FocusFrameColor;			//	フォーカス枠色
	} m_PluginTreeData;
	struct _POPUPMENUDATA{	//	ポップアップメニュー設定
		D3DCOLOR m_DefaultFontColor;		//	既定フォント色
		D3DCOLOR m_DisabledFontColor;		//	無効フォント色
		D3DCOLOR m_DisabledShadowColor;		//	無効陰影色
		D3DCOLOR m_SelectedBaseColor[4];	//	選択背景色
		D3DCOLOR m_SelectedFontColor;		//	選択フォント色
	} m_PopupMenuData;
	struct _MODELDATA{	//	各種モデル設定
		string m_ArrowModelFileName;		//	矢印モデルファイル名
		string m_LinkModelFileName;			//	接続点モデルファイル名
		string m_SegmentModelFileName;		//	セグメントモデルファイル名
		string m_CompassModelFileName[2];	//	コンパスモデルファイル名
		string m_WindDirModelFileName;		//	コンパスモデルファイル名
		float m_ArrowModelScale;			//	矢印モデルスケール
		float m_LinkModelScale;				//	接続点モデルスケール
		float m_SegmentModelScale;			//	セグメントモデルスケール
		float m_CompassModelScale[2];		//	コンパスモデルスケール
		float m_WindDirModelScale;			//	コンパスモデルスケール
	} m_ModelData;
	struct _SOUNDDATA{	//	サウンド設定
		string m_MouseDownWaveFileName;	//	マウス押 wav ファイル名
		string m_MouseUpWaveFileName;	//	マウス離 wav ファイル名
		string m_ErrorWaveFileName;		//	エラー wav ファイル名
		string m_ScreenShotWaveFileName;		//	スクリーンショット撮影 wav ファイル名
		string m_VideoStartWaveFileName;		//	ビデオ撮影開始 wav ファイル名
		string m_VideoStopWaveFileName;		//	ビデオ撮影停止 wav ファイル名
	} m_SoundData;
	LPTEX8 m_InterfaceTexture;		//	インターフェイステクスチャ
	LPTEX8 m_FrameTexture;			//	フレームテクスチャ
	LPTEX8 m_IconTexture[MODE_NUM];	//	アイコンテクスチャ
	LPTEX8 m_WallpaperTexture;		//	壁紙テクスチャ
	CMesh *m_ArrowMesh;				//	矢印メッシュ
	CMesh *m_LinkMesh;				//	接続点メッシュ
	CMesh *m_SegmentMesh;			//	セグメントメッシュ
	CMesh *m_CompassMesh[2];		//	コンパス
	CMesh *m_WindDirMesh;			//	風速計
	CWaveArray m_MouseDown;			//	マウス通過
	CWaveArray m_MouseUp;			//	クリック
	CWaveArray m_Error;				//	エラー
	CWaveArray m_ScreenShot;		//	スクリーンショット撮影
	CWaveArray m_VideoStart;		//	ビデオ撮影開始
	CWaveArray m_VideoStop;			//	ビデオ撮影停止
public:
	CSkinPlugin(char *);
	~CSkinPlugin();
	char *DirName(){ return "Skin"; }
	char *TextName2(){ return "Skin2.txt"; }
	bool Load();
	void SetPreview();
	void SetInterfaceTexture(){ devSetTexture(0, m_InterfaceTexture); }
	void SetFrameTexture(){ devSetTexture(0, m_FrameTexture); }
	void SetIconTexture(int i){ devSetTexture(0, m_IconTexture[i]); }
	HFONT GetFont(){ return m_InterfaceData.m_hFont; }
	HFONT GetLabelFont(){ return m_InterfaceData.m_hFont; }
	void MouseDown();
	void MouseUp();
	void Error();
	void ScreenShot();
	void VideoStart();
	void VideoStop();
	void DrawBackground(bool);
	void ScaleCompass(float);
	CPLUGIN_CASTFUNC(CSkinPlugin);
};

/*
 *	スキンプラグインリスト
 */
class CSkinPluginList: public CPluginList{
private:
public:
	char *DirName(){ return "Skin"; }
	char *TextName2(){ return "Skin2.txt"; }
	char *Default(){ return "Default_Blue"; }
	CPlugin *NewEntry(char *id){ return new CSkinPlugin(id); }
	CPLUGINLIST_CASTFUNC(CSkinPlugin);
};

//	外部グローバル
extern CObject g_ArrowObject;
extern CObject g_LinkObject;
extern CObject g_SegmentObject;
extern CObject g_CompassObject[];
extern CObject g_WindDirObject;
extern CSkinPlugin *g_Skin;
extern CSkinPluginList *g_SkinPluginList;

#endif
