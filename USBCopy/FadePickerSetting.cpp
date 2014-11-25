// FadePickerSetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FadePickerSetting.h"
#include "afxdialogex.h"


// CFadePickerSetting �Ի���

IMPLEMENT_DYNAMIC(CFadePickerSetting, CDialogEx)

CFadePickerSetting::CFadePickerSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFadePickerSetting::IDD, pParent)
	, m_bCheckRetainOriginData(FALSE)
	, m_bCheckFormatFinished(FALSE)
{
	m_pIni = NULL;
}

CFadePickerSetting::~CFadePickerSetting()
{
}

void CFadePickerSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_RETAI_ORIGIN_DATA, m_bCheckRetainOriginData);
	DDX_Check(pDX, IDC_CHECK_FORMAT, m_bCheckFormatFinished);
}


BEGIN_MESSAGE_MAP(CFadePickerSetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_RETAI_ORIGIN_DATA, &CFadePickerSetting::OnBnClickedCheckRetaiOriginData)
	ON_BN_CLICKED(IDC_CHECK_FORMAT, &CFadePickerSetting::OnBnClickedCheckFormat)
	ON_BN_CLICKED(IDOK, &CFadePickerSetting::OnBnClickedOk)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CFadePickerSetting ��Ϣ�������


BOOL CFadePickerSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni != NULL);

	m_bCheckRetainOriginData = m_pIni->GetBool(_T("FadePicker"),_T("En_RetainOriginData"),FALSE);
	m_bCheckFormatFinished = m_pIni->GetBool(_T("FadePicker"),_T("En_FormatFinished"),FALSE);

	if (m_bCheckRetainOriginData && m_bCheckFormatFinished)
	{
		m_bCheckFormatFinished = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CFadePickerSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CFadePickerSetting::OnBnClickedCheckRetaiOriginData()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (m_bCheckFormatFinished && m_bCheckRetainOriginData)
	{
		m_bCheckRetainOriginData = FALSE;
	}

	UpdateData(FALSE);
}


void CFadePickerSetting::OnBnClickedCheckFormat()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (m_bCheckFormatFinished && m_bCheckRetainOriginData)
	{
		m_bCheckFormatFinished = FALSE;
	}

	UpdateData(FALSE);
}


void CFadePickerSetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_pIni->WriteBool(_T("FadePicker"),_T("En_RetainOriginData"),m_bCheckRetainOriginData);
	m_pIni->WriteBool(_T("FadePicker"),_T("En_FormatFinished"),m_bCheckFormatFinished);

	CDialogEx::OnOK();
}


HBRUSH CFadePickerSetting::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
