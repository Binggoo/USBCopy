// QuickCopySettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "QuickCopySettingDlg.h"
#include "afxdialogex.h"
#include "Utils.h"

// CQuickCopySettingDlg 对话框

IMPLEMENT_DYNAMIC(CQuickCopySettingDlg, CDialogEx)

CQuickCopySettingDlg::CQuickCopySettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CQuickCopySettingDlg::IDD, pParent)
	, m_bEnCompare(FALSE)
	, m_bEnMutiMBRSupported(FALSE)
	, m_bComputeHash(FALSE)
	, m_bCleanDiskFirst(FALSE)
	, m_strFillValues(_T(""))
	, m_nCompareMethodIndex(0)
	, m_bCompareClean(FALSE)
	, m_nCompareCleanSeqIndex(0)
{
	m_pIni = NULL;
}

CQuickCopySettingDlg::~CQuickCopySettingDlg()
{
}

void CQuickCopySettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_COMPARE, m_bEnCompare);
	DDX_Check(pDX, IDC_CHECK_MUTI_MBR, m_bEnMutiMBRSupported);
	DDX_Control(pDX, IDC_COMBO_MUTI_MBR, m_ComboBox);
	DDX_Check(pDX, IDC_CHECK_COMPUTE_HASH, m_bComputeHash);
	DDX_Check(pDX, IDC_CHECK_CLEAN_DISK, m_bCleanDiskFirst);
	DDX_Control(pDX, IDC_COMBO_CLEAN_TIMES, m_ComboBoxCleanTimes);
	DDX_Text(pDX, IDC_EDIT_FILL_VALUE, m_strFillValues);
	DDX_Radio(pDX,IDC_RADIO_HASH_COMPARE,m_nCompareMethodIndex);
	DDX_Check(pDX,IDC_CHECK_CLEAN_COMPARE,m_bCompareClean);
	DDX_Radio(pDX,IDC_RADIO_CLEAN_IN,m_nCompareCleanSeqIndex);
	DDX_Control(pDX,IDC_LIST_CUSTOM_AREA,m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CQuickCopySettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_MUTI_MBR, &CQuickCopySettingDlg::OnBnClickedCheckMutiMbr)
	ON_BN_CLICKED(IDOK, &CQuickCopySettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_COMPUTE_HASH, &CQuickCopySettingDlg::OnBnClickedCheckQsComputeHash)
	ON_BN_CLICKED(IDC_CHECK_COMPARE, &CQuickCopySettingDlg::OnBnClickedCheckCompare)
	ON_BN_CLICKED(IDC_CHECK_CLEAN_DISK, &CQuickCopySettingDlg::OnBnClickedCheckCleanDisk)
	ON_CBN_SELCHANGE(IDC_COMBO_CLEAN_TIMES, &CQuickCopySettingDlg::OnCbnSelchangeComboCleanTimes)
	ON_BN_CLICKED(IDC_RADIO_HASH_COMPARE, &CQuickCopySettingDlg::OnBnClickedRadioHashCompare)
	ON_BN_CLICKED(IDC_BTN_ADD, &CQuickCopySettingDlg::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CQuickCopySettingDlg::OnBnClickedBtnRemove)
END_MESSAGE_MAP()


// CQuickCopySettingDlg 消息处理程序


BOOL CQuickCopySettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_bComputeHash = m_pIni->GetBool(_T("QuickCopy"),_T("En_ComputeHash"),FALSE);
	m_bEnCompare = m_pIni->GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE);
	m_nCompareMethodIndex = m_pIni->GetInt(_T("QuickCopy"),_T("CompareMethod"),FALSE);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bEnCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bEnCompare);

	if (!m_bComputeHash)
	{
		if (m_bEnCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}

	m_bCleanDiskFirst = m_pIni->GetBool(_T("QuickCopy"),_T("En_CleanDiskFirst"),FALSE);
	m_bCompareClean = m_pIni->GetBool(_T("QuickCopy"),_T("En_CompareClean"),FALSE);
	m_nCompareCleanSeqIndex = m_pIni->GetInt(_T("QuickCopy"),_T("CompareCleanSeq"),0);

	m_ComboBoxCleanTimes.AddString(_T("1"));
	m_ComboBoxCleanTimes.AddString(_T("2"));
	m_ComboBoxCleanTimes.AddString(_T("3"));

	GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(m_bCleanDiskFirst);

	m_strFillValues = m_pIni->GetString(_T("QuickCopy"),_T("FillValues"));
	UINT nCleanTimes = m_pIni->GetUInt(_T("QuickCopy"),_T("CleanTimes"),1);

	if (nCleanTimes < 1 || nCleanTimes > 3)
	{
		nCleanTimes = 1;
	}

	m_ComboBoxCleanTimes.SetCurSel(nCleanTimes - 1);

#ifndef SD_TF
	m_bEnMutiMBRSupported = m_pIni->GetBool(_T("QuickCopy"),_T("En_MutiMBR"),FALSE);
	UINT nMBRLastLBA = m_pIni->GetUInt(_T("QuickCopy"),_T("MBRLastLBA"),5);

	m_ComboBox.AddString(_T("5"));
	m_ComboBox.AddString(_T("10"));
	m_ComboBox.AddString(_T("62"));

	SetDlgItemInt(IDC_COMBO_MUTI_MBR,nMBRLastLBA);
	m_ComboBox.EnableWindow(m_bEnMutiMBRSupported);

#else
	m_ComboBox.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_MUTI_MBR)->ShowWindow(SW_HIDE);
#endif
	
	InitialListCtrl();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CQuickCopySettingDlg::OnBnClickedCheckMutiMbr()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_ComboBox.EnableWindow(m_bEnMutiMBRSupported);
}


void CQuickCopySettingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

#ifndef SD_TF
	UINT nMBRLastLBA = GetDlgItemInt(IDC_COMBO_MUTI_MBR);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_MutiMBR"),m_bEnMutiMBRSupported);
	m_pIni->WriteUInt(_T("QuickCopy"),_T("MBRLastLBA"),nMBRLastLBA);
#endif

	m_pIni->WriteBool(_T("QuickCopy"),_T("En_ComputeHash"),m_bComputeHash);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_Compare"),m_bEnCompare);
	m_pIni->WriteInt(_T("QuickCopy"),_T("CompareMethod"),m_nCompareMethodIndex);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_CleanDiskFirst"),m_bCleanDiskFirst);
	m_pIni->WriteBool(_T("QuickCopy"),_T("En_CompareClean"),m_bCompareClean);
	m_pIni->WriteUInt(_T("QuickCopy"),_T("CleanTimes"),m_ComboBoxCleanTimes.GetCurSel() + 1);
	m_pIni->WriteString(_T("QuickCopy"),_T("FillValues"),m_strFillValues);
	m_pIni->WriteInt(_T("QuickCopy"),_T("CompareCleanSeq"),m_nCompareCleanSeqIndex);

	// 先删除
	int nNumOfArea = m_pIni->GetInt(_T("QuickCopy"),_T("NumOfArea"),0);

	CString strKey,strStartLBA,strEndLBA;
	for (int i = 0; i < nNumOfArea;i++)
	{
		strKey.Format(_T("StartLBA_%d"),i);

		m_pIni->DeleteKey(_T("QuickCopy"),strKey);

		strKey.Format(_T("EndLBA_%d"),i);

		m_pIni->DeleteKey(_T("QuickCopy"),strKey);
	}

	// 再添加
	nNumOfArea = m_ListCtrl.GetItemCount();
	m_pIni->WriteInt(_T("QuickCopy"),_T("NumOfArea"),nNumOfArea);
	for (int i = 0; i < nNumOfArea;i++)
	{
		strStartLBA = m_ListCtrl.GetItemText(i,0);
		strEndLBA = m_ListCtrl.GetItemText(i,1);

		strKey.Format(_T("StartLBA_%d"),i);
		m_pIni->WriteString(_T("QuickCopy"),strKey,strStartLBA);

		strKey.Format(_T("EndLBA_%d"),i);
		m_pIni->WriteString(_T("QuickCopy"),strKey,strEndLBA);
	}
	
	CDialogEx::OnOK();
}

void CQuickCopySettingDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CQuickCopySettingDlg::OnBnClickedCheckQsComputeHash()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (!m_bComputeHash)
	{
		if (m_bEnCompare)
		{
			m_nCompareMethodIndex = 1;
		}
	}

	UpdateData(FALSE);
}


void CQuickCopySettingDlg::OnBnClickedCheckCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	GetDlgItem(IDC_RADIO_HASH_COMPARE)->EnableWindow(m_bEnCompare);
	GetDlgItem(IDC_RADIO_BYTE_COMPARE)->EnableWindow(m_bEnCompare);
}


void CQuickCopySettingDlg::OnBnClickedCheckCleanDisk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	GetDlgItem(IDC_COMBO_CLEAN_TIMES)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_EDIT_FILL_VALUE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_CHECK_CLEAN_COMPARE)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_IN)->EnableWindow(m_bCleanDiskFirst);
	GetDlgItem(IDC_RADIO_CLEAN_AFTER)->EnableWindow(m_bCleanDiskFirst);
}


void CQuickCopySettingDlg::OnCbnSelchangeComboCleanTimes()
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


void CQuickCopySettingDlg::OnBnClickedRadioHashCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_bComputeHash = TRUE;

	UpdateData(FALSE);
}

void CQuickCopySettingDlg::InitialListCtrl()
{
	CRect rect;
	m_ListCtrl.GetClientRect(&rect);

	int nItem = 0;
	CString strRes;

	strRes.LoadString(IDS_START_LBA);
	m_ListCtrl.InsertColumn(nItem++,strRes,LVCFMT_LEFT,rect.Width()/3,0);

	strRes.LoadString(IDS_END_LBA);
	m_ListCtrl.InsertColumn(nItem++,strRes,LVCFMT_LEFT,rect.Width()/3,0);

	strRes.LoadString(IDS_ITEM_SIZE);
	m_ListCtrl.InsertColumn(nItem++,strRes,LVCFMT_LEFT,rect.Width()/3,0);

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 插入数据
	int nNumOfArea = m_pIni->GetInt(_T("QuickCopy"),_T("NumOfArea"),0);

	CString strStartLBA,strEndLBA,strSize,strKey;
	ULONGLONG ullStartLBA = 0,ullEndLBA = 0;

	for (int i = 0; i < nNumOfArea;i++)
	{
		strKey.Format(_T("StartLBA_%d"),i);

		ullStartLBA = m_pIni->GetUInt64(_T("QuickCopy"),strKey,0);

		strStartLBA.Format(_T("%I64d"),ullStartLBA);

		m_ListCtrl.InsertItem(i,strStartLBA);

		strKey.Format(_T("EndLBA_%d"),i);

		ullEndLBA = m_pIni->GetUInt64(_T("QuickCopy"),strKey,0);

		strEndLBA.Format(_T("%I64d"),ullEndLBA);

		m_ListCtrl.SetItemText(i,1,strEndLBA);

		strSize = CUtils::AdjustFileSize((ullEndLBA - ullStartLBA) * BYTES_PER_SECTOR);

		m_ListCtrl.SetItemText(i,2,strSize);

	}
}


void CQuickCopySettingDlg::OnBnClickedBtnAdd()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strStartLBA,strEndLBA,strSize;

	GetDlgItemText(IDC_EDIT_START_LBA,strStartLBA);
	GetDlgItemText(IDC_EDIT_END_LBA,strEndLBA);

	strStartLBA.Trim();
	strEndLBA.Trim();

	ULONGLONG ullStartLBA = (ULONGLONG)_ttoi64(strStartLBA);
	ULONGLONG ullEndLBA = (ULONGLONG)_ttoi64(strEndLBA);

	if (ullEndLBA <= ullStartLBA)
	{
		return;
	}

	if (strStartLBA.IsEmpty() || strEndLBA.IsEmpty())
	{
		return;
	}

	strSize = CUtils::AdjustFileSize((ullEndLBA - ullStartLBA) * BYTES_PER_SECTOR);

	int nCount = m_ListCtrl.GetItemCount();

	m_ListCtrl.InsertItem(nCount,strStartLBA);
	m_ListCtrl.SetItemText(nCount,1,strEndLBA);
	m_ListCtrl.SetItemText(nCount,2,strSize);
}


void CQuickCopySettingDlg::OnBnClickedBtnRemove()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);
		m_ListCtrl.DeleteItem(nItem);
		pos = m_ListCtrl.GetFirstSelectedItemPosition();
	}
}
