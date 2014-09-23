// ExportLog.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ExportLog.h"
#include "afxdialogex.h"
#include "Utils.h"


// CExportLog 对话框

IMPLEMENT_DYNAMIC(CExportLog, CDialogEx)

CExportLog::CExportLog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CExportLog::IDD, pParent)
{

}

CExportLog::~CExportLog()
{
}

void CExportLog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DATE, m_ComboBoxDate);
	DDX_Control(pDX, IDC_LIST_RECORD, m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CExportLog, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_DEL, &CExportLog::OnBnClickedBtnDel)
	ON_BN_CLICKED(IDOK, &CExportLog::OnBnClickedOk)
	ON_CBN_EDITCHANGE(IDC_COMBO_DATE, &CExportLog::OnCbnEditchangeComboDate)
	ON_CBN_SELCHANGE(IDC_COMBO_DATE, &CExportLog::OnCbnSelchangeComboDate)
END_MESSAGE_MAP()


// CExportLog 消息处理程序


BOOL CExportLog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	m_ComboBoxDate.AddString(_T("0"));
	m_ComboBoxDate.AddString(_T("1"));
	m_ComboBoxDate.AddString(_T("3"));
	m_ComboBoxDate.AddString(_T("5"));
	m_ComboBoxDate.AddString(_T("7"));
	m_ComboBoxDate.AddString(_T("15"));
	m_ComboBoxDate.AddString(_T("30"));

	m_ComboBoxDate.SetCurSel(0);

	InitialListCtrl();
	AddListData(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CExportLog::InitialListCtrl()
{
	// 初始化ListCtrl
	CRect rect;
	m_ListCtrl.GetClientRect(&rect);
	int nWidth = rect.Width() - 200;

	CString strResText;

	int nItem = 0;

	strResText.LoadString(IDS_ITEM_FILE_NAME);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,200,0);

	strResText.LoadString(IDS_ITEM_MODIFY_TIME);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth/2,0);

	strResText.LoadString(IDS_ITEM_SIZE);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth/2,0);

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CExportLog::AddListData(DWORD days)
{

	CString strFindFile = m_strAppPath + _T("\\RECORD*.txt");
	CTime time = CTime::GetCurrentTime();

	if (days == 0)
	{
		days = -1;
	}

	m_ListCtrl.DeleteAllItems();

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	int nItem = 0;
	CString strModifyTime,strSize, strFileName;
	while (bFind)
	{
		bFind = ff.FindNextFile();

		CFileStatus status;

		CFile::GetStatus(ff.GetFilePath(),status);

		strModifyTime = status.m_mtime.Format("%Y-%m-%d %H:%M");
		strSize = CUtils::AdjustFileSize(status.m_size);
		strFileName = ff.GetFileName();

		CTimeSpan span = time - status.m_mtime;

		if (span.GetDays() < days)
		{
			m_ListCtrl.InsertItem(nItem,strFileName);
			m_ListCtrl.SetItemText(nItem,1,strModifyTime);
			m_ListCtrl.SetItemText(nItem,2,strSize);

			nItem++;
		}
	}
	ff.Close();
}

void CExportLog::OnBnClickedBtnDel()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);
		CString strRecordPath = m_strAppPath + _T("\\") + m_ListCtrl.GetItemText(nItem,0);

		if (DeleteFile(strRecordPath))
		{
			m_ListCtrl.DeleteItem(nItem);
			pos = m_ListCtrl.GetFirstSelectedItemPosition();
		}

	}
}


void CExportLog::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	int nCount = m_ListCtrl.GetItemCount();
	m_strFileArray.RemoveAll();

	for (int i = 0; i < nCount;i++)
	{
		CString strFile = m_ListCtrl.GetItemText(i,0);

		m_strFileArray.Add(strFile);
	}

	CDialogEx::OnOK();
}

void CExportLog::GetFileArray( CStringArray &strArray )
{
	strArray.Copy(m_strFileArray);
}


void CExportLog::OnCbnEditchangeComboDate()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	m_ComboBoxDate.GetWindowText(strText);

	if (strText.IsEmpty())
	{
		return;
	}

	TCHAR ch = strText.GetAt(strText.GetLength()-1);
	if (!_istdigit(ch))
	{
		m_ComboBoxDate.SetWindowText(_T(""));
	}
	else
	{
		DWORD dwDays = _ttoi(strText);

		AddListData(dwDays);
	}
}


void CExportLog::OnCbnSelchangeComboDate()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelIndex = m_ComboBoxDate.GetCurSel();

	CString strText;
	m_ComboBoxDate.GetLBText(nSelIndex,strText);

	DWORD dwDays = _ttoi(strText);

	AddListData(dwDays);
}
