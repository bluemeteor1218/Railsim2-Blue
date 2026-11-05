#ifndef CWAVEARRAY_H_INCLUDED
#define CWAVEARRAY_H_INCLUDED

/*
 *	サウンド同時再生管理クラス
 *
 *	同時再生するサウンドを管理する
 */
class CWaveArray{
private:
	int m_Number;	//	同時再生最大数
	CWave *m_Wave;	//	個別のサウンド
public:
	CWaveArray();
	~CWaveArray();
	void Load(char *, int, bool);
	void Free(){ DELETE_A(m_Wave); }
	void Add(VEC3, int, int ms = 0);
	void AddLoop(VEC3, int);
};

#endif
