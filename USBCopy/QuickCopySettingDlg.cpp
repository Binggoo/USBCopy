// QuickCopySettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "QuickCopySettingDlg.h"
#include "afxdialogex.h"


// CQuickCopySettingDlg 对话框

IMPLEMENT_DYNAMIC(CQuickCopySettingDlg, CDialogEx)

CQuickCopySettingDlg::CQuickCopySettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CQuickCopySettingDlg::IDD, pParent)
	, m_bEnCompare(FALSE)
	, m_bEnMutiMBRSupported(FALSE)
	, m_bComputeHash(FALSE)
{
	m_pIni = NULL;
}

CQuickCopySettingDlg::~CQuickCopySettingDlg()
{
}

void CQuickCopySettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bEnCompare);
	DDX_Check(pDX, IDC_CHECK_MUTI_MBR, m_bEnMutiMBRSupported);
	DDX_Control(pDX, IDC_COMBO_MUTI_MBR, m_ComboBox);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bComputeHash);
}


BEGIN_MESSAGE_MAP(CQuickCopySettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_MUTI_MBR, &CQuickCopySettingDlg::OnBnClickedCheckMutiMbr)
	ON_BN_CLICKED(IDOK, &CQuickCopySettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CQuickCopySettingDlg::OnBnClickedCheckQsComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CQuickCopySettingDlg::OnBnClickedCheckCompare)
END_MESSAGE_MAP()


// CQuickCopySettingDlg 消息处理程序


BOOL CQuickCopySettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("QuickCopy"),_T("En_ComputeHash"),FALSE);
	m_bEnCompare = m_pIni->GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE);

	if (!m_bComputeHash)
	{
		m_bEnCompare = FALSE;
	}

#ifndef SD_CF
	m_bEnMutiMBRSupported = m_pIni->GetBool(_T("QuickCopy"),_T("En_MutiMBR"),FALSE);
	UINT nMBRLastLBA = m_pIni->GetUInt(_T("QuickCopy"),_T("MBRLastLBA"),5);

	m_ComboBox.AddString(_T("5"));
	m_ComboBox.AddString(_T("10"));
	m_ComboBox.AddString(_T("62"));

	SetDlgItemInt(IDC_COMBO_MUTI_MBR,nMBRLastLBA);
	m_ComboBox.EnableWindow(m_bEnMutiMBRSupported);

#else
	m_ComboBox.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_MUTI_MBR)->ShowWindow(SW_HIDE);
#endif
	

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CQuickCopySettingDlg::OnBnClickedCheckMutiMbr()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_ComboBox.EnableWindow(m_bEnMutiMBRSupported);
}


void CQuickCopySettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

#ifndef SD_CF
	UINT nMBRLastLBA = GetDlgItemInt(IDC_COMBO_MUTI_MBR);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_MutiMBR"),m_bEnMutiMBRSupported);
	m_pIni->WriteUInt(_T("QuickCopy"),_T("MBRLastLBA"),nMBRLastLBA);
#endif

	m_pIni->WriteBool(_T("QuickCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_Compare"),m_bEnCompare);
	
	CDialogEx::OnOK();
}

void CQuickCopySettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CQuickCopySettingDlg::OnBnClickedCheckQsComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		m_bEnCompare = FALSE;
		UpdateData(FALSE);
	}
}


void CQuickCopySettingDlg::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bEnCompare)
	{
		m_bComputeHash = TRUE;
		UpdateData(FALSE);
	}
}
