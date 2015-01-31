// FileCopySetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FileCopySetting.h"
#include "Utils.h"
#include "afxdialogex.h"
#include "XFolderDialog/XFolderDialog.h"


// CFileCopySetting �Ի���

IMPLEMENT_DYNAMIC(CFileCopySetting, CDialogEx)

CFileCopySetting::CFileCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileCopySetting::IDD, pParent)
	, m_bCheckComputeHash(FALSE)
	, m_bCheckCompare(FALSE)
	, m_nCompareMethodIndex(0)
	, m_bCheckCopyAllFiles(FALSE)
	, m_bCleanupTarget(FALSE)
{
	m_pIni = NULL;
	m_strMasterPath = _T("M:\\");
}

CFileCopySetting::~CFileCopySetting()
{
}

void CFileCopySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_COPY, m_ListCtrl);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bCheckComputeHash);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCheckCompare);
	DDX_Radio(pDX,IDC_RADIO_HASH_COMPARE,m_nCompareMethodIndex);
	DDX_Check(pDX, IDC_CHECK_ALL_FILES, m_bCheckCopyAllFiles);
	DDX_Check(pDX, IDC_CHECK_CLEAR_UP_TARGETS, m_bCleanupTarget);
}


BEGIN_MESSAGE_MAP(CFileCopySetting, CDialogEx)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BTN_ADD_FOLDER, &CFileCopySetting::OnBnClickedBtnAddFolder)
	ON_BN_CLICKED(IDC_BTN_ADD_FILE, &CFileCopySetting::OnBnClickedBtnAddFile)
	ON_BN_CLICKED(IDC_BTN_DEL, &CFileCopySetting::OnBnClickedBtnDel)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CFileCopySetting::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CFileCopySetting::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDOK, &CFileCopySetting::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_HASH_COMPARE, &CFileCopySetting::OnBnClickedRadioHashCompare)
	ON_BN_CLICKED(IDC_CHECK_ALL_FILES, &CFileCopySetting::OnBnClickedCheckAllFiles)
END_MESSAGE_MAP()


// CFileCopySetting ��Ϣ�������


BOOL CFileCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni && m_MasterPort);

	m_bCheckComputeHash = m_pIni->GetBool(_T("FileCopy"),_T("En_ComputeHash"),FALSE);
	m_bCheckCompare = m_pIni->GetBool(_T("FileCopy"),_T("En_Compare"),FALSE);
	m_nCompareMethodIndex = m_pIni->GetInt(_T("FileCopy"),_T("CompareMethod"),0);
	m_bCheckCopyAllFiles = m_pIni->GetBool(_T("FileCopy"),_T("En_CopyAllFiles"),TRUE);
	m_bCleanupTarget = m_pIni->GetBool(_T("FileCopy"),_T("En_CleanupTargets"),FALSE);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCheckCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCheckCompare);

	if (!m_bCheckComputeHash)
	{
		if (m_bCheckCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}

	if (m_bCheckCopyAllFiles)
	{
		m_ListCtrl.EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DEL)->EnableWindow(FALSE);
	}

	InitialListCtrl();

	AfxBeginThread((AFX_THREADPROC)UpdateListCtrlThreadProc,this);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CFileCopySetting::SetConfig( CIni *pIni,CPort *port)
{
	m_pIni = pIni;
	m_MasterPort = port;
}

void CFileCopySetting::InitialListCtrl()
{
	CRect rect;
	m_ListCtrl.GetClientRect(&rect);

	int nWidth = rect.Width() - 220;

	CString strResText;

	int nItem = 0;

	strResText.LoadString(IDS_ITEM_PATH);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth,0);

	strResText.LoadString(IDS_ITEM_TYPE);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,60,0);

	strResText.LoadString(IDS_ITEM_SIZE);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,100,0);

	strResText.LoadString(IDS_ITEM_STATE);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,60,0);

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CFileCopySetting::UpdateListCtrl()
{
	GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_DEL)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	UINT nNumOfFolders = m_pIni->GetUInt(_T("FileCopy"),_T("NumOfFolders"),0);
	UINT nNumOfFiles = m_pIni->GetUInt(_T("FileCopy"),_T("NumOfFiles"),0);

	int nItem = 0;

	CString strKey,strFile,strPath;

	for (UINT i = 0;i < nNumOfFolders;i++)
	{
		strKey.Format(_T("Folder_%d"),i);
		strFile = m_pIni->GetString(_T("FileCopy"),strKey);

		if (strFile.IsEmpty())
		{
			continue;
		}

		m_ListCtrl.InsertItem(nItem,strFile);
		m_ListCtrl.SetItemText(nItem,1,_T("Folder"));

		strPath = m_strMasterPath + strFile;

		if (PathFileExists(strPath))
		{
			ULONGLONG ullSize = CUtils::GetFolderSize(strPath);
			m_ListCtrl.SetItemText(nItem,2,CUtils::AdjustFileSize(ullSize));
			m_ListCtrl.SetItemText(nItem,3,_T("YES"));
		}
		else
		{
			m_ListCtrl.SetItemText(nItem,2,_T("0"));
			m_ListCtrl.SetItemText(nItem,3,_T("NO"));
		}

		nItem++;
	}

	for (UINT i = 0;i < nNumOfFiles;i++)
	{
		strKey.Format(_T("File_%d"),i);
		strFile = m_pIni->GetString(_T("FileCopy"),strKey);

		if (strFile.IsEmpty())
		{
			continue;
		}

		m_ListCtrl.InsertItem(nItem,strFile);
		m_ListCtrl.SetItemText(nItem,1,_T("File"));

		strPath = m_strMasterPath + strFile;

		if (PathFileExists(strPath))
		{
			CFileStatus status;
			CFile::GetStatus(strPath,status);
			m_ListCtrl.SetItemText(nItem,2,CUtils::AdjustFileSize(status.m_size));
			m_ListCtrl.SetItemText(nItem,3,_T("YES"));
		}
		else
		{
			m_ListCtrl.SetItemText(nItem,2,_T("0"));
			m_ListCtrl.SetItemText(nItem,3,_T("NO"));
		}

		nItem++;
	}

	if (!m_bCheckCopyAllFiles)
	{
		GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_DEL)->EnableWindow(TRUE);
	}
	
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

DWORD WINAPI CFileCopySetting::UpdateListCtrlThreadProc( LPVOID lpParm )
{
	CFileCopySetting *pDlg = (CFileCopySetting *)lpParm;

	pDlg->UpdateListCtrl();

	return 1;
}


BOOL CFileCopySetting::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_ADD_FOLDER:
		case IDC_BTN_ADD_FILE:
		case IDC_BTN_DEL:
		case IDOK:
		case IDCANCEL:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CFileCopySetting::OnBnClickedBtnAddFolder()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_DEL)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

    CXFolderDialog dlg(m_strMasterPath);

	if (dlg.DoModal() == IDOK)
	{
		CString strFolder = dlg.GetPath();
		if (strFolder.Find(m_strMasterPath) != 0)
		{
			CString strResText;
			strResText.LoadString(IDS_MSG_SELECT_FOLDER_IN_MASTER);
			MessageBox(strResText);
		}
		else
		{
			CString strItem = strFolder.Right(strFolder.GetLength() - 3);

			if (IsAdded(strItem))
			{
				GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(TRUE);
				GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(TRUE);
				GetDlgItem(IDC_BTN_DEL)->EnableWindow(TRUE);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
				return;
			}

			ULONGLONG ullSize = CUtils::GetFolderSize(strFolder);
			int nCount = m_ListCtrl.GetItemCount();
			m_ListCtrl.InsertItem(nCount,strItem);
			m_ListCtrl.SetItemText(nCount,1,_T("Folder"));
			m_ListCtrl.SetItemText(nCount,2,CUtils::AdjustFileSize(ullSize));
			m_ListCtrl.SetItemText(nCount,3,_T("YES"));
		}
	}
	

	GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_DEL)->EnableWindow(TRUE);
	GetDlgItem(IDOK)->EnableWindow(TRUE);

	SetForegroundWindow();
}


void CFileCopySetting::OnBnClickedBtnAddFile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	int nCount = m_ListCtrl.GetItemCount();

	CString strFilter = _T("All Files(*.*)|*.*||");
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|   OFN_ALLOWMULTISELECT,strFilter,NULL); 
	dlg.m_ofn.lpstrInitialDir = m_strMasterPath;

	if (dlg.DoModal() == IDOK)
	{
		POSITION pos = dlg.GetStartPosition(); 
		while(pos) 
		{ 
			CString filename = dlg.GetNextPathName(pos);

			if (filename.Find(m_strMasterPath) != 0)
			{
				CString strResText;
				strResText.LoadString(IDS_MSG_SELECT_FILE_IN_MASTER);
				MessageBox(strResText);

				return;
			}

			CString strItem = filename.Right(filename.GetLength() - 3);

			if (IsAdded(strItem))
			{
				continue;
			}

			CFileStatus status;
			CFile::GetStatus(filename,status);

			m_ListCtrl.InsertItem(nCount,strItem);
			m_ListCtrl.SetItemText(nCount,1,_T("File"));
			m_ListCtrl.SetItemText(nCount,2,CUtils::AdjustFileSize(status.m_size));
			m_ListCtrl.SetItemText(nCount,3,_T("YES"));
			nCount++;
		} 
	}

	
}


void CFileCopySetting::OnBnClickedBtnDel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);
		m_ListCtrl.DeleteItem(nItem);
		pos = m_ListCtrl.GetFirstSelectedItemPosition();
	}
}


void CFileCopySetting::OnBnClickedCheckComputeHash()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	if (!m_bCheckComputeHash)
	{
		if (m_bCheckCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}
	UpdateData(FALSE);
}


void CFileCopySetting::OnBnClickedCheckCompare()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCheckCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCheckCompare);
}


void CFileCopySetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	int nCount = m_ListCtrl.GetItemCount();

	m_pIni->WriteBool(_T("FileCopy"),_T("En_ComputeHash"),m_bCheckComputeHash);
	m_pIni->WriteBool(_T("FileCopy"),_T("En_Compare"),m_bCheckCompare);
	m_pIni->WriteInt(_T("FileCopy"),_T("CompareMethod"),m_nCompareMethodIndex);
	m_pIni->WriteBool(_T("FileCopy"),_T("En_CopyAllFiles"),m_bCheckCopyAllFiles);
	m_pIni->WriteBool(_T("FileCopy"),_T("En_CleanupTargets"),m_bCleanupTarget);

	UINT nNumOfFolders = m_pIni->GetUInt(_T("FileCopy"),_T("NumOfFolders"),0);
	UINT nNumOfFiles = m_pIni->GetUInt(_T("FileCopy"),_T("NumOfFiles"),0);

	CString strType,strKey,strState,strPath;
	// ��ɾ��
	for (UINT i = 0; i < nNumOfFiles;i++)
	{
		strKey.Format(_T("File_%d"),i);
		m_pIni->DeleteKey(_T("FileCopy"),strKey);
	}

	for (UINT i = 0; i < nNumOfFolders;i++)
	{
		strKey.Format(_T("Folder_%d"),i);
		m_pIni->DeleteKey(_T("FileCopy"),strKey);
	}

	nNumOfFolders = 0;
	nNumOfFiles = 0;

	for (int i = 0; i < nCount;i++)
	{
		strState = m_ListCtrl.GetItemText(i,3);

		if (strState.CompareNoCase(_T("NO")) == 0)
		{
			continue;
		}

		strType = m_ListCtrl.GetItemText(i,1);

		if (strType.CompareNoCase(_T("File")) == 0)
		{
			strKey.Format(_T("File_%d"),nNumOfFiles);
			nNumOfFiles++;
		}
		else
		{
			strKey.Format(_T("Folder_%d"),nNumOfFolders);
			nNumOfFolders++;
		}

		strPath = m_ListCtrl.GetItemText(i,0);

		m_pIni->WriteString(_T("FileCopy"),strKey,strPath);

	}

	m_pIni->WriteUInt(_T("FileCopy"),_T("NumOfFolders"),nNumOfFolders);
	m_pIni->WriteUInt(_T("FileCopy"),_T("NumOfFiles"),nNumOfFiles);

	CDialogEx::OnOK();
}

int CALLBACK CFileCopySetting::BrowseCallbackProc( HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData )
{
	if(uMsg == BFFM_INITIALIZED)
	{
		::SendMessage(hwnd, BFFM_SETSELECTION,TRUE,lpData);
	}
	return 0;
}

BOOL CFileCopySetting::IsAdded( CString strItem )
{
	BOOL bRet = FALSE;
	int nCount = m_ListCtrl.GetItemCount();

	for (int i = 0; i < nCount;i++)
	{
		CString strPath = m_ListCtrl.GetItemText(i,0);

		if (strPath.CompareNoCase(strItem) == 0)
		{
			bRet = TRUE;
			break;
		}
	}

	return bRet;
}


void CFileCopySetting::OnBnClickedRadioHashCompare()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_bCheckComputeHash = TRUE;

	UpdateData(FALSE);
}


void CFileCopySetting::OnBnClickedCheckAllFiles()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_ListCtrl.EnableWindow(!m_bCheckCopyAllFiles);
	GetDlgItem(IDC_BTN_ADD_FILE)->EnableWindow(!m_bCheckCopyAllFiles);
	GetDlgItem(IDC_BTN_ADD_FOLDER)->EnableWindow(!m_bCheckCopyAllFiles);
	GetDlgItem(IDC_BTN_DEL)->EnableWindow(!m_bCheckCopyAllFiles);
}
