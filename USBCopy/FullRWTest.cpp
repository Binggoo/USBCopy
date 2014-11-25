// FullRWTest.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FullRWTest.h"
#include "afxdialogex.h"


// CFullRWTest �Ի���

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


// CFullRWTest ��Ϣ�������


BOOL CFullRWTest::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
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
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CFullRWTest::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (m_bCheckRetainOriginData && m_bCheckFormatFinish)
	{
		m_bCheckRetainOriginData = FALSE;
	}

	UpdateData(FALSE);
}


void CFullRWTest::OnBnClickedCheckFormat()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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

	// TODO:  �ڴ˸��� DC ���κ�����
	if (pWnd->GetDlgCtrlID() == IDC_STATIC)
	{
		pDC->SetTextColor(RGB(255,0,0));
	}

	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	return hbr;
}


void CFullRWTest::OnBnClickedCheckReadOnly()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
