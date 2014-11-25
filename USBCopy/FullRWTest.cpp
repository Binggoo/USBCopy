// FullRWTest.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FullRWTest.h"
#include "afxdialogex.h"


// CFullRWTest 对话框

IMPLEMENT_DYNAMIC(CFullRWTest, CDialogEx)

CFullRWTest::CFullRWTest(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFullRWTest::IDD, pParent)
	, m_bCheckRetainOriginData(FALSE)
	, m_bCheckFormatFinish(FALSE)
	, m_bCheckStopBad(FALSE)
	, m_bCheckReadOnlyTest(FALSE)
{
	m_pIni = NULL;
}

CFullRWTest::~CFullRWTest()
{
}

void CFullRWTest::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_RETAI_ORIGIN_DATA, m_bCheckRetainOriginData);
	DDX_Check(pDX, IDC_CHECK_FORMAT, m_bCheckFormatFinish);
	DDX_Check(pDX, IDC_CHECK_STOP_BAD, m_bCheckStopBad);
	DDX_Check(pDX, IDC_CHECK_READ_ONLY, m_bCheckReadOnlyTest);
}


BEGIN_MESSAGE_MAP(CFullRWTest, CDialogEx)
	ON_BN_CLICKED(IDOK, &CFullRWTest::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_RETAI_ORIGIN_DATA, &CFullRWTest::OnBnClickedCheckRetaiOriginData)
	ON_BN_CLICKED(IDC_CHECK_FORMAT, &CFullRWTest::OnBnClickedCheckFormat)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CHECK_READ_ONLY, &CFullRWTest::OnBnClickedCheckReadOnly)
END_MESSAGE_MAP()


// CFullRWTest 消息处理程序


BOOL CFullRWTest::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni != NULL);

	m_bCheckReadOnlyTest = m_pIni->GetBool(_T("FullRWTest"),_T("En_ReadOnlyTest"),FALSE);
	m_bCheckRetainOriginData = m_pIni->GetBool(_T("FullRWTest"),_T("En_RetainOriginData"),FALSE);
	m_bCheckFormatFinish = m_pIni->GetBool(_T("FullRWTest"),_T("En_FormatFinished"),FALSE);
	m_bCheckStopBad = m_pIni->GetBool(_T("FullRWTest"),_T("En_StopBadBlock"),FALSE);

	if (m_bCheckReadOnlyTest)
	{
		GetDlgItem(IDC_CHECK_RETAI_ORIGIN_DATA)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_FORMAT)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_STOP_BAD)->EnableWindow(FALSE);
	}

	if (m_bCheckRetainOriginData && m_bCheckFormatFinish)
	{
		m_bCheckFormatFinish = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CFullRWTest::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteBool(_T("FullRWTest"),_T("En_ReadOnlyTest"),m_bCheckReadOnlyTest);
	m_pIni->WriteBool(_T("FullRWTest"),_T("En_RetainOriginData"),m_bCheckRetainOriginData);
	m_pIni->WriteBool(_T("FullRWTest"),_T("En_FormatFinished"),m_bCheckFormatFinish);
	m_pIni->WriteBool(_T("FullRWTest"),_T("En_StopBadBlock"),m_bCheckStopBad);

	CDialogEx::OnOK();
}

void CFullRWTest::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CFullRWTest::OnBnClickedCheckRetaiOriginData()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckRetainOriginData && m_bCheckFormatFinish)
	{
		m_bCheckRetainOriginData = FALSE;
	}

	UpdateData(FALSE);
}


void CFullRWTest::OnBnClickedCheckFormat()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckRetainOriginData && m_bCheckFormatFinish)
	{
		m_bCheckFormatFinish = FALSE;
	}

	UpdateData(FALSE);
}


HBRUSH CFullRWTest::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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


void CFullRWTest::OnBnClickedCheckReadOnly()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckReadOnlyTest)
	{
		GetDlgItem(IDC_CHECK_RETAI_ORIGIN_DATA)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_FORMAT)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_STOP_BAD)->EnableWindow(FALSE);
	}
	else
	{		
		GetDlgItem(IDC_CHECK_RETAI_ORIGIN_DATA)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_FORMAT)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_STOP_BAD)->EnableWindow(TRUE);
	}
}
