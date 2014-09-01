// SystemMenu.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SystemMenu.h"
#include "afxdialogex.h"
#include "GlobalSetting.h"
#include "ImageManager.h"
#include "Utils.h"
#include "BurnIn.h"

// CSystemMenu �Ի���

IMPLEMENT_DYNAMIC(CSystemMenu, CDialogEx)

CSystemMenu::CSystemMenu(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSystemMenu::IDD, pParent)
{
	m_pIni = NULL;
	m_bSocketConnected = FALSE;
	m_pCommand = NULL;
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


// CSystemMenu ��Ϣ�������


BOOL CSystemMenu::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);

	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	m_nTargetNum = m_pIni->GetInt(_T("TargetDrives"),_T("NumOfTargetDrives"),0);

	m_BtnUpdate.SubclassDlgItem(IDC_BTN_SOFTWARE_UPDATE,this);
	m_BtnUpdate.SetFlat(FALSE);
	m_BtnUpdate.SetBitmaps(IDB_UPDATE,RGB(255,255,255));

	m_BtnRestore.SubclassDlgItem(IDC_BTN_FACTORY_RESTORE,this);
	m_BtnRestore.SetFlat(FALSE);
	m_BtnRestore.SetBitmaps(IDB_RESTORE,RGB(255,255,255));

	m_BtnSetting.SubclassDlgItem(IDC_BTN_GLOBAL_SETTING,this);
	m_BtnSetting.SetFlat(FALSE);
	m_BtnSetting.SetBitmaps(IDB_SETTING,RGB(255,255,255));

	m_BtnSyncImage.SubclassDlgItem(IDC_BTN_SYNCHRONIZE_IMAGE,this);
	m_BtnSyncImage.SetFlat(FALSE);
	m_BtnSyncImage.SetBitmaps(IDB_SYNC,RGB(255,255,255));

	m_BtnImageManager.SubclassDlgItem(IDC_BTN_IMAGE_MANAGER,this);
	m_BtnImageManager.SetFlat(FALSE);
	m_BtnImageManager.SetBitmaps(IDB_MANAGER,RGB(255,255,255));

	m_BtnViewLog.SubclassDlgItem(IDC_BTN_VIEW_LOG,this);
	m_BtnViewLog.SetFlat(FALSE);
	m_BtnViewLog.SetBitmaps(IDB_VIEW_LOG,RGB(255,255,255));

	m_BtnExport.SubclassDlgItem(IDC_BTN_EXPORT_LOG,this);
	m_BtnExport.SetFlat(FALSE);
	m_BtnExport.SetBitmaps(IDB_EXPORT,RGB(255,255,255));

	m_BtnDebug.SubclassDlgItem(IDC_BTN_DEBUG,this);
	m_BtnDebug.SetFlat(FALSE);
	m_BtnDebug.SetBitmaps(IDB_DEBUG,RGB(255,255,255));

	m_BtnBurnIn.SubclassDlgItem(IDC_BTN_BURN_IN,this);
	m_BtnBurnIn.SetFlat(FALSE);
	m_BtnBurnIn.SetBitmaps(IDB_BURN_IN,RGB(255,255,255));

	m_BtnReturn.SubclassDlgItem(IDOK,this);
	m_BtnReturn.SetFlat(FALSE);
	m_BtnReturn.SetBitmaps(IDB_RETURN,RGB(255,255,255));

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CSystemMenu::SetConfig( CIni *pIni ,CPortCommand *pCommand,BOOL bConnected)
{
	m_pIni = pIni;
	m_pCommand = pCommand;
	m_bSocketConnected = bConnected;
}


void CSystemMenu::OnBnClickedButtonUpdate()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	CString strCmd = _T("Update.exe USBCopy.exe");
	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		::SendMessage(GetParent()->GetSafeHwnd(),WM_UPDATE_SOFTWARE,0,0);
		
		PostQuitMessage(WM_CLOSE);
	}
}


void CSystemMenu::OnBnClickedButtonRestore()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strMsg,strTile;
	strMsg.LoadString(IDS_MSG_FACTORY_RESTORE);
	strTile.LoadString(IDS_FACTORY_RESTORE);
	if (IDYES == MessageBox(strMsg,strTile
		,MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
	{
		// �ָ��������á�
		CString strRestoreCmd = m_strAppPath + _T("\\restore.cmd");
		CFile file(strRestoreCmd,CFile::modeCreate | CFile::modeWrite);
		file.Write("@echo off\r\n",strlen("@echo off\r\n"));
		file.Write("ping -n 3 127.0.0.1 > nul\r\n",strlen("ping -n 3 127.0.0.1 > nul\r\n"));
		file.Write("copy FactoryDefault.ini USBCopy.ini /y > nul\r\n",strlen("copy FactoryDefault.ini USBCopy.ini /y > nul\r\n"));
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CGlobalSetting globalSetting;
	globalSetting.SetConfig(m_pIni,m_bSocketConnected);
	globalSetting.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonSynchImage()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CImageManager imgManager;
	imgManager.SetConfig(m_pIni);
	imgManager.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonViewLog()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// ֻ�����USB�ӿڵ�log

	for (UINT i = 0; i <= m_nTargetNum;i++)
	{
		m_pCommand->Power(i,FALSE);
	}

	CString strDrive,strPath,strMsg,strTitle;

	strTitle.LoadString(IDS_EXPORT_LOG);

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
			strMsg.LoadString(IDS_MSG_EXPORT_LOG_INSERT_U_DISK);
			if (IDCANCEL == MessageBox(strMsg,strTitle
				,MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONINFORMATION))
			{
				break;
			}
			
		}
		else if (letterNum > 1)
		{
			strMsg.LoadString(IDS_MSG_EXPORT_LOG_MORE_U_DISK);
			if (IDCANCEL == MessageBox(strMsg,strTitle
				,MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONINFORMATION))
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
				strMsg.LoadString(IDS_MSG_EXPORT_LOG_SUCCESS);
				MessageBox(strMsg,strTitle);
				break;
			}
			else
			{
				strMsg.LoadString(IDS_MSG_EXPORT_LOG_FAILED);
				if (IDCANCEL == MessageBox(strMsg,strTitle
					,MB_OKCANCEL | MB_DEFBUTTON1| MB_ICONERROR))
				{
					break;
				}
			}
		}
	}

	// �ϵ�
	for (UINT i = 0; i <= m_nTargetNum;i++)
	{
		m_pCommand->Power(i,TRUE);
	}

	CDialogEx::OnOK();
	
}


void CSystemMenu::OnBnClickedButtonBurnIn()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CBurnIn burnIn;
	burnIn.SetConfig(m_pIni);
	burnIn.DoModal();
	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonDebug()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnOK();
}


BOOL CSystemMenu::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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
