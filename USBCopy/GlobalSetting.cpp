// GlobalSetting.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "GlobalSetting.h"
#include "afxdialogex.h"


// CGlobalSetting 对话框

IMPLEMENT_DYNAMIC(CGlobalSetting, CDialogEx)

CGlobalSetting::CGlobalSetting(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGlobalSetting::IDD, pParent)
	, m_bCheckRelativeSpeed(FALSE)
	, m_nRelativeSpeed(0)
	, m_bCheckAbsoluteSpeed(FALSE)
	, m_nAbsolteSpeed(0)
	, m_bCheckUploadLog(FALSE)
	, m_iHashMethod(0)
	, m_strEditAlias(_T(""))
	, m_nListenPort(0)
	, m_bCheckShowCursor(FALSE)
	, m_bCheckBeep(FALSE)
	, m_strEditServerIP(_T(""))
{
	m_pIni = NULL;
}

CGlobalSetting::~CGlobalSetting()
{
}

void CGlobalSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SCAN_DISK_TIME, m_ComboBoxScanTime);
	DDX_Control(pDX, IDC_COMBO_DELAY_OFF_TIME, m_ComboBoxDelayTime);
	DDX_Check(pDX, IDC_CHECK_RELATIVE_SPEED, m_bCheckRelativeSpeed);
	DDX_Text(pDX, IDC_EDIT_RELATIVE_SPEED, m_nRelativeSpeed);
	DDX_Check(pDX, IDC_CHECK_ABSOLUTE_SPEED, m_bCheckAbsoluteSpeed);
	DDX_Text(pDX, IDC_EDIT_ABSOLUTE_SPEED, m_nAbsolteSpeed);
	DDX_Check(pDX, IDC_CHECK_UPLOAD_LOG, m_bCheckUploadLog);
	DDX_Radio(pDX, IDC_RADIO_CHECKSUM32, m_iHashMethod);
	DDX_Text(pDX, IDC_EDIT_ALIAS, m_strEditAlias);
	DDX_Text(pDX, IDC_EDIT_LISTEN_PORT, m_nListenPort);
	DDX_Check(pDX, IDC_CHECK_SHOW_CURSOR, m_bCheckShowCursor);
	DDX_Check(pDX, IDC_CHECK_BEEP, m_bCheckBeep);
	DDX_Text(pDX, IDC_EDIT_SERVER_IP, m_strEditServerIP);
}


BEGIN_MESSAGE_MAP(CGlobalSetting, CDialogEx)
	ON_BN_CLICKED(IDOK, &CGlobalSetting::OnBnClickedOk)
	ON_CBN_EDITCHANGE(IDC_COMBO_SCAN_DISK_TIME, &CGlobalSetting::OnCbnEditchangeComboScanDiskTime)
	ON_CBN_EDITCHANGE(IDC_COMBO_DELAY_OFF_TIME, &CGlobalSetting::OnCbnEditchangeComboDelayOffTime)
END_MESSAGE_MAP()


// CGlobalSetting 消息处理程序


BOOL CGlobalSetting::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_ComboBoxScanTime.AddString(_T("0"));
	m_ComboBoxScanTime.AddString(_T("10"));
	m_ComboBoxScanTime.AddString(_T("20"));
	m_ComboBoxScanTime.AddString(_T("30"));
	m_ComboBoxScanTime.AddString(_T("60"));
	m_ComboBoxScanTime.AddString(_T("90"));
	m_ComboBoxScanTime.AddString(_T("120"));

	m_ComboBoxDelayTime.AddString(_T("0"));
	m_ComboBoxDelayTime.AddString(_T("5"));
	m_ComboBoxDelayTime.AddString(_T("10"));
	m_ComboBoxDelayTime.AddString(_T("20"));
	m_ComboBoxDelayTime.AddString(_T("30"));

	CString strScanTime = m_pIni->GetString(_T("Option"),_T("ScanDiskTimeS"),_T("30"));
	CString strDelayTime = m_pIni->GetString(_T("Option"),_T("SpinDownTimeS"),_T("5"));
	m_ComboBoxScanTime.SetWindowText(strScanTime);
	m_ComboBoxDelayTime.SetWindowText(strDelayTime);

	m_bCheckShowCursor = m_pIni->GetBool(_T("Option"),_T("ShowCursor"),TRUE);
	m_bCheckBeep = m_pIni->GetBool(_T("Option"),_T("BeepWhenFinish"),TRUE);

	m_bCheckRelativeSpeed = m_pIni->GetBool(_T("Option"),_T("En_RelativeSpeed"),FALSE);
	m_nRelativeSpeed = m_pIni->GetUInt(_T("Option"),_T("RelativeSpeed"),60);

	m_bCheckAbsoluteSpeed = m_pIni->GetBool(_T("Option"),_T("En_AbsoluteSpeed"),FALSE);
	m_nAbsolteSpeed = m_pIni->GetUInt(_T("Option"),_T("AbsoluteSpeed"),5);

	m_bCheckUploadLog = m_pIni->GetBool(_T("Option"),_T("En_UploadLogAuto"),FALSE);
	m_strEditServerIP = m_pIni->GetString(_T("RemoteServer"),_T("ServerIP"),_T("192.168.0.1"));
	m_nListenPort = m_pIni->GetUInt(_T("RemoteServer"),_T("ListenPort"),7788);

	m_iHashMethod = m_pIni->GetInt(_T("Option"),_T("HashMethod"),0);

	m_strEditAlias = m_pIni->GetString(_T("Option"),_T("MachineAlias"),_T("PHIYO"));

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CGlobalSetting::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CGlobalSetting::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	CString strScanTime,strDelayTime;
	m_ComboBoxScanTime.GetWindowText(strScanTime);
	m_ComboBoxDelayTime.GetWindowText(strDelayTime);

	m_pIni->WriteString(_T("Option"),_T("ScanDiskTimeS"),strScanTime);
	m_pIni->WriteString(_T("Option"),_T("SpinDownTimeS"),strDelayTime);
	

	m_pIni->WriteBool(_T("Option"),_T("ShowCursor"),m_bCheckShowCursor);
	m_pIni->WriteBool(_T("Option"),_T("BeepWhenFinish"),m_bCheckBeep);

	// 显示光标执行一遍，不显示光标执行2遍
	ShowCursor(m_bCheckShowCursor);
	if (!m_bCheckShowCursor)
	{
		ShowCursor(m_bCheckShowCursor);
	}

	m_pIni->WriteBool(_T("Option"),_T("En_RelativeSpeed"),m_bCheckRelativeSpeed);
	m_pIni->WriteUInt(_T("Option"),_T("RelativeSpeed"),m_nRelativeSpeed);

	m_pIni->WriteBool(_T("Option"),_T("En_AbsoluteSpeed"),m_bCheckAbsoluteSpeed);
	m_pIni->WriteUInt(_T("Option"),_T("AbsoluteSpeed"),m_nAbsolteSpeed);

	m_pIni->WriteBool(_T("Option"),_T("En_UploadLogAuto"),m_bCheckUploadLog);
	m_pIni->WriteString(_T("RemoteServer"),_T("ServerIP"),m_strEditServerIP);
	m_pIni->WriteUInt(_T("RemoteServer"),_T("ListenPort"),m_nListenPort);

	m_pIni->WriteInt(_T("Option"),_T("HashMethod"),m_iHashMethod);

	m_pIni->WriteString(_T("Option"),_T("MachineAlias"),m_strEditAlias);

	CDialogEx::OnOK();
}


void CGlobalSetting::OnCbnEditchangeComboScanDiskTime()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	m_ComboBoxScanTime.GetWindowText(strText);

	if (strText.IsEmpty())
	{
		return;
	}

	TCHAR ch = strText.GetAt(strText.GetLength()-1);
	if (!_istdigit(ch))
	{
		m_ComboBoxScanTime.SetWindowText(_T(""));
	}
}


void CGlobalSetting::OnCbnEditchangeComboDelayOffTime()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	m_ComboBoxDelayTime.GetWindowText(strText);

	if (strText.IsEmpty())
	{
		return;
	}

	TCHAR ch = strText.GetAt(strText.GetLength()-1);
	if (!_istdigit(ch))
	{
		m_ComboBoxDelayTime.SetWindowText(_T(""));
	}
}
