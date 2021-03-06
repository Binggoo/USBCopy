
/*****************************************************************************
Copyright (c) Phiyo Technology 
Module Name:  
StorageEnum.cpp
Created on:
2011-9-22
Author:
Administrator
*****************************************************************************/


/******************************************************************************
* include files
******************************************************************************/
#include "stdafx.h"

#include <initguid.h>
#include <devioctl.h>
#include <usbioctl.h>
#include <dbt.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <Portabledevice.h>
#include "EnumStorage.h"

PDEVICELIST _pStorageListHead = NULL;
PDEVICELIST _pVolumeListHead = NULL;
PDEVICELIST _pWPDListHead = NULL;
PDEVICELIST _pUSBListHead = NULL;


/******************************************************************************
* GetStorageParentID
*  获取父系设备的ID
*  DWORD DevInst           DEVINST handle
******************************************************************************/
BOOL
	GetStorageParentID(DWORD DevInst,
	PTSTR pszDeviceID)
{
	DWORD   hDevParent;
	ULONG   ulSize;

	//Get Parant Device handle
	if(CR_SUCCESS != CM_Get_Parent(&hDevParent,DevInst,0))
	{
		return FALSE;
	}

	//Get Device ID String length
	ulSize = 0;
	if(CR_SUCCESS != CM_Get_Device_ID_Size(&ulSize,hDevParent,0))
	{
		return FALSE;
	}

	//Get parent Device DeviceID
	if(CR_SUCCESS != CM_Get_Device_ID(hDevParent,pszDeviceID,ulSize + 1,0))
	{
		//DeviceID 获取失败，释放内存
		return FALSE;
	}
	OutputDebugString(pszDeviceID);
	return TRUE;

}

/******************************************************************************
* GetDeviceID
*  获取设备的ID
*  DWORD DevInst           DEVINST handle
******************************************************************************/
BOOL
	GetDeviceID(DWORD DevInst,
	PTSTR pszDeviceID)
{
	ULONG   ulSize = 0;

	//Get Device ID String length
	if(CR_SUCCESS != CM_Get_Device_ID_Size(&ulSize,DevInst,0))
	{
		return FALSE;
	}

	//Get parent Device DeviceID
	if(CR_SUCCESS != CM_Get_Device_ID(DevInst,pszDeviceID,ulSize + 1,0))
	{
		//DeviceID 获取失败，释放内存
		return FALSE;
	}
	OutputDebugString(pszDeviceID);
	return TRUE;

}

/******************************************************************************
* GetStorageDeviceNumber
******************************************************************************/
BOOL
	GetStorageDeviceNumber(HANDLE hStorageDevice,
	PSTORAGE_DEVICE_NUMBER pStorageDevNumber)
{
	DWORD                   dwOutBytes;
	BOOL                    bResult;

	VOLUME_DISK_EXTENTS		DiskExtents;

	if(INVALID_HANDLE_VALUE == hStorageDevice)
	{
		return FALSE;
	}

	////使用IOCTL_STORAGE_GET_DEVICE_NUMBER
	bResult = DeviceIoControl(hStorageDevice,
		IOCTL_STORAGE_GET_DEVICE_NUMBER ,
		NULL,
		0,
		pStorageDevNumber,
		sizeof(STORAGE_DEVICE_NUMBER),
		&dwOutBytes,
		(LPOVERLAPPED) NULL);

	if(FALSE == bResult)
	{
		//IOCTL_STORAGE_GET_DEVICE_NUMBER 失败
		bResult = DeviceIoControl(	hStorageDevice,
			IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS ,
			NULL,
			0,
			&DiskExtents,
			sizeof(VOLUME_DISK_EXTENTS),
			&dwOutBytes,
			(LPOVERLAPPED) NULL);

		if(FALSE == bResult)
		{
			DWORD dwErrorCode = GetLastError();
			TCHAR szErrorMsg[1024] = {NULL};
			_stprintf_s(szErrorMsg,_T("GetStorageDeviceNumber - DeviceIoControl(%d)"),dwErrorCode);
			OutputDebugString(szErrorMsg);

			return FALSE;
		}

		//复制DiskExtents信息 到 pStorageDevNumber
		if(0 != DiskExtents.NumberOfDiskExtents)
		{
			pStorageDevNumber->DeviceNumber = DiskExtents.Extents[0].DiskNumber;
		}
	}

	return TRUE;

}

/******************************************************************************
* Cleanup_StorageInfo
******************************************************************************/
void Cleanup_StorageInfo(PDEVICELIST pDevList)
{
	PSTORAGEDEVIEINFO pStorageDevInfo;

	if(pDevList->pDevInfo)
	{
		pStorageDevInfo = (PSTORAGEDEVIEINFO)(pDevList->pDevInfo);

		//FREE 所有的子项
		if(pStorageDevInfo->pszParentDeviceID)
		{
			FREE(pStorageDevInfo->pszParentDeviceID);
			pStorageDevInfo->pszParentDeviceID = NULL;
		}

		if (pStorageDevInfo)
		{
			FREE(pStorageDevInfo);
			pStorageDevInfo = NULL;
		}
	}


	FREE(pDevList);
	pDevList = NULL;
}

/******************************************************************************
* Cleanup_VolumeInfo
******************************************************************************/
void Cleanup_VolumeInfo(PDEVICELIST pDevList)
{
	PVOLUMEDEVICEINFO pVolumeDevInfo;

	if(pDevList->pDevInfo)
	{
		pVolumeDevInfo = (PVOLUMEDEVICEINFO)(pDevList->pDevInfo);

		//FREE 所有的子项
		if(pVolumeDevInfo->pszVolumePath)
		{
			FREE(pVolumeDevInfo->pszVolumePath);
			pVolumeDevInfo->pszVolumePath = NULL;
		}

		if (pVolumeDevInfo)
		{
			FREE(pVolumeDevInfo);
			pVolumeDevInfo = NULL;
		}
	}


	FREE(pDevList);
	pDevList = NULL;
}

/******************************************************************************
* Cleanup_WPDInfo
******************************************************************************/
void Cleanup_WPDInfo(PDEVICELIST pDevList)
{
	PWPDDEVIEINFO pWPDDevInfo;

	if(pDevList->pDevInfo)
	{
		pWPDDevInfo = (PWPDDEVIEINFO)(pDevList->pDevInfo);

		//FREE 所有的子项
		if(pWPDDevInfo->pszDevicePath)
		{
			FREE(pWPDDevInfo->pszDevicePath);
			pWPDDevInfo->pszDevicePath = NULL;
		}

		if (pWPDDevInfo->pszParentDeviceID)
		{
			FREE(pWPDDevInfo->pszParentDeviceID);
			pWPDDevInfo->pszParentDeviceID = NULL;
		}

		if (pWPDDevInfo->pszDeviceID)
		{
			FREE(pWPDDevInfo->pszDeviceID);
			pWPDDevInfo->pszDeviceID = NULL;
		}

		if (pWPDDevInfo)
		{
			FREE(pWPDDevInfo);
			pWPDDevInfo = NULL;
		}
	}


	FREE(pDevList);
	pDevList = NULL;
}

/******************************************************************************
* Cleanup_WPDInfo
******************************************************************************/
void Cleanup_USBInfo(PDEVICELIST pDevList)
{
	PUSBDEVIEINFO pUSBDevInfo;

	if(pDevList->pDevInfo)
	{
		pUSBDevInfo = (PUSBDEVIEINFO)(pDevList->pDevInfo);

		//FREE 所有的子项
		if (pUSBDevInfo->pszDeviceID)
		{
			FREE(pUSBDevInfo->pszDeviceID);
			pUSBDevInfo->pszDeviceID = NULL;
		}

		if(pUSBDevInfo->pszLocationPath)
		{
			FREE(pUSBDevInfo->pszLocationPath);
			pUSBDevInfo->pszLocationPath = NULL;
		}

		if (pUSBDevInfo)
		{
			FREE(pUSBDevInfo);
			pUSBDevInfo = NULL;
		}
	}


	FREE(pDevList);
	pDevList = NULL;
}

/******************************************************************************
* EnumStorage
*  枚举Storage设备
******************************************************************************/
#define INTERFACE_DETAIL_SIZE       1024
VOID EnumStorage(LPBOOL flag)
{
	HDEVINFO                            hDevInfo;
	SP_DEVINFO_DATA                     spDevInfoData;
	SP_DEVICE_INTERFACE_DATA            spDevInterfaceData = {0};
	PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetail = NULL;


	TCHAR                   szParentDeviceID[1024];
	STORAGE_DEVICE_NUMBER   StorageDevNumber;
	PSTORAGEDEVIEINFO       pStorageDevInfo;
	HANDLE                  hStorageDevice;

	int     nDevice;
	int     nIndex;


	//生成StorageList
	if(_pStorageListHead == NULL)
	{
		//_pStorageListHead 不存在，申请一个新节点
		_pStorageListHead = DevList_AllocNewNode();
		if(NULL == _pStorageListHead)
		{
			return;
		}
	}
	//如果_pStorageList 中已存在数据，需要销毁
	/*
	if (_pStorageListHead->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(_pStorageListHead->pNext,
			Cleanup_StorageInfo);

		_pStorageListHead->pNext = NULL;
	}
	*/
	CleanupStorageDeviceList(_pStorageListHead);

	// 取得一个该GUID相关的设备信息集句柄
	hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_DISK,             // class GUID
		NULL,                                        // 无关键字
		NULL,                                        // 不指定父窗口句柄
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);      // 目前存在的设备

	// 失败...
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//::AfxMessageBox(_T("SetupDiGetClassDevs FAIL!"));
		OutputDebugString(_T("EnumStorage - SetupDiGetClassDevs"));
		return;
	}

	//扫描所有的设备，筛选出符合条件的
	pspDevInterfaceDetail =(PSP_DEVICE_INTERFACE_DETAIL_DATA) ALLOC(INTERFACE_DETAIL_SIZE * sizeof(TCHAR));
	pspDevInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	nIndex = 0;
	nDevice = 0;

	while ( !*flag )
	{
		spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
		//获取DevInfoData
		if(SetupDiEnumDeviceInfo(hDevInfo,nIndex,&spDevInfoData))
		{
			//获取InterfaceData
			if (SetupDiEnumDeviceInterfaces(hDevInfo,                               // 设备信息集句柄
				NULL,                                   // 不需额外的设备描述
				(LPGUID)&GUID_DEVINTERFACE_DISK,        // GUID
				(ULONG) nIndex,                         // 设备信息集里的设备序号
				&spDevInterfaceData))                   // 设备接口信息)
			{
				//获取InterfaceDetail
				if (SetupDiGetInterfaceDeviceDetail(hDevInfo,                       // 设备信息集句柄
					&spDevInterfaceData,            // 设备接口信息
					pspDevInterfaceDetail,          // 设备接口细节(设备路径)
					INTERFACE_DETAIL_SIZE,          // 输出缓冲区大小
					NULL,                           // 不需计算输出缓冲区大小(直接用设定值)
					NULL))                          // 不需额外的设备描述
				{
					//检测当前设备是否是USBSTOR设备
					if (_tcsstr(pspDevInterfaceDetail->DevicePath,
						_T("usbstor")))
					{
						//是Usb Storage设备

						//获取父系设备的DeviceID
						if(FALSE == GetStorageParentID(spDevInfoData.DevInst,szParentDeviceID))
						{
							//ParentDeviceID 获取失败，继续下一组
							nIndex += 1;
							continue;
						}

						//获取Sotrage设备的DeviceNumber
						//DeviceNumber作为不同存储设备的索引，以此可以根据DeviceNumber检索到盘符
						hStorageDevice = CreateFile(pspDevInterfaceDetail->DevicePath,                 // 设备路径
							GENERIC_READ | GENERIC_WRITE,                      // 读写方式
							FILE_SHARE_READ | FILE_SHARE_WRITE,                // 共享方式
							NULL,                                              // 默认的安全描述符
							OPEN_EXISTING,                                     // 创建方式
							0,                                                 // 不需设置文件属性
							NULL);                                             // 不需参照模板文件
						if(INVALID_HANDLE_VALUE == hStorageDevice)
						{
							DWORD dwErrorCode = GetLastError();
							TCHAR szErrorMsg[1024] = {NULL};
							_stprintf_s(szErrorMsg,_T("EnumStorage - CreateFile(%d)"),dwErrorCode);
							OutputDebugString(szErrorMsg);
							nIndex += 1;
							continue;           //检索下一个storage设备
						}
						//获取DeviceNumber
						if(FALSE == GetStorageDeviceNumber(hStorageDevice,&StorageDevNumber))
						{
							//DeviceNumber获取失败
							OutputDebugString(_T("EnumStorage - GetStorageDeviceNumber"));
							CloseHandle(hStorageDevice);
							nIndex += 1;
							continue;           //检索下一个storage设备
						}

						CloseHandle(hStorageDevice);

						//ParentDeviceID和DeviceNumber全部获取成功
						pStorageDevInfo = (PSTORAGEDEVIEINFO)ALLOC(sizeof(STORAGEDEVIEINFO));
						if(NULL == pStorageDevInfo)
						{
							//申请内测空间失败
							break;
						}
						//申请内存空间，存在设备的ParentDeviceID和设备盘符
						size_t szLen;
						szLen = _tcslen(szParentDeviceID) + 1;
						pStorageDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
						_tcscpy_s(pStorageDevInfo->pszParentDeviceID,szLen,szParentDeviceID);
						pStorageDevInfo->nDiskNum = StorageDevNumber.DeviceNumber;

						//插入到StorageList
						DevList_AddNode(_pStorageListHead,(PVOID)pStorageDevInfo);
					}
				}
			}

			nIndex += 1;
		}
		else
		{
			OutputDebugString(_T("EnumStorage - SetupDiEnumDeviceInfo"));
			break;
		}

	}

	FREE(pspDevInterfaceDetail);
	pspDevInterfaceDetail = NULL;
	SetupDiDestroyDeviceInfoList (hDevInfo);

}

VOID EnumVolume(LPBOOL flag)
{

	STORAGE_DEVICE_NUMBER   StorageDevNumber;
	PVOLUMEDEVICEINFO       pVolumeDevInfo;
	HANDLE                  hVolumeDevice;
	HANDLE                  hFindVolume;
	TCHAR                   szVolumePath[MAX_PATH];

	BOOL    bResult;


	//生成_pVolumeList
	if(_pVolumeListHead == NULL)
	{
		//_pStorageListHead 不存在，申请一个新节点
		_pVolumeListHead = DevList_AllocNewNode();
		if(NULL == _pVolumeListHead)
		{
			return;
		}
	}
	//如果_pVolumeList 中已存在数据，需要销毁

	CleanupVolumeDeviceList(_pVolumeListHead);

	hFindVolume = FindFirstVolume(szVolumePath,MAX_PATH);
	if (hFindVolume == INVALID_HANDLE_VALUE)
	{
		DWORD dwErrorCode = GetLastError();
		TCHAR szErrorMsg[1024] = {NULL};
		_stprintf_s(szErrorMsg,_T("EnumVolume - FindFirstVolume(%d)"),dwErrorCode);
		OutputDebugString(szErrorMsg);
		return;
	}

	do 
	{
		// 去掉VolumePath后面的'\'
		szVolumePath[_tcslen(szVolumePath)-1] = _T('\0');

		//获取Sotrage设备的DeviceNumber
		//DeviceNumber作为不同存储设备的索引，以此可以根据DeviceNumber检索到盘符
		hVolumeDevice = CreateFile(szVolumePath,                 // 设备路径
			GENERIC_READ | GENERIC_WRITE,                      // 读写方式
			FILE_SHARE_READ | FILE_SHARE_WRITE,                // 共享方式
			NULL,                                              // 默认的安全描述符
			OPEN_EXISTING,                                     // 创建方式
			0,                                                 // 不需设置文件属性
			NULL);                                             // 不需参照模板文件

		if(INVALID_HANDLE_VALUE == hVolumeDevice)
		{
			DWORD dwErrorCode = GetLastError();
			TCHAR szErrorMsg[1024] = {NULL};
			_stprintf_s(szErrorMsg,_T("EnumVolume - CreateFile(%d)"),dwErrorCode);
			OutputDebugString(szErrorMsg);
			break;
		}
		//获取DeviceNumber
		if(FALSE == GetStorageDeviceNumber(hVolumeDevice,&StorageDevNumber))
		{
			//DeviceNumber获取失败
			OutputDebugString(_T("EnumVolume - GetStorageDeviceNumber"));
			CloseHandle(hVolumeDevice);
			break;
		}

		CloseHandle(hVolumeDevice);

		//DeviceNumber全部获取成功
		pVolumeDevInfo = (PVOLUMEDEVICEINFO)ALLOC(sizeof(VOLUMEDEVICEINFO));
		if(NULL == pVolumeDevInfo)
		{
			//申请内测空间失败
			break;
		}
		//申请内存空间，存在设备的ParentDeviceID和设备盘符
		size_t szLen;
		szLen = _tcslen(szVolumePath) + 1;
		pVolumeDevInfo->pszVolumePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
		_tcscpy_s(pVolumeDevInfo->pszVolumePath,szLen,szVolumePath);
		pVolumeDevInfo->nDiskNum = StorageDevNumber.DeviceNumber;
		pVolumeDevInfo->PartitionNumber = StorageDevNumber.PartitionNumber;

		//插入到VolumeList
		DevList_AddNode(_pVolumeListHead,(PVOID)pVolumeDevInfo);

		bResult = FindNextVolume(hFindVolume,szVolumePath,MAX_PATH);
		if (!bResult)
		{
			break;
		}
	} while (bResult && !*flag );

	FindVolumeClose(hFindVolume);
}

VOID EnumWPD(LPBOOL flag)
{
	HDEVINFO                            hDevInfo;
	SP_DEVINFO_DATA                     spDevInfoData;
	SP_DEVICE_INTERFACE_DATA            spDevInterfaceData = {0};
	PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetail = NULL;
	PWPDDEVIEINFO                       pWPDDevInfo = NULL;

	TCHAR                               szParentDeviceID[1024] = {NULL};
	TCHAR                               szDeviceID[1024] = {NULL};

	int     nIndex = 0;

	//生成WPDList
	if(_pWPDListHead == NULL)
	{
		//_pStorageListHead 不存在，申请一个新节点
		_pWPDListHead = DevList_AllocNewNode();
		if(NULL == _pWPDListHead)
		{
			return;
		}
	}
	//如果_pWPDListHead 中已存在数据，需要销毁
	CleanupWPDDeviceList(_pWPDListHead);

	// 取得一个该GUID相关的设备信息集句柄
	hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_WPD,             // class GUID
		NULL,                                        // 无关键字
		NULL,                                        // 不指定父窗口句柄
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);      // 目前存在的设备

	// 失败...
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//::AfxMessageBox(_T("SetupDiGetClassDevs FAIL!"));
		OutputDebugString(_T("EnumWPD - SetupDiGetClassDevs"));
		return;
	}

	//扫描所有的设备，筛选出符合条件的
	pspDevInterfaceDetail =(PSP_DEVICE_INTERFACE_DETAIL_DATA) ALLOC(INTERFACE_DETAIL_SIZE * sizeof(TCHAR));
	pspDevInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	while ( !*flag )
	{
		spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
		//获取DevInfoData
		if(SetupDiEnumDeviceInfo(hDevInfo,nIndex,&spDevInfoData))
		{
			//获取InterfaceData
			if (SetupDiEnumDeviceInterfaces(hDevInfo,                               // 设备信息集句柄
				NULL,                                   // 不需额外的设备描述
				(LPGUID)&GUID_DEVINTERFACE_WPD,        // GUID
				(ULONG) nIndex,                         // 设备信息集里的设备序号
				&spDevInterfaceData))                   // 设备接口信息)
			{
				//获取InterfaceDetail
				if (SetupDiGetInterfaceDeviceDetail(hDevInfo,                       // 设备信息集句柄
					&spDevInterfaceData,            // 设备接口信息
					pspDevInterfaceDetail,          // 设备接口细节(设备路径)
					INTERFACE_DETAIL_SIZE,          // 输出缓冲区大小
					NULL,                           // 不需计算输出缓冲区大小(直接用设定值)
					NULL))                          // 不需额外的设备描述
				{

					//获取父系设备的DeviceID
					BOOL bOK = GetStorageParentID(spDevInfoData.DevInst,szParentDeviceID);
					bOK |= GetDeviceID(spDevInfoData.DevInst,szDeviceID);
					if(!bOK)
					{
						//ParentDeviceID 获取失败，继续下一组
						nIndex += 1;
						continue;
					}

					//申请内存空间，存在设备的ParentDeviceID和设备盘符
					pWPDDevInfo = (PWPDDEVIEINFO)ALLOC(sizeof(WPDDEVIEINFO));
					if(NULL == pWPDDevInfo)
					{
						//申请内测空间失败
						break;
					}
					size_t szLen;

					if (szParentDeviceID)
					{
						szLen = _tcslen(szParentDeviceID) + 1;
						pWPDDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
						_tcscpy_s(pWPDDevInfo->pszParentDeviceID,szLen,szParentDeviceID);
					}
					
					if (szDeviceID)
					{
						szLen = _tcslen(szDeviceID) + 1;
						pWPDDevInfo->pszDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
						_tcscpy_s(pWPDDevInfo->pszDeviceID,szLen,szDeviceID);
					}

					szLen = _tcslen(pspDevInterfaceDetail->DevicePath) + 1;
					pWPDDevInfo->pszDevicePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pWPDDevInfo->pszDevicePath,szLen,pspDevInterfaceDetail->DevicePath);

					//插入到StorageList
					DevList_AddNode(_pWPDListHead,(PVOID)pWPDDevInfo);
					
				}

			}

			nIndex += 1;
		}
		else
		{
			OutputDebugString(_T("EnumWPD - SetupDiEnumDeviceInfo"));
			break;
		}

	}

	FREE(pspDevInterfaceDetail);
	pspDevInterfaceDetail = NULL;
	SetupDiDestroyDeviceInfoList (hDevInfo);
}


/******************************************************************************
* MatchStorageDeivceID
******************************************************************************/
PSTORAGEDEVIEINFO 
	MatchStorageDeivceID(PTSTR pszDeviceID)
{
	OutputDebugString(_T("MatchStorageDeivceID"));
	OutputDebugString(pszDeviceID);
	if(NULL == pszDeviceID)
	{
		return NULL;
	}

	PDEVICELIST pListNode = _pStorageListHead->pNext;

	while(pListNode)
	{
		if(pListNode->pDevInfo)
		{
			//设备节点存在
			if(0 == _tcscmp(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID,pszDeviceID))
			{
				//找到了匹配的字串
				PSTORAGEDEVIEINFO pStorageDevInfo = (PSTORAGEDEVIEINFO)ALLOC(sizeof(STORAGEDEVIEINFO));
				if(NULL == pStorageDevInfo)
				{
					//申请内测空间失败
					break;
				}
				//申请内存空间，存在设备的ParentDeviceID和设备盘符
				size_t szLen;
				szLen = _tcslen(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID) + 1;
				pStorageDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pStorageDevInfo->pszParentDeviceID,szLen,((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID);
				pStorageDevInfo->nDiskNum = ((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->nDiskNum;
				return pStorageDevInfo;
			}
		}

		pListNode = pListNode->pNext;
	}


	return NULL;
}

PWPDDEVIEINFO 
	MatchWPDDeviceID(PTSTR pszDeviceID)
{
	OutputDebugString(_T("MatchWPDDeivceID"));
	OutputDebugString(pszDeviceID);
	if(NULL == pszDeviceID)
	{
		return NULL;
	}

	PDEVICELIST pListNode = _pWPDListHead->pNext;

	while(pListNode)
	{
		if(pListNode->pDevInfo)
		{
			//设备节点存在
			if(0 == _tcscmp(((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID,pszDeviceID)
				|| 0 == _tcscmp(((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID,pszDeviceID))
			{
				//找到了匹配的字串
				PWPDDEVIEINFO pWPDDevInfo = (PWPDDEVIEINFO)ALLOC(sizeof(WPDDEVIEINFO));
				if(NULL == pWPDDevInfo)
				{
					//申请内测空间失败
					break;
				}
				//申请内存空间，存在设备的ParentDeviceID和设备盘符
				size_t szLen;
				if (((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID != NULL)
				{
					szLen = _tcslen(((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID) + 1;
					pWPDDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pWPDDevInfo->pszParentDeviceID,szLen,((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID);
				}

				if (((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID != NULL)
				{
					szLen = _tcslen(((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID) + 1;
					pWPDDevInfo->pszDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pWPDDevInfo->pszDeviceID,szLen,((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID);
				}
				
				szLen = _tcslen(((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDevicePath) + 1;
				pWPDDevInfo->pszDevicePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pWPDDevInfo->pszDevicePath,szLen,((PWPDDEVIEINFO)(pListNode->pDevInfo))->pszDevicePath);

				return pWPDDevInfo;
			}
		}

		pListNode = pListNode->pNext;
	}


	return NULL;
}

PDEVICELIST MatchStorageDeivceIDs( PTSTR pszDeviceID )
{
	OutputDebugString(_T("MatchStorageDeivceID"));
	OutputDebugString(pszDeviceID);
	if(NULL == pszDeviceID)
	{
		return NULL;
	}

	PDEVICELIST pListStorage = NULL;
	pListStorage = DevList_AllocNewNode();

	PDEVICELIST pListNode = _pStorageListHead->pNext;

	while(pListNode)
	{
		if(pListNode->pDevInfo)
		{
			//设备节点存在
			if(0 == _tcscmp(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID,pszDeviceID))
			{
				PSTORAGEDEVIEINFO pStorageDevInfo = (PSTORAGEDEVIEINFO)ALLOC(sizeof(STORAGEDEVIEINFO));
				if(NULL == pStorageDevInfo)
				{
					//申请内测空间失败
					break;
				}
				//申请内存空间，存在设备的ParentDeviceID和设备盘符
				size_t szLen;
				szLen = _tcslen(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID) + 1;
				pStorageDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pStorageDevInfo->pszParentDeviceID,szLen,((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID);
				pStorageDevInfo->nDiskNum = ((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->nDiskNum;

				//插入到StorageList
				DevList_AddNode(pListStorage,(PVOID)pStorageDevInfo);
			}
		}

		pListNode = pListNode->pNext;
	}


	return pListStorage;
}

PDEVICELIST MatchVolumeDeviceDiskNums( int nDiskNum )
{
	OutputDebugString(_T("MatchVolumeDeivceDiskNum"));

	PDEVICELIST pListVolume = NULL;
	pListVolume = DevList_AllocNewNode();

	PDEVICELIST pListNode = _pVolumeListHead->pNext;

	while(pListNode)
	{
		if(pListNode->pDevInfo)
		{
			//设备节点存在
			if(nDiskNum == ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->nDiskNum)
			{
				PVOLUMEDEVICEINFO pVolumeDevInfo = (PVOLUMEDEVICEINFO)ALLOC(sizeof(VOLUMEDEVICEINFO));
				if(NULL == pVolumeDevInfo)
				{
					//申请内测空间失败
					break;
				}
				//申请内存空间，存在设备的ParentDeviceID和设备盘符
				size_t szLen;
				szLen = _tcslen(((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->pszVolumePath) + 1;
				pVolumeDevInfo->pszVolumePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pVolumeDevInfo->pszVolumePath,szLen,((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->pszVolumePath);
				pVolumeDevInfo->nDiskNum = ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->nDiskNum;
				pVolumeDevInfo->PartitionNumber = ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->PartitionNumber;

				//插入到StorageList
				DevList_AddNode(pListVolume,(PVOID)pVolumeDevInfo);
			}
		}

		pListNode = pListNode->pNext;
	}


	return pListVolume;
}

VOID CleanStorageDevieInfo( PSTORAGEDEVIEINFO pStoDevInfo )
{
	if (pStoDevInfo)
	{
		if (pStoDevInfo->pszParentDeviceID)
		{
			FREE(pStoDevInfo->pszParentDeviceID);
			pStoDevInfo->pszParentDeviceID = NULL;
		}

		pStoDevInfo->nDiskNum = -1;

		FREE(pStoDevInfo);
		pStoDevInfo = NULL;
	}
}

BOOL GetDriveProperty( HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc )
{
	STORAGE_PROPERTY_QUERY Query; // 查询输入参数
	DWORD dwOutBytes; // IOCTL输出数据长度
	BOOL bResult; // IOCTL返回值

	// 指定查询方式
	Query.PropertyId = StorageDeviceProperty;
	Query.QueryType = PropertyStandardQuery;

	// 用IOCTL_STORAGE_QUERY_PROPERTY取设备属性信息
	bResult = ::DeviceIoControl(hDevice, // 设备句柄
		IOCTL_STORAGE_QUERY_PROPERTY, // 取设备属性信息
		&Query, sizeof(STORAGE_PROPERTY_QUERY), // 输入数据缓冲区
		pDevDesc, pDevDesc->Size, // 输出数据缓冲区
		&dwOutBytes, // 输出数据长度
		(LPOVERLAPPED)NULL); // 用同步I/O    

	return bResult;
}

VOID CleanupStorageDeviceList( PDEVICELIST pDevList )
{
	if (pDevList && pDevList->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(pDevList->pNext,
			Cleanup_StorageInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanupStorage()
{
	if (_pStorageListHead->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(_pStorageListHead->pNext,
			Cleanup_StorageInfo);

		_pStorageListHead->pNext = NULL;
	}
}

VOID CleanupVolumeDeviceList( PDEVICELIST pDevList )
{
	if (pDevList && pDevList->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(pDevList->pNext,
			Cleanup_VolumeInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanVolumeDeviceInfo( PVOLUMEDEVICEINFO pVolumeDevInfo )
{
	if (pVolumeDevInfo)
	{
		if (pVolumeDevInfo->pszVolumePath)
		{
			FREE(pVolumeDevInfo->pszVolumePath);
			pVolumeDevInfo->pszVolumePath = NULL;
		}

		pVolumeDevInfo->nDiskNum = -1;

		FREE(pVolumeDevInfo);
		pVolumeDevInfo = NULL;
	}
}

VOID CleanupVolume()
{
	if (_pVolumeListHead->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(_pVolumeListHead->pNext,
			Cleanup_VolumeInfo);

		_pVolumeListHead->pNext = NULL;
	}
}

VOID CleanWPDDeviceInfo(PWPDDEVIEINFO pWPDDevInfo)
{
	if (pWPDDevInfo)
	{
		if (pWPDDevInfo->pszDevicePath)
		{
			FREE(pWPDDevInfo->pszDevicePath);
			pWPDDevInfo->pszDevicePath = NULL;
		}
		
		if (pWPDDevInfo->pszParentDeviceID)
		{
			FREE(pWPDDevInfo->pszParentDeviceID);
			pWPDDevInfo->pszParentDeviceID = NULL;
		}

		if (pWPDDevInfo->pszDeviceID)
		{
			FREE(pWPDDevInfo->pszDeviceID);
			pWPDDevInfo->pszDeviceID = NULL;
		}

		FREE(pWPDDevInfo);
		pWPDDevInfo = NULL;
	}
}

VOID CleanupWPDDeviceList(PDEVICELIST pDevList)
{
	if (pDevList && pDevList->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(pDevList->pNext,
			Cleanup_WPDInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanupWPD()
{
	CleanupWPDDeviceList(_pWPDListHead);
}

VOID EnumUSB( LPBOOL flag )
{
	HDEVINFO                            hDevInfo;
	SP_DEVINFO_DATA                     spDevInfoData;
	PUSBDEVIEINFO                       pUSBDevInfo = NULL;

	int     nIndex = 0;

	//生成USBList
	if(_pUSBListHead == NULL)
	{
		//_pStorageListHead 不存在，申请一个新节点
		_pUSBListHead = DevList_AllocNewNode();
		if(NULL == _pUSBListHead)
		{
			return;
		}
	}
	//如果_pWPDListHead 中已存在数据，需要销毁
	CleanupUSBDeviceList(_pUSBListHead);

	// 取得一个该GUID相关的设备信息集句柄
	hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_USB,             // class GUID
		NULL,                                        // 无关键字
		NULL,                                        // 不指定父窗口句柄
		DIGCF_PRESENT);      // 目前存在的设备

	// 失败...
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//::AfxMessageBox(_T("SetupDiGetClassDevs FAIL!"));
		OutputDebugString(_T("EnumUSB - SetupDiGetClassDevs"));
		return;
	}

	while ( !*flag )
	{
		spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
		//获取DevInfoData
		if(SetupDiEnumDeviceInfo(hDevInfo,nIndex,&spDevInfoData))
		{
			DWORD DataT;
			LPTSTR pszDeviceID = NULL;
			LPTSTR pszLocationPath = NULL;
			DWORD buffersize = 0;

			while ( !SetupDiGetDeviceInstanceId(
				hDevInfo, 
				&spDevInfoData, 
				pszDeviceID, 
				buffersize, 
				&buffersize))
			{
				DWORD dwError = GetLastError() ;
				if(  dwError == ERROR_INSUFFICIENT_BUFFER )
				{
					// Change the buffer size.
					if( pszDeviceID ) 
					{
						FREE(pszDeviceID );
					}

					pszDeviceID = (LPTSTR)ALLOC(buffersize * sizeof(TCHAR));
				}
				else
				{
					// Insert error handling here.
					break;
				}
			}

			buffersize = 0;
			while( !SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&spDevInfoData,
				SPDRP_LOCATION_PATHS,
				&DataT,
				(PBYTE)pszLocationPath,
				buffersize,
				&buffersize ) )
			{
				DWORD dwError = GetLastError() ;
				if(  dwError == ERROR_INSUFFICIENT_BUFFER )
				{
					// Change the buffer size.
					if(pszLocationPath)
					{
						FREE(pszLocationPath);
					}

					pszLocationPath = (LPTSTR)ALLOC(buffersize * sizeof(TCHAR));
				}
				else
				{
					// Insert error handling here.
					break;
				}
			}

			if (pszDeviceID && pszLocationPath)
			{
				pUSBDevInfo = (PUSBDEVIEINFO)ALLOC(sizeof(USBDEVIEINFO));
				if (pUSBDevInfo == NULL)
				{
					if (pszDeviceID)
					{
						FREE(pszDeviceID);
					}

					if (pszLocationPath)
					{
						FREE(pszLocationPath);
					}

					break;
				}

				size_t szLen;

				if (pszDeviceID)
				{
					szLen = _tcslen(pszDeviceID) + 1;
					pUSBDevInfo->pszDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pUSBDevInfo->pszDeviceID,szLen,pszDeviceID);

					FREE(pszDeviceID);
				}

				if (pszLocationPath)
				{
					szLen = _tcslen(pszLocationPath) + 1;
					pUSBDevInfo->pszLocationPath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pUSBDevInfo->pszLocationPath,szLen,pszLocationPath);

					FREE(pszLocationPath);
				}

				//插入到USBList
				DevList_AddNode(_pUSBListHead,(PVOID)pUSBDevInfo);
			}
			else
			{
				if (pszDeviceID)
				{
					FREE(pszDeviceID);
				}

				if (pszLocationPath)
				{
					FREE(pszLocationPath);
				}
			}

			

			nIndex += 1;
		}
		else
		{
			OutputDebugString(_T("EnumUSB - SetupDiEnumDeviceInfo"));
			break;
		}

	}

	SetupDiDestroyDeviceInfoList (hDevInfo);
}

PUSBDEVIEINFO MatchUSBDeviceID( PTSTR pszDeviceID )
{
	OutputDebugString(_T("MatchUSBDeviceID"));
	OutputDebugString(pszDeviceID);
	if(NULL == pszDeviceID)
	{
		return NULL;
	}

	PDEVICELIST pListNode = _pUSBListHead->pNext;

	PUSBDEVIEINFO pUSB = NULL;

	while(pListNode)
	{
		if(pListNode->pDevInfo)
		{
			//设备节点存在
			if(0 == _tcscmp(((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID,pszDeviceID))
			{
				//找到了匹配的字串
				pUSB = (PUSBDEVIEINFO)(pListNode->pDevInfo);
				break;
			}
		}

		pListNode = pListNode->pNext;
	}

	pListNode = _pUSBListHead->pNext;

	while(pListNode && pUSB)
	{
		if(pListNode->pDevInfo)
		{
			//设备的位置路径相同，deviceID不同的设备
			if(0 != _tcscmp(((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID,pszDeviceID)
				&& 0 == _tcsncmp(((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszLocationPath,pUSB->pszLocationPath,_tcslen(pUSB->pszLocationPath)))
			{
				//找到了匹配的字串
				//找到了匹配的字串
				PUSBDEVIEINFO pUSBDevInfo = (PUSBDEVIEINFO)ALLOC(sizeof(USBDEVIEINFO));
				if(NULL == pUSBDevInfo)
				{
					//申请内测空间失败
					break;
				}
				//申请内存空间，存在设备的ParentDeviceID和设备盘符
				size_t szLen;

				if (((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID != NULL)
				{
					szLen = _tcslen(((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID) + 1;
					pUSBDevInfo->pszDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pUSBDevInfo->pszDeviceID,szLen,((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszDeviceID);
				}

				if (((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszLocationPath != NULL)
				{
					szLen = _tcslen(((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszLocationPath) + 1;
					pUSBDevInfo->pszLocationPath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
					_tcscpy_s(pUSBDevInfo->pszLocationPath,szLen,((PUSBDEVIEINFO)(pListNode->pDevInfo))->pszLocationPath);
				}

				return pUSBDevInfo;
			}
		}

		pListNode = pListNode->pNext;
	}

	return NULL;
}

VOID CleanUSBDeviceInfo( PUSBDEVIEINFO pUSBDevInfo )
{
	if (pUSBDevInfo)
	{
		if (pUSBDevInfo->pszLocationPath)
		{
			FREE(pUSBDevInfo->pszLocationPath);
			pUSBDevInfo->pszLocationPath = NULL;
		}

		if (pUSBDevInfo->pszDeviceID)
		{
			FREE(pUSBDevInfo->pszDeviceID);
			pUSBDevInfo->pszDeviceID = NULL;
		}

		FREE(pUSBDevInfo);
		pUSBDevInfo = NULL;
	}
}

VOID CleanupUSBDeviceList( PDEVICELIST pDevList )
{
	if (pDevList && pDevList->pNext != NULL)
	{
		//已存在StorageList,销毁全部数据，重新生成
		DevList_Walk(pDevList->pNext,
			Cleanup_USBInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanupUSB()
{
	CleanupUSBDeviceList(_pUSBListHead);
}
