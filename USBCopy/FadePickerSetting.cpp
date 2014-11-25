// FadePickerSetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FadePickerSetting.h"
#include "afxdialogex.h"


// CFadePickerSetting 对话框

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


// CFadePickerSetting 消息处理程序


BOOL CFadePickerSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni != NULL);

	m_bCheckRetainOriginData = m_pIni->GetBool(_T("FadePicker"),_T("En_RetainOriginData"),FALSE);
	m_bCheckFormatFinished = m_pIni->GetBool(_T("FadePicker"),_T("En_FormatFinished"),FALSE);

	if (m_bCheckRetainOriginData && m_bCheckFormatFinished)
	{
		m_bCheckFormatFinished = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CFadePickerSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CFadePickerSetting::OnBnClickedCheckRetaiOriginData()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckFormatFinished && m_bCheckRetainOriginData)
	{
		m_bCheckRetainOriginData = FALSE;
	}

	UpdateData(FALSE);
}


void CFadePickerSetting::OnBnClickedCheckFormat()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_bCheckFormatFinished && m_bCheckRetainOriginData)
	{
		m_bCheckFormatFinished = FALSE;
	}

	UpdateData(FALSE);
}


void CFadePickerSetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteBool(_T("FadePicker"),_T("En_RetainOriginData"),m_bCheckRetainOriginData);
	m_pIni->WriteBool(_T("FadePicker"),_T("En_FormatFinished"),m_bCheckFormatFinished);

	CDialogEx::OnOK();
}


HBRUSH CFadePickerSetting::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
