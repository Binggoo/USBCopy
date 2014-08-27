
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
#define WM_UPDATE_STATISTIC (WM_USER + 1)
#define WM_RESET_MACHIEN_PORT (WM_USER + 2)
#define WM_PORT_RESET_POWER  (WM_USER + 3)
#define WM_UPDATE_SOFTWARE   (WM_USER + 4)
#define WM_RESET_POWER  (WM_USER + 5)
#define WM_BURN_IN_TEST (WM_USER + 6)
#define WM_DISCONNECT_SOCKET (WM_USER + 7)
#define WM_CONNECT_SOCKET (WM_USER + 8)
#define WM_VERIFY_START (WM_USER + 9)
#define WM_SOCKET_MSG   (WM_USER + 10)

class CUSBCopyApp : public CWinApp
{
public:
	CUSBCopyApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CUSBCopyApp theApp;

BOOL Send(SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode,DWORD dwTimeout = 60000);
BOOL Recv(SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode,DWORD dwTimeout = 60000);