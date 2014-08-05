// ImageManager.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageManager.h"
#include "afxdialogex.h"
#include "Utils.h"


// CImageManager 对话框

IMPLEMENT_DYNAMIC(CImageManager, CDialogEx)

CImageManager::CImageManager(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageManager::IDD, pParent)
	, m_strTotalSpace(_T(""))
	, m_strUsedSpace(_T(""))
	, m_strTotalImages(_T(""))
{
	m_pIni = NULL;
}

CImageManager::~CImageManager()
{
}

void CImageManager::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_IMAGES, m_ListImages);
	DDX_Text(pDX, IDC_TEXT_TOTAL_SPACE, m_strTotalSpace);
	DDX_Text(pDX, IDC_TEXT_USED_SPACE, m_strUsedSpace);
	DDX_Text(pDX, IDC_TEXT_TOTAL_IMAGE, m_strTotalImages);
}


BEGIN_MESSAGE_MAP(CImageManager, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_DELETE, &CImageManager::OnBnClickedButtonDelete)
END_MESSAGE_MAP()


// CImageManager 消息处理程序


BOOL CImageManager::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_strImagePath = m_pIni->GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
	m_strImagePath.TrimRight(_T('\\'));

	// 获取使用容量和剩余容量
	ULARGE_INTEGER   uiFreeBytesAvailableToCaller = {0}; 
	ULARGE_INTEGER   uiTotalNumberOfBytes = {0}; 
	ULARGE_INTEGER   uiTotalNumberOfFreeBytes = {0};
	GetDiskFreeSpaceEx(m_strImagePath,&uiFreeBytesAvailableToCaller,&uiTotalNumberOfBytes,&uiTotalNumberOfFreeBytes);

	CString strTotal,strUsed;
	ULARGE_INTEGER uiUsed = {0};
	uiUsed.QuadPart = uiTotalNumberOfBytes.QuadPart - uiFreeBytesAvailableToCaller.QuadPart;
	strTotal = CUtils::AdjustFileSize(uiTotalNumberOfBytes.QuadPart);
	strUsed = CUtils::AdjustFileSize(uiUsed.QuadPart);

	m_strTotalSpace.Format(_T("%s"),strTotal);
	m_strUsedSpace.Format(_T("%s"),strUsed);

	// 初始化List
	InitailListCtrl();

	DWORD dwCount = EnumImage();
	m_strTotalImages.Format(_T("%ld"),dwCount);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CImageManager::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}

DWORD CImageManager::EnumImage()
{
	DWORD dwCount = 0;
	CString strFile = m_strImagePath + _T("\\*.IMG");
	CFileFind fileFind;
	BOOL bFind = fileFind.FindFile(strFile);
	CString strName,strSize,strModifyTime;
	while (bFind)
	{
		bFind = fileFind.FindNextFile();

		if (!fileFind.IsDirectory())
		{
			CFileStatus status;
			CFile::GetStatus(fileFind.GetFilePath(),status);

			strName = fileFind.GetFileName();
			strSize = CUtils::AdjustFileSize(status.m_size);
			strModifyTime = status.m_mtime.Format(_T("%Y-%m-%d %H:%M:%S"));

			m_ListImages.InsertItem(dwCount,strName);
			m_ListImages.SetItemText(dwCount,1,strSize);
			m_ListImages.SetItemText(dwCount,2,strModifyTime);
			dwCount++;
		}
	}
	fileFind.Close();

	return dwCount;
}

void CImageManager::InitailListCtrl()
{
	m_ListImages.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	
	CRect rect;
	m_ListImages.GetClientRect(&rect);
	int nWidth = rect.Width() - 120;
	m_ListImages.InsertColumn(0,_T("ImageName"),LVCFMT_LEFT,nWidth/2);
	m_ListImages.InsertColumn(1,_T("Size"),LVCFMT_LEFT,120);
	m_ListImages.InsertColumn(2,_T("Modify Time"),LVCFMT_LEFT,nWidth/2);
}


void CImageManager::OnBnClickedButtonDelete()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_ListImages.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListImages.GetNextSelectedItem(pos);
		CString strImagePath = m_strImagePath + _T("\\") + m_ListImages.GetItemText(nItem,0);

		if (DeleteFile(strImagePath))
		{
			m_ListImages.DeleteItem(nItem);
			pos = m_ListImages.GetFirstSelectedItemPosition();
		}
		
	}
}
