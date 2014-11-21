// DiskCompareSettingDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "DiskCompareSettingDlg.h"
#include "afxdialogex.h"


// CDiskCompareSettingDlg �Ի���

IMPLEMENT_DYNAMIC(CDiskCompareSettingDlg, CDialogEx)

CDiskCompareSettingDlg::CDiskCompareSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDiskCompareSettingDlg::IDD, pParent)
	, m_nCompareModeIndex(0)
	, m_nCompareMethodIndex(0)
	
{
	m_pIni = NULL;
}

CDiskCompareSettingDlg::~CDiskCompareSettingDlg()
{
}

void CDiskCompareSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FULL_COMPARE, m_nCompareModeIndex);
	DDX_Radio(pDX, IDC_RADIO_HASH_COMPARE, m_nCompareMethodIndex);
}


BEGIN_MESSAGE_MAP(CDiskCompareSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDiskCompareSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()

void CDiskCompareSettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


// CDiskCompareSettingDlg ��Ϣ�������


BOOL CDiskCompareSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);

	m_nCompareModeIndex = m_pIni->GetInt(_T("DiskCompare"),_T("CompareMode"),0);
	m_nCompareMethodIndex = m_pIni->GetInt(_T("DiskCompare"),_T("CompareMethod"),0);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CDiskCompareSettingDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_pIni->WriteInt(_T("DiskCompare"),_T("CompareMode"),m_nCompareModeIndex);
	m_pIni->WriteInt(_T("DiskCompare"),_T("CompareMethod"),m_nCompareMethodIndex);

	CDialogEx::OnOK();
}
