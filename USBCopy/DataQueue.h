#pragma once

typedef struct _STRUCT_DATA_INFO
{
	LPTSTR szFileName; //在WPD中记录的是ObjectID
	ULONGLONG ullOffset;
	DWORD     dwDataSize;
	DWORD     dwOldSize;   // 用于记录压缩之前数据大小
	LPBYTE    pData;
}DATA_INFO,*PDATA_INFO;
typedef CList<PDATA_INFO,PDATA_INFO> DATA_QUEUE;
#define MAX_LENGTH_OF_DATA_QUEUE  2000

class CDataQueue
{
public:
	CDataQueue(void);
	~CDataQueue(void);

	int GetCount();
	void AddTail(PDATA_INFO dataInfo);
	PDATA_INFO GetHead();
	PDATA_INFO GetHeadRemove();
	void RemoveHead();
	void Clear();

private:
	DATA_QUEUE m_DataQueue;
	CCriticalSection m_cs;
};

typedef CList<CDataQueue *,CDataQueue *> CDataQueueList;

