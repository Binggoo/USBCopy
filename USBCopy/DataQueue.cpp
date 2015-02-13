#include "StdAfx.h"
#include "DataQueue.h"


CDataQueue::CDataQueue(void)
{
// 	m_hSemaphoneIn = CreateSemaphore(NULL,MAX_LENGTH_OF_DATA_QUEUE,MAX_LENGTH_OF_DATA_QUEUE,NULL);
// 
// 	m_hSemaphoneOut = CreateSemaphore(NULL,0,MAX_LENGTH_OF_DATA_QUEUE,NULL);
}


CDataQueue::~CDataQueue(void)
{
	Clear();

// 	if (m_hSemaphoneIn)
// 	{
// 		CloseHandle(m_hSemaphoneIn);
// 	}
// 
// 	if (m_hSemaphoneOut)
// 	{
// 		CloseHandle(m_hSemaphoneOut);
// 	}
}

void CDataQueue::AddTail( PDATA_INFO dataInfo )
{
//	WaitForSingleObject(m_hSemaphoneIn,SEMAPHONE_TIME_OUT);

	m_cs.Lock();
	m_DataQueue.AddTail(dataInfo);

//	ReleaseSemaphore(m_hSemaphoneOut,1,NULL);
	m_cs.Unlock();
}

PDATA_INFO CDataQueue::GetHead()
{
//	WaitForSingleObject(m_hSemaphoneOut,SEMAPHONE_TIME_OUT);

	m_cs.Lock();
	PDATA_INFO dataInfo = NULL;

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();
	}
	
	m_cs.Unlock();
	return dataInfo;
}

PDATA_INFO CDataQueue::GetHeadRemove()
{
//	WaitForSingleObject(m_hSemaphoneOut,SEMAPHONE_TIME_OUT);

	m_cs.Lock();
	PDATA_INFO temp = NULL;
	PDATA_INFO dataInfo = NULL;

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();

		if (dataInfo)
		{
			temp = new DATA_INFO;
			temp->ullOffset = dataInfo->ullOffset;
			temp->dwDataSize = dataInfo->dwDataSize;
			temp->dwOldSize = dataInfo->dwOldSize;
			temp->pData = new BYTE[dataInfo->dwDataSize];
			memcpy(temp->pData,dataInfo->pData,dataInfo->dwDataSize);

			if (dataInfo->szFileName)
			{
				temp->szFileName = new TCHAR[_tcslen(dataInfo->szFileName)+1];
				_tcscpy_s(temp->szFileName,_tcslen(dataInfo->szFileName)+1,dataInfo->szFileName);

				delete []dataInfo->szFileName;
				dataInfo->szFileName = NULL;
			}

			if (dataInfo->pData)
			{
				delete []dataInfo->pData;
				dataInfo->pData = NULL;
			}

			delete dataInfo;
			dataInfo = NULL;
			
		}
		
		m_DataQueue.RemoveHead();

//		ReleaseSemaphore(m_hSemaphoneIn,1,NULL);
	}	
	m_cs.Unlock();
	return temp;
}


void CDataQueue::RemoveHead()
{
	m_cs.Lock();
	PDATA_INFO dataInfo = NULL;

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();
	}

 	if (dataInfo)
	{
		if (dataInfo->pData)
		{
			delete [] dataInfo->pData;
			dataInfo->pData = NULL;
		}

		if (dataInfo->szFileName)
		{
			delete []dataInfo->szFileName;
			dataInfo->szFileName = NULL;
		}

		delete dataInfo;
		dataInfo = NULL;

		m_DataQueue.RemoveHead();

//		ReleaseSemaphore(m_hSemaphoneIn,1,NULL);
	}
	m_cs.Unlock();
}

void CDataQueue::Clear()
{
	m_cs.Lock();
	POSITION pos = m_DataQueue.GetHeadPosition();
	long count = m_DataQueue.GetCount();
	while (pos)
	{
		PDATA_INFO dataInfo = m_DataQueue.GetNext(pos);

		if (dataInfo)
		{
			if (dataInfo->pData)
			{
				delete [] dataInfo->pData;
				dataInfo->pData = NULL;
			}

			if (dataInfo->szFileName)
			{
				delete []dataInfo->szFileName;
				dataInfo->szFileName = NULL;
			}

			delete dataInfo;
			dataInfo = NULL;
		}
	}
	m_DataQueue.RemoveAll();

//	ReleaseSemaphore(m_hSemaphoneIn,count,NULL);

	m_cs.Unlock();
}

int CDataQueue::GetCount()
{
	int nCount = m_DataQueue.GetCount();
	return nCount;
}