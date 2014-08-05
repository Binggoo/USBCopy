// DiskCleanSettingDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "DiskCleanSettingDlg.h"
#include "afxdialogex.h"
#include "GlobalDef.h"

// CDiskCleanSettingDlg �Ի���

IMPLEMENT_DYNAMIC(CDiskCleanSettingDlg, CDialogEx)

CDiskCleanSettingDlg::CDiskCleanSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDiskCleanSettingDlg::IDD, pParent)
	, m_nRadioSelectIndex(-1)
{
	m_pIni = NULL;
}

CDiskCleanSettingDlg::~CDiskCleanSettingDlg()
{
}

void CDiskCleanSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_FULL_CLEAN_VALUE, m_ComboBoxFullCleanValue);
	DDX_Radio(pDX, IDC_RADIO_FULL_CLEAN, m_nRadioSelectIndex);
}


BEGIN_MESSAGE_MAP(CDiskCleanSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_RADIO_FULL_CLEAN, &CDiskCleanSettingDlg::OnBnClickedRadioFullClean)
	ON_BN_CLICKED(IDOK, &CDiskCleanSettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_QUICK_CLEAN, &CDiskCleanSettingDlg::OnBnClickedRadioQuickClean)
	ON_BN_CLICKED(IDC_RADIO_SAFE_CLEAN, &CDiskCleanSettingDlg::OnBnClickedRadioSafeClean)
END_MESSAGE_MAP()

void CDiskCleanSettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


// CDiskCleanSettingDlg ��Ϣ�������


BOOL CDiskCleanSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pIni);

	m_nRadioSelectIndex = m_pIni->GetInt(_T("DiskClean"),_T("CleanMode"),0);

	m_ComboBoxFullCleanValue.AddString(_T("00"));
	m_ComboBoxFullCleanValue.AddString(_T("FF"));
	m_ComboBoxFullCleanValue.AddString(_T("XX"));

	int nFillValue = m_pIni->GetInt(_T("DiskClean"),_T("FillValue"),0,16);
	CString strFileValue = m_pIni->GetString(_T("DiskClean"),_T("FillValue"));
	if (strFileValue.IsEmpty())
	{
		strFileValue = _T("00");
	}

	if (nFillValue == RANDOM_VALUE)
	{
		m_ComboBoxFullCleanValue.SetCurSel(2);
	}
	else
	{
		m_ComboBoxFullCleanValue.SetWindowText(strFileValue);
	}

	if (m_nRadioSelectIndex != (int)CleanMode_Full)
	{
		m_ComboBoxFullCleanValue.EnableWindow(FALSE);
	}
	else
	{
		m_ComboBoxFullCleanValue.EnableWindow(TRUE);
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


void CDiskCleanSettingDlg::OnBnClickedRadioFullClean()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	m_ComboBoxFullCleanValue.EnableWindow(TRUE);
}



void CDiskCleanSettingDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(TRUE);
	CString strFillValue;
	m_ComboBoxFullCleanValue.GetWindowText(strFillValue);

	int nFillValue = _tcstoul(strFillValue, NULL, 16);

	if (nFillValue == 0 && strFillValue.GetAt(0) != _T('0'))
	{
		nFillValue = RANDOM_VALUE;
	}

	m_pIni->WriteInt(_T("DiskClean"),_T("CleanMode"),m_nRadioSelectIndex);
	m_pIni->WriteInt(_T("DiskClean"),_T("FillValue"),nFillValue,16);
	
	CDialogEx::OnOK();
}


void CDiskCleanSettingDlg::OnBnClickedRadioQuickClean()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_ComboBoxFullCleanValue.EnableWindow(FALSE);
}


void CDiskCleanSettingDlg::OnBnClickedRadioSafeClean()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_ComboBoxFullCleanValue.EnableWindow(FALSE);
}
