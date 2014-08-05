#include "StdAfx.h"
#include "Thread.h"


CThread::CThread(void)
{
	m_hThread = NULL;
}

CThread::~CThread(void)
{
}

bool CThread::BeginThread()
{
	return true;
}

bool CThread::IsThreadRunning()
{
	return (m_hThread != NULL);
}

HANDLE CThread::GetThread()
{
	return m_hThread;
}

bool CThread::SuspendThread()
{
	return IsThreadRunning() ? ::SuspendThread(m_hThread) != 0xFFFFFFFF : false;
}

bool CThread::ResumeThread()
{
	return IsThreadRunning() ? ::ResumeThread(m_hThread) != 0xFFFFFFFF : false;
}

bool CThread::EndThread( DWORD dwWaitTime /*= 100*/ )
{
	if(IsThreadRunning()) 
	{
		if(::WaitForSingleObject(m_hThread, dwWaitTime) != WAIT_OBJECT_0)
		{
			if(!::TerminateThread(m_hThread, 0))
			{
				return false;
			}
		}

		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	return false;
}

BOOL CThread::WaitForEnd()
{
	WaitForSingleObject(m_hThread,INFINITE);
	DWORD dwExitCode = 0;
	GetExitCodeThread(m_hThread,&dwExitCode);
	CloseHandle(m_hThread);
	m_hThread = NULL;

	return dwExitCode;
}

void CThread::SetCancel( LPBOOL lpCancel )
{
	m_pbCancel = lpCancel;
}
