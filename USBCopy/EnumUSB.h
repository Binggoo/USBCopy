
/*****************************************************************************
                Copyright (c) Phiyo Technology 
Module Name:  
	EnumUSB.h
Created on:
	2011-9-9
Author:
	Administrator
*****************************************************************************/

#ifndef ENUMUSB_H_
#define ENUMUSB_H_

#pragma once

/******************************************************************************
 * include files
 ******************************************************************************/
#include <usbioctl.h>
#include <usbiodef.h>
#include <usb100.h>
#include <usb200.h>


//*****************************************************************************
// P R A G M A S
//*****************************************************************************
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4200) // named type definition in parentheses
#pragma warning(disable:4100) // named type definition in parentheses

#pragma intrinsic(strlen, strcpy, memset)

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#ifdef  DEBUG
#undef  DBG
#define DBG 1
#endif

#if DBG
//#define OOPS() Oops(_T(__FILE__), __LINE__)
#define OOPS()																						\
	{																								\
		TCHAR oopsBuf[256];																			\
		_stprintf_s(oopsBuf, sizeof(oopsBuf)/sizeof(TCHAR), _T("File: %s, Line %d"), _T(__FILE__), __LINE__);		\
		MessageBox(NULL, oopsBuf, _T("Oops!"), MB_OK);												\
	}
#else
#define OOPS()
#endif


#define ALLOC(dwBytes)			GlobalAlloc(GPTR,(dwBytes))

#define REALLOC(hMem, dwBytes)	GlobalReAlloc((hMem), (dwBytes), (GMEM_MOVEABLE|GMEM_ZEROINIT))

#define FREE(hMem)				GlobalFree((hMem))



//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

#include "usbstructure.h"
#include "usbdesc.h"

/******************************************************************************
 * º¯ÊýÉêÃ÷
 ******************************************************************************/

VOID
	CleanupInfo(PVOID info);

PUSBDEVICEINFO
	GetHubPortDeviceInfo(PTSTR pszRootHubDevPath,
	ULONG ConnectionIndex);

#endif /* ENUMUSB_H_ */
