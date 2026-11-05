//	Copyright (c) 2002 Midikyou

class CApp{
	CMutex m_mutex;

public:
	void (*m_func)();	//	ループ関数

	~CApp();
	BOOL Init(HINSTANCE hInst);
	void Run();
};
extern CApp theApp;

void WakeUp();
void StartUp();
void MainLoop();
void Main();
void CleanUp();

void GetAppPath(char *path);

/*
 *	ループ関数の設定
 *
 *	func	: 関数
 */
inline void SetLoopFunc(void (*func)()){
	theApp.m_func = func;
}
