#pragma once

typedef struct _STRUCT_DATA_INFO
{
	LPTSTR szFileName;
	ULONGLONG ullOffset;
	DWORD     dwDataSize;
	LPBYTE    pData;
}DATA_INFO,*PDATA_INFO;
typedef CList<DATA_INFO,DATA_INFO&> DATA_QUEUE;
#define MAX_LENGTH_OF_DATA_QUEUE  100

class CDataQueue
{
public:
	CDataQueue(void);
	~CDataQueue(void);

	int GetCount();
	void AddTail(DATA_INFO dataInfo);
	DATA_INFO GetHead();
	DATA_INFO GetHeadRemove();
	void RemoveHead();
	void Clear();

private:
	DATA_QUEUE m_DataQueue;
	CCriticalSection m_cs;
};

typedef CList<CDataQueue *,CDataQueue *> CDataQueueList;

