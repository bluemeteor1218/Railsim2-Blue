#ifndef CVIDEOMODE_H_INCLUDED
#define CVIDEOMODE_H_INCLUDED

#include "CStaticCtrl.h"
#include "CGroupBox.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CPushButton.h"
#include "CInterfaceMode.h"

/*
 *	シミュレーション設定モード
 */
class CVideoMode: public CInterfaceMode{
private:
	CWindowCtrl m_VideoWindow;		//	窓
	CGroupBox m_PictureGroup;		//	スクリーンショットグループ
	CStaticCtrl m_PictureExpr;		//	スクリーンショット説明
	CStaticCtrl m_PictureInfo;		//	スクリーンショット状態
	CStaticCtrl m_QualityLabel;		//	画質ラベル
	CRadioButton m_Quality[4];		//	画質チェック
	CGroupBox m_VideoGroup;			//	速度グループ
	CStaticCtrl m_VideoExpr;		//	スクリーンショット説明
	CStaticCtrl m_VideoInfo;		//	スクリーンショット状態
	CPushButton m_StartButton;		//	録画開始ボタン
	CPushButton m_StopButton;		//	録画停止ボタン
	CPushButton m_RewindButton;		//	巻戻しボタン
	CPushButton m_ForwardButton;	//	早送りボタン
	CCheckBox m_ExceptPause;		//	ポーズ中撮影しない
	CCheckBox m_OnlyPhotoMode;		//	撮影モード以外撮影しない
	CStaticCtrl m_DownsampleLabel;	//	ダウンサンプルラベル
	CRadioButton m_Downsample[3];	//	ダウンサンプルチェック
	CStaticCtrl m_FormatLabel;		//	フォーマットラベル
	CRadioButton m_Format[2];		//	フォーマットチェック
	//CCheckBox m_VideoSound;			//	録音チェック
public:
	CVideoMode();
	~CVideoMode(){}
	void WindowResized(int, int, CWindowCtrl *);
	char *LoadInterfaceSetting(char *);
	void SaveInterfaceSetting(FILE *);
	void EnterInterface();
	void ScanInputInterface();
	int GetPictureQuality();
	int GetExceptPause(){ return m_ExceptPause.GetCheck(); }
	int GetOnlyPhotoMode(){ return m_OnlyPhotoMode.GetCheck(); }
	int GetDownsample(){ return m_Downsample->GetNumber(); }
	int GetFormat(){ return m_Format->GetNumber(); }
//	int GetVideoSound(){ return m_VideoSound.GetCheck(); }
};

//	外部グローバル
extern CVideoMode *g_VideoMode;

#endif
