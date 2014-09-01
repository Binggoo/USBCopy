#include "StdAfx.h"
#include "DataQueue.h"


CDataQueue::CDataQueue(void)
{

}


CDataQueue::~CDataQueue(void)
{
	Clear();
}

void CDataQueue::AddTail( DATA_INFO dataInfo )
{
	m_cs.Lock();
	m_DataQueue.AddTail(dataInfo);
	m_cs.Unlock();
}

DATA_INFO CDataQueue::GetHead()
{
	m_cs.Lock();
	DATA_INFO dataInfo = {0};

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();
	}
	
	m_cs.Unlock();
	return dataInfo;
}

DATA_INFO CDataQueue::GetHeadRemove()
{
	m_cs.Lock();
	DATA_INFO temp = {0};
	DATA_INFO dataInfo = {0};

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();
		temp.ullOffset = dataInfo.ullOffset;
		temp.dwDataSize = dataInfo.dwDataSize;
		temp.dwOldSize = dataInfo.dwOldSize;
		temp.pData = new BYTE[dataInfo.dwDataSize];
		memcpy(temp.pData,dataInfo.pData,dataInfo.dwDataSize);

		if (dataInfo.szFileName)
		{
			temp.szFileName = new TCHAR[_tcslen(dataInfo.szFileName)+1];
			_tcscpy_s(temp.szFileName,_tcslen(dataInfo.szFileName)+1,dataInfo.szFileName);

			delete []dataInfo.szFileName;
		}
		delete []dataInfo.pData;
		m_DataQueue.RemoveHead();
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
		dataInfo = &(m_DataQueue.GetHead());
	}

 	if (dataInfo)
	{
		delete [] dataInfo->pData;
		dataInfo->pData = NULL;

		if (dataInfo->szFileName)
		{
			delete []dataInfo->szFileName;
			dataInfo->szFileName = NULL;
		}

		m_DataQueue.RemoveHead();
	}
	m_cs.Unlock();
}

void CDataQueue::Clear()
{
	m_cs.Lock();
	POSITION pos = m_DataQueue.GetHeadPosition();
	while (pos)
	{
		PDATA_INFO dataInfo = &(m_DataQueue.GetNext(pos));

		delete [] dataInfo->pData;
		dataInfo->pData = NULL;

		if (dataInfo->szFileName)
		{
			delete []dataInfo->szFileName;
			dataInfo->szFileName = NULL;
		}
	}
	m_DataQueue.RemoveAll();
	m_cs.Unlock();
}

int CDataQueue::GetCount()
{
	int nCount = m_DataQueue.GetCount();
	return nCount;
}