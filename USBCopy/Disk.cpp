#include "StdAfx.h"
#include "Disk.h"
#include <WinIoCtl.h>
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



#define HIDE_SECTORS  0x800   // 1M
#define COMPRESS_THREAD_COUNT 1

CDisk::CDisk(void)
{
	m_hLogFile = INVALID_HANDLE_VALUE;
	m_hMaster = INVALID_HANDLE_VALUE;
	m_dwBytesPerSector = 0;
	m_ullSectorNums = 0;
	m_ullValidSize = 0;
	m_ullCapacity = 0;

	m_bHashVerify = FALSE;
	m_pMasterHashMethod = NULL;

	m_nCurrentTargetCount = 0;

	ZeroMemory(m_ImageHash,LEN_DIGEST);
}


CDisk::~CDisk(void)
{

	if (m_hMaster != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hMaster);
	}

	POSITION pos = m_DataQueueList.GetHeadPosition();
	while (pos)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos);
		delete dataQueue;
	}
}

HANDLE CDisk::GetHandleOnPhysicalDrive( int iDiskNumber,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode )
{
	TCHAR szPhysicalDrive[50] = {NULL};

	_stprintf_s(szPhysicalDrive,_T("\\\\.\\PhysicalDrive%d"),iDiskNumber);

	HANDLE hDisk = CreateFile(szPhysicalDrive,
		GENERIC_READ | GENERIC_WRITE ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		dwFlagsAndAttributes,
		NULL);

	*pdwErrorCode = GetLastError();

	return hDisk;
}

HANDLE CDisk::GetHandleOnVolume( TCHAR letter,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode )
{
	TCHAR szVolumePath[50] = {NULL};

	_stprintf_s(szVolumePath,_T("\\\\.\\%c:"),letter);

	HANDLE hVolume = CreateFile(szVolumePath,
		GENERIC_READ | GENERIC_WRITE ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		dwFlagsAndAttributes,
		NULL);

	*pdwErrorCode = GetLastError();

	return hVolume;
}

HANDLE CDisk::GetHandleOnFile( LPCTSTR lpszFileName,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode )
{
	HANDLE hFile = CreateFile(lpszFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		NULL);
	*pdwErrorCode = GetLastError();
	return hFile;
}

ULONGLONG CDisk::GetNumberOfSectors( HANDLE hDevice,PDWORD pdwBytesPerSector )
{
	DWORD junk;
	DISK_GEOMETRY_EX diskgeometry;
	BOOL bResult;
	bResult = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskgeometry, sizeof(diskgeometry), &junk, NULL);
	if (!bResult)
	{
		return 0;
	}

	if (pdwBytesPerSector != NULL)
	{
		*pdwBytesPerSector = diskgeometry.Geometry.BytesPerSector;
	}

	return (ULONGLONG)diskgeometry.DiskSize.QuadPart / (ULONGLONG)diskgeometry.Geometry.BytesPerSector;
}

void CDisk::DeleteDirectory( LPCTSTR lpszPath )
{
	CFileFind ff;
	CString strTmpPath (lpszPath);
	strTmpPath += _T("\\*");
	BOOL bFound = ff.FindFile(strTmpPath, 0);
	while(bFound)
	{
		bFound = ff.FindNextFile();
		if ((ff.GetFileName() == _T(".")) || (ff.GetFileName() == _T("..")))
		{
			continue;
		}
		//
		SetFileAttributes(ff.GetFilePath(), FILE_ATTRIBUTE_NORMAL);
		if (ff.IsDirectory())
		{ //
			DeleteDirectory(ff.GetFilePath());
		}
		else
		{
			DeleteFile(ff.GetFilePath()); //
		}
	}
	ff.Close();
	RemoveDirectory(lpszPath);
}

BOOL CDisk::CreateDisk( int disk,ULONG PartitionNumber )
{
	HANDLE hDevice;               // handle to the drive to be examined
	BOOL result;                  // results flag
	DWORD readed;                 // discard results
	WORD i;
	TCHAR diskPath[50];
	DISK_GEOMETRY_EX pdgex;
	DWORD sectorSize;
	DWORD signature;
	LARGE_INTEGER diskSize;
	LARGE_INTEGER partSize;
	BYTE actualPartNum;

	DWORD layoutStructSize;
	DRIVE_LAYOUT_INFORMATION_EX *dl;
	CREATE_DISK newDisk;

	_stprintf_s(diskPath, _T("\\\\.\\PhysicalDrive%d"), disk);

	actualPartNum = 4;
	if (PartitionNumber > actualPartNum)
	{
		return (WORD)-1;
	}

	hDevice = CreateFile(
		diskPath,
		GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,           //default security attributes   
		OPEN_EXISTING, // disposition   
		0,              // file attributes   
		NULL
		);
	if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
		return FALSE;
	}

	// Create primary partition MBR
	newDisk.PartitionStyle = PARTITION_STYLE_MBR;
	signature = (DWORD)time(NULL);     //get signature from current time
	newDisk.Mbr.Signature = signature;

	result = DeviceIoControl(
		hDevice,
		IOCTL_DISK_CREATE_DISK,
		&newDisk,
		sizeof(CREATE_DISK),
		NULL,
		0,
		&readed,
		NULL
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_CREATE_DISK Error: %ld\n", GetLastError());
		CloseHandle(hDevice);
		return FALSE;
	}

	//fresh the partition table
	result = DeviceIoControl(
		hDevice,
		IOCTL_DISK_UPDATE_PROPERTIES, 
		NULL,
		0,
		NULL,
		0,
		&readed,
		NULL
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
		CloseHandle(hDevice);
		return FALSE;
	}

	//Now create the partitions
	result = DeviceIoControl(
		hDevice, 
		IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, 
		NULL, 
		0, 
		&pdgex, 
		sizeof(pdgex), 
		&readed, 
		NULL);

	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX Error: %ld\n", GetLastError());
		CloseHandle(hDevice);
		return FALSE;
	}

	sectorSize = pdgex.Geometry.BytesPerSector;
	diskSize.QuadPart = pdgex.DiskSize.QuadPart;       //calculate the disk size;
	partSize.QuadPart = diskSize.QuadPart / PartitionNumber;

	layoutStructSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (actualPartNum - 1) * sizeof(PARTITION_INFORMATION_EX);
	dl = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(layoutStructSize);
	if (NULL == dl)
	{
		CloseHandle(hDevice);
		return FALSE;
	}

	dl->PartitionStyle = (DWORD)PARTITION_STYLE_MBR;
	dl->PartitionCount = actualPartNum;
	dl->Mbr.Signature = signature;

	//clear the unused partitions
	for (i = 0; i < actualPartNum; i++){
		dl->PartitionEntry[i].RewritePartition = 1;
		dl->PartitionEntry[i].Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
		dl->PartitionEntry[i].Mbr.BootIndicator = FALSE;
	}
	//set the profile of the partitions
	for (i = 0; i < PartitionNumber; i++){
		dl->PartitionEntry[i].PartitionStyle = PARTITION_STYLE_MBR;
		dl->PartitionEntry[i].StartingOffset.QuadPart = 
			(partSize.QuadPart * i) + (HIDE_SECTORS * (LONGLONG)(pdgex.Geometry.BytesPerSector));   //�̶�1M
		dl->PartitionEntry[i].PartitionLength.QuadPart = partSize.QuadPart - dl->PartitionEntry[i].StartingOffset.QuadPart;
		dl->PartitionEntry[i].PartitionNumber = i + 1;
		dl->PartitionEntry[i].RewritePartition = TRUE;
		dl->PartitionEntry[i].Mbr.PartitionType = PARTITION_IFS;
		dl->PartitionEntry[i].Mbr.BootIndicator = FALSE;
		dl->PartitionEntry[i].Mbr.RecognizedPartition = TRUE;
		dl->PartitionEntry[i].Mbr.HiddenSectors = 
			HIDE_SECTORS + (DWORD)((partSize.QuadPart / sectorSize) * i);
	}
	//execute the layout   
	result = DeviceIoControl(
		hDevice, 
		IOCTL_DISK_SET_DRIVE_LAYOUT_EX, 
		dl, 
		layoutStructSize, 
		NULL, 
		0,
		&readed, 
		NULL
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_SET_DRIVE_LAYOUT_EX Error: %ld\n", GetLastError());
		free(dl);
		(void)CloseHandle(hDevice);
		return DWORD(-1);
	}

	//fresh the partition table
	result = DeviceIoControl(
		hDevice,
		IOCTL_DISK_UPDATE_PROPERTIES, 
		NULL,
		0,
		NULL,
		0,
		&readed,
		NULL
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
		free(dl);
		CloseHandle(hDevice);
		return FALSE;
	}

	free(dl);
	CloseHandle(hDevice);
	Sleep(1000);            //wait the operations take effect
	return TRUE;
}

BOOL CDisk::DestroyDisk( int disk )
{
	HANDLE hDevice;               // handle to the drive to be examined
	BOOL result;                  // results flag
	DWORD readed;                 // discard results
	TCHAR diskPath[50];

	_stprintf_s(diskPath, _T("\\\\.\\PhysicalDrive%d"), disk);

	hDevice = CreateFile(
		diskPath, // drive to open
		GENERIC_READ | GENERIC_WRITE,     // access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE, //share mode
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL            // do not copy file attribute
		);
	if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
		return FALSE;
	}

	result = DeviceIoControl(
		hDevice,               // handle to device
		IOCTL_DISK_DELETE_DRIVE_LAYOUT, // dwIoControlCode
		NULL,                           // lpInBuffer
		0,                              // nInBufferSize
		NULL,                           // lpOutBuffer
		0,                              // nOutBufferSize
		&readed,      // number of bytes returned
		NULL        // OVERLAPPED structure
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_DELETE_DRIVE_LAYOUT Error: %ld\n", GetLastError());
		CloseHandle(hDevice);
		return FALSE;
	}

	//fresh the partition table
	result = DeviceIoControl(
		hDevice,
		IOCTL_DISK_UPDATE_PROPERTIES, 
		NULL,
		0,
		NULL,
		0,
		&readed,
		NULL
		);
	if (!result)
	{
		fprintf(stderr, "IOCTL_DISK_UPDATE_PROPERTIES Error: %ld\n", GetLastError());
		CloseHandle(hDevice);
		return FALSE;
	}

	CloseHandle(hDevice);
	return TRUE;
}

BOOL CDisk::ChangeLetter( LPCTSTR lpszVolumeName,LPCTSTR lpszVolumePath )
{
	BOOL bFlag = TRUE;
	TCHAR szPath[MAX_PATH] = {NULL};
	DWORD dwReturn = 0;
	GetVolumePathNamesForVolumeName(lpszVolumeName,szPath,MAX_PATH,&dwReturn);

	if (_tccmp(szPath,lpszVolumePath) == 0)
	{
		return TRUE;
	}
	DeleteVolumeMountPoint(lpszVolumePath);
	DeleteVolumeMountPoint(szPath);

	bFlag = SetVolumeMountPoint(lpszVolumePath,lpszVolumeName);

	return bFlag;
}

BOOL CDisk::LockOnVolume( HANDLE hVolume,DWORD *pdwErrorCode )
{
	DWORD dwBytesReturned;
	BOOL bResult = DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	*pdwErrorCode = GetLastError();

	return bResult;
}

BOOL CDisk::UnLockVolume( HANDLE hVolume,DWORD *pdwErrorCode )
{
	DWORD dwBytesReturned;
	BOOL bResult = DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	*pdwErrorCode = GetLastError();

	return bResult;
}

BOOL CDisk::UnmountVolume( HANDLE hVolume,DWORD *pdwErrorCode )
{
	DWORD dwBytesReturned;
	BOOL bResult = DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	*pdwErrorCode = GetLastError();

	return bResult;
}

BOOL CDisk::IsVolumeUnmount( HANDLE hVolume )
{
	DWORD dwBytesReturned;
	BOOL bResult = DeviceIoControl(hVolume, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	return !bResult;
}

BOOL CDisk::ReadSectors( HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff, LPOVERLAPPED lpOverlap,DWORD *pdwErrorCode )
{
	ULONGLONG ullOffset = ullStartSector * dwBytesPerSector;
	DWORD dwLen = dwSectors * dwBytesPerSector;
	DWORD dwReadLen = 0;
	DWORD dwErrorCode = 0;

	if (lpOverlap)
	{
		lpOverlap->Offset = LODWORD(ullOffset);
		lpOverlap->OffsetHigh = HIDWORD(ullOffset);
	}
	
	if (!ReadFile(hDevice,lpSectBuff,dwLen,&dwReadLen,lpOverlap))
	{
		dwErrorCode = ::GetLastError();

		if(dwErrorCode == ERROR_IO_PENDING) // �����첽I/O
		{
			if (WaitForSingleObject(lpOverlap->hEvent, INFINITE) != WAIT_FAILED)
			{
				if(!::GetOverlappedResult(hDevice, lpOverlap, &dwReadLen, FALSE))
				{
					*pdwErrorCode = ::GetLastError();
					return FALSE;
				}
				else
				{
					return TRUE;
				}
			}
			else
			{
				*pdwErrorCode = ::GetLastError();
				return FALSE;
			}

		}
		else
		{
			*pdwErrorCode = dwErrorCode;
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

BOOL CDisk::WriteSectors( HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff,LPOVERLAPPED lpOverlap, DWORD *pdwErrorCode )
{
	ULONGLONG ullOffset = ullStartSector * dwBytesPerSector;
	DWORD dwLen = dwSectors * dwBytesPerSector;
	DWORD dwWriteLen = 0;
	DWORD dwErrorCode = 0;

	if (lpOverlap)
	{
		lpOverlap->Offset = LODWORD(ullOffset);
		lpOverlap->OffsetHigh = HIDWORD(ullOffset);
	}
	

	if (!WriteFile(hDevice,lpSectBuff,dwLen,&dwWriteLen,lpOverlap))
	{
		dwErrorCode = ::GetLastError();

		if(dwErrorCode == ERROR_IO_PENDING) // �����첽I/O
		{
			if (WaitForSingleObject(lpOverlap->hEvent, INFINITE) != WAIT_FAILED)
			{
				if(!::GetOverlappedResult(hDevice, lpOverlap, &dwWriteLen, FALSE))
				{
					*pdwErrorCode = ::GetLastError();
					return FALSE;
				}
				else
				{
					return TRUE;
				}
			}
			else
			{
				*pdwErrorCode = ::GetLastError();
				return FALSE;
			}

		}
		else
		{
			*pdwErrorCode = dwErrorCode;
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

BOOL CDisk::ReadFileAsyn( HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode )
{
	DWORD dwReadLen = 0;
	DWORD dwErrorCode = 0;

	if (lpOverlap)
	{
		lpOverlap->Offset = LODWORD(ullOffset);
		lpOverlap->OffsetHigh = HIDWORD(ullOffset);
	}
	

	if (!ReadFile(hFile,lpBuffer,dwSize,&dwReadLen,lpOverlap))
	{
		dwErrorCode = ::GetLastError();

		if(dwErrorCode == ERROR_IO_PENDING) // �����첽I/O
		{
			if (WaitForSingleObject(lpOverlap->hEvent, INFINITE) != WAIT_FAILED)
			{
				if(!::GetOverlappedResult(hFile, lpOverlap, &dwReadLen, FALSE))
				{
					*pdwErrorCode = ::GetLastError();
					return FALSE;
				}
				else
				{
					dwSize = dwReadLen;
					return TRUE;
				}
			}
			else
			{
				*pdwErrorCode = ::GetLastError();
				return FALSE;
			}

		}
		else
		{
			*pdwErrorCode = dwErrorCode;
			return FALSE;
		}
	}
	else
	{
		dwSize = dwReadLen;
		return TRUE;
	}
}

BOOL CDisk::WriteFileAsyn( HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode )
{
	DWORD dwWriteLen = 0;
	DWORD dwErrorCode = 0;

	if (lpOverlap)
	{
		lpOverlap->Offset = LODWORD(ullOffset);
		lpOverlap->OffsetHigh = HIDWORD(ullOffset);
	}
	

	if (!WriteFile(hFile,lpBuffer,dwSize,&dwWriteLen,lpOverlap))
	{
		dwErrorCode = ::GetLastError();

		if(dwErrorCode == ERROR_IO_PENDING) // �����첽I/O
		{
			if (WaitForSingleObject(lpOverlap->hEvent, INFINITE) != WAIT_FAILED)
			{
				if(!::GetOverlappedResult(hFile, lpOverlap, &dwWriteLen, FALSE))
				{
					*pdwErrorCode = ::GetLastError();
					return FALSE;
				}
				else
				{
					dwSize = dwWriteLen;
					return TRUE;
				}
			}
			else
			{
				*pdwErrorCode = ::GetLastError();
				return FALSE;
			}

		}
		else
		{
			*pdwErrorCode = dwErrorCode;
			return FALSE;
		}
	}
	else
	{
		dwSize = dwWriteLen;
		return TRUE;
	}
}

void CDisk::Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile )
{
	m_hWnd = hWnd;
	m_lpCancel = lpCancel;
	m_hLogFile = hLogFile;
}

void CDisk::SetMasterPort( CPort *port )
{
	m_MasterPort = port;
	m_MasterPort->Reset();

	DWORD dwErrorCode = 0;
	switch (m_MasterPort->GetPortType())
	{
	case PortType_MASTER_DISK:
		m_hMaster = GetHandleOnPhysicalDrive(m_MasterPort->GetDiskNum(),
			FILE_FLAG_OVERLAPPED,&dwErrorCode);
		if (m_hMaster == INVALID_HANDLE_VALUE)
		{
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);
			m_MasterPort->SetPortState(PortState_Fail);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenDisk error,system errorcode=%ld,%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		}
		else
		{
			m_MasterPort->SetPortState(PortState_Active);
			m_ullSectorNums = GetNumberOfSectors(m_hMaster,&m_dwBytesPerSector);
			m_ullCapacity = m_ullSectorNums * m_dwBytesPerSector;
		}
		break;

	case PortType_MASTER_FILE:
		m_hMaster = GetHandleOnFile(m_MasterPort->GetFileName(),
			OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);

		if (m_hMaster == INVALID_HANDLE_VALUE)
		{
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);
			m_MasterPort->SetPortState(PortState_Fail);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Open image file error,filename=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		}
		else
		{
			m_MasterPort->SetPortState(PortState_Active);
			LARGE_INTEGER li = {0};
			if (GetFileSizeEx(m_hMaster,&li))
			{
				m_ullCapacity = (ULONGLONG)li.QuadPart;
			}

		}
	}
}

void CDisk::SetTargetPorts( PortList *pList )
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
		
	}
}

void CDisk::SetHashMethod( BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod )
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

void CDisk::SetWorkMode( WorkMode workMode )
{
	m_WorkMode = workMode;
}

void CDisk::SetCleanMode( CleanMode cleanMode,int nFillValue )
{
	m_CleanMode = cleanMode;
	m_nFillValue = nFillValue;
}


BootSector CDisk::GetBootSectorType( const PBYTE pXBR )
{
	/* �ô˷��ж��Ƿ�DBR�д�ȷ��
	// EB 3C 90 -> FAT12/16
	// EB 58 90 -> FAT32
	// EB 52 90 -> NTFS
	// EB 76 90 -> EXFAT
	*/
	MASTER_BOOT_RECORD mbr;
	ZeroMemory(&mbr,sizeof(MASTER_BOOT_RECORD));
	memcpy(&mbr,pXBR + 0x1BE,sizeof(MASTER_BOOT_RECORD));

	if (mbr.Signature != 0xAA55)
	{
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_UNKNOWN"));
		return BootSector_UNKNOWN;
	}

	ULONGLONG ullStartSector = 0;
	DWORD dwErrorCode = 0;
	for (int i = 0;i < 4;i++)
	{
		ullStartSector = mbr.Partition[i].StartLBA;
		if (ullStartSector == 0)
		{
			continue;
		}

		if (ullStartSector > m_ullSectorNums - 1)
		{
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_DBR"));
			return BootSector_DBR;
		}

		BYTE *pDBRTemp = new BYTE[m_dwBytesPerSector];
		ZeroMemory(pDBRTemp,m_dwBytesPerSector);
		
		if (!ReadSectors(m_hMaster,ullStartSector,1,m_dwBytesPerSector,pDBRTemp,m_MasterPort->GetOverlapped(),&dwErrorCode))
		{
			delete []pDBRTemp;
			pDBRTemp = NULL;

			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_DBR"));
			return BootSector_DBR;
		}

		if (GetFileSystem(pDBRTemp,ullStartSector) != FileSystem_UNKNOWN)
		{
			delete []pDBRTemp;
			pDBRTemp = NULL;

			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_MBR"));
			return BootSector_MBR;
		}
		else
		{
			delete []pDBRTemp;
			pDBRTemp = NULL;

			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_DBR"));
			return BootSector_DBR;
		}
	}

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_DBR"));
	return BootSector_DBR;
}

PartitionStyle CDisk::GetPartitionStyle( const PBYTE pByte,BootSector bootSector )
{
	PartitionStyle partitionStyle = PartitionStyle_UNKNOWN;
	if (bootSector == BootSector_MBR)
	{
		if (ReadOffset(pByte,0x1C2,1) == 0xEE)
		{
			partitionStyle = PartitionStyle_GPT;

			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_GPT"));
		}
		else
		{
			partitionStyle = PartitionStyle_DOS;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_DOS"));
		}
	}
	else
	{
		if (ReadOffset(pByte,0,2) == 0x5245)
		{
			partitionStyle = PartitionStyle_APPLE;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_APPLE"));
		}
		else if (ReadOffset(pByte,0,4) == 0x82564557)
		{
			partitionStyle = PartitionStyle_BSD;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_BSD"));
		}
		else
		{
			partitionStyle = PartitionStyle_UNKNOWN;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_UNKNOWN"));
		}
	}

	return partitionStyle;
}

FileSystem CDisk::GetFileSystem( const PBYTE pDBR,ULONGLONG ullStartSector )
{
	FileSystem fileSystem = FileSystem_UNKNOWN;
	if (ReadOffset(pDBR,3,4) == 0x5346544E) //'NTFS'
	{
		fileSystem = FileSystem_NTFS;
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_NTFS")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
	}
	else if (ReadOffset(pDBR,3,5)== 0x5441465845) //'EXFAT'
	{
		fileSystem = FileSystem_EXFAT;
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_EXFAT")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
	}
	else if (ReadOffset(pDBR,0x36,5) == 0x3231544146) //'FAT12'
	{
		fileSystem = FileSystem_FAT12;
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_FAT12")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
	}
	else if (ReadOffset(pDBR,0x36,5) == 0x3631544146) //'FAT16'
	{
		fileSystem = FileSystem_FAT16;
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_FAT16")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
	}
	else if (ReadOffset(pDBR,0x52,5) == 0x3233544146) //'FAT32'
	{
		fileSystem = FileSystem_FAT32;
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_FAT32")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
	}
	else 
	{
		// ƫ��2��������ȷ���Ƿ���EXT_X�ļ�ϵͳ
		BYTE *pByte = new BYTE[BYTES_PER_SECTOR];
		ZeroMemory(pByte,BYTES_PER_SECTOR);

		DWORD dwErrorCode = 0;
		if (ReadSectors(m_hMaster,ullStartSector + 2,1,BYTES_PER_SECTOR,pByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
		{
			if (ReadOffset(pByte,0x38,2) == 0xEF53)
			{
				fileSystem = FileSystem_EXT_X;
				CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_EXT_X")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
			}
			else
			{
				fileSystem = FileSystem_UNKNOWN;
				CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_UNKNOWN")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
			}
		}
		else
		{
			fileSystem = FileSystem_UNKNOWN;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Port %s,Disk %d - GetFileSystem=FileSystem_UNKNOWN")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum());
		}
		delete []pByte;
		pByte = NULL;
	}

	return fileSystem;
}

BOOL CDisk::BriefAnalyze()
{
	DWORD dwErrorCode = 0;

	m_MasterPort->SetStartTime(CTime::GetCurrentTime());

	BYTE *pXBR = new BYTE[m_dwBytesPerSector];
	ZeroMemory(pXBR,m_dwBytesPerSector);
	if (!ReadSectors(m_hMaster,0,1,m_dwBytesPerSector,pXBR,m_MasterPort->GetOverlapped(),&dwErrorCode))
	{
		m_MasterPort->SetEndTime(CTime::GetCurrentTime());
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read sector 0 error,system errorcode=%ld,%s")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		delete []pXBR;
		pXBR = NULL;

		return FALSE;
	}

	// ��һ�����ж�0��������MBR������DBR��
	BootSector bootSector = GetBootSectorType(pXBR);

	// �ڶ������жϷ�����ϵ
	PartitionStyle partitionStyle = GetPartitionStyle(pXBR,bootSector);
	
	// ���������ж��ļ�ϵͳ
	FileSystem fileSystem = FileSystem_UNKNOWN;
	ULONGLONG ullStartSectors = 0;

	switch (partitionStyle)
	{
	case PartitionStyle_UNKNOWN:
		fileSystem = GetFileSystem(pXBR,ullStartSectors);

		if (!AppendEffDataList(pXBR,fileSystem,ullStartSectors,ullStartSectors,m_ullSectorNums))
		{
			delete []pXBR;
			pXBR = NULL;

			return FALSE;
		}
		break;

	case PartitionStyle_DOS:
		{
			fileSystem = FileSystem_EXTEND;
			if (!AppendEffDataList(pXBR,fileSystem,ullStartSectors,ullStartSectors,m_ullSectorNums))
			{
				delete []pXBR;
				pXBR = NULL;

				return FALSE;
			}
		}
		break;

	case PartitionStyle_GPT:
		{
			BYTE *pEFI = new BYTE[m_dwBytesPerSector];
			ZeroMemory(pEFI,m_dwBytesPerSector);

			if (!ReadSectors(m_hMaster,1,1,m_dwBytesPerSector,pEFI,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read EFI info error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			
				delete []pEFI;
				pEFI = NULL;

				delete []pXBR;
				pXBR = NULL;

				return FALSE;
			}

			EFI_HEAD efiHead;
			ZeroMemory(&efiHead,sizeof(EFI_HEAD));
			memcpy(&efiHead,pEFI,sizeof(EFI_HEAD));

			delete []pEFI;
			pEFI = NULL;

			// ��ӱ���MBR��EFI��Ϣ��GPTͷ����������34������
			EFF_DATA effData;
			effData.ullStartSector = 0;
			effData.ullSectors = efiHead.FirstUsableLBA;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;
			m_EffList.AddTail(effData);

			// ����������
			DWORD dwPatitionEntrySectors = efiHead.MaxNumberOfPartitionEntries * efiHead.SizeOfPartitionEntry / m_dwBytesPerSector;
			ULONGLONG ullStartSectors = efiHead.PartitionEntryLBA;
			DWORD dwEntryIndex = 1;
			for (DWORD i = 0;i < dwPatitionEntrySectors;i++)
			{
				BYTE *pByte = new BYTE[m_dwBytesPerSector];
				ZeroMemory(pByte,m_dwBytesPerSector);
				if (!ReadSectors(m_hMaster,ullStartSectors+i,1,m_dwBytesPerSector,pByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
				{
					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);
					
					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - StartSector:%ld - Read GUID Partition Table Entry #%d erorr,system errorcode=%ld,%s")
						,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),ullStartSectors,dwEntryIndex,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					delete []pByte;
					pByte = NULL;

					delete []pXBR;
					pXBR = NULL;

					return FALSE;
				}

				DWORD dwEntryNumsPerSector = m_dwBytesPerSector/efiHead.SizeOfPartitionEntry;
				GPT_PARTITION_ENTRY *gptPartitionEntry = new GPT_PARTITION_ENTRY[dwEntryNumsPerSector];
				ZeroMemory(gptPartitionEntry,sizeof(GPT_PARTITION_ENTRY) * dwEntryNumsPerSector);
				memcpy(gptPartitionEntry,pByte,sizeof(GPT_PARTITION_ENTRY) * dwEntryNumsPerSector);

				delete []pByte;
				pByte = NULL;

				for (DWORD entry = 0; entry < dwEntryNumsPerSector;entry++)
				{
					ULONGLONG ullTempStartSector = gptPartitionEntry[entry].StartingLBA;
					ULONGLONG ullSectors = gptPartitionEntry[entry].EndingLBA - gptPartitionEntry[entry].StartingLBA;

					if (ullSectors == 0)
					{
						continue;
					}

					/* Windows�����������Կ������Բ�����
					if (ullTempStartSector == 0)
					{
						dwEntryIndex++;
						continue;
					}
					*/

					BYTE *pTempDBR = new BYTE[m_dwBytesPerSector];
					ZeroMemory(pTempDBR,m_dwBytesPerSector);
					if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pTempDBR,m_MasterPort->GetOverlapped(),&dwErrorCode))
					{
						m_MasterPort->SetEndTime(CTime::GetCurrentTime());
						m_MasterPort->SetPortState(PortState_Fail);
						m_MasterPort->SetResult(FALSE);
						m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);


						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - StartSector:%ld - Read GUID Partition Table Entry #%d erorr,system errorcode=%ld,%s")
							,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),ullTempStartSector,dwEntryIndex,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

						delete []pTempDBR;
						pTempDBR = NULL;

						delete []pXBR;
						pXBR = NULL;

						delete []gptPartitionEntry;
						gptPartitionEntry = NULL;

						return FALSE;
					}

					fileSystem = GetFileSystem(pTempDBR,ullTempStartSector);

					if (!AppendEffDataList(pTempDBR,fileSystem,ullTempStartSector,ullTempStartSector,ullSectors))
					{
						delete []pTempDBR;
						pTempDBR = NULL;

						delete []pXBR;
						pXBR = NULL;

						delete []gptPartitionEntry;
						gptPartitionEntry = NULL;

						return FALSE;
					}

					delete []pTempDBR;
					pTempDBR = NULL;

					dwEntryIndex++;
				}

				delete []gptPartitionEntry;
				gptPartitionEntry = NULL;
			}


			// ��ӷ������ݷ�����
			effData.ullStartSector = efiHead.LastUsableLBA;
			effData.ullSectors = efiHead.MaxNumberOfPartitionEntries * efiHead.SizeOfPartitionEntry / m_dwBytesPerSector;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;
			m_EffList.AddTail(effData);

			// ��ӱ���EFI��Ϣ
			effData.ullStartSector = efiHead.BackupLBA;
			effData.ullSectors = 1;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;
			m_EffList.AddTail(effData);
		}
		break;
	}
	
	delete []pXBR;
	pXBR = NULL;

	return TRUE;
}

BOOL CDisk::AppendEffDataList( const PBYTE pDBR,FileSystem fileSystem,ULONGLONG ullStartSector,ULONGLONG ullMasterSectorOffset,ULONGLONG ullSectors )
{
	DWORD dwErrorCode = 0;
	switch (fileSystem)
	{
	case FileSystem_FAT12:
	case FileSystem_FAT16:
	case FileSystem_FAT32:
		{
			FAT_INFO fatInfo;
			ZeroMemory(&fatInfo,sizeof(FAT_INFO));

			fatInfo.wBytesPerSector = (WORD)ReadOffset(pDBR,0x0B,2);
			fatInfo.bySectorsPerCluster = (BYTE)ReadOffset(pDBR,0x0D,1);

			fatInfo.wReserverSectors = (WORD)ReadOffset(pDBR,0x0E,2);

			fatInfo.byNumsOfFAT = (BYTE)ReadOffset(pDBR,0x10,1);
			fatInfo.ullStartSector = ullStartSector;
			fatInfo.dwVolumeLength = (DWORD)ReadOffset(pDBR,0x20,4);

			if (fileSystem == FileSystem_FAT32)
			{
				fatInfo.dwFATLength = (DWORD)ReadOffset(pDBR,0x24,4);
				fatInfo.byFATBit = 32;
			}
			else
			{
				fatInfo.dwFATLength = (DWORD)ReadOffset(pDBR,0x16,2);
				fatInfo.wSectorsOfRoot = (WORD)ReadOffset(pDBR,0x11,2) * 32 / fatInfo.wBytesPerSector;

				if (fileSystem == FileSystem_FAT16)
				{
					fatInfo.byFATBit = 16;
				}
				else
				{
					fatInfo.byFATBit = 12;
				}
			}

			// ������Ч��
			DWORD dwFATLen = fatInfo.dwFATLength * fatInfo.wBytesPerSector;
			ULONGLONG ullFatStartSectors = fatInfo.ullStartSector + fatInfo.wReserverSectors;
			BYTE *pFATByte = new BYTE[dwFATLen];
			ZeroMemory(pFATByte,dwFATLen);
			if (!ReadSectors(m_hMaster,ullFatStartSectors,fatInfo.dwFATLength,fatInfo.wBytesPerSector,pFATByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read FAT Table error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pFATByte;
				pFATByte = NULL;
				return FALSE;
			}

			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = fatInfo.ullStartSector;
			effData.ullSectors = fatInfo.wReserverSectors + fatInfo.byNumsOfFAT * fatInfo.dwFATLength + fatInfo.wSectorsOfRoot;
			effData.wBytesPerSector = fatInfo.wBytesPerSector;

			// ����ʼ�������ݵ����ֿ���Ϊ������д��һ�������ݣ���ֹ�ļ�ϵͳ������ϵͳʶ����ܾ�д����
			m_EffList.AddTail(effData);
			effData.ullStartSector = fatInfo.ullStartSector + effData.ullSectors;
			effData.ullSectors = 0;
			effData.wBytesPerSector = fatInfo.wBytesPerSector;


			DWORD dwNumsOfCluster = dwFATLen * 8 / fatInfo.byFATBit;
			for (DWORD dwStartCluser = 2;dwStartCluser < dwNumsOfCluster;dwStartCluser++)
			{
				if (effData.ullStartSector > m_ullSectorNums)
				{
					break;
				}

				if (IsFATValidCluster(pFATByte,dwStartCluser,fatInfo.byFATBit))
				{
					effData.ullSectors += fatInfo.bySectorsPerCluster;

					// �Ƿ񳬹���������
					if (effData.ullStartSector + effData.ullSectors > m_ullSectorNums)
					{
						effData.ullSectors = m_ullSectorNums - effData.ullStartSector;
					}

				}
				else
				{
					if (effData.ullSectors != 0)
					{
						m_EffList.AddTail(effData);
					}

					effData.ullStartSector += (effData.ullSectors + fatInfo.bySectorsPerCluster);
					effData.ullSectors = 0;
				}
			}

			delete []pFATByte;
			pFATByte = NULL;

		}
		break;


	case FileSystem_EXFAT:
		{
			EXFAT_INFO exfatInfo;
			ZeroMemory(&exfatInfo,sizeof(EXFAT_INFO));

			exfatInfo.ullPartitionOffset = ReadOffset(pDBR,0x40,8);
			exfatInfo.ullVolumeLength = ReadOffset(pDBR,0x48,8);
			exfatInfo.dwFatOffset = (DWORD)ReadOffset(pDBR,0x50,4);
			exfatInfo.dwFatLength = (DWORD)ReadOffset(pDBR,0x54,4);
			exfatInfo.dwClusterHeapOffset = (DWORD)ReadOffset(pDBR,0x58,4);
			exfatInfo.dwClusterCount = (DWORD)ReadOffset(pDBR,0x5C,4);
			exfatInfo.dwRootDirectoryCluster = (DWORD)ReadOffset(pDBR,0x60,4);
			exfatInfo.wBytesPerSector = (WORD)pow(2,(double)ReadOffset(pDBR,0x6C,1));
			exfatInfo.dwSectorsPerCluster = (DWORD)pow(2,(double)ReadOffset(pDBR,0x6D,1));
			exfatInfo.byNumsOfFat = (BYTE)ReadOffset(pDBR,0x6E,1);

			// ��һ�����ҵ���Ŀ¼
			// ͨ��ClusterHeapOffset���Ի��2�Ŵص���ʼ������Ȼ��ͨ��RootDirectoryCluster�����ɻ�ø�Ŀ¼����ʼ����
			DWORD dwRootDirectoryStartSector = exfatInfo.dwClusterHeapOffset 
				+ (exfatInfo.dwRootDirectoryCluster - 2) * exfatInfo.dwSectorsPerCluster;
			BYTE *pRoot = new BYTE[exfatInfo.wBytesPerSector];
			ZeroMemory(pRoot,exfatInfo.wBytesPerSector);

			ULONGLONG ullTempStartSector = exfatInfo.ullPartitionOffset + dwRootDirectoryStartSector;
			if (!ReadSectors(m_hMaster,ullTempStartSector,1,exfatInfo.wBytesPerSector,pRoot,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %d,Disk %d - Read EXFAT Root Directory error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pRoot;
				pRoot = NULL;
				return FALSE;
			}

			// �ڶ��������BitMap��ʼ����
			DWORD dwDirectoryItems =  exfatInfo.wBytesPerSector / 0x20;
			DWORD dwBitMapStartSectors = 0;
			DWORD dwBitMapLength = 0;
			for (DWORD item = 0,offset = 0;item < dwDirectoryItems;item++,offset += 0x20)
			{
				BYTE byEntryType = (BYTE)ReadOffset(pRoot,offset,1);
				if (byEntryType == 0x81)
				{
					DWORD dwFirstCluster = (DWORD)ReadOffset(pRoot,offset+0x14,4);
					dwBitMapLength = (DWORD)ReadOffset(pRoot,offset+0x18,4);
					dwBitMapStartSectors = (dwFirstCluster - 2) * exfatInfo.dwSectorsPerCluster + exfatInfo.dwClusterHeapOffset;
					break;
				}
			}

			delete []pRoot;
			pRoot = NULL;

			BYTE *pBitMap = new BYTE[dwBitMapLength];
			ZeroMemory(pBitMap,dwBitMapLength);

			ullTempStartSector = exfatInfo.ullPartitionOffset + dwBitMapStartSectors;
			if (!ReadSectors(m_hMaster,ullTempStartSector,dwBitMapLength/exfatInfo.wBytesPerSector,exfatInfo.wBytesPerSector,pBitMap,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read EXFAT Bitmap error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pBitMap;
				pBitMap = NULL;
				return FALSE;
			}

			// ��������������Щ����Ч��,ע���2�Ŵؿ�ʼ
			// ��2�Ŵ�֮ǰ��������ӽ���Ч�����б�
			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = exfatInfo.ullPartitionOffset;
			effData.ullSectors = exfatInfo.dwClusterHeapOffset;
			effData.wBytesPerSector = exfatInfo.wBytesPerSector;

			m_EffList.AddTail(effData);

			effData.ullStartSector += effData.ullSectors;
			effData.ullSectors = 0;

			for (DWORD cluster = 0;cluster < dwBitMapLength * 8;cluster++)
			{
				if (effData.ullStartSector > m_ullSectorNums)
				{
					break;
				}

				// EXFAT ��NTFS����һ���ж���Ч�صķ���
				if (IsNTFSValidCluster(pBitMap,cluster))
				{
					effData.ullSectors += exfatInfo.dwSectorsPerCluster;

					if (effData.ullStartSector + effData.ullSectors > m_ullSectorNums)
					{
						effData.ullSectors = m_ullSectorNums - effData.ullStartSector;
					}
				}
				else
				{
					if (effData.ullSectors != 0)
					{
						m_EffList.AddTail(effData);
					}

					effData.ullStartSector += (effData.ullSectors + exfatInfo.dwSectorsPerCluster);
					effData.ullSectors = 0;

				}
			}

			delete []pBitMap;
			pBitMap = NULL;

		}
		break;

	case FileSystem_NTFS:
		{
			NTFS_INFO ntfsInfo;
			ZeroMemory(&ntfsInfo,sizeof(NTFS_INFO));

			ntfsInfo.wBytesPerSectors = (WORD)ReadOffset(pDBR,0x0B,2);
			ntfsInfo.bySectorsPerCluster = (BYTE)ReadOffset(pDBR,0x0D,1);
			ntfsInfo.ullVolumeLength = ReadOffset(pDBR,0x28,8);
			ntfsInfo.ullStartCluserOfMFT = ReadOffset(pDBR,0x30,8);
			ntfsInfo.byMFTSize = (BYTE)ReadOffset(pDBR,0x40,1);
			ntfsInfo.ullStartSector = ullStartSector;

			// ������Чcu
			// Step1.�ҵ�6��MFT���BITMAP�ÿ��MFT��ռ1024���ֽ�
			ULONGLONG ullBitMapMFTStartSector = ntfsInfo.ullStartSector + ntfsInfo.ullStartCluserOfMFT * ntfsInfo.bySectorsPerCluster
				+ 1024 / ntfsInfo.wBytesPerSectors * 6;

			// Step2 ��BITMAP��DATA���ԣ�����������Ϊ0x80
			BYTE *pMFT = new BYTE[1024];
			ZeroMemory(pMFT,1024);
			if (!ReadSectors(m_hMaster,ullBitMapMFTStartSector,1024/ntfsInfo.wBytesPerSectors,ntfsInfo.wBytesPerSectors,pMFT,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read Bitmmap MFT error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pMFT;
				pMFT = NULL;
				return FALSE;
			}

			ULONGLONG ullBitMapDataStartCluster = 0;
			DWORD dwBitMapDataClusters = 0;
			WORD wFirstAttributeOffset = (WORD)ReadOffset(pMFT,0x14,2);
			DWORD dwAttributeType = 0;;
			DWORD dwAttributeLength = 0;
			BYTE  byNonResident = 0;
			DWORD dwAttributeOffset = wFirstAttributeOffset;
			for (int i = 0; i < 128;i++)
			{
				dwAttributeType = (DWORD)ReadOffset(pMFT,dwAttributeOffset,4);
				dwAttributeLength = (DWORD)ReadOffset(pMFT,dwAttributeOffset + 0x04,4);
				byNonResident = (BYTE)ReadOffset(pMFT,dwAttributeOffset + 0x08,1);
				if (dwAttributeType == 0x80 && byNonResident == 1)
				{
					WORD wDataRunOffset = (WORD)ReadOffset(pMFT,dwAttributeOffset + 0x20,2);

					// ��һ���֣���һ���ֽڣ�����λ��ʾ������ʼλ�õ��ֽ�������ʼ�غţ�������λ��ʾ�������ȵ��ֽ�����������
					// �ڶ����֣����������Ĵ���
					// �������֣�������ʼ�غ�
					BYTE byPartOne = (BYTE)ReadOffset(pMFT,dwAttributeOffset + wDataRunOffset,1);
					BYTE low=0,high=0;
					low = byPartOne & 0x0F;
					high = (byPartOne >> 4) & 0x0F;

					DWORD dwPartTwo = (DWORD)ReadOffset(pMFT,dwAttributeOffset + wDataRunOffset + 1,low);
					ULONGLONG ullPartThree = ReadOffset(pMFT,dwAttributeOffset + wDataRunOffset + 1 + low,high);

					dwBitMapDataClusters= dwPartTwo;
					ullBitMapDataStartCluster = ullPartThree;

					break;
				}

				dwAttributeOffset += dwAttributeLength;
			}
			delete []pMFT;
			pMFT = NULL;

			// Step3 �ҵ�Bitmapλ�ã�ȷ����Щ����Ч��
			ULONGLONG ullBitMapDataStartSector = ntfsInfo.ullStartSector + ullBitMapDataStartCluster * ntfsInfo.bySectorsPerCluster;
			DWORD dwLengthPerCluster = ntfsInfo.bySectorsPerCluster * ntfsInfo.wBytesPerSectors;

			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = ntfsInfo.ullStartSector;
			effData.ullSectors = ntfsInfo.bySectorsPerCluster;
			effData.wBytesPerSector = ntfsInfo.wBytesPerSectors;

			// 0�Ŵص������
			m_EffList.AddTail(effData);

			effData.ullStartSector = ntfsInfo.ullStartSector + ntfsInfo.bySectorsPerCluster;
			effData.ullSectors = 0;

			BYTE *pBitmap = new BYTE[dwLengthPerCluster];

			ULONGLONG ullClusterIndex = 0;
			for (DWORD i = 0;i < dwBitMapDataClusters;i++)
			{
				ZeroMemory(pBitmap,dwLengthPerCluster);
				ULONGLONG ullTempStart = ullBitMapDataStartSector + i * ntfsInfo.bySectorsPerCluster;
				if (!ReadSectors(m_hMaster,ullTempStart,ntfsInfo.bySectorsPerCluster,ntfsInfo.wBytesPerSectors,pBitmap,m_MasterPort->GetOverlapped(),&dwErrorCode))
				{
					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read Bitmap data error,system errorcode=%ld,%s")
						,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					delete []pBitmap;
					pBitmap = NULL;
					return FALSE;
				}

				for (DWORD cluster = 0;cluster < dwLengthPerCluster * 8;cluster++)
				{
					// 0�Ŵ��Ѿ����
					if (ullClusterIndex == 0)
					{
						ullClusterIndex++;
						continue;
					}

					if (effData.ullStartSector > m_ullSectorNums)
					{
						break;
					}

					if (IsNTFSValidCluster(pBitmap,cluster))
					{
						effData.ullSectors += ntfsInfo.bySectorsPerCluster;
						if (effData.ullStartSector + effData.ullSectors > m_ullSectorNums)
						{
							effData.ullSectors = m_ullSectorNums - effData.ullStartSector;
						}
					}
					else
					{
						if (effData.ullSectors != 0)
						{
							*m_EffList.AddTail(effData);
						}

						effData.ullStartSector += (effData.ullSectors + ntfsInfo.bySectorsPerCluster);
						effData.ullSectors = 0;
					}

					ullClusterIndex++;
				}


			}

			delete []pBitmap;
			pBitmap = NULL;

		}
		break;

	case FileSystem_EXT_X:
		{
			ULONGLONG ullTempStartSector = ullStartSector + 1024/m_dwBytesPerSector;
			BYTE *pSuperBlock = new BYTE[m_dwBytesPerSector];
			ZeroMemory(pSuperBlock,m_dwBytesPerSector);

			if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pSuperBlock,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read Super Block error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));


				delete []pSuperBlock;
				pSuperBlock = NULL;

				return FALSE;
			}

			SUPER_BLOCK_INFO superBlockInfo;
			ZeroMemory(&superBlockInfo,sizeof(SUPER_BLOCK_INFO));
			superBlockInfo.dwBlocksOfVolume = (DWORD)ReadOffset(pSuperBlock,0x04,4);
			superBlockInfo.dwStartBlockOfGroup0 = (DWORD)ReadOffset(pSuperBlock,0x14,4);
			superBlockInfo.dwBytesPerBlock = (1024 << (DWORD)ReadOffset(pSuperBlock,0x18,4));
			superBlockInfo.dwBlocksPerBlockGroup = (DWORD)ReadOffset(pSuperBlock,0x20,4);
			superBlockInfo.dwNodesPerBlockGroup = (DWORD)ReadOffset(pSuperBlock,0x28,4);
			superBlockInfo.dwSectorsPerBlock = superBlockInfo.dwBytesPerBlock / m_dwBytesPerSector;

			delete []pSuperBlock;
			pSuperBlock = NULL;

			// ��ӳ�����֮ǰ�Ŀ鵽��Ч�����б�
			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = ullStartSector;
			effData.ullSectors = superBlockInfo.dwStartBlockOfGroup0 * superBlockInfo.dwSectorsPerBlock;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			if (effData.ullSectors != 0)
			{
				m_EffList.AddTail(effData);
			}

			// ��������
			DWORD dwTotalGroupBlocks = superBlockInfo.dwBlocksOfVolume / superBlockInfo.dwBlocksPerBlockGroup;
			if (superBlockInfo.dwBlocksOfVolume % superBlockInfo.dwBlocksPerBlockGroup)
			{
				dwTotalGroupBlocks++;
			}

			// ��λ�������������

			// ������Ŀ��
			DWORD dwSuperBlockNum = 1;
			if (superBlockInfo.dwBytesPerBlock / 1024 > 1)
			{
				dwSuperBlockNum = 0;
			}

			// ��������Ŀ�ţ�����������ʼ�ڳ��������ڿ����һ��
			DWORD dwGroupDescriptionBlockNum = dwSuperBlockNum + 1;
			DWORD dwGroupDescriptionStartSector = dwGroupDescriptionBlockNum * superBlockInfo.dwSectorsPerBlock;

			// ����������ռN�Ŀ飬����һ���飬���ݿ����С���� 2014-06-27 Binggoo

			DWORD dwGroupDescriptionBlocks = dwTotalGroupBlocks * 0x20 / superBlockInfo.dwBytesPerBlock;
			if (dwTotalGroupBlocks * 0x20 % superBlockInfo.dwBytesPerBlock != 0)
			{
				dwGroupDescriptionBlocks++;
			}

			DWORD dwGroupDescriptionLen = dwGroupDescriptionBlocks * superBlockInfo.dwBytesPerBlock;
			BYTE *pGroupDescrption = new BYTE[dwGroupDescriptionLen];
			ZeroMemory(pGroupDescrption,dwGroupDescriptionLen);

			ullTempStartSector = ullStartSector + dwGroupDescriptionStartSector;
			if (!ReadSectors(m_hMaster,ullTempStartSector,dwGroupDescriptionBlocks * superBlockInfo.dwSectorsPerBlock,m_dwBytesPerSector,pGroupDescrption,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read Group Block Description error,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pGroupDescrption;
				pGroupDescrption = NULL;

				return FALSE;
			}

			DWORD dwBlocksIndex = superBlockInfo.dwStartBlockOfGroup0;

			for (DWORD group=0,offset=0;group < dwTotalGroupBlocks;group++,offset += 0x20)
			{
				// ��λͼ����ʼλ�úʹ�С
				DWORD dwBlockBitMapStartBlock = (DWORD)ReadOffset(pGroupDescrption,offset,4);
				DWORD dwBlockBitMapStartSector = dwBlockBitMapStartBlock * superBlockInfo.dwSectorsPerBlock;
				DWORD dwBlockBitMapLength = superBlockInfo.dwBlocksPerBlockGroup / 8;

				BYTE *pBitMap = new BYTE[dwBlockBitMapLength];
				ZeroMemory(pBitMap,dwBlockBitMapLength);

				ullTempStartSector = ullStartSector + dwBlockBitMapStartSector;
				if (!ReadSectors(m_hMaster,ullTempStartSector,dwBlockBitMapLength/m_dwBytesPerSector,m_dwBytesPerSector,pBitMap,m_MasterPort->GetOverlapped(),&dwErrorCode))
				{
					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Read Block Bitmap error,system errorcode=%ld,%s")
						,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					delete []pBitMap;
					pBitMap = NULL;

					delete []pGroupDescrption;
					pGroupDescrption = NULL;

					return FALSE;

				}

				// ��λͼֻ�����������ڵĿ�ķ������
				effData.ullStartSector = ullStartSector 
					+ superBlockInfo.dwStartBlockOfGroup0 * superBlockInfo.dwSectorsPerBlock 
					+ group * superBlockInfo.dwBlocksPerBlockGroup * superBlockInfo.dwSectorsPerBlock;
				effData.ullSectors = 0;

				// �ж���Ч��
				for (DWORD block = 0;block < superBlockInfo.dwBlocksPerBlockGroup;block++)
				{
					// ���һ������Ŀ����п���С��ʵ��ÿ�����Ŀ���
					if (dwBlocksIndex > superBlockInfo.dwBlocksOfVolume - 1)
					{
						break;
					}

					if (effData.ullStartSector > m_ullSectorNums)
					{
						break;
					}

					if (IsExtXValidBlock(pBitMap,block))
					{
						effData.ullSectors += superBlockInfo.dwSectorsPerBlock;

						if (effData.ullStartSector + effData.ullSectors > m_ullSectorNums)
						{
							effData.ullSectors = m_ullSectorNums - effData.ullStartSector;
						}
					}
					else
					{
						if (effData.ullSectors != 0)
						{
							m_EffList.AddTail(effData);
						}

						effData.ullStartSector += (effData.ullSectors + superBlockInfo.dwSectorsPerBlock);
						effData.ullSectors = 0;
					}

					dwBlocksIndex++;
				}

				delete []pBitMap;
				pBitMap = NULL;

			}

			delete []pGroupDescrption;
			pGroupDescrption = NULL;

		}
		break;

	case FileSystem_EXTEND:
		{

			EFF_DATA headData;
			headData.ullStartSector = ullStartSector;
			headData.ullSectors = 63;
			headData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(headData);

			MASTER_BOOT_RECORD ebr;
			ZeroMemory(&ebr,sizeof(MASTER_BOOT_RECORD));
			memcpy(&ebr,pDBR + 0x1BE,sizeof(MASTER_BOOT_RECORD));

			CString strPartions,strByte;

			for (int row = 0;row < 4;row++)
			{
				for (int col =0;col < 16;col++)
				{
					int offset = 0x1BE + row * 16 + col;
					strByte.Format(_T("%02X "),pDBR[offset]);
					strPartions += strByte;
				}
				strPartions += _T("\r\n");
			}

			CUtils::WriteLogFile(m_hLogFile,FALSE,strPartions);

			for (int i = 0; i < 4;i++)
			{
				if (ebr.Partition[i].StartLBA == 0)
				{
					continue;
				}
				BYTE *pDBRTemp = new BYTE[m_dwBytesPerSector];
				ZeroMemory(pDBRTemp,m_dwBytesPerSector);

				ULONGLONG ullTempStartSector = ebr.Partition[i].StartLBA;

				if (ebr.Partition[i].PartitionType == 0x0F) // ����չ����
				{ 
					ullMasterSectorOffset = ullTempStartSector;
				}
				else if (ebr.Partition[i].PartitionType == 0x05) // ������չ����
				{
					ullTempStartSector += ullMasterSectorOffset;
				}
				else
				{
					ullTempStartSector += ullStartSector;
				}

				if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pDBRTemp,m_MasterPort->GetOverlapped(),&dwErrorCode))
				{
					m_MasterPort->SetEndTime(CTime::GetCurrentTime());
					m_MasterPort->SetPortState(PortState_Fail);
					m_MasterPort->SetResult(FALSE);
					m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

					CString strErrMsg;
					strErrMsg.Format(_T("Partition=%d,FileSystem=%02X,StartLBA=0x%X,Sectors=0x%X - Read DBR Error"),
						i+1,ebr.Partition[i].PartitionType,ebr.Partition[i].StartLBA,ebr.Partition[i].TotalSector);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,system errorcode=%ld,%s")
						,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),strErrMsg,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

					delete []pDBRTemp;
					pDBRTemp = NULL;

					return FALSE;
				}

				if (ebr.Partition[i].PartitionType == 0x05 || ebr.Partition[i].PartitionType == 0x0F) // ��չ����
				{
					fileSystem = FileSystem_EXTEND;
				}
				else
				{
					fileSystem = GetFileSystem(pDBRTemp,ullTempStartSector);
				}

				if (!AppendEffDataList(pDBRTemp,fileSystem,ullTempStartSector,ullMasterSectorOffset,
					ebr.Partition[i].TotalSector))
				{
					delete []pDBRTemp;
					pDBRTemp = NULL;

					return FALSE;
				}

				delete []pDBRTemp;
				pDBRTemp = NULL;
			}
		}
		break;

	case FileSystem_UNKNOWN:
		{
			// ��ʶ����ļ�ϵͳ��������������
			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = ullStartSector;
			effData.ullSectors = ullSectors;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(effData);
		}
		break;
	}

	return TRUE;
}

ULONGLONG CDisk::ReadOffset( const PBYTE pByte,DWORD offset,BYTE bytes )
{
	ASSERT(bytes <= 8 && bytes > 0);
	ULONGLONG ullMask = 0xFFFFFFFFFFFFFFFF;
	ullMask = (ullMask >> (sizeof(ULONGLONG) - sizeof(BYTE) * bytes) * 8);

	ULONGLONG ullValue = *(PULONGLONG)(pByte+offset) & ullMask;
	return ullValue;
}

BOOL CDisk::IsFATValidCluster( const PBYTE pByte,DWORD cluster,BYTE byFATBit )
{
	// byFAT = 12/16/32;
	// cluster begin 2
	// ���ر�־ FAT12 - 0xFF7, FAT16 - 0xFFF7, FAT32 - 0x0FFFFFF7
	if (byFATBit == 32)
	{
		DWORD dwValue = (DWORD)ReadOffset(pByte,cluster * 4,4);

		return (dwValue != 0 && dwValue != 0x0FFFFFF7);
	}
	else if (byFATBit == 16)
	{
		WORD wValue = (WORD)ReadOffset(pByte,cluster * 2,2);

		return (wValue != 0 && wValue != 0xFFF7);
	}
	else
	{
		DWORD offset = cluster * 3 / 2;
		WORD wValue = 0;
		if (cluster * 3 % 2 == 0)
		{
			wValue = (WORD)ReadOffset(pByte,offset,2);
		}
		else
		{
			wValue = (WORD)ReadOffset(pByte,offset,2);
		}

		return (wValue != 0 && wValue != 0xFF7);
	}
}

BOOL CDisk::IsNTFSValidCluster( const PBYTE pByte,DWORD cluster )
{
	DWORD dwByteIndex = cluster/8;
	BYTE  bitIndex = cluster % 8;
	BYTE byValue = (BYTE)ReadOffset(pByte,dwByteIndex,1);

	return ((byValue >> bitIndex) & 0x1);
}

BOOL CDisk::IsExtXValidBlock( const PBYTE pByte,DWORD block )
{
	DWORD dwByteIndex = block/8;
	BYTE  bitIndex = block % 8;
	BYTE byValue = (BYTE)ReadOffset(pByte,dwByteIndex,1);

	return ((byValue >> bitIndex) & 0x1);
}

ULONGLONG CDisk::GetValidSize()
{
	ULONGLONG ullValidSize = 0;

	POSITION pos = m_EffList.GetHeadPosition();
	while (pos)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);
		ullValidSize += effData.ullSectors * effData.wBytesPerSector;
	}

	return ullValidSize;
}

void CDisk::SetValidSize( ULONGLONG ullSize )
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

bool CDisk::IsAllFailed(ErrorType &errType,PDWORD pdwErrorCode)
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

BOOL CDisk::OnCopyDisk()
{
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			EFF_DATA effData;
			effData.ullStartSector = 0;
			effData.ullSectors = BUF_SECTORS;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(effData);

			effData.ullStartSector = BUF_SECTORS;
			effData.ullSectors = m_ullSectorNums - BUF_SECTORS;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(effData);

			m_ullValidSize = GetValidSize();
			SetValidSize(m_ullValidSize);
		}

		break;

	case WorkMode_QuickCopy:
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze start......"));
			if (!BriefAnalyze())
			{
				return FALSE;
			}

			m_ullValidSize = GetValidSize();
			SetValidSize(m_ullValidSize);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze end, valid data size=%I64d"),m_ullValidSize);

		}
		break;
	}

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Valid Data List:"));
	CString strList;
	POSITION pos = m_EffList.GetHeadPosition();
	while (pos)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);
		strList.Format(_T("StartSector:%I64d  Sectors:%I64d  BytesPerSector:%d"),effData.ullStartSector,effData.ullSectors,effData.wBytesPerSector);
		CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
	}

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	UINT nCount = 0;
	HANDLE *hWriteThreads = new HANDLE[m_nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		if (port->IsConnected())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteDiskThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateWriteDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//�ȴ�д�����߳̽���
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
	}
	delete []hWriteThreads;

	//�ȴ��������߳̽���
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bHashVerify)
	{
		char *buf = "Verify";
		PostMessage(m_hWnd,WM_SEND_FUNCTION_TEXT,(WPARAM)buf,0);

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
		while (pos)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
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

				hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyThreadProc,lpVoid,0,&dwVerifyId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

				nCount++;
			}
		}

		//�ȴ�У���߳̽���
		WaitForMultipleObjects(nCount,hVerifyThreads,TRUE,INFINITE);

		bResult = FALSE;
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hVerifyThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hVerifyThreads[i]);
		}
		delete []hVerifyThreads;
	}

	return bResult;
}

BOOL CDisk::OnCopyImage()
{
	IMAGE_HEADER imgHead = {0};
	DWORD dwErrorCode = 0;
	DWORD dwLen = SIZEOF_IMAGE_HEADER;
	if (!ReadFileAsyn(m_hMaster,0,dwLen,(LPBYTE)&imgHead,m_MasterPort->GetOverlapped(),&dwErrorCode))
	{
		m_MasterPort->SetEndTime(CTime::GetCurrentTime());
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetResult(FALSE);
		m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read Image Head error,filename=%s,system errorcode=%ld,%s")
			,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	m_ullValidSize = imgHead.ullValidSize;
	HashMethod hashMethod = (HashMethod)imgHead.dwHashType;

	memcpy(m_ImageHash,imgHead.byImageDigest,imgHead.dwHashLen);

	SetValidSize(imgHead.ullValidSize);
	SetHashMethod(m_bComputeHash,m_bHashVerify,hashMethod);

	m_MasterPort->SetHashMethod(hashMethod);

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwUncompressId;

	HANDLE hReadThread = CreateThread(NULL,0,ReadImageThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image(%s) - CreateReadImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetFileName(),dwReadId,hReadThread);

	// ���������ѹ���߳�
	HANDLE *hUncompressThreads = new HANDLE[COMPRESS_THREAD_COUNT];

	for (UINT i = 0;i < COMPRESS_THREAD_COUNT;i++)
	{
		hUncompressThreads[i] = CreateThread(NULL,0,UnCompressThreadProc,this,0,&dwUncompressId);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress Image(%02d) - CreateUncompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,i,dwUncompressId,hUncompressThreads[i]);
	}

	UINT nCount = 0;
	HANDLE *hWriteThreads = new HANDLE[m_nCurrentTargetCount];

	POSITION pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		if (port->IsConnected())
		{
			port->SetHashMethod(hashMethod);

			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteDiskThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateWriteDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	// �ȴ�д�����߳̽���
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
	}
	delete []hWriteThreads;

	// �ȴ���ѹ���߳̽���
	WaitForMultipleObjects(COMPRESS_THREAD_COUNT,hUncompressThreads,TRUE,INFINITE);
	for (UINT i = 0;i < COMPRESS_THREAD_COUNT;i++)
	{
		CloseHandle(hUncompressThreads[i]);
	}
	delete []hUncompressThreads;

	//�ȴ��������߳̽���
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bHashVerify)
	{
		char *buf = "Verify";
		PostMessage(m_hWnd,WM_SEND_FUNCTION_TEXT,(WPARAM)buf,0);

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
		while (pos)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
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

				hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyThreadProc,lpVoid,0,&dwVerifyId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

				nCount++;
			}
		}

		// �ȴ�У���߳̽���
		WaitForMultipleObjects(nCount,hVerifyThreads,TRUE,INFINITE);
		bResult = FALSE;
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hVerifyThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hVerifyThreads[i]);
		}
		delete []hVerifyThreads;
	}

	return bResult;

}

BOOL CDisk::OnMakeImage()
{
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze start......"));
	if (!BriefAnalyze())
	{
		return FALSE;
	}

	m_ullValidSize = GetValidSize();
	SetValidSize(m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze end, valid data size=%I64d"),m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Valid Data List:"));
	CString strList;
	POSITION pos = m_EffList.GetHeadPosition();
	while (pos)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);
		strList.Format(_T("StartSector:%I64d  Sectors:%I64d  BytesPerSector:%d"),effData.ullStartSector,effData.ullSectors,effData.wBytesPerSector);
		CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
	}

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwCompressId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image Make(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	HANDLE *hCompressThreads = new HANDLE[COMPRESS_THREAD_COUNT];
	for (UINT i = 0; i < COMPRESS_THREAD_COUNT;i++)
	{
		hCompressThreads[i] = CreateThread(NULL,0,CompressThreadProc,this,0,&dwCompressId);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress Image(%02d) - CreateCompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,i,dwCompressId,hCompressThreads[i]);
	}

	UINT nCount = 0;
	HANDLE *hWriteThreads = new HANDLE[m_nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		if (port->IsConnected())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteImageThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image Make - CreateWriteImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//�ȴ�дӳ���߳̽���
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
	}
	delete []hWriteThreads;

	//�ȴ�ѹ���߳̽���
	WaitForMultipleObjects(COMPRESS_THREAD_COUNT,hCompressThreads,TRUE,INFINITE);
	for (UINT i = 0; i < COMPRESS_THREAD_COUNT;i++)
	{
		CloseHandle(hCompressThreads[i]);
	}
	delete []hCompressThreads;

	//�ȴ��������߳̽���
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	return bResult;
}

BOOL CDisk::OnCompareDisk()
{
	switch (m_CompareMode)
	{
	case CompareMode_Full:
		{
			EFF_DATA effData;
			effData.ullStartSector = 0;
			effData.ullSectors = BUF_SECTORS;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(effData);

			effData.ullStartSector = BUF_SECTORS;
			effData.ullSectors = m_ullSectorNums - BUF_SECTORS;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			m_EffList.AddTail(effData);

			m_ullValidSize = GetValidSize();
			SetValidSize(m_ullValidSize);
		}

		break;

	case CompareMode_Quick:
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze start......"));
			if (!BriefAnalyze())
			{
				return FALSE;
			}

			m_ullValidSize = GetValidSize();
			SetValidSize(m_ullValidSize);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Analyze end, valid data size=%I64d"),m_ullValidSize);

		}
		break;
	}

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Valid Data List:"));
	CString strList;
	POSITION pos = m_EffList.GetHeadPosition();
	while (pos)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);
		strList.Format(_T("StartSector:%I64d  Sectors:%I64d  BytesPerSector:%d"),effData.ullStartSector,effData.ullSectors,effData.wBytesPerSector);
		CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
	}

	BOOL bResult = FALSE;
	DWORD dwVerifyId,dwErrorCode;

	//ĸ�̶��߳�
	LPVOID_PARM lpMasterVoid = new VOID_PARM;
	lpMasterVoid->lpVoid1 = this;
	lpMasterVoid->lpVoid2 = m_MasterPort;
	lpMasterVoid->lpVoid3 = m_pMasterHashMethod;

	HANDLE hMasterVerifyThread = CreateThread(NULL,0,VerifyThreadProc,lpMasterVoid,0,&dwVerifyId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwVerifyId,hMasterVerifyThread);

	//���̶��߳�
	HANDLE *hVerifyThreads = new HANDLE[m_nCurrentTargetCount];
	UINT nCount = 0;
	pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected())
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

			hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyThreadProc,lpVoid,0,&dwVerifyId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

			nCount++;
		}
	}

	//�ȴ�У���߳̽���
	WaitForMultipleObjects(nCount,hVerifyThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hVerifyThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hVerifyThreads[i]);
	}
	delete []hVerifyThreads;

	//�ȴ�ĸ�̶��߳̽���
	WaitForSingleObject(hMasterVerifyThread,INFINITE);
	GetExitCodeThread(hMasterVerifyThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hMasterVerifyThread);

	return bResult;
}

BOOL CDisk::OnCleanDisk()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;

	//����д�߳�
	HANDLE *hWriteThreads = new HANDLE[m_nCurrentTargetCount];
	UINT nCount = 0;
	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;

			hWriteThreads[nCount] = CreateThread(NULL,0,CleanDiskThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateCleanDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//�ȴ�У���߳̽���
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
	}
	delete []hWriteThreads;

	return bResult;
}

BOOL CDisk::ReadDisk()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullReadSectors = 0;
	ULONGLONG ullRemainSectors = 0;
	ULONGLONG ullStartSectors = 0;
	DWORD dwSectors = BUF_SECTORS;
	DWORD dwLen = BUF_SECTORS * m_dwBytesPerSector;

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	POSITION pos = m_EffList.GetHeadPosition();

	// ��һ�������������д
	EFF_DATA headData;
	if (pos)
	{
		headData = m_EffList.GetNext(pos);
	}

	while (pos && !*m_lpCancel && bResult && m_MasterPort->GetPortState() == PortState_Active)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);

		ullReadSectors = 0;
		ullRemainSectors = 0;
		ullStartSectors = effData.ullStartSector;
		dwSectors = BUF_SECTORS;
		dwLen = BUF_SECTORS * m_dwBytesPerSector;

		while (bResult && !*m_lpCancel && ullReadSectors < effData.ullSectors && ullStartSectors < m_ullSectorNums)
		{
			QueryPerformanceCounter(&t0);
			// �ж϶����Ƿ�ﵽ����ֵ
			while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE) && !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
			{
				Sleep(0);
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

			ullRemainSectors = effData.ullSectors - ullReadSectors;

			if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
			{
				ullRemainSectors = m_ullSectorNums - ullStartSectors;
			}

			if (ullRemainSectors < BUF_SECTORS)
			{
				dwSectors = (DWORD)ullRemainSectors;

				dwLen = dwSectors * effData.wBytesPerSector;
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadSectors(m_hMaster,ullStartSectors,dwSectors,effData.wBytesPerSector,pByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
			{
				bResult = FALSE;
				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			else
			{
				DATA_INFO dataInfo = {0};
				dataInfo.ullOffset = ullStartSectors * effData.wBytesPerSector;
				dataInfo.dwDataSize = dwLen;
				dataInfo.pData = new BYTE[dwLen];
				memcpy(dataInfo.pData,pByte,dwLen);

				if (m_WorkMode == WorkMode_ImageMake)
				{
					m_CompressQueue.AddTail(dataInfo);
				}
				else
				{
					AddDataQueueList(dataInfo);
					delete []dataInfo.pData;
				}

				if (m_bComputeHash)
				{
					m_pMasterHashMethod->update((void *)pByte,dwLen);
				}

				delete []pByte;

				ullStartSectors += dwSectors;
				ullReadSectors += dwSectors;

				QueryPerformanceCounter(&t2);

				dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
				dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��

				m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
				m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);
				m_MasterPort->AppendCompleteSize(dwLen);

				// 				TRACE1("ReadDisk NoWaitSpeed:%.1f MB/s",(double)dwLen / (dbTimeNoWait * 1024 * 1024));
				// 				TRACE2("ReadDisk WaitTimeSpeed:%.1f MB/s, WaitTime:%f ms",(double)dwLen / (dbTimeWait * 1024 * 1024),(dbTimeWait - dbTimeNoWait)*1000);
				TRACE1("ReadDisk NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
				TRACE1("ReadDisk WaitTime:%.1f ms",dbTimeWait * 1000);
			}

		}
	}

	// д��һ������
	ullReadSectors = 0;
	ullRemainSectors = 0;
	ullStartSectors = headData.ullStartSector;
	dwSectors = BUF_SECTORS;
	dwLen = BUF_SECTORS * m_dwBytesPerSector;

	while (bResult && !*m_lpCancel && ullReadSectors < headData.ullSectors && ullStartSectors < m_ullSectorNums)
	{
		//DWORD dwTimeWait = timeGetTime();
		QueryPerformanceCounter(&t0);
		// �ж϶����Ƿ�ﵽ����ֵ
		while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE) && !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
		{
			Sleep(0);
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

		ullRemainSectors = headData.ullSectors - ullReadSectors;

		if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
		{
			ullRemainSectors = m_ullSectorNums - ullStartSectors;
		}

		if (ullRemainSectors < BUF_SECTORS)
		{
			dwSectors = (DWORD)ullRemainSectors;

			dwLen = dwSectors * headData.wBytesPerSector;
		}

		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		QueryPerformanceCounter(&t1);
		if (!ReadSectors(m_hMaster,ullStartSectors,dwSectors,headData.wBytesPerSector,pByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;
			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}
		else
		{
			DATA_INFO dataInfo = {0};
			dataInfo.ullOffset = ullStartSectors * headData.wBytesPerSector;
			dataInfo.dwDataSize = dwLen;
			dataInfo.pData = pByte;

			AddDataQueueList(dataInfo);

			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)pByte,dwLen);
			}

			delete []pByte;

			ullStartSectors += dwSectors;
			ullReadSectors += dwSectors;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��
			m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
			m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);
			m_MasterPort->AppendCompleteSize(dwLen);

		}

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode);
	}

	// �������ݶ�������
	// �������ݶ�������
	if (bResult)
	{
		POSITION posQueue = m_DataQueueList.GetHeadPosition();
		POSITION posTarg = m_TargetPorts->GetHeadPosition();
		while (posQueue && posTarg)
		{
			CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
			CPort *port = m_TargetPorts->GetNext(posTarg);

			while (dataQueue->GetCount() > 0 && m_MasterPort->GetResult() && port->GetResult())
			{
				Sleep(100);
			}
		}
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

			CString strMasterHash;
			for (int i = 0; i < m_pMasterHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),m_pMasterHashMethod->digest()[i]);
				strMasterHash += strHash;
			}

			CString strHashMethod(m_pMasterHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),strHashMethod,strMasterHash);

		}
	}
	else
	{
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CDisk::WriteDisk( CPort *port, CDataQueue *pDataQueue)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	QuickClean(port,&dwErrorCode);

	HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hDisk == INVALID_HANDLE_VALUE)
	{
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_System,dwErrorCode);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenDisk error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	DWORD dwBytesPerSector = port->GetBytesPerSector();

	QueryPerformanceFrequency(&freq);

	port->SetStartTime(CTime::GetCurrentTime());
	while (!*m_lpCancel && m_MasterPort->GetResult() && bResult)
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			Sleep(0);
		}

		if (!m_MasterPort->GetResult())
		{
			errType = m_MasterPort->GetErrorCode(&dwErrorCode);
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

		DATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}

		ULONGLONG ullStartSectors = dataInfo.ullOffset / dwBytesPerSector;
		DWORD dwSectors = dataInfo.dwDataSize / dwBytesPerSector;

		QueryPerformanceCounter(&t1);
		if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytesPerSector,dataInfo.pData,port->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}
		else
		{
			QueryPerformanceCounter(&t2);

			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

			port->AppendUsedWaitTimeS(dbTimeWait);
			port->AppendUsedNoWaitTimeS(dbTimeNoWait);
			port->AppendCompleteSize(dataInfo.dwDataSize);

			delete []dataInfo.pData;

			// 			TRACE1("WriteDisk NoWaitSpeed:%.1f MB/s",(double)dataInfo.dwDataSize / (dbTimeNoWait * 1024 * 1024));
			// 			TRACE2("WriteDisk WaitSpeed %.1f MB/s, WaitTime:%f ms",(double)dataInfo.dwDataSize / (dbTimeWait * 1024 * 1024),(dbTimeWait - dbTimeNoWait) * 1000);
			TRACE1("WriteDisk NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
			TRACE1("WriteDisk WaitTime:%.1f ms",dbTimeWait * 1000);
		}

	}

	CloseHandle(hDisk);

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}

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

BOOL CDisk::QuickClean(CPort *port,PDWORD pdwErrorCode )
{
	CStringArray strVolumeArray;
	port->GetVolumeArray(strVolumeArray);

	BYTE *pByte = new BYTE[CLEAN_LENGTH];
	ZeroMemory(pByte,CLEAN_LENGTH);

	BOOL bResult = TRUE;
	for (int i = 0; i < strVolumeArray.GetCount();i++)
	{
		CString strVolume = strVolumeArray.GetAt(i);

		HANDLE hVolume = CreateFile(strVolume,                 // �豸·��
			GENERIC_READ | GENERIC_WRITE,                      // ��д��ʽ
			FILE_SHARE_READ | FILE_SHARE_WRITE,                // ����ʽ
			NULL,                                              // Ĭ�ϵİ�ȫ������
			OPEN_EXISTING,                                     // ������ʽ
			0,                                                 // ���������ļ�����
			NULL);                                             // �������ģ���ļ�

		if(INVALID_HANDLE_VALUE == hVolume)
		{
			continue;
		}

		if (!LockOnVolume(hVolume,pdwErrorCode))
		{
			bResult = FALSE;
			CloseHandle(hVolume);
			break;
		}

		if (!IsVolumeUnmount(hVolume))
		{
			if (!UnmountVolume(hVolume,pdwErrorCode))
			{
				bResult = FALSE;
				CloseHandle(hVolume);
				break;
			}
		}

		DWORD dwBytesPerSector = 0;
		ULONGLONG ullSectorNums = GetNumberOfSectors(hVolume,&dwBytesPerSector);
		DWORD dwSectors = CLEAN_LENGTH/dwBytesPerSector;
		DWORD dwSize = 0;

		if (!WriteSectors(hVolume,0,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(),pdwErrorCode))
		{
			bResult = FALSE;
			CloseHandle(hVolume);
			break;
		}

		if (!WriteSectors(hVolume,ullSectorNums-dwSectors,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(),pdwErrorCode))
		{
			bResult = FALSE;
			CloseHandle(hVolume);
			break;
		}

		//fresh the partition table
		BOOL result = DeviceIoControl(
			hVolume,
			IOCTL_DISK_UPDATE_PROPERTIES,
			NULL,
			0,
			NULL,
			0,
			&dwSize,
			NULL
			);
		if (!result)
		{
			bResult = FALSE;
			*pdwErrorCode = GetLastError();
			CloseHandle(hVolume);
			break;
		}

		CloseHandle(hVolume);
	}


	delete []pByte;

	bResult = DestroyDisk(port->GetDiskNum());

	return bResult;
}

BOOL CDisk::Verify( CPort *port,CHashMethod *pHashMethod )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullReadSectors = 0;
	ULONGLONG ullRemainSectors = 0;
	ULONGLONG ullStartSectors = 0;
	DWORD dwSectors = BUF_SECTORS;
	DWORD dwLen = BUF_SECTORS * m_dwBytesPerSector;

	HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hDisk == INVALID_HANDLE_VALUE)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_System,dwErrorCode);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenDisk error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	POSITION pos = m_EffList.GetHeadPosition();

	// ��һ�������������д
	EFF_DATA headData;
	if (pos)
	{
		headData = m_EffList.GetNext(pos);
	}

	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);

		ullReadSectors = 0;
		ullRemainSectors = 0;
		ullStartSectors = effData.ullStartSector;
		dwSectors = BUF_SECTORS;
		dwLen = BUF_SECTORS * m_dwBytesPerSector;

		while (bResult && !*m_lpCancel && ullReadSectors < effData.ullSectors && ullStartSectors < m_ullSectorNums)
		{
			QueryPerformanceCounter(&t0);

			ullRemainSectors = effData.ullSectors - ullReadSectors;

			if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
			{
				ullRemainSectors = m_ullSectorNums - ullStartSectors;
			}

			if (ullRemainSectors < BUF_SECTORS)
			{
				dwSectors = (DWORD)ullRemainSectors;

				dwLen = dwSectors * effData.wBytesPerSector;
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadSectors(hDisk,ullStartSectors,dwSectors,effData.wBytesPerSector,pByte,port->GetOverlapped(),&dwErrorCode))
			{
				bResult = FALSE;
				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			else
			{
				pHashMethod->update((void *)pByte,dwLen);	

				delete []pByte;

				ullStartSectors += dwSectors;
				ullReadSectors += dwSectors;

				QueryPerformanceCounter(&t2);

				dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
				dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��

				port->AppendUsedWaitTimeS(dbTimeWait);
				port->AppendUsedNoWaitTimeS(dbTimeNoWait);
				port->AppendCompleteSize(dwLen);

				// 				TRACE1("ReadDisk NoWaitSpeed:%.1f MB/s",(double)dwLen / (dbTimeNoWait * 1024 * 1024));
				// 				TRACE2("ReadDisk WaitTimeSpeed:%.1f MB/s, WaitTime:%f ms",(double)dwLen / (dbTimeWait * 1024 * 1024),(dbTimeWait - dbTimeNoWait)*1000);
				TRACE1("ReadDisk NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
				TRACE1("ReadDisk WaitTime:%.1f ms",dbTimeWait * 1000);
			}

		}
	}

	// д��һ������
	ullReadSectors = 0;
	ullRemainSectors = 0;
	ullStartSectors = headData.ullStartSector;
	dwSectors = BUF_SECTORS;
	dwLen = BUF_SECTORS * m_dwBytesPerSector;

	while (bResult && !*m_lpCancel && ullReadSectors < headData.ullSectors && ullStartSectors < m_ullSectorNums)
	{
		QueryPerformanceCounter(&t0);

		ullRemainSectors = headData.ullSectors - ullReadSectors;

		if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
		{
			ullRemainSectors = m_ullSectorNums - ullStartSectors;
		}

		if (ullRemainSectors < BUF_SECTORS)
		{
			dwSectors = (DWORD)ullRemainSectors;

			dwLen = dwSectors * headData.wBytesPerSector;
		}

		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		QueryPerformanceCounter(&t1);
		if (!ReadSectors(hDisk,ullStartSectors,dwSectors,headData.wBytesPerSector,pByte,port->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;
			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}
		else
		{	
			pHashMethod->update((void *)pByte,dwLen);
			
			delete []pByte;

			ullStartSectors += dwSectors;
			ullReadSectors += dwSectors;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��
			port->AppendUsedWaitTimeS(dbTimeWait);
			port->AppendUsedNoWaitTimeS(dbTimeNoWait);
			port->AppendCompleteSize(dwLen);

		}

	}

	CloseHandle(hDisk);
	hDisk = INVALID_HANDLE_VALUE;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (bResult)
	{
		port->SetHash(pHashMethod->digest(),pHashMethod->getHashLength());

		// ��������̣���Ҫ�ȴ�ĸ�̽���
		if (port->GetPortType() == PortType_TARGET_DISK)
		{
			while (m_MasterPort->GetPortState() == PortState_Active)
			{
				Sleep(10);
			}

			// �Ƚ�HASHֵ
			CString strVerifyHash,strMasterHash;
			for (int i = 0; i < pHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),m_pMasterHashMethod->digest()[i]);
				strMasterHash += strHash;

				strHash.Format(_T("%02X"),pHashMethod->digest()[i]);
				strVerifyHash += strHash;
			}

			CString strHashMethod(pHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,port->GetPortName(),port->GetDiskNum(),strHashMethod,strVerifyHash);

			if (strVerifyHash == strMasterHash)
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

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Verify FAIL")
					,port->GetPortName(),port->GetDiskNum());
			}
		}
		else if (port->GetPortType() == PortType_MASTER_DISK)
		{

			// �Ƚ�HASHֵ
			CString strMasterHash;
			for (int i = 0; i < pHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),pHashMethod->digest()[i]);
				strMasterHash += strHash;
			}

			CString strHashMethod(pHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,port->GetPortName(),port->GetDiskNum(),strHashMethod,strMasterHash);

			bResult = TRUE;
			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(TRUE);
			port->SetPortState(PortState_Pass);
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

BOOL CDisk::Start()
{
	// 
	
	BOOL bRet = FALSE;
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
	case WorkMode_QuickCopy:
		bRet = OnCopyDisk();
		break;

	case WorkMode_ImageCopy:
		bRet = OnCopyImage();
		break;

	case WorkMode_ImageMake:
		bRet = OnMakeImage();
		break;

	case WorkMode_DiskCompare:
		bRet = OnCompareDisk();
		break;

	case WorkMode_DiskClean:
		bRet = OnCleanDisk();
		break;

	}

	return bRet;
}

BOOL CDisk::ReadImage()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	ULONGLONG ullReadSize = SIZEOF_IMAGE_HEADER;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	while (bResult && !*m_lpCancel && ullReadSize < m_ullCapacity && m_MasterPort->GetPortState() == PortState_Active)
	{
		QueryPerformanceCounter(&t0);
		// �ж϶����Ƿ�ﵽ����ֵ
		while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE)
			&& !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
		{
			Sleep(0);
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
		if (!ReadFileAsyn(m_hMaster,ullOffset,dwLen,pkgHead,m_MasterPort->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = *(PDWORD)&pkgHead[8];

		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		if (!ReadFileAsyn(m_hMaster,ullOffset,dwLen,pByte,m_MasterPort->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;
			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (pByte[dwLen-1] != IMAGE_HEADER_END)
		{
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_Image_Format_Error;
			bResult = FALSE;
			delete []pByte;
			break;
		}

		DATA_INFO dataInfo = {0};
		dataInfo.ullOffset = ullOffset;
		dataInfo.dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo.pData = new BYTE[dataInfo.dwDataSize];
		memcpy(dataInfo.pData,&pByte[PKG_HEADER_SIZE],dataInfo.dwDataSize);
		m_CompressQueue.AddTail(dataInfo);
		delete []pByte;

		ullOffset += dwLen;
		ullReadSize += dwLen;

		QueryPerformanceCounter(&t2);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);
		m_MasterPort->AppendCompleteSize(dwLen);

		// 		TRACE1("ReadImage NoWaitSpeed:%.1f MB/s",(double)dwLen / (dbTimeNoWait * 1024 * 1024));
		// 		TRACE2("ReadImage WaitSpeed:%.1f MB/s, WaitTime:%f ms",(double)dwLen / (dbTimeWait * 1024 * 1024),(dbTimeWait-dbTimeNoWait)*1000);

		TRACE1("ReadImage NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
		TRACE1("ReadImage WaitTime:%.1f ms",dbTimeWait * 1000);

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode);
	}

	// �������ݶ�������
	if (bResult)
	{
		POSITION posQueue = m_DataQueueList.GetHeadPosition();
		POSITION posTarg = m_TargetPorts->GetHeadPosition();
		while (posQueue && posTarg)
		{
			CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
			CPort *port = m_TargetPorts->GetNext(posTarg);

			while (dataQueue->GetCount() > 0 && m_MasterPort->GetResult() && port->GetResult())
			{
				Sleep(100);
			}
		}
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
	}
	else
	{
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CDisk::Compress()
{
	BOOL bResult = TRUE;

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);
	while (!*m_lpCancel && m_MasterPort->GetResult())
	{
		QueryPerformanceCounter(&t0);
		while (m_CompressQueue.GetCount() <= 0 
			&& !*m_lpCancel 
			&& m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			Sleep(0);
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
		DATA_INFO dataInfo = m_CompressQueue.GetHeadRemove();
		if (dataInfo.pData == NULL)
		{
			continue;
		}

		DWORD dwSourceLen = sizeof(ULONGLONG) + sizeof(DWORD) + dataInfo.dwDataSize;
		BYTE *pBuffer = new BYTE[dwSourceLen];
		ZeroMemory(pBuffer,dwSourceLen);
		memcpy(pBuffer,&dataInfo.ullOffset,sizeof(ULONGLONG));
		memcpy(pBuffer + sizeof(ULONGLONG),&dataInfo.dwDataSize,sizeof(DWORD));
		memcpy(pBuffer + sizeof(ULONGLONG) + sizeof(DWORD),dataInfo.pData,dataInfo.dwDataSize);

		delete []dataInfo.pData;

		DWORD dwDestLen = MAX_COMPRESS_BUF;
		BYTE *pDest = new BYTE[MAX_COMPRESS_BUF];
		ZeroMemory(pDest,MAX_COMPRESS_BUF);

		int ret = compress2(pDest,&dwDestLen,pBuffer,dwSourceLen,Z_DEFAULT_COMPRESSION);

		if (ret == Z_OK)
		{
			DATA_INFO compressData = {0};
			compressData.dwDataSize = dwDestLen + sizeof(ULONGLONG) + sizeof(DWORD) + 1;
			compressData.pData = new BYTE[compressData.dwDataSize];
			ZeroMemory(compressData.pData,compressData.dwDataSize);

			compressData.pData[compressData.dwDataSize - 1] = 0xED; //������־
			memcpy(compressData.pData + sizeof(ULONGLONG),&compressData.dwDataSize,sizeof(DWORD));
			memcpy(compressData.pData + sizeof(ULONGLONG) + sizeof(DWORD),pDest,dwDestLen);

			AddDataQueueList(compressData);

			delete []compressData.pData;

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // ��

			//			TRACE1("Compress NoWaitSpeed:%.1f MB/s",(double)dwSourceLen / (dbTimeNoWait * 1024 * 1024));
			//			TRACE2("Compress WaitSpeed:%.1f MB/s, WaitTime:%f ms",(double)dwSourceLen / (dbTimeWait * 1024 * 1024),(dbTimeWait - dbTimeNoWait) * 1000);

			TRACE1("Compress NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
			TRACE1("Compress WaitTime:%.1f ms",dbTimeWait * 1000);
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
	}

	return bResult;
}

BOOL CDisk::Uncompress()
{
	BOOL bResult = TRUE;
	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	while (!*m_lpCancel && m_MasterPort->GetResult() && bResult)
	{
		QueryPerformanceCounter(&t0);
		while (m_CompressQueue.GetCount() <= 0
			&& !*m_lpCancel 
			&& m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			Sleep(10);
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
		DATA_INFO dataInfo = m_CompressQueue.GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}

		DWORD dwDestLen = MAX_COMPRESS_BUF;
		BYTE *pDest = new BYTE[MAX_COMPRESS_BUF];
		ZeroMemory(pDest,MAX_COMPRESS_BUF);

		int ret = uncompress(pDest,&dwDestLen,dataInfo.pData,dataInfo.dwDataSize);

		delete []dataInfo.pData;

		if (ret == Z_OK)
		{
			DATA_INFO uncompressData = {0};
			uncompressData.ullOffset = *(PULONGLONG)pDest;
			uncompressData.dwDataSize = *(PDWORD)(pDest + sizeof(ULONGLONG));

			uncompressData.pData = new BYTE[uncompressData.dwDataSize];
			ZeroMemory(uncompressData.pData,uncompressData.dwDataSize);
			memcpy(uncompressData.pData,pDest + sizeof(ULONGLONG) + sizeof(DWORD),uncompressData.dwDataSize);
			
			AddDataQueueList(uncompressData);

			m_CS.Lock();
			EFF_DATA effData;
			effData.ullStartSector = uncompressData.ullOffset/BYTES_PER_SECTOR;
			effData.ullSectors = uncompressData.dwDataSize/BYTES_PER_SECTOR;
			effData.wBytesPerSector = BYTES_PER_SECTOR;
			m_EffList.AddTail(effData);

			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)uncompressData.pData,uncompressData.dwDataSize);
			}

			delete []uncompressData.pData;

			m_CS.Unlock();

			QueryPerformanceCounter(&t2);

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // ��
			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			// 			TRACE1("Uncompress NoWaitSpeed:%.1f MB/s",(double)dataInfo.dwDataSize / (dbTimeNoWait * 1024 * 1024));
			// 			TRACE2("Uncompress WaitSpeed:%.1f MB/s, WaitTime:%f ms",(double)dataInfo.dwDataSize / (dbTimeWait * 1024 * 1024),(dbTimeWait-dbTimeNoWait) * 1000);
			TRACE1("Uncompress NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
			TRACE1("Uncompress WaitTime:%.1f ms",dbTimeWait * 1000);
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
	}

	return bResult;
}

BOOL CDisk::WriteImage( CPort *port,CDataQueue *pDataQueue )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	HANDLE hFile = GetHandleOnFile(port->GetFileName(),CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_System,dwErrorCode);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create image file error,filename=%s,system errorcode=%ld,%s")
			,port->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	// ���㾫ȷ�ٶ�
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetStartTime(CTime::GetCurrentTime());
	while (!*m_lpCancel && m_MasterPort->GetResult())
	{
		//dwTimeWait = timeGetTime();
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <=0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active)
		{
			Sleep(10);
		}

		if (!m_MasterPort->GetResult())
		{
			errType = m_MasterPort->GetErrorCode(&dwErrorCode);
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

		DATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}
		*(PULONGLONG)dataInfo.pData = ullPkgIndex;
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = dataInfo.dwDataSize;

		if (!WriteFileAsyn(hFile,ullOffset,dwLen,dataInfo.pData,port->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetFileName(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		//DWORD dwTime = timeGetTime();
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		ullOffset += dwLen;

		port->AppendCompleteSize(dwLen);
		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedWaitTimeS(dbTimeNoWait);

		delete []dataInfo.pData;

		// 		TRACE1("WriteImage NoWaitSpeed:%.1f MB/s",(double)dataInfo.dwDataSize / (dbTimeNoWait * 1024 * 1024));
		// 		TRACE2("WriteImage WaitSpeed:%.1f MB/s, WaitTime:%f ms",(double)dataInfo.dwDataSize / (dbTimeWait * 1024 * 1024),(dbTimeWait - dbTimeNoWait) * 1000);
		TRACE1("WriteImage NoWaitTime:%.1f ms",dbTimeNoWait * 1000);
		TRACE1("WriteImage WaitTime:%.1f ms",dbTimeWait * 1000);
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetFileName(),port->GetRealSpeed(),dwErrorCode);
	}

	if (!m_MasterPort->GetResult())
	{
		bResult = FALSE;
		errType = m_MasterPort->GetErrorCode(&dwErrorCode);
	}

	// дIMAGEͷ
	if (bResult)
	{
		LARGE_INTEGER liSize = {0};
		GetFileSizeEx(hFile,&liSize);
		// �������һ������ΪIMAGEID

		IMAGE_HEADER imgHead;
		ZeroMemory(&imgHead,sizeof(IMAGE_HEADER));
		memcpy(imgHead.szImageFlag,IMAGE_FLAG,strlen(IMAGE_FLAG));
		imgHead.ullImageSize = liSize.QuadPart;
		memcpy(imgHead.szAppVersion,APP_VERSION,strlen(APP_VERSION));
		imgHead.dwImageVersion = IMAGE_VERSION;
		imgHead.dwMaxSizeOfPackage = MAX_COMPRESS_BUF;
		imgHead.ullCapacitySize = m_MasterPort->GetTotalSize();
		imgHead.dwBytesPerSector = m_MasterPort->GetBytesPerSector();
		memcpy(imgHead.szZipVer,ZIP_VERSION,strlen(ZIP_VERSION));
		imgHead.ullPkgCount = ullPkgIndex;
		imgHead.ullValidSize = m_MasterPort->GetValidSize();
		imgHead.dwHashLen = m_pMasterHashMethod->getHashLength();
		imgHead.dwHashType = m_MasterPort->GetHashMethod();
		memcpy(imgHead.byImageDigest,m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());
		imgHead.byEnd = IMAGE_HEADER_END;

		dwLen = SIZEOF_IMAGE_HEADER;

		if (!WriteFileAsyn(hFile,0,dwLen,(LPBYTE)&imgHead,port->GetOverlapped(),&dwErrorCode))
		{
			bResult = FALSE;
		}
	}

	CloseHandle(hFile);
		
	port->SetEndTime(CTime::GetCurrentTime());
	port->SetResult(bResult);
	
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

BOOL CDisk::CleanDisk( CPort *port )
{
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;

	switch(m_CleanMode)
	{
	case CleanMode_Quick:
		{
			port->SetStartTime(CTime::GetCurrentTime());
			DWORD dwErrorCode = 0;
			BOOL bResult = QuickClean(port,&dwErrorCode);

			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(bResult);

			if (bResult)
			{
				port->SetPortState(PortState_Pass);
			}
			else
			{
				port->SetPortState(PortState_Pass);
				port->SetErrorCode(ErrorType_System,dwErrorCode);
			}
			
			return bResult;
		}
		
	case CleanMode_Full:
		{
			BOOL bResult = TRUE;
			DWORD dwErrorCode = 0;
			ErrorType errType = ErrorType_System;

			ULONGLONG ullStartSectors = 0;
			DWORD dwSectors = BUF_SECTORS;
			DWORD dwLen = BUF_SECTORS * dwBytePerSector;

			QuickClean(port,&dwErrorCode);

			HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,&dwErrorCode);

			if (hDisk == INVALID_HANDLE_VALUE)
			{
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_System,dwErrorCode);
				port->SetPortState(PortState_Fail);
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenDisk error,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				return FALSE;
			}

			// ���㾫ȷ�ٶ�
			LARGE_INTEGER freq,t0,t1,t2;
			double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

			QueryPerformanceFrequency(&freq);

			port->SetStartTime(CTime::GetCurrentTime());
			port->SetValidSize(port->GetTotalSize());

			while (!*m_lpCancel && ullStartSectors < ullSectorNums)
			{
				QueryPerformanceCounter(&t0);

				if (ullStartSectors + dwSectors > ullSectorNums)
				{
					dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
					dwLen = dwSectors * dwBytePerSector;
				}

				BYTE *pFillByte = new BYTE[dwLen];

				if (m_nFillValue != RANDOM_VALUE)
				{
					memset(pFillByte,m_nFillValue,dwLen);
				}
				else
				{
					srand((unsigned int)time(NULL));

					for (DWORD i = 0;i < dwLen;i++)
					{
						pFillByte[i] = (BYTE)rand();
					}
				}

				//dwTimeNoWait = timeGetTime();
				QueryPerformanceCounter(&t1);
				if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(),&dwErrorCode))
				{
					bResult = FALSE;
					delete []pFillByte;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
					break;
				}
				else
				{
					//DWORD dwTime = timeGetTime();
					QueryPerformanceCounter(&t2);

					dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

					dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

					ullStartSectors += dwSectors;

					port->AppendCompleteSize(dwLen);
					port->AppendUsedWaitTimeS(dbTimeWait);
					port->AppendUsedNoWaitTimeS(dbTimeNoWait);

				}

				delete []pFillByte;

			}

			if (*m_lpCancel)
			{
				bResult = FALSE;
				dwErrorCode = CustomError_Cancel;
				errType = ErrorType_Custom;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
			}

			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(bResult);

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

	case CleanMode_Safe:
		{
			BOOL bResult = TRUE;
			DWORD dwErrorCode = 0;
			ErrorType errType = ErrorType_System;

			QuickClean(port,&dwErrorCode);

			HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,&dwErrorCode);

			if (hDisk == INVALID_HANDLE_VALUE)
			{
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_System,dwErrorCode);
				port->SetPortState(PortState_Fail);
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenDisk error,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				return FALSE;
			}

			// ���㾫ȷ�ٶ�
			LARGE_INTEGER freq,t0,t1,t2;
			double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

			QueryPerformanceFrequency(&freq);

			port->SetStartTime(CTime::GetCurrentTime());
			port->SetValidSize(port->GetTotalSize());

			for (int i = 0; i < 7;i++)
			{
				ULONGLONG ullStartSectors = 0;
				DWORD dwSectors = BUF_SECTORS;
				DWORD dwLen = BUF_SECTORS * dwBytePerSector;

				port->SetCompleteSize(0);
				port->SetUsedNoWaitTimeS(0);
				port->SetUsedWaitTimeS(0);

				while (!*m_lpCancel && ullStartSectors < ullSectorNums)
				{
					QueryPerformanceCounter(&t0);

					if (ullStartSectors + dwSectors > ullSectorNums)
					{
						dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
						dwLen = dwSectors * dwBytePerSector;
					}

					BYTE *pFillByte = new BYTE[dwLen];

					// The write head passes over each sector seven times (0x00, 0xFF, Random,
					// 0x96, 0x00, 0xFF, Random).
					switch (i)
					{
					case 0:
					case 4:
						memset(pFillByte,0,dwLen);
						break;

					case 1:
					case 5:
						memset(pFillByte,0xFF,dwLen);
						break;

					case 2:
						memset(pFillByte,0x96,dwLen);
						break;

					case 3:
					case 6:
						srand((unsigned int)time(NULL));

						for (DWORD i = 0;i < dwLen;i++)
						{
							pFillByte[i] = (BYTE)rand();
						}

						break;
					}

					QueryPerformanceCounter(&t1);
					if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(),&dwErrorCode))
					{
						bResult = FALSE;
						delete []pFillByte;

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
							,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
						break;
					}
					else
					{
						//DWORD dwTime = timeGetTime();
						QueryPerformanceCounter(&t2);

						dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

						dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

						ullStartSectors += dwSectors;

						port->AppendCompleteSize(dwLen);
						port->AppendUsedWaitTimeS(dbTimeWait);
						port->AppendUsedNoWaitTimeS(dbTimeWait);
						
					}

					delete []pFillByte;

				}

				if (*m_lpCancel)
				{
					bResult = FALSE;
					dwErrorCode = CustomError_Cancel;
					errType = ErrorType_Custom;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
					break;
				}
			}

			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(bResult);

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

	}

	return TRUE;
}

DWORD WINAPI CDisk::ReadDiskThreadProc( LPVOID lpParm )
{
	CDisk *disk = (CDisk *)lpParm;

	BOOL bResult = disk->ReadDisk();

	return bResult;
}

DWORD WINAPI CDisk::ReadImageThreadProc( LPVOID lpParm )
{
	CDisk *disk = (CDisk *)lpParm;
	
	BOOL bResult = disk->ReadImage();

	return bResult;
}

DWORD WINAPI CDisk::WriteDiskThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CDataQueue *dataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bResult = disk->WriteDisk(port,dataQueue);

	delete parm;

	return bResult;

}

DWORD WINAPI CDisk::WriteImageThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CDataQueue *dataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bResult = disk->WriteImage(port,dataQueue);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::VerifyThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CHashMethod *pHashMethod = (CHashMethod *)parm->lpVoid3;

	BOOL bResult = disk->Verify(port,pHashMethod);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::CompressThreadProc( LPVOID lpParm )
{
	CDisk *disk = (CDisk *)lpParm;

	BOOL bResult = disk->Compress();

	return bResult;
}

DWORD WINAPI CDisk::UnCompressThreadProc( LPVOID lpParm )
{
	CDisk *disk = (CDisk *)lpParm;

	BOOL bResult = disk->Uncompress();

	return bResult;
}

DWORD WINAPI CDisk::CleanDiskThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->CleanDisk(port);

	delete parm;

	return bResult;
}

void CDisk::AddDataQueueList( DATA_INFO dataInfo )
{
	POSITION pos1 = m_DataQueueList.GetHeadPosition();
	POSITION pos2 = m_TargetPorts->GetHeadPosition();

	while (pos1 && pos2)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos1);
		CPort *port = m_TargetPorts->GetNext(pos2);

		if (port->GetResult())
		{
			DATA_INFO data;
			data.dwDataSize = dataInfo.dwDataSize;
			data.ullOffset = dataInfo.ullOffset;
			data.pData = new BYTE[dataInfo.dwDataSize];
			memcpy(data.pData,dataInfo.pData,dataInfo.dwDataSize);
			dataQueue->AddTail(data);
		}
	}
}

bool CDisk::IsReachLimitQty( int limit )
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

	return (bReached || m_CompressQueue.GetCount() > limit);
}

void CDisk::SetCompareMode( CompareMode compareMode )
{
	m_CompareMode = compareMode;
}

