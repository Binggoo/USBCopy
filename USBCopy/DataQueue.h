#pragma once

typedef struct _STRUCT_DATA_INFO
{
	LPTSTR szFileName; //��WPD�м�¼����ObjectID
	ULONGLONG ullOffset;
	DWORD     dwDataSize;
	DWORD     dwOldSize;   // ���ڼ�¼ѹ��֮ǰ���ݴ�С
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

