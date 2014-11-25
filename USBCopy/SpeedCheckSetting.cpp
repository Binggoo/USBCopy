// SpeedCheckSetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SpeedCheckSetting.h"
#include "afxdialogex.h"


// CSpeedCheckSetting 对话框

IMPLEMENT_DYNAMIC(CSpeedCheckSetting, CDialogEx)

CSpeedCheckSetting::CSpeedCheckSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSpeedCheckSetting::IDD, pParent)
	, m_bCheckSpeedUpStandard(FALSE)
	, m_dbEditReadSpeed(0)
	, m_dbEditWriteSpeed(0)
{
	m_pIni = NULL;
}

CSpeedCheckSetting::~CSpeedCheckSetting()
{
}

void CSpeedCheckSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_SPEED_UP_STANDARD, m_bCheckSpeedUpStandard);
	DDX_Text(pDX, IDC_EDIT_READ_SPEED, m_dbEditReadSpeed);
	DDX_Text(pDX, IDC_EDIT_WRITE_SPEED, m_dbEditWriteSpeed);
}


BEGIN_MESSAGE_MAP(CSpeedCheckSetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_SPEED_UP_STANDARD, &CSpeedCheckSetting::OnBnClickedCheckSpeedUpStandard)
	ON_BN_CLICKED(IDOK, &CSpeedCheckSetting::OnBnClickedOk)
END_MESSAGE_MAP()


// CSpeedCheckSetting 消息处理程序


BOOL CSpeedCheckSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni != NULL);

	m_bCheckSpeedUpStandard = m_pIni->GetBool(_T("SpeedCheck"),_T("En_SpeedUpStandard"),FALSE);
	m_dbEditReadSpeed = m_pIni->GetDouble(_T("SpeedCheck"),_T("ReadSpeed"),0.0);
	m_dbEditWriteSpeed = m_pIni->GetDouble(_T("SpeedCheck"),_T("WriteSpeed"),0.0);

	GetDlgItem(IDC_EDIT_WRITE_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
	GetDlgItem(IDC_EDIT_READ_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSpeedCheckSetting::OnBnClickedCheckSpeedUpStandard()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	GetDlgItem(IDC_EDIT_WRITE_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
	GetDlgItem(IDC_EDIT_READ_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
}


void CSpeedCheckSetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pIni->WriteBool(_T("SpeedCheck"),_T("En_SpeedUpStandard"),m_bCheckSpeedUpStandard);
	m_pIni->WriteDouble(_T("SpeedCheck"),_T("ReadSpeed"),m_dbEditReadSpeed);
	m_pIni->WriteDouble(_T("SpeedCheck"),_T("WriteSpeed"),m_dbEditWriteSpeed);

	CDialogEx::OnOK();
}

void CSpeedCheckSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}
