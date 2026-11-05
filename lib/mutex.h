//	Copyright (c) 2002 Midikyou

#define MUTEX_NAME "udx mutex"

class CMutex{
	HANDLE m_hMutex;
public:

	BOOL BeginSingleBoot();
	void EndSingleBoot();
};

/*
 *	多重起動禁止
 */
inline BOOL CMutex::BeginSingleBoot(){
	Debug("多重起動を禁止します.\n");

	m_hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
	DWORD ret = WaitForSingleObject(m_hMutex, 0);

	if(!(ret==WAIT_OBJECT_0 || ret==WAIT_ABANDONED))
		return FALSE;
	else
		return TRUE;
}

/*
 *	多重起動解禁
 */
inline void CMutex::EndSingleBoot(){
	ReleaseMutex(m_hMutex);
}
