// MTPCopySetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "MTPCopySetting.h"
#include "afxdialogex.h"


// CMTPCopySetting �Ի���

IMPLEMENT_DYNAMIC(CMTPCopySetting, CDialogEx)

CMTPCopySetting::CMTPCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMTPCopySetting::IDD, pParent)
	, m_bComputeHash(FALSE)
	, m_bCompare(FALSE)
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
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCompare);;
}


BEGIN_MESSAGE_MAP(CMTPCopySetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CMTPCopySetting::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CMTPCopySetting::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDOK, &CMTPCopySetting::OnBnClickedOk)
END_MESSAGE_MAP()


// CMTPCopySetting ��Ϣ�������


BOOL CMTPCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("MTPCopy"),_T("En_ComputeHash"),FALSE);
	m_bCompare = m_pIni->GetBool(_T("MTPCopy"),_T("En_Compare"),FALSE);

	if (!m_bComputeHash)
	{
		m_bCompare = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CMTPCopySetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CMTPCopySetting::OnBnClickedCheckComputeHash()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		m_bCompare = m_bComputeHash;
		UpdateData(FALSE);
	}
}


void CMTPCopySetting::OnBnClickedCheckCompare()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	if (m_bCompare)
	{
		m_bComputeHash = TRUE;
		UpdateData(FALSE);
	}
}


void CMTPCopySetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	UpdateData(TRUE);

	m_pIni->WriteBool(_T("MTPCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("MTPCopy"),_T("En_Compare"),m_bCompare);

	CDialogEx::OnOK();
}
