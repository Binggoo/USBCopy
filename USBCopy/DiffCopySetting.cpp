// DiffCopySetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "DiffCopySetting.h"
#include "afxdialogex.h"


// CDiffCopySetting 对话框

IMPLEMENT_DYNAMIC(CDiffCopySetting, CDialogEx)

CDiffCopySetting::CDiffCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDiffCopySetting::IDD, pParent)
	, m_nRadioSourceType(0)
	, m_nRadioCompareRule(0)
	, m_nRadioLocation(0)
	, m_bCheckComputeHash(FALSE)
	, m_bCheckCompare(FALSE)
	, m_bCheckUpload(FALSE)
	, m_bCheckIncludeSub(FALSE)
{
	m_MasterPort = NULL;
	m_pIni = NULL;
	m_strMasterPath = _T("M:\\");
}

CDiffCopySetting::~CDiffCopySetting()
{
}

void CDiffCopySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_PKG, m_nRadioSourceType);
	DDX_Radio(pDX, IDC_RADIO_FILE_SIZE, m_nRadioCompareRule);
	DDX_Radio(pDX, IDC_RADIO_LOCAL, m_nRadioLocation);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bCheckComputeHash);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCheckCompare);
	DDX_Check(pDX, IDC_CHECK_USER_LOG, m_bCheckUpload);
	DDX_Check(pDX,IDC_CHECK_INCLUDE_SUB,m_bCheckIncludeSub);
	DDX_Control(pDX, IDC_LIST_LOG_LOCATION, m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CDiffCopySetting, CDialogEx)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CDiffCopySetting::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CDiffCopySetting::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDC_BTN_BROWSE, &CDiffCopySetting::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_ADD, &CDiffCopySetting::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CDiffCopySetting::OnBnClickedBtnRemove)
	ON_BN_CLICKED(IDOK, &CDiffCopySetting::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_USER_LOG, &CDiffCopySetting::OnBnClickedCheckUserLog)
	ON_BN_CLICKED(IDC_RADIO_PKG, &CDiffCopySetting::OnBnClickedRadioPkg)
	ON_BN_CLICKED(IDC_RADIO_MASTER, &CDiffCopySetting::OnBnClickedRadioMaster)
END_MESSAGE_MAP()


// CDiffCopySetting 消息处理程序


BOOL CDiffCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni && m_MasterPort);

	// 0 - Package，1 - Master
	m_nRadioSourceType = m_pIni->GetInt(_T("DifferenceCopy"),_T("SourceType"),0);
	// 0 - FileSize,1 - HashValue
	m_nRadioCompareRule = m_pIni->GetInt(_T("DifferenceCopy"),_T("CompareRule"),0);
	// 0 - Local,1 - Remote
	m_nRadioLocation = m_pIni->GetInt(_T("DifferenceCopy"),_T("PkgLocation"),0);
	m_bCheckComputeHash = m_pIni->GetBool(_T("DifferenceCopy"),_T("En_ComputeHash"),FALSE);
	m_bCheckCompare = m_pIni->GetBool(_T("DifferenceCopy"),_T("En_Compare"),FALSE);
	m_bCheckUpload = m_pIni->GetBool(_T("DifferenceCopy"),_T("En_UserLog"),FALSE);
	m_bCheckIncludeSub = m_pIni->GetBool(_T("DifferenceCopy"),_T("En_IncludeSub"),FALSE);

	if (!m_bCheckComputeHash)
	{
		m_bCheckCompare = FALSE;
	}

	if (m_nRadioSourceType == 0) // Package
	{
		GetDlgItem(IDC_RADIO_LOCAL)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO_REMOTE)->EnableWindow(TRUE);

		GetDlgItem(IDC_RADIO_FILE_SIZE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_HASH_VALUE)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_RADIO_LOCAL)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_REMOTE)->EnableWindow(FALSE);

		GetDlgItem(IDC_RADIO_FILE_SIZE)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO_HASH_VALUE)->EnableWindow(TRUE);
		
	}

	GetDlgItem(IDC_EDIT_LOG_PATH)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_EDIT_LOG_EXT)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_BROWSE)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_CHECK_INCLUDE_SUB)->EnableWindow(m_bCheckUpload);
	m_ListCtrl.EnableWindow(m_bCheckUpload);

	InitialListCtrl();
	UpdateListCtrl();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


BOOL CDiffCopySetting::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_ADD:
		case IDC_BTN_BROWSE:
		case IDC_BTN_REMOVE:
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

void CDiffCopySetting::SetConfig( CIni *pIni,CPort *port )
{
	m_pIni = pIni;
	m_MasterPort = port;
}

void CDiffCopySetting::InitialListCtrl()
{
	CRect rect;
	m_ListCtrl.GetClientRect(&rect);

	int nWidth = rect.Width() / 2;

	CString strResText;

	int nItem = 0;

	strResText.LoadString(IDS_ITEM_PATH);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth,0);

	strResText.LoadString(IDS_ITEM_TYPE);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth,0);

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CDiffCopySetting::UpdateListCtrl()
{
	UINT nNumOfLogPath = m_pIni->GetUInt(_T("DifferenceCopy"),_T("NumOfLogPath"),0);

	CString strKey,strValue,strPath,strExt;

	for (UINT i = 0;i < nNumOfLogPath;i++)
	{
		strKey.Format(_T("LogPath_%d"),i);
		strValue = m_pIni->GetString(_T("DifferenceCopy"),strKey);

		// Path:Ext
		int nPos = strValue.ReverseFind(_T(':'));
		strPath = strValue.Left(nPos);
		strExt = strValue.Right(strValue.GetLength() - nPos - 1);

		strPath.Trim();
		strExt.Trim();

		m_ListCtrl.InsertItem(i,strPath);
		m_ListCtrl.SetItemText(i,1,strExt);
	}
}


void CDiffCopySetting::OnBnClickedCheckComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bCheckComputeHash)
	{
		m_bCheckCompare = FALSE;
		UpdateData(FALSE);
	}
}


void CDiffCopySetting::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckCompare)
	{
		m_bCheckComputeHash = TRUE;
		UpdateData(FALSE);
	}
}


void CDiffCopySetting::OnBnClickedBtnBrowse()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strResText;
	if (!m_MasterPort->IsConnected())
	{
		strResText.LoadString(IDS_MSG_NO_MASTER);
		MessageBox(strResText);

		return;
	}
	else if(!PathFileExists(m_strMasterPath))
	{
		strResText.LoadString(IDS_MSG_MASTER_UNRECOGNIZED);
		MessageBox(strResText);

		return;
	}

	CString strFilter = _T("All Files(*.*)|*.*||");
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,strFilter,NULL); 
	dlg.m_ofn.lpstrInitialDir = m_strMasterPath;

	if (dlg.DoModal() == IDOK)
	{
		CString strFileName = dlg.GetPathName();

		if (strFileName.Find(m_strMasterPath) != 0)
		{
			strResText.LoadString(IDS_MSG_SELECT_FILE_IN_MASTER);
			MessageBox(strResText);

			return;
		}

		// 只保留中间路径
		CString strPath,strExt;
		int nPos1 = strFileName.Find(_T('\\'));
		int nPos2 = strFileName.ReverseFind(_T('\\'));

		strPath = strFileName.Mid(nPos1,nPos2 - nPos1);

		if (strPath.IsEmpty())
		{
			strPath = _T("\\");
		}

		strExt.Format(_T("*.%s"),dlg.GetFileExt());

		SetDlgItemText(IDC_EDIT_LOG_PATH,strPath);
		SetDlgItemText(IDC_EDIT_LOG_EXT,strExt);
	}

}


void CDiffCopySetting::OnBnClickedBtnAdd()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData(TRUE);

	CString strPath,strExt;
	GetDlgItemText(IDC_EDIT_LOG_PATH,strPath);
	GetDlgItemText(IDC_EDIT_LOG_EXT,strExt);

	if (strPath.Left(1) != _T("\\"))
	{
		strPath = _T("\\") + strPath;
	}

	if (IsAdded(strPath,strExt))
	{
		return;
	}

	int nCount = m_ListCtrl.GetItemCount();
	m_ListCtrl.InsertItem(nCount,strPath);
	m_ListCtrl.SetItemText(nCount,1,strExt);
}


void CDiffCopySetting::OnBnClickedBtnRemove()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);
		m_ListCtrl.DeleteItem(nItem);
		pos = m_ListCtrl.GetFirstSelectedItemPosition();
	}
}


void CDiffCopySetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	int nCount = m_ListCtrl.GetItemCount();

	m_pIni->WriteInt(_T("DifferenceCopy"),_T("SourceType"),m_nRadioSourceType);
	m_pIni->WriteInt(_T("DifferenceCopy"),_T("CompareRule"),m_nRadioCompareRule);
	m_pIni->WriteInt(_T("DifferenceCopy"),_T("PkgLocation"),m_nRadioLocation);
	m_pIni->WriteBool(_T("DifferenceCopy"),_T("En_ComputeHash"),m_bCheckComputeHash);
	m_pIni->WriteBool(_T("DifferenceCopy"),_T("En_Compare"),m_bCheckCompare);
	m_pIni->WriteBool(_T("DifferenceCopy"),_T("En_UserLog"),m_bCheckUpload);
	m_pIni->WriteBool(_T("DifferenceCopy"),_T("En_IncludeSub"),m_bCheckIncludeSub);

	UINT nNumOfLogPath = m_pIni->GetUInt(_T("DifferenceCopy"),_T("NumOfLogPath"),0);

	CString strValue,strKey;
	// 先删除
	for (UINT i = 0; i < nNumOfLogPath;i++)
	{
		strKey.Format(_T("LogPath_%d"),i);
		m_pIni->DeleteKey(_T("DifferenceCopy"),strKey);
	}

	nNumOfLogPath = nCount;

	m_pIni->WriteUInt(_T("DifferenceCopy"),_T("NumOfLogPath"),nNumOfLogPath);

	for (int i = 0; i < nCount;i++)
	{
		strKey.Format(_T("LogPath_%d"),i);
		strValue.Format(_T("%s:%s"),m_ListCtrl.GetItemText(i,0),m_ListCtrl.GetItemText(i,1));

		m_pIni->WriteString(_T("DifferenceCopy"),strKey,strValue);
	}

	
	CDialogEx::OnOK();
}

BOOL CDiffCopySetting::IsAdded( CString strPath,CString strExt )
{
	int nCount = m_ListCtrl.GetItemCount();

	BOOL bAdded = FALSE;

	for (int i = 0; i < nCount; i++)
	{
		if (m_ListCtrl.GetItemText(i,0).CompareNoCase(strPath) == 0
			&& m_ListCtrl.GetItemText(i,1).CompareNoCase(strExt) == 0)
		{
			bAdded = TRUE;
			break;
		}
	}

	return bAdded;
}


void CDiffCopySetting::OnBnClickedCheckUserLog()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	GetDlgItem(IDC_EDIT_LOG_PATH)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_EDIT_LOG_EXT)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_BROWSE)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(m_bCheckUpload);
	GetDlgItem(IDC_CHECK_INCLUDE_SUB)->EnableWindow(m_bCheckUpload);
	m_ListCtrl.EnableWindow(m_bCheckUpload);
}


void CDiffCopySetting::OnBnClickedRadioPkg()
{
	// TODO: 在此添加控件通知处理程序代码

	GetDlgItem(IDC_RADIO_LOCAL)->EnableWindow(TRUE);
	GetDlgItem(IDC_RADIO_REMOTE)->EnableWindow(TRUE);

	GetDlgItem(IDC_RADIO_FILE_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_RADIO_HASH_VALUE)->EnableWindow(FALSE);
}


void CDiffCopySetting::OnBnClickedRadioMaster()
{
	// TODO: 在此添加控件通知处理程序代码

	GetDlgItem(IDC_RADIO_LOCAL)->EnableWindow(FALSE);
	GetDlgItem(IDC_RADIO_REMOTE)->EnableWindow(FALSE);

	GetDlgItem(IDC_RADIO_FILE_SIZE)->EnableWindow(TRUE);
	GetDlgItem(IDC_RADIO_HASH_VALUE)->EnableWindow(TRUE);
}
