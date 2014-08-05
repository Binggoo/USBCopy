
/*****************************************************************************
                Copyright (c) Phiyo Technology 
Module Name:  
	DeviceList.h
Created on:
	2011-9-22
Author:
	Administrator
*****************************************************************************/

#ifndef DEVICELIST_H_
#define DEVICELIST_H_

/******************************************************************************
 * Memory Operation
 ******************************************************************************/
#ifndef _MEM_OPERATION
#define _MEM_OPERATION

#define ALLOC(dwBytes)          GlobalAlloc(GPTR,(dwBytes))
#define REALLOC(hMem, dwBytes)  GlobalReAlloc((hMem), (dwBytes), (GMEM_MOVEABLE|GMEM_ZEROINIT))
#define FREE(hMem)              GlobalFree((hMem))

#endif

/******************************************************************************
 * DeviceList数据结构体
 ******************************************************************************/
typedef struct  _t_DEVICELIST
{
    PVOID					pDevInfo;
    struct _t_DEVICELIST	*pNext;
}DEVICELIST,*PDEVICELIST;

// Callback function for walking DeviceList Items
typedef VOID
(*LPFN_DEVLIST_CALLBACK)(PDEVICELIST pDevList);

/******************************************************************************
 * 函数申明
 ******************************************************************************/
PDEVICELIST
DevList_AllocNewNode(void);
BOOL
DevList_AddNode(PDEVICELIST pPrev,
                PVOID pDevInfo);
VOID
DevList_Walk(PDEVICELIST pDevList,
             LPFN_DEVLIST_CALLBACK lpfnDevListCallBack);

#endif /* DEVICELIST_H_ */
