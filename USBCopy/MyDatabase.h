#pragma once
#include "Port.h"
#define DB_NAME _T("\\phiyo.db")
#define TABLE_NAME _T("table_phiyo")

class CMyDatabase
{
public:
	CMyDatabase(void);
	~CMyDatabase(void);

	BOOL InitialDatabase(CString strDBPath);
	BOOL AddData(CPort *port);

};

