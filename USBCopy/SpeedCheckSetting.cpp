// SpeedCheckSetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SpeedCheckSetting.h"
#include "afxdialogex.h"


// CSpeedCheckSetting �Ի���

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


// CSpeedCheckSetting ��Ϣ�������


BOOL CSpeedCheckSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni != NULL);

	m_bCheckSpeedUpStandard = m_pIni->GetBool(_T("SpeedCheck"),_T("En_SpeedUpStandard"),FALSE);
	m_dbEditReadSpeed = m_pIni->GetDouble(_T("SpeedCheck"),_T("ReadSpeed"),0.0);
	m_dbEditWriteSpeed = m_pIni->GetDouble(_T("SpeedCheck"),_T("WriteSpeed"),0.0);

	GetDlgItem(IDC_EDIT_WRITE_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
	GetDlgItem(IDC_EDIT_READ_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CSpeedCheckSetting::OnBnClickedCheckSpeedUpStandard()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	GetDlgItem(IDC_EDIT_WRITE_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
	GetDlgItem(IDC_EDIT_READ_SPEED)->EnableWindow(m_bCheckSpeedUpStandard);
}


void CSpeedCheckSetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
