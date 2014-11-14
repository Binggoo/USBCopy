// ImageCopySetting.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageCopySetting.h"
#include "afxdialogex.h"


// CImageCopySetting �Ի���

IMPLEMENT_DYNAMIC(CImageCopySetting, CDialogEx)

CImageCopySetting::CImageCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageCopySetting::IDD, pParent)
	, m_nRadioPriorityIndex(0)
	, m_bCheckCompare(FALSE)
	, m_nRadioImageTypeIndex(0)
{
	m_pIni = NULL;
}

CImageCopySetting::~CImageCopySetting()
{
}

void CImageCopySetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_LOCAL_FIRST, m_nRadioPriorityIndex);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCheckCompare);
	DDX_Radio(pDX, IDC_RADIO_DISK_IMAGE, m_nRadioImageTypeIndex);
}


BEGIN_MESSAGE_MAP(CImageCopySetting, CDialogEx)
	ON_BN_CLICKED(IDOK, &CImageCopySetting::OnBnClickedOk)
END_MESSAGE_MAP()


// CImageCopySetting ��Ϣ�������


BOOL CImageCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);

	m_nRadioPriorityIndex = m_pIni->GetUInt(_T("ImageCopy"),_T("PathType"),0);
	m_bCheckCompare = m_pIni->GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE);
	m_nRadioImageTypeIndex = m_pIni->GetUInt(_T("ImageCopy"),_T("ImageType"),0);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CImageCopySetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CImageCopySetting::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);

	m_pIni->WriteUInt(_T("ImageCopy"),_T("PathType"),m_nRadioPriorityIndex);
	m_pIni->WriteBool(_T("ImageCopy"),_T("En_Compare"),m_bCheckCompare);
	m_pIni->WriteUInt(_T("ImageCopy"),_T("ImageType"),m_nRadioImageTypeIndex);

	CDialogEx::OnOK();
}
