// DiskCompareSettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "DiskCompareSettingDlg.h"
#include "afxdialogex.h"


// CDiskCompareSettingDlg 对话框

IMPLEMENT_DYNAMIC(CDiskCompareSettingDlg, CDialogEx)

CDiskCompareSettingDlg::CDiskCompareSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDiskCompareSettingDlg::IDD, pParent)
	, m_nCompareModeIndex(0)
	, m_nCompareMethodIndex(0)
	
{
	m_pIni = NULL;
}

CDiskCompareSettingDlg::~CDiskCompareSettingDlg()
{
}

void CDiskCompareSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FULL_COMPARE, m_nCompareModeIndex);
	DDX_Radio(pDX, IDC_RADIO_HASH_COMPARE, m_nCompareMethodIndex);
}


BEGIN_MESSAGE_MAP(CDiskCompareSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDiskCompareSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()

void CDiskCompareSettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


// CDiskCompareSettingDlg 消息处理程序


BOOL CDiskCompareSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	m_nCompareModeIndex = m_pIni->GetInt(_T("DiskCompare"),_T("CompareMode"),0);
	m_nCompareMethodIndex = m_pIni->GetInt(_T("DiskCompare"),_T("CompareMethod"),0);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CDiskCompareSettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteInt(_T("DiskCompare"),_T("CompareMode"),m_nCompareModeIndex);
	m_pIni->WriteInt(_T("DiskCompare"),_T("CompareMethod"),m_nCompareMethodIndex);

	CDialogEx::OnOK();
}
