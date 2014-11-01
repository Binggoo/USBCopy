#include "StdAfx.h"
#include "DataQueue.h"


CDataQueue::CDataQueue(void)
{

}


CDataQueue::~CDataQueue(void)
{
	Clear();
}

void CDataQueue::AddTail( PDATA_INFO dataInfo )
{
	m_cs.Lock();
	m_DataQueue.AddTail(dataInfo);
	m_cs.Unlock();
}

PDATA_INFO CDataQueue::GetHead()
{
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
	m_cs.Lock();
	PDATA_INFO temp = NULL;
	PDATA_INFO dataInfo = NULL;

	if (m_DataQueue.GetHeadPosition())
	{
		dataInfo = m_DataQueue.GetHead();

		if (dataInfo)
		{
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

			if (dataInfo->szParentObjectID)
			{
				temp->szParentObjectID = new TCHAR[_tcslen(dataInfo->szParentObjectID)+1];
				_tcscpy_s(temp->szParentObjectID,_tcslen(dataInfo->szParentObjectID)+1,dataInfo->szParentObjectID);

				delete []dataInfo->szParentObjectID;
				dataInfo->szParentObjectID = NULL;
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

		if (dataInfo->szParentObjectID)
		{
			delete []dataInfo->szParentObjectID;
			dataInfo->szParentObjectID = NULL;
		}

		delete dataInfo;
		dataInfo = NULL;

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

			if (dataInfo->szParentObjectID)
			{
				delete []dataInfo->szParentObjectID;
				dataInfo->szParentObjectID = NULL;
			}

			delete dataInfo;
			dataInfo = NULL;
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