#include "StdAfx.h"
#include "USBCopy.h"
#include "WpdDevice.h"
#include "Utils.h"
#include "Checksum32.h"
#include "CRC32.h"
#include "MD5.h"
#include "zlib.h"

#ifdef _DEBUG
#pragma comment(lib,"zlib128d.lib")
#else
#pragma comment(lib,"zlib128.lib")
#endif

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

	m_bCompare = FALSE;
	m_pMasterHashMethod = NULL;

	ZeroMemory(m_ImageHash,LEN_DIGEST);

	m_bCompressComplete = TRUE;

	m_bServerFirst = FALSE;

	m_ClientSocket = INVALID_SOCKET;

	m_iCompressLevel = Z_BEST_SPEED;

	m_hImage = INVALID_HANDLE_VALUE;

	m_bDataCompress = FALSE;
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

	m_MapObjectProerties.RemoveAll();

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

	if (m_hImage != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hImage);
		m_hImage = INVALID_HANDLE_VALUE;
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
		}

		CDataQueue *dataQueue = new CDataQueue();
		m_DataQueueList.AddTail(dataQueue);

		CMapStringToString *pMap = new CMapStringToString();
		m_TargetMapStringFileObjectsList.AddTail(pMap);

	}
}

void CWpdDevice::SetHashMethod( BOOL bComputeHash,HashMethod hashMethod )
{
	m_bComputeHash = bComputeHash;
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

void CWpdDevice::SetSocket( SOCKET sClient,BOOL bServerFirst)
{
	m_ClientSocket = sClient;
	m_bServerFirst = bServerFirst;
}

void CWpdDevice::SetMakeImageParm(BOOL bCompress,int compressLevel)
{
	m_iCompressLevel = compressLevel;
	m_bDataCompress = bCompress;
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

	case WorkMode_ImageMake:
		bRet = OnMakeImage();
		break;

	case WorkMode_ImageCopy:
		bRet = OnCopyImage();
		break;
	}

	return bRet;
}

BOOL CWpdDevice::OnCopyWpdFiles()
{
	// 清理上一次的结果
	m_ullValidSize = 0;
	m_MapObjectProerties.RemoveAll();
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
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	POSITION posMap = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(posMap);
		if (port->IsConnected() && port->GetResult())
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

	if (bResult && m_bCompare)
	{
		char function[1024] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,WPARAM(function),0);

		nCurrentTargetCount = GetCurrentTargetCount();

		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];


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

BOOL CWpdDevice::OnMakeImage()
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
	DWORD dwReadId,dwWriteId,dwCompressId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadWpdFilesThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Copy(Master) - CreateReadWpdFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	HANDLE hCompressThread = NULL;

	if (m_bDataCompress)
	{
		hCompressThread = CreateThread(NULL,0,CompressThreadProc,this,0,&dwCompressId);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress Image - CreateCompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,dwCompressId,hCompressThread);
	}

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	POSITION posMap = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(posMap);
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;
			lpVoid->lpVoid4 = pMap;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteImageThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s - CreateWriteWpdImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
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

	if (m_bDataCompress)
	{
		//等待压缩线程结束
		WaitForSingleObject(hCompressThread,INFINITE);
		CloseHandle(hCompressThread);
	}

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	return bResult;
}

BOOL CWpdDevice::OnCopyImage()
{
	// 清理上一次的结果
	m_ullValidSize = 0;
	m_MapObjectProerties.RemoveAll();
	CleanupObjectProperties();

	IMAGE_HEADER imgHead = {0};
	DWORD dwErrorCode = 0;
	DWORD dwLen = SIZEOF_IMAGE_HEADER;

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwUncompressId;

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic start......"));

	if (m_bServerFirst)
	{

		//创建本地Image
		CString strTempName,strImageFile;
		strImageFile = m_MasterPort->GetFileName();

		strTempName.Format(_T("%s.$$$"),strImageFile.Left(strImageFile.GetLength() - 4));

		m_hImage = GetHandleOnFile(strTempName,
			CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

		if (m_hImage == INVALID_HANDLE_VALUE)
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create mtp image file error,filename=%s,system errorcode=%ld,%s")
				,strTempName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		}

		USES_CONVERSION;
		CString strImageName = CUtils::GetFileName(strImageFile);
		char *fileName = W2A(strImageName);

		dwLen = sizeof(CMD_IN) + strlen(fileName) + 2;

		BYTE *bufSend = new BYTE[dwLen];
		ZeroMemory(bufSend,dwLen);
		bufSend[dwLen - 1] = END_FLAG;

		CMD_IN queryImageIn = {0};
		queryImageIn.dwCmdIn = CMD_QUERY_IMAGE_IN;
		queryImageIn.dwSizeSend = dwLen;

		memcpy(bufSend,&queryImageIn,sizeof(CMD_IN));
		memcpy(bufSend + sizeof(CMD_IN),fileName,strlen(fileName));

		if (!Send(m_ClientSocket,(char *)bufSend,dwLen,NULL,&dwErrorCode))
		{
			delete []bufSend;

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query mtp image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));


			return FALSE;
		}

		delete []bufSend;

		// 服务端会把文件头和文件列表一起发过来
		CMD_OUT queryImageOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&queryImageOut,dwLen,NULL,&dwErrorCode))
		{
			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query mtp image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			
			return FALSE;
		}

		dwLen = queryImageOut.dwSizeSend - sizeof(CMD_OUT);

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		DWORD dwRead = 0;

		while(dwRead < dwLen)
		{
			DWORD dwByteRead = dwLen - dwRead;
			if (!Recv(m_ClientSocket,(char *)(pByte+dwRead),dwByteRead,NULL,&dwErrorCode))
			{
				delete []pByte;

				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query mtp image from server error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				return FALSE;
			}

			dwRead += dwByteRead;
		}

		if (dwRead < dwLen)
		{
			bResult = FALSE;

			delete []pByte;

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query mtp image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		if (queryImageOut.dwErrorCode != 0)
		{
			delete []pByte;

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(queryImageOut.errType,queryImageOut.dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query mtp image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		if (queryImageOut.dwCmdOut != CMD_QUERY_IMAGE_OUT || queryImageOut.dwSizeSend != dwLen + sizeof(CMD_OUT))
		{
			delete []pByte;

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Get_Data_From_Server_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query mtp image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		memcpy(&imgHead,pByte,SIZEOF_IMAGE_HEADER);

		if (m_hImage != INVALID_HANDLE_VALUE)
		{
			WriteFileAsyn(m_hImage,0,dwLen,pByte,m_MasterPort->GetOverlapped(FALSE),&dwErrorCode);
		}

		// 读文件夹列表和文件列表
		PBYTE pTemp = pByte;
		pTemp += SIZEOF_IMAGE_HEADER;

		DWORD dwIndex,dwLength,dwCount;

		for (int type = 0; type < 2;type++)
		{
			if (type == 0)
			{
				// 文件夹
				dwCount = imgHead.dwFolderCount;
			}
			else
			{
				//文件
				dwCount = imgHead.dwFileCount;
			}

			for (DWORD i = 0; i < dwCount;i++)
			{

				ULONGLONG ullValue = *(ULONGLONG *)(pTemp+1);

				dwIndex = LODWORD(ullValue);
				dwLength = HIDWORD(ullValue);

				// 判断IMAGE格式是否正确
				if (dwIndex != i || pTemp[dwLength - 1] != END_FLAG || *pTemp != type)
				{
					delete []pByte;

					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Format_Error);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP image file list format error,filename=%s,system errorcode=%ld,%s")
						,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					return FALSE;
				}

				// 解析Object Properties
				size_t len = 0;
				PObjectProperties pObject = new ObjectProperties;
				ZeroMemory(pObject,sizeof(ObjectProperties));

				pTemp += sizeof(ULONGLONG) + 1;

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectID = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectID,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectParentID = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectParentID,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectName = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectName,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectOringalFileName = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectOringalFileName,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = sizeof(GUID);
				memcpy(&pObject->guidObjectContentType,pTemp,len);
				pTemp += len;

				len = sizeof(GUID);
				memcpy(&pObject->guidObjectFormat,pTemp,len);
				pTemp += len;

				len = sizeof(ULONGLONG);
				memcpy(&pObject->ullObjectSize,pTemp,len);
				pTemp += len;

				len = sizeof(int);
				memcpy(&pObject->level,pTemp,len);
				pTemp += len;

				// 再一次判断格式是否正确
				if (*pTemp != END_FLAG)
				{
					delete []pByte;

					CoTaskMemFree(pObject->pwszObjectID);
					CoTaskMemFree(pObject->pwszObjectParentID);
					CoTaskMemFree(pObject->pwszObjectName);
					CoTaskMemFree(pObject->pwszObjectOringalFileName);

					delete pObject;

					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Format_Error);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP image file list format error,filename=%s,system errorcode=%ld,%s")
						,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					return FALSE;
				}

				m_MasterFolderObjectsList.AddTail(pObject);

				pTemp += 1;
			}
		}

		delete []pByte;
		
	}
	else
	{
		m_hImage = GetHandleOnFile(m_MasterPort->GetFileName(),
			OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);

		if (m_hImage == INVALID_HANDLE_VALUE)
		{
			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Open mtp image file error,filename=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		if (!ReadFileAsyn(m_hImage,0,dwLen,(LPBYTE)&imgHead,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read Image Head error,filename=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		dwLen = imgHead.dwListSize;
		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		if (!ReadFileAsyn(m_hImage,SIZEOF_IMAGE_HEADER,dwLen,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			delete []pByte;

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read MTP Image file list error,filename=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}

		// 读文件夹列表和文件列表
		PBYTE pTemp = pByte;

		DWORD dwIndex,dwLength,dwCount;

		for (int type = 0; type < 2;type++)
		{
			if (type == 0)
			{
				// 文件夹
				dwCount = imgHead.dwFolderCount;
			}
			else
			{
				//文件
				dwCount = imgHead.dwFileCount;
			}

			for (DWORD i = 0; i < dwCount;i++)
			{

				ULONGLONG ullValue = *(ULONGLONG *)(pTemp+1);

				dwIndex = LODWORD(ullValue);
				dwLength = HIDWORD(ullValue);

				// 判断IMAGE格式是否正确
				if (dwIndex != i || pTemp[dwLength - 1] != END_FLAG || *pTemp != type)
				{
					delete []pByte;

					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Format_Error);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP image file list format error,filename=%s,system errorcode=%ld,%s")
						,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					return FALSE;
				}

				// 解析Object Properties
				size_t len = 0;
				PObjectProperties pObject = new ObjectProperties;
				ZeroMemory(pObject,sizeof(ObjectProperties));

				pTemp += sizeof(ULONGLONG) + 1;

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectID = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectID,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectParentID = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectParentID,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectName = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectName,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = wcslen((WCHAR *)pTemp) + 1;
				pObject->pwszObjectOringalFileName = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
				wcscpy_s(pObject->pwszObjectOringalFileName,len,(WCHAR *)pTemp);
				pTemp += len * sizeof(WCHAR);

				len = sizeof(GUID);
				memcpy(&pObject->guidObjectContentType,pTemp,len);
				pTemp += len;

				len = sizeof(GUID);
				memcpy(&pObject->guidObjectFormat,pTemp,len);
				pTemp += len;

				len = sizeof(ULONGLONG);
				memcpy(&pObject->ullObjectSize,pTemp,len);
				pTemp += len;

				len = sizeof(int);
				memcpy(&pObject->level,pTemp,len);
				pTemp += len;

				// 再一次判断格式是否正确
				if (*pTemp != END_FLAG)
				{
					delete []pByte;

					CoTaskMemFree(pObject->pwszObjectID);
					CoTaskMemFree(pObject->pwszObjectParentID);
					CoTaskMemFree(pObject->pwszObjectName);
					CoTaskMemFree(pObject->pwszObjectOringalFileName);

					delete pObject;

					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Format_Error);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP image file list format error,filename=%s,system errorcode=%ld,%s")
						,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					return FALSE;
				}

				if (pObject->guidObjectContentType == WPD_CONTENT_TYPE_FOLDER)
				{
					m_MasterFolderObjectsList.AddTail(pObject);
				}
				else
				{
					m_MapObjectProerties.SetAt(pObject->pwszObjectID,pObject);
					m_MasterFileObjectsList.AddTail(pObject);
					m_ullValidSize += pObject->ullObjectSize;
				}
				

				pTemp += 1;
			}
		}

		delete []pByte;
	}

	m_ullValidSize = imgHead.ullValidSize;
	m_ullImageSize = imgHead.ullImageSize;
	m_dwPkgOffset = imgHead.dwPkgOffset;
	m_bDataCompress = (imgHead.byUnCompress == 0) ? TRUE : FALSE;

	HashMethod hashMethod = (HashMethod)imgHead.dwHashType;

	memcpy(m_ImageHash,imgHead.byImageDigest,imgHead.dwHashLen);

	SetValidSize(m_ullValidSize);
	SetHashMethod(m_bComputeHash,hashMethod);

	m_MasterPort->SetHashMethod(hashMethod);
	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);

		if (port->IsConnected())
		{
			port->SetHashMethod(hashMethod);
		}
	}

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic end, total files count=%d, total size=%I64d")
		,m_MasterFileObjectsList.GetCount(),m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Files List:"));
	CString strList;
	pos = m_MasterFileObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObjectProp = m_MasterFileObjectsList.GetNext(pos);

		if (pObjectProp)
		{
			strList.Format(_T("ObjectID:%s\tFileName:%s\tFileSize:%I64d"),pObjectProp->pwszObjectID,pObjectProp->pwszObjectOringalFileName,pObjectProp->ullObjectSize);
			CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
		}

	}

	HANDLE hReadThread = CreateThread(NULL,0,ReadImageThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image(%s) - CreateReadImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetFileName(),dwReadId,hReadThread);

	// 创建多个解压缩线程
	HANDLE hUncompressThread = NULL;

	if (m_bDataCompress)
	{
		hUncompressThread = CreateThread(NULL,0,UnCompressThreadProc,this,0,&dwUncompressId);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image - CreateUncompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,dwUncompressId,hUncompressThread);

	}

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	POSITION posMap = m_TargetMapStringFileObjectsList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		CMapStringToString *pMap = m_TargetMapStringFileObjectsList.GetNext(posMap);
		if (port->IsConnected() && port->GetResult())
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

	// 等待写磁盘线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	if (m_bDataCompress)
	{
		// 等待解压缩线程结束
		WaitForSingleObject(hUncompressThread,INFINITE);
		CloseHandle(hUncompressThread);
	}

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bCompare)
	{
		char function[1024] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,WPARAM(function),0);

		nCurrentTargetCount = GetCurrentTargetCount();

		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];


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

DWORD WINAPI CWpdDevice::ReadImageThreadProc(LPVOID lpParm)
{
	CWpdDevice *wpd = (CWpdDevice *)lpParm;

	BOOL bRet = wpd->ReadImage();

	return bRet;
}

DWORD WINAPI CWpdDevice::WriteImageThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;

	CWpdDevice *wpd = (CWpdDevice *)parm->lpVoid1;
	CPort *port = (CPort*)parm->lpVoid2;
	CDataQueue *pDataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bRet = wpd->WriteImage(port,pDataQueue);

	return bRet;
}

DWORD WINAPI CWpdDevice::CompressThreadProc(LPVOID lpParm)
{
	CWpdDevice *wpd = (CWpdDevice *)lpParm;

	BOOL bRet = wpd->Compress();

	return bRet;
}

DWORD WINAPI CWpdDevice::UnCompressThreadProc(LPVOID lpParm)
{
	CWpdDevice *wpd = (CWpdDevice *)lpParm;

	BOOL bRet = wpd->Uncompress();

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
	LARGE_INTEGER freq,t0,t1,t2,t3;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	// 等待其他线程创建好,最多等5次
	Sleep(100);

	int nTimes = 5;
	while (!IsTargetsReady() && nTimes > 0)
	{
		Sleep(100);
		nTimes--;
	}

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
				SwitchToThread();
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

			QueryPerformanceCounter(&t2);

			PDATA_INFO dataInfo = new DATA_INFO;
			ZeroMemory(dataInfo,sizeof(DATA_INFO));
			dataInfo->szFileName = new TCHAR[strObjectID.GetLength() + 1];
			_tcscpy_s(dataInfo->szFileName,strObjectID.GetLength()+1,strObjectID);
			dataInfo->ullOffset = ullCompleteSize;
			dataInfo->dwDataSize = dwLen;

			dataInfo->pData = new BYTE[dwLen];
			memcpy(dataInfo->pData,pByte,dwLen);

			if (m_WorkMode == WorkMode_ImageMake && m_bDataCompress)
			{
				m_CompressQueue.AddTail(dataInfo);
			}
			else if (m_WorkMode == WorkMode_ImageMake)
			{
				// 不压缩，加上文件头和文件尾
				PDATA_INFO compressData = new DATA_INFO;
				ZeroMemory(compressData,sizeof(DATA_INFO));

				compressData->dwDataSize = dataInfo->dwDataSize + sizeof(ULONGLONG) + sizeof(DWORD) + 1;
				compressData->dwOldSize = dataInfo->dwDataSize;
				compressData->pData = new BYTE[compressData->dwDataSize];
				ZeroMemory(compressData->pData,compressData->dwDataSize);

				compressData->pData[compressData->dwDataSize - 1] = 0xED; //结束标志

				memcpy(compressData->pData,&dataInfo->ullOffset,sizeof(ULONGLONG));
				memcpy(compressData->pData + sizeof(ULONGLONG),&compressData->dwDataSize,sizeof(DWORD));
				memcpy(compressData->pData + sizeof(ULONGLONG) + sizeof(DWORD),pByte,dwLen);

				AddDataQueueList(compressData);

				delete []compressData->pData;
				delete []compressData;

				delete []dataInfo->pData;
				delete dataInfo;
			}
			else
			{
				AddDataQueueList(dataInfo);

				delete []dataInfo->szFileName;
				delete []dataInfo->pData;
				delete dataInfo;
			}


			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)pByte,dwLen);
			}

			delete []pByte;

			ullCompleteSize += dwLen;

			QueryPerformanceCounter(&t3);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
			dbTimeWait = (double)(t3.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

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

	// 所有数据都拷贝完
	while (!m_bCompressComplete)
	{
		SwitchToThread();
		Sleep(100);
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
			SwitchToThread();
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
		if (m_bCompare)
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

BOOL CWpdDevice::WriteImage( CPort* port,CDataQueue *pDataQueue )
{
	if (m_bServerFirst)
	{
		return WriteRemoteImage(port,pDataQueue);
	}
	else
	{
		return WriteLocalImage(port,pDataQueue);
	}
}

BOOL CWpdDevice::WriteLocalImage( CPort *port,CDataQueue *pDataQueue )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;
	DWORD dwPkgOffset = 0;
	DWORD dwListSize = 0;

	port->Active();

	HANDLE hFile = GetHandleOnFile(port->GetFileName(),CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_System,dwErrorCode);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create mtp image file error,filename=%s,system errorcode=%ld,%s")
			,port->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	// 先写Folder List 和 File List
	//0/1, dwIndex,dwLength,PObjectProperties,0xED
	// 0-表示文件，1-表示文件夹
	POSITION pos = m_MasterFolderObjectsList.GetHeadPosition();
	DWORD dwIndex = 0;
	while (pos)
	{
		PObjectProperties pObject = m_MasterFolderObjectsList.GetNext(pos);

		if (pObject)
		{
			//计算所需要的长度
			size_t len = 0;
			dwLen = sizeof(DWORD) * 2 + 2;
			dwLen += (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			dwLen += sizeof(GUID);
			dwLen += sizeof(GUID);
			dwLen += sizeof(ULONGLONG);
			dwLen += sizeof(int);

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);
			pByte[0] = 0;
			pByte[dwLen - 1] = 0xED;

			PBYTE pTemp = pByte;
			pTemp++;

			memcpy(pTemp,&dwIndex,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			memcpy(pTemp,&dwLen,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			len = (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectParentID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectName,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectOringalFileName,len);
			pTemp += len;

			memcpy(pTemp,&pObject->guidObjectContentType,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->guidObjectFormat,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->ullObjectSize,sizeof(ULONGLONG));
			pTemp += sizeof(ULONGLONG);

			memcpy(pTemp,&pObject->level,sizeof(int));
			pTemp += sizeof(int);

			if (!WriteFileAsyn(hFile,ullOffset,dwLen,pByte,port->GetOverlapped(FALSE),&dwErrorCode))
			{

				CloseHandle(hFile);

				bResult = FALSE;

				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image folder list error,filename=%s,system errorcode=%ld,%s")
					,port->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				port->SetEndTime(CTime::GetCurrentTime());
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_System,dwErrorCode);
				port->SetPortState(PortState_Fail);

				return FALSE;
			}

			dwIndex++;
			ullOffset += dwLen;
			dwListSize += dwLen;

			delete []pByte;
		}
	}

	dwIndex = 0;
	pos = m_MasterFileObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObject = m_MasterFileObjectsList.GetNext(pos);

		if (pObject)
		{
			//计算所需要的长度
			size_t len = 0;
			dwLen = sizeof(DWORD) * 2 + 2;
			dwLen += (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			dwLen += sizeof(GUID);
			dwLen += sizeof(GUID);
			dwLen += sizeof(ULONGLONG);
			dwLen += sizeof(int);

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);
			pByte[0] = 1;
			pByte[dwLen - 1] = 0xED;

			PBYTE pTemp = pByte;
			pTemp++;

			memcpy(pTemp,&dwIndex,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			memcpy(pTemp,&dwLen,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			len = (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectParentID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectName,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectOringalFileName,len);
			pTemp += len;

			memcpy(pTemp,&pObject->guidObjectContentType,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->guidObjectFormat,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->ullObjectSize,sizeof(ULONGLONG));
			pTemp += sizeof(ULONGLONG);

			memcpy(pTemp,&pObject->level,sizeof(int));
			pTemp += sizeof(int);

			if (!WriteFileAsyn(hFile,ullOffset,dwLen,pByte,port->GetOverlapped(FALSE),&dwErrorCode))
			{

				CloseHandle(hFile);

				bResult = FALSE;

				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image file list error,filename=%s,system errorcode=%ld,%s")
					,port->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				port->SetEndTime(CTime::GetCurrentTime());
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_System,dwErrorCode);
				port->SetPortState(PortState_Fail);

				return FALSE;
			}

			dwIndex++;
			ullOffset += dwLen;
			dwListSize += dwLen;

			delete []pByte;
		}
	}

	//offset 取扇区的整数倍
	ullOffset = (ullOffset + 511) / 512 * 512;
	dwPkgOffset = (DWORD)ullOffset;

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult())
	{
		//dwTimeWait = timeGetTime();
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <=0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& (m_MasterPort->GetPortState() == PortState_Active || !m_bCompressComplete)
			&& port->GetResult())
		{
			SwitchToThread();
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

		if (*m_lpCancel)
		{
			dwErrorCode = CustomError_Cancel;
			errType = ErrorType_Custom;
			bResult = FALSE;
			break;
		}

		if (pDataQueue->GetCount() <= 0 && m_MasterPort->GetPortState() != PortState_Active && m_bCompressComplete)
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

		if (m_bDataCompress)
		{
			*(PULONGLONG)dataInfo->pData = ullPkgIndex;
		}
		
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = dataInfo->dwDataSize;

		if (!WriteFileAsyn(hFile,ullOffset,dwLen,dataInfo->pData,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetFileName(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		//DWORD dwTime = timeGetTime();
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		ullOffset += dwLen;

		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedWaitTimeS(dbTimeNoWait);

		// 压缩的数据比实际数据短，不能取压缩后的长度，要不压缩之前的长度
		port->AppendCompleteSize(dataInfo->dwOldSize);

		delete []dataInfo->pData;
		delete dataInfo;

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetFileName(),port->GetRealSpeed(),dwErrorCode);
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

	// 写IMAGE头
	if (bResult)
	{
		LARGE_INTEGER liSize = {0};
		GetFileSizeEx(hFile,&liSize);
		// 随机生成一个数作为IMAGEID

		IMAGE_HEADER imgHead;
		ZeroMemory(&imgHead,sizeof(IMAGE_HEADER));
		memcpy(imgHead.szImageFlag,IMAGE_FLAG,strlen(IMAGE_FLAG));
		imgHead.ullImageSize = liSize.QuadPart;
		memcpy(imgHead.szAppVersion,APP_VERSION,strlen(APP_VERSION));

		imgHead.dwImageType = MTP_IMAGE;

		imgHead.dwMaxSizeOfPackage = MAX_COMPRESS_BUF;
		imgHead.ullCapacitySize = m_MasterPort->GetTotalSize();
		imgHead.dwBytesPerSector = m_MasterPort->GetBytesPerSector();
		memcpy(imgHead.szZipVer,ZIP_VERSION,strlen(ZIP_VERSION));
		imgHead.byUnCompress = m_bDataCompress ? 0 : 1;

		imgHead.ullPkgCount = ullPkgIndex;
		imgHead.ullValidSize = m_MasterPort->GetValidSize();
		imgHead.dwHashLen = m_pMasterHashMethod->getHashLength();
		imgHead.dwHashType = m_MasterPort->GetHashMethod();
		memcpy(imgHead.byImageDigest,m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());

		imgHead.dwPkgOffset = dwPkgOffset;
		imgHead.dwFolderCount = m_MasterFolderObjectsList.GetCount();
		imgHead.dwFileCount = m_MasterFileObjectsList.GetCount();
		imgHead.dwListSize = dwListSize;

		imgHead.byEnd = END_FLAG;

		dwLen = SIZEOF_IMAGE_HEADER;

		if (!WriteFileAsyn(hFile,0,dwLen,(LPBYTE)&imgHead,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image head error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetFileName(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		}
	}

	CloseHandle(hFile);

	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		port->SetPortState(PortState_Pass);
	}
	else
	{
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CWpdDevice::WriteRemoteImage( CPort *port,CDataQueue *pDataQueue )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwSendLen = 0;
	DWORD dwPkgOffset = 0;
	DWORD dwListSize = 0;

	port->Active();

	CString strImageName = CUtils::GetFileName(port->GetFileName());

	USES_CONVERSION;
	char *fileName = W2A(strImageName);

	CMD_IN makeImageIn = {0};
	makeImageIn.dwCmdIn = CMD_MAKE_IMAGE_IN;
	makeImageIn.byStop = FALSE;

	WSAOVERLAPPED olSend = {0};
	olSend.hEvent = WSACreateEvent();

	WSAOVERLAPPED olRecv = {0};
	olRecv.hEvent = WSACreateEvent();

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	// 先写Folder List 和 File List
	//0/1, dwIndex,dwLength,PObjectProperties,0xED
	// 0-表示文件，1-表示文件夹
	POSITION pos = m_MasterFolderObjectsList.GetHeadPosition();
	DWORD dwIndex = 0;
	while (pos)
	{
		PObjectProperties pObject = m_MasterFolderObjectsList.GetNext(pos);

		if (pObject)
		{
			//计算所需要的长度
			size_t len = 0;
			DWORD dwLen = 0;
			dwLen = sizeof(DWORD) * 2 + 2;
			dwLen += (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			dwLen += sizeof(GUID);
			dwLen += sizeof(GUID);
			dwLen += sizeof(ULONGLONG);
			dwLen += sizeof(int);

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);
			pByte[0] = 0;
			pByte[dwLen - 1] = 0xED;

			PBYTE pTemp = pByte;
			pTemp++;

			memcpy(pTemp,&dwIndex,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			memcpy(pTemp,&dwLen,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			len = (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectParentID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectName,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectOringalFileName,len);
			pTemp += len;

			memcpy(pTemp,&pObject->guidObjectContentType,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->guidObjectFormat,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->ullObjectSize,sizeof(ULONGLONG));
			pTemp += sizeof(ULONGLONG);

			memcpy(pTemp,&pObject->level,sizeof(int));
			pTemp += sizeof(int);

			// 发送到服务器
			dwSendLen = sizeof(CMD_IN) + strlen(fileName) + 2 + dwLen;

			makeImageIn.dwSizeSend = dwSendLen;

			BYTE *pSendByte = new BYTE[dwSendLen];
			ZeroMemory(pSendByte,dwSendLen);
			memcpy(pSendByte,&makeImageIn,sizeof(CMD_IN));
			memcpy(pSendByte+sizeof(CMD_IN),fileName,strlen(fileName));
			memcpy(pSendByte+sizeof(CMD_IN)+strlen(fileName)+1,pByte,dwLen);

			if (!Send(m_ClientSocket,(char *)pSendByte,dwSendLen,&olSend,&dwErrorCode))
			{
				delete []pSendByte;
				delete []pByte;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write mtp image folder list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			delete []pSendByte;
			delete []pByte;

			// 读返回值
			CMD_OUT makeImageOut = {0};
			dwSendLen = sizeof(CMD_OUT);
			if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwSendLen,&olRecv,&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image folder list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			if (makeImageOut.dwErrorCode != 0)
			{
				dwErrorCode = makeImageOut.dwErrorCode;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image folder list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwSendLen)
			{
				dwErrorCode = CustomError_Get_Data_From_Server_Error;
				errType = ErrorType_Custom;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image folder list error,filename=%s,custom errorcode=0x%X,get data from server error")
					,strImageName,dwErrorCode);

				goto END;
			}

			dwIndex++;
			ullOffset += dwLen;
			dwListSize += dwLen;

		}
	}

	dwIndex = 0;
	pos = m_MasterFileObjectsList.GetHeadPosition();
	while (pos)
	{
		PObjectProperties pObject = m_MasterFileObjectsList.GetNext(pos);

		if (pObject)
		{
			//计算所需要的长度
			size_t len = 0;
			DWORD dwLen = 0;
			dwLen = sizeof(DWORD) * 2 + 2;
			dwLen += (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			dwLen += (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			dwLen += sizeof(GUID);
			dwLen += sizeof(GUID);
			dwLen += sizeof(ULONGLONG);
			dwLen += sizeof(int);

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);
			pByte[0] = 1;
			pByte[dwLen - 1] = 0xED;

			PBYTE pTemp = pByte;
			pTemp++;

			memcpy(pTemp,&dwIndex,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			memcpy(pTemp,&dwLen,sizeof(DWORD));
			pTemp += sizeof(DWORD);

			len = (wcslen(pObject->pwszObjectID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectParentID) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectParentID,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectName,len);
			pTemp += len;

			len = (wcslen(pObject->pwszObjectOringalFileName) + 1) * sizeof(WCHAR);
			memcpy(pTemp,pObject->pwszObjectOringalFileName,len);
			pTemp += len;

			memcpy(pTemp,&pObject->guidObjectContentType,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->guidObjectFormat,sizeof(GUID));
			pTemp += sizeof(GUID);

			memcpy(pTemp,&pObject->ullObjectSize,sizeof(ULONGLONG));
			pTemp += sizeof(ULONGLONG);

			memcpy(pTemp,&pObject->level,sizeof(int));
			pTemp += sizeof(int);

			// 发送到服务器
			dwSendLen = sizeof(CMD_IN) + strlen(fileName) + 2 + dwLen;

			makeImageIn.dwSizeSend = dwSendLen;

			BYTE *pSendByte = new BYTE[dwSendLen];
			ZeroMemory(pSendByte,dwSendLen);
			memcpy(pSendByte,&makeImageIn,sizeof(CMD_IN));
			memcpy(pSendByte+sizeof(CMD_IN),fileName,strlen(fileName));
			memcpy(pSendByte+sizeof(CMD_IN)+strlen(fileName)+1,pByte,dwLen);

			if (!Send(m_ClientSocket,(char *)pSendByte,dwSendLen,&olSend,&dwErrorCode))
			{
				delete []pSendByte;
				delete []pByte;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write mtp image file list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			delete []pSendByte;
			delete []pByte;

			// 读返回值
			CMD_OUT makeImageOut = {0};
			dwSendLen = sizeof(CMD_OUT);
			if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwSendLen,&olRecv,&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image file list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			if (makeImageOut.dwErrorCode != 0)
			{
				dwErrorCode = makeImageOut.dwErrorCode;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image file list error,filename=%s,system errorcode=%ld,%s")
					,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				goto END;
			}

			if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwSendLen)
			{
				dwErrorCode = CustomError_Get_Data_From_Server_Error;
				errType = ErrorType_Custom;
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file list error,filename=%s,custom errorcode=0x%X,get data from server error")
					,strImageName,dwErrorCode);

				goto END;
			}

			dwIndex++;
			ullOffset += dwLen;
			dwListSize += dwLen;
		}
	}

	//offset 取扇区的整数倍
	ullOffset = (ullOffset + 511) / 512 * 512;
	dwPkgOffset = (DWORD)ullOffset;

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult())
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <=0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& (m_MasterPort->GetPortState() == PortState_Active || !m_bCompressComplete)
			&& port->GetResult())
		{
			SwitchToThread();
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

		if (*m_lpCancel)
		{
			dwErrorCode = CustomError_Cancel;
			errType = ErrorType_Custom;
			bResult = FALSE;
			break;
		}

		if (pDataQueue->GetCount() <= 0 && m_MasterPort->GetPortState() != PortState_Active && m_bCompressComplete)
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

		if (m_bDataCompress)
		{
			*(PULONGLONG)dataInfo->pData = ullPkgIndex;
		}
		
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwSendLen = sizeof(CMD_IN) + strlen(fileName) + 2 + dataInfo->dwDataSize;

		makeImageIn.dwSizeSend = dwSendLen;

		BYTE *pByte = new BYTE[dwSendLen];
		ZeroMemory(pByte,dwSendLen);
		memcpy(pByte,&makeImageIn,sizeof(CMD_IN));
		memcpy(pByte+sizeof(CMD_IN),fileName,strlen(fileName));
		memcpy(pByte+sizeof(CMD_IN)+strlen(fileName)+1,dataInfo->pData,dataInfo->dwDataSize);

		if (!Send(m_ClientSocket,(char *)pByte,dwSendLen,&olSend,&dwErrorCode))
		{
			delete []pByte;
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write mtp image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		delete []pByte;
		delete []dataInfo->pData;
		delete dataInfo;

		// 读返回值
		CMD_OUT makeImageOut = {0};
		dwSendLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwSendLen,&olRecv,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwErrorCode != 0)
		{
			dwErrorCode = makeImageOut.dwErrorCode;
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwSendLen)
		{
			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			errType = ErrorType_Custom;
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strImageName,port->GetRealSpeed(),dwErrorCode);

			break;
		}

		//DWORD dwTime = timeGetTime();
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		ullOffset += dataInfo->dwDataSize;

		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedWaitTimeS(dbTimeNoWait);

		// 压缩的数据比实际数据短，不能取压缩后的长度，要不压缩之前的长度
		port->AppendCompleteSize(dataInfo->dwOldSize);

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write mtp image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetFileName(),port->GetRealSpeed(),dwErrorCode);
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

	// 写IMAGE头
	if (bResult)
	{
		IMAGE_HEADER imgHead;
		ZeroMemory(&imgHead,sizeof(IMAGE_HEADER));
		memcpy(imgHead.szImageFlag,IMAGE_FLAG,strlen(IMAGE_FLAG));
		imgHead.ullImageSize = ullOffset;
		memcpy(imgHead.szAppVersion,APP_VERSION,strlen(APP_VERSION));

		imgHead.dwImageType = MTP_IMAGE;

		imgHead.dwMaxSizeOfPackage = MAX_COMPRESS_BUF;
		imgHead.ullCapacitySize = m_MasterPort->GetTotalSize();
		imgHead.dwBytesPerSector = m_MasterPort->GetBytesPerSector();
		memcpy(imgHead.szZipVer,ZIP_VERSION,strlen(ZIP_VERSION));
		imgHead.byUnCompress = m_bDataCompress ? 0 : 1;

		imgHead.ullPkgCount = ullPkgIndex;
		imgHead.ullValidSize = m_MasterPort->GetValidSize();
		imgHead.dwHashLen = m_pMasterHashMethod->getHashLength();
		imgHead.dwHashType = m_MasterPort->GetHashMethod();
		memcpy(imgHead.byImageDigest,m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());

		imgHead.dwPkgOffset = dwPkgOffset;
		imgHead.dwFolderCount = m_MasterFolderObjectsList.GetCount();
		imgHead.dwFileCount = m_MasterFileObjectsList.GetCount();
		imgHead.dwListSize = dwListSize;

		imgHead.byEnd = END_FLAG;

		dwSendLen = sizeof(CMD_IN) + strlen(fileName) + 2 + SIZEOF_IMAGE_HEADER;

		makeImageIn.dwSizeSend = dwSendLen;

		BYTE *pByte = new BYTE[dwSendLen];
		ZeroMemory(pByte,dwSendLen);
		pByte[dwSendLen - 1] = END_FLAG;

		memcpy(pByte,&makeImageIn,sizeof(CMD_IN));
		memcpy(pByte+sizeof(CMD_IN),fileName,strlen(fileName));
		memcpy(pByte+sizeof(CMD_IN)+strlen(fileName)+1,&imgHead,SIZEOF_IMAGE_HEADER);

		if (!Send(m_ClientSocket,(char *)pByte,dwSendLen,&olSend,&dwErrorCode))
		{
			delete []pByte;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write mtp image head error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			goto END;
		}

		delete []pByte;

		// 读返回值
		CMD_OUT makeImageOut = {0};
		dwSendLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwSendLen,&olRecv,&dwErrorCode))
		{
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image head error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			goto END;
		}

		if (makeImageOut.dwErrorCode != 0)
		{
			dwErrorCode = makeImageOut.dwErrorCode;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image head error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			goto END;
		}

		if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwSendLen)
		{
			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			errType = ErrorType_Custom;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write mtp image head error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strImageName,port->GetRealSpeed(),dwErrorCode);

			goto END;
		}
	}

END:
	WSACloseEvent(olSend.hEvent);
	WSACloseEvent(olRecv.hEvent);

	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		port->SetPortState(PortState_Pass);
	}
	else
	{
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CWpdDevice::ReadImage()
{
	if (m_bServerFirst)
	{
		return ReadRemoteImage();
	}
	else
	{
		return ReadLocalImage();
	}
}

BOOL CWpdDevice::ReadLocalImage()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	DWORD dwLen = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2,t3;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	// 等待其他线程创建好,最多等5次
	Sleep(100);

	int nTimes = 5;
	while (!IsTargetsReady() && nTimes > 0)
	{
		Sleep(100);
		nTimes--;
	}

	ULONGLONG ullReadSize = m_dwPkgOffset;
	ULONGLONG ullOffset = m_dwPkgOffset;

	while (bResult && !*m_lpCancel && ullReadSize < m_ullImageSize && m_MasterPort->GetPortState() == PortState_Active)
	{
		QueryPerformanceCounter(&t0);
		// 判断队列是否达到限制值
		while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE)
			&& !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
		{
			SwitchToThread();
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

		QueryPerformanceCounter(&t1);

		BYTE pkgHead[PKG_HEADER_SIZE] = {NULL};
		dwLen = PKG_HEADER_SIZE;
		if (!ReadFileAsyn(m_hImage,ullOffset,dwLen,pkgHead,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read mtp image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = *(PDWORD)&pkgHead[8];

		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		if (!ReadFileAsyn(m_hImage,ullOffset,dwLen,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;
			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (pByte[dwLen-1] != END_FLAG)
		{
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_Image_Format_Error;
			bResult = FALSE;
			delete []pByte;
			break;
		}

		QueryPerformanceCounter(&t2);

		PDATA_INFO dataInfo = new DATA_INFO;
		ZeroMemory(dataInfo,sizeof(DATA_INFO));
		dataInfo->ullOffset = *(PULONGLONG)pByte;
		dataInfo->dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo->pData = new BYTE[dataInfo->dwDataSize];
		memcpy(dataInfo->pData,&pByte[PKG_HEADER_SIZE],dataInfo->dwDataSize);

		delete []pByte;

		if (m_bDataCompress)
		{
			m_CompressQueue.AddTail(dataInfo);
		}
		else
		{
			AddDataQueueList(dataInfo);

			m_pMasterHashMethod->update(dataInfo->pData,dataInfo->dwDataSize);

			delete []dataInfo->pData;
			delete dataInfo;
		}

		ullOffset += dwLen;
		ullReadSize += dwLen;

		QueryPerformanceCounter(&t3);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
		dbTimeWait = (double)(t3.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);

		// 因为是压缩数据，长度比实际长度短，所以要根据速度计算
		m_MasterPort->SetCompleteSize(m_MasterPort->GetValidSize() * ullReadSize / m_ullImageSize);

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read mtp image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode);
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

	// 所有数据都拷贝完
	while (!m_bCompressComplete)
	{
		SwitchToThread();
		Sleep(100);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}


	m_MasterPort->SetResult(bResult);
	m_MasterPort->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (m_bComputeHash)
		{
			m_MasterPort->SetHash(m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());

			CString strImageHash;
			for (int i = 0; i < m_pMasterHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),m_pMasterHashMethod->digest()[i]);
				m_strMasterHash += strHash;

				strHash.Format(_T("%02X"),m_ImageHash[i]);
				strImageHash += strHash;
			}

			CString strHashMethod(m_pMasterHashMethod->getHashMetod());

			// 此处加入判断IMAGE解压过程中是否出错
			if (strImageHash.CompareNoCase(m_strMasterHash) != 0)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Image,%s - %s,Image hash value was changed,Compute=%s,Record=%s")
					,m_MasterPort->GetFileName(),strHashMethod,m_strMasterHash,strImageHash);

				bResult = FALSE;
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Hash_Value_Changed);
			}
			else
			{
				m_MasterPort->SetResult(TRUE);
				m_MasterPort->SetPortState(PortState_Pass);
				m_MasterPort->SetErrorCode(errType,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Image,%s - %s,HashValue=%s")
					,m_MasterPort->GetFileName(),strHashMethod,m_strMasterHash);
			}

		}
		else
		{
			m_MasterPort->SetResult(TRUE);
			m_MasterPort->SetPortState(PortState_Pass);
			m_MasterPort->SetErrorCode(errType,dwErrorCode);
		}
	}
	else
	{
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CWpdDevice::ReadRemoteImage()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	ULONGLONG ullReadSize = m_dwPkgOffset;
	DWORD dwLen = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2,t3;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	WSAOVERLAPPED ol = {0};
	ol.hEvent = WSACreateEvent();

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	// 等待其他线程创建好,最多等5次
	Sleep(100);

	int nTimes = 5;
	while (!IsTargetsReady() && nTimes > 0)
	{
		Sleep(100);
		nTimes--;
	}

	// 发送COPY IMAGE命令
	CString strImageName = CUtils::GetFileName(m_MasterPort->GetFileName());

	USES_CONVERSION;
	char *fileName = W2A(strImageName);

	DWORD dwSendLen = sizeof(CMD_IN) + strlen(fileName) + 2;
	BYTE *sendBuf = new BYTE[dwSendLen];
	ZeroMemory(sendBuf,dwSendLen);
	sendBuf[dwSendLen - 1] = END_FLAG;

	CMD_IN copyImageIn = {0};
	copyImageIn.dwCmdIn = CMD_COPY_IMAGE_IN;
	copyImageIn.dwSizeSend = dwSendLen;

	memcpy(sendBuf,&copyImageIn,sizeof(CMD_IN));
	memcpy(sendBuf + sizeof(CMD_IN),fileName,strlen(fileName));

	while (bResult && !*m_lpCancel && ullReadSize < m_ullImageSize && m_MasterPort->GetPortState() == PortState_Active)
	{
		QueryPerformanceCounter(&t0);
		// 判断队列是否达到限制值
		while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE)
			&& !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
		{
			SwitchToThread();
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

		QueryPerformanceCounter(&t1);

		if (!Send(m_ClientSocket,(char *)sendBuf,dwSendLen,NULL,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send copy mtp image command error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		CMD_OUT copyImageOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&copyImageOut,dwLen,&ol,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy mtp image error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = copyImageOut.dwSizeSend - sizeof(CMD_OUT);

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		DWORD dwRead = 0;

		while(dwRead < dwLen)
		{
			DWORD dwByteRead = dwLen - dwRead;
			if (!Recv(m_ClientSocket,(char *)(pByte+dwRead),dwByteRead,&ol,&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy mtp image error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
					,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}

			dwRead += dwByteRead;
		}

		if (dwRead < dwLen)
		{
			bResult = FALSE;

			delete []pByte;

			break;
		}

		if (copyImageOut.dwErrorCode != 0)
		{
			bResult = FALSE;
			errType = copyImageOut.errType;
			dwErrorCode = copyImageOut.dwErrorCode;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy mtp image error,filename=%s,Speed=%.2f,system errorcode=%d,%s")
				,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (copyImageOut.dwCmdOut != CMD_COPY_IMAGE_OUT || copyImageOut.dwSizeSend != dwLen + sizeof(CMD_OUT))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_Get_Data_From_Server_Error;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy mtp image error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode);
			break;
		}

		QueryPerformanceCounter(&t2);

		// 去除尾部标志
		dwLen -= 1;

		PDATA_INFO dataInfo = new DATA_INFO;
		ZeroMemory(dataInfo,sizeof(DATA_INFO));

		dataInfo->ullOffset = *(PULONGLONG)pByte;
		dataInfo->dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo->pData = new BYTE[dataInfo->dwDataSize];
		memcpy(dataInfo->pData,&pByte[PKG_HEADER_SIZE],dataInfo->dwDataSize);

		if (m_bDataCompress)
		{
			m_CompressQueue.AddTail(dataInfo);
		}
		else
		{
			AddDataQueueList(dataInfo);

			m_pMasterHashMethod->update(dataInfo->pData,dataInfo->dwDataSize);

			delete []dataInfo->pData;
			delete dataInfo;
		}

		// 写文件
		if (m_hImage != INVALID_HANDLE_VALUE)
		{
			WriteFileAsyn(m_hImage,ullReadSize,dwLen,pByte,m_MasterPort->GetOverlapped(FALSE),&dwErrorCode);
		}


		dwErrorCode = 0;

		delete []pByte;

		ullReadSize += dwLen;

		QueryPerformanceCounter(&t3);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
		dbTimeWait = (double)(t3.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);

		// 因为是压缩数据，长度比实际长度短，所以要根据速度计算
		m_MasterPort->SetCompleteSize(m_MasterPort->GetValidSize() * ullReadSize / m_ullImageSize);

	}

	WSACloseEvent(ol.hEvent);

	if (m_hImage != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hImage);
		m_hImage = INVALID_HANDLE_VALUE;
	}

	if (!bResult)
	{
		// 发送停止命令
		copyImageIn.byStop = TRUE;
		memcpy(sendBuf,&copyImageIn,sizeof(CMD_IN));
		memcpy(sendBuf + sizeof(CMD_IN),fileName,strlen(fileName));

		DWORD dwError = 0;

		Send(m_ClientSocket,(char *)sendBuf,dwSendLen,NULL,&dwError);
	}

	delete []sendBuf;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read mtp image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode);
	}

	// 先设置为停止状态
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


	// 所有数据都拷贝完
	while (!m_bCompressComplete)
	{
		SwitchToThread();
		Sleep(100);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}

	m_MasterPort->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (m_bComputeHash)
		{
			m_MasterPort->SetHash(m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());

			CString strImageHash;
			for (int i = 0; i < m_pMasterHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),m_pMasterHashMethod->digest()[i]);
				m_strMasterHash += strHash;

				strHash.Format(_T("%02X"),m_ImageHash[i]);
				strImageHash += strHash;
			}

			CString strHashMethod(m_pMasterHashMethod->getHashMetod());

			// 此处加入判断IMAGE解压过程中是否出错
			if (strImageHash.CompareNoCase(m_strMasterHash) != 0)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Image,%s - %s,Image hash value was changed,Compute=%s,Record=%s")
					,m_MasterPort->GetFileName(),strHashMethod,m_strMasterHash,strImageHash);

				bResult = FALSE;
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Image_Hash_Value_Changed);
			}
			else
			{
				m_MasterPort->SetResult(TRUE);
				m_MasterPort->SetPortState(PortState_Pass);
				m_MasterPort->SetErrorCode(errType,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("MTP Image,%s - %s,HashValue=%s")
					,m_MasterPort->GetFileName(),strHashMethod,m_strMasterHash);
			}

		}
		else
		{
			m_MasterPort->SetResult(TRUE);
			m_MasterPort->SetPortState(PortState_Pass);
			m_MasterPort->SetErrorCode(errType,dwErrorCode);
		}
	}
	else
	{
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CWpdDevice::Compress()
{
	m_bCompressComplete = FALSE;
	BOOL bResult = TRUE;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	// 等待其他线程创建好,最多等5次
	Sleep(100);

	int nTimes = 5;
	while (!IsTargetsReady() && nTimes > 0)
	{
		Sleep(100);
		nTimes--;
	}

	QueryPerformanceFrequency(&freq);
	while (!*m_lpCancel && m_MasterPort->GetResult())
	{
		QueryPerformanceCounter(&t0);
		while (m_CompressQueue.GetCount() <= 0 
			&& !*m_lpCancel 
			&& m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			SwitchToThread();
			Sleep(5);
		}

		if (!m_MasterPort->GetResult() || *m_lpCancel )
		{
			bResult = FALSE;
			break;
		}

		if (m_CompressQueue.GetCount() <= 0 && m_MasterPort->GetPortState() != PortState_Active)
		{
			bResult = TRUE;
			break;
		}

		QueryPerformanceCounter(&t1);
		PDATA_INFO dataInfo = m_CompressQueue.GetHeadRemove();
		if (dataInfo == NULL)
		{
			continue;
		}

		size_t len = 0;
		DWORD dwSourceLen = 0;
		dwSourceLen += (_tcslen(dataInfo->szFileName) + 1) * sizeof(TCHAR);
		dwSourceLen += sizeof(DWORD);
		dwSourceLen += dataInfo->dwDataSize;

		BYTE *pBuffer = new BYTE[dwSourceLen];
		ZeroMemory(pBuffer,dwSourceLen);
		PBYTE pTemp = pBuffer;

		len = (_tcslen(dataInfo->szFileName) + 1) * sizeof(TCHAR);

		memcpy(pTemp,dataInfo->szFileName,len);
		pTemp += len;

		memcpy(pTemp,&dataInfo->dwDataSize,sizeof(DWORD));
		pTemp += sizeof(DWORD);

		memcpy(pTemp,dataInfo->pData,dataInfo->dwDataSize);
		pTemp += dataInfo->dwDataSize;

		delete []dataInfo->pData;

		DWORD dwDestLen = MAX_COMPRESS_BUF;
		BYTE *pDest = new BYTE[MAX_COMPRESS_BUF];
		ZeroMemory(pDest,MAX_COMPRESS_BUF);

		int ret = compress2(pDest,&dwDestLen,pBuffer,dwSourceLen,m_iCompressLevel);

		if (ret == Z_OK)
		{
			PDATA_INFO compressData = new DATA_INFO;
			ZeroMemory(compressData,sizeof(DATA_INFO));

			compressData->dwDataSize = dwDestLen + sizeof(ULONGLONG) + sizeof(DWORD) + 1;
			compressData->dwOldSize = dataInfo->dwDataSize;
			compressData->pData = new BYTE[compressData->dwDataSize];
			ZeroMemory(compressData->pData,compressData->dwDataSize);

			compressData->pData[compressData->dwDataSize - 1] = 0xED; //结束标志
			memcpy(compressData->pData + sizeof(ULONGLONG),&compressData->dwDataSize,sizeof(DWORD));
			memcpy(compressData->pData + sizeof(ULONGLONG) + sizeof(DWORD),pDest,dwDestLen);

			AddDataQueueList(compressData);

			delete []compressData->pData;
			delete []compressData;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

		}
		else if (ret == Z_DATA_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Compress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress data error."));
		}
		else if (ret == Z_MEM_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Compress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress memory error."));
		}
		else if (ret == Z_BUF_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Compress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress buffer error."));
		}
		else
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_Compress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress data error."));
		}

		delete []pBuffer;
		delete []pDest;
		delete dataInfo;
	}

	m_bCompressComplete = TRUE;

	return bResult;
}

BOOL CWpdDevice::Uncompress()
{
	m_bCompressComplete = FALSE;

	BOOL bResult = TRUE;
	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	// 等待其他线程创建好,最多等5次
	Sleep(100);

	int nTimes = 5;
	while (!IsTargetsReady() && nTimes > 0)
	{
		Sleep(100);
		nTimes--;
	}

	QueryPerformanceFrequency(&freq);

	while (!*m_lpCancel && m_MasterPort->GetResult() && bResult)
	{
		QueryPerformanceCounter(&t0);
		while (m_CompressQueue.GetCount() <= 0
			&& !*m_lpCancel 
			&& m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			SwitchToThread();
			Sleep(5);
		}

		if (!m_MasterPort->GetResult() || *m_lpCancel)
		{
			bResult = FALSE;
			break;
		}

		if (m_CompressQueue.GetCount() <= 0 && m_MasterPort->GetPortState() != PortState_Active)
		{
			bResult = TRUE;
			break;
		}

		QueryPerformanceCounter(&t1);
		PDATA_INFO dataInfo = m_CompressQueue.GetHeadRemove();

		if (dataInfo == NULL)
		{
			continue;
		}

		DWORD dwDestLen = MAX_COMPRESS_BUF;
		BYTE *pDest = new BYTE[MAX_COMPRESS_BUF];
		ZeroMemory(pDest,MAX_COMPRESS_BUF);

		int ret = uncompress(pDest,&dwDestLen,dataInfo->pData,dataInfo->dwDataSize);

		delete []dataInfo->pData;

		if (ret == Z_OK)
		{
			PDATA_INFO uncompressData = new DATA_INFO;
			ZeroMemory(uncompressData,sizeof(DATA_INFO));

			// ObjectID,DataSize,Data
			size_t len = (wcslen((WCHAR *)pDest) + 1) * sizeof(WCHAR);
			uncompressData->szFileName = new WCHAR[wcslen((WCHAR *)pDest) + 1];
			wcscpy_s(uncompressData->szFileName,wcslen((WCHAR *)pDest) + 1,(WCHAR *)pDest);
			uncompressData->dwDataSize = *(PDWORD)(pDest + len);

			uncompressData->pData = new BYTE[uncompressData->dwDataSize];
			ZeroMemory(uncompressData->pData,uncompressData->dwDataSize);
			memcpy(uncompressData->pData,pDest + len + sizeof(DWORD),uncompressData->dwDataSize);

			AddDataQueueList(uncompressData);

			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)uncompressData->pData,uncompressData->dwDataSize);
			}

			delete []uncompressData->pData;
			delete []uncompressData->szFileName;
			delete uncompressData;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		}
		else if (ret == Z_DATA_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_UnCompress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress data error."));
		}
		else if (ret == Z_MEM_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_UnCompress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress memory error."));
		}
		else if (ret == Z_BUF_ERROR)
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_UnCompress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress buffer error."));
		}
		else
		{
			bResult = FALSE;
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetErrorCode(ErrorType_Custom,CustomError_UnCompress_Error);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress data error."));
		}

		delete []pDest;
		delete dataInfo;
	}

	m_bCompressComplete = TRUE;

	return bResult;
}

UINT CWpdDevice::GetCurrentTargetCount()
{
	UINT nCount = 0;
	POSITION pos = m_TargetPorts->GetHeadPosition();

	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);

		if (port)
		{
			if (port->IsConnected() && port->GetResult())
			{
				nCount++;
			}
		}
	}

	return nCount;
}

void CWpdDevice::SetCompareParm( BOOL bCompare,CompareMethod method )
{
	m_bCompare = bCompare;
}

bool CWpdDevice::IsTargetsReady()
{
	POSITION pos = m_TargetPorts->GetHeadPosition();
	bool bReady = true;
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);

		if (port->IsConnected() && port->GetPortState() != PortState_Active)
		{
			bReady = false;

			break;
		}

	}

	return bReady;
}




