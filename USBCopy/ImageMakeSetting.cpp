// ImageMakeSetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageMakeSetting.h"
#include "afxdialogex.h"


// CImageMakeSetting �Ի���

IMPLEMENT_DYNAMIC(CImageMakeSetting, CDialogEx)

CImageMakeSetting::CImageMakeSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageMakeSetting::IDD, pParent)
	, m_nRadioSelectIndex(0)
	, m_bCheckSupportMutiMBR(FALSE)
{
	m_pIni = NULL;
}

CImageMakeSetting::~CImageMakeSetting()
{
}

void CImageMakeSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_MUTI_MBR, m_bCheckSupportMutiMBR);
	DDX_Control(pDX, IDC_COMBO_MUTI_MBR, m_ComboBoxMBRLBA);
	DDX_Radio(pDX, IDC_RADIO_LOCAL, m_nRadioSelectIndex);
}


BEGIN_MESSAGE_MAP(CImageMakeSetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_MUTI_MBR, &CImageMakeSetting::OnBnClickedCheckMutiMbr)
	ON_BN_CLICKED(IDOK, &CImageMakeSetting::OnBnClickedOk)
END_MESSAGE_MAP()


// CImageMakeSetting ��Ϣ�������


BOOL CImageMakeSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);

	m_nRadioSelectIndex = m_pIni->GetUInt(_T("ImageMake"),_T("PathType"),0);

#ifndef SD_CF
	m_bCheckSupportMutiMBR = m_pIni->GetBool(_T("ImageMake"),_T("En_MutiMBR"),FALSE);
	UINT nMBRLastLBA = m_pIni->GetUInt(_T("ImageMake"),_T("MBRLastLBA"),5);

	m_ComboBoxMBRLBA.AddString(_T("5"));
	m_ComboBoxMBRLBA.AddString(_T("10"));
	m_ComboBoxMBRLBA.AddString(_T("62"));

	SetDlgItemInt(IDC_COMBO_MUTI_MBR,nMBRLastLBA);
	m_ComboBoxMBRLBA.EnableWindow(m_bCheckSupportMutiMBR);

#else
	m_ComboBoxMBRLBA.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_MUTI_MBR)->ShowWindow(SW_HIDE);
#endif


	UpdateData(FALSE);


	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CImageMakeSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CImageMakeSetting::OnBnClickedCheckMutiMbr()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	m_ComboBoxMBRLBA.EnableWindow(m_bCheckSupportMutiMBR);
}


void CImageMakeSetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_pIni->WriteUInt(_T("ImageMake"),_T("SavePath"),m_nRadioSelectIndex);

#ifndef SD_CF
	UINT nMBRLastLBA = GetDlgItemInt(IDC_COMBO_MUTI_MBR);
	m_pIni->WriteBool(_T("ImageMake"),_T("En_MutiMBR"),m_bCheckSupportMutiMBR);
	m_pIni->WriteUInt(_T("ImageMake"),_T("MBRLastLBA"),nMBRLastLBA);
#endif

	CDialogEx::OnOK();
}
