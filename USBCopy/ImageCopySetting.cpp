// ImageCopySetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageCopySetting.h"
#include "afxdialogex.h"


// CImageCopySetting 对话框

IMPLEMENT_DYNAMIC(CImageCopySetting, CDialogEx)

CImageCopySetting::CImageCopySetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageCopySetting::IDD, pParent)
	, m_nRadioPriorityIndex(0)
	, m_bCheckCompare(FALSE)
	, m_nRadioImageTypeIndex(0)
	, m_bCleanDiskFirst(FALSE)
	, m_strFillValues(_T(""))
	, m_nCompareMethodIndex(0)
	, m_bCompareClean(FALSE)
	, m_nCompareCleanSeqIndex(0)
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
	DDX_Check(pDX, IDC_CHECK_CLEAN_DISK_FIRST, m_bCleanDiskFirst);
	DDX_Control(pDX, IDC_COMBO_CLEAN_TIMES, m_ComboBoxCleanTimes);
	DDX_Text(pDX, IDC_EDIT_FILL_VALUE, m_strFillValues);
	DDX_Radio(pDX,IDC_RADIO_HASH_COMPARE,m_nCompareMethodIndex);
	DDX_Check(pDX,IDC_CHECK_CLEAN_COMPARE,m_bCompareClean);
	DDX_Radio(pDX,IDC_RADIO_CLEAN_IN,m_nCompareCleanSeqIndex);
}


BEGIN_MESSAGE_MAP(CImageCopySetting, CDialogEx)
	ON_BN_CLICKED(IDOK, &CImageCopySetting::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_DISK_IMAGE, &CImageCopySetting::OnBnClickedRadioDiskImage)
	ON_BN_CLICKED(IDC_RADIO_MTP_IMAGE, &CImageCopySetting::OnBnClickedRadioMtpImage)
	ON_BN_CLICKED(IDC_CHECK_CLEAN_DISK_FIRST, &CImageCopySetting::OnBnClickedCheckCleanDisk)
	ON_CBN_SELCHANGE(IDC_COMBO_CLEAN_TIMES, &CImageCopySetting::OnCbnSelchangeComboCleanTimes)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CImageCopySetting::OnBnClickedCheckCompare)
END_MESSAGE_MAP()


// CImageCopySetting 消息处理程序


BOOL CImageCopySetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni != NULL);

	m_nRadioPriorityIndex = m_pIni->GetUInt(_T("ImageCopy"),_T("PathType"),0);
	m_bCheckCompare = m_pIni->GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE);
	m_nRadioImageTypeIndex = m_pIni->GetUInt(_T("ImageCopy"),_T("ImageType"),0);
	m_nCompareMethodIndex = m_pIni->GetUInt(_T("ImageCopy"),_T("CompareMethod"),0);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCheckCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCheckCompare);

	m_bCleanDiskFirst = m_pIni->GetBool(_T("ImageCopy"),_T("En_CleanDiskFirst"),FALSE);
	m_bCompareClean = m_pIni->GetBool(_T("ImageCopy"),_T("En_CompareClean"),FALSE);
	m_nCompareCleanSeqIndex = m_pIni->GetInt(_T("ImageCopy"),_T("CompareCleanSeq"),0);

	m_ComboBoxCleanTimes.AddString(_T("1"));
	m_ComboBoxCleanTimes.AddString(_T("2"));
	m_ComboBoxCleanTimes.AddString(_T("3"));

	// MTP 映像
	if (m_nRadioImageTypeIndex == 1)
	{
		GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_CLEAN_DISK_FIRST)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(FALSE);

		if (m_bCheckCompare)
		{
			m_nCompareMethodIndex = 0;
		}
	}
	else
	{
		GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(m_bCleanDiskFirst);
		GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
		GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(m_bCleanDiskFirst);
		GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(m_bCleanDiskFirst);
		GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(m_bCleanDiskFirst);
	}

	m_strFillValues = m_pIni->GetString(_T("ImageCopy"),_T("FillValues"));
	UINT nCleanTimes = m_pIni->GetUInt(_T("ImageCopy"),_T("CleanTimes"),1);

	if (nCleanTimes < 1 || nCleanTimes > 3)
	{
		nCleanTimes = 1;
	}

	m_ComboBoxCleanTimes.SetCurSel(nCleanTimes - 1);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CImageCopySetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CImageCopySetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteUInt(_T("ImageCopy"),_T("PathType"),m_nRadioPriorityIndex);
	m_pIni->WriteBool(_T("ImageCopy"),_T("En_Compare"),m_bCheckCompare);
	m_pIni->WriteUInt(_T("ImageCopy"),_T("CompareMethod"),m_nCompareMethodIndex);

	m_pIni->WriteUInt(_T("ImageCopy"),_T("ImageType"),m_nRadioImageTypeIndex);
	m_pIni->WriteBool(_T("ImageCopy"),_T("En_CleanDiskFirst"),m_bCleanDiskFirst);
	m_pIni->WriteBool(_T("ImageCopy"),_T("En_CompareClean"),m_bCompareClean);
	m_pIni->WriteString(_T("ImageCopy"),_T("FillValues"),m_strFillValues);
	m_pIni->WriteUInt(_T("ImageCopy"),_T("CleanTimes"),m_ComboBoxCleanTimes.GetCurSel() + 1);

	m_pIni->WriteInt(_T("ImageCopy"),_T("CompareCleanSeq"),m_nCompareCleanSeqIndex);

	CDialogEx::OnOK();
}


void CImageCopySetting::OnBnClickedRadioDiskImage()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	GetDlgItem(IDC_CHECK_CLEAN_DISK_FIRST)->EnableWindow(TRUE);

	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(m_bCleanDiskFirst);
	m_ComboBoxCleanTimes.EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(m_bCleanDiskFirst);

	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCheckCompare);
}


void CImageCopySetting::OnBnClickedRadioMtpImage()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	GetDlgItem(IDC_CHECK_CLEAN_DISK_FIRST)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(FALSE);

	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(FALSE);
	m_ComboBoxCleanTimes.EnableWindow(FALSE);

	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(FALSE);

	GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(FALSE);
	GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(FALSE);

	if (m_bCheckCompare)
	{
		m_nCompareMethodIndex = 0;
	}
	
	UpdateData(FALSE);
}


void CImageCopySetting::OnBnClickedCheckCleanDisk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
	m_ComboBoxCleanTimes.EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(m_bCleanDiskFirst);
}


void CImageCopySetting::OnCbnSelchangeComboCleanTimes()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelectIndex = m_ComboBoxCleanTimes.GetCurSel();

	switch (nSelectIndex)
	{
	case 0:
		m_strFillValues = _T("00");
		break;

	case 1:
		m_strFillValues = _T("FF;00");
		break;

	case 2:
		m_strFillValues = _T("00;FF;XX");
		break;
	}

	UpdateData(FALSE);
}


void CImageCopySetting::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bCheckCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bCheckCompare);
}
