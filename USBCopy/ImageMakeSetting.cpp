// ImageMakeSetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageMakeSetting.h"
#include "afxdialogex.h"
#include "Utils.h"

// CImageMakeSetting 对话框

IMPLEMENT_DYNAMIC(CImageMakeSetting, CDialogEx)

CImageMakeSetting::CImageMakeSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageMakeSetting::IDD, pParent)
	, m_nRadioSelectIndex(0)
	, m_bCheckSupportMutiMBR(FALSE)
	, m_nRadioSaveModeIndex(0)
	, m_bCheckDataCompress(FALSE)
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
	DDX_Radio(pDX, IDC_RADIO_FULL, m_nRadioSaveModeIndex);
	DDX_Check(pDX,IDC_CHECK_COMPRESS,m_bCheckDataCompress);
	DDX_Control(pDX,IDC_LIST_CUSTOM_AREA,m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CImageMakeSetting, CDialogEx)
	ON_BN_CLICKED(IDC_CHECK_MUTI_MBR, &CImageMakeSetting::OnBnClickedCheckMutiMbr)
	ON_BN_CLICKED(IDOK, &CImageMakeSetting::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_ADD, &CImageMakeSetting::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CImageMakeSetting::OnBnClickedBtnRemove)
	ON_BN_CLICKED(IDC_RADIO_FULL, &CImageMakeSetting::OnBnClickedRadioFull)
	ON_BN_CLICKED(IDC_RADIO_QUICK, &CImageMakeSetting::OnBnClickedRadioQuick)
	ON_BN_CLICKED(IDC_RADIO_MTP, &CImageMakeSetting::OnBnClickedRadioMtp)
	ON_BN_CLICKED(IDC_RADIO_FILE, &CImageMakeSetting::OnBnClickedRadioFile)
END_MESSAGE_MAP()


// CImageMakeSetting 消息处理程序


BOOL CImageMakeSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	m_nRadioSelectIndex = m_pIni->GetUInt(_T("ImageMake"),_T("SavePath"),0);
	m_nRadioSaveModeIndex = m_pIni->GetUInt(_T("ImageMake"),_T("SaveMode"),0);
	m_bCheckDataCompress = m_pIni->GetBool(_T("ImageMake"),_T("En_DataCompress"),TRUE);

	if (m_nRadioSaveModeIndex != QUICK_IMAGE)
	{
		GetDlgItem(IDC_EDIT_START_LBA)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_END_LBA)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_ADD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(FALSE);
	}

#ifndef SD_TF
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

	InitialListCtrl();

	UpdateData(FALSE);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CImageMakeSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CImageMakeSetting::OnBnClickedCheckMutiMbr()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_ComboBoxMBRLBA.EnableWindow(m_bCheckSupportMutiMBR);
}


void CImageMakeSetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_pIni->WriteUInt(_T("ImageMake"),_T("SavePath"),m_nRadioSelectIndex);
	m_pIni->WriteUInt(_T("ImageMake"),_T("SaveMode"),m_nRadioSaveModeIndex);
	m_pIni->WriteBool(_T("ImageMake"),_T("En_DataCompress"),m_bCheckDataCompress);

	// 先删除
	int nNumOfArea = m_pIni->GetInt(_T("ImageMake"),_T("NumOfArea"),0);

	CString strKey,strStartLBA,strEndLBA;
	for (int i = 0; i < nNumOfArea;i++)
	{
		strKey.Format(_T("StartLBA_%d"),i);

		m_pIni->DeleteKey(_T("ImageMake"),strKey);

		strKey.Format(_T("EndLBA_%d"),i);

		m_pIni->DeleteKey(_T("ImageMake"),strKey);
	}

	// 再添加
	nNumOfArea = m_ListCtrl.GetItemCount();
	m_pIni->WriteInt(_T("ImageMake"),_T("NumOfArea"),nNumOfArea);
	for (int i = 0; i < nNumOfArea;i++)
	{
		strStartLBA = m_ListCtrl.GetItemText(i,0);
		strEndLBA = m_ListCtrl.GetItemText(i,1);

		strKey.Format(_T("StartLBA_%d"),i);
		m_pIni->WriteString(_T("ImageMake"),strKey,strStartLBA);

		strKey.Format(_T("EndLBA_%d"),i);
		m_pIni->WriteString(_T("ImageMake"),strKey,strEndLBA);
	}

#ifndef SD_TF
	UINT nMBRLastLBA = GetDlgItemInt(IDC_COMBO_MUTI_MBR);
	m_pIni->WriteBool(_T("ImageMake"),_T("En_MutiMBR"),m_bCheckSupportMutiMBR);
	m_pIni->WriteUInt(_T("ImageMake"),_T("MBRLastLBA"),nMBRLastLBA);
#endif

	CDialogEx::OnOK();
}

void CImageMakeSetting::InitialListCtrl()
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
	int nNumOfArea = m_pIni->GetInt(_T("ImageMake"),_T("NumOfArea"),0);

	CString strStartLBA,strEndLBA,strKey,strSize;
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


void CImageMakeSetting::OnBnClickedBtnAdd()
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


void CImageMakeSetting::OnBnClickedBtnRemove()
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


void CImageMakeSetting::OnBnClickedRadioFull()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_EDIT_START_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_END_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(FALSE);
}


void CImageMakeSetting::OnBnClickedRadioQuick()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_EDIT_START_LBA)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_END_LBA)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(TRUE);

}


void CImageMakeSetting::OnBnClickedRadioMtp()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_EDIT_START_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_END_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(FALSE);
}


void CImageMakeSetting::OnBnClickedRadioFile()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_EDIT_START_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_END_LBA)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_ADD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_REMOVE)->EnableWindow(FALSE);
}
