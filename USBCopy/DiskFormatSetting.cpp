// DiskFormatSetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "DiskFormatSetting.h"
#include "afxdialogex.h"


typedef struct { 
	TCHAR SizeString[16]; 
	DWORD ClusterSize; 
} SIZEDEFINITION, *PSIZEDEFINITION; 

SIZEDEFINITION LegalSizes[] = { 
	{ _T("Default"), 0 }, 
	{ _T("512"), 512 }, 
	{ _T("1024"), 1024 }, 
	{ _T("2048"), 2048 }, 
	{ _T("4096"), 4096 }, 
	{ _T("8192"), 8192 }, 
	{ _T("16K"), 16384 }, 
	{ _T("32K"), 32768 }, 
	{ _T("64K"), 65536 }, 
	{ _T("128K"), 65536 * 2 }, 
	{ _T("256K"), 65536 * 4 }
}; 

// CDiskFormatSetting 对话框

IMPLEMENT_DYNAMIC(CDiskFormatSetting, CDialogEx)

CDiskFormatSetting::CDiskFormatSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDiskFormatSetting::IDD, pParent)
	, m_strEditLabel(_T(""))
	, m_bCheckQuickFormat(FALSE)
{
	m_pIni = NULL;
}

CDiskFormatSetting::~CDiskFormatSetting()
{
}

void CDiskFormatSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_LABEL, m_strEditLabel);
	DDX_Control(pDX, IDC_COMBO_FILE_SYSTEM, m_ComboBoxFileSystem);
	DDX_Control(pDX, IDC_COMBO_CLUSTER_SIZE, m_ComboBoxClusterSize);
	DDX_Check(pDX, IDC_CHECK_QUICK_FORMAT, m_bCheckQuickFormat);
}


BEGIN_MESSAGE_MAP(CDiskFormatSetting, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDiskFormatSetting::OnBnClickedOk)
END_MESSAGE_MAP()


// CDiskFormatSetting 消息处理程序


BOOL CDiskFormatSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	// 隐藏快速格式化
	GetDlgItem(IDC_CHECK_QUICK_FORMAT)->ShowWindow(SW_HIDE);

	m_ComboBoxFileSystem.AddString(_T("NTFS"));
	m_ComboBoxFileSystem.AddString(_T("FAT32"));
	m_ComboBoxFileSystem.AddString(_T("exFAT"));

	int len = sizeof(LegalSizes)/sizeof(SIZEDEFINITION);

	for (int i = 0; i < len;i++)
	{
		m_ComboBoxClusterSize.AddString(LegalSizes[i].SizeString);
	}

	m_strEditLabel = m_pIni->GetString(_T("DiskFormat"),_T("VolumeLabel"));

	CString strFileSystem = m_pIni->GetString(_T("DiskFormat"),_T("FileSystem"),_T("NTFS"));
	UINT nClusterSize = m_pIni->GetUInt(_T("DiskFormat"),_T("ClusterSize"),0);

	int nCount = m_ComboBoxFileSystem.GetCount();
	int nSelectIndex = 0;
	for (int i = 0; i < nCount;i++)
	{
		CString strItem;
		m_ComboBoxFileSystem.GetLBText(i,strItem);

		if (strItem.CompareNoCase(strFileSystem) == 0)
		{
			nSelectIndex = i;
			break;
		}
	}

	m_ComboBoxFileSystem.SetCurSel(nSelectIndex);

	nSelectIndex = 0;
	for (int i = 0;i < len;i++)
	{
		if (nClusterSize == LegalSizes[i].ClusterSize)
		{
			nSelectIndex = i;
			break;
		}
	}
	m_ComboBoxClusterSize.SetCurSel(nSelectIndex);

	m_bCheckQuickFormat = m_pIni->GetBool(_T("DiskFormat"),_T("QuickFormat"),TRUE);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CDiskFormatSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CDiskFormatSetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteString(_T("DiskFormat"),_T("VolumeLabel"),m_strEditLabel);

	CString strFileSystem;
	m_ComboBoxFileSystem.GetWindowText(strFileSystem);

	m_pIni->WriteString(_T("DiskFormat"),_T("FileSystem"),strFileSystem);

	UINT nClusterSize = LegalSizes[m_ComboBoxClusterSize.GetCurSel()].ClusterSize;

	m_pIni->WriteUInt(_T("DiskFormat"),_T("ClusterSize"),nClusterSize);

	m_pIni->WriteBool(_T("DiskFormat"),_T("QuickFormat"),m_bCheckQuickFormat);

	CDialogEx::OnOK();
}
