// SoftwareRecovery.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SoftwareRecovery.h"
#include "afxdialogex.h"
#include "Utils.h"


// CSoftwareRecovery 对话框

IMPLEMENT_DYNAMIC(CSoftwareRecovery, CDialogEx)

CSoftwareRecovery::CSoftwareRecovery(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSoftwareRecovery::IDD, pParent)
{

}

CSoftwareRecovery::~CSoftwareRecovery()
{
}

void CSoftwareRecovery::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SOFTWARE, m_SoftwareList);
}


BEGIN_MESSAGE_MAP(CSoftwareRecovery, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSoftwareRecovery::OnBnClickedOk)
END_MESSAGE_MAP()


// CSoftwareRecovery 消息处理程序


BOOL CSoftwareRecovery::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString strAppName = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strAppName);
	
	InitialListCtrl();
	AddListData();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSoftwareRecovery::InitialListCtrl()
{
	// 初始化ListCtrl
	CRect rect;
	m_SoftwareList.GetClientRect(&rect);
	int nWidth = rect.Width();

	CString strResText;

	int nItem = 0;

	strResText.LoadString(IDS_ITEM_VERSION);
	m_SoftwareList.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth/3,0);

	strResText.LoadString(IDS_ITEM_MODIFY_TIME);
	m_SoftwareList.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth/3,0);

	strResText.LoadString(IDS_ITEM_SIZE);
	m_SoftwareList.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth/3,0);

	m_SoftwareList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CSoftwareRecovery::AddListData()
{
	//备份到old文件夹
	CString strBackupPath = m_strAppPath + _T("\\old");

	if (!PathFileExists(strBackupPath))
	{
		SHCreateDirectoryEx(NULL,strBackupPath,NULL);
	}

	CString strFindFile = strBackupPath + _T("\\USBCopy*.exe");

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	int nItem = 0;
	CString strVersion,strModifyTime,strSize, strFileName;
	while (bFind)
	{
		bFind = ff.FindNextFile();

		CFileStatus status;

		CFile::GetStatus(ff.GetFilePath(),status);

		strVersion.Format(_T("v%s"),CUtils::GetAppVersion(ff.GetFilePath()));
		strModifyTime = status.m_mtime.Format("%Y-%m-%d %H:%M");
		strSize = CUtils::AdjustFileSize(status.m_size);
		strFileName = ff.GetFileName();

		TCHAR *szFileName = new TCHAR[ff.GetFileName().GetLength() + 1];

		_tcscpy_s(szFileName,strFileName.GetLength() + 1,strFileName);

		m_SoftwareList.InsertItem(nItem,strVersion);
		m_SoftwareList.SetItemText(nItem,1,strModifyTime);
		m_SoftwareList.SetItemText(nItem,2,strSize);
		m_SoftwareList.SetItemData(nItem,(DWORD_PTR)szFileName);
		
		nItem++;
	}
	ff.Close();

}


BOOL CSoftwareRecovery::DestroyWindow()
{
	// TODO: 在此添加专用代码和/或调用基类

	int nCount = m_SoftwareList.GetItemCount();

	for (int i = 0;i < nCount;i++)
	{
		TCHAR *sFileName = (TCHAR *)m_SoftwareList.GetItemData(i);

		delete []sFileName;
	}

	return CDialogEx::DestroyWindow();
}


void CSoftwareRecovery::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	POSITION pos = m_SoftwareList.GetFirstSelectedItemPosition();
	BOOL bDefaultSetting = ((CButton *)GetDlgItem(IDC_CHECK_DEFAULT_SETTING))->GetCheck();

	CString strMsg,strTile,strFileName;
	strTile.LoadString(IDS_FACTORY_RESTORE);

	if (pos == NULL && !bDefaultSetting)
	{
		return;
	}
	else if (pos && !bDefaultSetting)
	{
		int nSelectItem = m_SoftwareList.GetNextSelectedItem(pos);
		strFileName = CString((TCHAR *)m_SoftwareList.GetItemData(nSelectItem));

		strMsg.LoadString(IDS_MSG_RESTORE_SOFTWARE);
	}
	else if (pos == NULL && bDefaultSetting)
	{
		strMsg.LoadString(IDS_MSG_FACTORY_RESTORE);
	}
	else
	{
		strMsg.LoadString(IDS_MSG_RESTORE_SOFT_SETTING);
	}


	if (IDYES == MessageBox(strMsg,strTile
		,MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
	{
		// 恢复出厂设置。
		CString strCopy;
		strCopy.Format(_T("copy USBCopy.exe old\\USBCopy.exe /y > nul\r\ncopy old\\%s USBCopy.exe /y > nul\r\n"),strFileName);
		USES_CONVERSION;
		char *copy = W2A(strCopy);
		
		CString strRestoreCmd = m_strAppPath + _T("\\restore.cmd");
		CFile file(strRestoreCmd,CFile::modeCreate | CFile::modeWrite);
		file.Write("@echo off\r\n",strlen("@echo off\r\n"));
		file.Write("ping -n 3 127.0.0.1 > nul\r\n",strlen("ping -n 3 127.0.0.1 > nul\r\n"));

		if (bDefaultSetting)
		{
			file.Write("copy FactoryDefault.ini USBCopy.ini /y > nul\r\n",strlen("copy FactoryDefault.ini USBCopy.ini /y > nul\r\n"));
		}

		if (pos)
		{
			file.Write(copy,strlen(copy));
		}

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
