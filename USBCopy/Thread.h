#pragma once
class CThread
{
public:
	CThread(void);
	virtual ~CThread(void);

	// Thread
	//�����߳̿��� �������߳�
	virtual bool BeginThread();
	//�߳��Ƿ�����
	bool IsThreadRunning();
	//����߳̾��
	HANDLE GetThread();
	//��ͣ�����߳�
	bool SuspendThread();
	//�ָ������߳�
	bool ResumeThread();
	//��ֹ�߳�
	bool EndThread(DWORD dwWaitTime = 100);
	// �ȴ��߳���ֹ
	BOOL WaitForEnd();

	void SetCancel(LPBOOL lpCancel);

protected:
	HANDLE m_hThread;
	LPBOOL m_pbCancel;
};

