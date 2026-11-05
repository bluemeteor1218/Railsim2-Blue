#ifndef CJOBTIMER_H_INCLUDED
#define CJOBTIMER_H_INCLUDED

#define ENABLE_JOB_TIMER (0)

class CTimerObj{
public:
	struct CTimerData{
	public:
		LONGLONG m_Start;
		LONGLONG m_End;
	public:
		LONGLONG GetDelta() const{ return m_End-m_Start; }
	};

	struct CFrameData{
	public:
		vector<CTimerData> m_Data;
		LONGLONG m_Total;
	public:
		CFrameData(): m_Total(0) {}
	};

private:
	CFrameData m_FrameData[2];
	int m_OddFrame;

	string m_Name;
	int m_FrameCount;
	bool m_IsStarted;
	int m_InactiveFrames;
public:
	CTimerObj(const char* name);
	void InitFrame();
	void Start();
	void Stop();
	void Update();

	const char* GetName() const{ return m_Name.c_str(); }
	void SetName(const char* name){ m_Name = name; }
	bool IsStarted() const{ return m_IsStarted; }
	int GetInactiveFrames() const{ return m_InactiveFrames; }
	const CFrameData& GetFrameData(int flip) const{ return m_FrameData[(m_OddFrame+flip)&1]; }
};

class CTimerRAII{
private:
	CTimerObj* m_Obj;
public:
	CTimerRAII(CTimerObj* obj): m_Obj(obj) { m_Obj->Start(); }
	~CTimerRAII() { m_Obj->Stop(); }
};
#if ENABLE_JOB_TIMER
#define TIMER_RAII(name) \
	static CTimerObj _timer_obj_##__LINE__(name); \
	CTimerRAII _timer_raii_##__LINE__(&_timer_obj_##__LINE__)
#else
#define TIMER_RAII(name)
#endif

class CJobTimer{
private:
	struct CFrameData{
	public:
		vector<CTimerObj*> m_ObjList;
		LONGLONG m_Start;
		LONGLONG m_End;
	};
	CFrameData m_FrameData[2];
	int m_OddFrame;
	int m_FrameCount;
	int m_SpanFrames;
	int m_SpanFrameCount;

public:
	CJobTimer();
	int GetFrameCount() const{ return m_FrameCount; }
	void Clear();
	void AddObj(CTimerObj*);
	void Start();
	void Stop();
	void DrawResult();
};

extern CJobTimer g_JobTimer;

#endif
