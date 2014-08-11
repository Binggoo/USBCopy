
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