
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

#include "EnumStorage.h"

PDEVICELIST _pStorageListHead = NULL;
PDEVICELIST _pVolumeListHead = NULL;


/******************************************************************************
* GetStorageParentID
*  ��ȡ��ϵ�豸��ID
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
		return NULL;
	}

	//Get parent Device DeviceID
	if(CR_SUCCESS != CM_Get_Device_ID(hDevParent,pszDeviceID,ulSize + 1,0))
	{
		//DeviceID ��ȡʧ�ܣ��ͷ��ڴ�
		return NULL;
	}

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

	////ʹ��IOCTL_STORAGE_GET_DEVICE_NUMBER
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
		//IOCTL_STORAGE_GET_DEVICE_NUMBER ʧ��
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

		//����DiskExtents��Ϣ �� pStorageDevNumber
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

		//FREE ���е�����
		if(pStorageDevInfo->pszParentDeviceID)
		{
			FREE(pStorageDevInfo->pszParentDeviceID);
		}

		if (pStorageDevInfo)
		{
			FREE(pStorageDevInfo);
		}
	}


	FREE(pDevList);
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

		//FREE ���е�����
		if(pVolumeDevInfo->pszVolumePath)
		{
			FREE(pVolumeDevInfo->pszVolumePath);
		}

		if (pVolumeDevInfo)
		{
			FREE(pVolumeDevInfo);
		}
	}


	FREE(pDevList);
}

/******************************************************************************
* EnumStorage
*  ö��Storage�豸
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


	//����StorageList
	if(_pStorageListHead == NULL)
	{
		//_pStorageListHead �����ڣ�����һ���½ڵ�
		_pStorageListHead = DevList_AllocNewNode();
		if(NULL == _pStorageListHead)
		{
			return;
		}
	}
	//���_pStorageList ���Ѵ������ݣ���Ҫ����
	/*
	if (_pStorageListHead->pNext != NULL)
	{
		//�Ѵ���StorageList,����ȫ�����ݣ���������
		DevList_Walk(_pStorageListHead->pNext,
			Cleanup_StorageInfo);

		_pStorageListHead->pNext = NULL;
	}
	*/
	CleanupStorageDeviceList(_pStorageListHead);

	// ȡ��һ����GUID��ص��豸��Ϣ�����
	hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_DISK,             // class GUID
		NULL,                                        // �޹ؼ���
		NULL,                                        // ��ָ�������ھ��
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);      // Ŀǰ���ڵ��豸

	// ʧ��...
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//::AfxMessageBox(_T("SetupDiGetClassDevs FAIL!"));
		OutputDebugString(_T("EnumStorage - SetupDiGetClassDevs"));
		return;
	}

	//ɨ�����е��豸��ɸѡ������������
	pspDevInterfaceDetail =(PSP_DEVICE_INTERFACE_DETAIL_DATA) ALLOC(INTERFACE_DETAIL_SIZE * sizeof(TCHAR));
	pspDevInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	nIndex = 0;
	nDevice = 0;

	while ( !*flag )
	{
		spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
		//��ȡDevInfoData
		if(SetupDiEnumDeviceInfo(hDevInfo,nIndex,&spDevInfoData))
		{
			//��ȡInterfaceData
			if (SetupDiEnumDeviceInterfaces(hDevInfo,                               // �豸��Ϣ�����
				NULL,                                   // ���������豸����
				(LPGUID)&GUID_DEVINTERFACE_DISK,        // GUID
				(ULONG) nIndex,                         // �豸��Ϣ������豸���
				&spDevInterfaceData))                   // �豸�ӿ���Ϣ)
			{
				//��ȡInterfaceDetail
				if (SetupDiGetInterfaceDeviceDetail(hDevInfo,                       // �豸��Ϣ�����
					&spDevInterfaceData,            // �豸�ӿ���Ϣ
					pspDevInterfaceDetail,          // �豸�ӿ�ϸ��(�豸·��)
					INTERFACE_DETAIL_SIZE,          // �����������С
					NULL,                           // ������������������С(ֱ�����趨ֵ)
					NULL))                          // ���������豸����
				{
					//��⵱ǰ�豸�Ƿ���USBSTOR�豸
					if (_tcsstr(pspDevInterfaceDetail->DevicePath,
						_T("usbstor")))
					{
						//��Usb Storage�豸

						//��ȡ��ϵ�豸��DeviceID
						if(FALSE == GetStorageParentID(spDevInfoData.DevInst,szParentDeviceID))
						{
							//ParentDeviceID ��ȡʧ�ܣ�������һ��
							nIndex += 1;
							continue;
						}

						//��ȡSotrage�豸��DeviceNumber
						//DeviceNumber��Ϊ��ͬ�洢�豸���������Դ˿��Ը���DeviceNumber�������̷�
						hStorageDevice = CreateFile(pspDevInterfaceDetail->DevicePath,                 // �豸·��
							GENERIC_READ | GENERIC_WRITE,                      // ��д��ʽ
							FILE_SHARE_READ | FILE_SHARE_WRITE,                // ����ʽ
							NULL,                                              // Ĭ�ϵİ�ȫ������
							OPEN_EXISTING,                                     // ������ʽ
							0,                                                 // ���������ļ�����
							NULL);                                             // �������ģ���ļ�
						if(INVALID_HANDLE_VALUE == hStorageDevice)
						{
							DWORD dwErrorCode = GetLastError();
							TCHAR szErrorMsg[1024] = {NULL};
							_stprintf_s(szErrorMsg,_T("EnumStorage - CreateFile(%d)"),dwErrorCode);
							OutputDebugString(szErrorMsg);
							nIndex += 1;
							continue;           //������һ��storage�豸
						}
						//��ȡDeviceNumber
						if(FALSE == GetStorageDeviceNumber(hStorageDevice,&StorageDevNumber))
						{
							//DeviceNumber��ȡʧ��
							OutputDebugString(_T("EnumStorage - GetStorageDeviceNumber"));
							CloseHandle(hStorageDevice);
							nIndex += 1;
							continue;           //������һ��storage�豸
						}

						CloseHandle(hStorageDevice);

						//ParentDeviceID��DeviceNumberȫ����ȡ�ɹ�
						pStorageDevInfo = (PSTORAGEDEVIEINFO)ALLOC(sizeof(STORAGEDEVIEINFO));
						if(NULL == pStorageDevInfo)
						{
							//�����ڲ�ռ�ʧ��
							break;
						}
						//�����ڴ�ռ䣬�����豸��ParentDeviceID���豸�̷�
						size_t szLen;
						szLen = _tcslen(szParentDeviceID) + 1;
						pStorageDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
						_tcscpy_s(pStorageDevInfo->pszParentDeviceID,szLen,szParentDeviceID);
						pStorageDevInfo->nDiskNum = StorageDevNumber.DeviceNumber;

						//���뵽StorageList
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


	//����_pVolumeList
	if(_pVolumeListHead == NULL)
	{
		//_pStorageListHead �����ڣ�����һ���½ڵ�
		_pVolumeListHead = DevList_AllocNewNode();
		if(NULL == _pVolumeListHead)
		{
			return;
		}
	}
	//���_pVolumeList ���Ѵ������ݣ���Ҫ����

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
		// ȥ��VolumePath�����'\'
		szVolumePath[_tcslen(szVolumePath)-1] = _T('\0');

		//��ȡSotrage�豸��DeviceNumber
		//DeviceNumber��Ϊ��ͬ�洢�豸���������Դ˿��Ը���DeviceNumber�������̷�
		hVolumeDevice = CreateFile(szVolumePath,                 // �豸·��
			GENERIC_READ | GENERIC_WRITE,                      // ��д��ʽ
			FILE_SHARE_READ | FILE_SHARE_WRITE,                // ����ʽ
			NULL,                                              // Ĭ�ϵİ�ȫ������
			OPEN_EXISTING,                                     // ������ʽ
			0,                                                 // ���������ļ�����
			NULL);                                             // �������ģ���ļ�

		if(INVALID_HANDLE_VALUE == hVolumeDevice)
		{
			DWORD dwErrorCode = GetLastError();
			TCHAR szErrorMsg[1024] = {NULL};
			_stprintf_s(szErrorMsg,_T("EnumVolume - CreateFile(%d)"),dwErrorCode);
			OutputDebugString(szErrorMsg);
			break;
		}
		//��ȡDeviceNumber
		if(FALSE == GetStorageDeviceNumber(hVolumeDevice,&StorageDevNumber))
		{
			//DeviceNumber��ȡʧ��
			OutputDebugString(_T("EnumVolume - GetStorageDeviceNumber"));
			CloseHandle(hVolumeDevice);
			break;
		}

		CloseHandle(hVolumeDevice);

		//DeviceNumberȫ����ȡ�ɹ�
		pVolumeDevInfo = (PVOLUMEDEVICEINFO)ALLOC(sizeof(VOLUMEDEVICEINFO));
		if(NULL == pVolumeDevInfo)
		{
			//�����ڲ�ռ�ʧ��
			break;
		}
		//�����ڴ�ռ䣬�����豸��ParentDeviceID���豸�̷�
		size_t szLen;
		szLen = _tcslen(szVolumePath) + 1;
		pVolumeDevInfo->pszVolumePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
		_tcscpy_s(pVolumeDevInfo->pszVolumePath,szLen,szVolumePath);
		pVolumeDevInfo->nDiskNum = StorageDevNumber.DeviceNumber;
		pVolumeDevInfo->PartitionNumber = StorageDevNumber.PartitionNumber;

		//���뵽VolumeList
		DevList_AddNode(_pVolumeListHead,(PVOID)pVolumeDevInfo);

		bResult = FindNextVolume(hFindVolume,szVolumePath,MAX_PATH);
		if (!bResult)
		{
			break;
		}
	} while (bResult && !*flag );

	FindVolumeClose(hFindVolume);
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
			//�豸�ڵ����
			if(0 == _tcscmp(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID,pszDeviceID))
			{
				//�ҵ���ƥ����ִ�
				return (PSTORAGEDEVIEINFO)(pListNode->pDevInfo);
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
			//�豸�ڵ����
			if(0 == _tcscmp(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID,pszDeviceID))
			{
				PSTORAGEDEVIEINFO pStorageDevInfo = (PSTORAGEDEVIEINFO)ALLOC(sizeof(STORAGEDEVIEINFO));
				if(NULL == pStorageDevInfo)
				{
					//�����ڲ�ռ�ʧ��
					break;
				}
				//�����ڴ�ռ䣬�����豸��ParentDeviceID���豸�̷�
				size_t szLen;
				szLen = _tcslen(((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID) + 1;
				pStorageDevInfo->pszParentDeviceID = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pStorageDevInfo->pszParentDeviceID,szLen,((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->pszParentDeviceID);
				pStorageDevInfo->nDiskNum = ((PSTORAGEDEVIEINFO)(pListNode->pDevInfo))->nDiskNum;

				//���뵽StorageList
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
			//�豸�ڵ����
			if(nDiskNum == ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->nDiskNum)
			{
				PVOLUMEDEVICEINFO pVolumeDevInfo = (PVOLUMEDEVICEINFO)ALLOC(sizeof(VOLUMEDEVICEINFO));
				if(NULL == pVolumeDevInfo)
				{
					//�����ڲ�ռ�ʧ��
					break;
				}
				//�����ڴ�ռ䣬�����豸��ParentDeviceID���豸�̷�
				size_t szLen;
				szLen = _tcslen(((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->pszVolumePath) + 1;
				pVolumeDevInfo->pszVolumePath = (PTSTR)ALLOC(szLen * sizeof(TCHAR));
				_tcscpy_s(pVolumeDevInfo->pszVolumePath,szLen,((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->pszVolumePath);
				pVolumeDevInfo->nDiskNum = ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->nDiskNum;
				pVolumeDevInfo->PartitionNumber = ((PVOLUMEDEVICEINFO)(pListNode->pDevInfo))->PartitionNumber;

				//���뵽StorageList
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
		FREE(pStoDevInfo->pszParentDeviceID);
		pStoDevInfo->pszParentDeviceID = NULL;

		pStoDevInfo->nDiskNum = -1;

		FREE(pStoDevInfo);

		pStoDevInfo = NULL;
	}
}

BOOL GetDriveProperty( HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc )
{
	STORAGE_PROPERTY_QUERY Query; // ��ѯ�������
	DWORD dwOutBytes; // IOCTL������ݳ���
	BOOL bResult; // IOCTL����ֵ

	// ָ����ѯ��ʽ
	Query.PropertyId = StorageDeviceProperty;
	Query.QueryType = PropertyStandardQuery;

	// ��IOCTL_STORAGE_QUERY_PROPERTYȡ�豸������Ϣ
	bResult = ::DeviceIoControl(hDevice, // �豸���
		IOCTL_STORAGE_QUERY_PROPERTY, // ȡ�豸������Ϣ
		&Query, sizeof(STORAGE_PROPERTY_QUERY), // �������ݻ�����
		pDevDesc, pDevDesc->Size, // ������ݻ�����
		&dwOutBytes, // ������ݳ���
		(LPOVERLAPPED)NULL); // ��ͬ��I/O    

	return bResult;
}

VOID CleanupStorageDeviceList( PDEVICELIST pDevList )
{
	if (pDevList->pNext != NULL)
	{
		//�Ѵ���StorageList,����ȫ�����ݣ���������
		DevList_Walk(pDevList->pNext,
			Cleanup_StorageInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanupStorage()
{
	if (_pStorageListHead->pNext != NULL)
	{
		//�Ѵ���StorageList,����ȫ�����ݣ���������
		DevList_Walk(_pStorageListHead->pNext,
			Cleanup_StorageInfo);

		_pStorageListHead->pNext = NULL;
	}
}

VOID CleanupVolumeDeviceList( PDEVICELIST pDevList )
{
	if (pDevList->pNext != NULL)
	{
		//�Ѵ���StorageList,����ȫ�����ݣ���������
		DevList_Walk(pDevList->pNext,
			Cleanup_VolumeInfo);

		pDevList->pNext = NULL;
	}
}

VOID CleanVolumeDeviceInfo( PVOLUMEDEVICEINFO pVolumeDevInfo )
{
	if (pVolumeDevInfo)
	{
		FREE(pVolumeDevInfo->pszVolumePath);
		pVolumeDevInfo->pszVolumePath = NULL;

		pVolumeDevInfo->nDiskNum = -1;

		FREE(pVolumeDevInfo);

		pVolumeDevInfo = NULL;
	}
}

VOID CleanupVolume()
{
	if (_pVolumeListHead->pNext != NULL)
	{
		//�Ѵ���StorageList,����ȫ�����ݣ���������
		DevList_Walk(_pVolumeListHead->pNext,
			Cleanup_VolumeInfo);

		_pVolumeListHead->pNext = NULL;
	}
}
