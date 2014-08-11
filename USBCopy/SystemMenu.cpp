// SystemMenu.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SystemMenu.h"
#include "afxdialogex.h"
#include "GlobalSetting.h"
#include "ImageManager.h"
#include "Utils.h"

// CSystemMenu 对话框

IMPLEMENT_DYNAMIC(CSystemMenu, CDialogEx)

CSystemMenu::CSystemMenu(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSystemMenu::IDD, pParent)
{
	m_pIni = NULL;
}

CSystemMenu::~CSystemMenu()
{
}

void CSystemMenu::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSystemMenu, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_SOFTWARE_UPDATE, &CSystemMenu::OnBnClickedButtonUpdate)
	ON_BN_CLICKED(IDC_BTN_FACTORY_RESTORE, &CSystemMenu::OnBnClickedButtonRestore)
	ON_BN_CLICKED(IDC_BTN_GLOBAL_SETTING, &CSystemMenu::OnBnClickedButtonGlobal)
	ON_BN_CLICKED(IDC_BTN_SYNCHRONIZE_IMAGE, &CSystemMenu::OnBnClickedButtonSynchImage)
	ON_BN_CLICKED(IDC_BTN_IMAGE_MANAGER, &CSystemMenu::OnBnClickedButtonImageManager)
	ON_BN_CLICKED(IDC_BTN_VIEW_LOG, &CSystemMenu::OnBnClickedButtonViewLog)
	ON_BN_CLICKED(IDC_BTN_EXPORT_LOG, &CSystemMenu::OnBnClickedButtonExportLog)
	ON_BN_CLICKED(IDC_BTN_BURN_IN, &CSystemMenu::OnBnClickedButtonBurnIn)
	ON_BN_CLICKED(IDC_BTN_DEBUG, &CSystemMenu::OnBnClickedButtonDebug)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CSystemMenu 消息处理程序


BOOL CSystemMenu::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	m_nTargetNum = m_pIni->GetInt(_T("TargetDrives"),_T("NumOfTargetDrives"),0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSystemMenu::SetConfig( CIni *pIni ,CPortCommand *pCommand)
{
	m_pIni = pIni;
	m_pCommand = pCommand;
}


void CSystemMenu::OnBnClickedButtonUpdate()
{
	// TODO: 在此添加控件通知处理程序代码

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	CString strCmd = _T("Update.exe");
	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		PostQuitMessage(WM_CLOSE);
	}
}


void CSystemMenu::OnBnClickedButtonRestore()
{
	// TODO: 在此添加控件通知处理程序代码
	if (IDYES == MessageBox(_T("Do you want to restore all setting with factory default ?"),_T("Factory Restore")
		,MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
	{
		// 恢复出厂设置。
		CString strRestoreCmd = m_strAppPath + _T("\\restore.cmd");
		CFile file(strRestoreCmd,CFile::modeCreate | CFile::modeWrite);
		file.Write("@echo off\r\n",strlen("@echo off\r\n"));
		file.Write("ping -n 3 127.0.0.1 > nul\r\n",strlen("ping -n 3 127.0.0.1 > nul\r\n"));
		file.Write("copy FactoryDefa.ini USBCopy.ini /y > nul\r\n",strlen("copy FactoryDefa.ini USBCopy.ini /y > nul\r\n"));
		file.Write("start \"\" \"USBCopy.exe\"\r\n",strlen("start \"\" \"USBCopy.exe\"\r\n"));
		file.Write("del restore.cmd\r\n",strlen("del restore.cmd\r\n"));
		file.Flush();
		file.Close();

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si,sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		CString strCmd = _T("restore.cmd");
		if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			PostQuitMessage(WM_CLOSE);
		}
	}
	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonGlobal()
{
	// TODO: 在此添加控件通知处理程序代码
	CGlobalSetting globalSetting;
	globalSetting.SetConfig(m_pIni);
	globalSetting.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonSynchImage()
{
	// TODO: 在此添加控件通知处理程序代码
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	CString strCmd = _T("SynchImage.exe");
	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CDialogEx::OnOK();
	}
}


void CSystemMenu::OnBnClickedButtonImageManager()
{
	// TODO: 在此添加控件通知处理程序代码
	CImageManager imgManager;
	imgManager.SetConfig(m_pIni);
	imgManager.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonViewLog()
{
	// TODO: 在此添加控件通知处理程序代码
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMAXIMIZED;

	CString strCmd = _T("notepad.exe USBCopy.log");

	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CDialogEx::OnOK();
	}
}


void CSystemMenu::OnBnClickedButtonExportLog()
{
	// TODO: 在此添加控件通知处理程序代码
	// 只允许从USB接口导log

	for (UINT i = 0; i <= m_nTargetNum;i++)
	{
		m_pCommand->Power(i,FALSE);
	}

	CString strDrive,strPath;

	while (1)
	{
		DWORD bmLetters = GetLogicalDrives();
		TCHAR letter = _T('A');
		DWORD letterNum = 0;
		for (int i = 0;i < sizeof(DWORD) * 8;i++)
		{
			DWORD mask = 0x01 << i;

			if ((mask & bmLetters) == 0)
			{
				continue;
			}

			letter = _T('A') + i;

			strPath.Format(_T("%c:\\"),letter);

			UINT uType = GetDriveType(strPath);

			if (uType == DRIVE_REMOVABLE)
			{
				letterNum++;
				strDrive = strPath;
			}
			else
			{
				bmLetters &= ~mask;     //clear this bit
				continue;
			}
		}

		if (letterNum == 0)
		{
			if (IDCANCEL == MessageBox(_T("Please insert U disk and click OK!"),_T("Export Log")
				,MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONINFORMATION))
			{
				break;
			}
			
		}
		else if (letterNum > 1)
		{
			if (IDCANCEL == MessageBox(_T("There are more than one U disk, please remove and keep one U disk, than click OK!")
				,_T("Export Log"),MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONINFORMATION))
			{
				break;
			}
		}
		else
		{
			CTime time = CTime::GetCurrentTime();
			CString strDestFile = strDrive + time.Format(_T("USBCopy_%Y%m%d%H%M%S.log"));

			CString strSourceFile = m_strAppPath + _T("\\USBCopy.log");

			BOOL bSuc = CopyFile(strSourceFile,strDestFile,FALSE);

			strSourceFile = m_strAppPath + _T("\\USBCopy.log.bak");

			if (PathFileExists(strSourceFile))
			{
				strDestFile = strDrive + time.Format(_T("USBCopy_%Y%m%d%H%M%S.log.bak"));

				bSuc &= CopyFile(strSourceFile,strDestFile,FALSE);
			}

			if (bSuc)
			{
				MessageBox(_T("Export log success"),_T("Export Log"));
				break;
			}
			else
			{
				if (IDCANCEL == MessageBox(_T("Export log failed, please change an U disk to retry!")
					,_T("Export Log"),MB_OKCANCEL | MB_DEFBUTTON1| MB_ICONERROR))
				{
					break;
				}
			}
		}
	}

	// 上电
	for (UINT i = 0; i <= m_nTargetNum;i++)
	{
		m_pCommand->Power(i,TRUE);
	}

	CDialogEx::OnOK();
	
}


void CSystemMenu::OnBnClickedButtonBurnIn()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CSystemMenu::OnBnClickedButtonDebug()
{
	// TODO: 在此添加控件通知处理程序代码
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	CString strCmd = _T("cmd.exe");
	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CDialogEx::OnOK();
	}

}


void CSystemMenu::OnBnClickedButtonReturn()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


BOOL CSystemMenu::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_SOFTWARE_UPDATE:
		case IDC_BTN_FACTORY_RESTORE:
		case IDC_BTN_GLOBAL_SETTING:
		case IDC_BTN_SYNCHRONIZE_IMAGE:
		case IDC_BTN_IMAGE_MANAGER:
		case IDC_BTN_VIEW_LOG:
		case IDC_BTN_EXPORT_LOG:
		case IDC_BTN_BURN_IN:
		case IDC_BTN_DEBUG:
		case IDOK:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}
