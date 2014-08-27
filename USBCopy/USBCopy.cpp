
// USBCopy.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "USBCopy.h"
#include "USBCopyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CUSBCopyApp

BEGIN_MESSAGE_MAP(CUSBCopyApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CUSBCopyApp construction

CUSBCopyApp::CUSBCopyApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CUSBCopyApp object

CUSBCopyApp theApp;


// CUSBCopyApp initialization

BOOL CUSBCopyApp::InitInstance()
{
	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CUSBCopyDlg dlg;
	m_pMainWnd = &dlg;

	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL Send( SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode,DWORD dwTimeout/* = 60000*/ )
{
	WSAOVERLAPPED ol = {0};

	if (lpOverlapped)
	{
		ol = *lpOverlapped;
	}
	else
	{
		ol.hEvent = WSACreateEvent();
	}

	WSABUF wsaBuf;
	wsaBuf.buf = buf;
	wsaBuf.len = dwLen;

	DWORD dwRet = 0,dwFlags = 0;
	BOOL bResult = TRUE;

	dwRet = WSASend(socket,&wsaBuf,1,&dwLen,dwFlags,&ol,NULL);
	if (dwRet == SOCKET_ERROR)
	{
		*pdwErrorCode = WSAGetLastError();

		if (*pdwErrorCode != ERROR_IO_PENDING)
		{
			bResult = FALSE;
			goto END;
		}
	}

	dwRet = WSAWaitForMultipleEvents(1,&ol.hEvent,FALSE,dwTimeout,FALSE);

	if (dwRet == WSA_WAIT_FAILED || dwRet == WSA_WAIT_TIMEOUT)
	{
		*pdwErrorCode = WSAGetLastError();
		bResult = FALSE;
		goto END;
	}

	WSAResetEvent(ol.hEvent);

	if (!WSAGetOverlappedResult(socket,&ol,&dwLen,FALSE,&dwFlags))
	{
		*pdwErrorCode = WSAGetLastError();
		bResult = FALSE;
		goto END;
	}

	*pdwErrorCode = 0;
	

END:
	if (lpOverlapped == NULL)
	{
		WSACloseEvent(ol.hEvent);
	}

	return bResult;
}

BOOL Recv( SOCKET socket,char *buf,DWORD &dwLen,LPWSAOVERLAPPED lpOverlapped,PDWORD pdwErrorCode, DWORD dwTimeout/* = 60000*/)
{
	WSAOVERLAPPED ol = {0};

	if (lpOverlapped)
	{
		ol = *lpOverlapped;
	}
	else
	{
		ol.hEvent = WSACreateEvent();
	}

	WSABUF wsaBuf;
	wsaBuf.buf = buf;
	wsaBuf.len = dwLen;

	DWORD dwRet = 0,dwFlags = 0;
	BOOL bResult = TRUE;

	dwRet = WSARecv(socket,&wsaBuf,1,&dwLen,&dwFlags,&ol,NULL);
	if (dwRet == SOCKET_ERROR)
	{
		*pdwErrorCode = WSAGetLastError();

		if (*pdwErrorCode != ERROR_IO_PENDING)
		{
			bResult = FALSE;
			goto END;
		}
	}

	dwRet = WSAWaitForMultipleEvents(1,&ol.hEvent,FALSE,dwTimeout,FALSE);

	if (dwRet == WSA_WAIT_FAILED || dwRet == WSA_WAIT_TIMEOUT)
	{
		*pdwErrorCode = WSAGetLastError();
		bResult = FALSE;
		goto END;
	}

	WSAResetEvent(ol.hEvent);

	if (!WSAGetOverlappedResult(socket,&ol,&dwLen,FALSE,&dwFlags))
	{
		*pdwErrorCode = WSAGetLastError();

		bResult = FALSE;
		goto END;
	}

	*pdwErrorCode = 0;

END:
	if (lpOverlapped == NULL)
	{
		WSACloseEvent(ol.hEvent);
	}

	return bResult;
}
