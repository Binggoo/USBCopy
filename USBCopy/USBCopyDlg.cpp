
// USBCopyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "USBCopy.h"
#include "USBCopyDlg.h"
#include "afxdialogex.h"
#include "Utils.h"
#include "Disk.h"
#include "Password.h"
#include "SystemMenu.h"
#include "SelectCurModeDlg.h"
#include "FullCopySettingDlg.h"
#include "QuickCopySettingDlg.h"
#include "ImageCopySetting.h"
#include "ImageMakeSetting.h"
#include "DiskCleanSettingDlg.h"
#include "DiskCompareSettingDlg.h"
#include "ImageNameDlg.h"
#include "ScanningDlg.h"
#include "CompleteMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CUSBCopyDlg dialog




CUSBCopyDlg::CUSBCopyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUSBCopyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_nPortNum = 18;
	m_bLock = TRUE;
	m_bCancel = FALSE;
	m_bResult = TRUE;
}

void CUSBCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS2, m_ProgressCtrl);
}

BEGIN_MESSAGE_MAP(CUSBCopyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CUSBCopyDlg::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_SYSTEM, &CUSBCopyDlg::OnBnClickedBtnSystem)
	ON_BN_CLICKED(IDC_BTN_WORK_SELECT, &CUSBCopyDlg::OnBnClickedBtnWorkSelect)
	ON_BN_CLICKED(IDC_BTN_SETTING, &CUSBCopyDlg::OnBnClickedBtnSetting)
	ON_BN_CLICKED(IDC_BTN_START, &CUSBCopyDlg::OnBnClickedBtnStart)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_STOP, &CUSBCopyDlg::OnBnClickedBtnStop)
	ON_MESSAGE(WM_UPDATE_STATISTIC, &CUSBCopyDlg::OnUpdateStatistic)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_SEND_FUNCTION_TEXT, &CUSBCopyDlg::OnSendFunctionText)
	ON_MESSAGE(ON_COM_RECEIVE, &CUSBCopyDlg::OnComReceive)
	ON_MESSAGE(WM_RESET_MACHIEN_PORT, &CUSBCopyDlg::OnResetMachienPort)
	ON_MESSAGE(WM_COMPLETE, &CUSBCopyDlg::OnComplete)
END_MESSAGE_MAP()


// CUSBCopyDlg message handlers

BOOL CUSBCopyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	GetClientRect(&m_Rect);
	SetWindowPos(NULL,0,0,800,600,SWP_NOZORDER|SWP_NOMOVE);

	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	CString strLogFile = m_strAppPath + LOG_FILE;
	m_hLogFile = CreateFile(strLogFile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if (m_hLogFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(_T("CreateFile Error"),_T("Error"),MB_ICONERROR);

		SendMessage(WM_CLOSE);
	}

	//添加状态栏
	CString strVersion,strSN,strModel,strAlias;

	CFileStatus status;
	CFile::GetStatus(strPath,status);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T(""));
	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("********************************************************************************"));
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("USBCopy Ver:%s BLD:%s"),CUtils::GetAppVersion(strPath),status.m_mtime.Format(_T("%Y-%m-%d")));

	// 读机器信息
	m_Config.SetPathName(m_strAppPath + MACHINE_INFO);
	strVersion.Format(_T("Suzhou PHIYO Ver:%s  BLD:%s "),CUtils::GetAppVersion(strPath),status.m_mtime.Format(_T("%Y-%m-%d")));
	strSN.Format(_T("SN : %s"),m_Config.GetString(_T("MachineInfo"),_T("SN")));
	strModel.Format(_T("Model : %s"),m_Config.GetString(_T("MachineInfo"),_T("Model")));
	strAlias.Format(_T("%s"),m_Config.GetString(_T("MachineInfo"),_T("Alias"),_T("PHIYO")));

	m_font.CreatePointFont(100,_T("Arial"));
	GetDlgItem(IDC_TEXT_SN)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_MODEL)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_ALIAS)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_COPYRIGHT)->SetFont(&m_font);

	SetDlgItemText(IDC_TEXT_SN,strSN);
	SetDlgItemText(IDC_TEXT_MODEL,strModel);
	SetDlgItemText(IDC_TEXT_ALIAS,strAlias);
	SetDlgItemText(IDC_TEXT_COPYRIGHT,strVersion);

	CString strConfigPath = m_strAppPath + CONFIG_NAME;
	m_Config.SetPathName(strConfigPath);

	// 初始化
	InitialPortPath();

	InitialPortFrame();

	InitialCurrentWorkMode();

	m_ProgressCtrl.ShowPercent(TRUE);
	m_ProgressCtrl.SetRange32(0,100);

	InitialStatisticInfo();

	InitialSerialPort();
	m_Command.Init(&m_SerialPort,m_hLogFile);

	AfxBeginThread((AFX_THREADPROC)InitialMachineThreadProc,this);

	GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CUSBCopyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUSBCopyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CUSBCopyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CUSBCopyDlg::InitialPortFrame()
{
	m_PortFrames = new CPortDlg[m_nPortNum];

	CRect rectFrame;
	GetDlgItem(IDC_PIC_FRAME)->GetWindowRect(&rectFrame);
	ScreenToClient(&rectFrame);

	int nWidth = (rectFrame.Width()-2) / COLUMNS;
	int nHeight = (rectFrame.Height()-2) / ROWS;

	// 母盘
	m_PortFrames[0].SetPort(&m_Command,&m_MasterPort,&m_TargetPorts);

	m_PortFrames[0].Create(IDD_DIALOG_PORT,this);

	m_PortFrames[0].MoveWindow(rectFrame.left + 1,
		rectFrame.top + 1,
		nWidth,
		nHeight);

	m_PortFrames[0].ShowWindow(SW_SHOW);


	// 子盘
	POSITION pos = m_TargetPorts.GetHeadPosition();
	int nItem = 1;
	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);

		m_PortFrames[nItem].SetPort(&m_Command,port,&m_TargetPorts);

		m_PortFrames[nItem].Create(IDD_DIALOG_PORT,this);

		UINT nRow = nItem / COLUMNS;
		UINT nCol = nItem % COLUMNS;

		m_PortFrames[nItem].MoveWindow(rectFrame.left + 1 + nWidth * nCol,
			rectFrame.top + 1 + nHeight * nRow,
			nWidth,
			nHeight);

		m_PortFrames[nItem].ShowWindow(SW_SHOW);

		nItem++;
	}
}

void CUSBCopyDlg::ResetPortFrame()
{
	for (UINT i = 0; i < m_nPortNum;i++)
	{
		m_PortFrames[i].Initial();
	}
}

void CUSBCopyDlg::UpdatePortFrame(BOOL bStart)
{
	for (UINT i = 0;i < m_nPortNum;i++)
	{
		CPort *port = m_PortFrames[i].GetPort();
		
		m_PortFrames[i].Update(bStart);
	}
}

void CUSBCopyDlg::ChangeSize( CWnd *pWnd,int cx, int cy )
{
	if (pWnd)
	{
		CRect rect;
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.left=rect.left*cx/m_Rect.Width();
		rect.right=rect.right*cx/m_Rect.Width();
		rect.top=rect.top*cy/m_Rect.Height();
		rect.bottom=rect.bottom*cy/m_Rect.Height();
		pWnd->MoveWindow(rect);
	}
}



void CUSBCopyDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	CWnd *pWnd = GetWindow(GW_CHILD);
	while (pWnd)
	{
		ChangeSize(pWnd,cx,cy);
		pWnd = pWnd->GetWindow(GW_HWNDNEXT);
	}

	GetClientRect(&m_Rect);
}

void CUSBCopyDlg::InitialPortPath()
{
	// 清空List
	m_TargetPorts.RemoveAll();

	// Master Port
	CString strPath1,strPath2;
	m_MasterPort.SetPortNum(0);
	m_MasterPort.SetPortType(PortType_MASTER_DISK);
	strPath1 = m_Config.GetString(_T("MasterDrives"),_T("M1-1"));
	strPath2 = m_Config.GetString(_T("MasterDrives"),_T("M1-2"));

	m_MasterPort.SetPath1AndIndex(strPath1);

	m_MasterPort.SetPath2AndIndex(strPath2);

	// Target Port
	int nNumOfTargetDrives = m_Config.GetInt(_T("TargetDrives"),_T("NumOfTargetDrives"),0);
	m_nPortNum = nNumOfTargetDrives + 1;

	for (int i = 1;i <= nNumOfTargetDrives;i++)
	{
		CPort *port = new CPort;
		port->SetPortNum(i);
		port->SetPortType(PortType_TARGET_DISK);

		CString strKey1,strKey2;
		strKey1.Format(_T("T%d-1"),i);
		strKey2.Format(_T("T%d-2"),i);

		strPath1 = m_Config.GetString(_T("TargetDrives"),strKey1);
		strPath2 = m_Config.GetString(_T("TargetDrives"),strKey2);

		port->SetPath1AndIndex(strPath1);

		port->SetPath2AndIndex(strPath2);

		m_TargetPorts.AddTail(port);
	}
}

void CUSBCopyDlg::InitialSerialPort()
{
	DWORD dwPortNum = m_Config.GetUInt(_T("SerialPort"),_T("PortNumber"),1);
	DWORD dwBaudRate = m_Config.GetUInt(_T("SerialPort"),_T("BaudRate"),9600);
	BYTE byDataBits = (BYTE)m_Config.GetUInt(_T("SerialPort"),_T("DataBits"),8);
	BYTE byStopBits = (BYTE)m_Config.GetUInt(_T("SerialPort"),_T("StopBits"),1);

	CString strSerialPort;
	strSerialPort.Format(_T("%ld,n,%d,%d"),dwBaudRate,byDataBits,byStopBits);

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] COM%ld,%s"),dwPortNum,strSerialPort);

	m_SerialPort.SetWnd(m_hWnd);

	if (!m_SerialPort.Open(dwPortNum,strSerialPort.GetBuffer()))
	{
		DWORD dwErrorCode = m_SerialPort.GetErrorCode();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] Open COM%ld failed,system errorcode=%ld,%s")
			,dwPortNum,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
	}
	else
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] Open COM%ld success."),dwPortNum);
	}
}

void CUSBCopyDlg::InitialCurrentWorkMode()
{
	m_WorkMode = (WorkMode)m_Config.GetUInt(_T("Option"),_T("FunctionMode"),1);
	BOOL bShowCursor = m_Config.GetBool(_T("Option"),_T("ShowCursor"),TRUE);
	ShowCursor(bShowCursor);

	CString strWorkMode,strWorkModeParm;
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			strWorkMode = strWorkModeParm = _T("Full Copy");

			if (m_Config.GetBool(_T("FullCopy"),_T("En_Compare"),FALSE))
			{
				strWorkModeParm = _T("FullCopy + Compare");
			}
		}
		break;

	case WorkMode_QuickCopy:
		{
			strWorkMode = strWorkModeParm = _T("Quick Copy");

			if (m_Config.GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE))
			{
				strWorkModeParm = _T("QuickCopy + Compare");
			}
		}
		break;

	case WorkMode_ImageCopy:
		{
			strWorkMode = strWorkModeParm = _T("Image Copy");

			if (m_Config.GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE))
			{
				strWorkModeParm = _T("ImageCopy + Compare");
			}
		}
		break;

	case WorkMode_ImageMake:
		strWorkMode = strWorkModeParm =  _T("Image Make");
		break;

	case WorkMode_DiskClean:
		{
			strWorkMode = strWorkModeParm = _T("Disk Clean");

			CleanMode cleanMode = (CleanMode)m_Config.GetInt(_T("DiskClean"),_T("CleanMode"),0);
			switch (cleanMode)
			{
			case CleanMode_Full:
				strWorkModeParm = _T("DiskClean - FullClean");
				break;

			case CleanMode_Quick:
				strWorkModeParm = _T("DiskClean - QuickClean");
				break;

			case CleanMode_Safe:
				strWorkModeParm = _T("DiskClean - SafeClean");
				break;
			}
		}

		break;

	case WorkMode_DiskCompare:
		{
			strWorkMode = strWorkModeParm = _T("Disk Compare");

			CompareMode compareMode = (CompareMode)m_Config.GetInt(_T("DiskCompare"),_T("CompareMode"),0);

			switch (compareMode)
			{
			case CompareMode_Full:
				strWorkModeParm = _T("DiskCompare - FullCompare");
				break;

			case CompareMode_Quick:
				strWorkModeParm = _T("DiskCompare - QuickCompare");
				break;
			}
		}
		break;

	case WorkMode_FileCopy:
		{
			strWorkMode = strWorkModeParm = _T("File Copy");

			if (m_Config.GetBool(_T("FileCopy"),_T("En_Compare"),FALSE))
			{
				strWorkModeParm = _T("FileCopy + Compare");
			}
		}
		break;
	}

	SetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);
	SetDlgItemText(IDC_GROUP_WORK_MODE,strWorkModeParm);
}

void CUSBCopyDlg::InitialStatisticInfo()
{
	SetDlgItemText(IDC_TEXT_FUNCTION2,_T(""));
	SetDlgItemText(IDC_TEXT_SPEED2,_T("0"));
	SetDlgItemText(IDC_TEXT_TIME_ELAPSED2,_T("00:00:00"));
	SetDlgItemText(IDC_TEXT_TIME_REMAINNING2,_T("00:00:00"));
	SetDlgItemText(IDC_TEXT_USAGE,_T(""));
	m_ProgressCtrl.SetPos(0);
}

void CUSBCopyDlg::EnableControls( BOOL bEnable )
{
	if (m_bLock)
	{
		GetDlgItem(IDC_BTN_SYSTEM)->EnableWindow(bEnable);
		GetDlgItem(IDC_BTN_WORK_SELECT)->EnableWindow(bEnable);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(bEnable);
	}
	else
	{
		GetDlgItem(IDC_BTN_SYSTEM)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_WORK_SELECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(FALSE);
	}
}


BOOL CUSBCopyDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_LOCK:
		case IDC_BTN_SYSTEM:
		case IDC_BTN_WORK_SELECT:
		case IDC_BTN_SETTING:
		case IDC_BTN_START:
		case IDC_BTN_STOP:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}
	

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CUSBCopyDlg::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bLock)
	{
		CPassword password(TRUE);
		if (IDOK == password.DoModal())
		{
			m_strPassWord = password.GetPassword();
			m_bLock = FALSE;
			SetDlgItemText(IDC_BTN_LOCK,_T("Unlock"));
			EnableControls(FALSE);
		}

	}
	else
	{
		CPassword password(FALSE);
		password.SetPassword(m_strPassWord);
		if (IDOK == password.DoModal())
		{
			m_bLock = TRUE;
			SetDlgItemText(IDC_BTN_LOCK,_T("Lock"));
			EnableControls(TRUE);
		}
	}
}


void CUSBCopyDlg::OnBnClickedBtnSystem()
{
	// TODO: 在此添加控件通知处理程序代码
	CSystemMenu menu;
	menu.SetConfig(&m_Config);
	menu.DoModal();

	CString strAlias = m_Config.GetString(_T("Option"),_T("MachineAlias"),_T("PHIYO"));
	SetDlgItemText(IDC_TEXT_ALIAS,strAlias);
}


void CUSBCopyDlg::OnBnClickedBtnWorkSelect()
{
	// TODO: 在此添加控件通知处理程序代码
	CSelectCurModeDlg dlg;
	dlg.SetConfig(&m_Config);
	dlg.DoModal();

	InitialCurrentWorkMode();
}


void CUSBCopyDlg::OnBnClickedBtnSetting()
{
	// TODO: 在此添加控件通知处理程序代码
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			CFullCopySettingDlg dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_QuickCopy:
		{
			CQuickCopySettingDlg dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_DiskClean:
		{
			CDiskCleanSettingDlg dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_DiskCompare:
		{
			CDiskCompareSettingDlg dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_ImageMake:
		{
			CImageMakeSetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_ImageCopy:
		{
			CImageCopySetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;
	}

	InitialCurrentWorkMode();
}


void CUSBCopyDlg::OnBnClickedBtnStart()
{
	// TODO: 在此添加控件通知处理程序代码

	switch(m_WorkMode)
	{
	case WorkMode_ImageMake:
		{
			CImageNameDlg dlg(TRUE);
			dlg.SetConfig(&m_Config);
			if (dlg.DoModal() == IDCANCEL)
			{
				return;
			}

		}
		break;

	case WorkMode_ImageCopy:
		{
			CImageNameDlg dlg(FALSE);
			dlg.SetConfig(&m_Config);
			if (dlg.DoModal() == IDCANCEL)
			{
				return;
			}

		}
		break;
	}

	m_bCancel = FALSE;
	m_bResult = TRUE;

	GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);

	EnableControls(FALSE);

	CString strWorkMode;
	GetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);
	CUtils::WriteLogFile(m_hLogFile,FALSE,_T(""));
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("%s begain......"),strWorkMode);

	SetDlgItemText(IDC_TEXT_FUNCTION2,strWorkMode);

	switch (m_WorkMode)
	{
	case WorkMode_ImageMake:
		// 只给母盘上电
		m_Command.Power(0,TRUE);
		break;

	case WorkMode_DiskClean:
	case WorkMode_ImageCopy:
		// 只给子盘上电
		for (UINT i = 1; i < m_nPortNum;i++)
		{
			m_Command.Power(i,TRUE);
		}
		break;

	default:
		// 全部上电
		m_Command.Power(0,TRUE);

		for (UINT i = 1; i < m_nPortNum;i++)
		{
			m_Command.Power(i,TRUE);
		}

		break;

	}

	UpdatePortFrame(TRUE);

	// 使能闪烁命令
	m_Command.WorkLight();

	CScanningDlg dlg;
	dlg.SetLogFile(m_hLogFile);
	dlg.SetConfig(&m_Config);
	dlg.SetDeviceInfoList(&m_MasterPort,&m_TargetPorts);
	dlg.SetBegining(TRUE);
	dlg.DoModal();

	// 判读是否可以拷贝
	if (!IsReady())
	{
		// 全部FAIL
		m_Command.AllFail();

		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_STOP, BN_CLICKED), (LPARAM)m_hWnd); 
		return;
	}

	// 判断端口状态，哪些直接FAIL，哪些可以工作
	POSITION pos = NULL;
	switch (m_WorkMode)
	{
	case WorkMode_ImageMake:
		m_Command.GreenLightFlash(0,TRUE);
		break;

	case WorkMode_DiskClean:
	case WorkMode_ImageCopy:
		pos = m_TargetPorts.GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts.GetNext(pos);
			if (port->IsConnected())
			{
				m_Command.GreenLightFlash(port->GetPortNum(),FALSE);
			}
			else
			{
				m_Command.RedLight(port->GetPortNum(),TRUE,FALSE);
			}
		}
		break;

	default:
		m_Command.GreenLightFlash(0,TRUE);

		pos = m_TargetPorts.GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts.GetNext(pos);
			if (port->IsConnected())
			{
				m_Command.GreenLightFlash(port->GetPortNum(),FALSE);
			}
			else
			{
				m_Command.RedLight(port->GetPortNum(),TRUE,FALSE);
			}
		}

		break;

	}

	AfxBeginThread((AFX_THREADPROC)StartThreadProc,this);
}


void CUSBCopyDlg::OnBnClickedBtnStop()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bCancel = TRUE;

// 	CScanningDlg dlg;
// 	dlg.SetLogFile(m_hLogFile);
// 	dlg.SetConfig(&m_Config);
// 	dlg.SetDeviceInfoList(&m_MasterPort,&m_TargetPorts);
// 	dlg.SetBegining(FALSE);
// 	dlg.DoModal();

	UpdatePortFrame(FALSE);

	GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);

	EnableControls(TRUE);
	GetDlgItem(IDC_BTN_START)->SetFocus();

	PostMessage(WM_COMPLETE);
}


void CUSBCopyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent)
	{
	case TIMER_UPDATE_STATISTIC:
		UpdateStatisticInfo();
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CUSBCopyDlg::UpdateStatisticInfo()
{
	int iMinPercent = m_MasterPort.GetPercent();
	double dbAvgSpeed = 0;
	ULONGLONG ullMinCompleteSize = m_MasterPort.GetCompleteSize();
	ULONGLONG ullMaxValidSize = m_MasterPort.GetValidSize();

	int nIndex = 0;
	if (m_WorkMode == WorkMode_DiskClean || m_WorkMode == WorkMode_ImageCopy)
	{
		iMinPercent = 100;
		ullMinCompleteSize = -1;
		ullMaxValidSize = 0;
	}
	else
	{
		nIndex++;
		dbAvgSpeed += m_MasterPort.GetAvgSpeed();
	}

	if (m_WorkMode != WorkMode_ImageMake)
	{
		POSITION pos = m_TargetPorts.GetHeadPosition();

		while (pos)
		{
			CPort *port = m_TargetPorts.GetNext(pos);

			if (port->IsConnected() && port->GetResult() && port->GetPortState() == PortState_Active)
			{
				if (port->GetPercent() < iMinPercent)
				{
					iMinPercent = port->GetPercent();
				}

				if (port->GetCompleteSize() < ullMinCompleteSize)
				{
					ullMinCompleteSize = port->GetCompleteSize();
				}

				if (port->GetValidSize() > ullMaxValidSize)
				{
					ullMaxValidSize = port->GetValidSize();
				}

				dbAvgSpeed += port->GetAvgSpeed();

				nIndex++;
			}
		}
	}

	if (nIndex > 0)
	{
		dbAvgSpeed /= nIndex;

		m_ProgressCtrl.SetPos(iMinPercent);

		CString strText;
		strText.Format(_T("%s / %s"),CUtils::AdjustFileSize(ullMinCompleteSize),CUtils::AdjustFileSize(ullMaxValidSize));
		SetDlgItemText(IDC_TEXT_USAGE,strText);

		strText.Format(_T("%d MB/s"),(int)dbAvgSpeed);
		SetDlgItemText(IDC_TEXT_SPEED2,strText);
	
		CTimeSpan spanU = CTime::GetCurrentTime() - m_StartTime;
		SetDlgItemText(IDC_TEXT_TIME_ELAPSED2,spanU.Format(_T("%H:%M:%S")));	

		if (dbAvgSpeed > 0)
		{
			__time64_t time = (__time64_t)((ullMaxValidSize - ullMinCompleteSize)/1024/1024/dbAvgSpeed);
			CTimeSpan spanR(time);
			SetDlgItemText(IDC_TEXT_TIME_REMAINNING2,spanR.Format(_T("%H:%M:%S")));
		}

	}
}

BOOL CUSBCopyDlg::IsReady()
{
	CString strWorkMode,strMsg;
	GetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);

	BOOL bRet = TRUE;

	switch (m_WorkMode)
	{
	case WorkMode_ImageCopy:
	case WorkMode_DiskClean:
		if (!IsExistTarget())
		{
			strMsg.Format(_T("%s failed! Custom errorcode=0x%X, No Targets."),strWorkMode,CustomError_No_Target);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;
		}
		break;

	case WorkMode_ImageMake:
		if (!IsExistMaster())
		{
			strMsg.Format(_T("%s failed! Custom errorcode=0x%X, No Master."),strWorkMode,CustomError_No_Master);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;
		}
		break;

	default:
		if (!IsExistMaster())
		{
			strMsg.Format(_T("%s failed! Custom errorcode=0x%X, No Master."),strWorkMode,CustomError_No_Master);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;
		}

		if (!IsExistTarget())
		{
			strMsg.Format(_T("%s failed! Custom errorcode=0x%X, No Targets."),strWorkMode,CustomError_No_Master);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;
		}
		break;

	}

	m_strMsg = strMsg;
	m_bResult = bRet;

	return bRet;
}

BOOL CUSBCopyDlg::IsExistMaster()
{
	return m_MasterPort.IsConnected();
}

BOOL CUSBCopyDlg::IsExistTarget()
{
	BOOL bOK = FALSE;
	POSITION pos = m_TargetPorts.GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);
		if (port->IsConnected())
		{
			bOK = TRUE;
			break;
		}
	}

	return bOK;
}

afx_msg LRESULT CUSBCopyDlg::OnUpdateStatistic(WPARAM wParam, LPARAM lParam)
{
	UpdateStatisticInfo();
	return 0;
}

void CUSBCopyDlg::OnStart()
{
	m_StartTime = CTime::GetCurrentTime();

	// 开始线程
	SetTimer(TIMER_UPDATE_STATISTIC,1000,NULL);

	CDisk disk;
	disk.Init(m_hWnd,&m_bCancel,m_hLogFile);
	disk.SetWorkMode(m_WorkMode);

	HashMethod hashMethod = (HashMethod)m_Config.GetInt(_T("Option"),_T("HashMethod"),0);

	CString strMsg;
	BOOL bResult = TRUE;

	switch(m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("FullCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("FullCopy"),_T("En_Compare"),FALSE);

			// 设置端口状态
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,bCompare,hashMethod);
			
			bResult = disk.Start();

			CString strHashValue;
			if (bResult)
			{
				if (bComputeHash)
				{
					int len = m_MasterPort.GetHashLength();
					BYTE *pHash = new BYTE[len];
					ZeroMemory(pHash,len);
					m_MasterPort.GetHash(pHash,len);
					for (int i = 0;i < len;i++)
					{
						CString strHash;
						strHash.Format(_T("%02X"),pHash[i]);

						strHashValue += strHash;
					}
					delete []pHash;
					strMsg.Format(_T("FULL COPY Completed ! Hash Method = %s,Hash Value = %s")
						,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("FULL COPY Completed !!!"));
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("FULL COPY Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("FULL COPY Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
				
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;
	case WorkMode_QuickCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("QuickCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE);
			HashMethod hashMethod = (HashMethod)m_Config.GetInt(_T("Option"),_T("HashMethod"),0);

			// 设置端口状态
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,bCompare,hashMethod);

			bResult = disk.Start();

			CString strMsg,strHashValue;
			if (bResult)
			{
				if (bComputeHash)
				{
					int len = m_MasterPort.GetHashLength();
					BYTE *pHash = new BYTE[len];
					ZeroMemory(pHash,len);
					m_MasterPort.GetHash(pHash,len);
					for (int i = 0;i < len;i++)
					{
						CString strHash;
						strHash.Format(_T("%02X"),pHash[i]);

						strHashValue += strHash;
					}
					delete []pHash;
					strMsg.Format(_T("QUICK COPY Completed ! Hash Method = %s,Hash Value = %s")
						,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("QUICK COPY Completed !!!"));
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("QUICK COPY Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("QUICK COPY Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_DiskClean:
		{
			CleanMode cleanMode = (CleanMode)m_Config.GetInt(_T("DiskClean"),_T("CleanMode"),0);
			int nFillVal = m_Config.GetInt(_T("DiskClean"),_T("FillValue"),0,16);

			// 设置端口状态
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
				}
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetCleanMode(cleanMode,nFillVal);

			bResult = disk.Start();

			CString strMsg;
			if (bResult)
			{
				strMsg.Format(_T("DISK CLEAN Completed !!!"));
			}
			else
			{
				// 任意取一个错误
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				pos = m_TargetPorts.GetHeadPosition();
				while (pos)
				{
					CPort *port = m_TargetPorts.GetNext(pos);

					if (port->IsConnected() && !port->GetResult())
					{
						errType = port->GetErrorCode(&dwErrorCode);
						break;
					}
				}
				
				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("DISK CLEAN Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("DISK CLEAN Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_DiskCompare:
		{
			CompareMode compareMode = (CompareMode)m_Config.GetInt(_T("DiskCompare"),_T("CompareMode"),0);

			// 设置端口状态
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(TRUE,TRUE,hashMethod);
			disk.SetCompareMode(compareMode);

			bResult = disk.Start();

			CString strMsg,strHashValue;
			if (bResult)
			{
				int len = m_MasterPort.GetHashLength();
				BYTE *pHash = new BYTE[len];
				ZeroMemory(pHash,len);
				m_MasterPort.GetHash(pHash,len);
				for (int i = 0;i < len;i++)
				{
					CString strHash;
					strHash.Format(_T("%02X"),pHash[i]);

					strHashValue += strHash;
				}
				delete []pHash;
				strMsg.Format(_T("DISK COMPARE Completed ! Hash Method = %s,Hash Value = %s")
					,m_MasterPort.GetHashMethodString(),strHashValue);

			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("DISK COMPARE Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("DISK COMPARE Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_ImageMake:
		{
			CString strImagePath = m_Config.GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
			CString strImageName = m_Config.GetString(_T("ImagePath"),_T("ImageName"));

			// 设置端口状态
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);

			strImagePath.TrimRight(_T('\\'));
			if (strImageName.Right(4).CompareNoCase(_T(".IMG")) != 0)
			{
				strImageName += _T(".IMG");
			}

			CString strTempFile,strImageFile;
			strTempFile.Format(_T("%s\\%s.$$$"),strImagePath,strImageName.Left(strImageName.GetLength() - 4));
			strImageFile.Format(_T("%s\\%s"),strImagePath,strImageName);

			m_FilePort.SetPortType(PortType_TARGET_FILE);
			m_FilePort.SetFileName(strTempFile);
			m_FilePort.SetConnected(TRUE);
			m_FilePort.SetPortState(PortState_Online);
			m_FilePort.SetHashMethod(hashMethod);
			m_FilePort.SetWorkMode(m_WorkMode);

			PortList filePortList;
			filePortList.AddTail(&m_FilePort);

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&filePortList);
			disk.SetHashMethod(TRUE,FALSE,hashMethod);

			bResult = disk.Start();

			CString strMsg,strHashValue;
			if (bResult)
			{
				MoveFile(strTempFile,strImageFile);

				int len = m_MasterPort.GetHashLength();
				BYTE *pHash = new BYTE[len];
				ZeroMemory(pHash,len);
				m_MasterPort.GetHash(pHash,len);
				for (int i = 0;i < len;i++)
				{
					CString strHash;
					strHash.Format(_T("%02X"),pHash[i]);

					strHashValue += strHash;
				}
				delete []pHash;
				strMsg.Format(_T("IMAGE MAKE Completed ! Hash Method = %s,Hash Value = %s")
					,m_MasterPort.GetHashMethodString(),strHashValue);
				
			}
			else
			{
				DeleteFile(strTempFile);

				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("IMAGE MAKE Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("IMAGE MAKE Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case  WorkMode_ImageCopy:
		{
			BOOL bCompare = m_Config.GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE);

			// 设置端口状态
			CString strImagePath = m_Config.GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
			CString strImageName = m_Config.GetString(_T("ImagePath"),_T("ImageName"));
			strImagePath.TrimRight(_T('\\'));
			if (strImageName.Right(4).CompareNoCase(_T(".IMG")) != 0)
			{
				strImageName += _T(".IMG");
			}

			CString strImageFile;
			strImageFile.Format(_T("%s\\%s"),strImagePath,strImageName);

			m_FilePort.SetPortType(PortType_MASTER_FILE);
			m_FilePort.SetFileName(strImageFile);
			m_FilePort.SetConnected(TRUE);
			m_FilePort.SetPortState(PortState_Online);
			m_FilePort.SetWorkMode(m_WorkMode);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetWorkMode(m_WorkMode);
				}
			}

			disk.SetMasterPort(&m_FilePort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(TRUE,bCompare,hashMethod);

			bResult = disk.Start();

			CString strMsg,strHashValue;
			if (bResult)
			{
				int len = m_MasterPort.GetHashLength();
				BYTE *pHash = new BYTE[len];
				ZeroMemory(pHash,len);
				m_MasterPort.GetHash(pHash,len);
				for (int i = 0;i < len;i++)
				{
					CString strHash;
					strHash.Format(_T("%02X"),pHash[i]);

					strHashValue += strHash;
				}
				delete []pHash;
				strMsg.Format(_T("IMAGE COPY Completed ! Hash Value = %s"),strHashValue);
				
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_FilePort.GetErrorCode(&dwErrorCode);

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("IMAGE COPY Failed ! System errorCode=%d,%s"),dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("IMAGE COPY Failed ! Custom errorCode=0x%X,%s"),dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;
	}

	m_strMsg = strMsg;
	m_bResult = bResult;

	KillTimer(TIMER_UPDATE_STATISTIC);
	PostMessage(WM_UPDATE_STATISTIC);
	PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_STOP, BN_CLICKED), (LPARAM)m_hWnd); 
}

DWORD CUSBCopyDlg::StartThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;
	pDlg->OnStart();

	return 1;
}

CString CUSBCopyDlg::GetCustomErrorMsg( CustomError customError )
{
	CString strError;
	switch (customError)
	{
		case CustomError_Cancel:
			strError = _T("Cancelled by user.");
			break;

		case CustomError_No_Master:
			strError = _T("No master.");
			break;

		case CustomError_No_Target:
			strError = _T("No target.");
			break;

		case CustomError_Compare_Failed:
			strError = _T("Compare faied.");
			break;

		case CustomError_All_Targets_Failed:
			strError = _T("All targets failed.");
			break;

		case CustomError_Master_Failed:
			strError = _T("Master failed.");
			break;

		case CustomError_Image_Format_Error:
			strError = _T("Image format error.");
			break;

		case CustomError_Compress_Error:
			strError = _T("Compress error.");
			break;

		case CustomError_UnCompress_Error:
			strError = _T("Uncompress error.");
			break;
	}

	return strError;
}


HBRUSH CUSBCopyDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_TEXT_FUNCTION2:
	case IDC_TEXT_SPEED2:
	case IDC_TEXT_TIME_ELAPSED2:
	case IDC_TEXT_TIME_REMAINNING2:
	case IDC_TEXT_USAGE:
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(30,144,255));
		break;
		
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


afx_msg LRESULT CUSBCopyDlg::OnSendFunctionText(WPARAM wParam, LPARAM lParam)
{
	char *buf = (char *)wParam;
	CString strMsg(buf);

	SetDlgItemText(IDC_TEXT_FUNCTION2,strMsg);

	return 0;
}

void CUSBCopyDlg::ResetPortInfo()
{
	m_MasterPort.Initial();
	m_FilePort.Initial();

	POSITION pos = m_TargetPorts.GetHeadPosition();
	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);
		port->Initial();
	}
}


BOOL CUSBCopyDlg::DestroyWindow()
{
	// TODO: 在此添加专用代码和/或调用基类
	POSITION pos = m_TargetPorts.GetHeadPosition();

	while(pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);
		delete port;
	}

	m_TargetPorts.RemoveAll();

	for (UINT i = 0;i < m_nPortNum;i++)
	{
		m_PortFrames[i].DestroyWindow();
	}

	if (m_hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hLogFile);
		m_hLogFile = INVALID_HANDLE_VALUE;
	}

	m_Config.SetPathName(m_strAppPath + MACHINE_INFO);
	CString strAlias;
	GetDlgItemText(IDC_TEXT_ALIAS,strAlias);
	m_Config.WriteString(_T("MachineInfo"),_T("Alias"),strAlias);

	return CDialogEx::DestroyWindow();
}

void CUSBCopyDlg::InitailMachine()
{
	//初始化上电
	m_Command.Reset();

	// 全部上电
	for (UINT i = 0; i < m_nPortNum;i++)
	{
		m_Command.Power(i,TRUE);
		Sleep(100);
	}

	// 切屏
	m_Command.SwitchScreen();

	// 使能闪烁命令
	m_Command.WorkLight();
}

DWORD CUSBCopyDlg::InitialMachineThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;
	pDlg->InitailMachine();

	return 1;
}


afx_msg LRESULT CUSBCopyDlg::OnComReceive(WPARAM wParam, LPARAM lParam)
{
	DWORD dwBufferLength = m_SerialPort.GetInBufferLength();

	if (dwBufferLength == 0)
	{
		return 0;
	}

	BYTE *pBuffer = new BYTE[dwBufferLength];
	memset(pBuffer,0,dwBufferLength);
	m_SerialPort.ReadBytes(pBuffer,dwBufferLength);

	for (DWORD i = 0;i < dwBufferLength;i++)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("[SerialPort] RD = %02X"),(BYTE)pBuffer[i]);
		if (pBuffer[i] == 0x5A)
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Start event click."),(BYTE)pBuffer[i]);
			keybd_event(VK_RETURN,0,0,0);
			keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
		}
		else if (pBuffer[i] == 0x76)
		{
			if (GetDlgItem(IDC_BTN_STOP)->IsWindowEnabled() && ::GetActiveWindow() == m_hWnd)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Stop event click."),(BYTE)pBuffer[i]);
				PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_STOP, BN_CLICKED), (LPARAM)m_hWnd); 
			}		
		}
	}

	return 0;
}

afx_msg LRESULT CUSBCopyDlg::OnResetMachienPort(WPARAM wParam, LPARAM lParam)
{
	InitialStatisticInfo();
	ResetPortFrame();
	ResetPortInfo();

	// 复位
	m_Command.Reset();
	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnComplete(WPARAM wParam, LPARAM lParam)
{
	CCompleteMsg completeMsg(m_strMsg,m_bResult);

	completeMsg.DoModal();

	return 0;
}
