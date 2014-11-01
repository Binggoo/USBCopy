
/*****************************************************************************
                Copyright (c) Phiyo Technology 
Module Name:  
	StorageEnum.h
Created on:
	2011-9-22
Author:
	Administrator
*****************************************************************************/

#ifndef STORAGEENUM_H_
#define STORAGEENUM_H_
#include <WinIoCtl.h>
#include "DeviceList.h"

/******************************************************************************
 * StorageEnum 数据结构体
 ******************************************************************************/
typedef struct
{
    PTSTR   pszParentDeviceID;
	int     nDiskNum;
}STORAGEDEVIEINFO,*PSTORAGEDEVIEINFO;

typedef struct
{
	PTSTR   pszParentDeviceID;
	PTSTR   pszDeviceID;
	PTSTR   pszDevicePath;
}WPDDEVIEINFO,*PWPDDEVIEINFO;

typedef struct
{
	int nDiskNum;
	PTSTR pszVolumePath;
	ULONG PartitionNumber;
}VOLUMEDEVICEINFO,*PVOLUMEDEVICEINFO;

/******************************************************************************
 * 函数申明
 ******************************************************************************/
VOID EnumStorage(LPBOOL flag);
VOID EnumVolume(LPBOOL flag);
VOID EnumWPD(LPBOOL flag);

PSTORAGEDEVIEINFO 
	MatchStorageDeivceID(PTSTR pszDeviceID);
PDEVICELIST
	MatchStorageDeivceIDs(PTSTR pszDeviceID);

PDEVICELIST MatchVolumeDeviceDiskNums(int nDiskNum);

PWPDDEVIEINFO MatchWPDDeivceID(PTSTR pszDeviceID);

VOID CleanStorageDevieInfo( PSTORAGEDEVIEINFO pStoDevInfo );
VOID CleanVolumeDeviceInfo(PVOLUMEDEVICEINFO pVolumeDevInfo);
VOID CleanWPDDeviceInfo(PWPDDEVIEINFO pWPDDevInfo);

VOID CleanupStorageDeviceList(PDEVICELIST pDevList);
VOID CleanupVolumeDeviceList(PDEVICELIST pDevList);
VOID CleanupWPDDeviceList(PDEVICELIST pDevList);

VOID CleanupStorage();
VOID CleanupVolume();
VOID CleanupWPD();

BOOL GetDriveProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc);

#endif /* STORAGEENUM_H_ */
