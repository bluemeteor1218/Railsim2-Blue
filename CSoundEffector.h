#ifndef CSOUNDEFFECTOR_H_INCLUDED
#define CSOUNDEFFECTOR_H_INCLUDED

class CNamedObject;
class CModelPlugin;
class CSoundEffector;
class CScene;

/*
 *	サウンドエフェクタ状態
 */
class CSoundState{
	friend class CSoundEffector;
private:
	int m_State;		//	状態
	bool m_ApplyFlag;	//	適用フラグ
	CWave m_Wave;		//	サウンドデータ
public:
	CSoundState(){ Reset(); }
	~CSoundState();
	void Reset(){ m_State = 0; }
	void Confirm(CSoundEffector *, bool);
};

//	反復子
typedef list<CSoundState>::iterator ISoundState;
typedef set<CSoundState *>::iterator ISPSoundState;

/*
 *	サウンドエフェクタ
 */
class CSoundEffector{
private:
	static set<CSoundState *> ms_PlayList;	//	プレイリスト
	string m_WaveFileName;		//	wave ファイル名
	CNamedObject *m_Link;		//	接続先オブジェクト
	VEC3 m_SourceCoord;			//	位置
	int m_Volume;				//	音量
	bool m_Loop;				//	ループ
	CSoundState *m_LinkState;	//	状態変数
public:
	static void InitPlayList();
	static void DeleteSound(CSoundState *ss){ ms_PlayList.erase(ss); }
	char *Read(char *, CModelPlugin *);
	void LoadData(CSoundState *);
	void SetPos(CSoundState *);
	void PlayWave(CSoundState *);
	void Link(CSoundState *);
	void Register(CScene *);
};

//	反復子
typedef list<CSoundEffector>::iterator ISoundEffector;

#endif
