
// USBCopy.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CUSBCopyApp:
// See USBCopy.cpp for the implementation of this class
//

#define CONFIG_NAME     _T("\\USBCopy.ini")
#define MACHINE_INFO    _T("\\MachineInfo.ini")
#define LOG_FILE     _T("\\USBCopy.log")
#define LOG_FILE_BAK     _T("\\USBCopy.log.bak")
#define MASTER_PATH  _T("M:\\")
#define RECODE_FILE  _T("\\record.txt");
#define LOGO_TS         _T("TF/SD DUPLICATOR")
#define LOGO_USB         _T("USB DUPLICATOR")
#define LOGO_NGFF        _T("NGFF DUPLICATOR")
#define LOGO_MTP         _T("MTP DUPLICATOR")


#define WM_UPDATE_STATISTIC (WM_USER + 1)
#define WM_RESET_MACHIEN_PORT (WM_USER + 2)
#define WM_PORT_RESET_POWER  (WM_USER + 3)
#define WM_UPDATE_SOFTWARE   (WM_USER + 4)
#define WM_RESET_POWER  (WM_USER + 5)
#define WM_BURN_IN_TEST (WM_USER + 6)
#define WM_DISCONNECT_SOCKET (WM_USER + 7)
#define WM_CONNECT_SOCKET (WM_USER + 8)
#define WM_SOCKET_MSG   (WM_USER + 10)
#define WM_UPDATE_FUNCTION (WM_USER + 11)
#define WM_WORK_MODE_SELECT (WM_USER + 12)
#define WM_SET_BURN_IN_TEXT (WM_USER + 13)
#define WM_INIT_CURRENT_WORKMODE (WM_USER + 14)
#define WM_EXPORT_LOG_START (WM_USER + 15)
#define WM_EXPORT_LOG_END (WM_USER + 16)
#define WM_LOAD_CONFIG (WM_USER + 17)

class CUSBCopyApp : public CWinApp
{
public:
	CUSBCopyApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CUSBCopyApp theApp;

BOOL Send(SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode,DWORD dwTimeout = 60000);
BOOL Recv(SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode,DWORD dwTimeout = 60000);