// FullCopySettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FullCopySettingDlg.h"
#include "afxdialogex.h"


// CFullCopySettingDlg 对话框

IMPLEMENT_DYNAMIC(CFullCopySettingDlg, CDialogEx)

CFullCopySettingDlg::CFullCopySettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFullCopySettingDlg::IDD, pParent)
	, m_bComputeHash(FALSE)
	, m_bCompare(FALSE)
{
	m_pIni = NULL;
}

CFullCopySettingDlg::~CFullCopySettingDlg()
{
}

void CFullCopySettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bComputeHash);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCompare);;
}


BEGIN_MESSAGE_MAP(CFullCopySettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CFullCopySettingDlg::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CFullCopySettingDlg::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDOK, &CFullCopySettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()

void CFullCopySettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


// CFullCopySettingDlg 消息处理程序


BOOL CFullCopySettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("FullCopy"),_T("En_ComputeHash"),FALSE);
	m_bCompare = m_pIni->GetBool(_T("FullCopy"),_T("En_Compare"),FALSE);

	if (!m_bComputeHash)
	{
		m_bCompare = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CFullCopySettingDlg::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_bCompare)
	{
		m_bComputeHash = TRUE;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedCheckComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		m_bCompare = m_bComputeHash;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteBool(_T("FullCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("FullCopy"),_T("En_Compare"),m_bCompare);


	CDialogEx::OnOK();
}
