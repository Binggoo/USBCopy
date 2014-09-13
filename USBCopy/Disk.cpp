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

#include <ntddscsi.h>
#include "IoctlDef.h"

#ifdef _DEBUG
#pragma comment(lib,"zlib128d.lib")
#else
#pragma comment(lib,"zlib128.lib")
#endif



#define HIDE_SECTORS  0x800   // 1M
#define COMPRESS_THREAD_COUNT 3

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

	m_bQuickFormat = TRUE;
	m_dwClusterSize = 0;

	m_bCompressComplete = TRUE;

	m_bServerFirst = FALSE;

	m_ClientSocket = INVALID_SOCKET;

	m_iCompressLevel = Z_BEST_SPEED;
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

	if (m_pMasterHashMethod != NULL)
	{
		delete m_pMasterHashMethod;
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

	return 0;
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
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

		if(dwErrorCode == ERROR_IO_PENDING) // 结束异步I/O
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

void CDisk::Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile ,CPortCommand *pCommand)
{
	m_hWnd = hWnd;
	m_lpCancel = lpCancel;
	m_hLogFile = hLogFile;
	m_pCommand = pCommand;
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

		break;

	case PortType_SERVER:
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

	m_MasterPort->SetStartTime(CTime::GetCurrentTime());

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
		PostMessage(m_hWnd,WM_VERIFY_START,0,0);

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

BOOL CDisk::OnCopyImage()
{
	IMAGE_HEADER imgHead = {0};
	DWORD dwErrorCode = 0;
	DWORD dwLen = SIZEOF_IMAGE_HEADER;

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
	SetHashMethod(m_bComputeHash,m_bHashVerify,hashMethod);

	m_MasterPort->SetHashMethod(hashMethod);

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwUncompressId;

	HANDLE hReadThread = CreateThread(NULL,0,ReadImageThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Copy Image(%s) - CreateReadImageThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetFileName(),dwReadId,hReadThread);

	// 创建多个解压缩线程
	HANDLE hUncompressThread = CreateThread(NULL,0,UnCompressThreadProc,this,0,&dwUncompressId);
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Uncompress Image - CreateUncompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwUncompressId,hUncompressThread);

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

	// 等待写磁盘线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
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

	if (bResult && m_bHashVerify)
	{
		PostMessage(m_hWnd,WM_VERIFY_START,0,0);

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

	HANDLE hCompressThread = CreateThread(NULL,0,CompressThreadProc,this,0,&dwCompressId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Compress Image - CreateCompressThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,dwCompressId,hCompressThread);


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

	//等待写映像线程结束
	WaitForMultipleObjects(nCount,hWriteThreads,TRUE,INFINITE);
	for (UINT i = 0; i < nCount;i++)
	{
		GetExitCodeThread(hWriteThreads[i],&dwErrorCode);
		bResult |= dwErrorCode;
		CloseHandle(hWriteThreads[i]);
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

	//母盘读线程
	LPVOID_PARM lpMasterVoid = new VOID_PARM;
	lpMasterVoid->lpVoid1 = this;
	lpMasterVoid->lpVoid2 = m_MasterPort;
	lpMasterVoid->lpVoid3 = m_pMasterHashMethod;

	HANDLE hMasterVerifyThread = CreateThread(NULL,0,VerifyThreadProc,lpMasterVoid,0,&dwVerifyId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateVerifyDiskThread,ThreadId=0x%04X,HANDLE=0x%04X")
		,m_MasterPort->GetPortName(),m_MasterPort->GetDiskNum(),dwVerifyId,hMasterVerifyThread);

	//子盘读线程
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

	//等待线程结束
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

BOOL CDisk::OnCopyFiles()
{
	m_ullValidSize = 0;
	m_MapFiles.RemoveAll();

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

			m_MapFiles.SetAt(strFilePath,liFileSize.QuadPart);

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
		,m_MapFiles.GetCount(),m_ullValidSize);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("Copy Files List:"));
	CString strList;
	POSITION pos = m_MapFiles.GetStartPosition();
	CString strFile;
	ULONGLONG ullFileSize = 0;
	while (pos)
	{
		m_MapFiles.GetNextAssoc(pos,strFile,ullFileSize);
		strList.Format(_T("FilePath:%s  FileSize:%I64d"),strFile,ullFileSize);
		CUtils::WriteLogFile(m_hLogFile,FALSE,strList);
	}

	BOOL bResult = FALSE;
	DWORD dwReadId,dwWriteId,dwVerifyId,dwErrorCode;

	HANDLE hReadThread = CreateThread(NULL,0,ReadFilesThreadProc,this,0,&dwReadId);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("File Copy(Master) - CreateReadFilesThread,ThreadId=0x%04X,HANDLE=0x%04X")
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
	}
	delete []hWriteThreads;

	//等待读磁盘线程结束
	WaitForSingleObject(hReadThread,INFINITE);
	GetExitCodeThread(hReadThread,&dwErrorCode);
	bResult &= dwErrorCode;
	CloseHandle(hReadThread);

	if (bResult && m_bHashVerify)
	{
		PostMessage(m_hWnd,WM_VERIFY_START,0,0);

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

BOOL CDisk::OnFormatDisk()
{
	BOOL bResult = FALSE;
	DWORD dwWriteId,dwErrorCode;

	//子盘写线程
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

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	POSITION pos = m_EffList.GetHeadPosition();

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
			// 判断队列是否达到限制值
			while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE) && !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
			{
				Sleep(10);
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

	port->SetStartTime(CTime::GetCurrentTime());

	// 判断容量
	if (port->GetTotalSize() < m_ullValidSize)
	{
		port->SetEndTime(CTime::GetCurrentTime());
		port->SetResult(FALSE);
		port->SetErrorCode(ErrorType_Custom,CustomError_Target_Small);
		port->SetPortState(PortState_Fail);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop copy,custom errorcode=0x%X,target is small")
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
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - Stop copy,custom errorcode=0x%X,target is small")
					,port->GetPortName(),port->GetDiskNum(),CustomError_Target_Small);

				return FALSE;
			}
		}
	}

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

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	DWORD dwBytesPerSector = port->GetBytesPerSector();

	QueryPerformanceFrequency(&freq);

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult() && bResult)
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& (m_MasterPort->GetPortState() == PortState_Active || !m_bCompressComplete)
			&& port->GetResult())
		{
			Sleep(10);
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

		DATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}

		ULONGLONG ullStartSectors = dataInfo.ullOffset / dwBytesPerSector;
		DWORD dwSectors = dataInfo.dwDataSize / dwBytesPerSector;

		QueryPerformanceCounter(&t1);
		if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytesPerSector,dataInfo.pData,port->GetOverlapped(FALSE),&dwErrorCode))
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

	if (!port->GetResult())
	{
		errType = port->GetErrorCode(&dwErrorCode);
		bResult = FALSE;
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
	BOOL bResult = TRUE;
	HANDLE hDisk = GetHandleOnPhysicalDrive(port->GetDiskNum(),FILE_FLAG_OVERLAPPED,pdwErrorCode);

	BYTE *pByte = new BYTE[CLEAN_LENGTH];
	ZeroMemory(pByte,CLEAN_LENGTH);

	if (SetDiskAtrribute(hDisk,FALSE,TRUE,pdwErrorCode))
	{
		DWORD dwBytesPerSector = 0;
		ULONGLONG ullSectorNums = GetNumberOfSectors(hDisk,&dwBytesPerSector);
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

BOOL CDisk::Verify( CPort *port,CHashMethod *pHashMethod )
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullReadSectors = 0;
	ULONGLONG ullRemainSectors = 0;
	ULONGLONG ullStartSectors = 0;
	DWORD dwSectors = BUF_SECTORS;
	DWORD dwLen = BUF_SECTORS * BYTES_PER_SECTOR;

	if (m_WorkMode == WorkMode_DiskCompare)
	{
		port->SetStartTime(CTime::GetCurrentTime());
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

	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active)
	{
		EFF_DATA effData = m_EffList.GetNext(pos);

		ullReadSectors = 0;
		ullRemainSectors = 0;
		ullStartSectors = effData.ullStartSector;
		dwSectors = BUF_SECTORS;
		dwLen = BUF_SECTORS * effData.wBytesPerSector;

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

	if (bResult)
	{
		port->SetHash(pHashMethod->digest(),pHashMethod->getHashLength());

		// 如果是子盘，需要等待母盘结束
		if (port->GetPortType() == PortType_TARGET_DISK)
		{
			while (m_MasterPort->GetPortState() == PortState_Active)
			{
				Sleep(10);
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

BOOL CDisk::ReadFiles()
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwLen = BUF_SECTORS * BYTES_PER_SECTOR;
	ULONGLONG ullCompleteSize = 0;

	// 计算精确速度
	LARGE_INTEGER freq,t0,t1,t2;
	double dbTimeNoWait = 0.0,dbTimeWait = 0.0;

	QueryPerformanceFrequency(&freq);

	POSITION pos = m_MapFiles.GetStartPosition();
	ULONGLONG ullFileSize = 0;
	CString strSourceFile,strDestFile;
	while (pos && !*m_lpCancel && bResult && m_MasterPort->GetPortState() == PortState_Active)
	{
		m_MapFiles.GetNextAssoc(pos,strSourceFile,ullFileSize);

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
		dwLen = BUF_SECTORS * BYTES_PER_SECTOR;
		while (bResult && !*m_lpCancel && ullCompleteSize < ullFileSize && m_MasterPort->GetPortState() == PortState_Active)
		{
			QueryPerformanceCounter(&t0);
			// 判断队列是否达到限制值
			while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE) && !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
			{
				Sleep(10);
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
				DATA_INFO dataInfo = {0};
				dataInfo.szFileName = new TCHAR[strDestFile.GetLength()+1];
				_tcscpy_s(dataInfo.szFileName,strDestFile.GetLength()+1,strDestFile);
				dataInfo.ullOffset = ullCompleteSize;
				dataInfo.dwDataSize = dwLen;
				dataInfo.pData = pByte;
				AddDataQueueList(dataInfo);

				delete []dataInfo.szFileName;

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

		}

		CloseHandle(hFile);
	}

	if (m_MapFiles.GetCount() == 0)
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

	port->SetStartTime(CTime::GetCurrentTime());

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

	while (!*m_lpCancel && m_MasterPort->GetResult() && port->GetResult() && bResult)
	{
		QueryPerformanceCounter(&t0);
		while(pDataQueue->GetCount() <= 0 && !*m_lpCancel && m_MasterPort->GetResult() 
			&& m_MasterPort->GetPortState() == PortState_Active && port->GetResult())
		{
			Sleep(10);
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

		strCurrentFileName = strVolume + CString(dataInfo.szFileName);

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

				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d - CreateFile error,file name=%s,system errorcode=%ld,%s")
					,port->GetPortName(),port->GetDiskNum(),strCurrentFileName,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				break;
			}

			strOldFileName = strCurrentFileName;
		}

		QueryPerformanceCounter(&t1);
		if (!WriteFileAsyn(hFile,dataInfo.ullOffset,dataInfo.dwDataSize,dataInfo.pData,port->GetOverlapped(FALSE),&dwErrorCode))
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
			delete []dataInfo.szFileName;
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

BOOL CDisk::VerifyFiles(CPort *port,CHashMethod *pHashMethod)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	DWORD dwLen = BUF_SECTORS * BYTES_PER_SECTOR;
	ULONGLONG ullCompleteSize = 0;

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

	POSITION pos = m_MapFiles.GetStartPosition();
	ULONGLONG ullFileSize = 0;
	CString strSourceFile,strDestFile;
	while (pos && !*m_lpCancel && bResult && port->GetPortState() == PortState_Active)
	{
		m_MapFiles.GetNextAssoc(pos,strSourceFile,ullFileSize);

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
		dwLen = BUF_SECTORS * BYTES_PER_SECTOR;

		while (bResult && !*m_lpCancel && ullCompleteSize < ullFileSize && port->GetPortState() == PortState_Active)
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

		}

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

BOOL CDisk::Start()
{
	// 初始化
	m_ullValidSize = 0;
	m_EffList.RemoveAll();
	ZeroMemory(m_ImageHash,LEN_DIGEST);
	m_CompressQueue.Clear();
	m_MapFiles.RemoveAll();

	
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

	while (bResult && !*m_lpCancel && ullReadSize < m_ullCapacity && m_MasterPort->GetPortState() == PortState_Active)
	{
		QueryPerformanceCounter(&t0);
		// 判断队列是否达到限制值
		while (IsReachLimitQty(MAX_LENGTH_OF_DATA_QUEUE)
			&& !*m_lpCancel && !IsAllFailed(errType,&dwErrorCode))
		{
			Sleep(10);
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
			Sleep(10);
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

			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Send copy image command error,system errorcode=%ld,%s")
				,m_MasterPort->GetFileName(),m_MasterPort->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
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

		DATA_INFO dataInfo = {0};
		dataInfo.dwDataSize = dwLen - PKG_HEADER_SIZE - 1;
		dataInfo.pData = new BYTE[dataInfo.dwDataSize];
		memcpy(dataInfo.pData,&pByte[PKG_HEADER_SIZE],dataInfo.dwDataSize);

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

	if (m_hMaster != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hMaster);
		m_hMaster = INVALID_HANDLE_VALUE;
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
			Sleep(10);
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

		int ret = compress2(pDest,&dwDestLen,pBuffer,dwSourceLen,m_iCompressLevel);

		if (ret == Z_OK)
		{
			DATA_INFO compressData = {0};
			compressData.dwDataSize = dwDestLen + sizeof(ULONGLONG) + sizeof(DWORD) + 1;
			compressData.dwOldSize = dataInfo.dwDataSize;
			compressData.pData = new BYTE[compressData.dwDataSize];
			ZeroMemory(compressData.pData,compressData.dwDataSize);

			compressData.pData[compressData.dwDataSize - 1] = 0xED; //结束标志
			memcpy(compressData.pData + sizeof(ULONGLONG),&compressData.dwDataSize,sizeof(DWORD));
			memcpy(compressData.pData + sizeof(ULONGLONG) + sizeof(DWORD),pDest,dwDestLen);

			AddDataQueueList(compressData);

			delete []compressData.pData;

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

	port->SetStartTime(CTime::GetCurrentTime());

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
			Sleep(10);
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

		DATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}
		*(PULONGLONG)dataInfo.pData = ullPkgIndex;
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = dataInfo.dwDataSize;

		if (!WriteFileAsyn(hFile,ullOffset,dwLen,dataInfo.pData,port->GetOverlapped(FALSE),&dwErrorCode))
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

		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedWaitTimeS(dbTimeNoWait);

		// 压缩的数据比实际数据短，不能取压缩后的长度，要不压缩之前的长度
		port->AppendCompleteSize(dataInfo.dwOldSize);

		delete []dataInfo.pData;

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
		imgHead.byEnd = END_FLAG;

		dwLen = SIZEOF_IMAGE_HEADER;

		if (!WriteFileAsyn(hFile,0,dwLen,(LPBYTE)&imgHead,port->GetOverlapped(FALSE),&dwErrorCode))
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

BOOL CDisk::WriteRemoteImage(CPort *port,CDataQueue *pDataQueue)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	ErrorType errType = ErrorType_System;
	ULONGLONG ullPkgIndex = 0;
	ULONGLONG ullOffset = SIZEOF_IMAGE_HEADER;
	DWORD dwLen = 0;

	port->SetStartTime(CTime::GetCurrentTime());

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
			Sleep(10);
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

		DATA_INFO dataInfo = pDataQueue->GetHeadRemove();

		if (dataInfo.pData == NULL)
		{
			continue;
		}
		*(PULONGLONG)dataInfo.pData = ullPkgIndex;
		ullPkgIndex++;

		QueryPerformanceCounter(&t1);

		dwLen = sizeof(CMD_IN) + strlen(fileName) + 2 + dataInfo.dwDataSize;
		
		makeImageIn.dwSizeSend = dwLen;

		BYTE *pByte = new BYTE[dwLen];
		ZeroMemory(pByte,dwLen);
		memcpy(pByte,&makeImageIn,sizeof(CMD_IN));
		memcpy(pByte+sizeof(CMD_IN),fileName,strlen(fileName));
		memcpy(pByte+sizeof(CMD_IN)+strlen(fileName)+1,dataInfo.pData,dataInfo.dwDataSize);

		if (!Send(m_ClientSocket,(char *)pByte,dwLen,&olSend,&dwErrorCode))
		{
			delete []pByte;
			bResult = FALSE;
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
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwErrorCode != 0)
		{
			dwErrorCode = makeImageOut.dwErrorCode;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,system errorcode=%ld,%s")
				,strImageName,port->GetRealSpeed(),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			break;
		}

		if (makeImageOut.dwCmdOut != CMD_MAKE_IMAGE_OUT || makeImageOut.dwSizeSend != dwLen)
		{
			dwErrorCode = CustomError_Get_Data_From_Server_Error;
			errType = ErrorType_Custom;
			bResult = FALSE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv write image file error,filename=%s,Speed=%.2f,custom errorcode=0x%X,get data from server error")
				,strImageName,port->GetRealSpeed(),dwErrorCode);

			break;
		}

		//DWORD dwTime = timeGetTime();
		QueryPerformanceCounter(&t2);

		dbTimeWait = (double)(t2.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

		dbTimeNoWait = (double)(t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

		ullOffset += dataInfo.dwDataSize;

		port->AppendUsedNoWaitTimeS(dbTimeNoWait);
		port->AppendUsedWaitTimeS(dbTimeNoWait);

		// 压缩的数据比实际数据短，不能取压缩后的长度，要不压缩之前的长度
		port->AppendCompleteSize(dataInfo.dwOldSize);

		delete []dataInfo.pData;

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
			port->SetValidSize(port->GetTotalSize());
			DWORD dwErrorCode = 0;

			// 计算精确速度
			LARGE_INTEGER freq,t0,t1;
			double dbTimeNoWait = 0.0;
			QueryPerformanceFrequency(&freq);
			QueryPerformanceCounter(&t0);
			BOOL bResult = QuickClean(port,&dwErrorCode);
			QueryPerformanceCounter(&t1);

			dbTimeNoWait = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

			port->SetUsedNoWaitTimeS(dbTimeNoWait);
			port->SetUsedWaitTimeS(dbTimeNoWait);
			port->SetCompleteSize(port->GetTotalSize());
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

			port->SetStartTime(CTime::GetCurrentTime());

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
				if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
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

			port->SetStartTime(CTime::GetCurrentTime());

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
					if (!WriteSectors(hDisk,ullStartSectors,dwSectors,dwBytePerSector,pFillByte,port->GetOverlapped(FALSE),&dwErrorCode))
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

BOOL CDisk::FormatDisk(CPort *port)
{
	BOOL bResult = TRUE;
	DWORD dwErrorCode = 0;
	port->SetStartTime(CTime::GetCurrentTime());
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
	



	/*
	CString strFile,strComand,strBuffer;
	strFile.Format(_T("format_%d.script"),port->GetPortNum());
	strComand.Format(_T("diskpart.exe /s %s"),strFile);
	CFile file(strFile,CFile::modeCreate | CFile::modeWrite);

	if (m_dwClusterSize == 0)
	{
		strBuffer.Format(_T("select disk=%d\r\n")
						 _T("clean\r\n")
						 _T("create partition primary\r\n")
						 _T("select partition=1\r\n")
						 _T("format fs=%s label=\"%s\" quick override\r\n")
						 _T("exit\r\n"),port->GetDiskNum(),m_strFileSystem,m_strVolumnLabel);
	}
	else
	{
		strBuffer.Format(_T("select disk=%d\r\n")
						 _T("clean\r\n")
						 _T("create partition primary\r\n")
						 _T("select partition=1\r\n")
						 _T("format fs=%s label=\"%s\" unit=%d quick override\r\n")
						 _T("exit\r\n"),port->GetDiskNum(),m_strFileSystem,m_strVolumnLabel,m_dwClusterSize);
	}

	USES_CONVERSION;
	char *buffer = W2A(strBuffer);

	file.Write(buffer,strlen(buffer));
	file.Flush();
	file.Close();

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	
	if (CreateProcess(NULL,strComand.GetBuffer(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
	{
		WaitForSingleObject(pi.hProcess,INFINITE);

		GetExitCodeProcess(pi.hProcess,&dwErrorCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (dwErrorCode != 0)
		{
			bResult = FALSE;
		}
		else
		{
			bResult = TRUE;
		}
		
	}
	else
	{
		bResult = FALSE;
	}

	DeleteFile(strFile);
	*/
	
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
	
	port->SetCompleteSize(port->GetTotalSize());
	port->SetEndTime(CTime::GetCurrentTime());
	port->SetResult(bResult);

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

DWORD WINAPI CDisk::FormatDiskThreadProc(LPVOID lpParm)
{
	LPVOID_PARM parm = (LPVOID_PARM)lpParm;
	CDisk *disk = (CDisk *)parm->lpVoid1;
	CPort *port = (CPort *)parm->lpVoid2;

	BOOL bResult = disk->FormatDisk(port);

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
			DATA_INFO data = {0};

			if (dataInfo.szFileName)
			{
				data.szFileName = new TCHAR[_tcslen(dataInfo.szFileName) + 1];
				_tcscpy_s(data.szFileName,_tcslen(dataInfo.szFileName) + 1,dataInfo.szFileName);
			}
			
			data.dwDataSize = dataInfo.dwDataSize;
			data.dwOldSize = dataInfo.dwOldSize;
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

void CDisk::SetFileAndFolder( const CStringArray &fileArray,const CStringArray &folderArray )
{
	m_FileArray.Copy(fileArray);
	m_FodlerArray.Copy(folderArray);
}

int CDisk::EnumFile( CString strSource )
{
	int nCount = 0; // 文件夹的数量
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
			DWORD dwErrorCode = 0;
			HANDLE hFile = GetHandleOnFile(ff.GetFilePath(),OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,&dwErrorCode);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				LARGE_INTEGER liFileSize = {0};

				GetFileSizeEx(hFile,&liFileSize);

				m_ullValidSize += liFileSize.QuadPart;

				m_MapFiles.SetAt(ff.GetFilePath(),liFileSize.QuadPart);

				CloseHandle(hFile);
			}

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

void CDisk::SetSocket( SOCKET sClient,BOOL bServerFirst )
{
	m_ClientSocket = sClient;
	m_bServerFirst = bServerFirst;
}

void CDisk::SetMakeImageParm( int compressLevel /*= Z_BEST_SPEED*/ )
{
	m_iCompressLevel = compressLevel;
}
