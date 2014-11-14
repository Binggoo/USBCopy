// FullCopySettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "FullCopySettingDlg.h"
#include "afxdialogex.h"


// CFullCopySettingDlg 对话框

IMPLEMENT_DYNAMIC(CFullCopySettingDlg, CDialogEx)

CFullCopySettingDlg::CFullCopySettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFullCopySettingDlg::IDD, pParent)
	, m_bComputeHash(FALSE)
	, m_bCompare(FALSE)
	, m_bAllowCapGap(FALSE)
	, m_bCleanDiskFirst(FALSE)
	, m_strFillValues(_T(""))
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
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bCompare);
	DDX_Check(pDX, IDC_CHECK_CAPA_GAP, m_bAllowCapGap);
	DDX_Check(pDX, IDC_CHECK_CLEAN_DISK, m_bCleanDiskFirst);
	DDX_Control(pDX,IDC_COMBO_CAP_GAP,m_ComboBoxCapGap);
	DDX_Control(pDX, IDC_COMBO_CLEAN_TIMES, m_ComboBoxCleanTimes);
	DDX_Text(pDX, IDC_EDIT_FILL_VALUE, m_strFillValues);
}


BEGIN_MESSAGE_MAP(CFullCopySettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CFullCopySettingDlg::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CFullCopySettingDlg::OnBnClickedCheckComputeHash)
	ON_BN_CLICKED(IDOK, &CFullCopySettingDlg::OnBnClickedOk)
	ON_CBN_EDITCHANGE(IDC_COMBO_CAP_GAP, &CFullCopySettingDlg::OnCbnEditchangeComboCapGap)
	ON_BN_CLICKED(IDC_CHECK_CAPA_GAP, &CFullCopySettingDlg::OnBnClickedCheckCapaGap)
	ON_BN_CLICKED(IDC_CHECK_CLEAN_DISK, &CFullCopySettingDlg::OnBnClickedCheckCleanDisk)
	ON_CBN_SELCHANGE(IDC_COMBO_CLEAN_TIMES, &CFullCopySettingDlg::OnCbnSelchangeComboCleanTimes)
END_MESSAGE_MAP()

void CFullCopySettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


// CFullCopySettingDlg 消息处理程序


BOOL CFullCopySettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("FullCopy"),_T("En_ComputeHash"),FALSE);
	m_bCompare = m_pIni->GetBool(_T("FullCopy"),_T("En_Compare"),FALSE);

	if (!m_bComputeHash)
	{
		m_bCompare = FALSE;
	}

	m_bAllowCapGap = m_pIni->GetBool(_T("FullCopy"),_T("En_AllowCapaGap"),FALSE);
	m_bCleanDiskFirst = m_pIni->GetBool(_T("FullCopy"),_T("En_CleanDiskFirst"),FALSE);

	m_ComboBoxCleanTimes.AddString(_T("1"));
	m_ComboBoxCleanTimes.AddString(_T("2"));
	m_ComboBoxCleanTimes.AddString(_T("3"));

	GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);

	m_strFillValues = m_pIni->GetString(_T("FullCopy"),_T("FillValues"));
	UINT nCleanTimes = m_pIni->GetUInt(_T("FullCopy"),_T("CleanTimes"),1);

	if (nCleanTimes < 1 || nCleanTimes > 3)
	{
		nCleanTimes = 1;
	}

	m_ComboBoxCleanTimes.SetCurSel(nCleanTimes - 1);

	m_ComboBoxCapGap.AddString(_T("1"));
	m_ComboBoxCapGap.AddString(_T("2"));
	m_ComboBoxCapGap.AddString(_T("3"));
	m_ComboBoxCapGap.AddString(_T("4"));
	m_ComboBoxCapGap.AddString(_T("5"));
	m_ComboBoxCapGap.AddString(_T("6"));
	m_ComboBoxCapGap.AddString(_T("7"));
	m_ComboBoxCapGap.AddString(_T("8"));
	m_ComboBoxCapGap.AddString(_T("9"));
	m_ComboBoxCapGap.AddString(_T("10"));

	CString strCapGap = m_pIni->GetString(_T("FullCopy"),_T("CapacityGap"));

	m_ComboBoxCapGap.SetWindowText(strCapGap);
	m_ComboBoxCapGap.EnableWindow(m_bAllowCapGap);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CFullCopySettingDlg::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_bCompare)
	{
		m_bComputeHash = TRUE;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedCheckComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		m_bCompare = m_bComputeHash;
		UpdateData(FALSE);
	}
}


void CFullCopySettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	CString strCapGap;
	m_ComboBoxCapGap.GetWindowText(strCapGap);

	m_pIni->WriteBool(_T("FullCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("FullCopy"),_T("En_Compare"),m_bCompare);
	m_pIni->WriteBool(_T("FullCopy"),_T("En_AllowCapaGap"),m_bAllowCapGap);
	m_pIni->WriteString(_T("FullCopy"),_T("CapacityGap"),strCapGap);
	m_pIni->WriteBool(_T("FullCopy"),_T("En_CleanDiskFirst"),m_bCleanDiskFirst);
	m_pIni->WriteUInt(_T("FullCopy"),_T("CleanTimes"),m_ComboBoxCleanTimes.GetCurSel() + 1);
	m_pIni->WriteString(_T("FullCopy"),_T("FillValues"),m_strFillValues);

	CDialogEx::OnOK();
}


void CFullCopySettingDlg::OnCbnEditchangeComboCapGap()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	m_ComboBoxCapGap.GetWindowText(strText);

	if (strText.IsEmpty())
	{
		return;
	}

	TCHAR ch = strText.GetAt(strText.GetLength()-1);
	if (!_istdigit(ch))
	{
		m_ComboBoxCapGap.SetWindowText(_T(""));
	}
}


void CFullCopySettingDlg::OnBnClickedCheckCapaGap()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_ComboBoxCapGap.EnableWindow(m_bAllowCapGap);
}


void CFullCopySettingDlg::OnBnClickedCheckCleanDisk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
}


void CFullCopySettingDlg::OnCbnSelchangeComboCleanTimes()
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
