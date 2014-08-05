#pragma once
class CThread
{
public:
	CThread(void);
	virtual ~CThread(void);

	// Thread
	//辅助线程控制 建监视线程
	virtual bool BeginThread();
	//线程是否运行
	bool IsThreadRunning();
	//获得线程句柄
	HANDLE GetThread();
	//暂停监视线程
	bool SuspendThread();
	//恢复监视线程
	bool ResumeThread();
	//终止线程
	bool EndThread(DWORD dwWaitTime = 100);
	// 等待线程终止
	BOOL WaitForEnd();

	void SetCancel(LPBOOL lpCancel);

protected:
	HANDLE m_hThread;
	LPBOOL m_pbCancel;
};

