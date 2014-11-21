// MTPCopySetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "MTPCopySetting.h"
#include "afxdialogex.h"


// CMTPCopySetting 对话框

IMPLEMENT_DYNAMIC(CMTPCopySetting, CDialogEx)

CMTPCopySetting::CMTPCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMTPCopySetting::IDD, pParent)
	, m_bComputeHash(FALSE)
	, m_bCompare(FALSE)
	, m_nCompareMethodIndex(FALSE)
{
	m_pIni = NULL;
}

CMTPCopySetting::~CMTPCopySetting()
{
}

void CMTPCopySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bComputeHash);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCompare);
	DDX_Radio(pDX,IDC_RADIO_HASH_COMPARE,m_nCompareMethodIndex);
}


BEGIN_MESSAGE_MAP(CMTPCopySetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CMTPCopySetting::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CMTPCopySetting::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDOK, &CMTPCopySetting::OnBnClickedOk)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_RADIO_HASH_COMPARE, &CMTPCopySetting::OnBnClickedRadioHashCompare)
END_MESSAGE_MAP()


// CMTPCopySetting 消息处理程序


BOOL CMTPCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("MTPCopy"),_T("En_ComputeHash"),FALSE);
	m_bCompare = m_pIni->GetBool(_T("MTPCopy"),_T("En_Compare"),FALSE);
	m_nCompareMethodIndex = m_pIni->GetInt(_T("MTPCopy"),_T("CompareMethod"),0);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCompare);

	if (!m_bComputeHash)
	{
		if (m_bCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CMTPCopySetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CMTPCopySetting::OnBnClickedCheckComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		if (m_bCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}

	UpdateData(FALSE);
}


void CMTPCopySetting::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	
	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCompare);
}


void CMTPCopySetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData(TRUE);

	m_pIni->WriteBool(_T("MTPCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("MTPCopy"),_T("En_Compare"),m_bCompare);
	m_pIni->WriteInt(_T("MTPCopy"),_T("CompareMethod"),m_nCompareMethodIndex);

	CDialogEx::OnOK();
}


HBRUSH CMTPCopySetting::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if (pWnd->GetDlgCtrlID() == IDC_STATIC)
	{
		pDC->SetTextColor(RGB(255,0,0));
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


void CMTPCopySetting::OnBnClickedRadioHashCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_bComputeHash = TRUE;

	UpdateData(FALSE);
}
