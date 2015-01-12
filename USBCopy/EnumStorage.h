
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
	PTSTR   pszDeviceID;
	PTSTR   pszLocationPath;
}USBDEVIEINFO,*PUSBDEVIEINFO;

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
VOID EnumUSB(LPBOOL flag);

PSTORAGEDEVIEINFO 
	MatchStorageDeivceID(PTSTR pszDeviceID);
PDEVICELIST
	MatchStorageDeivceIDs(PTSTR pszDeviceID);

PDEVICELIST MatchVolumeDeviceDiskNums(int nDiskNum);

PWPDDEVIEINFO MatchWPDDeviceID(PTSTR pszDeviceID);

PUSBDEVIEINFO MatchUSBDeviceID(PTSTR pszDeviceID);

VOID CleanStorageDevieInfo( PSTORAGEDEVIEINFO pStoDevInfo );
VOID CleanVolumeDeviceInfo(PVOLUMEDEVICEINFO pVolumeDevInfo);
VOID CleanWPDDeviceInfo(PWPDDEVIEINFO pWPDDevInfo);
VOID CleanUSBDeviceInfo(PUSBDEVIEINFO pUSBDevInfo);

VOID CleanupStorageDeviceList(PDEVICELIST pDevList);
VOID CleanupVolumeDeviceList(PDEVICELIST pDevList);
VOID CleanupWPDDeviceList(PDEVICELIST pDevList);
VOID CleanupUSBDeviceList(PDEVICELIST pDevList);

VOID CleanupStorage();
VOID CleanupVolume();
VOID CleanupWPD();
VOID CleanupUSB();

BOOL GetDriveProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc);

#endif /* STORAGEENUM_H_ */
