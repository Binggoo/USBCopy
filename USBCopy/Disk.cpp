#include "StdAfx.h"
#include "USBCopy.h"
#include "Disk.h"
#include <WinIoCtl.h>
#include "Utils.h"
#include "Checksum32.h"
#include "CRC32.h"
#include "MD5.h"
#include "zlib.h"

#include "Fat32.h"
#include "Ini.h"

#ifdef _DEBUG
#pragma comment(lib,"zlib128d.lib")
#else
#pragma comment(lib,"zlib128.lib")
#endif



#define HIDE_SECTORS  0x800   // 1M
#define COMPRESS_THREAD_COUNT 3

typedef struct _STRUCT_LPVOID_PARM
{
	LPVOID lpVoid1;   //CDisk
	LPVOID lpVoid2;   //CPort
	LPVOID lpVoid3;   //CHashMethod or CDataQueue
	LPVOID lpVoid4;   //Other
}VOID_PARM,*LPVOID_PARM;


CDisk::CDisk(void)
{
	m_hLogFile = INVALID_HANDLE_VALUE;
	m_hMaster = INVALID_HANDLE_VALUE;
	m_dwBytesPerSector = 0;
	m_ullSectorNums = 0;
	m_ullValidSize = 0;
	m_ullCapacity = 0;

	m_bCompare = FALSE;
	m_pMasterHashMethod = NULL;

	ZeroMemory(m_ImageHash,LEN_DIGEST);

	m_bQuickFormat = TRUE;
	m_dwClusterSize = 8 * 512;

	m_bCompressComplete = TRUE;

	m_bServerFirst = FALSE;

	m_ClientSocket = INVALID_SOCKET;

	m_iCompressLevel = Z_BEST_SPEED;

	m_nBlockSectors = BUF_SECTORS;

	// 2014-10-14 增量拷贝新增
	m_nSourceType = SourceType_Package;
	m_nCompareRule = 0;
	m_bIncludeSubDir = FALSE;
	m_bUploadUserLog = FALSE;

	// 2014-11-12 新增
	m_bAllowCapGap = FALSE;
	m_nCapGapPercent = 0;
	m_bQuickImage = TRUE;
	m_bCleanDiskFirst = FALSE;
	m_nCleanTimes = 0;
	m_pCleanValues = NULL;

	m_bEnd = TRUE;
	m_bCompareClean = FALSE;

	m_CompareMethod = CompareMethod_Hash;
	m_bVerify = FALSE;

	m_bReadOnlyTest = FALSE;
	m_bRetainOrginData = FALSE;
	m_bFormatFinished = FALSE;
	m_bStopBadBlock = FALSE;

	m_bCheckSpeed = FALSE;
	m_dbMinReadSpeed = 0.0;
	m_dbMinWriteSpeed = 0.0;

	m_CompareCleanSeq = CompareCleanSeq_In;
	m_pFillBytes = NULL;
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
	m_DataQueueList.RemoveAll();

	if (m_pMasterHashMethod != NULL)
	{
		delete m_pMasterHashMethod;
	}

	if (m_pCleanValues)
	{
		delete []m_pCleanValues;
	}

	if (m_pFillBytes)
	{
		delete []m_pFillBytes;
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

ULONGLONG CDisk::GetNumberOfSectors( HANDLE hDevice,PDWORD pdwBytesPerSector,MEDIA_TYPE *type )
{
	DWORD junk;
	DISK_GEOMETRY_EX diskgeometry;
	BOOL bResult;
	bResult = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskgeometry, sizeof(diskgeometry), &junk, NULL);
	if (!bResult)
	{
		return 0;
	}

	if (type != NULL)
	{
		*type = diskgeometry.Geometry.MediaType;
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
			(partSize.QuadPart * i) + (HIDE_SECTORS * (LONGLONG)(pdgex.Geometry.BytesPerSector));   //固定1M
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

BOOL CDisk::DestroyDisk( HANDLE hDisk )
{
	BOOL result;                  // results flag
	DWORD readed;                 // discard results

	result = DeviceIoControl(
		hDisk,               // handle to device
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
		return FALSE;
	}

	//fresh the partition table
	result = DeviceIoControl(
		hDisk,
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
		return FALSE;
	}

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

BOOL CDisk::SetDiskAtrribute(HANDLE hDisk,BOOL bReadOnly,BOOL bOffline,PDWORD pdwErrorCode)
{
	BOOL bSuc;
	GET_DISK_ATTRIBUTES getAttr;
	SET_DISK_ATTRIBUTES setAttr;
	DWORD dwRet = 0;
	memset(&getAttr, 0, sizeof(GET_DISK_ATTRIBUTES));
	memset(&setAttr, 0, sizeof(SET_DISK_ATTRIBUTES));

	bSuc = DeviceIoControl(hDisk, IOCTL_DISK_GET_DISK_ATTRIBUTES, NULL, 0,
		&getAttr, sizeof(GET_DISK_ATTRIBUTES), &dwRet, NULL);

	if (bSuc)
	{
		if (bOffline)
		{
			if (!(getAttr.Attributes & DISK_ATTRIBUTE_OFFLINE)) 
			{
				setAttr.Attributes |= DISK_ATTRIBUTE_OFFLINE;
				setAttr.AttributesMask |= DISK_ATTRIBUTE_OFFLINE;
			}
		}
		else
		{
			if (getAttr.Attributes & DISK_ATTRIBUTE_OFFLINE) 
			{
				setAttr.Attributes &= ~DISK_ATTRIBUTE_OFFLINE;
				setAttr.AttributesMask |= DISK_ATTRIBUTE_OFFLINE;
			}
		}


		if (bReadOnly)
		{
			if (!(getAttr.Attributes & DISK_ATTRIBUTE_READ_ONLY))
			{
				setAttr.Attributes |= DISK_ATTRIBUTE_READ_ONLY;
				setAttr.AttributesMask |= DISK_ATTRIBUTE_READ_ONLY;
			}
		}
		else
		{
			if (getAttr.Attributes & DISK_ATTRIBUTE_READ_ONLY)
			{
				setAttr.Attributes &= ~DISK_ATTRIBUTE_READ_ONLY;
				setAttr.AttributesMask |= DISK_ATTRIBUTE_READ_ONLY;
			}
		}


		if (setAttr.AttributesMask)
		{
			setAttr.Version = sizeof(SET_DISK_ATTRIBUTES);
			bSuc = DeviceIoControl(hDisk, IOCTL_DISK_SET_DISK_ATTRIBUTES, &setAttr, sizeof(SET_DISK_ATTRIBUTES),NULL, 0,
				&dwRet, NULL);

			if (!bSuc)
			{

				for (int nRetry = 0; nRetry < 5; nRetry++)
				{
					Sleep(500);
					bSuc = DeviceIoControl(hDisk, IOCTL_DISK_SET_DISK_ATTRIBUTES, &setAttr, sizeof(SET_DISK_ATTRIBUTES),NULL, 0,
						&dwRet, NULL);

					if (bSuc)
					{
						break;
					}
				}

				*pdwErrorCode = GetLastError();
			}
		}
	}

	return bSuc;
}

BOOL CDisk::GetTSModelNameAndSerialNumber( HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode )
{
	BYTE buffer[18];
	BYTE cdb[16];
	DWORD dwRet = 0;

	memset(buffer,0,18);
	memset(cdb,0,16);

	cdb[0] = 0xD0;
	cdb[1] = 0x11;
	cdb[2] = 0x53;
	cdb[3] = 0x44;
	cdb[4] = 0x20;
	cdb[5] = 0x43;
	cdb[6] = 0x61;
	cdb[7] = 0x72;
	cdb[8] = 0x64;

	dwRet = ScsiCommand(hDevice,cdb,16,buffer,18,SCSI_IOCTL_DATA_IN,NULL,0,2);

	if (dwRet != 0)
	{
		*pdwErrorCode = dwRet; 
		return FALSE;
	}

	memset(cdb,0,16);
	cdb[0] = 0xD1;
	cdb[1] = 0x12;
	cdb[2] = 0x0A;
	cdb[3] = buffer[7];
	cdb[4] = buffer[6];
	cdb[10] = 0x06;
	cdb[11] = 0xFF;

	dwRet = ScsiCommand(hDevice,cdb,16,NULL,0,SCSI_IOCTL_DATA_OUT,NULL,0,2);

	if (dwRet != 0)
	{
		*pdwErrorCode = dwRet; 
		return FALSE;
	}

	memset(buffer,0,18);
	memset(cdb,0,16);

	cdb[0] = 0xD4;
	cdb[1] = 0x10;
	cdb[8] = 0x11;
	cdb[9] = 0xFF;

	dwRet = ScsiCommand(hDevice,cdb,16,buffer,17,SCSI_IOCTL_DATA_IN,NULL,0,2);

	if (dwRet != 0)
	{
		*pdwErrorCode = dwRet; 
		return FALSE;
	}
	TCHAR szModelName[6] = {NULL};
	TCHAR szSerialNumber[10] = {NULL};
	_stprintf_s(szModelName,_T("%c%c%c%c%c"),buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);
	_stprintf_s(szSerialNumber,_T("%02X%02X%02X%02X"),buffer[10],buffer[11],buffer[12],buffer[13]);

	_tcscpy_s(lpszSerialNum,_tcslen(szSerialNumber) + 1, szSerialNumber);
	_tcscpy_s(lpszModulName,_tcslen(szModelName) + 1, szModelName);

	return TRUE;
}

BOOL CDisk::GetUsbHDDModelNameAndSerialNumber(HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode)
{
	IDENTIFY_DEVICE identify = {0};

	DWORD dwRet = DoIdentifyDeviceSat(hDevice,0xA0,&identify,CMD_TYPE_SAT);

	if (dwRet != 0)
	{
		*pdwErrorCode =dwRet;
	}

	DWORD dwDiskData[IDENTIFY_BUFFER_SIZE / 2];
	WORD *pIDSector; // 对应结构IDSECTOR，见头文件
	pIDSector = (WORD *)(&identify);      //lint !e826
	for(DWORD i = 0; i < IDENTIFY_BUFFER_SIZE / 2; i++)
	{
		dwDiskData[i] = pIDSector[i];       //lint !e662 !e661
	}

	// get serial number
	TCHAR *pSN = ConvertSENDCMDOUTPARAMSBufferToString(dwDiskData, 10, 19);
	int nLen = _tcslen(pSN) + 1;
	_tcscpy_s(lpszSerialNum,nLen, pSN);

	// get model number
	TCHAR *pModel = ConvertSENDCMDOUTPARAMSBufferToString(dwDiskData, 27, 46);
	nLen = _tcslen(pModel) + 1;
	_tcscpy_s(lpszModulName,nLen,pModel);

	return TRUE;
}

BOOL CDisk::GetDiskModelNameAndSerialNumber( HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode )
{
	DWORD readed = 0;
	GETVERSIONINPARAMS gvopVersionParams;
	BOOL result = DeviceIoControl(
		hDevice, 
		SMART_GET_VERSION,
		NULL, 
		0, 
		&gvopVersionParams,
		sizeof(gvopVersionParams),
		&readed, 
		NULL);
	if (!result)        //fail
	{
		*pdwErrorCode = GetLastError();
		return FALSE;
	}

	if(0 == gvopVersionParams.bIDEDeviceMap)
	{
		return FALSE;
	}

	// IDE or ATAPI IDENTIFY cmd
	BYTE btIDCmd;
	SENDCMDINPARAMS inParams;
	BYTE nDrive =0;
	btIDCmd = (gvopVersionParams.bIDEDeviceMap >> nDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

	// output structure
	BYTE outParams[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];   // + 512 - 1

	//fill in the input buffer
	inParams.cBufferSize = 0;           //or IDENTIFY_BUFFER_SIZE ?
	inParams.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;
	inParams.irDriveRegs.bSectorCountReg = 1;
	inParams.irDriveRegs.bSectorNumberReg = 1;
	inParams.irDriveRegs.bCylLowReg = 0;
	inParams.irDriveRegs.bCylHighReg = 0;

	inParams.irDriveRegs.bDriveHeadReg = (nDrive & 1) ? 0xB0 : 0xA0;
	inParams.irDriveRegs.bCommandReg = btIDCmd;
	//inParams.bDriveNumber = nDrive;

	//get the attributes
	result = DeviceIoControl(
		hDevice, 
		SMART_RCV_DRIVE_DATA,
		&inParams,
		sizeof(SENDCMDINPARAMS) - 1,
		outParams,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		&readed,
		NULL);
	if (!result)        //fail
	{
		*pdwErrorCode = GetLastError();
		return FALSE;
	}

	DWORD dwDiskData[IDENTIFY_BUFFER_SIZE / 2];
	WORD *pIDSector; // 对应结构IDSECTOR，见头文件
	pIDSector = (WORD *)(((SENDCMDOUTPARAMS*)outParams)->bBuffer);      //lint !e826
	for(DWORD i = 0; i < IDENTIFY_BUFFER_SIZE / 2; i++)
	{
		dwDiskData[i] = pIDSector[i];       //lint !e662 !e661
	}

	// get serial number
	TCHAR *pSN = ConvertSENDCMDOUTPARAMSBufferToString(dwDiskData, 10, 19);
	int nLen = _tcslen(pSN) + 1;
	_tcscpy_s(lpszSerialNum,nLen, pSN);

	// get model number
	TCHAR *pModel = ConvertSENDCMDOUTPARAMSBufferToString(dwDiskData, 27, 46);
	nLen = _tcslen(pModel) + 1;
	_tcscpy_s(lpszModulName,nLen,pModel);

	return TRUE;
}

TCHAR * CDisk::ConvertSENDCMDOUTPARAMSBufferToString( const DWORD *dwDiskData, DWORD nFirstIndex, DWORD nLastIndex )
{
	static TCHAR szResBuf[IDENTIFY_BUFFER_SIZE];     //512
	DWORD nIndex = 0;
	DWORD nPosition = 0;

	for (nIndex = nFirstIndex; nIndex <= nLastIndex; nIndex++)
	{
		// get high byte
		szResBuf[nPosition] = (TCHAR)(dwDiskData[nIndex] >> 8);
		nPosition++;

		// get low byte
		szResBuf[nPosition] = (TCHAR)(dwDiskData[nIndex] & 0xff);
		nPosition++;
	}

	// End the string
	szResBuf[nPosition] = _T('\0');

	return szResBuf;
}

DWORD CDisk::ScsiCommand( HANDLE hDevice, void *pCdb, UCHAR ucCdbLength, void *pDataBuffer, ULONG ulDataLength, UCHAR ucDirection , void *pSenseBuffer, UCHAR ucSenseLength, ULONG ulTimeout )
{
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.sptd.PathId = 0;
	sptdwb.sptd.TargetId = 1;
	sptdwb.sptd.Lun = 0;
	sptdwb.sptd.CdbLength = ucCdbLength;
	sptdwb.sptd.SenseInfoLength = sizeof(sptdwb.ucSenseBuf);
	sptdwb.sptd.DataIn = ucDirection;
	sptdwb.sptd.DataTransferLength = ulDataLength;
	sptdwb.sptd.TimeOutValue = ulTimeout;
	sptdwb.sptd.DataBuffer = pDataBuffer;
	sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

	memcpy(sptdwb.sptd.Cdb,pCdb,ucCdbLength);
	DWORD dwLength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	DWORD dwReturn = 0;
	BOOL bOK = DeviceIoControl(hDevice,IOCTL_SCSI_PASS_THROUGH_DIRECT,&sptdwb,dwLength,&sptdwb,dwLength,&dwReturn,NULL);

	if (!bOK)
	{
		DWORD dwError = GetLastError();
		if (pSenseBuffer && ucSenseLength)
		{
			ZeroMemory(pSenseBuffer,ucSenseLength);
		}
		return dwError;
	}
	if (pSenseBuffer && ucSenseLength)
	{
		memcpy(pSenseBuffer,sptdwb.ucSenseBuf,(ucSenseLength < sizeof(sptdwb.ucSenseBuf)) ? ucSenseLength : sizeof(sptdwb.ucSenseBuf));
	}

	return 0;
}

DWORD CDisk::DoIdentifyDeviceSat( HANDLE hDevice, BYTE target, IDENTIFY_DEVICE* data, COMMAND_TYPE type )
{
	BOOL	bRet;
	DWORD	dwReturned;
	DWORD	length;

	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;

	if(data == NULL)
	{
		return	-1;
	}

	::ZeroMemory(data, sizeof(IDENTIFY_DEVICE));

	if(hDevice == INVALID_HANDLE_VALUE)
	{
		return	-1;
	}

	::ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 0;
	sptwb.spt.Lun = 0;
	sptwb.spt.SenseInfoLength = 24;
	sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
	sptwb.spt.DataTransferLength = IDENTIFY_BUFFER_SIZE;
	sptwb.spt.TimeOutValue = 2;
	sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucSenseBuf);

	if(type == CMD_TYPE_SAT)
	{
		sptwb.spt.CdbLength = 12;
		sptwb.spt.Cdb[0] = 0xA1;//ATA PASS THROUGH(12) OPERATION CODE(A1h)
		sptwb.spt.Cdb[1] = (4 << 1) | 0; //MULTIPLE_COUNT=0,PROTOCOL=4(PIO Data-In),Reserved
		sptwb.spt.Cdb[2] = (1 << 3) | (1 << 2) | 2;//OFF_LINE=0,CK_COND=0,Reserved=0,T_DIR=1(ToDevice),BYTE_BLOCK=1,T_LENGTH=2
		sptwb.spt.Cdb[3] = 0;//FEATURES (7:0)
		sptwb.spt.Cdb[4] = 1;//SECTOR_COUNT (7:0)
		sptwb.spt.Cdb[5] = 0;//LBA_LOW (7:0)
		sptwb.spt.Cdb[6] = 0;//LBA_MID (7:0)
		sptwb.spt.Cdb[7] = 0;//LBA_HIGH (7:0)
		sptwb.spt.Cdb[8] = target;
		sptwb.spt.Cdb[9] = ID_CMD;//COMMAND
	}
	else if(type == CMD_TYPE_SUNPLUS)
	{
		sptwb.spt.CdbLength = 12;
		sptwb.spt.Cdb[0] = 0xF8;
		sptwb.spt.Cdb[1] = 0x00;
		sptwb.spt.Cdb[2] = 0x22;
		sptwb.spt.Cdb[3] = 0x10;
		sptwb.spt.Cdb[4] = 0x01;
		sptwb.spt.Cdb[5] = 0x00; 
		sptwb.spt.Cdb[6] = 0x01; 
		sptwb.spt.Cdb[7] = 0x00; 
		sptwb.spt.Cdb[8] = 0x00;
		sptwb.spt.Cdb[9] = 0x00;
		sptwb.spt.Cdb[10] = target; 
		sptwb.spt.Cdb[11] = 0xEC; // ID_CMD
	}
	else if(type == CMD_TYPE_IO_DATA)
	{
		sptwb.spt.CdbLength = 12;
		sptwb.spt.Cdb[0] = 0xE3;
		sptwb.spt.Cdb[1] = 0x00;
		sptwb.spt.Cdb[2] = 0x00;
		sptwb.spt.Cdb[3] = 0x01;
		sptwb.spt.Cdb[4] = 0x01;
		sptwb.spt.Cdb[5] = 0x00; 
		sptwb.spt.Cdb[6] = 0x00; 
		sptwb.spt.Cdb[7] = target;
		sptwb.spt.Cdb[8] = 0xEC;  // ID_CMD
		sptwb.spt.Cdb[9] = 0x00;
		sptwb.spt.Cdb[10] = 0x00; 
		sptwb.spt.Cdb[11] = 0x00;
	}
	else if(type == CMD_TYPE_LOGITEC)
	{
		sptwb.spt.CdbLength = 10;
		sptwb.spt.Cdb[0] = 0xE0;
		sptwb.spt.Cdb[1] = 0x00;
		sptwb.spt.Cdb[2] = 0x00;
		sptwb.spt.Cdb[3] = 0x00;
		sptwb.spt.Cdb[4] = 0x00;
		sptwb.spt.Cdb[5] = 0x00; 
		sptwb.spt.Cdb[6] = 0x00; 
		sptwb.spt.Cdb[7] = target; 
		sptwb.spt.Cdb[8] = 0xEC;  // ID_CMD
		sptwb.spt.Cdb[9] = 0x4C;
	}
	else if(type == CMD_TYPE_JMICRON)
	{
		sptwb.spt.CdbLength = 12;
		sptwb.spt.Cdb[0] = 0xDF;
		sptwb.spt.Cdb[1] = 0x10;
		sptwb.spt.Cdb[2] = 0x00;
		sptwb.spt.Cdb[3] = 0x02;
		sptwb.spt.Cdb[4] = 0x00;
		sptwb.spt.Cdb[5] = 0x00; 
		sptwb.spt.Cdb[6] = 0x01; 
		sptwb.spt.Cdb[7] = 0x00; 
		sptwb.spt.Cdb[8] = 0x00;
		sptwb.spt.Cdb[9] = 0x00;
		sptwb.spt.Cdb[10] = target; 
		sptwb.spt.Cdb[11] = 0xEC; // ID_CMD
	}
	else if(type == CMD_TYPE_CYPRESS)
	{
		sptwb.spt.CdbLength = 16;
		sptwb.spt.Cdb[0] = 0x24;
		sptwb.spt.Cdb[1] = 0x24;
		sptwb.spt.Cdb[2] = 0x00;
		sptwb.spt.Cdb[3] = 0xBE;
		sptwb.spt.Cdb[4] = 0x01;
		sptwb.spt.Cdb[5] = 0x00; 
		sptwb.spt.Cdb[6] = 0x00; 
		sptwb.spt.Cdb[7] = 0x01; 
		sptwb.spt.Cdb[8] = 0x00;
		sptwb.spt.Cdb[9] = 0x00;
		sptwb.spt.Cdb[10] = 0x00; 
		sptwb.spt.Cdb[11] = target;
		sptwb.spt.Cdb[12] = 0xEC; // ID_CMD
		sptwb.spt.Cdb[13] = 0x00;
		sptwb.spt.Cdb[14] = 0x00;
		sptwb.spt.Cdb[15] = 0x00;
	}
	else
	{
		return -1;
	}

	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + sptwb.spt.DataTransferLength;

	bRet = ::DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH, 
		&sptwb, sizeof(SCSI_PASS_THROUGH),
		&sptwb, length,	&dwReturned, NULL);

	::CloseHandle(hDevice);

	if(bRet == FALSE || dwReturned != length)
	{
		return	GetLastError();
	}

	memcpy_s(data, sizeof(IDENTIFY_DEVICE), sptwb.ucDataBuf, sizeof(IDENTIFY_DEVICE));

	return	0;
}

BOOL CDisk::ReadSectors( HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector,
	LPBYTE lpSectBuff, LPOVERLAPPED lpOverlap,DWORD *pdwErrorCode,DWORD dwTimeOut /*= 2000*/)
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
		{
			DWORD dwRet = WaitForSingleObject(lpOverlap->hEvent, dwTimeOut);
			if (dwRet != WAIT_FAILED && dwRet != WAIT_TIMEOUT)
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

BOOL CDisk::WriteSectors( HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, 
	LPBYTE lpSectBuff,LPOVERLAPPED lpOverlap, DWORD *pdwErrorCode ,DWORD dwTimeOut /*= 2000*/)
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
		{
			DWORD dwRet = WaitForSingleObject(lpOverlap->hEvent, dwTimeOut);
			if (dwRet != WAIT_FAILED && dwRet != WAIT_TIMEOUT)
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

BOOL CDisk::ReadFileAsyn( HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,
	LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode ,DWORD dwTimeOut /*= 2000*/)
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
		{
			DWORD dwRet = WaitForSingleObject(lpOverlap->hEvent, dwTimeOut);
			if (dwRet != WAIT_FAILED && dwRet != WAIT_TIMEOUT)
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

BOOL CDisk::WriteFileAsyn( HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,
	LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode ,DWORD dwTimeOut /*= 2000*/)
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
		{
			DWORD dwRet = WaitForSingleObject(lpOverlap->hEvent, dwTimeOut);
			if (dwRet != WAIT_FAILED && dwRet != WAIT_TIMEOUT)
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

void CDisk::Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile ,CPortCommand *pCommand,UINT nBlockSectors)
{
	m_hWnd = hWnd;
	m_lpCancel = lpCancel;
	m_hLogFile = hLogFile;
	m_pCommand = pCommand;
	m_nBlockSectors = nBlockSectors;
}

void CDisk::SetMasterPort( CPort *port )
{
	m_MasterPort = port;
	m_MasterPort->Reset();

	DWORD dwErrorCode = 0;
	switch (m_MasterPort->GetPortType())
	{
	case PortType_MASTER_DISK:
		{
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
				m_ullSectorNums = GetNumberOfSectors(m_hMaster,&m_dwBytesPerSector,NULL);
				m_ullCapacity = m_ullSectorNums * m_dwBytesPerSector;
			}
		}

		break;

	case PortType_MASTER_FILE:
		{
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


		break;

	case PortType_SERVER:
		{
			// 本地创建IMAGE
			CString strTempName,strImageFile;
			strImageFile = m_MasterPort->GetFileName();

			strTempName.Format(_T("%s.$$$"),strImageFile.Left(strImageFile.GetLength() - 4));

			m_hMaster = GetHandleOnFile(strTempName,
				CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

			if (m_hMaster == INVALID_HANDLE_VALUE)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Open image file error,filename=%s,system errorcode=%ld,%s")
					,strTempName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			}

			m_MasterPort->SetPortState(PortState_Active);
		}


		break;

	default:
		m_MasterPort->SetPortState(PortState_Active);

		break;
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
		}

		CDataQueue *dataQueue = new CDataQueue();
		m_DataQueueList.AddTail(dataQueue);

	}
}

void CDisk::SetHashMethod( BOOL bComputeHash,HashMethod hashMethod )
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

void CDisk::SetWorkMode( WorkMode workMode)
{
	m_WorkMode = workMode;
}

void CDisk::SetCleanMode( CleanMode cleanMode,int nFillValue,BOOL bCompareClean ,CompareCleanSeq seq)
{
	m_CleanMode = cleanMode;
	m_nFillValue = nFillValue;
	m_bCompareClean = bCompareClean;
	m_CompareCleanSeq = seq;
}


BootSector CDisk::GetBootSectorType( const PBYTE pXBR )
{
	/* 用此法判断是否DBR有待确定
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

	if (ReadOffset(pXBR,0,3) == 0x000000)
	{
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_MBR"));
		return BootSector_MBR;
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

		if (mbr.Partition[i].StartLBA == 0x01 && mbr.Partition[i].TotalSector == 0xFFFFFFFF
			&& mbr.Partition[i].PartitionType == 0xEE)
		{
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetBootSectorType=BootSector_MBR"));
			return BootSector_MBR;
		}

		BYTE *pDBRTemp = new BYTE[m_dwBytesPerSector];
		ZeroMemory(pDBRTemp,m_dwBytesPerSector);

		if (!ReadSectors(m_hMaster,ullStartSector,1,m_dwBytesPerSector,pDBRTemp,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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
		MASTER_BOOT_RECORD mbr;
		ZeroMemory(&mbr,sizeof(MASTER_BOOT_RECORD));
		memcpy(&mbr,pByte + 0x1BE,sizeof(MASTER_BOOT_RECORD));

		for (int i = 0;i < 4;i++)
		{
			if (mbr.Partition[i].StartLBA == 0)
			{
				continue;
			}

			if (mbr.Partition[i].StartLBA == 0x01 && mbr.Partition[i].TotalSector == 0xFFFFFFFF
				&& mbr.Partition[i].PartitionType == 0xEE)
			{
				partitionStyle = PartitionStyle_GPT;
				CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_GPT"));
				break;
			}

			if (mbr.Partition[i].PartitionType == 0xEE)
			{
				partitionStyle = PartitionStyle_GPT;
				CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_GPT"));
				break;
			}

			partitionStyle = PartitionStyle_DOS;
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("GetPartitionStyle=PartitionStyle_DOS"));
			break;

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
		// 偏移2个扇区，确定是否是EXT_X文件系统
		BYTE *pByte = new BYTE[BYTES_PER_SECTOR];
		ZeroMemory(pByte,BYTES_PER_SECTOR);

		DWORD dwErrorCode = 0;
		if (ReadSectors(m_hMaster,ullStartSector + 2,1,BYTES_PER_SECTOR,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

	BYTE *pXBR = new BYTE[m_dwBytesPerSector];
	ZeroMemory(pXBR,m_dwBytesPerSector);
	if (!ReadSectors(m_hMaster,0,1,m_dwBytesPerSector,pXBR,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

	// 第一步，判断0号扇区是MBR，还是DBR？
	BootSector bootSector = GetBootSectorType(pXBR);

	// 第二步，判断分区体系
	PartitionStyle partitionStyle = GetPartitionStyle(pXBR,bootSector);

	// 第三步，判断文件系统
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

			if (!ReadSectors(m_hMaster,1,1,m_dwBytesPerSector,pEFI,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

			// 添加保护MBR、EFI信息（GPT头）、分区表，34个分区
			EFF_DATA effData;
			effData.ullStartSector = 0;
			effData.ullSectors = efiHead.FirstUsableLBA;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;
			m_EffList.AddTail(effData);

			// 读分区表项
			DWORD dwPatitionEntrySectors = efiHead.MaxNumberOfPartitionEntries * efiHead.SizeOfPartitionEntry / m_dwBytesPerSector;
			ULONGLONG ullStartSectors = efiHead.PartitionEntryLBA;
			DWORD dwEntryIndex = 1;
			for (DWORD i = 0;i < dwPatitionEntrySectors;i++)
			{
				BYTE *pByte = new BYTE[m_dwBytesPerSector];
				ZeroMemory(pByte,m_dwBytesPerSector);
				if (!ReadSectors(m_hMaster,ullStartSectors+i,1,m_dwBytesPerSector,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

					/* Windows保留分区可以拷贝可以不拷贝
					if (ullTempStartSector == 0)
					{
					dwEntryIndex++;
					continue;
					}
					*/

					BYTE *pTempDBR = new BYTE[m_dwBytesPerSector];
					ZeroMemory(pTempDBR,m_dwBytesPerSector);
					if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pTempDBR,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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


			// 添加分区表备份分区表
			effData.ullStartSector = efiHead.LastUsableLBA;
			effData.ullSectors = efiHead.MaxNumberOfPartitionEntries * efiHead.SizeOfPartitionEntry / m_dwBytesPerSector;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;
			m_EffList.AddTail(effData);

			// 添加备份EFI信息
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

			// 分析有效簇
			DWORD dwFATLen = fatInfo.dwFATLength * fatInfo.wBytesPerSector;
			ULONGLONG ullFatStartSectors = fatInfo.ullStartSector + fatInfo.wReserverSectors;
			BYTE *pFATByte = new BYTE[dwFATLen];
			ZeroMemory(pFATByte,dwFATLen);
			if (!ReadSectors(m_hMaster,ullFatStartSectors,fatInfo.dwFATLength,fatInfo.wBytesPerSector,pFATByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

			// 把起始扇区数据单独分开，为了最后读写这一部分数据，防止文件系统产生被系统识别而拒绝写磁盘
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

					// 是否超过总扇区数
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

			// 第一步，找到根目录
			// 通过ClusterHeapOffset可以获得2号簇的起始扇区，然后通过RootDirectoryCluster，即可获得根目录的起始扇区
			DWORD dwRootDirectoryStartSector = exfatInfo.dwClusterHeapOffset 
				+ (exfatInfo.dwRootDirectoryCluster - 2) * exfatInfo.dwSectorsPerCluster;
			BYTE *pRoot = new BYTE[exfatInfo.wBytesPerSector];
			ZeroMemory(pRoot,exfatInfo.wBytesPerSector);

			ULONGLONG ullTempStartSector = exfatInfo.ullPartitionOffset + dwRootDirectoryStartSector;
			if (!ReadSectors(m_hMaster,ullTempStartSector,1,exfatInfo.wBytesPerSector,pRoot,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

			// 第二步，获得BitMap起始扇区
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
			if (!ReadSectors(m_hMaster,ullTempStartSector,dwBitMapLength/exfatInfo.wBytesPerSector,exfatInfo.wBytesPerSector,pBitMap,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

			// 第三步，分析哪些是有效簇,注意从2号簇开始
			// 把2号簇之前的数据添加进有效数据列表
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

				// EXFAT 和NTFS共用一个判断有效簇的方法
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

			// 分析有效cu
			// Step1.找到6号MFT项，即BITMAP项，每个MFT项占1024个字节
			ULONGLONG ullBitMapMFTStartSector = ntfsInfo.ullStartSector + ntfsInfo.ullStartCluserOfMFT * ntfsInfo.bySectorsPerCluster
				+ 1024 / ntfsInfo.wBytesPerSectors * 6;

			// Step2 找BITMAP的DATA属性，即属性类型为0x80
			BYTE *pMFT = new BYTE[1024];
			ZeroMemory(pMFT,1024);
			if (!ReadSectors(m_hMaster,ullBitMapMFTStartSector,1024/ntfsInfo.wBytesPerSectors,ntfsInfo.wBytesPerSectors,pMFT,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

					// 第一部分：第一个字节，高四位表示簇流起始位置的字节数（起始簇号），低四位表示簇流长度的字节数（簇数）
					// 第二部分：簇流包含的簇数
					// 第三部分：簇流起始簇号
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

			// Step3 找到Bitmap位置，确定哪些是有效簇
			ULONGLONG ullBitMapDataStartSector = ntfsInfo.ullStartSector + ullBitMapDataStartCluster * ntfsInfo.bySectorsPerCluster;
			DWORD dwLengthPerCluster = ntfsInfo.bySectorsPerCluster * ntfsInfo.wBytesPerSectors;

			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = ntfsInfo.ullStartSector;
			effData.ullSectors = ntfsInfo.bySectorsPerCluster;
			effData.wBytesPerSector = ntfsInfo.wBytesPerSectors;

			// 0号簇单独添加
			m_EffList.AddTail(effData);

			effData.ullStartSector = ntfsInfo.ullStartSector + ntfsInfo.bySectorsPerCluster;
			effData.ullSectors = 0;

			BYTE *pBitmap = new BYTE[dwLengthPerCluster];

			ULONGLONG ullClusterIndex = 0;
			for (DWORD i = 0;i < dwBitMapDataClusters;i++)
			{
				ZeroMemory(pBitmap,dwLengthPerCluster);
				ULONGLONG ullTempStart = ullBitMapDataStartSector + i * ntfsInfo.bySectorsPerCluster;
				if (!ReadSectors(m_hMaster,ullTempStart,ntfsInfo.bySectorsPerCluster,ntfsInfo.wBytesPerSectors,pBitmap,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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
					// 0号簇已经添加
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

			if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pSuperBlock,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

			// 添加超级块之前的块到有效数据列表
			EFF_DATA effData;
			ZeroMemory(&effData,sizeof(EFF_DATA));
			effData.ullStartSector = ullStartSector;
			effData.ullSectors = superBlockInfo.dwStartBlockOfGroup0 * superBlockInfo.dwSectorsPerBlock;
			effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

			if (effData.ullSectors != 0)
			{
				m_EffList.AddTail(effData);
			}

			// 组块的总数
			DWORD dwTotalGroupBlocks = superBlockInfo.dwBlocksOfVolume / superBlockInfo.dwBlocksPerBlockGroup;
			if (superBlockInfo.dwBlocksOfVolume % superBlockInfo.dwBlocksPerBlockGroup)
			{
				dwTotalGroupBlocks++;
			}

			// 定位组描述表的扇区

			// 超级块的块号
			DWORD dwSuperBlockNum = 1;
			if (superBlockInfo.dwBytesPerBlock / 1024 > 1)
			{
				dwSuperBlockNum = 0;
			}

			// 组描述表的块号，组描述表起始于超级块所在块的下一块
			DWORD dwGroupDescriptionBlockNum = dwSuperBlockNum + 1;
			DWORD dwGroupDescriptionStartSector = dwGroupDescriptionBlockNum * superBlockInfo.dwSectorsPerBlock;

			// 组描述符表占N的块，而非一个块，根据块组大小决定 2014-06-27 Binggoo

			DWORD dwGroupDescriptionBlocks = dwTotalGroupBlocks * 0x20 / superBlockInfo.dwBytesPerBlock;
			if (dwTotalGroupBlocks * 0x20 % superBlockInfo.dwBytesPerBlock != 0)
			{
				dwGroupDescriptionBlocks++;
			}

			DWORD dwGroupDescriptionLen = dwGroupDescriptionBlocks * superBlockInfo.dwBytesPerBlock;
			BYTE *pGroupDescrption = new BYTE[dwGroupDescriptionLen];
			ZeroMemory(pGroupDescrption,dwGroupDescriptionLen);

			ullTempStartSector = ullStartSector + dwGroupDescriptionStartSector;
			if (!ReadSectors(m_hMaster,ullTempStartSector,dwGroupDescriptionBlocks * superBlockInfo.dwSectorsPerBlock,m_dwBytesPerSector,pGroupDescrption,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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
				// 块位图的起始位置和大小
				DWORD dwBlockBitMapStartBlock = (DWORD)ReadOffset(pGroupDescrption,offset,4);
				DWORD dwBlockBitMapStartSector = dwBlockBitMapStartBlock * superBlockInfo.dwSectorsPerBlock;
				DWORD dwBlockBitMapLength = superBlockInfo.dwBlocksPerBlockGroup / 8;

				BYTE *pBitMap = new BYTE[dwBlockBitMapLength];
				ZeroMemory(pBitMap,dwBlockBitMapLength);

				ullTempStartSector = ullStartSector + dwBlockBitMapStartSector;
				if (!ReadSectors(m_hMaster,ullTempStartSector,dwBlockBitMapLength/m_dwBytesPerSector,m_dwBytesPerSector,pBitMap,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

				// 块位图只描述本块组内的块的分配情况
				effData.ullStartSector = ullStartSector 
					+ superBlockInfo.dwStartBlockOfGroup0 * superBlockInfo.dwSectorsPerBlock 
					+ group * superBlockInfo.dwBlocksPerBlockGroup * superBlockInfo.dwSectorsPerBlock;
				effData.ullSectors = 0;

				// 判断有效块
				for (DWORD block = 0;block < superBlockInfo.dwBlocksPerBlockGroup;block++)
				{
					// 最后一个块组的块数有可能小于实际每个组块的块数
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

				if (ebr.Partition[i].PartitionType == 0x0F) // 主扩展分区
				{ 
					ullMasterSectorOffset = ullTempStartSector;
				}
				else if (ebr.Partition[i].PartitionType == 0x05) // 二级扩展分区
				{
					ullTempStartSector += ullMasterSectorOffset;
				}
				else
				{
					ullTempStartSector += ullStartSector;
				}

				if (!ReadSectors(m_hMaster,ullTempStartSector,1,m_dwBytesPerSector,pDBRTemp,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

				if (ebr.Partition[i].PartitionType == 0x05 || ebr.Partition[i].PartitionType == 0x0F) // 扩展分区
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
			// 不识别的文件系统，拷贝所有资料
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
	// 坏簇标志 FAT12 - 0xFF7, FAT16 - 0xFFF7, FAT32 - 0x0FFFFFF7
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
	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwErrorCode;

	m_bEnd = TRUE;

	// 是否要执行全盘擦除
	if (m_bCleanDiskFirst)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Clean disk first......"));

		m_CleanMode = CleanMode_Full;

		char function[255] = {NULL};

		m_bEnd = FALSE;

		//子盘写线程

		for (int times = 0; times < m_nCleanTimes;times++)
		{
			sprintf_s(function,"DISK CLEAN %d/%d",times+1,m_nCleanTimes);
			::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

			m_nFillValue = m_pCleanValues[times];

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy - DISK CLEAN %d/%d,fill value %d"),times+1,m_nCleanTimes,m_nFillValue);

			UINT nCurrentTargetCount = GetCurrentTargetCount();
			HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

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

			//等待线程结束
			WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);

			bResult = FALSE;
			for (UINT i = 0; i < nCount;i++)
			{
				GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
				bResult |= dwErrorCode;
				CloseHandle(hWriteThreads[i]);
				hWriteThreads[i] = NULL;
			}
			delete []hWriteThreads;

			if (!bResult)
			{
				return FALSE;
			}

			//是写过程中比较还是写之后比较
			if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_After)
			{
				sprintf_s(function,"COMPARE CLEAN %d/%d",times+1,m_nCleanTimes);
				::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy - COMPARE CLEAN %d/%d"),times+1,m_nCleanTimes);

				nCurrentTargetCount = GetCurrentTargetCount();
				HANDLE *hCompareCleanThreads = new HANDLE[nCurrentTargetCount];

				nCount = 0;
				pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected() && port->GetResult())
					{
						LPVOID_PARM lpVoid = new VOID_PARM;
						lpVoid->lpVoid1 = this;
						lpVoid->lpVoid2 = port;

						hCompareCleanThreads[nCount] = CreateThread(NULL,0,CompareCleanThreadProc,lpVoid,0,&dwWriteId);

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateCompareCleanThread,ThreadId=0x%04X,HANDLE=0x%04X")
							,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

						nCount++;
					}
				}

				//等待线程结束
				WaitForMultipleObjects(nCount,hCompareCleanThreads,TRUE,INFINITE);

				bResult = FALSE;
				for (UINT i = 0; i < nCount;i++)
				{
					GetExitCodeThread(hCompareCleanThreads[i],&dwErrorCode);
					bResult |= dwErrorCode;
					CloseHandle(hCompareCleanThreads[i]);
					hCompareCleanThreads[i] = NULL;
				}
				delete []hCompareCleanThreads;

				if (!bResult)
				{
					return FALSE;
				}
			}

		}

		if (m_WorkMode == WorkMode_FullCopy)
		{
			strcpy_s(function,"FULL COPY");
		}
		else
		{
			strcpy_s(function,"QUICK COPY");
		}
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Clean disk completed"));
	}

	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			EFF_DATA effData;
			effData.ullStartSector = 0;
			effData.ullSectors = m_ullSectorNums;
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

	HANDLE hReadThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		dataQueue->Clear();
		if (port->IsConnected() && port->GetResult())
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

	//等待写磁盘线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);

	bResult = FALSE;
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bCompare)
	{
		m_bVerify = TRUE;
		char function[255] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		m_bEnd = TRUE;

		nCurrentTargetCount = GetCurrentTargetCount();

		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];

		// 如果允许容量误差，并且设置的误差不是0，则需要安装Sector by Sector的方式进行比较
		if (m_bAllowCapGap && m_nCapGapPercent != 0)
		{
			m_CompareMethod = CompareMethod_Byte;
		}

		if (m_CompareMethod == CompareMethod_Byte)
		{
			m_bComputeHash = FALSE;

			hReadThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwReadId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read Disk(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,dwReadId,hReadThread);

			nCount = 0;
			pos = m_TargetPorts->GetHeadPosition();
			posQueue = m_DataQueueList.GetHeadPosition();
			while (pos && posQueue)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
				dataQueue->Clear();
				if (port->IsConnected() && port->GetResult())
				{
					LPVOID_PARM lpVoid = new VOID_PARM;
					lpVoid->lpVoid1 = this;
					lpVoid->lpVoid2 = port;
					lpVoid->lpVoid3 = dataQueue;

					hVerifyThreads[nCount] = CreateThread(NULL,0,VerifySectorThreadProc,lpVoid,0,&dwVerifyId);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskSectorThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

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

			//等待读磁盘线程结束
			WaitForSingleObject(hReadThread,INFINITE);
			GetExitCodeThread(hReadThread,&dwErrorCode);
			bResult &= dwErrorCode;
			CloseHandle(hReadThread);
		}
		else
		{
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
		}

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

BOOL CDisk::OnCopyImage()
{
	IMAGE_HEADER imgHead = {0};
	DWORD dwErrorCode = 0;
	DWORD dwLen = SIZEOF_IMAGE_HEADER;


	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwUncompressId;

	m_bEnd = TRUE;

	// 是否要执行全盘擦除
	if (m_bCleanDiskFirst)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Clean disk first......"));

		m_CleanMode = CleanMode_Full;
		char function[255] = {NULL};

		m_bEnd = FALSE;

		//子盘写线程

		for (int times = 0; times < m_nCleanTimes;times++)
		{
			sprintf_s(function,"DISK CLEAN %d/%d",times+1,m_nCleanTimes);
			::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

			m_nFillValue = m_pCleanValues[times];

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy - DISK CLEAN %d/%d,fill value %d"),times+1,m_nCleanTimes,m_nFillValue);

			UINT nCurrentTargetCount = GetCurrentTargetCount();
			HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

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

			//等待线程结束
			WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
			for (UINT i = 0; i < nCount;i++)
			{
				GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
				bResult |= dwErrorCode;
				CloseHandle(hWriteThreads[i]);
				hWriteThreads[i] = NULL;
			}
			delete []hWriteThreads;

			if (!bResult)
			{
				return FALSE;
			}

			//是写过程中比较还是写之后比较
			if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_After)
			{
				sprintf_s(function,"COMPARE CLEAN %d/%d",times+1,m_nCleanTimes);
				::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Copy - COMPARE CLEAN %d/%d"),times+1,m_nCleanTimes);

				nCurrentTargetCount = GetCurrentTargetCount();
				HANDLE *hCompareCleanThreads = new HANDLE[nCurrentTargetCount];

				nCount = 0;
				pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected() && port->GetResult())
					{
						LPVOID_PARM lpVoid = new VOID_PARM;
						lpVoid->lpVoid1 = this;
						lpVoid->lpVoid2 = port;

						hCompareCleanThreads[nCount] = CreateThread(NULL,0,CompareCleanThreadProc,lpVoid,0,&dwWriteId);

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateCompareCleanThread,ThreadId=0x%04X,HANDLE=0x%04X")
							,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

						nCount++;
					}
				}

				//等待线程结束
				WaitForMultipleObjects(nCount,hCompareCleanThreads,TRUE,INFINITE);

				bResult = FALSE;
				for (UINT i = 0; i < nCount;i++)
				{
					GetExitCodeThread(hCompareCleanThreads[i],&dwErrorCode);
					bResult |= dwErrorCode;
					CloseHandle(hCompareCleanThreads[i]);
					hCompareCleanThreads[i] = NULL;
				}
				delete []hCompareCleanThreads;

				if (!bResult)
				{
					return FALSE;
				}
			}
		}


		strcpy_s(function,"IMAGE COPY");

		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Clean disk completed"));

	}

	if (m_bServerFirst)
	{
		USES_CONVERSION;
		CString strImageName = CUtils::GetFileName(m_MasterPort->GetFileName());
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

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));


			return FALSE;
		}

		delete []bufSend;

		QUERY_IMAGE_OUT queryImageOut = {0};
		dwLen = sizeof(QUERY_IMAGE_OUT);
		if (!Recv(m_ClientSocket,(char *)&queryImageOut,dwLen,NULL,&dwErrorCode))
		{

			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query image from server error,filename=%s,system errorcode=%ld,%s")
				,strImageName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}


		if (queryImageOut.dwErrorCode != 0 || queryImageOut.dwCmdOut != CMD_QUERY_IMAGE_OUT || queryImageOut.dwSizeSend != dwLen)
		{

			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_Custom,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query image from server error,filename=%s,custom errorcode=0x%X,get data from server error")
				,strImageName,dwErrorCode);

			return FALSE;
		}

		memcpy(&imgHead,queryImageOut.byHead,SIZEOF_IMAGE_HEADER);

		if (m_hMaster != INVALID_HANDLE_VALUE)
		{
			dwLen = SIZEOF_IMAGE_HEADER;
			WriteFileAsyn(m_hMaster,0,dwLen,(LPBYTE)&imgHead,m_MasterPort->GetOverlapped(FALSE),&dwErrorCode);
		}

	}
	else
	{
		if (!ReadFileAsyn(m_hMaster,0,dwLen,(LPBYTE)&imgHead,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			m_MasterPort->SetEndTime(CTime::GetCurrentTime());
			m_MasterPort->SetPortState(PortState_Fail);
			m_MasterPort->SetResult(FALSE);
			m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read Image Head error,filename=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			return FALSE;
		}
	}

	m_ullValidSize = imgHead.ullValidSize;
	m_ullSectorNums = imgHead.ullCapacitySize / imgHead.dwBytesPerSector;
	m_ullCapacity = imgHead.ullImageSize;
	m_dwBytesPerSector = imgHead.dwBytesPerSector;

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

	HANDLE hReadThread = CreateThread(NULL,0,ReadImageThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image(%s) - CreateReadImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetFileName(),dwReadId,hReadThread);

	// 创建多个解压缩线程
	HANDLE hUncompressThread = CreateThread(NULL,0,UnCompressThreadProc,this,0,&dwUncompressId);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image - CreateUncompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwUncompressId,hUncompressThread);

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		dataQueue->Clear();
		if (port->IsConnected() && port->GetResult())
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

	// 等待解压缩线程结束
	WaitForSingleObject(hUncompressThread,INFINITE);
	CloseHandle(hUncompressThread);

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bCompare)
	{
		m_bVerify = TRUE;
		char function[255] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		nCurrentTargetCount = GetCurrentTargetCount();
		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];

		if (m_CompareMethod == CompareMethod_Byte)
		{
			m_bComputeHash = FALSE;
			m_bServerFirst = FALSE;

			hReadThread = CreateThread(NULL,0,ReadImageThreadProc,this,0,&dwReadId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Verfiy Image(%s) - CreateReadImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,m_MasterPort->GetFileName(),dwReadId,hReadThread);

			// 创建多个解压缩线程
			hUncompressThread = CreateThread(NULL,0,UnCompressThreadProc,this,0,&dwUncompressId);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Verify Image - CreateUncompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,dwUncompressId,hUncompressThread);


			nCount = 0;
			pos = m_TargetPorts->GetHeadPosition();
			posQueue = m_DataQueueList.GetHeadPosition();
			while (pos && posQueue)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
				dataQueue->Clear();
				if (port->IsConnected() && port->GetResult())
				{
					LPVOID_PARM lpVoid = new VOID_PARM;
					lpVoid->lpVoid1 = this;
					lpVoid->lpVoid2 = port;
					lpVoid->lpVoid3 = dataQueue;

					hVerifyThreads[nCount] = CreateThread(NULL,0,VerifySectorThreadProc,lpVoid,0,&dwVerifyId);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskSectorThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

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

			// 等待解压缩线程结束
			WaitForSingleObject(hUncompressThread,INFINITE);
			CloseHandle(hUncompressThread);

			//等待读磁盘线程结束
			WaitForSingleObject(hReadThread,INFINITE);
			GetExitCodeThread(hReadThread,&dwErrorCode);
			bResult &= dwErrorCode;
			CloseHandle(hReadThread);

		}
		else
		{
			nCount = 0;
			pos = m_TargetPorts->GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				if (port->IsConnected() && port->GetResult())
				{
					CHashMethod *pHashMethod;

					port->SetHashMethod(hashMethod);

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

			// 等待校验线程结束
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

BOOL CDisk::OnMakeImage()
{
	if (m_bQuickImage)
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
	else
	{
		EFF_DATA effData;
		effData.ullStartSector = 0;
		effData.ullSectors = m_ullSectorNums;
		effData.wBytesPerSector = (WORD)m_dwBytesPerSector;

		m_EffList.AddTail(effData);

		m_ullValidSize = GetValidSize();
		SetValidSize(m_ullValidSize);
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
	DWORD dwReadId,dwWriteId,dwCompressId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image Make(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	HANDLE hCompressThread = CreateThread(NULL,0,CompressThreadProc,this,0,&dwCompressId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress Image - CreateCompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwCompressId,hCompressThread);


	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		dataQueue->Clear();
		if (port->IsConnected() && port->GetResult())
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

	//等待写映像线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	//等待压缩线程结束
	WaitForSingleObject(hCompressThread,INFINITE);
	CloseHandle(hCompressThread);

	//等待读磁盘线程结束
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
			effData.ullSectors = m_ullSectorNums;
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

	HANDLE hMasterVerifyThread = NULL;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];
	UINT nCount = 0;

	if (m_CompareMethod = CompareMethod_Byte)
	{
		m_bComputeHash = FALSE;

		hMasterVerifyThread = CreateThread(NULL,0,ReadDiskThreadProc,this,0,&dwVerifyId);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Compare(Master) - CreateReadDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,dwVerifyId,hMasterVerifyThread);

		pos = m_TargetPorts->GetHeadPosition();
		POSITION posQueue = m_DataQueueList.GetHeadPosition();
		while (pos && posQueue)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
			CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
			dataQueue->Clear();
			if (port->IsConnected() && port->GetResult())
			{
				LPVOID_PARM lpVoid = new VOID_PARM;
				lpVoid->lpVoid1 = this;
				lpVoid->lpVoid2 = port;
				lpVoid->lpVoid3 = dataQueue;

				hVerifyThreads[nCount] = CreateThread(NULL,0,VerifySectorThreadProc,lpVoid,0,&dwVerifyId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskSectorThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

				nCount++;
			}
		}
	}
	else
	{
		//母盘读线程
		LPVOID_PARM lpMasterVoid = new VOID_PARM;
		lpMasterVoid->lpVoid1 = this;
		lpMasterVoid->lpVoid2 = m_MasterPort;
		lpMasterVoid->lpVoid3 = m_pMasterHashMethod;

		hMasterVerifyThread = CreateThread(NULL,0,VerifyThreadProc,lpMasterVoid,0,&dwVerifyId);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwVerifyId,hMasterVerifyThread);

		//子盘读线程
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
	}

	//等待校验线程结束
	WaitForMultipleObjects(nCount,hVerifyThreads,TRUE,INFINITE);
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

	//等待母盘读线程结束
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

	//子盘写线程
	if (m_CleanMode == CleanMode_Safe)
	{
		m_CleanMode = CleanMode_Full;

		m_nCleanTimes = 7;
		m_pCleanValues = new int[7];
		m_pCleanValues[0] = 0;
		m_pCleanValues[1] = 0xFF;
		m_pCleanValues[2] = RANDOM_VALUE;
		m_pCleanValues[3] = 0x96;
		m_pCleanValues[4] = 0;
		m_pCleanValues[5] = 0xFF;
		m_pCleanValues[6] = RANDOM_VALUE;

		char function[255] = {NULL};
		m_bEnd = FALSE;
		for (int times = 0; times < m_nCleanTimes;times++)
		{
			sprintf_s(function,"DISK CLEAN %d/%d",times+1,m_nCleanTimes);
			::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

			if (times == m_nCleanTimes - 1)
			{
				m_bEnd = TRUE;
			}

			m_nFillValue = m_pCleanValues[times];

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("DISK CLEAN %d/%d,fill value %d"),times+1,m_nCleanTimes,m_nFillValue);

			UINT nCount = 0;
			UINT nCurrentTargetCount = GetCurrentTargetCount();
			HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

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

			//等待线程结束
			WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
			for (UINT i = 0; i < nCount;i++)
			{
				GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
				bResult |= dwErrorCode;
				CloseHandle(hWriteThreads[i]);
				hWriteThreads[i] = NULL;
			}
			delete []hWriteThreads;

			if (!bResult)
			{
				return FALSE;
			}

			//是写过程中比较还是写之后比较
			if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_After)
			{
				sprintf_s(function,"COMPARE CLEAN %d/%d",times+1,m_nCleanTimes);
				::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Clean - COMPARE CLEAN %d/%d"),times+1,m_nCleanTimes);

				nCurrentTargetCount = GetCurrentTargetCount();
				HANDLE *hCompareCleanThreads = new HANDLE[nCurrentTargetCount];

				nCount = 0;
				pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected() && port->GetResult())
					{
						LPVOID_PARM lpVoid = new VOID_PARM;
						lpVoid->lpVoid1 = this;
						lpVoid->lpVoid2 = port;

						hCompareCleanThreads[nCount] = CreateThread(NULL,0,CompareCleanThreadProc,lpVoid,0,&dwWriteId);

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateCompareCleanThread,ThreadId=0x%04X,HANDLE=0x%04X")
							,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

						nCount++;
					}
				}

				//等待线程结束
				WaitForMultipleObjects(nCount,hCompareCleanThreads,TRUE,INFINITE);

				bResult = FALSE;
				for (UINT i = 0; i < nCount;i++)
				{
					GetExitCodeThread(hCompareCleanThreads[i],&dwErrorCode);
					bResult |= dwErrorCode;
					CloseHandle(hCompareCleanThreads[i]);
					hCompareCleanThreads[i] = NULL;
				}
				delete []hCompareCleanThreads;

				if (!bResult)
				{
					return FALSE;
				}
			}
		}
	}
	else
	{

		if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_After)
		{
			m_bEnd = FALSE;
		}

		UINT nCount = 0;
		UINT nCurrentTargetCount = GetCurrentTargetCount();
		HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

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

		//等待线程结束
		WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);

		bResult = FALSE;
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hWriteThreads[i]);
			hWriteThreads[i] = NULL;
		}
		delete []hWriteThreads;

		if (!bResult)
		{
			return FALSE;
		}

		//是写过程中比较还是写之后比较
		if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_After)
		{
			m_bEnd = TRUE;

			char function[255] = {NULL};
			sprintf_s(function,"COMPARE CLEAN");
			::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Disk Clean - COMPARE CLEAN"));

			nCurrentTargetCount = GetCurrentTargetCount();
			HANDLE *hCompareCleanThreads = new HANDLE[nCurrentTargetCount];

			nCount = 0;
			pos = m_TargetPorts->GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				if (port->IsConnected() && port->GetResult())
				{
					LPVOID_PARM lpVoid = new VOID_PARM;
					lpVoid->lpVoid1 = this;
					lpVoid->lpVoid2 = port;

					hCompareCleanThreads[nCount] = CreateThread(NULL,0,CompareCleanThreadProc,lpVoid,0,&dwWriteId);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateCompareCleanThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

					nCount++;
				}
			}

			//等待线程结束
			WaitForMultipleObjects(nCount,hCompareCleanThreads,TRUE,INFINITE);

			bResult = FALSE;
			for (UINT i = 0; i < nCount;i++)
			{
				GetExitCodeThread(hCompareCleanThreads[i],&dwErrorCode);
				bResult |= dwErrorCode;
				CloseHandle(hCompareCleanThreads[i]);
				hCompareCleanThreads[i] = NULL;
			}
			delete []hCompareCleanThreads;

			if (!bResult)
			{
				return FALSE;
			}
		}
	}

	return bResult;
}

BOOL CDisk::OnCopyFiles()
{
	m_bServerFirst = FALSE;
	m_ullValidSize = 0;
	m_MapCopyFiles.RemoveAll();

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic start......"));

	int nFilesCount = m_FileArray.GetCount();
	for (int i = 0; i < nFilesCount;i++)
	{
		CString strFilePath = m_FileArray.GetAt(i);

		DWORD dwErrorCode = 0;
		HANDLE hFile = GetHandleOnFile(strFilePath,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,&dwErrorCode);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER liFileSize = {0};

			GetFileSizeEx(hFile,&liFileSize);

			m_ullValidSize += liFileSize.QuadPart;

			m_MapCopyFiles.SetAt(strFilePath,liFileSize.QuadPart);

			CloseHandle(hFile);
		}
	}

	int nFolderCount = m_FodlerArray.GetCount();
	for (int i = 0;i < nFolderCount;i++)
	{
		CString strFolder = m_FodlerArray.GetAt(i);

		EnumFile(strFolder);
	}

	SetValidSize(m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Files statistic end, total files count=%d, total size=%I64d")
		,m_MapCopyFiles.GetCount(),m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Copy Files List:"));
	CString strList;
	POSITION pos = m_MapCopyFiles.GetStartPosition();
	CString strFile;
	ULONGLONG ullFileSize = 0;
	while (pos)
	{
		m_MapCopyFiles.GetNextAssoc(pos,strFile,ullFileSize);
		strList.Format(_T("FilePath:%s  FileSize:%I64d"),strFile,ullFileSize);
		CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
	}

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadFilesThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("File Copy(Master) - CreateReadFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		dataQueue->Clear();
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteFilesThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateWriteFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

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
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bCompare)
	{
		m_bVerify = TRUE;
		char function[255] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		nCurrentTargetCount = GetCurrentTargetCount();

		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];

		if (m_CompareMethod == CompareMethod_Byte)
		{
			m_bComputeHash = FALSE;

			hReadThread = CreateThread(NULL,0,ReadFilesThreadProc,this,0,&dwReadId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("File Copy(Master) - CreateReadFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,dwReadId,hReadThread);

			nCount = 0;
			pos = m_TargetPorts->GetHeadPosition();
			posQueue = m_DataQueueList.GetHeadPosition();
			while (pos && posQueue)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
				dataQueue->Clear();
				if (port->IsConnected() && port->GetResult())
				{
					LPVOID_PARM lpVoid = new VOID_PARM;
					lpVoid->lpVoid1 = this;
					lpVoid->lpVoid2 = port;
					lpVoid->lpVoid3 = dataQueue;

					hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyFilesByteThreadProc,lpVoid,0,&dwVerifyId);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyFilesByteThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

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

			//等待读磁盘线程结束
			WaitForSingleObject(hReadThread,INFINITE);
			GetExitCodeThread(hReadThread,&dwErrorCode);
			bResult &= dwErrorCode;
			CloseHandle(hReadThread);

		}
		else
		{
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

					hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyFilesThreadProc,lpVoid,0,&dwVerifyId);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

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
		}


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

BOOL CDisk::OnFormatDisk()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;

	//子盘写线程
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];
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

			hWriteThreads[nCount] = CreateThread(NULL,0,FormatDiskThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFormatDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//等待线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	return bResult;
}

BOOL CDisk::OnDiffCopy()
{
	m_ullValidSize = 0;
	m_ullHashFilesSize = 0;
	m_MapCopyFiles.RemoveAll();
	m_ArrayDelFiles.RemoveAll();
	CleanMapPortStringArray();
	m_MapHashDestFiles.RemoveAll();
	m_MapHashSourceFiles.RemoveAll();
	m_MapSizeDestFiles.RemoveAll();
	m_MapSizeSourceFiles.RemoveAll();
	m_ArraySameFile.RemoveAll();
	m_ArrayDelFolders.RemoveAll();
	m_MapSourceFolders.RemoveAll();
	m_MapDestFolders.RemoveAll();

	BOOL bResult = FALSE;
	DWORD dwErrorCode = 0;

	if (m_bUploadUserLog)
	{
		char function[255] = "USER LOG";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Search user log..."));

		DWORD dwSearchThreadId = 0;
		UINT nCurrentTargetCount = GetCurrentTargetCount();
		HANDLE *hSearchThreads = new HANDLE[nCurrentTargetCount];

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

				hSearchThreads[nCount] = CreateThread(NULL,0,SearchUserLogThreadProc,lpVoid,0,&dwSearchThreadId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateSearchUserLogThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwSearchThreadId,hSearchThreads[nCount]);

				nCount++;
			}
		}

		//等待线程结束
		WaitForMultipleObjects(nCount,hSearchThreads,TRUE,INFINITE);
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hSearchThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hSearchThreads[i]);
		}
		delete []hSearchThreads;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Upload user log..."));

		bResult = UploadUserLog();

		CleanMapPortStringArray();

		strcpy_s(function,"DIFF. COPY");
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);
	}

	if (m_nSourceType == SourceType_Package)
	{
		if (m_bServerFirst)
		{
			//第一步查询package大小
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query package..."));

			USES_CONVERSION;
			CString strPkgName = CUtils::GetFileName(m_MasterPort->GetFileName());
			char *fileName = W2A(strPkgName);

			DWORD dwLen = sizeof(CMD_IN) + strlen(fileName) + 2;

			BYTE *bufSend = new BYTE[dwLen];
			ZeroMemory(bufSend,dwLen);
			bufSend[dwLen - 1] = END_FLAG;

			CMD_IN queryPkgIn = {0};
			queryPkgIn.dwCmdIn = CMD_QUERY_PKG_IN;
			queryPkgIn.dwSizeSend = dwLen;

			memcpy(bufSend,&queryPkgIn,sizeof(CMD_IN));
			memcpy(bufSend + sizeof(CMD_IN),fileName,strlen(fileName));

			if (!Send(m_ClientSocket,(char *)bufSend,dwLen,NULL,&dwErrorCode))
			{
				delete []bufSend;

				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query package from server error,Pkgname=%s,system errorcode=%ld,%s")
					,strPkgName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));


				return FALSE;
			}

			delete []bufSend;

			QUERY_PKG_OUT queryPkgOut = {0};
			dwLen = sizeof(QUERY_PKG_OUT);
			if (!Recv(m_ClientSocket,(char *)&queryPkgOut,dwLen,NULL,&dwErrorCode))
			{

				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_System,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query package from server error,Pkgname=%s,system errorcode=%ld,%s")
					,strPkgName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				POSITION pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected())
					{
						port->SetEndTime(CTime::GetCurrentTime());
						port->SetPortState(PortState_Fail);
						port->SetResult(FALSE);
						port->SetErrorCode(ErrorType_System,dwErrorCode);
					}
				}

				return FALSE;
			}

			if ((queryPkgOut.dwErrorCode != 0 && queryPkgOut.dwErrorCode != 2 && queryPkgOut.dwErrorCode != 18)
				|| queryPkgOut.dwCmdOut != CMD_QUERY_PKG_OUT || queryPkgOut.dwSizeSend != dwLen)
			{

				dwErrorCode = CustomError_Get_Data_From_Server_Error;
				m_MasterPort->SetEndTime(CTime::GetCurrentTime());
				m_MasterPort->SetPortState(PortState_Fail);
				m_MasterPort->SetResult(FALSE);
				m_MasterPort->SetErrorCode(ErrorType_Custom,dwErrorCode);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query package from server error,Pkgname=%s,custom errorcode=0x%X,get data from server error")
					,strPkgName,dwErrorCode);

				POSITION pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected())
					{
						port->SetEndTime(CTime::GetCurrentTime());
						port->SetPortState(PortState_Fail);
						port->SetResult(FALSE);
						port->SetErrorCode(ErrorType_System,dwErrorCode);
					}
				}

				return FALSE;
			}

			m_ullValidSize = queryPkgOut.ullFileSize;
			m_ullCapacity = queryPkgOut.ullFileSize;

			// 第二步下载变更列表信息
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Query change list..."));
			if (QueryChangeList())
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Down change list..."));
				if (DownloadChangeList())
				{
					CString strChangeFile;
					strChangeFile.Format(_T(".\\%s.ini"),m_MasterPort->GetFileName());

					CIni ini(strChangeFile);
					ini.GetKeyLines(_T("DelFiles"),&m_ArrayDelFiles);
					ini.GetKeyLines(_T("DelFolders"),&m_ArrayDelFolders);

				}
			}

			// 第三步
		}
		else
		{
			//遍历母盘中的Package
			EnumFile(m_MasterPort->GetFileName());

			//获取ChangeLIST
			CString strChangeFile;
			strChangeFile.Format(_T("%s.ini"),m_MasterPort->GetFileName());

			CIni ini(strChangeFile);
			ini.GetKeyNames(_T("DelFiles"),&m_ArrayDelFiles);
		}
	}
	else
	{
		//第一步，遍历母盘和任一子盘中的文件
		char function[255] = "COMPARE";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Enum files start..."));

		bResult = TRUE;
		DWORD dwThreadId = 0;
		HANDLE hThreads[2] = {NULL};

		LPVOID_PARM lpVoidMaster = new VOID_PARM;
		lpVoidMaster->lpVoid1 = this;
		lpVoidMaster->lpVoid2 = m_MasterPort;
		lpVoidMaster->lpVoid3 = &m_MapSizeSourceFiles;
		lpVoidMaster->lpVoid4 = &m_MapSourceFolders;

		hThreads[0] = CreateThread(NULL,0,EnumFileThreadProc,lpVoidMaster,0,&dwThreadId); 

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Diff. Copy(Master) - CreateEnumFileThread,ThreadId=0x%04X,HANDLE=0x%04X")
			,dwThreadId,hThreads[0]);

		POSITION pos = m_TargetPorts->GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts->GetNext(pos);
			if (port->IsConnected() && port->GetResult())
			{
				LPVOID_PARM lpVoidTarget = new VOID_PARM;
				lpVoidTarget->lpVoid1 = this;
				lpVoidTarget->lpVoid2 = port;
				lpVoidTarget->lpVoid3 = &m_MapSizeDestFiles;
				lpVoidTarget->lpVoid4 = &m_MapDestFolders;

				hThreads[1] = CreateThread(NULL,0,EnumFileThreadProc,lpVoidTarget,0,&dwThreadId); 

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Diff. Copy(Target-%d) - CreateEnumFileThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortNum(),dwThreadId,hThreads[1]);
			}
		}

		// 等待遍历结束
		WaitForMultipleObjects(2,hThreads,TRUE,INFINITE);
		for (UINT i = 0; i < 2;i++)
		{
			GetExitCodeThread(hThreads[i],&dwErrorCode);
			bResult &= dwErrorCode;
			CloseHandle(hThreads[i]);
		}

		if (!bResult)
		{
			//失败则退出
			ErrorType errType = m_MasterPort->GetErrorCode(&dwErrorCode);

			if (dwErrorCode == 0)
			{
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
			}

			pos = m_TargetPorts->GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				if (port->IsConnected())
				{
					port->SetEndTime(CTime::GetCurrentTime());
					port->SetPortState(PortState_Fail);
					port->SetResult(FALSE);
					port->SetErrorCode(errType,dwErrorCode);
				}
			}

			return FALSE;
		}

		// 第二步，比较文件名称和大小,判断哪些是要删除，哪些是要拷贝
		CompareFileSize();

		if (m_nCompareRule != CompareRule_Fast)
		{
			//需要比较哈希值
			//创建计算哈希值线程

			SetValidSize(m_ullHashFilesSize);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compute files hash start..."));

			lpVoidMaster = new VOID_PARM;
			lpVoidMaster->lpVoid1 = this;
			lpVoidMaster->lpVoid2 = m_MasterPort;
			lpVoidMaster->lpVoid3 = &m_MapHashSourceFiles;


			hThreads[0] = CreateThread(NULL,0,ComputeHashThreadProc,lpVoidMaster,0,&dwThreadId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Diff. Copy(Master) - ComputeHashThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,dwThreadId,hThreads[0]);

			pos = m_TargetPorts->GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts->GetNext(pos);
				if (port->IsConnected() && port->GetResult())
				{
					LPVOID_PARM lpVoidTarget = new VOID_PARM;
					lpVoidTarget->lpVoid1 = this;
					lpVoidTarget->lpVoid2 = port;
					lpVoidTarget->lpVoid3 = &m_MapHashDestFiles;

					hThreads[1] = CreateThread(NULL,0,ComputeHashThreadProc,lpVoidTarget,0,&dwThreadId); 

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Diff. Copy(Target-%d) - ComputeHashThread,ThreadId=0x%04X,HANDLE=0x%04X")
						,port->GetPortNum(),dwThreadId,hThreads[1]);
				}
			}

			// 等待遍历结束
			WaitForMultipleObjects(2,hThreads,TRUE,INFINITE);
			for (UINT i = 0; i < 2;i++)
			{
				GetExitCodeThread(hThreads[i],&dwErrorCode);
				bResult &= dwErrorCode;
				CloseHandle(hThreads[i]);
			}

			if (!bResult)
			{
				//失败则退出
				ErrorType errType = m_MasterPort->GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
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
				}

				pos = m_TargetPorts->GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts->GetNext(pos);
					if (port->IsConnected())
					{
						port->SetEndTime(CTime::GetCurrentTime());
						port->SetPortState(PortState_Fail);
						port->SetResult(FALSE);
						port->SetErrorCode(errType,dwErrorCode);
					}
				}

				return FALSE;
			}

			// 比较文件HASH值
			CompareHashValue();
		}

		strcpy_s(function,"DIFF. COPY");
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);
	}

	SetValidSize(m_ullValidSize);

	if (!m_bServerFirst)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Files statistic: total files count=%d, total size=%I64d")
			,m_MapCopyFiles.GetCount(),m_ullValidSize);

		CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Copy Files List:"));
		CString strList;
		POSITION pos = m_MapCopyFiles.GetStartPosition();
		CString strFile;
		ULONGLONG ullFileSize = 0;
		while (pos)
		{
			m_MapCopyFiles.GetNextAssoc(pos,strFile,ullFileSize);
			strList.Format(_T("FilePath:%s  FileSize:%I64d"),strFile,ullFileSize);
			CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
		}
	}

	int nDelFileCount = m_ArrayDelFiles.GetCount();
	int nDelFolderCout = m_ArrayDelFolders.GetCount();

	if (nDelFileCount > 0 || nDelFolderCout > 0)
	{
		if (nDelFileCount > 0)
		{
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Delete Files List:"));

			for (int i = 0; i < nDelFileCount;i++)
			{
				CUtils::WriteLogFile(m_hLogFile,FALSE,m_ArrayDelFiles.GetAt(i));
			}
		}

		if (nDelFolderCout > 0)
		{
			CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Delete Folders List:"));

			for (int i = 0; i < nDelFolderCout;i++)
			{
				CUtils::WriteLogFile(m_hLogFile,FALSE,m_ArrayDelFolders.GetAt(i));
			}
		}


		// 创建删除文件线程
		char function[255] = "DELETE";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		DWORD dwDelThreadId = 0;
		UINT nCurrentTargetCount = GetCurrentTargetCount();
		HANDLE *hDelThreads = new HANDLE[nCurrentTargetCount];

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

				hDelThreads[nCount] = CreateThread(NULL,0,DeleteChangeThreadProc,lpVoid,0,&dwDelThreadId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateDeleteChangeThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwDelThreadId,hDelThreads[nCount]);

				nCount++;
			}
		}

		//等待线程结束
		WaitForMultipleObjects(nCount,hDelThreads,TRUE,INFINITE);
		for (UINT i = 0; i < nCount;i++)
		{
			GetExitCodeThread(hDelThreads[i],&dwErrorCode);
			bResult |= dwErrorCode;
			CloseHandle(hDelThreads[i]);
		}
		delete []hDelThreads;

		strcpy_s(function,"DIFF. COPY");
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);
	}

	// 开始拷贝文件
	bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId;

	HANDLE hReadThread = CreateThread(NULL,0,ReadFilesThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Diff. Copy - CreateReadFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwReadId,hReadThread);

	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	POSITION pos = m_TargetPorts->GetHeadPosition();
	POSITION posQueue = m_DataQueueList.GetHeadPosition();
	while (pos && posQueue)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		CDataQueue *dataQueue = m_DataQueueList.GetNext(posQueue);
		dataQueue->Clear();
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;
			lpVoid->lpVoid3 = dataQueue;

			hWriteThreads[nCount] = CreateThread(NULL,0,WriteFilesThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateWriteFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

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
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bCompare)
	{
		m_bVerify = TRUE;
		char function[255] = "VERIFY";
		::SendMessage(m_hWnd,WM_UPDATE_FUNCTION,(WPARAM)function,0);

		nCurrentTargetCount = GetCurrentTargetCount();
		HANDLE *hVerifyThreads = new HANDLE[nCurrentTargetCount];

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

				hVerifyThreads[nCount] = CreateThread(NULL,0,VerifyFilesThreadProc,lpVoid,0,&dwVerifyId);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
					,port->GetPortName(),port->GetDiskNum(),dwVerifyId,hVerifyThreads[nCount]);

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

BOOL CDisk::OnFullRWTest()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;
	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	if (m_bFormatFinished)
	{
		m_bEnd = FALSE;
	}

	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;

			hWriteThreads[nCount] = CreateThread(NULL,0,FullRWTestThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFullRWTestThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//等待线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	return bResult;
}

BOOL CDisk::OnFadePickerTest()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;
	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	if (m_bFormatFinished)
	{
		m_bEnd = FALSE;
	}

	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;

			hWriteThreads[nCount] = CreateThread(NULL,0,FadePickerThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFadePickerThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//等待线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
	}
	delete []hWriteThreads;

	return bResult;
}

BOOL CDisk::OnSpeedCheck()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;
	UINT nCount = 0;
	UINT nCurrentTargetCount = GetCurrentTargetCount();
	HANDLE *hWriteThreads = new HANDLE[nCurrentTargetCount];

	if (m_bFormatFinished)
	{
		m_bEnd = FALSE;
	}

	POSITION pos = m_TargetPorts->GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts->GetNext(pos);
		if (port->IsConnected() && port->GetResult())
		{
			LPVOID_PARM lpVoid = new VOID_PARM;
			lpVoid->lpVoid1 = this;
			lpVoid->lpVoid2 = port;

			hWriteThreads[nCount] = CreateThread(NULL,0,SpeedCheckThreadProc,lpVoid,0,&dwWriteId);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateSpeedCheckThread,ThreadId=0x%04X,HANDLE=0x%04X")
				,port->GetPortName(),port->GetDiskNum(),dwWriteId,hWriteThreads[nCount]);

			nCount++;
		}
	}

	//等待线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
		hWriteThreads[i] = NULL;
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
	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * m_dwBytesPerSector;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	// 如果要先擦除，此时需要等待擦除结束

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	POSITION pos = m_EffList.GetHeadPosition();

	while (pos && !*m_lpCancel && bResult && m_MasterPort->GetPortState() == PortState_Active)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);

		ullReadSectors = 0;
		ullRemainSectors = 0;
		ullStartSectors = effData.ullStartSector;
		dwSectors = m_nBlockSectors;
		dwLen = m_nBlockSectors * m_dwBytesPerSector;

		while (bResult && !*m_lpCancel && ullReadSectors < effData.ullSectors && ullStartSectors < m_ullSectorNums)
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

			ullRemainSectors = effData.ullSectors - ullReadSectors;

			if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
			{
				ullRemainSectors = m_ullSectorNums - ullStartSectors;
			}

			if (ullRemainSectors < m_nBlockSectors)
			{
				dwSectors = (DWORD)ullRemainSectors;

				dwLen = dwSectors * effData.wBytesPerSector;
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadSectors(m_hMaster,ullStartSectors,dwSectors,effData.wBytesPerSector,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
			{
				bResult = FALSE;
				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			else
			{
				PDATA_INFO dataInfo = new DATA_INFO;
				ZeroMemory(dataInfo,sizeof(DATA_INFO));
				dataInfo->ullOffset = ullStartSectors * effData.wBytesPerSector;
				dataInfo->dwDataSize = dwLen;
				dataInfo->pData = new BYTE[dwLen];
				memcpy(dataInfo->pData,pByte,dwLen);

				if (m_WorkMode == WorkMode_ImageMake)
				{
					m_CompressQueue.AddTail(dataInfo);
				}
				else
				{
					AddDataQueueList(dataInfo);
					delete []dataInfo->pData;
					delete dataInfo;
				}

				if (m_bComputeHash)
				{
					m_pMasterHashMethod->update((void *)pByte,dwLen);
				}

				delete []pByte;

				ullStartSectors += dwSectors;
				ullReadSectors += dwSectors;

				QueryPerformanceCounter(&t2);

				dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
				dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

				m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
				m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);
				m_MasterPort->AppendCompleteSize(dwLen);

			}

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

	m_MasterPort->SetResult(bResult);
	m_MasterPort->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (!m_bCompare || m_CompareMethod == CompareMethod_Hash)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

		if (m_bVerify)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

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
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),strHashMethod,m_strMasterHash);

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

	port->Active();

	// 判断容量
	if (port->GetTotalSize() < m_ullValidSize)
	{
		// 比允许的误差还要小
		if (!m_bAllowCapGap || port->GetTotalSize() < m_ullValidSize * (100-m_nCapGapPercent)/100)
		{
			port->SetEndTime(CTime::GetCurrentTime());
			port->SetResult(FALSE);
			port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
			port->SetPortState(PortState_Fail);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop copy,custom errorcode=0x%X,target is small")
				,port->GetPortName(),port->GetDiskNum(),CustomError_Target_Small);

			return FALSE;
		}	
	}
	else
	{
		//有效资料的最大扇区有没有超过子盘容量
		POSITION pos = m_EffList.GetHeadPosition();
		while (pos)
		{
			EFF_DATA effData = m_EffList.GetNext(pos);

			if ( (effData.ullStartSector + effData.ullSectors) * effData.wBytesPerSector > port->GetTotalSize() )
			{
				port->SetEndTime(CTime::GetCurrentTime());
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
				port->SetPortState(PortState_Fail);
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop copy,custom errorcode=0x%X,target is small")
					,port->GetPortName(),port->GetDiskNum(),CustomError_Target_Small);

				return FALSE;
			}
		}
	}

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	DWORD dwBytesPerSector = port->GetBytesPerSector();

	QueryPerformanceFrequency(&freq);

	// 是否需要先执行全盘擦除
	if (!m_bCleanDiskFirst)
	{
		QuickClean(hDisk,port,&dwErrorCode);
	}

	port->Active();

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult() && bResult && !port->IsKickOff())
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& (m_MasterPort->GetPortState() == PortState_Active || !m_bCompressComplete)
			&& port->GetResult() && !port->IsKickOff())
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
			bResult = FALSE;
			dwErrorCode = CustomError_Speed_Too_Slow;
			errType = ErrorType_Custom;
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

		// 如果子盘的容量小于有效资料的大小，并且已经达到允许误差了，不用继续拷贝，但是母口还在继续丢资料，所以要continue
		if (port->GetTotalSize() < m_ullValidSize)
		{
			if (m_bAllowCapGap && dataInfo->ullOffset >= port->GetTotalSize())
			{
				//为了保证进度条显示正确，实际完成资料量要继续添加
				port->AppendCompleteSize(dataInfo->dwDataSize);
				delete []dataInfo->pData;
				delete dataInfo;
				continue;
			}
		}

		ULONGLONG ullStartSectors = dataInfo->ullOffset / dwBytesPerSector;
		DWORD dwSectors = dataInfo->dwDataSize / dwBytesPerSector;

		QueryPerformanceCounter(&t1);
		if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytesPerSector,dataInfo->pData,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

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
			port->AppendCompleteSize(dataInfo->dwDataSize);

			delete []dataInfo->pData;
			delete dataInfo;
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

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
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


	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

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

BOOL CDisk::QuickClean(CPort *port,PDWORD pdwErrorCode )
{
	BOOL bResult = TRUE;
	HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,pdwErrorCode);

	BYTE *pByte = new BYTE[CLEAN_LENGTH];
	ZeroMemory(pByte,CLEAN_LENGTH);

	if (SetDiskAtrribute(hDisk,FALSE,TRUE,pdwErrorCode))
	{
		DWORD dwBytesPerSector = 0;
		ULONGLONG ullSectorNums = GetNumberOfSectors(hDisk,&dwBytesPerSector,NULL);
		DWORD dwSectors = CLEAN_LENGTH/dwBytesPerSector;
		DWORD dwSize = 0;

		if (!WriteSectors(hDisk,0,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(FALSE),pdwErrorCode))
		{
			bResult = FALSE;
		}

		if (!WriteSectors(hDisk,ullSectorNums-dwSectors,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(FALSE),pdwErrorCode))
		{
			bResult = FALSE;
		}

		if (bResult)
		{
			//fresh the partition table
			bResult = DeviceIoControl(
				hDisk,
				IOCTL_DISK_UPDATE_PROPERTIES,
				NULL,
				0,
				NULL,
				0,
				&dwSize,
				NULL
				);
			if (!bResult)
			{
				*pdwErrorCode = GetLastError();
			}
		}	

	}
	else
	{
		bResult = FALSE;
	}

	CloseHandle(hDisk);

	delete []pByte;

	return bResult;
}

BOOL CDisk::QuickClean( HANDLE hDisk,CPort *port,PDWORD pdwErrorCode )
{
	BOOL bResult = TRUE;
	BYTE *pByte = new BYTE[CLEAN_LENGTH];
	ZeroMemory(pByte,CLEAN_LENGTH);

	DWORD dwBytesPerSector = 0;
	ULONGLONG ullSectorNums = GetNumberOfSectors(hDisk,&dwBytesPerSector,NULL);
	DWORD dwSectors = CLEAN_LENGTH/dwBytesPerSector;
	DWORD dwSize = 0;

	if (!WriteSectors(hDisk,0,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(FALSE),pdwErrorCode))
	{
		bResult = FALSE;
	}

	if (!WriteSectors(hDisk,ullSectorNums-dwSectors,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(FALSE),pdwErrorCode))
	{
		bResult = FALSE;
	}

	delete []pByte;

	return bResult;
}

BOOL CDisk::VerifyDisk( CPort *port,CHashMethod *pHashMethod )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullReadSectors = 0;
	ULONGLONG ullRemainSectors = 0;
	ULONGLONG ullStartSectors = 0;
	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * BYTES_PER_SECTOR;

	port->Active();

	if (m_WorkMode == WorkMode_DiskCompare)
	{

		if (port->GetPortType() == PortType_TARGET_DISK)
		{
			// 判断容量
			if (port->GetTotalSize() < m_ullValidSize)
			{
				port->SetEndTime(CTime::GetCurrentTime());
				port->SetResult(FALSE);
				port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
				port->SetPortState(PortState_Fail);
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop compare,custom errorcode=0x%X,target is small")
					,port->GetPortName(),port->GetDiskNum(),CustomError_Target_Small);

				return FALSE;	
			}
			else
			{
				//有效资料的最大扇区有没有超过子盘容量
				POSITION pos = m_EffList.GetHeadPosition();
				while (pos)
				{
					EFF_DATA effData = m_EffList.GetNext(pos);

					if ( (effData.ullStartSector + effData.ullSectors) * effData.wBytesPerSector > port->GetTotalSize() )
					{
						port->SetEndTime(CTime::GetCurrentTime());
						port->SetResult(FALSE);
						port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
						port->SetPortState(PortState_Fail);
						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop compare,custom errorcode=0x%X,target is small")
							,port->GetPortName(),port->GetDiskNum(),CustomError_Target_Small);

						return FALSE;
					}
				}
			}
		}
	}

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

	// 设置只读属性
	SetDiskAtrribute(hDisk,TRUE,FALSE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	POSITION pos = m_EffList.GetHeadPosition();

	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active && !port->IsKickOff())
	{
		EFF_DATA effData = m_EffList.GetNext(pos);

		ullReadSectors = 0;
		ullRemainSectors = 0;
		ullStartSectors = effData.ullStartSector;
		dwSectors = m_nBlockSectors;
		dwLen = m_nBlockSectors * effData.wBytesPerSector;

		while (bResult && !*m_lpCancel && !port->IsKickOff() 
			&& ullReadSectors < effData.ullSectors && ullStartSectors < m_ullSectorNums)
		{
			QueryPerformanceCounter(&t0);

			ullRemainSectors = effData.ullSectors - ullReadSectors;

			if (ullRemainSectors + ullStartSectors > m_ullSectorNums)
			{
				ullRemainSectors = m_ullSectorNums - ullStartSectors;
			}

			if (ullRemainSectors < m_nBlockSectors)
			{
				dwSectors = (DWORD)ullRemainSectors;

				dwLen = dwSectors * effData.wBytesPerSector;
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadSectors(hDisk,ullStartSectors,dwSectors,effData.wBytesPerSector,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
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

				dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
				dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

				port->AppendUsedWaitTimeS(dbTimeWait);
				port->AppendUsedNoWaitTimeS(dbTimeNoWait);
				port->AppendCompleteSize(dwLen);

			}

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

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	if (bResult)
	{
		port->SetHash(pHashMethod->digest(),pHashMethod->getHashLength());

		// 如果是子盘，需要等待母盘结束
		if (port->GetPortType() == PortType_TARGET_DISK)
		{
			while (m_MasterPort->GetPortState() == PortState_Active)
			{
				SwitchToThread();
				Sleep(5);
			}

			// 比较HASH值
			CString strVerifyHash;
			for (int i = 0; i < pHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),pHashMethod->digest()[i]);
				strVerifyHash += strHash;
			}

			CString strHashMethod(pHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,port->GetPortName(),port->GetDiskNum(),strHashMethod,strVerifyHash);

			port->SetEndTime(CTime::GetCurrentTime());
			if (0 == m_strMasterHash.CompareNoCase(strVerifyHash))
			{
				bResult = TRUE;
				port->SetResult(TRUE);
				port->SetPortState(PortState_Pass);
			}
			else
			{
				bResult = FALSE;
				port->SetResult(FALSE);
				port->SetPortState(PortState_Fail);
				port->SetErrorCode(ErrorType_Custom,CustomError_Compare_Failed);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Verify FAIL,Target=%s,Master=%s")
					,port->GetPortName(),port->GetDiskNum(),strVerifyHash,m_strMasterHash);
			}
		}
		else if (port->GetPortType() == PortType_MASTER_DISK)
		{

			for (int i = 0; i < pHashMethod->getHashLength();i++)
			{
				CString strHash;
				strHash.Format(_T("%02X"),pHashMethod->digest()[i]);
				m_strMasterHash += strHash;
			}

			CString strHashMethod(pHashMethod->getHashMetod());
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,port->GetPortName(),port->GetDiskNum(),strHashMethod,m_strMasterHash);

			bResult = TRUE;
			port->SetResult(TRUE);
			port->SetEndTime(CTime::GetCurrentTime());
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

BOOL CDisk::VerifyDisk( CPort *port,CDataQueue *pDataQueue )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	port->Active();

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

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	DWORD dwBytesPerSector = port->GetBytesPerSector();

	QueryPerformanceFrequency(&freq);

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult() && bResult && !port->IsKickOff())
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& (m_MasterPort->GetPortState() == PortState_Active || !m_bCompressComplete)
			&& port->GetResult() && !port->IsKickOff())
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
			bResult = FALSE;
			dwErrorCode = CustomError_Speed_Too_Slow;
			errType = ErrorType_Custom;
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

		// 已经读到子盘末尾了，不用继续比较，但是母口还在继续丢资料，所以要continue
		if (dataInfo->ullOffset >= port->GetTotalSize())
		{
			port->AppendCompleteSize(dataInfo->dwDataSize);
			delete []dataInfo->pData;
			delete dataInfo;
			continue;
		}

		ULONGLONG ullStartSectors = dataInfo->ullOffset / dwBytesPerSector;
		DWORD dwSectors = dataInfo->dwDataSize / dwBytesPerSector;

		PBYTE pByte = new BYTE[dataInfo->dwDataSize];
		ZeroMemory(pByte,dataInfo->dwDataSize);

		QueryPerformanceCounter(&t1);
		if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytesPerSector,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []pByte;
			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}
		else
		{

			// 比较
			for (DWORD i = 0; i < dataInfo->dwDataSize;i++)
			{
				if (pByte[i] != dataInfo->pData[i])
				{
					//比较出错
					bResult = FALSE;
					errType = ErrorType_Custom;
					dwErrorCode = CustomError_Compare_Failed;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=sector %I64d, offset %d")
						,port->GetPortName(),port->GetDiskNum(),pByte[i],dataInfo->pData[i],ullStartSectors + i / dwBytesPerSector,i % dwBytesPerSector);

					break;
				}
			}

			QueryPerformanceCounter(&t2);

			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

			port->AppendUsedWaitTimeS(dbTimeWait);
			port->AppendUsedNoWaitTimeS(dbTimeNoWait);
			port->AppendCompleteSize(dataInfo->dwDataSize);

			delete []pByte;
			delete []dataInfo->pData;
			delete dataInfo;

			if (!bResult)
			{
				break;
			}
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

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
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

BOOL CDisk::ReadFiles()
{
	BOOL bRet;
	if (m_bServerFirst)
	{
		bRet = ReadRemoteFiles();
	}
	else
	{
		bRet = ReadLocalFiles();
	}

	return bRet;
}

BOOL CDisk::ReadLocalFiles()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwLen = m_nBlockSectors * BYTES_PER_SECTOR;
	ULONGLONG ullCompleteSize = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	POSITION pos = m_MapCopyFiles.GetStartPosition();
	ULONGLONG ullFileSize = 0;
	CString strSourceFile,strDestFile;
	while (pos && !*m_lpCancel && bResult && m_MasterPort->GetPortState() == PortState_Active)
	{
		m_MapCopyFiles.GetNextAssoc(pos,strSourceFile,ullFileSize);

		// 目标文件为去掉盘符剩余的文件
		strDestFile = strSourceFile.Right(strSourceFile.GetLength() - 2);

		HANDLE hFile = GetHandleOnFile(strSourceFile,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenFile error,file name=%s,system errorcode=%ld,%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),strSourceFile,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		ullCompleteSize = 0;
		dwLen = m_nBlockSectors * BYTES_PER_SECTOR;

		// 改成do{}while;为了使文件大小为0的文件也会被创建
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

			if (ullFileSize - ullCompleteSize < dwLen)
			{
				dwLen = (DWORD)(ullFileSize - ullCompleteSize);
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadFileAsyn(hFile,ullCompleteSize,dwLen,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
			{
				bResult = FALSE;
				delete []pByte;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
					,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			else
			{
				PDATA_INFO dataInfo = new DATA_INFO;
				ZeroMemory(dataInfo,sizeof(DATA_INFO));
				dataInfo->szFileName = new TCHAR[strDestFile.GetLength()+1];
				_tcscpy_s(dataInfo->szFileName,strDestFile.GetLength()+1,strDestFile);
				dataInfo->ullOffset = ullCompleteSize;
				dataInfo->dwDataSize = dwLen;
				dataInfo->pData = new BYTE[dwLen];
				memcpy(dataInfo->pData,pByte,dwLen);
				AddDataQueueList(dataInfo);

				delete []dataInfo->szFileName;
				delete []dataInfo->pData;
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
			}

		}while (bResult && !*m_lpCancel && ullCompleteSize < ullFileSize && m_MasterPort->GetPortState() == PortState_Active);

		CloseHandle(hFile);
	}

	if (m_MapCopyFiles.GetCount() == 0)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_No_File_Select;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,no file has been selected.")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode);
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),m_MasterPort->GetRealSpeed(),dwErrorCode);
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

	m_MasterPort->SetResult(bResult);
	m_MasterPort->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (!m_bCompare || m_CompareMethod == CompareMethod_Hash)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

		if (m_bVerify)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

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
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
				,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),strHashMethod,m_strMasterHash);

		}
	}
	else
	{
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}

BOOL CDisk::ReadRemoteFiles()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	ULONGLONG ullReadSize = 0,ullCompleteSize = 0;
	DWORD dwLen = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	WSAOVERLAPPED ol = {0};
	ol.hEvent = WSACreateEvent();

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	// 发送DOWN PACKAGE命令
	CString strPackageName = CUtils::GetFileName(m_MasterPort->GetFileName());

	USES_CONVERSION;
	char *pkgName = W2A(strPackageName);

	DWORD dwSendLen = sizeof(CMD_IN) + strlen(pkgName) + 2;
	BYTE *sendBuf = new BYTE[dwSendLen];
	ZeroMemory(sendBuf,dwSendLen);
	sendBuf[dwSendLen - 1] = END_FLAG;

	CMD_IN downPkgIn = {0};
	downPkgIn.dwCmdIn = CMD_DOWN_PKG_IN;
	downPkgIn.dwSizeSend = dwSendLen;

	memcpy(sendBuf,&downPkgIn,sizeof(CMD_IN));
	memcpy(sendBuf + sizeof(CMD_IN),pkgName,strlen(pkgName));

	while (bResult && !*m_lpCancel && ullReadSize < m_ullCapacity && m_MasterPort->GetPortState() == PortState_Active)
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

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send down package command error,Pkgname=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strPackageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		CMD_OUT downPkgOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&downPkgOut,dwLen,&ol,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down package error,Pkgname=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strPackageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = downPkgOut.dwSizeSend - sizeof(CMD_OUT);

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		DWORD dwRead = 0;

		while(dwRead < dwLen)
		{
			DWORD dwByteRead = dwLen - dwRead;
			if (!Recv(m_ClientSocket,(char *)(pByte+dwRead),dwByteRead,&ol,&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down package error,Pkgname=%s,Speed=%.2f,system errorcode=%ld,%s")
					,strPackageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
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

		if (downPkgOut.dwErrorCode != 0)
		{
			bResult = FALSE;
			errType = downPkgOut.errType;
			dwErrorCode = downPkgOut.dwErrorCode;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down package error,Pkgname=%s,Speed=%.2f,system errorcode=%d,%s")
				,strPackageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (downPkgOut.dwCmdOut != CMD_DOWN_PKG_OUT || downPkgOut.dwSizeSend != dwLen + sizeof(CMD_OUT))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_Get_Data_From_Server_Error;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down package error,Pkgname=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strPackageName,m_MasterPort->GetRealSpeed(),dwErrorCode);
			break;
		}

		// CM_OUT + 文件名称 + '\0' + 文件内容 + 结束标志
		int len = strlen((char *)pByte) + 1;
		char *file = new char[len];
		strcpy_s(file,len,(char *)pByte);

		CString strDestFile(file);
		delete []file;

		if (strDestFile.Left(1) != _T("\\"))
		{
			strDestFile = _T("\\") + strDestFile;
		}

		BYTE flag = pByte[dwLen - 1];

		DWORD dwFileLen = dwLen - len - 1;
		BYTE *pFileData = new BYTE[dwFileLen];
		ZeroMemory(pFileData,dwFileLen);
		memcpy(pFileData,pByte + len,dwFileLen);

		delete []pByte;
		pByte = NULL;

		PDATA_INFO dataInfo = new DATA_INFO;
		ZeroMemory(dataInfo,sizeof(DATA_INFO));
		dataInfo->szFileName = new TCHAR[strDestFile.GetLength()+1];
		_tcscpy_s(dataInfo->szFileName,strDestFile.GetLength()+1,strDestFile);
		dataInfo->ullOffset = ullCompleteSize;
		dataInfo->dwDataSize = dwFileLen;
		dataInfo->pData = pFileData;
		AddDataQueueList(dataInfo);

		delete []dataInfo->szFileName;
		delete []dataInfo;

		if (m_bComputeHash)
		{
			m_pMasterHashMethod->update((void *)pFileData,dwFileLen);
		}

		dwErrorCode = 0;

		delete []pFileData;

		ullReadSize += dwFileLen;
		ullCompleteSize += dwFileLen;

		if (flag == END_FLAG)
		{
			CString strSourceFile = _T("M:") + strDestFile;
			m_MapCopyFiles.SetAt(strSourceFile,ullCompleteSize);
			ullCompleteSize = 0;
		}

		QueryPerformanceCounter(&t2);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);

		// 因为是压缩数据，长度比实际长度短，所以要根据速度计算
		m_MasterPort->SetCompleteSize(m_MasterPort->GetValidSize() * ullReadSize / m_ullCapacity);

	}

	WSACloseEvent(ol.hEvent);

	if (m_hMaster != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hMaster);
		m_hMaster = INVALID_HANDLE_VALUE;
	}

	if (!bResult)
	{
		// 发送停止命令
		downPkgIn.byStop = TRUE;
		memcpy(sendBuf,&downPkgIn,sizeof(CMD_IN));
		memcpy(sendBuf + sizeof(CMD_IN),pkgName,strlen(pkgName));

		DWORD dwError = 0;

		Send(m_ClientSocket,(char *)sendBuf,dwSendLen,NULL,&dwError);
	}

	delete []sendBuf;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
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

	m_MasterPort->SetResult(bResult);
	m_MasterPort->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (!m_bCompare || m_CompareMethod == CompareMethod_Hash)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

		if (m_bVerify)
		{
			m_MasterPort->SetPortState(PortState_Pass);
		}

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
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Package %s - %s,HashValue=%s")
				,m_MasterPort->GetFileName(),strHashMethod,m_strMasterHash);

		}
	}
	else
	{
		m_MasterPort->SetPortState(PortState_Fail);
		m_MasterPort->SetErrorCode(errType,dwErrorCode);
	}

	return bResult;
}


BOOL CDisk::WriteFiles(CPort *port,CDataQueue *pDataQueue)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	CString strOldFileName,strCurrentFileName;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->Active();

	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);
		port->SetPortState(PortState_Fail);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	CString strVolume = volumeArray.GetAt(0);

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

		strCurrentFileName = strVolume + CString(dataInfo->szFileName);

		CString strPath = CUtils::GetFilePathWithoutName(strCurrentFileName);

		if (!PathFileExists(strPath))
		{
			SHCreateDirectory(NULL,strPath);
		}

		if (strCurrentFileName != strOldFileName)
		{
			if (hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hFile);
				hFile = INVALID_HANDLE_VALUE;
			}

			hFile = GetHandleOnFile(strCurrentFileName,CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				bResult = FALSE;

				delete []dataInfo->pData;
				delete []dataInfo->szFileName;
				delete dataInfo;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFile error,file name=%s,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),strCurrentFileName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}

			strOldFileName = strCurrentFileName;
		}

		QueryPerformanceCounter(&t1);
		if (!WriteFileAsyn(hFile,dataInfo->ullOffset,dataInfo->dwDataSize,dataInfo->pData,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []dataInfo->pData;
			delete []dataInfo->szFileName;
			delete dataInfo;

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
			port->AppendCompleteSize(dataInfo->dwDataSize);

			delete []dataInfo->pData;
			delete []dataInfo->szFileName;
			delete dataInfo;
		}

	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
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

	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

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

BOOL CDisk::VerifyFiles(CPort *port,CHashMethod *pHashMethod)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwLen = m_nBlockSectors * BYTES_PER_SECTOR;
	ULONGLONG ullCompleteSize = 0;

	port->Active();

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);
		port->SetPortState(PortState_Fail);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	CString strVolume = volumeArray.GetAt(0);

	POSITION pos = m_MapCopyFiles.GetStartPosition();
	ULONGLONG ullFileSize = 0;
	CString strSourceFile,strDestFile;
	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active && !port->IsKickOff())
	{
		m_MapCopyFiles.GetNextAssoc(pos,strSourceFile,ullFileSize);

		// 目标文件为去掉盘符剩余的文件
		strDestFile = strSourceFile.Right(strSourceFile.GetLength() - 2);

		strDestFile = strVolume + strDestFile;

		HANDLE hFile = GetHandleOnFile(strDestFile,OPEN_EXISTING,FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING,&dwErrorCode);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - OpenFile error,file name=%s,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),strDestFile,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		ullCompleteSize = 0;
		dwLen = m_nBlockSectors * BYTES_PER_SECTOR;

		// 改成do{}while;为了读取文件长度为0的文件
		do
		{
			QueryPerformanceCounter(&t0);

			if (ullFileSize - ullCompleteSize < dwLen)
			{
				dwLen = (DWORD)(ullFileSize - ullCompleteSize);

				// 判断是否是一个整的扇区,以FILE_FLAG_NO_BUFFERING标志打开文件必须扇区对齐
				if (dwLen % BYTES_PER_SECTOR != 0)
				{
					dwLen = (dwLen / BYTES_PER_SECTOR + 1) * BYTES_PER_SECTOR;
				}
			}

			PBYTE pByte = new BYTE[dwLen];
			ZeroMemory(pByte,dwLen);

			QueryPerformanceCounter(&t1);
			if (!ReadFileAsyn(hFile,ullCompleteSize,dwLen,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
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

				ullCompleteSize += dwLen;

				QueryPerformanceCounter(&t2);

				dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
				dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒

				port->AppendUsedWaitTimeS(dbTimeWait);
				port->AppendUsedNoWaitTimeS(dbTimeNoWait);
				port->AppendCompleteSize(dwLen);
			}

		}while (bResult && !*m_lpCancel && !port->IsKickOff()
			&& ullCompleteSize < ullFileSize && port->GetPortState() == PortState_Active);

		CloseHandle(hFile);
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
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
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - %s,HashValue=%s")
			,port->GetPortName(),port->GetDiskNum(),strHashMethod,strVerifyHash);

		port->SetEndTime(CTime::GetCurrentTime());
		if (0 == m_strMasterHash.CompareNoCase(strVerifyHash))
		{
			bResult = TRUE;
			port->SetResult(TRUE);
			port->SetPortState(PortState_Pass);
		}
		else
		{
			bResult = FALSE;
			port->SetResult(FALSE);
			port->SetPortState(PortState_Fail);
			port->SetErrorCode(ErrorType_Custom,CustomError_Compare_Failed);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Verify FAIL,Target=%s,Master=%s")
				,port->GetPortName(),port->GetDiskNum(),strVerifyHash,m_strMasterHash);
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

BOOL CDisk::VerifyFiles( CPort *port,CDataQueue *pDataQueue )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	CString strOldFileName,strCurrentFileName;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->Active();

	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);
		port->SetPortState(PortState_Fail);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	CString strVolume = volumeArray.GetAt(0);

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

		strCurrentFileName = strVolume + CString(dataInfo->szFileName);

		CString strPath = CUtils::GetFilePathWithoutName(strCurrentFileName);

		if (strCurrentFileName != strOldFileName)
		{
			if (hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hFile);
				hFile = INVALID_HANDLE_VALUE;
			}

			hFile = GetHandleOnFile(strCurrentFileName,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				bResult = FALSE;

				delete []dataInfo->pData;
				delete []dataInfo->szFileName;
				delete dataInfo;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFile error,file name=%s,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),strCurrentFileName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}

			strOldFileName = strCurrentFileName;
		}

		PBYTE pByte = new BYTE[dataInfo->dwDataSize];
		ZeroMemory(pByte,dataInfo->dwDataSize);

		QueryPerformanceCounter(&t1);
		if (!ReadFileAsyn(hFile,dataInfo->ullOffset,dataInfo->dwDataSize,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []pByte;
			delete []dataInfo->pData;
			delete []dataInfo->szFileName;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}
		else
		{
			// 比较
			for (DWORD i = 0; i < dataInfo->dwDataSize;i++)
			{
				if (pByte[i] != dataInfo->pData[i])
				{
					//比较出错
					bResult = FALSE;
					errType = ErrorType_Custom;
					dwErrorCode = CustomError_Compare_Failed;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=%I64d")
						,port->GetPortName(),port->GetDiskNum(),pByte[i],dataInfo->pData[i],dataInfo->ullOffset + i);

					break;
				}
			}

			QueryPerformanceCounter(&t2);

			dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

			port->AppendUsedWaitTimeS(dbTimeWait);
			port->AppendUsedNoWaitTimeS(dbTimeNoWait);
			port->AppendCompleteSize(dataInfo->dwDataSize);

			delete []pByte;
			delete []dataInfo->pData;
			delete []dataInfo->szFileName;
			delete dataInfo;

			if (!bResult)
			{
				break;
			}
		}

	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
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

	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

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

BOOL CDisk::Start()
{
	// 初始化
	m_ullValidSize = 0;
	m_EffList.RemoveAll();
	ZeroMemory(m_ImageHash,LEN_DIGEST);
	m_CompressQueue.Clear();
	m_MapCopyFiles.RemoveAll();


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

	case WorkMode_FileCopy:
		bRet = OnCopyFiles();
		break;

	case WorkMode_DiskFormat:
		bRet = OnFormatDisk();
		break;

	case WorkMode_DifferenceCopy:
		bRet = OnDiffCopy();
		break;

	case WorkMode_Full_RW_Test:
		bRet = OnFullRWTest();
		break;

	case WorkMode_Fade_Picker:
		bRet = OnFadePickerTest();
		break;

	case WorkMode_Speed_Check:
		bRet = OnSpeedCheck();
		break;

	}

	return bRet;
}

BOOL CDisk::ReadImage()
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

BOOL CDisk::ReadLocalImage()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	ULONGLONG ullReadSize = SIZEOF_IMAGE_HEADER;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

	while (bResult && !*m_lpCancel && ullReadSize < m_ullCapacity && m_MasterPort->GetPortState() == PortState_Active)
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
		if (!ReadFileAsyn(m_hMaster,ullOffset,dwLen,pkgHead,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = *(PDWORD)&pkgHead[8];

		PBYTE pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		if (!ReadFileAsyn(m_hMaster,ullOffset,dwLen,pByte,m_MasterPort->GetOverlapped(TRUE),&dwErrorCode))
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

		PDATA_INFO dataInfo = new DATA_INFO;
		ZeroMemory(dataInfo,sizeof(DATA_INFO));
		dataInfo->ullOffset = ullOffset;
		dataInfo->dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo->pData = new BYTE[dataInfo->dwDataSize];
		memcpy(dataInfo->pData,&pByte[PKG_HEADER_SIZE],dataInfo->dwDataSize);
		m_CompressQueue.AddTail(dataInfo);
		delete []pByte;

		ullOffset += dwLen;
		ullReadSize += dwLen;

		QueryPerformanceCounter(&t2);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);

		// 因为是压缩数据，长度比实际长度短，所以要根据速度计算
		m_MasterPort->SetCompleteSize(m_MasterPort->GetValidSize() * ullReadSize / m_ullCapacity);

	}

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
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
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image,%s - %s,Image hash value was changed,Compute=%s,Record=%s")
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

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image,%s - %s,HashValue=%s")
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

BOOL CDisk::ReadRemoteImage()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;

	ULONGLONG ullReadSize = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	WSAOVERLAPPED ol = {0};
	ol.hEvent = WSACreateEvent();

	QueryPerformanceFrequency(&freq);

	m_MasterPort->Active();

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

	while (bResult && !*m_lpCancel && ullReadSize < m_ullCapacity && m_MasterPort->GetPortState() == PortState_Active)
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

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send copy image command error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		CMD_OUT copyImageOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&copyImageOut,dwLen,&ol,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy image error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
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

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy image error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
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

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy image error,filename=%s,Speed=%.2f,system errorcode=%d,%s")
				,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (copyImageOut.dwCmdOut != CMD_COPY_IMAGE_OUT || copyImageOut.dwSizeSend != dwLen + sizeof(CMD_OUT))
		{
			bResult = FALSE;
			errType = ErrorType_Custom;
			dwErrorCode = CustomError_Get_Data_From_Server_Error;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv copy image error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strImageName,m_MasterPort->GetRealSpeed(),dwErrorCode);
			break;
		}


		// 去除尾部标志
		dwLen -= 1;

		PDATA_INFO dataInfo = new DATA_INFO;
		ZeroMemory(dataInfo,sizeof(DATA_INFO));
		dataInfo->dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo->pData = new BYTE[dataInfo->dwDataSize];
		memcpy(dataInfo->pData,&pByte[PKG_HEADER_SIZE],dataInfo->dwDataSize);

		m_CompressQueue.AddTail(dataInfo);

		// 写文件
		if (m_hMaster != INVALID_HANDLE_VALUE)
		{
			WriteFileAsyn(m_hMaster,ullReadSize,dwLen,pByte,m_MasterPort->GetOverlapped(FALSE),&dwErrorCode);
		}


		dwErrorCode = 0;

		delete []pByte;

		ullReadSize += dwLen;

		QueryPerformanceCounter(&t2);

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart; // 秒
		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // 秒
		m_MasterPort->AppendUsedWaitTimeS(dbTimeWait);
		m_MasterPort->AppendUsedNoWaitTimeS(dbTimeNoWait);

		// 因为是压缩数据，长度比实际长度短，所以要根据速度计算
		m_MasterPort->SetCompleteSize(m_MasterPort->GetValidSize() * ullReadSize / m_ullCapacity);

	}

	WSACloseEvent(ol.hEvent);

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

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
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
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image,%s - %s,Image hash value was changed,Compute=%s,Record=%s")
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

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Image,%s - %s,HashValue=%s")
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

BOOL CDisk::Compress()
{
	m_bCompressComplete = FALSE;
	BOOL bResult = TRUE;

	// 计算精确速度
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

		DWORD dwSourceLen = sizeof(ULONGLONG) + sizeof(DWORD) + dataInfo->dwDataSize;
		BYTE *pBuffer = new BYTE[dwSourceLen];
		ZeroMemory(pBuffer,dwSourceLen);
		memcpy(pBuffer,&dataInfo->ullOffset,sizeof(ULONGLONG));
		memcpy(pBuffer + sizeof(ULONGLONG),&dataInfo->dwDataSize,sizeof(DWORD));
		memcpy(pBuffer + sizeof(ULONGLONG) + sizeof(DWORD),dataInfo->pData,dataInfo->dwDataSize);

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

BOOL CDisk::Uncompress()
{
	m_bCompressComplete = FALSE;

	BOOL bResult = TRUE;
	// 计算精确速度
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
			uncompressData->ullOffset = *(PULONGLONG)pDest;
			uncompressData->dwDataSize = *(PDWORD)(pDest + sizeof(ULONGLONG));

			uncompressData->pData = new BYTE[uncompressData->dwDataSize];
			ZeroMemory(uncompressData->pData,uncompressData->dwDataSize);
			memcpy(uncompressData->pData,pDest + sizeof(ULONGLONG) + sizeof(DWORD),uncompressData->dwDataSize);

			AddDataQueueList(uncompressData);

			EFF_DATA effData;
			effData.ullStartSector = uncompressData->ullOffset/BYTES_PER_SECTOR;
			effData.ullSectors = uncompressData->dwDataSize/BYTES_PER_SECTOR;
			effData.wBytesPerSector = BYTES_PER_SECTOR;
			m_EffList.AddTail(effData);

			if (m_bComputeHash)
			{
				m_pMasterHashMethod->update((void *)uncompressData->pData,uncompressData->dwDataSize);
			}

			delete []uncompressData->pData;
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

BOOL CDisk::WriteImage( CPort *port,CDataQueue *pDataQueue )
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

BOOL CDisk::WriteLocalImage(CPort *port,CDataQueue *pDataQueue)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	port->Active();

	HANDLE hFile = GetHandleOnFile(port->GetFileName(),CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_System,dwErrorCode);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create image file error,filename=%s,system errorcode=%ld,%s")
			,port->GetFileName(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

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
		*(PULONGLONG)dataInfo->pData = ullPkgIndex;
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = dataInfo->dwDataSize;

		if (!WriteFileAsyn(hFile,ullOffset,dwLen,dataInfo->pData,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
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

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
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

		if (m_bQuickImage)
		{
			imgHead.dwImageType = QUICK_IMAGE;
		}
		else
		{
			imgHead.dwImageType = FULL_IMAGE;
		}

		imgHead.dwMaxSizeOfPackage = MAX_COMPRESS_BUF;
		imgHead.ullCapacitySize = m_MasterPort->GetTotalSize();
		imgHead.dwBytesPerSector = m_MasterPort->GetBytesPerSector();
		memcpy(imgHead.szZipVer,ZIP_VERSION,strlen(ZIP_VERSION));
		imgHead.ullPkgCount = ullPkgIndex;
		imgHead.ullValidSize = m_MasterPort->GetValidSize();
		imgHead.dwHashLen = m_pMasterHashMethod->getHashLength();
		imgHead.dwHashType = m_MasterPort->GetHashMethod();
		memcpy(imgHead.byImageDigest,m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());
		imgHead.byEnd = END_FLAG;

		dwLen = SIZEOF_IMAGE_HEADER;

		if (!WriteFileAsyn(hFile,0,dwLen,(LPBYTE)&imgHead,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Write image head error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
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

BOOL CDisk::WriteRemoteImage(CPort *port,CDataQueue *pDataQueue)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

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
		*(PULONGLONG)dataInfo->pData = ullPkgIndex;
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = sizeof(CMD_IN) + strlen(fileName) + 2 + dataInfo->dwDataSize;

		makeImageIn.dwSizeSend = dwLen;

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);
		memcpy(pByte,&makeImageIn,sizeof(CMD_IN));
		memcpy(pByte+sizeof(CMD_IN),fileName,strlen(fileName));
		memcpy(pByte+sizeof(CMD_IN)+strlen(fileName)+1,dataInfo->pData,dataInfo->dwDataSize);

		if (!Send(m_ClientSocket,(char *)pByte,dwLen,&olSend,&dwErrorCode))
		{
			delete []pByte;
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		delete []pByte;

		// 读返回值
		CMD_OUT makeImageOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwLen,&olRecv,&dwErrorCode))
		{
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwErrorCode != 0)
		{
			dwErrorCode = makeImageOut.dwErrorCode;
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwLen)
		{
			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			errType = ErrorType_Custom;
			bResult = FALSE;

			delete []dataInfo->pData;
			delete dataInfo;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
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

		delete []dataInfo->pData;
		delete dataInfo;

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

		if (m_bQuickImage)
		{
			imgHead.dwImageType = QUICK_IMAGE;
		}
		else
		{
			imgHead.dwImageType = FULL_IMAGE;
		}

		imgHead.dwMaxSizeOfPackage = MAX_COMPRESS_BUF;
		imgHead.ullCapacitySize = m_MasterPort->GetTotalSize();
		imgHead.dwBytesPerSector = m_MasterPort->GetBytesPerSector();
		memcpy(imgHead.szZipVer,ZIP_VERSION,strlen(ZIP_VERSION));
		imgHead.ullPkgCount = ullPkgIndex;
		imgHead.ullValidSize = m_MasterPort->GetValidSize();
		imgHead.dwHashLen = m_pMasterHashMethod->getHashLength();
		imgHead.dwHashType = m_MasterPort->GetHashMethod();
		memcpy(imgHead.byImageDigest,m_pMasterHashMethod->digest(),m_pMasterHashMethod->getHashLength());
		imgHead.byEnd = END_FLAG;

		dwLen = sizeof(CMD_IN) + strlen(fileName) + 2 + SIZEOF_IMAGE_HEADER;

		makeImageIn.dwSizeSend = dwLen;

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);
		pByte[dwLen - 1] = END_FLAG;

		memcpy(pByte,&makeImageIn,sizeof(CMD_IN));
		memcpy(pByte+sizeof(CMD_IN),fileName,strlen(fileName));
		memcpy(pByte+sizeof(CMD_IN)+strlen(fileName)+1,&imgHead,SIZEOF_IMAGE_HEADER);

		if (!Send(m_ClientSocket,(char *)pByte,dwLen,&olSend,&dwErrorCode))
		{
			delete []pByte;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			goto END;
		}

		delete []pByte;

		// 读返回值
		CMD_OUT makeImageOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&makeImageOut,dwLen,&olRecv,&dwErrorCode))
		{
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			goto END;
		}

		if (makeImageOut.dwErrorCode != 0)
		{
			dwErrorCode = makeImageOut.dwErrorCode;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			goto END;
		}

		if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwLen)
		{
			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			errType = ErrorType_Custom;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
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

BOOL CDisk::CleanDisk( CPort *port )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;

	port->Active();

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetValidSize(port->GetTotalSize());


	switch(m_CleanMode)
	{
	case CleanMode_Quick:
		{
			QueryPerformanceCounter(&t0);
			BOOL bResult = QuickClean(hDisk,port,&dwErrorCode);
			QueryPerformanceCounter(&t1);

			dbTimeNoWait = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			port->SetUsedNoWaitTimeS(dbTimeNoWait);
			port->SetUsedWaitTimeS(dbTimeNoWait);
			port->SetCompleteSize(port->GetTotalSize());
		}
		break;

	case CleanMode_Full:
		{
			ULONGLONG ullStartSectors = 0;
			DWORD dwSectors = m_nBlockSectors;
			DWORD dwLen = m_nBlockSectors * dwBytePerSector;

			BYTE *pFillByte = new BYTE[dwLen];
			BYTE *pReadByte = new BYTE[dwLen];

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

			if (m_pFillBytes != NULL)
			{
				delete []m_pFillBytes;
			}

			m_pFillBytes = new BYTE[dwLen];
			memcpy(m_pFillBytes,pFillByte,dwLen);

			while (!*m_lpCancel && ullStartSectors < ullSectorNums && !port->IsKickOff())
			{
				QueryPerformanceCounter(&t0);

				if (ullStartSectors + dwSectors > ullSectorNums)
				{
					dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
					dwLen = dwSectors * dwBytePerSector;
				}

				//dwTimeNoWait = timeGetTime();
				QueryPerformanceCounter(&t1);
				if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
				{
					bResult = FALSE;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
					break;
				}
				else
				{

					if (m_bCompareClean && m_CompareCleanSeq == CompareCleanSeq_In)
					{
						Sleep(1);
						if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pReadByte,port->GetOverlapped(TRUE),&dwErrorCode))
						{
							bResult = FALSE;

							CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
								,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
							break;
						}


						for (DWORD i = 0; i < dwLen;i++)
						{
							if (pReadByte[i] != pFillByte[i])
							{
								//比较出错
								bResult = FALSE;
								errType = ErrorType_Custom;
								dwErrorCode = CustomError_Compare_Failed;

								CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=sector %I64d, offset %d")
									,port->GetPortName(),port->GetDiskNum(),pReadByte[i],pFillByte[i],ullStartSectors + i / dwBytePerSector,i % dwBytePerSector);

								break;
							}
						}
					}

					//DWORD dwTime = timeGetTime();
					QueryPerformanceCounter(&t2);

					dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

					dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

					ullStartSectors += dwSectors;

					port->AppendCompleteSize(dwLen);
					port->AppendUsedWaitTimeS(dbTimeWait);
					port->AppendUsedNoWaitTimeS(dbTimeNoWait);

					if (!bResult)
					{
						break;
					}

				}

			}

			delete []pFillByte;
			delete []pReadByte;

			if (*m_lpCancel)
			{
				bResult = FALSE;
				dwErrorCode = CustomError_Cancel;
				errType = ErrorType_Custom;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
			}

			if (port->IsKickOff())
			{
				bResult = FALSE;
				dwErrorCode = CustomError_Speed_Too_Slow;
				errType = ErrorType_Custom;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
			}
		}
		break;

	case CleanMode_Safe:
		{
			for (int i = 0; i < 7;i++)
			{
				ULONGLONG ullStartSectors = 0;
				DWORD dwSectors = m_nBlockSectors;
				DWORD dwLen = m_nBlockSectors * dwBytePerSector;

				BYTE *pFillByte = new BYTE[dwLen];

				port->SetCompleteSize(0);
				port->SetUsedNoWaitTimeS(0);
				port->SetUsedWaitTimeS(0);

				while (!*m_lpCancel && ullStartSectors < ullSectorNums && !port->IsKickOff())
				{
					QueryPerformanceCounter(&t0);

					if (ullStartSectors + dwSectors > ullSectorNums)
					{
						dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
						dwLen = dwSectors * dwBytePerSector;
					}

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
					if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
					{
						bResult = FALSE;

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

				}

				delete []pFillByte;

				if (*m_lpCancel)
				{
					bResult = FALSE;
					dwErrorCode = CustomError_Cancel;
					errType = ErrorType_Custom;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
					break;
				}

				if (port->IsKickOff())
				{
					bResult = FALSE;
					dwErrorCode = CustomError_Speed_Too_Slow;
					errType = ErrorType_Custom;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);

					break;
				}
			}			
		}
		break;
	}

	CloseHandle(hDisk);
	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (m_bEnd)
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

BOOL CDisk::FormatDisk(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	port->SetValidSize(port->GetTotalSize());

	QuickClean(port,&dwErrorCode);

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

	CFat32 fat32;
	USES_CONVERSION;
	char *pBuffer = W2A(m_strVolumnLabel);

	if (!fat32.Init(hDisk,m_dwClusterSize,pBuffer))
	{
		bResult = FALSE;
		dwErrorCode = fat32.GetErrorCode();

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Format init error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		goto FORMAT_END;
	}

	if (!fat32.InitialDisk())
	{
		bResult = FALSE;

		dwErrorCode = fat32.GetErrorCode();

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Format init disk error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		goto FORMAT_END;
	}

	if (!fat32.PartitionDisk())
	{
		bResult = FALSE;

		dwErrorCode = fat32.GetErrorCode();

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Format partition disk error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		goto FORMAT_END;
	}

	if (!fat32.FormatPartition())
	{
		bResult = FALSE;

		dwErrorCode = fat32.GetErrorCode();

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Format partition error,system errorcode=%ld,%s")
			,port->GetPortName(),port->GetDiskNum(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
		goto FORMAT_END;
	}

FORMAT_END:
	SetDiskAtrribute(hDisk,FALSE,FALSE,&dwErrorCode);
	CloseHandle(hDisk);

	if (bResult)
	{
		//重新上电
		m_pCommand->Power(port->GetPortNum(),FALSE);
		Sleep(500);
		m_pCommand->Power(port->GetPortNum(),TRUE);
	}

	if (!m_bEnd)
	{
		port->SetCompleteSize(port->GetTotalSize());
		port->SetResult(bResult);
		port->SetEndTime(CTime::GetCurrentTime());

		if (bResult)
		{
			port->SetPortState(PortState_Pass);
		}
		else
		{
			port->SetPortState(PortState_Fail);
			port->SetErrorCode(ErrorType_Custom,CustomError_Format_Error);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,custom errorcode=0x%X,disk format error.")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Format_Error);
		}
	}

	return bResult;
}

BOOL CDisk::FullRWTest(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;

	port->Active();

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2,t3;
	double dbTimeNoWait = 0.0,dbTimeRead = 0.0,dbTimeWrite = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetValidSize(port->GetTotalSize());

	ULONGLONG ullStartSectors = 0;
	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * dwBytePerSector;

	BYTE *pFillByte = new BYTE[dwLen];
	BYTE *pReadByte = new BYTE[dwLen];
	BYTE *pOriginByte = new BYTE[dwLen];

	srand((unsigned int)time(NULL));

	for (DWORD i = 0;i < dwLen;i++)
	{
		pFillByte[i] = (BYTE)rand();
	}

	while (!*m_lpCancel && ullStartSectors < ullSectorNums && !port->IsKickOff())
	{
		if (ullStartSectors + dwSectors > ullSectorNums)
		{
			dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
			dwLen = dwSectors * dwBytePerSector;
		}

		// 只读测试
		if (m_bReadOnlyTest)
		{
			QueryPerformanceCounter(&t0);
			if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(TRUE),&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			QueryPerformanceCounter(&t1);

			dbTimeNoWait = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
			dbTimeRead = (double)(t1.QuadPart - t1.QuadPart) / (double)freq.QuadPart;
		}
		else
		{
			if (m_bRetainOrginData)
			{
				if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(TRUE),&dwErrorCode))
				{
					bResult = FALSE;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
					break;
				}
				Sleep(1);
			}

			QueryPerformanceCounter(&t0);
			if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,WriteSpeed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			QueryPerformanceCounter(&t1);

			Sleep(1);
			if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pReadByte,port->GetOverlapped(TRUE),&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}

			QueryPerformanceCounter(&t2);

			if (m_bRetainOrginData)
			{
				Sleep(1);
				if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(FALSE),&dwErrorCode))
				{
					bResult = FALSE;

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,WriteSpeed=%.2f,system errorcode=%ld,%s")
						,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
					break;
				}
			}

			// 划分成扇区一个扇区一个扇区比较，这样好统计坏扇
			for (DWORD i = 0; i < dwLen;i++)
			{
				if (pReadByte[i] != pFillByte[i])
				{
					//比较出错
					bResult = FALSE;
					errType = ErrorType_Custom;
					dwErrorCode = CustomError_Compare_Failed;

					DWORD sector = i / dwBytePerSector;
					DWORD offset = i % dwBytePerSector;

					port->AddBadBlock(ullStartSectors + sector);

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=sector %I64d, offset %d")
						,port->GetPortName(),port->GetDiskNum(),pReadByte[i],pFillByte[i],ullStartSectors + sector,offset);

					if (m_bStopBadBlock)
					{
						break;
					}
					else
					{
						//跳过这个扇区继续下一个扇区
						i += dwBytePerSector - offset;

						continue;
					}

				}
			}

			QueryPerformanceCounter(&t3);

			dbTimeNoWait = (double)(t3.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
			dbTimeRead = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;
			dbTimeWrite = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
		}

		ullStartSectors += dwSectors;

		port->AppendCompleteSize(dwLen);
		port->AppendUsedWaitTimeS(dbTimeNoWait);
		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedTimeS(dbTimeRead,TRUE);
		port->AppendUsedTimeS(dbTimeWrite,FALSE);

		if (!bResult && m_bStopBadBlock)
		{
			break;
		}

	}

	delete []pFillByte;
	delete []pReadByte;
	delete []pOriginByte;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	CloseHandle(hDisk);

	if (m_bFormatFinished && bResult)
	{
		bResult = FormatDisk(port);

		if (!bResult)
		{
			errType = port->GetErrorCode(&dwErrorCode);
		}
	}

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

BOOL CDisk::FadePicker(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;
	DWORD dwSkipSectors = (DWORD)(ullSectorNums / 100); // 步进单位根据容量大小决定，固定取100次

	port->Active();

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2,t3;
	double dbTimeNoWait = 0.0,dbTimeRead = 0.0,dbTimeWrite = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetValidSize(port->GetTotalSize());

	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * dwBytePerSector;
	ULONGLONG ullStartSectors = 0;

	BYTE *pFillByte = new BYTE[dwLen];
	BYTE *pReadByte = new BYTE[dwLen];
	BYTE *pOriginByte = new BYTE[dwLen];

	srand((unsigned int)time(NULL));

	for (DWORD i = 0;i < dwLen;i++)
	{
		pFillByte[i] = (BYTE)rand();
	}

	while (!*m_lpCancel && ullStartSectors < ullSectorNums && !port->IsKickOff())
	{	
		if (ullStartSectors + dwSectors > ullSectorNums)
		{
			dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
			dwLen = dwSectors * dwBytePerSector;
		}

		if (m_bRetainOrginData)
		{
			if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(TRUE),&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
			Sleep(1);
		}

		QueryPerformanceCounter(&t0);
		if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,WriteSpeed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}
		QueryPerformanceCounter(&t1);

		Sleep(1);
		if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pReadByte,port->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		QueryPerformanceCounter(&t2);

		if (m_bRetainOrginData)
		{
			Sleep(1);
			if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(FALSE),&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,WriteSpeed=%.2f,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}
		}

		// 划分成扇区一个扇区一个扇区比较，这样好统计坏扇
		for (DWORD i = 0; i < dwLen;i++)
		{
			if (pReadByte[i] != pFillByte[i])
			{
				//比较出错
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_Compare_Failed;

				DWORD sector = i / dwBytePerSector;
				DWORD offset = i % dwBytePerSector;

				port->AddBadBlock(ullStartSectors + sector);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=sector %I64d, offset %d")
					,port->GetPortName(),port->GetDiskNum(),pReadByte[i],pFillByte[i],ullStartSectors + sector,offset);

				break;

			}
		}

		QueryPerformanceCounter(&t3);

		dbTimeNoWait = (double)(t3.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
		dbTimeRead = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;
		dbTimeWrite = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;


		if (ullStartSectors + dwSkipSectors > ullSectorNums)
		{
			dwSkipSectors = (DWORD)(ullSectorNums - ullStartSectors);
		}

		ullStartSectors += dwSkipSectors;

		// 为了让速度显示得不上过大，所花时间也成倍增长

		int rate = dwSkipSectors * dwBytePerSector / dwLen;

		port->AppendCompleteSize(dwSkipSectors * dwBytePerSector);
		port->AppendUsedWaitTimeS(dbTimeNoWait * rate);
		port->AppendUsedNoWaitTimeS(dbTimeNoWait * rate);
		port->AppendUsedTimeS(dbTimeRead * rate,TRUE);
		port->AppendUsedTimeS(dbTimeWrite * rate,FALSE);

		if (!bResult)
		{
			break;
		}

	}

	delete []pFillByte;
	delete []pReadByte;
	delete []pOriginByte;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	CloseHandle(hDisk);

	if (m_bFormatFinished && bResult)
	{
		bResult = FormatDisk(port);

		if (!bResult)
		{
			errType = port->GetErrorCode(&dwErrorCode);
		}
	}

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

BOOL CDisk::SpeedCheck(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;

	port->Active();

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeRead = 0.0,dbTimeWrite = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetValidSize(port->GetTotalSize());

	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * dwBytePerSector;
	ULONGLONG ullStartSectors = 0;

	BYTE *pOriginByte = new BYTE[dwLen];

	// 随机取10个地方计算平均值
	int nCount = 0;
	srand((int)time(NULL));
	while (!*m_lpCancel && ullStartSectors < ullSectorNums && nCount < 10)
	{	
		do 
		{
			ullStartSectors = rand();

		} while (ullStartSectors + dwSectors >= ullSectorNums);

		nCount++;

		QueryPerformanceCounter(&t0);
		if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,ReadSpeed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		QueryPerformanceCounter(&t1);
		
		if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pOriginByte,port->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,WriteSpeed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		QueryPerformanceCounter(&t2);

		dbTimeRead = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
		dbTimeWrite = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		port->AppendCompleteSize(dwLen);
		port->AppendUsedTimeS(dbTimeRead,TRUE);
		port->AppendUsedTimeS(dbTimeWrite,FALSE);

	}

	delete []pOriginByte;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	CloseHandle(hDisk);

	if (bResult)
	{
		// 判断速度是否达标
		if (m_bCheckSpeed)
		{
			if (m_dbMinReadSpeed > port->GetRealSpeed(TRUE))
			{
				bResult = FALSE;
				dwErrorCode = CustomError_ReadSpeed_Slow;
				errType = ErrorType_Custom;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,ReadSpeed=%.2f < %.2f,custom errorcode=0x%X,read speed slow.")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(TRUE),m_dbMinReadSpeed,dwErrorCode);
			}

			if (m_dbMinReadSpeed > port->GetRealSpeed(FALSE))
			{
				bResult = FALSE;
				dwErrorCode = CustomError_WriteSpeed_Slow;
				errType = ErrorType_Custom;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,WriteSpeed=%.2f < %.2f,custom errorcode=0x%X,write speed slow.")
					,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(FALSE),m_dbMinWriteSpeed,dwErrorCode);
			}
		}
	}

	// 设置结束

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

BOOL CDisk::CompareClean(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwBytePerSector = port->GetBytesPerSector();
	ULONGLONG ullSectorNums = port->GetTotalSize() / dwBytePerSector;

	port->Active();

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

	SetDiskAtrribute(hDisk,FALSE,TRUE,&dwErrorCode);

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	port->SetValidSize(port->GetTotalSize());

	DWORD dwSectors = m_nBlockSectors;
	DWORD dwLen = m_nBlockSectors * dwBytePerSector;
	ULONGLONG ullStartSectors = 0;

	BYTE *pReadByte = new BYTE[dwLen];

	while (!*m_lpCancel && ullStartSectors < ullSectorNums && !port->IsKickOff())
	{
		QueryPerformanceCounter(&t0);

		if (ullStartSectors + dwSectors > ullSectorNums)
		{
			dwSectors = (DWORD)(ullSectorNums - ullStartSectors);
			dwLen = dwSectors * dwBytePerSector;
		}

		//dwTimeNoWait = timeGetTime();
		QueryPerformanceCounter(&t1);

		if (!ReadSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pReadByte,port->GetOverlapped(TRUE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}


		for (DWORD i = 0; i < dwLen;i++)
		{
			if (pReadByte[i] != m_pFillBytes[i])
			{
				//比较出错
				bResult = FALSE;
				errType = ErrorType_Custom;
				dwErrorCode = CustomError_Compare_Failed;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Compare failed(%02X:%02X),location=sector %I64d, offset %d")
					,port->GetPortName(),port->GetDiskNum(),pReadByte[i],m_pFillBytes[i],ullStartSectors + i / dwBytePerSector,i % dwBytePerSector);

				break;
			}
		}

		//DWORD dwTime = timeGetTime();
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		ullStartSectors += dwSectors;

		port->AppendCompleteSize(dwLen);
		port->AppendUsedWaitTimeS(dbTimeWait);
		port->AppendUsedNoWaitTimeS(dbTimeNoWait);

	}

	delete []pReadByte;

	if (*m_lpCancel)
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Cancel;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port=%s,Disk %d,Speed=%.2f,custom errorcode=0x%X,user cancelled.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),dwErrorCode);
	}

	if (port->IsKickOff())
	{
		bResult = FALSE;
		dwErrorCode = CustomError_Speed_Too_Slow;
		errType = ErrorType_Custom;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Speed=%.2f,custom errorcode=0x%X,speed too slow.")
			,port->GetPortName(),port->GetDiskNum(),port->GetRealSpeed(),CustomError_Speed_Too_Slow);
	}

	CloseHandle(hDisk);
	port->SetResult(bResult);
	port->SetEndTime(CTime::GetCurrentTime());

	if (bResult)
	{
		if (m_bEnd)
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

	BOOL bResult = disk->VerifyDisk(port,pHashMethod);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::VerifySectorThreadProc( LPVOID lpParm )
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CDataQueue *pDataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bResult = disk->VerifyDisk(port,pDataQueue);

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

DWORD WINAPI CDisk::ReadFilesThreadProc(LPVOID lpParm)
{
	CDisk *disk = (CDisk *)lpParm;

	BOOL bResult = disk->ReadFiles();

	return bResult;
}

DWORD WINAPI CDisk::WriteFilesThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CDataQueue *dataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bResult = disk->WriteFiles(port,dataQueue);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::VerifyFilesThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CHashMethod *pHashMethod = (CHashMethod *)parm->lpVoid3;

	BOOL bResult = disk->VerifyFiles(port,pHashMethod);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::VerifyFilesByteThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CDataQueue *dataQueue = (CDataQueue *)parm->lpVoid3;

	BOOL bResult = disk->VerifyFiles(port,dataQueue);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::FormatDiskThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->FormatDisk(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::SearchUserLogThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->SearchUserLog(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::DeleteChangeThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->DeleteChange(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::EnumFileThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CMapStringToULL *pMap = (CMapStringToULL *)parm->lpVoid3;
	CMapStringToString *pArray = (CMapStringToString *)parm->lpVoid4;

	BOOL bResult = disk->EnumFileSize(port,pMap,pArray);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::ComputeHashThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;
	CMapStringToString *pMap = (CMapStringToString *)parm->lpVoid3;

	BOOL bResult = disk->ComputeHashValue(port,pMap);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::FullRWTestThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->FullRWTest(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::FadePickerThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->FadePicker(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::SpeedCheckThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->SpeedCheck(port);

	delete parm;

	return bResult;
}

DWORD WINAPI CDisk::CompareCleanThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->CompareClean(port);

	delete parm;

	return bResult;
}

void CDisk::AddDataQueueList( PDATA_INFO dataInfo )
{
	POSITION pos1 = m_DataQueueList.GetHeadPosition();
	POSITION pos2 = m_TargetPorts->GetHeadPosition();

	while (pos1 && pos2)
	{
		CDataQueue *dataQueue = m_DataQueueList.GetNext(pos1);
		CPort *port = m_TargetPorts->GetNext(pos2);

		if (port->GetResult() && port->GetPortState() == PortState_Active)
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

void CDisk::SetCompareMode( CompareMode compareMode)
{
	m_CompareMode = compareMode;
}

void CDisk::SetFileAndFolder( const CStringArray &fileArray,const CStringArray &folderArray )
{
	m_FileArray.Copy(fileArray);
	m_FodlerArray.Copy(folderArray);
}

int CDisk::EnumFile( CString strSource )
{
	int nCount = 0; // 文件夹的数量

	strSource.TrimRight(_T('\\'));
	CString strFindFile = strSource + _T("\\*");

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	while (bFind)
	{
		bFind = ff.FindNextFile();

		if (ff.GetFileName() == _T(".") || ff.GetFileName() == _T(".."))
		{
			continue;
		}

		if (ff.IsDirectory())
		{
			nCount++;
			nCount += EnumFile(ff.GetFilePath());
		}
		else
		{
			m_ullValidSize += ff.GetLength();
			m_MapCopyFiles.SetAt(ff.GetFilePath(),ff.GetLength());
		}
	}
	ff.Close();

	return nCount;
}

int CDisk::EnumFile( CString strPath,CString strVolume,CMapStringToULL *pMap ,CMapStringToString *pArray)
{
	int nCount = 0; // 文件的数量

	strPath.TrimRight(_T('\\'));

	CString strFindFile = strPath + _T("\\*");

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	while (bFind)
	{
		bFind = ff.FindNextFile();

		if (ff.GetFileName() == _T(".") || ff.GetFileName() == _T(".."))
		{
			continue;
		}

		CString strPathFile = ff.GetFilePath();

		// 去掉盘符
		strPathFile = strPathFile.Mid(strVolume.GetLength());

		if (ff.IsDirectory())
		{
			nCount += EnumFile(ff.GetFilePath(),strVolume,pMap,pArray);

			pArray->SetAt(strPathFile,strPathFile);
		}
		else
		{
			nCount++;

			pMap->SetAt(strPathFile,ff.GetLength());
		}
	}
	ff.Close();

	return nCount;
}

void CDisk::SetFormatParm( CString strVolumeLabel,FileSystem fileSystem,DWORD dwClusterSize,BOOL bQuickFormat )
{
	m_strVolumnLabel = strVolumeLabel;
	m_FileSystem = fileSystem;
	m_dwClusterSize = dwClusterSize;
	m_bQuickFormat = bQuickFormat;

	if (m_dwClusterSize == 0)
	{
		m_dwClusterSize = 8 * 512;
	}
}

void CDisk::SetDiffCopyParm(UINT nSourceType,BOOL bServer,UINT nCompareRule,BOOL bUpload,const CStringArray &logArray,BOOL bIncludeSunDir)
{
	m_nSourceType = nSourceType;
	m_bServerFirst = bServer;
	m_nCompareRule = nCompareRule;
	m_ArrayLogPath.Copy(logArray);
	m_bIncludeSubDir = bIncludeSunDir;
	m_bUploadUserLog = bUpload;
}

void CDisk::SetSocket( SOCKET sClient,BOOL bServerFirst )
{
	m_ClientSocket = sClient;
	m_bServerFirst = bServerFirst;
}

void CDisk::SetMakeImageParm(BOOL bQuickImage, int compressLevel /*= Z_BEST_SPEED*/ )
{
	m_iCompressLevel = compressLevel;
	m_bQuickImage = bQuickImage;
}

void CDisk::SetFullCopyParm(BOOL bAllowCapGap,UINT nPercent)
{
	m_bAllowCapGap = bAllowCapGap;
	m_nCapGapPercent = nPercent;
}

BOOL CDisk::SearchUserLog( CPort *port )
{
	int nCount = m_ArrayLogPath.GetCount();

	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{

		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	if (nCount > 0)
	{
		CString strVolume = volumeArray.GetAt(0);
		CStringArray *pArray = new CStringArray;
		CString strPath,strType,strValue;

		for (int i = 0; i < nCount;i++)
		{
			// Path:Ext
			strValue = m_ArrayLogPath.GetAt(i);
			int nPos = strValue.ReverseFind(_T(':'));
			strPath = strValue.Left(nPos);
			strType = strValue.Right(strValue.GetLength() - nPos - 1);

			strPath.Trim();
			strType.Trim();

			strPath = strVolume + strPath;

			SearchUserLog(strPath,strType,pArray);
		}

		m_MapPortStringArray.SetAt(port,pArray);
	}

	return TRUE;
}

void CDisk::SearchUserLog( CString strPath,CString strType,CStringArray *pArray )
{
	CString strFindFile;

	strFindFile.Format(_T("%s%s"),strPath,strType);

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	while (bFind)
	{
		bFind = ff.FindNextFile();

		if (ff.GetFileName() == _T(".") || ff.GetFileName() == _T(".."))
		{
			continue;
		}

		if (ff.IsDirectory())
		{
			if (m_bIncludeSubDir)
			{
				SearchUserLog(ff.GetFilePath(),strType,pArray);
			}
		}
		else
		{			
			pArray->Add(ff.GetFilePath());
		}
	}

	ff.Close();
}

void CDisk::CleanMapPortStringArray()
{
	CPort *port = NULL;
	CStringArray *pArray = NULL;

	POSITION pos = m_MapPortStringArray.GetStartPosition();

	while (pos)
	{
		m_MapPortStringArray.GetNextAssoc(pos,port,pArray);

		if (pArray != NULL)
		{
			delete pArray;
			pArray = NULL;
		}
	}

	m_MapPortStringArray.RemoveAll();
}

BOOL CDisk::UploadUserLog()
{
	CPort *port = NULL;
	CStringArray *pArray = NULL;
	DWORD dwErrorCode = 0;
	POSITION pos = m_MapPortStringArray.GetStartPosition();
	BOOL bResult = TRUE;
	while (pos)
	{
		m_MapPortStringArray.GetNextAssoc(pos,port,pArray);

		if (pArray != NULL)
		{
			int nCount = pArray->GetCount();
			CString strLogName,strSN,strFileName;
			strSN = port->GetSN();
			if (strSN.IsEmpty())
			{
				strSN.Format(_T("NOSN-%d"),port->GetPortNum());
			}

			for (int i = 0;i < nCount;i++)
			{
				strFileName = pArray->GetAt(i);
				strLogName.Format(_T("%s\\%s"),strSN,CUtils::GetFileName(strFileName));

				HANDLE hFile = GetHandleOnFile(strFileName,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);

				if (hFile == INVALID_HANDLE_VALUE)
				{
					continue;
				}

				LARGE_INTEGER liFileSize;
				GetFileSizeEx(hFile,&liFileSize);

				ULONGLONG ullCompleteSize = 0;
				DWORD dwLen = m_nBlockSectors * BYTES_PER_SECTOR;

				while (ullCompleteSize < (ULONGLONG)liFileSize.QuadPart)
				{
					PBYTE pByte = new BYTE[dwLen];
					ZeroMemory(pByte,dwLen);

					if (!ReadFileAsyn(hFile,ullCompleteSize,dwLen,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
					{
						bResult = FALSE;
						delete []pByte;

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,read user log file %s failed,system errorcode=%ld,%s")
							,port->GetPortName(),port->GetDiskNum(),strFileName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
						break;
					}
					else
					{
						ullCompleteSize += dwLen;
						// 发送到服务器
						USES_CONVERSION;
						char *filename = W2A(strLogName);

						CMD_IN userLogIn = {0};

						DWORD dwSize = sizeof(CMD_IN) + strlen(filename) + 1 + dwLen + 1;

						userLogIn.dwCmdIn = CMD_USER_LOG_IN;
						userLogIn.dwSizeSend = dwSize;

						BYTE *pSendData = new BYTE[dwSize];
						ZeroMemory(pSendData,dwSize);

						memcpy(pSendData,&userLogIn,sizeof(CMD_IN));
						memcpy(pSendData + sizeof(CMD_IN),filename,strlen(filename));
						memcpy(pSendData + sizeof(CMD_IN) + strlen(filename) + 1,pByte,dwLen);

						if (ullCompleteSize >= (ULONGLONG)liFileSize.QuadPart)
						{
							pSendData[dwSize - 1] = END_FLAG;
						}

						delete []pByte;
						pByte = NULL;

						DWORD dwErrorCode = 0;
						if (!Send(m_ClientSocket,(char *)pSendData,dwSize,NULL,&dwErrorCode))
						{
							bResult = FALSE;
							delete []pSendData;

							CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSASend() failed with system error code:%d,%s")
								,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

							break;
						}
						delete []pSendData;

						UPLOAD_LOG_OUT uploadLogOut = {0};
						dwSize = sizeof(UPLOAD_LOG_OUT);
						if (!Recv(m_ClientSocket,(char *)&uploadLogOut,dwSize,NULL,&dwErrorCode))
						{
							bResult = FALSE;
							CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSARecv() failed with system error code:%d,%s")
								,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

							break;
						}

						if (uploadLogOut.dwErrorCode != 0)
						{
							bResult = FALSE;
							dwErrorCode = uploadLogOut.dwErrorCode;
							CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv() failed with system error code:%d,%s")
								,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

							break;
						}
					}
				}

				CloseHandle(hFile);

			}
		}
	}

	return bResult;
}

BOOL CDisk::DownloadChangeList()
{
	DWORD dwErrorCode = 0;
	BOOL bResult = TRUE;
	DWORD dwLen = 0;

	WSAOVERLAPPED ol = {0};
	ol.hEvent = WSACreateEvent();

	CString strChangeList;
	strChangeList.Format(_T("%s.ini"),m_MasterPort->GetFileName());

	HANDLE hFile = GetHandleOnFile(strChangeList,CREATE_ALWAYS,FILE_FLAG_OVERLAPPED,&dwErrorCode);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create change list file failed,filename=%s,system errorcode=%ld,%s")
			,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	USES_CONVERSION;
	char *changelist = W2A(strChangeList);

	DWORD dwSendLen = sizeof(CMD_IN) + strlen(changelist) + 2;

	CMD_IN downChangeList = {0};
	downChangeList.dwCmdIn = CMD_DOWN_LIST_IN;
	downChangeList.dwSizeSend = dwSendLen;

	BYTE *bufSend = new BYTE[dwSendLen];
	ZeroMemory(bufSend,dwSendLen);

	memcpy(bufSend,&downChangeList,sizeof(CMD_IN));
	memcpy(bufSend + sizeof(CMD_IN),changelist,strlen(changelist));
	bufSend[dwSendLen - 1] = END_FLAG;

	ULONGLONG ullOffset = 0;

	while (1)
	{
		if (!Send(m_ClientSocket,(char *)bufSend,dwSendLen,&ol,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send down change list command error,filename=%s,system errorcode=%ld,%s")
				,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;;
		}

		CMD_OUT downListOut = {0};
		dwLen = sizeof(CMD_OUT);
		if (!Recv(m_ClientSocket,(char *)&downListOut,dwLen,&ol,&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down change list error,filename=%s,system errorcode=%ld,%s")
				,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		dwLen = downListOut.dwSizeSend - sizeof(CMD_OUT);

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		DWORD dwRead = 0;

		while(dwRead < dwLen)
		{
			DWORD dwByteRead = dwLen - dwRead;
			if (!Recv(m_ClientSocket,(char *)(pByte+dwRead),dwByteRead,&ol,&dwErrorCode))
			{
				bResult = FALSE;

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down change list error,Pkgname=%s,Speed=%.2f,system errorcode=%ld,%s")
					,strChangeList,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
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

		if (downListOut.dwErrorCode != 0)
		{
			bResult = FALSE;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down change list error,Pkgname=%s,Speed=%.2f,system errorcode=%d,%s")
				,strChangeList,m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
			break;
		}

		if (downListOut.dwCmdOut != CMD_DOWN_LIST_OUT || downListOut.dwSizeSend != dwLen + sizeof(CMD_OUT))
		{
			bResult = FALSE;

			delete []pByte;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv down change list error,Pkgname=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strChangeList,m_MasterPort->GetRealSpeed(),dwErrorCode);
			break;
		}

		BYTE flag = pByte[dwLen - 1];
		DWORD dwFileLen = dwLen - 1;
		BYTE *pFileData = new BYTE[dwFileLen];
		ZeroMemory(pFileData,dwFileLen);

		memcpy(pFileData,pByte,dwFileLen);
		delete []pByte;

		if (!WriteFileAsyn(hFile,ullOffset,dwFileLen,pFileData,m_MasterPort->GetOverlapped(FALSE),&dwErrorCode))
		{
			bResult = FALSE;

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Create change list file failed,filename=%s,system errorcode=%ld,%s")
				,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		ullOffset += dwFileLen;

		if (flag == END_FLAG)
		{
			ullOffset = 0;
			break;
		}
	}
	CloseHandle(hFile);
	WSACloseEvent(ol.hEvent);

	if (!bResult)
	{
		DeleteFile(strChangeList);

		// 发送停止命令
		downChangeList.byStop = TRUE;
		memcpy(bufSend,&downChangeList,sizeof(CMD_IN));
		memcpy(bufSend + sizeof(CMD_IN),changelist,strlen(changelist));

		DWORD dwError = 0;

		Send(m_ClientSocket,(char *)bufSend,dwSendLen,NULL,&dwError);
	}

	delete []bufSend;

	return bResult;

}

BOOL CDisk::QueryChangeList()
{
	DWORD dwErrorCode = 0,dwLen = 0;
	CString strChangeList;
	strChangeList.Format(_T("%s.ini"),m_MasterPort->GetFileName());

	USES_CONVERSION;
	char *changelist = W2A(strChangeList);

	DWORD dwSendLen = sizeof(CMD_IN) + strlen(changelist) + 2;

	CMD_IN queryChangeList = {0};
	queryChangeList.dwCmdIn = CMD_QUERY_LIST_IN;
	queryChangeList.dwSizeSend = dwSendLen;

	BYTE *bufSend = new BYTE[dwSendLen];
	ZeroMemory(bufSend,dwSendLen);

	memcpy(bufSend,&queryChangeList,sizeof(CMD_IN));
	memcpy(bufSend + sizeof(CMD_IN),changelist,strlen(changelist));
	bufSend[dwSendLen - 1] = END_FLAG;

	if (!Send(m_ClientSocket,(char *)bufSend,dwSendLen,NULL,&dwErrorCode))
	{

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send query change list command error,filename=%s,system errorcode=%ld,%s")
			,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		delete []bufSend;

		return FALSE;
	}

	delete []bufSend;

	QUERY_LIST_OUT queryListOut = {0};
	dwLen = sizeof(QUERY_LIST_OUT);
	if (!Recv(m_ClientSocket,(char *)&queryListOut,dwLen,NULL,&dwErrorCode))
	{

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv query change list command error,filename=%s,system errorcode=%ld,%s")
			,strChangeList,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));


		return FALSE;
	}

	if (queryListOut.dwErrorCode != 0)
	{
		// 不存在变更文件

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("The change list file(%s) doesn't exist in server.")
			,strChangeList);

		return FALSE;
	}

	return TRUE;
}

BOOL CDisk::DeleteChange( CPort *port )
{
	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	CString strVolume = volumeArray.GetAt(0);

	port->Active();

	// 删除文件
	int nCount = m_ArrayDelFiles.GetCount();

	for (int i = 0;i < nCount;i++)
	{
		CString strFile = m_ArrayDelFiles.GetAt(i);

		if (strFile.Left(1) != _T("\\"))
		{
			strFile = _T("\\") + strFile;
		}

		strFile = strVolume + strFile;

		DeleteFile(strFile);
	}

	// 删除文件夹
	nCount = m_ArrayDelFolders.GetCount();
	for (int i = 0;i < nCount;i++)
	{
		CString strFolder = m_ArrayDelFolders.GetAt(i);

		if (strFolder.Left(1) != _T("\\"))
		{
			strFolder = _T("\\") + strFolder;
		}

		strFolder = strVolume + strFolder;

		CUtils::DeleteDirectory(strFolder);
	}


	return TRUE;
}

BOOL CDisk::ComputeHashValue( CPort *port,CMapStringToString *pMap )
{
	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetResult(FALSE);
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);
		port->SetPortState(PortState_Fail);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	int nFileCount = m_ArraySameFile.GetCount();

	if (nFileCount == 0)
	{
		return TRUE;
	}

	CHashMethod *pHashMethod = NULL;

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

	default:
		pHashMethod = new CChecksum32();
		break;
	}

	CString strVolume = volumeArray.GetAt(0);

	CString strFilePath;
	ULONGLONG ullCompleteSize = 0;
	DWORD dwErrorCode = 0;
	DWORD dwLen =  m_nBlockSectors * BYTES_PER_SECTOR;

	// 计算精确速度
	LARGE_INTEGER freq,t1,t2;
	double dbTime = 0.0;

	QueryPerformanceFrequency(&freq);

	port->Active();

	for (int i = 0; i < nFileCount; i++)
	{
		strFilePath = strVolume + m_ArraySameFile.GetAt(i);

		HANDLE hFile = GetHandleOnFile(strFilePath,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,&dwErrorCode);

		if (hFile == INVALID_HANDLE_VALUE)
		{

			port->SetResult(FALSE);
			port->SetEndTime(CTime::GetCurrentTime());
			port->SetErrorCode(ErrorType_System,dwErrorCode);
			port->SetPortState(PortState_Fail);

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - open file %s failed,system errorcode=%ld,%s")
				,port->GetPortName(),port->GetDiskNum(),strFilePath,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			delete pHashMethod;
			return FALSE;
		}

		LARGE_INTEGER liFileSize;
		GetFileSizeEx(hFile,&liFileSize);

		ullCompleteSize = 0;
		dwLen =  m_nBlockSectors * BYTES_PER_SECTOR;

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);

		pHashMethod->reset();

		while (ullCompleteSize < (ULONGLONG)liFileSize.QuadPart)
		{
			QueryPerformanceCounter(&t1);
			if (!ReadFileAsyn(hFile,ullCompleteSize,dwLen,pByte,port->GetOverlapped(TRUE),&dwErrorCode))
			{

				port->SetResult(FALSE);
				port->SetEndTime(CTime::GetCurrentTime());
				port->SetErrorCode(ErrorType_System,dwErrorCode);
				port->SetPortState(PortState_Fail);

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - read file %s failed,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),strFilePath,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

				delete []pByte;
				CloseHandle(hFile);
				delete pHashMethod;
				return FALSE;
			}

			ullCompleteSize += dwLen;

			pHashMethod->update((void *)pByte,dwLen);

			QueryPerformanceCounter(&t2);

			dbTime = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

			port->AppendUsedWaitTimeS(dbTime);
			port->AppendUsedNoWaitTimeS(dbTime);
			port->AppendCompleteSize(dwLen);

		}

		delete []pByte;

		CloseHandle(hFile);

		CString strHashValue;
		for (int k = 0; k < pHashMethod->getHashLength();k++)
		{
			CString strValue;
			strValue.Format(_T("%02X"),pHashMethod->digest()[k]);
			strHashValue += strValue;
		}

		pMap->SetAt(m_ArraySameFile.GetAt(i),strHashValue);
	}
	delete pHashMethod;

	return TRUE;
}

BOOL CDisk::EnumFileSize( CPort *port,CMapStringToULL *pMap,CMapStringToString *pArray)
{
	CStringArray volumeArray;
	port->GetVolumeArray(volumeArray);
	if (volumeArray.GetCount() != 1)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetPortState(PortState_Fail);
		port->SetErrorCode(ErrorType_Custom,CustomError_Unrecognized_Partition);

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - partition error,custom errorcode=0x%X,unrecognized partition")
			,port->GetPortName(),port->GetDiskNum(),CustomError_Unrecognized_Partition);

		return FALSE;

	}

	CString strVolume = volumeArray.GetAt(0);

	int nFileCount = EnumFile(strVolume,strVolume,pMap,pArray);

	return TRUE;
}

void CDisk::CompareFileSize()
{
	//比较母盘与目标盘
	CString strSourceFile,strSourceFileWithVolume;
	ULONGLONG ullSourceFileSize = 0,ullDestFileSize = 0;

	POSITION pos = m_MapSizeSourceFiles.GetStartPosition();

	while (pos)
	{
		m_MapSizeSourceFiles.GetNextAssoc(pos,strSourceFile,ullSourceFileSize);

		if (m_MapSizeDestFiles.Lookup(strSourceFile,ullDestFileSize))
		{

			if (ullSourceFileSize == ullDestFileSize)
			{
				m_ArraySameFile.Add(strSourceFile);
				m_ullHashFilesSize += ullSourceFileSize;
			}
			else
			{
				if (strSourceFile.Left(1) == _T("\\"))
				{
					strSourceFileWithVolume = _T("M:") + strSourceFile;
				}
				else
				{
					strSourceFileWithVolume = _T("M:\\") + strSourceFile;
				}

				m_MapCopyFiles.SetAt(strSourceFileWithVolume,ullSourceFileSize);
				m_ullValidSize += ullSourceFileSize;
			}

			m_MapSizeDestFiles.RemoveKey(strSourceFile);
		}
		else
		{
			if (strSourceFile.Left(1) == _T("\\"))
			{
				strSourceFileWithVolume = _T("M:") + strSourceFile;
			}
			else
			{
				strSourceFileWithVolume = _T("M:\\") + strSourceFile;
			}

			m_MapCopyFiles.SetAt(strSourceFileWithVolume,ullSourceFileSize);
			m_ullValidSize += ullSourceFileSize;
		}
	}

	// m_MapSizeDestFiles中剩余的文件是母盘中没有的文件需要删除
	CString strDestFile;
	pos = m_MapSizeDestFiles.GetStartPosition();

	while (pos)
	{
		m_MapSizeDestFiles.GetNextAssoc(pos,strDestFile,ullDestFileSize);

		m_ArrayDelFiles.Add(strDestFile);
	}

	// 判断哪些文件夹需要删除
	pos = m_MapDestFolders.GetStartPosition();
	CString strValue;
	while (pos)
	{
		m_MapDestFolders.GetNextAssoc(pos,strSourceFile,strValue);

		if (!m_MapSourceFolders.Lookup(strSourceFile,strValue))
		{
			m_ArrayDelFolders.Add(strSourceFile);
		}
	}

}

void CDisk::CompareHashValue()
{
	CString strSourceFile,strSourceFileWithVolume,strSourceHashValue,strDestHashValue;
	ULONGLONG ullSourceFileSize;

	POSITION pos = m_MapHashSourceFiles.GetStartPosition();

	while (pos)
	{
		m_MapHashSourceFiles.GetNextAssoc(pos,strSourceFile,strSourceHashValue);

		if (m_MapHashDestFiles.Lookup(strSourceFile,strDestHashValue))
		{

			if (strSourceHashValue.CompareNoCase(strDestHashValue) != 0)
			{
				if (strSourceFile.Left(1) == _T("\\"))
				{
					strSourceFileWithVolume = _T("M:") + strSourceFile;
				}
				else
				{
					strSourceFileWithVolume = _T("M:\\") + strSourceFile;
				}

				ullSourceFileSize = m_MapSizeSourceFiles[strSourceFile];

				m_MapCopyFiles.SetAt(strSourceFileWithVolume,ullSourceFileSize);
				m_ullValidSize += ullSourceFileSize;
			}
		}
		else
		{
			if (strSourceFile.Left(1) == _T("\\"))
			{
				strSourceFileWithVolume = _T("M:") + strSourceFile;
			}
			else
			{
				strSourceFileWithVolume = _T("M:\\") + strSourceFile;
			}

			ullSourceFileSize = m_MapSizeSourceFiles[strSourceFile];

			m_MapCopyFiles.SetAt(strSourceFileWithVolume,ullSourceFileSize);
			m_ullValidSize += ullSourceFileSize;
		}
	}
}

void CDisk::SetCleanDiskFirst( BOOL bCleanDiskFist,BOOL bCompareClean,CompareCleanSeq seq,int times,int *values )
{
	m_bCleanDiskFirst = bCleanDiskFist;
	m_bCompareClean = bCompareClean;
	m_CompareCleanSeq = seq;

	if (m_bCleanDiskFirst)
	{
		m_nCleanTimes = times;

		m_pCleanValues = new int[times];

		for (int i = 0; i < times;i++)
		{
			m_pCleanValues[i] = values[i];
		}
	}
}

void CDisk::SetCompareParm( BOOL bCompare,CompareMethod method )
{
	m_bCompare = bCompare;
	m_CompareMethod = method;
}

UINT CDisk::GetCurrentTargetCount()
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

void CDisk::SetFullRWTestParm(BOOL bReadOnlyTest, BOOL bRetainOriginData,BOOL bFormatFinished,BOOL bStopBadBlock )
{
	m_bReadOnlyTest = bReadOnlyTest;
	m_bRetainOrginData = bRetainOriginData;
	m_bFormatFinished = bFormatFinished;
	m_bStopBadBlock = bStopBadBlock;
}

void CDisk::SetFadePickerParm( BOOL bRetainOriginData,BOOL bFormatFinished )
{
	m_bRetainOrginData = bRetainOriginData;
	m_bFormatFinished = bFormatFinished;
}

void CDisk::SetSpeedCheckParm( BOOL bCheckSpeed,double dbReaddSpeed,double dbWriteSpeed )
{
	m_bCheckSpeed = bCheckSpeed;
	m_dbMinReadSpeed = dbReaddSpeed;
	m_dbMinWriteSpeed = dbWriteSpeed;
}

