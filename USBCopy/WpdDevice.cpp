#include "StdAfx.h"
#include "USBCopy.h"
#include "WpdDevice.h"
#include "Utils.h"
#include "Checksum32.h"
#include "CRC32.h"
#include "MD5.h"

typedef struct _STRUCT_LPVOID_PARM
{
	LPVOID lpVoid1;   //CDisk
	LPVOID lpVoid2;   //CPort
	LPVOID lpVoid3;   //CHashMethod or CDataQueue
	LPVOID lpVoid4;   //Other
}VOID_PARM,*LPVOID_PARM;

CWpdDevice::CWpdDevice(void)
{
	m_hLogFile = INVALID_HANDLE_VALUE;
	m_ullValidSize = 0;

	m_bHashVerify = FALSE;
	m_pMasterHashMethod = NULL;

	m_nCurrentTargetCount = 0;

	ZeroMemory(m_ImageHash,LEN_DIGEST);
}


CWpdDevice::~CWpdDevice(void)
{
	POSITION pos = m_DataQueueList.GetHeadPosition();
	while (pos)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos);
		delete dataQueue;
	}
	m_DataQueueList.RemoveAll();

	CleanupObjectProperties();

	pos = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos)
	{
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(pos);

		if (pMap)
		{
			pMap->RemoveAll();

			delete pMap;
			pMap = NULL;
		}
	}
	m_TargetMapStringFileObjectsList.RemoveAll();

	if (m_pMasterHashMethod != NULL)
	{
		delete m_pMasterHashMethod;
	}
}

void CWpdDevice::Init( HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile,CPortCommand *pCommand )
{
	m_hWnd = hWnd;
	m_lpCancel = lpCancel;
	m_hLogFile = hLogFile;
	m_pCommand = pCommand;
}

void CWpdDevice::SetMasterPort( CPort *port )
{
	m_MasterPort = port;
	m_MasterPort->Reset();
	m_MasterPort->Active();
}

void CWpdDevice::SetTargetPorts( PortList *pList )
{
	m_TargetPorts = pList;
	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected())
		{
			port->Reset();
			port->SetPortState(PortState_Active);
			m_nCurrentTargetCount++;
		}

		CDataQueue *dataQueue = new CDataQueue();
		m_DataQueueList.AddTail(dataQueue);

		CMapStringToString *pMap = new CMapStringToString();
		m_TargetMapStringFileObjectsList.AddTail(pMap);

	}
}

void CWpdDevice::SetHashMethod( BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod )
{
	m_bComputeHash = bComputeHash;
	m_bHashVerify = bHashVerify;
	m_HashMethod = hashMethod;

	if (m_pMasterHashMethod != NULL)
	{
		delete m_pMasterHashMethod;
		m_pMasterHashMethod = NULL;
	}

	if (bComputeHash)
	{
		switch (hashMethod)
		{
		case HashMethod_CHECKSUM32:
			m_pMasterHashMethod = new CChecksum32();
			break;

		case HashMethod_CRC32:
			m_pMasterHashMethod = new CCRC32();
			break;

		case HashMethod_MD5:
			m_pMasterHashMethod = new MD5();
			break;
		}
	}
}

void CWpdDevice::SetWorkMode( WorkMode workMode )
{
	m_WorkMode = workMode;
}

BOOL CWpdDevice::Start()
{
	ZeroMemory(m_ImageHash,LEN_DIGEST);

	BOOL bRet = FALSE;

	switch (m_WorkMode)
	{
	case WorkMode_MTPCopy:
		bRet = OnCopyWpdFiles();
		break;
	}

	return bRet;
}

BOOL CWpdDevice::OnCopyWpdFiles()
{
	// 清理上一次的结果
	m_ullValidSize = 0;
	CleanupObjectProperties();

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic start......"));
	EnumAllObjects();

	SetValidSize(m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic end, total files count=%d, total size=%I64d")
		,m_MasterFileObjectsList.GetCount(),m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Files List:"));
	CString strList;
	POSITION pos = m_MasterFileObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObjectProp = m_MasterFileObjectsList.GetNext(pos);

		if (pObjectProp)
		{
			strList.Format(_T("ObjectID:%s\tFileName:%s\tFileSize:%I64d"),pObjectProp->pwszObjectID,pObjectProp->pwszObjectOringalFileName,pObjectProp->ullObjectSize);
			CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
		}
		
	}

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadWpdFilesThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Copy(Master) - CreateReadWpdFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	UINT nCount = 0;
	HANDLE *hWriteThreads = new HANDLE[m_nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	POSITION posMap = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(posMap);
		if (port->IsConnected())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;
			lpVoid->lpVoid4 = pMap;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteWpdFilesThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateWriteWpdFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//等待写磁盘线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
	}
	delete []hWriteThreads;

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bHashVerify)
	{
		char function[1024] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,WPARAM(function),0);

		nCount = 0;
		pos = m_TargetPorts->GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
			if (port->IsConnected() && port->GetResult())
			{
				port->SetPortState(PortState_Active);
				port->SetCompleteSize(0);
				port->SetUsedNoWaitTimeS(0);
				port->SetUsedWaitTimeS(0);
				nCount++;
			}
		}

		HANDLE *hVerifyThreads = new HANDLE[nCount];

		nCount = 0;
		pos = m_TargetPorts->GetHeadPosition();
		posMap = m_TargetMapStringFileObjectsList.GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
			CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(posMap);
			if (port->IsConnected() && port->GetResult())
			{
				CHashMethod *pHashMethod;

				switch (port->GetHashMethod())
				{
				case HashMethod_CHECKSUM32:
					pHashMethod = new CChecksum32();
					break;

				case HashMethod_CRC32:
					pHashMethod = new CCRC32();
					break;

				case HashMethod_MD5:
					pHashMethod = new MD5();
					break;
				}

				LPVOID_PARM lpVoid = new VOID_PARM;
				lpVoid->lpVoid1 = this;
				lpVoid->lpVoid2 = port;
				lpVoid->lpVoid3 = pHashMethod;
				lpVoid->lpVoid4 = pMap;

				hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyWpdFilesThreadProc,lpVoid,0,&dwVerifyId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateVerifyWpdFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),dwVerifyId,hVerifyThreads[nCount]);

				nCount++;
			}
		}

		//等待校验线程结束
		WaitForMultipleObjects(nCount,hVerifyThreads,TRUE,INFINITE);

		bResult = FALSE;
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hVerifyThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hVerifyThreads[i]);
		}
		delete []hVerifyThreads;

		if (!bResult)
		{
			// 任意取一个错误
			ErrorType errType = ErrorType_System;
			DWORD dwErrorCode = 0;

			pos = m_TargetPorts->GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				if (port->IsConnected() && !port->GetResult())
				{
					errType = port->GetErrorCode(&dwErrorCode);
					break;
				}
			}

			m_MasterPort->SetErrorCode(errType,dwErrorCode);
		}
	}

	return bResult;

}

void CWpdDevice::EnumAllObjects()
{
	CComPtr<IPortableDevice> pIPortableDevice;
	CComPtr<IPortableDeviceContent> pContent;
	HRESULT hr = OpenDevice(&pIPortableDevice,m_MasterPort->GetDevicePath());

	if (SUCCEEDED(hr) && pIPortableDevice != NULL)
	{
		 hr = pIPortableDevice->Content(&pContent);

		 if (SUCCEEDED(hr))
		 {
			 RecursiveEnumerate(WPD_DEVICE_OBJECT_ID,pContent,0);
		 }
		 else
		 {
			 CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr);
			 return;
		 }
	}
	else
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Failed to open device, hr = 0x%lx"),hr);
		return;
	}

}

void CWpdDevice::RecursiveEnumerate( LPCWSTR szObjectID,IPortableDeviceContent* pContent,int level )
{
	CComPtr<IEnumPortableDeviceObjectIDs> pEnumObjectIDs;
	HRESULT hr = S_OK;

	// 除去DEVICE和根目录，只保存目录和文件
	if (level > 1)
	{
		PObjectProperties pObjectProperties = new ObjectProperties;
		ZeroMemory(pObjectProperties,sizeof(ObjectProperties));

		hr = GetObjectProperties(pContent,szObjectID,pObjectProperties);

		if (FAILED(hr))
		{
			CUtils::WriteLogFile(INVALID_HANDLE_VALUE,TRUE,_T("Failed to GetObjectProperties,hr = 0x%lx"),hr);
			return;
		}

		pObjectProperties->level = level;

		if (pObjectProperties->guidObjectContentType == WPD_CONTENT_TYPE_FOLDER)
		{
			//文件夹
			m_MasterFolderObjectsList.AddTail(pObjectProperties);
		}
		else
		{
			m_MapObjectProerties.SetAt(szObjectID,pObjectProperties);
			m_MasterFileObjectsList.AddTail(pObjectProperties);
			m_ullValidSize += pObjectProperties->ullObjectSize;
		}
	}

	// Get an IEnumPortableDeviceObjectIDs interface by calling EnumObjects with the
	// specified parent object identifier.
	hr = pContent->EnumObjects(0,               // Flags are unused
		szObjectID,     // Starting from the passed in object
		NULL,            // Filter is unused
		&pEnumObjectIDs);
	if (FAILED(hr))
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Failed to get IEnumPortableDeviceObjectIDs from IPortableDeviceContent, hr = 0x%lx"),hr);
	}

	level++;

	// Loop calling Next() while S_OK is being returned.
	while(hr == S_OK)
	{
		DWORD  cFetched = 0;
		PWSTR  szObjectIDArray[NUM_OBJECTS_TO_REQUEST] = {0};
		hr = pEnumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST,   // Number of objects to request on each NEXT call
			szObjectIDArray,          // Array of PWSTR array which will be populated on each NEXT call
			&cFetched);               // Number of objects written to the PWSTR array
		if (SUCCEEDED(hr))
		{
			// Traverse the results of the Next() operation and recursively enumerate
			// Remember to free all returned object identifiers using CoTaskMemFree()
			for (DWORD dwIndex = 0; dwIndex < cFetched; dwIndex++)
			{
				RecursiveEnumerate(szObjectIDArray[dwIndex],pContent,level);

				// Free allocated PWSTRs after the recursive enumeration call has completed.
				CoTaskMemFree(szObjectIDArray[dwIndex]);
				szObjectIDArray[dwIndex] = NULL;

				// 只取机器本身内存,扩展卡不计在内
				if (level == 1)
				{
					break;
				}
			}
		}
	}

}

DWORD WINAPI CWpdDevice::ReadWpdFilesThreadProc( LPVOID lpParm )
{
	CWpdDevice *wpd = (CWpdDevice *)lpParm;

	BOOL bRet = wpd->ReadWpdFiles();

	return bRet;
}

DWORD WINAPI CWpdDevice::WriteWpdFilesThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;

	CWpdDevice *wpd = (CWpdDevice *)parm->lpVoid1;
	CPort *port = (CPort*)parm->lpVoid2;
	CDataQueue *pDataQueue = (CDataQueue *)parm->lpVoid3;
	CMapStringToString *pMap = (CMapStringToString *)parm->lpVoid4;

	BOOL bRet = wpd->WriteWpdFiles(port,pDataQueue,pMap);

	return bRet;
}

DWORD WINAPI CWpdDevice::VerifyWpdFilesThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;

	CWpdDevice *wpd = (CWpdDevice *)parm->lpVoid1;
	CPort *port = (CPort*)parm->lpVoid2;
	CHashMethod *pHashMethod = (CHashMethod *)parm->lpVoid3;
	CMapStringToString *pMap = (CMapStringToString *)parm->lpVoid4;

	BOOL bRet = wpd->VerifyWpdFiles(port,pHashMethod,pMap);

	return bRet;
}

BOOL CWpdDevice::ReadWpdFiles()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullCompleteSize = 0;
	DWORD dwLen = 0;

	CComPtr<IPortableDevice> pIPortableDevice;
	HRESULT hr = OpenDevice(&pIPortableDevice,m_MasterPort->GetDevicePath());

	if (FAILED(hr))
	{
		m_MasterPort->SetEndTime(CTime::GetCurrentTime());
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_MTP_OpenDevice_Failed);
		m_MasterPort->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP copy,custom errorcode=0x%X,Open MTP device failed,hr=0x%lx")
			,m_MasterPort->GetPortName(),CustomError_MTP_OpenDevice_Failed,hr);

		return FALSE;
	}

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	POSITION pos = m_MasterFileObjectsList.GetHeadPosition();
	ULONGLONG ullFileSize = 0;
	CString strObjectID,strFileName;
	while (pos && !*m_lpCancel && bResult && m_MasterPort->GetPortState() == PortState_Active)
	{
		PObjectProperties pObjectProp = m_MasterFileObjectsList.GetNext(pos);

		if (pObjectProp == NULL)
		{
			continue;
		}

		strObjectID = CString(pObjectProp->pwszObjectID);
		strFileName = CString(pObjectProp->pwszObjectOringalFileName);

		CComPtr<IStream> pDataStream;
		DWORD dwTransferSize = 0;
		hr = CreateDataInStream(pIPortableDevice,strObjectID,&pDataStream,&dwTransferSize);

		if (FAILED(hr))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_CreateDataStream_Failed;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateDataInStream failed,object id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
				,m_MasterPort->GetPortName(),strObjectID,strFileName,dwErrorCode,hr);
			break;
		}

		ullFileSize = pObjectProp->ullObjectSize;
		ullCompleteSize = 0;
		do
		{
			QueryPerformanceCounter(&t0);
			// 判断队列是否达到限制值
			while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE) && !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
			{
				Sleep(5);
			}

			if (*m_lpCancel)
			{
				dwErrorCode = CustomError_Cancel;
				errType = ErrorType_Custom;
				bResult = FALSE;
				break;
			}

			if (IsAllFailed(errType,&dwErrorCode))
			{
				bResult = FALSE;
				break;
			}

			dwLen = dwTransferSize;
			if (ullFileSize - ullCompleteSize < dwLen)
			{
				dwLen = (DWORD)(ullFileSize - ullCompleteSize);
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);

			hr = ReadWpdFile(pDataStream,pByte,dwLen);

			if (FAILED(hr))
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_ReadFile_Failed;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - ReadWpdFile failed,object id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
					,m_MasterPort->GetPortName(),strObjectID,strFileName,dwErrorCode,hr);
				break;
			}

			PDATA_INFO dataInfo = new DATA_INFO;
			ZeroMemory(dataInfo,sizeof(DATA_INFO));
			dataInfo->szFileName = new TCHAR[strObjectID.GetLength() + 1];
			_tcscpy_s(dataInfo->szFileName,strObjectID.GetLength()+1,strObjectID);
			dataInfo->ullOffset = ullCompleteSize;
			dataInfo->dwDataSize = dwLen;
			dataInfo->pData = pByte;
			AddDataQueueList(dataInfo);

			delete []dataInfo->szFileName;
			delete dataInfo;

			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)pByte,dwLen);
			}

			delete []pByte;

			ullCompleteSize += dwLen;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

			m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
			m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);
			m_MasterPort->AppendCompleteSize(dwLen);
			
		}while (bResult && !*m_lpCancel && ullCompleteSize < ullFileSize && m_MasterPort->GetPortState() == PortState_Active);
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetPortName(),m_MasterPort->GetRealSpeed(),dwErrorCode);
	}

	// 先设置为停止状态
	if (bResult)
	{
		m_MasterPort->SetPortState(PortState_Stop);
	}
	else
	{
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}

	m_MasterPort->SetEndTime(CTime::GetCurrentTime());
	m_MasterPort->SetResult(bResult);

	if (bResult)
	{
		m_MasterPort->SetPortState(PortState_Pass);
		if (m_bComputeHash)
		{
			m_MasterPort->SetHash(m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());

			for (int i = 0; i < m_pMasterHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),m_pMasterHashMethod->digest()[i]);
				m_strMasterHash += strHash;
			}

			CString strHashMethod(m_pMasterHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - %s,HashValue=%s")
				,m_MasterPort->GetPortName(),strHashMethod,m_strMasterHash);

		}
	}
	else
	{
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CWpdDevice::WriteWpdFiles( CPort* port,CDataQueue *pDataQueue,CMapStringToString *pMap )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	port->Active();

	CComPtr<IPortableDevice> pIPortableDevice;
	HRESULT hr = OpenDevice(&pIPortableDevice,port->GetDevicePath());

	if (FAILED(hr))
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_MTP_OpenDevice_Failed);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP copy,custom errorcode=0x%X,Open MTP device failed,hr=0x%lx")
			,port->GetPortName(),CustomError_MTP_OpenDevice_Failed,hr);

		return FALSE;
	}

	// 获取Storage ID
	LPWSTR *ppwszObjectIDs = NULL;
	CString strStorageID;
	DWORD dwObjects = EnumStorageIDs(pIPortableDevice,&ppwszObjectIDs);

	if (dwObjects == 0)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_MTP_EnumStorageID_Failed);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP copy,custom errorcode=0x%X,EnumStorageIDs failed")
			,port->GetPortName(),CustomError_MTP_EnumStorageID_Failed);

		return FALSE;
	}

	strStorageID = CStringW(ppwszObjectIDs[0]);

	for (DWORD index = 0; index < dwObjects;index++)
	{
		delete []ppwszObjectIDs[index];
		ppwszObjectIDs[index] = NULL;
	}

	delete []ppwszObjectIDs;
	ppwszObjectIDs = NULL;
	
	// 清空DEVICE
	CleanupDeviceStorage(pIPortableDevice,strStorageID);

	//判断有效扇区是否超过
	ULONGLONG ullTotalSize = 0,ullFreeSize = 0;
	GetStorageFreeAndTotalCapacity(pIPortableDevice,strStorageID,&ullFreeSize,&ullTotalSize);
	
	if (m_ullValidSize > ullFreeSize)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP Copy,validsize=%I64d,freesize=%I64d,custom errorcode=0x%X,target small")
			,port->GetPortName(),m_ullValidSize,ullFreeSize,CustomError_Target_Small);

		return FALSE;
	}

	// 创建文件夹
	POSITION pos = m_MasterFolderObjectsList.GetHeadPosition();
	CString strParentID,strNewID;

	while (pos)
	{
		PObjectProperties pObject = m_MasterFolderObjectsList.GetNext(pos);

		if (pObject)
		{
			if (pObject->level == 2)
			{
				pMap->SetAt(pObject->pwszObjectParentID,strStorageID);
			}

			if (pMap->Lookup(pObject->pwszObjectParentID,strParentID))
			{
				hr = CreateFolder(pIPortableDevice,strParentID,pObject->pwszObjectOringalFileName,strNewID.GetBuffer(MAX_PATH));
				strNewID.ReleaseBuffer();

				if (SUCCEEDED(hr))
				{
					pMap->SetAt(pObject->pwszObjectID,strNewID);
				}
				else
				{
					port->SetEndTime(CTime::GetCurrentTime());
					port->SetResult(FALSE);
					port->SetErrorCode(ErrorType_Custom,CustomError_MTP_CreateFolder_Failed);
					port->SetPortState(PortState_Fail);
					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP Copy,custom errorcode=0x%X,create folder failed,hr = 0x%lx")
						,port->GetPortName(),CustomError_MTP_CreateFolder_Failed,hr);

					return FALSE;
				}
			}
			else
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_NO_ObjectID;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Not find the map parent id(%s),file name=%s,custom errorcode=0x%X")
					,port->GetPortName(),pObject->pwszObjectParentID,pObject->pwszObjectOringalFileName,dwErrorCode);
				
				return FALSE;
			}
			
		}
	}

	CString strCurrentObjectID,strOldObjectID,strFileName;
	PObjectProperties pObjectProperty;
	CComPtr<IPortableDeviceDataStream> pDataStream = NULL;
	DWORD dwTransferDataSize = 0;
	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	// 开始拷贝文件
	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult() && bResult && !port->IsKickOff())
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active && port->GetResult() && !port->IsKickOff())
		{
			Sleep(5);
		}

		if (!m_MasterPort->GetResult())
		{
			errType = m_MasterPort->GetErrorCode(&dwErrorCode);
			bResult = FALSE;
			break;
		}

		if (!port->GetResult())
		{
			errType = port->GetErrorCode(&dwErrorCode);
			bResult = FALSE;
			break;
		}

		if (port->IsKickOff())
		{
			dwErrorCode = CustomError_Speed_Too_Slow;
			errType = ErrorType_Custom;
			bResult = FALSE;
			break;
		}

		if (*m_lpCancel)
		{
			dwErrorCode = CustomError_Cancel;
			errType = ErrorType_Custom;
			bResult = FALSE;
			break;
		}

		if (pDataQueue->GetCount() <= 0 && m_MasterPort->GetPortState() != PortState_Active)
		{
			dwErrorCode = 0;
			bResult = TRUE;
			break;
		}

		PDATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo == NULL)
		{
			continue;
		}

		strCurrentObjectID = CString(dataInfo->szFileName);

		if (strCurrentObjectID != strOldObjectID)
		{
			// 上一个文件的收尾工作
			if (pDataStream != NULL)
			{
				hr = pDataStream->Commit(0);

				if (FAILED(hr))
				{
					bResult = FALSE;
					errType = ErrorType_Custom;
					dwErrorCode = CustomError_MTP_WriteFile_Failed;
					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - DataStream commit failed,parent id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
						,port->GetPortName(),strNewID,strFileName,dwErrorCode,hr);
					break;
				}

				PWSTR pszNewlyCreatedObject = NULL;
				hr = pDataStream->GetObjectID(&pszNewlyCreatedObject);

				strNewID = CString(pszNewlyCreatedObject);

				CoTaskMemFree(pszNewlyCreatedObject);
				pszNewlyCreatedObject = NULL;

				if (FAILED(hr))
				{
					bResult = FALSE;
					errType = ErrorType_Custom;
					dwErrorCode = CustomError_MTP_WriteFile_Failed;
					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Get new object id failed,parent id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
						,port->GetPortName(),strNewID,strFileName,dwErrorCode,hr);
					break;
				}

				pMap->SetAt(strOldObjectID,strNewID);

				pDataStream = NULL;

			}

			// 新文件开始
			if (!m_MapObjectProerties.Lookup(strCurrentObjectID,pObjectProperty))
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_NO_ObjectID;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Not find the map object id(%s),custom errorcode=0x%X")
					,port->GetPortName(),strCurrentObjectID,dwErrorCode);
				break;
			}

			strFileName = CString(pObjectProperty->pwszObjectOringalFileName);
			strParentID = CString(pObjectProperty->pwszObjectParentID);

			if (!pMap->Lookup(strParentID,strNewID))
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_NO_ObjectID;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Not find the map parent id(%s),file name=%s,custom errorcode=0x%X")
					,port->GetPortName(),strParentID,strFileName,dwErrorCode);
				break;
			}

			hr = CreateDataOutStream(pIPortableDevice,strNewID,pObjectProperty,&pDataStream,&dwTransferDataSize);
			if (FAILED(hr))
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_CreateDataStream_Failed;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateDataStream failed,parent id = %s,file name=%s,custom errorcode=0x%X,hr=0x%lx")
					,port->GetPortName(),strNewID,strFileName,dwErrorCode,hr);
				break;
			}

			strOldObjectID = strCurrentObjectID;
		}

		DWORD dwDataSize = dataInfo->dwDataSize;
		DWORD dwCompleteLen = 0;
		QueryPerformanceCounter(&t1);
		do 
		{
			DWORD dwWriten = dwTransferDataSize;

			if (dwDataSize < dwTransferDataSize)
			{
				dwWriten = dwDataSize;
			}

			hr = WriteWpdFile(pDataStream,&dataInfo->pData[dwCompleteLen],dwWriten);

			if (FAILED(hr))
			{
				bResult = FALSE;
				break;
			}

			dwCompleteLen += dwWriten;
			dwDataSize -= dwWriten;

		} while (dwDataSize > 0 && bResult);

		if (!bResult)
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_WriteFile_Failed;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - WriteWpdFile failed,parent id = %s,file name=%s,custom errorcode=0x%X,hr=0x%lx")
				,port->GetPortName(),pObjectProperty->pwszObjectParentID,pObjectProperty->pwszObjectOringalFileName,dwErrorCode,hr);
			break;
		}
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		port->AppendUsedWaitTimeS(dbTimeWait);
		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendCompleteSize(dwCompleteLen);

		delete []dataInfo->pData;
		delete []dataInfo->szFileName;

		delete dataInfo;

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}

	if (!port->GetResult())
	{
		errType = port->GetErrorCode(&dwErrorCode);
		bResult = FALSE;
	}

	if (pDataStream != NULL && bResult)
	{
		hr = pDataStream->Commit(0);

		if (FAILED(hr))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_WriteFile_Failed;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - DataStream commit failed,parent id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
				,port->GetPortName(),strNewID,strFileName,dwErrorCode,hr);

			goto END;
		}

		PWSTR pszNewlyCreatedObject = NULL;
		hr = pDataStream->GetObjectID(&pszNewlyCreatedObject);

		strNewID = CString(pszNewlyCreatedObject);

		CoTaskMemFree(pszNewlyCreatedObject);
		pszNewlyCreatedObject = NULL;

		if (FAILED(hr))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_WriteFile_Failed;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Get new object id failed,parent id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
				,port->GetPortName(),strNewID,strFileName,dwErrorCode,hr);
			goto END;
		}

		pMap->SetAt(strOldObjectID,strNewID);

		pDataStream = NULL;

	}

END:
	port->SetEndTime(CTime::GetCurrentTime());
	port->SetResult(bResult);

	if (bResult)
	{
		if (m_bHashVerify)
		{
			port->SetPortState(PortState_Active);
		}
		else
		{
			port->SetPortState(PortState_Pass);
		}

	}
	else
	{
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
	
}

BOOL CWpdDevice::VerifyWpdFiles( CPort *port,CHashMethod *pHashMethod,CMapStringToString *pMap )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullCompleteSize = 0;
	DWORD dwLen = 0;

	CComPtr<IPortableDevice> pIPortableDevice;
	HRESULT hr = OpenDevice(&pIPortableDevice,port->GetDevicePath());

	if (FAILED(hr))
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_MTP_OpenDevice_Failed);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Stop MTP copy,custom errorcode=0x%X,Open MTP device failed,hr=0x%lx")
			,port->GetPortName(),CustomError_MTP_OpenDevice_Failed,hr);

		return FALSE;
	}

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->Active();

	POSITION pos = m_MasterFileObjectsList.GetHeadPosition();
	ULONGLONG ullFileSize = 0;
	CString strObjectID,strFileName;
	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active && !port->IsKickOff())
	{
		PObjectProperties pObjectProp = m_MasterFileObjectsList.GetNext(pos);

		if (pObjectProp == NULL)
		{
			continue;
		}
		strFileName = CString(pObjectProp->pwszObjectOringalFileName);

		if (!pMap->Lookup(pObjectProp->pwszObjectID,strObjectID))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_NO_ObjectID;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - No find the map object id(%s), file name = %s,custom errorcode=0x%X")
				,port->GetPortName(),pObjectProp->pwszObjectID,strFileName,dwErrorCode);
			break;
		}

		CComPtr<IStream> pDataStream;
		DWORD dwTransferSize = 0;
		hr = CreateDataInStream(pIPortableDevice,strObjectID,&pDataStream,&dwTransferSize);

		if (FAILED(hr))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_MTP_CreateDataStream_Failed;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateDataInStream failed,object id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
				,port->GetPortName(),strObjectID,strFileName,dwErrorCode,hr);
			break;
		}

		ullFileSize = pObjectProp->ullObjectSize;
		ullCompleteSize = 0;
		do
		{
			QueryPerformanceCounter(&t0);

			dwLen = dwTransferSize;
			if (ullFileSize - ullCompleteSize < dwLen)
			{
				dwLen = (DWORD)(ullFileSize - ullCompleteSize);
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);

			hr = ReadWpdFile(pDataStream,pByte,dwLen);

			if (FAILED(hr))
			{
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_MTP_ReadFile_Failed;
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - ReadWpdFile failed,object id = %s, file name = %s,custom errorcode=0x%X,hr=0x%lx")
					,port->GetPortName(),strObjectID,strFileName,dwErrorCode,hr);
				break;
			}


			pHashMethod->update((void *)pByte,dwLen);

			delete []pByte;

			ullCompleteSize += dwLen;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

			port->AppendUsedWaitTimeS(dbTimeWait);
			port->AppendUsedNoWaitTimeS(dbTimeNoWait);
			port->AppendCompleteSize(dwLen);

		}while (bResult && !*m_lpCancel && ullCompleteSize < ullFileSize 
			&& port->GetPortState() == PortState_Active && !port->IsKickOff());
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	if (bResult)
	{
		port->SetHash(pHashMethod->digest(),pHashMethod->getHashLength());

		// 比较HASH值
		CString strVerifyHash;
		for (int i = 0; i < pHashMethod->getHashLength();i++)
		{
			CString strHash;
			strHash.Format(_T("%02X"),pHashMethod->digest()[i]);
			strVerifyHash += strHash;
		}

		CString strHashMethod(pHashMethod->getHashMetod());
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - %s,HashValue=%s")
			,port->GetPortName(),strHashMethod,strVerifyHash);

		if (0 == m_strMasterHash.CompareNoCase(strVerifyHash))
		{
			bResult = TRUE;
			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(TRUE);
			port->SetPortState(PortState_Pass);
		}
		else
		{
			bResult = FALSE;
			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(FALSE);
			port->SetPortState(PortState_Fail);
			port->SetErrorCode(ErrorType_Custom,CustomError_Compare_Failed);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - Verify FAIL,Target=%s,Master=%s")
				,port->GetPortName(),strVerifyHash,m_strMasterHash);
		}

	}
	else
	{
		bResult = FALSE;
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

HRESULT CWpdDevice::CleanupDeviceStorage( IPortableDevice *pDevice,LPCWSTR lpszObjectID )
{
	HRESULT                               hr = S_OK;
	CComPtr<IPortableDeviceContent>       pContent;
	CComPtr<IEnumPortableDeviceObjectIDs> pEnumObjectIDs;

	// Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	hr = pDevice->Content(&pContent);
	// Enumerate content starting from the "DEVICE" object.
	if (SUCCEEDED(hr))
	{
		// Get an IEnumPortableDeviceObjectIDs interface by calling EnumObjects with the
		// specified parent object identifier.
		hr = pContent->EnumObjects(0,               // Flags are unused
			lpszObjectID,     // Starting from the passed in object
			NULL,            // Filter is unused
			&pEnumObjectIDs);
		if (SUCCEEDED(hr))
		{
			while(hr == S_OK)
			{
				DWORD  cFetched = 0;
				PWSTR  szObjectIDArray[NUM_OBJECTS_TO_REQUEST] = {0};
				hr = pEnumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST,   // Number of objects to request on each NEXT call
					szObjectIDArray,          // Array of PWSTR array which will be populated on each NEXT call
					&cFetched);               // Number of objects written to the PWSTR array
				if (SUCCEEDED(hr))
				{
					// Traverse the results of the Next() operation and recursively enumerate
					// Remember to free all returned object identifiers using CoTaskMemFree()
					for (DWORD dwIndex = 0; dwIndex < cFetched; dwIndex++)
					{
						DeleteWpdFile(pDevice,szObjectIDArray[dwIndex],1);
						// Free allocated PWSTRs after the recursive enumeration call has completed.
						CoTaskMemFree(szObjectIDArray[dwIndex]);
						szObjectIDArray[dwIndex] = NULL;
					}
				}
			}
		}
		else
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("CleanupStorage failed, failed to get IEnumPortableDeviceObjectIDs from IPortableDeviceContent, hr = 0x%lx"),hr);
		}

	}
	else
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("CleanupStorage failed, failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr);
	}



	return hr;
}

void CWpdDevice::SetValidSize( ULONGLONG ullSize )
{
	m_MasterPort->SetValidSize(ullSize);

	POSITION pos = m_TargetPorts->GetHeadPosition();

	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected())
		{
			port->SetValidSize(ullSize);	
		}
	}
}

bool CWpdDevice::IsAllFailed( ErrorType &errType,PDWORD pdwErrorCode )
{
	bool bAllFailed = true;
	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *pPort = m_TargetPorts->GetNext(pos);
		if (pPort->IsConnected())
		{
			if (pPort->GetResult())
			{
				bAllFailed = false;
				break;
			}
			else
			{
				errType = pPort->GetErrorCode(pdwErrorCode);
			}
		}

	}

	return bAllFailed;
}

void CWpdDevice::AddDataQueueList( PDATA_INFO dataInfo )
{
	POSITION pos1 = m_DataQueueList.GetHeadPosition();
	POSITION pos2 = m_TargetPorts->GetHeadPosition();

	while (pos1 && pos2)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos1);
		CPort *port = m_TargetPorts->GetNext(pos2);

		if (port->GetResult())
		{
			PDATA_INFO data = new DATA_INFO;
			ZeroMemory(data,sizeof(DATA_INFO));

			if (dataInfo->szFileName)
			{
				data->szFileName = new TCHAR[_tcslen(dataInfo->szFileName) + 1];
				_tcscpy_s(data->szFileName,_tcslen(dataInfo->szFileName) + 1,dataInfo->szFileName);
			}

			data->dwDataSize = dataInfo->dwDataSize;
			data->dwOldSize = dataInfo->dwOldSize;
			data->ullOffset = dataInfo->ullOffset;
			data->pData = new BYTE[dataInfo->dwDataSize];
			memcpy(data->pData,dataInfo->pData,dataInfo->dwDataSize);
			dataQueue->AddTail(data);
		}
	}
}

bool CWpdDevice::IsReachLimitQty( int limit )
{
	bool bReached = false;
	POSITION pos1 = m_DataQueueList.GetHeadPosition();
	POSITION pos2 = m_TargetPorts->GetHeadPosition();
	while (pos1 && pos2)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos1);
		CPort *port = m_TargetPorts->GetNext(pos2);
		if (port->GetResult() && dataQueue->GetCount() > limit)
		{
			bReached = true;
			break;
		}
	}

	return bReached;
}

void CWpdDevice::CleanupObjectProperties()
{
	POSITION pos = m_MasterFileObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObjectProp = m_MasterFileObjectsList.GetNext(pos);

		if (pObjectProp)
		{
			if (pObjectProp->pwszObjectID)
			{
				CoTaskMemFree(pObjectProp->pwszObjectID);
				pObjectProp->pwszObjectID = NULL;
			}

			if (pObjectProp->pwszObjectName)
			{
				CoTaskMemFree(pObjectProp->pwszObjectName);
				pObjectProp->pwszObjectName = NULL;
			}

			if (pObjectProp->pwszObjectOringalFileName)
			{
				CoTaskMemFree(pObjectProp->pwszObjectOringalFileName);
				pObjectProp->pwszObjectOringalFileName = NULL;
			}

			if (pObjectProp->pwszObjectParentID)
			{
				CoTaskMemFree(pObjectProp->pwszObjectParentID);
				pObjectProp->pwszObjectParentID = NULL;
			}
			
			delete []pObjectProp;
			pObjectProp = NULL;
		}
	}
	m_MasterFileObjectsList.RemoveAll();

	pos = m_MasterFolderObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObjectProp = m_MasterFolderObjectsList.GetNext(pos);

		if (pObjectProp)
		{
			if (pObjectProp->pwszObjectID)
			{
				CoTaskMemFree(pObjectProp->pwszObjectID);
				pObjectProp->pwszObjectID = NULL;
			}

			if (pObjectProp->pwszObjectName)
			{
				CoTaskMemFree(pObjectProp->pwszObjectName);
				pObjectProp->pwszObjectName = NULL;
			}

			if (pObjectProp->pwszObjectOringalFileName)
			{
				CoTaskMemFree(pObjectProp->pwszObjectOringalFileName);
				pObjectProp->pwszObjectOringalFileName = NULL;
			}

			if (pObjectProp->pwszObjectParentID)
			{
				CoTaskMemFree(pObjectProp->pwszObjectParentID);
				pObjectProp->pwszObjectParentID = NULL;
			}

			delete []pObjectProp;
			pObjectProp = NULL;
		}
	}
	m_MasterFolderObjectsList.RemoveAll();

	// 清空Map
	pos = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos)
	{
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(pos);

		if (pMap)
		{
			pMap->RemoveAll();
		}
	}
}




