// FullCopySettingDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FullCopySettingDlg.h"
#include "afxdialogex.h"


// CFullCopySettingDlg �Ի���

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


// CFullCopySettingDlg ��Ϣ�������


BOOL CFullCopySettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("FullCopy"),_T("En_ComputeHash"),FALSE);
	m_bCompare = m_pIni->GetBool(_T("FullCopy"),_T("En_Compare"),FALSE);

	if (!m_bComputeHash)
	{
		m_bCompare = FALSE;
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CFullCopySettingDlg::OnBnClickedCheckCompare()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	if (m_bCompare)
	{
		m_bComputeHash = TRUE;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedCheckComputeHash()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		m_bCompare = m_bComputeHash;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_pIni->WriteBool(_T("FullCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("FullCopy"),_T("En_Compare"),m_bCompare);


	CDialogEx::OnOK();
}
