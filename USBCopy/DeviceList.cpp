
/*****************************************************************************
                Copyright (c) Phiyo Technology 
Module Name:  
	DeviceList.cpp
Created on:
	2011-9-22
Author:
	Administrator
*****************************************************************************/


/******************************************************************************
 * include files
 ******************************************************************************/
#include "stdafx.h"

#include "DeviceList.h"


/******************************************************************************
 * DevList_AllocNewNode
 ******************************************************************************/
PDEVICELIST
DevList_AllocNewNode(void)
{
    PDEVICELIST pNew = (PDEVICELIST)ALLOC(sizeof(DEVICELIST));
    if(!pNew)
    {
        return NULL;
    }

    pNew->pDevInfo = NULL;
    pNew->pNext = NULL;

    return pNew;
}

/******************************************************************************
 * DevList_AddNode
 ******************************************************************************/
BOOL
DevList_AddNode(PDEVICELIST pPrev,
                PVOID pDevInfo)
{
    PDEVICELIST pNew;

    pNew = DevList_AllocNewNode();
    if(!pNew)
    {
        return FALSE;
    }

    pNew->pDevInfo = pDevInfo;
    pNew->pNext = pPrev->pNext;
    pPrev->pNext = pNew;

    return TRUE;

}

/******************************************************************************
 * DevList_Walk
 *  DevList 遍历执行功能
 ******************************************************************************/
VOID
DevList_Walk(PDEVICELIST pDevList,
             LPFN_DEVLIST_CALLBACK lpfnDevListCallBack)
{

    if(pDevList)
    {
        DevList_Walk(pDevList->pNext,lpfnDevListCallBack);

        //执行节点功能
        lpfnDevListCallBack(pDevList);
    }
}
