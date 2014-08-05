
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
	int nDiskNum;
	PTSTR pszVolumePath;
	ULONG PartitionNumber;
}VOLUMEDEVICEINFO,*PVOLUMEDEVICEINFO;

/******************************************************************************
 * 函数申明
 ******************************************************************************/
VOID EnumStorage();
VOID EnumVolume();

PSTORAGEDEVIEINFO 
	MatchStorageDeivceID(PTSTR pszDeviceID);
PDEVICELIST
	MatchStorageDeivceIDs(PTSTR pszDeviceID);

PDEVICELIST MatchVolumeDeviceDiskNums(int nDiskNum);

VOID CleanStorageDevieInfo( PSTORAGEDEVIEINFO pStoDevInfo );
VOID CleanVolumeDeviceInfo(PVOLUMEDEVICEINFO pVolumeDevInfo);
VOID CleanupStorageDeviceList(PDEVICELIST pDevList);
VOID CleanupVolumeDeviceList(PDEVICELIST pDevList);
VOID CleanupStorage();
VOID CleanupVolume();

BOOL GetDriveProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc);

#endif /* STORAGEENUM_H_ */
