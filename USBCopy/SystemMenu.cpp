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
#include "SoftwareRecovery.h"
#include "MoreFunction.h"
#include "ExportLog.h"
#include "MyMessageBox.h"
#include "LoadConfig.h"
#include "SaveConfig.h"

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
	ON_COMMAND(ID_MOREFUNCTION_SOFTWARERECOVERY, &CSystemMenu::OnBnClickedButtonRestore)
	ON_BN_CLICKED(IDC_BTN_GLOBAL_SETTING, &CSystemMenu::OnBnClickedButtonGlobal)
	//ON_BN_CLICKED(IDC_BTN_SYNCHRONIZE_IMAGE, &CSystemMenu::OnBnClickedButtonSynchImage)
	ON_BN_CLICKED(IDC_BTN_IMAGE_MANAGER, &CSystemMenu::OnBnClickedButtonImageManager)
	ON_COMMAND(ID_MOREFUNCTION_VIEWLOG, &CSystemMenu::OnBnClickedButtonViewLog)
	ON_BN_CLICKED(IDC_BTN_EXPORT_LOG, &CSystemMenu::OnBnClickedButtonExportLog)
	ON_COMMAND(ID_MOREFUNCTION_BURNIN, &CSystemMenu::OnBnClickedButtonBurnIn)
	ON_COMMAND(ID_MOREFUNCTION_DEBUG, &CSystemMenu::OnBnClickedButtonDebug)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BTN_MORE, &CSystemMenu::OnBnClickedBtnMore)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BTN_SHUTDOWN, &CSystemMenu::OnBnClickedBtnShutdown)
	ON_BN_CLICKED(IDC_BTN_LOAD_CONFIG, &CSystemMenu::OnBnClickedBtnLoadConfig)
	ON_BN_CLICKED(IDC_BTN_SAVE_CONFIG, &CSystemMenu::OnBnClickedBtnSaveConfig)
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

	m_BtnSetting.SubclassDlgItem(IDC_BTN_GLOBAL_SETTING,this);
	m_BtnSetting.SetFlat(FALSE);
	m_BtnSetting.SetBitmaps(IDB_SETTING,RGB(255,255,255));

// 	m_BtnSyncImage.SubclassDlgItem(IDC_BTN_SYNCHRONIZE_IMAGE,this);
// 	m_BtnSyncImage.SetFlat(FALSE);
// 	m_BtnSyncImage.SetBitmaps(IDB_SYNC,RGB(255,255,255));

	m_BtnImageManager.SubclassDlgItem(IDC_BTN_IMAGE_MANAGER,this);
	m_BtnImageManager.SetFlat(FALSE);
	m_BtnImageManager.SetBitmaps(IDB_MANAGER,RGB(255,255,255));

	m_BtnExport.SubclassDlgItem(IDC_BTN_EXPORT_LOG,this);
	m_BtnExport.SetFlat(FALSE);
	m_BtnExport.SetBitmaps(IDB_EXPORT,RGB(255,255,255));

	m_BtnReturn.SubclassDlgItem(IDOK,this);
	m_BtnReturn.SetBitmaps(IDB_RETURN,RGB(255,255,255));
	//m_BtnReturn.SetFlat(FALSE);
	m_BtnReturn.DrawBorder(FALSE);
	SetDlgItemText(IDOK,_T(""));

	m_BtnMore.SubclassDlgItem(IDC_BTN_MORE,this);
	m_BtnMore.SetFlat(FALSE);
	m_BtnMore.SetBitmaps(IDB_MORE,RGB(255,255,255));

	m_BtnPackage.SubclassDlgItem(IDC_BTN_PKG_MANAGER,this);
	m_BtnPackage.SetFlat(FALSE);
	m_BtnPackage.SetBitmaps(IDB_PACKAGE,RGB(255,255,255));

	m_BtnShutDown.SubclassDlgItem(IDC_BTN_SHUTDOWN,this);
	m_BtnShutDown.SetBitmaps(IDB_SHUTDOWN,RGB(255,255,255));
	//m_BtnShutDown.SetFlat(FALSE);
	m_BtnShutDown.DrawBorder(FALSE);
	SetDlgItemText(IDC_BTN_SHUTDOWN,_T(""));

	m_BtnLoadConfig.SubclassDlgItem(IDC_BTN_LOAD_CONFIG,this);
	m_BtnLoadConfig.SetFlat(FALSE);
	m_BtnLoadConfig.SetBitmaps(IDB_LOAD,RGB(255,255,255));

	m_BtnSaveConfig.SubclassDlgItem(IDC_BTN_SAVE_CONFIG,this);
	m_BtnSaveConfig.SetFlat(FALSE);
	m_BtnSaveConfig.SetBitmaps(IDB_SAVE,RGB(255,255,255));

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CSystemMenu::SetConfig( CIni *pIni ,CPortCommand *pCommand,BOOL bConnected,BOOL bLisence,UINT nMachineType)
{
	m_pIni = pIni;
	m_pCommand = pCommand;
	m_bSocketConnected = bConnected;
	m_bLisence = bLisence;

	m_nMachineType = nMachineType;
}


void CSystemMenu::OnBnClickedButtonUpdate()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	CString strCmd = _T("Update.exe USBCopy.exe");
	if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,m_strAppPath,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		//���ݵ�old�ļ���
		CString strAppName = CUtils::GetAppPath();
		CString strBackupPath = CUtils::GetFilePathWithoutName(strAppName) + _T("\\old");

		if (!PathFileExists(strBackupPath))
		{
			SHCreateDirectoryEx(NULL,strBackupPath,NULL);
		}

		CString strNewName;
		strNewName.Format(_T("%s\\USBCopy_v%s.exe"),strBackupPath,CUtils::GetAppVersion(strAppName));

		CopyFile(strAppName,strNewName,FALSE);

		::SendMessage(GetParent()->GetSafeHwnd(),WM_UPDATE_SOFTWARE,0,0);
		
		PostQuitMessage(WM_CLOSE);
	}
}


void CSystemMenu::OnBnClickedButtonRestore()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	CSoftwareRecovery recovery;
	recovery.DoModal();
	
	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonGlobal()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	CGlobalSetting globalSetting;
	globalSetting.SetConfig(m_pIni,m_bSocketConnected);
	globalSetting.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonSynchImage()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

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

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	CImageManager imgManager;
	imgManager.SetConfig(m_pIni,m_pCommand,m_nMachineType);
	imgManager.DoModal();

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonViewLog()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

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

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	// ֻ�����USB�ӿڵ�log
	CExportLog exportLog;

	if (exportLog.DoModal() == IDCANCEL)
	{
		return;
	}

	CStringArray strFilesArray;
	exportLog.GetFileArray(strFilesArray);

	GetDlgItem(IDC_BTN_EXPORT_LOG)->EnableWindow(FALSE);

	::SendMessage(GetParent()->GetSafeHwnd(),WM_EXPORT_LOG_START,0,0);

	
	// ȫ���ϵ�
	m_pCommand->ResetPower();

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

			strSourceFile = m_strAppPath + _T("\\record.txt");
			int nCount = strFilesArray.GetCount();

			for (int i = 0; i < nCount;i++)
			{
				strSourceFile = m_strAppPath + _T("\\") + strFilesArray.GetAt(i);
				strDestFile = strDrive + strFilesArray.GetAt(i);

				CopyFile(strSourceFile,strDestFile,FALSE);
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

	if (m_nMachineType != MT_NGFF)
	{
		// �ϵ�
		for (UINT i = 0; i <= m_nTargetNum;i++)
		{
			m_pCommand->Power(i,TRUE);
			Sleep(100);
		}
	}
	
	GetDlgItem(IDC_BTN_EXPORT_LOG)->EnableWindow(TRUE);

	::SendMessage(GetParent()->GetSafeHwnd(),WM_EXPORT_LOG_END,0,0);

	CDialogEx::OnOK();
	
}


void CSystemMenu::OnBnClickedButtonBurnIn()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bLisence)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
		return;
	}

	CBurnIn burnIn;
	burnIn.SetConfig(m_pIni);
	burnIn.DoModal();
	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedButtonDebug()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

// 	if (!m_bLisence)
// 	{
// 		CString strResText;
// 		strResText.LoadString(IDS_MSG_LISENCE_FAILED);
// 		MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);
// 		return;
// 	}

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
		case IDC_BTN_GLOBAL_SETTING:
		//case IDC_BTN_SYNCHRONIZE_IMAGE:
		case IDC_BTN_IMAGE_MANAGER:
		case IDC_BTN_EXPORT_LOG:
		case IDOK:
		case IDC_BTN_MORE:
		case IDC_BTN_PKG_MANAGER:
		case IDC_BTN_SHUTDOWN:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CSystemMenu::OnBnClickedBtnMore()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	CRect rectBtn,rectDlg;
	GetDlgItem(IDC_BTN_MORE)->GetWindowRect(rectBtn);

	CMoreFunction *moreFunc = new CMoreFunction(this);
	moreFunc->Create(IDD_DIALOG_MORE_FUNCTION);
	moreFunc->GetWindowRect(&rectDlg);

	moreFunc->MoveWindow(rectBtn.right,rectBtn.top,rectDlg.Width(),rectDlg.Height());
	moreFunc->ShowWindow(SW_SHOW);

}


void CSystemMenu::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: �ڴ˴������Ϣ����������
	CRect rect,rectParent;
	GetWindowRect(&rect);
	GetParent()->GetClientRect(rectParent);
	GetParent()->ClientToScreen(&rectParent);

	MoveWindow(rectParent.left + 100,rect.top,rect.Width(),rect.Height());
}


void CSystemMenu::OnBnClickedBtnShutdown()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CMyMessageBox msg;
	msg.SetTitle(IDS_MSG_WARNING);
	msg.SetText(IDS_MSG_SHUT_DOWN);
	msg.SetFont(160);
	msg.SetTextColor(RGB(255,0,0));

	if (IDOK == msg.DoModal())
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si,sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		CString strCmd = _T("shutdown.exe /s /f /t 0");
		if (CreateProcess(NULL,strCmd.GetBuffer(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			PostQuitMessage(WM_CLOSE);
		}
		
	}
	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedBtnLoadConfig()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strInitialPath = m_strAppPath + _T("\\config");

	if (!PathFileExists(strInitialPath))
	{
		SHCreateDirectory(NULL,strInitialPath);
	}

// 	CFileDialog load(TRUE,_T("ini"),NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("Config(*.ini)|*.ini||"));
// 	load.m_ofn.lpstrInitialDir = strInitialPath;

	CLoadConfig load;
	if (load.DoModal() == IDOK)
	{
		m_pIni->SetPathName(load.GetPathName());
		//PostMessage
		::PostMessage(GetParent()->GetSafeHwnd(),WM_LOAD_CONFIG,0,0);
	}

	CDialogEx::OnOK();
}


void CSystemMenu::OnBnClickedBtnSaveConfig()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strInitialPath = m_strAppPath + _T("\\config");

	if (!PathFileExists(strInitialPath))
	{
		SHCreateDirectory(NULL,strInitialPath);
	}

// 	CFileDialog save(FALSE,_T("ini"),NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("Config(*.ini)|*.ini||"));
// 	save.m_ofn.lpstrInitialDir = strInitialPath;

	CSaveConfig save;
	save.SetSourcePath(m_pIni->GetPathName());
	save.SetDestPath(strInitialPath);
	save.DoModal();

	CDialogEx::OnOK();
}
