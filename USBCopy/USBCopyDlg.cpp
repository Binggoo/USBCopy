
// USBCopyDlg.cpp : implementation file
//
//V1.0.0.1 2014-09-16 Binggoo 1. �޸��޳����ƣ���ǰ���öϵ�ķ��������ڸĳɳ�����ơ�
//                            2. ����������USB����������ⲻ���豸ʱ���ܶϵ�.
//V1.0.1.0 2014-09-16 Binggoo 1. �����ֶ�ѡ�����ݿ�������С����ʽ������
//V1.0.1.1 2014-09-17 Binggoo 1. ��д���̺��ļ�ʱ�����볬ʱ
//V1.0.2.0 2014-09-19 Binggoo 1. �޸ĳ�ʱʱ��Ϊ10s
//                            2. ��Ӽ�¼�豸�����кź��ͺš�
//                            3. Record��ÿ�����ʽ��¼��
//                            4. ����logʱ������ѡ��Record��ʱ��
//V1.0.3.0 2014-09-25 Binggoo 1.��������ʾTF/SD�������к�
//                            2.������ƶ���SD����ʱ����ʾ�˿�����Ϣ
//V1.0.4.0 2014-10-15 Binggoo 1.���Ӳ��쿽������
//                            2.����ͨ�������ļ������ó�����ʾ���ԣ�������ʱ��ϵͳ���Ծ���
//V1.0.3.1 2014-10-30 Binggoo 1.�޸��жϵ�һ��������MBR����DBR�ķ�ʽ��
//                            2.�����޳����ʱ�䡣
//V1.0.5.0 2014-11-04 Binggoo 1.����MTP����ģʽ
//                            2.���������书��
//                            3.�����ػ�����
//V1.0.6.0 2014-11-13 Binggoo 1.Full Copy�м�������������������
//                            2.Full Copy��Quick Copy�м��뿽��֮ǰ��ȫ�̲�������
//                            3.Image Make��Image Copy�м���Image����/ģʽ����

//V1.0.6.1 2014-11-14 Binggoo 1.Image Copy�м��뿽��֮ǰ��ȫ�̲���������
//                            2.�������������λس����Ϳո����
//V1.0.6.2 2014-11-17 Binggoo 1.���TS-117N ��ͣˢ������
//V1.0.6.3 2014-11-17 Binggoo 1.������������бȶ�
//V1.0.6.4 2014-11-18 Binggoo 1.����By-Byte�ȶԷ�ʽ
//V1.0.7.0 2014-11-20 Binggoo 1.����MTPӳ��������ӳ�񿽱�
//                            2.�������ô��ڴ�С����
//V1.0.8.0 2014-11-26 Binggoo 1.���뿨��⹦��
//                            2.�ȶԲ����м�������бȶԺͲ�����ȶ�
//                            3.����NGFF����������
//v1.0.8.2 2014-11-27 Binggoo 1.�Ż��ջ�����
//                            2.����ɨ���豸ʱ�䲻������Ϊ0������
//                            3.���ⲿ����ȡ��ʱ��ʱ2s��������������Ͱ��������ص�
//                            4.����ֶ�ֹͣʱ������ֽ������������⡣
//v1.0.8.3 2014-11-29 Binggoo 1.���봴���߳�ʧ�ܼ�飬��ֹ��Ϊ�����߳�ʧ�ܶ����³��������
//v1.0.9.0 2014-12-02 Binggoo 1.���USB-123����12x2��
//                            2.�޸�ջ�ڴ��СΪ10000000
//                            3.�޸����NFGG�ĵ�Դ����
//v1.0.9.1 2014-12-03 Binggoo 1.Ϊ�˱�������ڴ治���õ������ȡ�����ݿ�������Ϊ1024��512�����á�
//         2014-12-18 Binggoo 2.�Ѵ�MBR����һ������֮������ݶ���Ϊ��Ч����
//v1.0.9.2 2014-12-18 Binggoo 1.EXT�ļ�ϵͳ��Bitmap����ҪȡBlock���ȵ���������
//                            2.������̱Ƚ�ѡ��By-Byte��ʽʱ��ĸ�̲���ʾPASS�����⡣
//                            3.���Ӣ�İ���ٿ�������ʱ�������⡣
//v1.1.0.0 2014-12-22 Binggoo 1.�޸Ķ�д���̲�����ϵͳ���棬ֱ�ӶԴ��̽��ж�д��
//                            2.��������̲߳�ͬ�����µĶ������⡣
//                            3.����ĸ�̺���������������ͬ��ȫ�ֲ��������ȫ�̿��������������������ѡ��˲�����Ч��
//                            4.����ӳ����������м��������Ƿ�ѹ��ѡ�
//                            5.���ٿ����Ϳ���ӳ���У��û������Զ�����Ҫ���������򣬴˹��ܿ�������ĸ���д�������������޷������ķ�����
//                            6.���TS-117N���������̺��п��ܵ�����LOG�����⡣
//                            7.������̱ȶ��м��밴�ֽڱȶԺ�ѡ��HASH�ȶ���Ч�����⡣
//v1.1.1.0 2014-12-26 Binggoo 1.���MTP������MTP�豸������չ����������⣬Ŀǰ�����MTP�豸�е���չ����
//v1.1.2.0 2015-01-05 Binggoo 1.ͬ��ӳ���м���ӳ��ĵ��뵼�����ܣ����Դӱ����ƶ��豸���뵼����
//v1.1.3.0 2015-01-06 Binggoo 1.���v1092���޸�EXT�ļ�ϵͳʱ���Ķ���NTFS�ļ�ϵͳ������NTFS�ļ�ϵͳ���ݷ����ظ������⡣
//v1.1.4.0 2015-01-12 Binggoo 1.������ѡ�񿽱�֮ǰ����ʱ���ڲ���֮ǰ��Ҫ����������飬������鲻����Ҫ�������ֱ��Fail�������������
//                            2.���ӳ�񿽱�ʱ����ѡ�����ȶ�Ҳ����в����ȶԵ����⡣
//                            3.������ÿ���֮ǰ�������������֮����ʾ��ͬ�������⡣
//v1.1.5.0 2015-01-12 Binggoo 1.�������ƽ����USB�洢ģʽʱ����ʶ������⡣
//         2015-01-15 Binggoo 2.���v1.1.4.0�޸��������⡣
//         2015-01-16 Binggoo 3.�������֮ǰ�������в�ͬ�����Ŀ��������ʾ���Ȳ�ͬ�������⡣
//v1.1.6.0 2015-01-23 Binggoo 1.��������ʹ����˸�����λ�á�
//                            2.�����������������˳��
//v1.1.7.0 2015-01-30 Binggoo 1.�ļ������м��뿽��ȫ���ļ��Լ����Ŀ���̲�����
//                            2.�ļ��������ļ�ʱ�������޻��淽ʽ���ļ�����ֹ��ϵͳ�����ж�����ٶȹ���
//                            3.log�ļ�����1G���Զ�ɾ�����Զ�ɾ�����������record�ļ���
//         2015-01-31 Binggoo 4.���������ѹ��ӳ��ʱ��ѡ��Hash�ȶԳ�������⡣
//v1.1.8.0 2015-02-09 Binggoo 1.������ӷ����������������⡣
//                            2.���������жϺ�ÿ��5s�Զ�������
//                            3.�����ԭ����ӻָ��������ÿ�ѡ�
//                            4.�޸Ĵ��̱ȶ�ʱ��ĸ�̲��������ʾ��
//         2015-02-13 Binggoo 5.���ݶ�������Ӻ��Ƴ�ͨ���ź������ܿء�
//                            6.�ָ���ԭ���ܿض��еķ�ʽ��
//         2015-03-05 Binggoo 7.���ٶ�����ʱ�����ڴ������
//         2015-03-10 Binggoo 8.������������ļ��ͱ��������ļ����ܡ�


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
#include "FileCopySetting.h"
#include "DiskFormatSetting.h"
#include "DiffCopySetting.h"
#include "PackageName.h"
#include "Lisence.h"

#include <dbt.h>
#include "EnumUSB.h"
#include "EnumStorage.h"
#include "WPDFunction.h"
#include "MTPCopySetting.h"
#include "WpdDevice.h"
#include "BurnIn.h"
#include "FullRWTest.h"
#include "FadePickerSetting.h"
#include "SpeedCheckSetting.h"
#include <mstcpip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 600

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
	m_bRunning = FALSE;
	m_bUpdate = FALSE;
	m_bExportLog = FALSE;
	m_bBurnInTest = FALSE;
	m_bStart = FALSE;
	m_bLisence = FALSE;
	m_bSockeConnected = FALSE;
	m_bIsMTP = FALSE;

	m_nMachineType = MT_TS;

	m_bEnableButton = TRUE;

	m_nCols = 6;
	m_nRows = 3;
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
	ON_MESSAGE(WM_UPDATE_STATISTIC, &CUSBCopyDlg::OnUpdateStatistic)
	ON_WM_CTLCOLOR()
	ON_MESSAGE(ON_COM_RECEIVE, &CUSBCopyDlg::OnComReceive)
	ON_MESSAGE(WM_RESET_MACHIEN_PORT, &CUSBCopyDlg::OnResetMachienPort)
	ON_WM_DEVICECHANGE()
	ON_MESSAGE(WM_UPDATE_SOFTWARE, &CUSBCopyDlg::OnUpdateSoftware)
	ON_MESSAGE(WM_RESET_POWER, &CUSBCopyDlg::OnResetPower)
	ON_MESSAGE(WM_BURN_IN_TEST, &CUSBCopyDlg::OnBurnInTest)
	ON_MESSAGE(WM_CONNECT_SOCKET, &CUSBCopyDlg::OnConnectSocket)
	ON_MESSAGE(WM_DISCONNECT_SOCKET, &CUSBCopyDlg::OnDisconnectSocket)
	ON_MESSAGE(WM_SOCKET_MSG, &CUSBCopyDlg::OnSocketMsg)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_UPDATE_FUNCTION, &CUSBCopyDlg::OnUpdateFunction)
	ON_MESSAGE(WM_SET_BURN_IN_TEXT, &CUSBCopyDlg::OnSetBurnInText)
	ON_MESSAGE(WM_INIT_CURRENT_WORKMODE, &CUSBCopyDlg::OnInitCurrentWorkmode)
	ON_MESSAGE(WM_EXPORT_LOG_START, &CUSBCopyDlg::OnExportLogStart)
	ON_MESSAGE(WM_EXPORT_LOG_END, &CUSBCopyDlg::OnExportLogEnd)
	ON_MESSAGE(WM_LOAD_CONFIG, &CUSBCopyDlg::OnLoadConfig)
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

	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	CString strLogFile = m_strAppPath + LOG_FILE;
	m_hLogFile = CreateFile(strLogFile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if (m_hLogFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(_T("Create Log File Error"),_T("Error"),MB_ICONERROR);

		SendMessage(WM_QUIT);
	}

	m_hBurninEvent = CreateEvent(NULL,FALSE,TRUE,NULL);

	if (m_hBurninEvent == NULL)
	{
		MessageBox(_T("CreateEvent Error"),_T("Error"),MB_ICONERROR);

		SendMessage(WM_QUIT);
	}

	// ���log�ļ���С����2M������һ��
	LARGE_INTEGER liFillSize = {0};
	GetFileSizeEx(m_hLogFile,&liFillSize);
	if (liFillSize.QuadPart > 2 * 1024 * 1024)
	{
		BackupLogfile(m_hLogFile,(DWORD)liFillSize.QuadPart);
		CloseHandle(m_hLogFile);
		DeleteFile(strLogFile);

		m_hLogFile = CreateFile(strLogFile,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

		if (m_hLogFile == INVALID_HANDLE_VALUE)
		{
			MessageBox(_T("CreateFile Error"),_T("Error"),MB_ICONERROR);

			SendMessage(WM_CLOSE);
		}

	}

	//���״̬��
	CString strVersion,strSN,strModel,strAlias;

	CFileStatus status;
	CFile::GetStatus(strPath,status);

	CUtils::WriteLogFile(m_hLogFile,FALSE,_T(""));
	CUtils::WriteLogFile(m_hLogFile,FALSE,_T("********************************************************************************"));
	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("USBCopy Ver:%s BLD:%s"),CUtils::GetAppVersion(strPath),status.m_mtime.Format(_T("%Y-%m-%d")));

	// ��������Ϣ
	m_Config.SetPathName(m_strAppPath + MACHINE_INFO);
	strVersion.Format(_T("Ver:%s  BLD:%s "),CUtils::GetAppVersion(strPath),status.m_mtime.Format(_T("%Y-%m-%d")));
	strSN.Format(_T("%s"),m_Config.GetString(_T("MachineInfo"),_T("SN")));
	strModel.Format(_T("%s"),m_Config.GetString(_T("MachineInfo"),_T("Model")));
	strAlias.Format(_T("%s"),m_Config.GetString(_T("MachineInfo"),_T("Alias"),_T("PHIYO")));

	// �ж���USB�ӿڻ���TF/SD�ӿ�
	if (strModel.Find(_T("TS")) != -1)
	{
		m_nMachineType = MT_TS;
	}
	else if (strModel.Find(_T("USB")) != -1)
	{
		m_nMachineType = MT_USB;;
	}
	else if (strModel.Find(_T("NGFF")) != -1)
	{
		m_nMachineType = MT_NGFF;;
	}

	// �������ļ����û�USBCopy.ini
	m_strConfigPath = m_strAppPath + CONFIG_NAME;
	m_Config.SetPathName(m_strConfigPath);

	// ��ȡ������ص�����
	CString strRecentConfig = m_Config.GetString(_T("AppSetting"),_T("RecentConfig"));

	if (!strRecentConfig.IsEmpty() && PathFileExists(strRecentConfig))
	{
		m_Config.SetPathName(strRecentConfig);
		m_strConfigPath = strRecentConfig;
	}

	m_Config.WriteString(_T("Option"),_T("MachineAlias"),strAlias);

	int cx = m_Config.GetInt(_T("AppSetting"),_T("Width"),DEFAULT_WIDTH);
	int cy = m_Config.GetInt(_T("AppSetting"),_T("Height"),DEFAULT_HEIGHT);

	if (cx == 0 || cy == 0)
	{
		cx = DEFAULT_WIDTH;
		cy = DEFAULT_HEIGHT;
	}
	else if (cx == -1 || cy == -1)
	{
		cx  = GetSystemMetrics(SM_CXSCREEN);
		cy = GetSystemMetrics(SM_CYSCREEN);
	}

	SetWindowPos(NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);

	m_font.CreatePointFont(100,_T("Arial"));
	GetDlgItem(IDC_TEXT_SN)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_MODEL)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_ALIAS)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_COPYRIGHT)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_CONNECT)->SetFont(&m_font);
	GetDlgItem(IDC_TEXT_CONFIG)->SetFont(&m_font);

	SetDlgItemText(IDC_TEXT_SN,strSN);
	SetDlgItemText(IDC_TEXT_MODEL,strModel);
	SetDlgItemText(IDC_TEXT_ALIAS,strAlias);
	SetDlgItemText(IDC_TEXT_COPYRIGHT,strVersion);
	SetDlgItemText(IDC_TEXT_CONNECT,_T(""));
	SetDlgItemText(IDC_TEXT_CONFIG,CUtils::GetFileTitle(m_strConfigPath));

	m_LogoFont.CreateFont(60,
						  0,
						  0,
						  0,
						  FW_BOLD,
						  FALSE,
						  FALSE,
						  0,
						  ANSI_CHARSET,
						  OUT_DEFAULT_PRECIS,
						  CLIP_DEFAULT_PRECIS,
						  DEFAULT_QUALITY, 
						  DEFAULT_PITCH | FF_SWISS, 
						  _T("Arial"));

	GetDlgItem(IDC_TEXT_LOGO)->SetFont(&m_LogoFont);

	switch (m_nMachineType)
	{
	case MT_TS:
		SetDlgItemText(IDC_TEXT_LOGO,LOGO_TS);
		break;

	case MT_USB:
		SetDlgItemText(IDC_TEXT_LOGO,LOGO_USB);
		break;

	case MT_NGFF:
		SetDlgItemText(IDC_TEXT_LOGO,LOGO_NGFF);
		break;
	}

	GetDlgItem(IDC_BTN_START)->SetFont(&m_LogoFont);

	BOOL bShowCursor = m_Config.GetBool(_T("Option"),_T("ShowCursor"),TRUE);
	ShowCursor(bShowCursor);

	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2,2),&wsd) != 0)
	{
		DWORD dwErrorCode  = WSAGetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSAStartup() failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
	}
	else
	{
		// �������磬ͬ��ʱ��
		//PostMessage(WM_CONNECT_SOCKET);
		AfxBeginThread((AFX_THREADPROC)ConnectSocketThreadProc,this);
	}

	m_bLock = m_Config.GetBool(_T("Option"),_T("En_Lock"),FALSE);
	m_strPassWord = m_Config.GetString(_T("Option"),_T("Password"),_T("123456"));

	// Button
	m_BtnStart.SubclassDlgItem(IDC_BTN_START,this);
	m_BtnStart.SetBitmaps(IDB_START,RGB(255,255,255));
	m_BtnStart.DrawBorder(FALSE);
	SetDlgItemText(IDC_BTN_START,_T(""));

	m_BtnLock.SubclassDlgItem(IDC_BTN_LOCK,this);
	m_BtnLock.SetBitmaps(IDB_LOCK,RGB(255,255,255));
	m_BtnLock.SetFlat(FALSE);

	if (m_bLock)
	{
		EnableControls(FALSE);

		CString strLock;
		strLock.LoadString(IDS_UNLOCK);
		SetDlgItemText(IDC_BTN_LOCK,strLock);
		m_BtnLock.SetBitmaps(IDB_UNLOCK,RGB(255,255,255));
	}

	m_BtnMenu.SubclassDlgItem(IDC_BTN_SYSTEM,this);
	m_BtnMenu.SetBitmaps(IDB_MENU,RGB(255,255,255));
	m_BtnMenu.SetFlat(FALSE);

	m_BtnSetting.SubclassDlgItem(IDC_BTN_SETTING,this);
	m_BtnSetting.SetBitmaps(IDB_SETTING,RGB(255,255,255));
	m_BtnSetting.SetFlat(FALSE);

	m_BtnWorkMode.SubclassDlgItem(IDC_BTN_WORK_SELECT,this);
	m_BtnWorkMode.SetFlat(FALSE);
	
	
	// ��ʼ��
	InitialPortPath();

	InitialPortFrame();

	InitialCurrentWorkMode();

	m_ProgressCtrl.ShowPercent(TRUE);
	m_ProgressCtrl.SetRange32(0,100);

	InitialStatisticInfo();

	InitialSerialPort();
	m_Command.Init(&m_SerialPort,m_hLogFile);

	AfxBeginThread((AFX_THREADPROC)InitialMachineThreadProc,this);

	UpdatePortFrame(TRUE);

	//�����NGFF����ʵʱ���
	if (m_nMachineType != MT_NGFF)
	{
		m_ThreadListen = AfxBeginThread((AFX_THREADPROC)EnumDeviceThreadProc,this,0,CREATE_SUSPENDED);

		if (m_ThreadListen != NULL)
		{
			m_ThreadListen->m_bAutoDelete = FALSE;
			m_ThreadListen->ResumeThread();
		}
	}

	SetTimer(TIMER_LISENCE,100,NULL);

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

	// ����18���ӿڣ���һ��12������
	UINT nIDTemplate = IDD_DIALOG_PORT;
	if (m_nPortNum > 18)
	{
		m_nCols = 12;

		nIDTemplate = IDD_DIALOG_PORT_V;
	}

	int rows = m_nPortNum / m_nCols;

	if (m_nPortNum % m_nCols)
	{
		rows++;
	}

	if (rows == 1)
	{
		rows = 2;
	}

	int nWidth = (rectFrame.Width()-2) / m_nCols;
	int nHeight = (rectFrame.Height()-2) / rows;

	// ĸ��
	m_PortFrames[0].SetPort(&m_Config,m_hLogFile,&m_Command,&m_MasterPort,&m_TargetPorts,m_nMachineType);

	m_PortFrames[0].Create(nIDTemplate,this);

	m_PortFrames[0].MoveWindow(rectFrame.left + 1,
		rectFrame.top + 1,
		nWidth,
		nHeight);

	m_PortFrames[0].ShowWindow(SW_SHOW);

	// ����
	POSITION pos = m_TargetPorts.GetHeadPosition();
	int nItem = 1;
	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);

		m_PortFrames[nItem].SetPort(&m_Config,m_hLogFile,&m_Command,port,&m_TargetPorts,m_nMachineType);

		m_PortFrames[nItem].Create(nIDTemplate,this);

		UINT nRow = nItem / m_nCols;
		UINT nCol = nItem % m_nCols;

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
		m_PortFrames[i].Update(bStart);
	}
}

void CUSBCopyDlg::EnableKickOff(BOOL bEnable)
{
	for (UINT i = 0;i < m_nPortNum;i++)
	{
		m_PortFrames[i].EnableKickOff(bEnable);
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

	// TODO: �ڴ˴������Ϣ����������
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
	// ���List
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

	CString strWorkMode,strWorkModeParm;
	CString strResText1,strResText2,strResText3;
	UINT nBitMap = IDB_QUICK_COPY;
	m_bIsMTP = FALSE;
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			nBitMap = IDB_FULL_COPY;
			strResText1.LoadString(IDS_WORK_MODE_FULL_COPY);

			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("FullCopy"),_T("En_CleanDiskFirst"),FALSE))
			{
				strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

				strWorkModeParm = strResText3;
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText1;
			}

			if (m_Config.GetBool(_T("FullCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText2;
			}
		}
		break;

	case WorkMode_QuickCopy:
		{
			nBitMap = IDB_QUICK_COPY;

			strResText1.LoadString(IDS_WORK_MODE_QUICK_COPY);
			strWorkMode = strWorkModeParm = strResText1;


			if (m_Config.GetBool(_T("QuickCopy"),_T("En_CleanDiskFirst"),FALSE))
			{
				strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

				strWorkModeParm = strResText3;
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText1;
			}

			if (m_Config.GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText2;
			}
		}
		break;

	case WorkMode_ImageCopy:
		{
			nBitMap = IDB_IMAGE_COPY;

			strResText1.LoadString(IDS_WORK_MODE_IMAGE_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}

			// MTP Copy
			if (m_Config.GetUInt(_T("ImageCopy"),_T("ImageType"),0) == 1)
			{
				m_bIsMTP = TRUE;
				strResText3.LoadString(IDS_MTP_IMAGE);
				strWorkModeParm += _T(" - ");
				strWorkModeParm += strResText3;
			}
			else
			{
				if (m_Config.GetBool(_T("ImageCopy"),_T("En_CleanDiskFirst"),FALSE))
				{
					strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

					strWorkModeParm = strResText3 + _T(" + ") + strWorkModeParm;
				}
			}
		}
		break;

	case WorkMode_ImageMake:
		{
			nBitMap = IDB_MAKE_IMAGE;
			strResText1.LoadString(IDS_WORK_MODE_IMAGE_MAKE);
			strWorkMode = strWorkModeParm = strResText1;

			UINT nSaveMode = m_Config.GetUInt(_T("ImageMake"),_T("SaveMode"),0);

			switch (nSaveMode)
			{
			case 0: //Full image
				strResText3.LoadString(IDS_FULL_IMAGE);
				
				break;

			case 1: //Quick image
				strResText3.LoadString(IDS_QUICK_IMAGE);
				break;

			case 2: //MTP image
				m_bIsMTP = TRUE;
				strResText3.LoadString(IDS_MTP_IMAGE);
				break;

			}

			strWorkModeParm += _T(" - ");
			strWorkModeParm += strResText3;
		}
		
		break;

	case WorkMode_DiskClean:
		{
			nBitMap = IDB_DISK_CLEAN;
			strResText1.LoadString(IDS_WORK_MODE_DISK_CLEAN);
			strWorkMode = strWorkModeParm = strResText1;

			CleanMode cleanMode = (CleanMode)m_Config.GetInt(_T("DiskClean"),_T("CleanMode"),0);
			switch (cleanMode)
			{
			case CleanMode_Full:
				strResText2.LoadString(IDS_FULL_CLEAN);
				break;

			case CleanMode_Quick:
				strResText2.LoadString(IDS_QUICK_CLEAN);
				break;

			case CleanMode_Safe:
				strResText2.LoadString(IDS_SAFE_CLEAN);
				break;
			}

			strWorkModeParm.Format(_T("%s - %s"),strResText1,strResText2);
		}

		break;

	case WorkMode_DiskCompare:
		{
			nBitMap = IDB_DISK_COMPARE;
			strResText1.LoadString(IDS_WORK_MODE_DISK_COMPARE);
			strWorkMode = strWorkModeParm = strResText1;

			CompareMode compareMode = (CompareMode)m_Config.GetInt(_T("DiskCompare"),_T("CompareMode"),0);

			switch (compareMode)
			{
			case CompareMode_Full:
				strResText2.LoadString(IDS_FULL_COMPARE);
				break;

			case CompareMode_Quick:
				strResText2.LoadString(IDS_QUICK_COMPARE);
				break;
			}

			strWorkModeParm.Format(_T("%s - %s"),strResText1,strResText2);
		}
		break;

	case WorkMode_FileCopy:
		{
			nBitMap = IDB_FILE_COPY;
			strResText1.LoadString(IDS_WORK_MODE_FILE_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("FileCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}
		}
		break;

	case WorkMode_DiskFormat:
		{
			nBitMap = IDB_DISK_FORMAT;
			strResText1.LoadString(IDS_WORK_MODE_DISK_FORMAT);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_DifferenceCopy:
		{
			nBitMap = IDB_DIFF_COPY;
			strResText1.LoadString(IDS_WORK_MODE_DIFF_COPY);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_MTPCopy:
		{
			nBitMap = IDB_MTP_COPY;
			strResText1.LoadString(IDS_WORK_MODE_MTP_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("MTPCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}

			m_bIsMTP = TRUE;
		}
		break;

	case WorkMode_Full_RW_Test:
		{
			nBitMap = IDB_FULL_SCAN;
			strResText1.LoadString(IDS_WORK_MODE_FULL_RW_TEST);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_Fade_Picker:
		{
			nBitMap = IDB_FAKE_PICKER;
			strResText1.LoadString(IDS_WORK_MODE_FADE_PICKER);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_Speed_Check:
		{
			nBitMap = IDB_SPEED_TEST;
			strResText1.LoadString(IDS_WORK_MODE_SPEED_CHECK);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_Burnin_Test:
		{
			nBitMap = IDB_BURN_IN;
			strResText1.LoadString(IDS_WORK_MODE_BURN_IN);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;
	}

	SetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);
	SetDlgItemText(IDC_GROUP_WORK_MODE,strWorkModeParm);

	m_BtnWorkMode.SetBitmaps(nBitMap,RGB(255,255,255));
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
		GetDlgItem(IDC_BTN_SYSTEM)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_WORK_SELECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_BTN_SYSTEM)->EnableWindow(bEnable);
		GetDlgItem(IDC_BTN_WORK_SELECT)->EnableWindow(bEnable);
		GetDlgItem(IDC_BTN_SETTING)->EnableWindow(bEnable);
	}
}


BOOL CUSBCopyDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_LOCK:
		case IDC_BTN_SYSTEM:
		case IDC_BTN_WORK_SELECT:
		case IDC_BTN_SETTING:
		case IDC_BTN_START:
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strResText;
	if (m_bLock)
	{
		CPassword password(FALSE);
		password.SetPassword(m_strPassWord);
		if (IDOK == password.DoModal())
		{
			m_bLock = FALSE;
			strResText.LoadString(IDS_LOCK);
			SetDlgItemText(IDC_BTN_LOCK,strResText);
			EnableControls(TRUE);

			m_BtnLock.SetBitmaps(IDB_LOCK,RGB(255,255,255));
		}
		
	}
	else
	{
		CPassword password(TRUE);
		password.SetPassword(m_strPassWord);
		if (IDOK == password.DoModal())
		{
			m_strPassWord = password.GetPassword();
			m_bLock = TRUE;
			strResText.LoadString(IDS_UNLOCK);
			SetDlgItemText(IDC_BTN_LOCK,strResText);
			EnableControls(FALSE);

			m_BtnLock.SetBitmaps(IDB_UNLOCK,RGB(255,255,255));
		}
	}

	m_Config.WriteBool(_T("Option"),_T("En_Lock"),m_bLock);
	m_Config.WriteString(_T("Option"),_T("Password"),m_strPassWord);
}


void CUSBCopyDlg::OnBnClickedBtnSystem()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CSystemMenu menu;
	menu.SetConfig(&m_Config,&m_Command,m_bSockeConnected,m_bLisence,m_nMachineType);
	menu.DoModal();

	CString strAlias = m_Config.GetString(_T("Option"),_T("MachineAlias"),_T("PHIYO"));
	SetDlgItemText(IDC_TEXT_ALIAS,strAlias);

	CIni machine(m_strAppPath + MACHINE_INFO);
	machine.WriteString(_T("MachineInfo"),_T("Alias"),strAlias);
}


void CUSBCopyDlg::OnBnClickedBtnWorkSelect()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CSelectCurModeDlg dlg;
	dlg.SetConfig(&m_Config);
	dlg.DoModal();

	InitialCurrentWorkMode();
}


void CUSBCopyDlg::OnBnClickedBtnSetting()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

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

	case WorkMode_FileCopy:
		{
			CString strMsg;
			if (m_MasterPort.IsConnected())
			{
				CStringArray strVolumeArray;
				m_MasterPort.GetVolumeArray(strVolumeArray);

				if (strVolumeArray.GetCount() == 0)
				{
					strMsg.LoadString(IDS_MSG_MASTER_UNRECOGNIZED);
					MessageBox(strMsg);
				}
				else
				{
					//��ĸ�̷����̷�
					CString strVolume = strVolumeArray.GetAt(0);
					strVolume += _T("\\");
					CDisk::ChangeLetter(strVolume,MASTER_PATH);

					if (PathFileExists(MASTER_PATH))
					{
						CFileCopySetting dlg;
						dlg.SetConfig(&m_Config,&m_MasterPort);
						dlg.DoModal();
					}
					else
					{
						strMsg.LoadString(IDS_MSG_MASTER_UNRECOGNIZED);
						MessageBox(strMsg);
					}
				}
			}
			else
			{
				strMsg.LoadString(IDS_MSG_NO_MASTER);
				MessageBox(strMsg);
			}
			
		}
		break;

	case WorkMode_MTPCopy:
		{
			CMTPCopySetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_DiskFormat:
		{
			CDiskFormatSetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_DifferenceCopy:
		{
			CDiffCopySetting dlg;
			dlg.SetConfig(&m_Config,&m_MasterPort);
			dlg.DoModal();
		}
		break;

	case WorkMode_Burnin_Test:
		{
			CBurnIn burnIn;
			burnIn.SetConfig(&m_Config);
			burnIn.DoModal();
		}
		break;

	case WorkMode_Full_RW_Test:
		{
			CFullRWTest dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_Fade_Picker:
		{
			CFadePickerSetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;

	case WorkMode_Speed_Check:
		{
			CSpeedCheckSetting dlg;
			dlg.SetConfig(&m_Config);
			dlg.DoModal();
		}
		break;
	}

	InitialCurrentWorkMode();
}


void CUSBCopyDlg::OnBnClickedBtnStart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (!m_bEnableButton)
	{
		return;
	}

	CString strResText;

	if (m_bStart)
	{
		m_bStart = FALSE;
		m_bEnableButton = FALSE;

		//OnStop();
		AfxBeginThread((AFX_THREADPROC)StopThreadProc,this);

		//strResText.LoadString(IDS_BTN_START);
		//SetDlgItemText(IDC_BTN_START,strResText);
	}
	else
	{
		m_bStart = TRUE;

		switch(m_WorkMode)
		{
		case WorkMode_ImageMake:
			{

				if (!m_bBurnInTest)
				{
					CImageNameDlg dlg(TRUE);
					dlg.SetConfig(&m_Config,m_ClientSocket);
					if (dlg.DoModal() == IDCANCEL)
					{
						m_bStart = FALSE;
						return;
					}
				}

			}
			break;

		case WorkMode_ImageCopy:
			{

				m_bServerFirst = FALSE;
				if (!m_bBurnInTest)
				{
					CImageNameDlg dlg(FALSE);
					dlg.SetConfig(&m_Config,m_ClientSocket);
					if (dlg.DoModal() == IDCANCEL)
					{
						m_bStart = FALSE;
						return;
					}

					m_bServerFirst = dlg.GetServerFirst();
				}	

			}
			break;

		case WorkMode_DifferenceCopy:
			{
				// ����Ǵӷ���ˣ���Ҫ�������ذ�����
				if (m_Config.GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Package)
				{
					CPackageName dlg;
					dlg.SetConfig(&m_Config,&m_MasterPort,m_ClientSocket);

					if (dlg.DoModal() == IDCANCEL)
					{
						m_bStart = FALSE;
						return;
					}
					
				}
			}
			break;

		case WorkMode_Burnin_Test:
			SendMessage(WM_BURN_IN_TEST);
			m_bStart = FALSE;
			return;
		}

		m_bCancel = FALSE;
		m_bResult = TRUE;
		m_bRunning = TRUE;

		m_bStart = TRUE;
		m_BtnStart.SetBitmaps(IDB_STOP,RGB(255,255,255));
		EnableControls(FALSE);

		//strResText.LoadString(IDS_BTN_STOP);
		//SetDlgItemText(IDC_BTN_START,strResText);

		if (m_ThreadListen && m_ThreadListen->m_hThread)
		{
			WaitForSingleObject(m_ThreadListen->m_hThread,INFINITE);
			delete m_ThreadListen;
			m_ThreadListen = NULL;
		}

		CString strWorkMode = GetWorkModeString(m_WorkMode);
		//GetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);
		CUtils::WriteLogFile(m_hLogFile,FALSE,_T(""));
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("%s begain......"),strWorkMode);

		SetDlgItemText(IDC_TEXT_FUNCTION2,strWorkMode);

		// �����NGFF˳���ϵ�
		if (m_nMachineType == MT_NGFF)
		{
			// ȫ���ϵ�
			for (UINT i = 0; i < m_nPortNum;i++)
			{
				m_Command.Power(i,TRUE);
				Sleep(100);
				m_Command.GreenLight(0,TRUE);
				Sleep(100);
			}
		}

		CScanningDlg dlg;
		dlg.SetLogFile(m_hLogFile);
		dlg.SetConfig(&m_Config,m_WorkMode,m_nMachineType);
		dlg.SetDeviceInfoList(&m_MasterPort,&m_TargetPorts);
		dlg.SetBegining(TRUE);
		dlg.DoModal();

		// �ж��Ƿ���Կ���
		if (!IsReady())
		{
			// ȫ��FAIL
			m_Command.AllFail();

			m_bRunning = FALSE;

			MSG msg;
			while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
			{
				DispatchMessage(&msg);
				TranslateMessage(&msg); 
			}

			PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_START, BN_CLICKED), (LPARAM)m_hWnd); 
			return;
		}

		// �ж϶˿�״̬����Щֱ��FAIL����Щ���Թ���
		POSITION pos = NULL;
		switch (m_WorkMode)
		{
			//ֻ��Ҫĸ��
		case WorkMode_ImageMake:
			m_Command.GreenLightFlash(0);
			break;

			//ֻ��Ҫ����
		case WorkMode_DiskClean:
		case WorkMode_ImageCopy:
		case WorkMode_DiskFormat:
		case WorkMode_Full_RW_Test:
		case WorkMode_Fade_Picker:
		case WorkMode_Speed_Check:
			pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					m_Command.GreenLightFlash(port->GetPortNum());
				}
				else
				{
					m_Command.RedLight(port->GetPortNum(),TRUE);

					// �����NGFF,��������ʱ����Ҫ�ϵ�
					if (m_nMachineType == MT_NGFF)
					{
						Sleep(100);
						m_Command.Power(port->GetPortNum(),FALSE);
					}
				}
			}
			break;

		case WorkMode_DifferenceCopy:
			pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					m_Command.GreenLightFlash(port->GetPortNum());
				}
				else
				{
					m_Command.RedLight(port->GetPortNum(),TRUE);

					// �����NGFF,��������ʱ����Ҫ�ϵ�
					if (m_nMachineType == MT_NGFF)
					{
						Sleep(100);
						m_Command.Power(port->GetPortNum(),FALSE);
					}
				}
			}

			if (m_Config.GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Master
				|| m_Config.GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0) == PathType_Local)
			{
				m_Command.GreenLightFlash(0);
			}

			break;

		default:
			m_Command.GreenLightFlash(0);

			pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					m_Command.GreenLightFlash(port->GetPortNum());
				}
				else
				{
					m_Command.RedLight(port->GetPortNum(),TRUE);

					// �����NGFF,��������ʱ����Ҫ�ϵ�
					if (m_nMachineType == MT_NGFF)
					{
						Sleep(100);
						m_Command.Power(port->GetPortNum(),FALSE);
					}
				}
			}

			break;

		}

		// ʹ����˸����
		m_Command.EnableFlashLight();

		AfxBeginThread((AFX_THREADPROC)StartThreadProc,this);
	}

}

void CUSBCopyDlg::OnStop()
{
	m_bCancel = TRUE;

	while(m_bRunning)
	{
		SwitchToThread();
		Sleep(100);
	}

	CString strLog = GetUploadLogString();
	WriteUploadLog(strLog);
	// �ϴ�log
	BOOL bUpload = m_Config.GetBool(_T("Option"),_T("En_UploadLogAuto"),FALSE);

	if (bUpload)
	{
		if (!m_bSockeConnected)
		{
			SendMessage(WM_CONNECT_SOCKET);
		}

		CString strFileName,strMachineSN;
		GetDlgItemText(IDC_TEXT_SN,strMachineSN);
		strFileName.Format(_T("record_%s_%s.txt"),strMachineSN,CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("upload log start..."));

		DWORD dwUpload = UploadLog(strFileName,strLog);

		while (dwUpload != 0)
		{
			CString strMsg,strFail;
			strFail.LoadString(IDS_MSG_UPLOAD_FAILED);
			strMsg.Format(_T("%s%s"),CUtils::GetErrorMsg(dwUpload),strFail);
			CString strTitle;
			strTitle.LoadString(IDS_MSG_TITLE_UPLOAD);
			if (MessageBox(strMsg,strTitle,MB_YESNO | MB_DEFBUTTON1 | MB_ICONERROR) == IDNO)
			{
				break;
			}

			if (m_ClientSocket != INVALID_SOCKET)
			{
				closesocket(m_ClientSocket);
				m_ClientSocket = INVALID_SOCKET;
			}
			
			SendMessage(WM_CONNECT_SOCKET);

			dwUpload = UploadLog(strFileName,strLog);
		}

		if (dwUpload != 0)
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("upload log faild."));
			m_bResult = FALSE;
			m_strMsg += _T(" Upload log failed !");

			// ���������ó�fail
			SetAllFailed(0,ErrorType_System);
		}

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("upload log success."));
	}

	UpdatePortFrame(FALSE);
	// �������BurnIn Testֻ��ȫ��FAILʱ������ɶԻ���
	if (m_bBurnInTest)
	{
		if (!m_bResult)
		{
			// ���������Ի���
			CCompleteMsg completeMsg(&m_Config,&m_Command,m_strMsg,m_bResult);
			completeMsg.DoModal();
		}

		SetEvent(m_hBurninEvent);

	}
	else
	{
		// ���������Ի���
		m_BtnStart.SetBitmaps(IDB_START,RGB(255,255,255));
		CCompleteMsg completeMsg(&m_Config,&m_Command,m_strMsg,m_bResult);
		completeMsg.DoModal();

		//��λ
		SendMessage(WM_RESET_MACHIEN_PORT);

		EnableControls(TRUE);
		GetDlgItem(IDC_BTN_START)->SetFocus();

		m_bRunning = FALSE;
	}

	m_bEnableButton = TRUE;
}

void CUSBCopyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	switch (nIDEvent)
	{
	case TIMER_UPDATE_STATISTIC:
		UpdateStatisticInfo();
		break;

	case TIMER_LISENCE:
		KillTimer(nIDEvent);
		if (!IsLisence())
		{
			m_bLisence = FALSE;
			m_bEnableButton = FALSE;
			CString strResText;
			strResText.LoadString(IDS_MSG_LISENCE_FAILED);
			MessageBox(strResText,_T("USBCopy"),MB_ICONERROR | MB_SETFOREGROUND);

			GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);

		}
		else
		{
			m_bLisence = TRUE;
			m_bEnableButton = TRUE;
		}
		break;
		
	case TIMER_CONNECT_SERVER:
		AfxBeginThread((AFX_THREADPROC)ConnectSocketThreadProc,this);
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CUSBCopyDlg::UpdateStatisticInfo()
{
	// ����Make Image֮�⣬��������ĸ��
	int iMinPercent = 100; //m_MasterPort.GetPercent();
	ULONGLONG ullMinCompleteSize = -1; //m_MasterPort.GetCompleteSize();
	ULONGLONG ullMaxValidSize = 0; //m_MasterPort.GetValidSize();

	int nIndex = 0;
	/*
	if (m_WorkMode == WorkMode_DiskClean || m_WorkMode == WorkMode_DiskFormat)
	{
		iMinPercent = 100;
		ullMinCompleteSize = -1;
		ullMaxValidSize = 0;
	}
	else if (m_WorkMode == WorkMode_ImageCopy
		|| (m_WorkMode == WorkMode_DifferenceCopy && m_Config.GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Package
		&& m_Config.GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0) == PathType_Remote))
	{
		iMinPercent = m_FilePort.GetPercent();
		ullMinCompleteSize = m_FilePort.GetCompleteSize();
		ullMaxValidSize = m_FilePort.GetValidSize();
	}
	else
	{
		nIndex++;
	}
	*/

	if (m_WorkMode == WorkMode_DiskCompare)
	{
		iMinPercent = m_MasterPort.GetPercent();
		ullMinCompleteSize = m_MasterPort.GetCompleteSize();
		ullMaxValidSize = m_MasterPort.GetValidSize();
		nIndex++;
	}

	// ���¼���ģʽ�����ٶȱȷֱȺ�ʣ��ʱ��ͳ��
	if (m_WorkMode == WorkMode_Speed_Check)
	{
		CTime time = CTime::GetCurrentTime();
		CTimeSpan spanTotal = time - m_StartTime;
		SetDlgItemText(IDC_TEXT_TIME_ELAPSED2,spanTotal.Format(_T("%H:%M:%S")));

		return;
	}


	if (m_WorkMode != WorkMode_ImageMake)
	{
		// ����������
		POSITION pos = m_TargetPorts.GetHeadPosition();
		while (pos)
		{
			CPort *port = m_TargetPorts.GetNext(pos);

			if (port->IsConnected())
			{
				if (port->GetResult() && port->GetPortState() == PortState_Active)
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

					nIndex++;
				}
				
			}
		}

		// ȫ�����
		if (nIndex == 0)
		{
			pos = m_TargetPorts.GetHeadPosition();

			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);

				if (port->IsConnected())
				{
					if (port->GetResult() && port->GetPortState() != PortState_Active)
					{
						if (port->GetPercent() < iMinPercent)
						{
							iMinPercent = port->GetPercent();
						}

						if (port->GetValidSize() > ullMaxValidSize)
						{
							ullMaxValidSize = port->GetValidSize();
							ullMinCompleteSize = ullMaxValidSize;
						}

						nIndex++;
					}
				}
			}
		}

	}
	else
	{
		if (m_FilePort.GetPercent() < iMinPercent)
		{
			iMinPercent = m_FilePort.GetPercent();
		}

		if (m_FilePort.GetCompleteSize() < ullMinCompleteSize)
		{
			ullMinCompleteSize = m_FilePort.GetCompleteSize();
		}

		if (m_FilePort.GetValidSize() > ullMaxValidSize)
		{
			ullMaxValidSize = m_FilePort.GetValidSize();
		}

		nIndex++;
	}

	if (nIndex > 0)
	{
		if (ullMaxValidSize > 0)
		{
			int nPercent = (int)(ullMinCompleteSize * 100 / ullMaxValidSize);
			m_ProgressCtrl.SetPos(nPercent);
		}

		CString strText;
		strText.Format(_T("%s / %s"),CUtils::AdjustFileSize(ullMinCompleteSize),CUtils::AdjustFileSize(ullMaxValidSize));
		SetDlgItemText(IDC_TEXT_USAGE,strText);

		CTime time = CTime::GetCurrentTime();
		CTimeSpan spanTotal = time - m_StartTime;
		CTimeSpan spanTemp = time - m_StartTimeTemp;
		SetDlgItemText(IDC_TEXT_TIME_ELAPSED2,spanTotal.Format(_T("%H:%M:%S")));

		ULONGLONG ullCompleteSize = ullMinCompleteSize;

		ULONGLONG ullAvgSpeed = 0;

		if (spanTemp.GetTotalSeconds() > 0)
		{
			ullAvgSpeed = ullCompleteSize / spanTemp.GetTotalSeconds();
		}

		// ����5s��ʹ�����̹���
		if (spanTotal.GetTotalSeconds() > 5)
		{
			EnableKickOff(TRUE);
		}
		else
		{
			EnableKickOff(FALSE);
		}

		strText = CUtils::AdjustSpeed(ullAvgSpeed);
		SetDlgItemText(IDC_TEXT_SPEED2,strText);

		if (ullAvgSpeed > 0)
		{
			__time64_t time = (__time64_t)((ullMaxValidSize - ullMinCompleteSize)/ullAvgSpeed);
			CTimeSpan spanR(time);
			SetDlgItemText(IDC_TEXT_TIME_REMAINNING2,spanR.Format(_T("%H:%M:%S")));
		}

	}
// 	else
// 	{
// 		m_ProgressCtrl.SetPos(100);
// 	}
}

BOOL CUSBCopyDlg::IsReady()
{
	CString strWorkMode,strMsg;
	//GetDlgItemText(IDC_BTN_WORK_SELECT,strWorkMode);
	strWorkMode = GetWorkModeString(m_WorkMode);

	BOOL bRet = TRUE;

	switch (m_WorkMode)
	{
	case WorkMode_ImageCopy:
	case WorkMode_DiskClean:
	case WorkMode_DiskFormat:
	case WorkMode_Fade_Picker:
	case WorkMode_Full_RW_Test:
	case WorkMode_Speed_Check:
		if (!IsExistTarget())
		{
			strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Targets."),strWorkMode,CustomError_No_Target);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;

			SetAllFailed(CustomError_No_Target,ErrorType_Custom);
		}
		break;

	case WorkMode_DifferenceCopy:
		{
			if (!IsExistTarget())
			{
				strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Targets."),strWorkMode,CustomError_No_Target);
				CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

				bRet = FALSE;

				SetAllFailed(CustomError_No_Target,ErrorType_Custom);
			}

			if (m_Config.GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Master
				|| m_Config.GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0) == PathType_Local)
			{
				if (!IsExistMaster())
				{
					strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Master."),strWorkMode,CustomError_No_Master);
					CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

					SetAllFailed(CustomError_No_Master,ErrorType_Custom);

					bRet = FALSE;
				}
			}
		}
		break;

	case WorkMode_ImageMake:
		if (!IsExistMaster())
		{
			strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Master."),strWorkMode,CustomError_No_Master);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			bRet = FALSE;

			SetAllFailed(CustomError_No_Master,ErrorType_Custom);
		}
		break;

	default:
		if (!IsExistMaster())
		{
			strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Master."),strWorkMode,CustomError_No_Master);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			SetAllFailed(CustomError_No_Master,ErrorType_Custom);

			bRet = FALSE;
		}

		if (!IsExistTarget())
		{
			strMsg.Format(_T("%s failed!\r\nCustom errorcode=0x%X, No Targets."),strWorkMode,CustomError_No_Target);
			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

			SetAllFailed(CustomError_No_Target,ErrorType_Custom);

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
	if (m_bBurnInTest)
	{
		m_StartTimeTemp = CTime::GetCurrentTime();
	}
	else
	{
		m_StartTimeTemp = m_StartTime = CTime::GetCurrentTime();
	}
	
	// ��ʼ�߳�
	SetTimer(TIMER_UPDATE_STATISTIC,1000,NULL);

	HashMethod hashMethod = (HashMethod)m_Config.GetInt(_T("Option"),_T("HashMethod"),0);
	UINT nBlockSectors = m_Config.GetUInt(_T("Option"),_T("BlockSectors"),256); // Ĭ��128KB
	BOOL bMustSameCapacity = m_Config.GetBool(_T("Option"),_T("En_SameCapacity"),FALSE);

	CDisk disk;
	disk.Init(m_hWnd,&m_bCancel,m_hLogFile,&m_Command,nBlockSectors);
	disk.SetWorkMode(m_WorkMode);
	disk.SetGlobalParm(bMustSameCapacity);

	CString strMsg,strWorkMode;
	BOOL bResult = TRUE;

	strWorkMode = GetWorkModeString(m_WorkMode);

	switch(m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("FullCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("FullCopy"),_T("En_Compare"),FALSE);
			CompareMethod compareMothod = (CompareMethod)m_Config.GetInt(_T("FullCopy"),_T("CompareMethod"),0);

			BOOL bAllowCapGap = m_Config.GetBool(_T("FullCopy"),_T("En_AllowCapaGap"),FALSE);
			UINT nPercent = m_Config.GetUInt(_T("FullCopy"),_T("CapacityGap"),0);
			BOOL bCleanDiskFirst = m_Config.GetBool(_T("FullCopy"),_T("En_CleanDiskFirst"),FALSE);
			BOOL bCompareClean = m_Config.GetBool(_T("FullCopy"),_T("En_CompareClean"),FALSE);
			UINT nCleanTimes = m_Config.GetUInt(_T("FullCopy"),_T("CleanTimes"),1);
			CString strFillValues = m_Config.GetString(_T("FullCopy"),_T("FillValues"));
			CompareCleanSeq seq = (CompareCleanSeq)m_Config.GetInt(_T("FullCopy"),_T("CompareCleanSeq"),0);

			if (nCleanTimes < 0 || nCleanTimes > 3)
			{
				nCleanTimes = 1;
			}

			int *pCleanFillValues = new int[nCleanTimes];
			memset(pCleanFillValues,0,nCleanTimes * sizeof(int));

			if (bCleanDiskFirst)
			{
				int nCurPos = 0;
		
				CString strToken;
				UINT nCount = 0;

				strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				while (nCurPos != -1 && nCount < nCleanTimes)
				{
					int nFillValue = _tcstoul(strToken, NULL, 16);

					if (nFillValue == 0 && strToken.GetAt(0) != _T('0'))
					{
						nFillValue = RANDOM_VALUE;
					}

					pCleanFillValues[nCount++] = nFillValue;

					strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				}
			}
			
			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,hashMethod);
			disk.SetCompareParm(bCompare,compareMothod);
			disk.SetFullCopyParm(bAllowCapGap,nPercent);
			disk.SetCleanDiskFirst(bCleanDiskFirst,bCompareClean,seq,nCleanTimes,pCleanFillValues);
			delete []pCleanFillValues;
			
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
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !!!"),strWorkMode);
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
				
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;
	case WorkMode_QuickCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("QuickCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE);
			CompareMethod compareMethod = (CompareMethod)m_Config.GetInt(_T("QuickCopy"),_T("CompareMethod"),0);

			BOOL bCleanDiskFirst = m_Config.GetBool(_T("QuickCopy"),_T("En_CleanDiskFirst"),FALSE);
			BOOL bCompareClean = m_Config.GetBool(_T("QuickCopy"),_T("En_CompareClean"),FALSE);
			UINT nCleanTimes = m_Config.GetUInt(_T("QuickCopy"),_T("CleanTimes"),1);
			CString strFillValues = m_Config.GetString(_T("QuickCopy"),_T("FillValues"));
			CompareCleanSeq seq = (CompareCleanSeq)m_Config.GetInt(_T("QuickCopy"),_T("CompareCleanSeq"),0);

			int nNumOfArea = m_Config.GetInt(_T("QuickCopy"),_T("NumOfArea"),0);

			PRANGE_FROM_TO ranges = NULL;

			if (nNumOfArea > 0)
			{
				ranges = new RANGE_FROM_TO[nNumOfArea];

				CString strKey;
				ULONGLONG ullStartLBA = 0, ullEndLBA = 0;
				for (int i = 0; i < nNumOfArea;i++)
				{
					strKey.Format(_T("StartLBA_%d"),i);

					ullStartLBA = m_Config.GetUInt64(_T("QuickCopy"),strKey,0);

					strKey.Format(_T("EndLBA_%d"),i);

					ullEndLBA = m_Config.GetUInt64(_T("QuickCopy"),strKey,0);

					ranges[i].ullStartLBA = ullStartLBA;
					ranges[i].ullEndingLBA = ullEndLBA;
				}
			}

			if (nCleanTimes < 0 || nCleanTimes > 3)
			{
				nCleanTimes = 1;
			}

			int *pCleanFillValues = new int[nCleanTimes];
			memset(pCleanFillValues,0,nCleanTimes * sizeof(int));

			if (bCleanDiskFirst)
			{
				int nCurPos = 0;

				CString strToken;
				UINT nCount = 0;
				strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				while (nCurPos != -1 && nCount < nCleanTimes)
				{
					int nFillValue = _tcstoul(strToken, NULL, 16);

					if (nFillValue == 0 && strToken.GetAt(0) != _T('0'))
					{
						nFillValue = RANDOM_VALUE;
					}

					pCleanFillValues[nCount++] = nFillValue;

					strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				}
			}

			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,hashMethod);
			disk.SetCompareParm(bCompare,compareMethod);
			disk.SetCleanDiskFirst(bCleanDiskFirst,bCompareClean,seq,nCleanTimes,pCleanFillValues);
			disk.SetQuickCopyParm(ranges,nNumOfArea);

			if (ranges)
			{
				delete []ranges;
			}
			delete []pCleanFillValues;

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
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !!!"),strWorkMode);
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_DiskClean:
		{
			CleanMode cleanMode = (CleanMode)m_Config.GetInt(_T("DiskClean"),_T("CleanMode"),0);
			int nFillVal = m_Config.GetInt(_T("DiskClean"),_T("FillValue"),0,16);
			BOOL bCompareClean = m_Config.GetBool(_T("DiskClean"),_T("En_CompareClean"),FALSE);
			CompareCleanSeq seq = (CompareCleanSeq)m_Config.GetInt(_T("DiskClean"),_T("CompareCleanSeq"),0);

			// ���ö˿�״̬
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetCleanMode(cleanMode,nFillVal,bCompareClean,seq);

			bResult = disk.Start();

			if (bResult)
			{
				strMsg.Format(_T("%s Completed !!!"),strWorkMode);
			}
			else
			{
				// ����ȡһ������
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
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_DiskCompare:
		{
			CompareMode compareMode = (CompareMode)m_Config.GetInt(_T("DiskCompare"),_T("CompareMode"),0);
			CompareMethod compareMethod = (CompareMethod)m_Config.GetInt(_T("DiskCompare"),_T("CompareMethod"),0);

			BOOL bComputeHash = TRUE;

			if (compareMethod == CompareMethod_Byte)
			{
				bComputeHash = FALSE;
			}
			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,hashMethod);
			disk.SetCompareParm(TRUE,compareMethod);
			disk.SetCompareMode(compareMode);

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
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !"),strWorkMode);
				}
				

			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_ImageMake:
		{
			CString strImagePath = m_Config.GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
			CString strImageName = m_Config.GetString(_T("ImagePath"),_T("ImageName"));
			BOOL bServerFirst = m_Config.GetBool(_T("ImageMake"),_T("SavePath"),FALSE);
			int compressLevel = m_Config.GetInt(_T("ImageMake"),_T("CompressLevel"),1);
			UINT nImageMode = m_Config.GetInt(_T("ImageMake"),_T("SaveMode"),0);
			BOOL bDataCompress = m_Config.GetBool(_T("ImageMake"),_T("En_DataCompress"),TRUE);
			//BOOL bMtpImage = (nImageMode == 2);

			int nNumOfArea = m_Config.GetInt(_T("ImageMake"),_T("NumOfArea"),0);

			PRANGE_FROM_TO ranges = NULL;

			if (nNumOfArea > 0)
			{
				ranges = new RANGE_FROM_TO[nNumOfArea];

				CString strKey;
				ULONGLONG ullStartLBA = 0, ullEndLBA = 0;
				for (int i = 0; i < nNumOfArea;i++)
				{
					strKey.Format(_T("StartLBA_%d"),i);

					ullStartLBA = m_Config.GetUInt64(_T("ImageMake"),strKey,0);

					strKey.Format(_T("EndLBA_%d"),i);

					ullEndLBA = m_Config.GetUInt64(_T("ImageMake"),strKey,0);

					ranges[i].ullStartLBA = ullStartLBA;
					ranges[i].ullEndingLBA = ullEndLBA;
				}
			}

			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			strImagePath.TrimRight(_T('\\'));

			switch (nImageMode)
			{
			case FULL_IMAGE: //����ӳ��
			case QUICK_IMAGE:
				if (strImageName.Right(4).CompareNoCase(_T(".IMG")) != 0)
				{
					strImageName += _T(".IMG");
				}
				break;

			case MTP_IMAGE: //MTPӳ��
				if (strImageName.Right(4).CompareNoCase(_T(".MTP")) != 0)
				{
					strImageName += _T(".MTP");
				}
				break;

			case FILE_IMAGE: //�ļ�ӳ��
				if (strImageName.Right(4).CompareNoCase(_T(".FIL")) != 0)
				{
					strImageName += _T(".FIL");
				}
				break;
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

			if (nImageMode == MTP_IMAGE)
			{
				CWpdDevice wpdDevice;
				wpdDevice.Init(m_hWnd,&m_bCancel,m_hLogFile,&m_Command);
				wpdDevice.SetWorkMode(WorkMode_ImageMake);

				wpdDevice.SetMasterPort(&m_MasterPort);
				wpdDevice.SetTargetPorts(&filePortList);
				wpdDevice.SetHashMethod(TRUE,hashMethod);
				wpdDevice.SetSocket(m_ClientSocket,bServerFirst);
				wpdDevice.SetMakeImageParm(bDataCompress,compressLevel);

				bResult = wpdDevice.Start();
			}
			else
			{
				disk.SetMasterPort(&m_MasterPort);
				disk.SetTargetPorts(&filePortList);
				disk.SetHashMethod(TRUE,hashMethod);
				disk.SetSocket(m_ClientSocket,bServerFirst);
				disk.SetMakeImageParm(nImageMode,bDataCompress,compressLevel);
				disk.SetQuickCopyParm(ranges,nNumOfArea);

				bResult = disk.Start();
			}

			if (ranges)
			{
				delete []ranges;
			}

			CString strHashValue;
			if (bResult)
			{
				MoveFileEx(strTempFile,strImageFile,MOVEFILE_REPLACE_EXISTING);

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
				strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
					,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				
			}
			else
			{
				DeleteFile(strTempFile);

				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
					errType = m_FilePort.GetErrorCode(&dwErrorCode);
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case  WorkMode_ImageCopy:
		{
			BOOL bCompare = m_Config.GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE);
			CompareMethod compareMethod = (CompareMethod)m_Config.GetInt(_T("ImageCopy"),_T("CompareMethod"),0);
			UINT nImageType = m_Config.GetBool(_T("ImageCopy"),_T("ImageType"),0);

			BOOL bCleanDiskFirst = m_Config.GetBool(_T("ImageCopy"),_T("En_CleanDiskFirst"),FALSE);
			BOOL bCompareClean = m_Config.GetBool(_T("ImageCopy"),_T("En_CompareClean"),FALSE);
			UINT nCleanTimes = m_Config.GetUInt(_T("ImageCopy"),_T("CleanTimes"),1);
			CString strFillValues = m_Config.GetString(_T("ImageCopy"),_T("FillValues"));
			CompareCleanSeq seq = (CompareCleanSeq)m_Config.GetInt(_T("ImageCopy"),_T("CompareCleanSeq"),0);

			//BOOL bMtpImage = (nImageType == 1);

			if (nCleanTimes < 0 || nCleanTimes > 3)
			{
				nCleanTimes = 1;
			}

			int *pCleanFillValues = new int[nCleanTimes];
			memset(pCleanFillValues,0,nCleanTimes * sizeof(int));

			if (bCleanDiskFirst)
			{
				int nCurPos = 0;

				CString strToken;
				UINT nCount = 0;

				strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				while (nCurPos != -1 && nCount < nCleanTimes)
				{
					int nFillValue = _tcstoul(strToken, NULL, 16);

					if (nFillValue == 0 && strToken.GetAt(0) != _T('0'))
					{
						nFillValue = RANDOM_VALUE;
					}

					pCleanFillValues[nCount++] = nFillValue;

					strToken = strFillValues.Tokenize(_T(";"),nCurPos);
				}
			}

			// ���ö˿�״̬
			CString strImagePath = m_Config.GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
			CString strImageName = m_Config.GetString(_T("ImagePath"),_T("ImageName"));
			strImagePath.TrimRight(_T('\\'));

			switch (nImageType)
			{
			case 0: //����ӳ��
				if (strImageName.Right(4).CompareNoCase(_T(".IMG")) != 0)
				{
					strImageName += _T(".IMG");
				}
				break;

			case 1: //MTPӳ��
				if (strImageName.Right(4).CompareNoCase(_T(".MTP")) != 0)
				{
					strImageName += _T(".MTP");
				}
				break;

			case 2: //�ļ�ӳ��
				if (strImageName.Right(4).CompareNoCase(_T(".FIL")) != 0)
				{
					strImageName += _T(".FIL");
				}
				break;
			}

			CString strImageFile,strTempFile;
			strTempFile.Format(_T("%s\\%s.$$$"),strImagePath,strImageName.Left(strImageName.GetLength() - 4));
			strImageFile.Format(_T("%s\\%s"),strImagePath,strImageName);

			if (m_bServerFirst)
			{
				m_FilePort.SetPortType(PortType_SERVER);
			}
			else
			{
				m_FilePort.SetPortType(PortType_MASTER_FILE);
			}
			
			m_FilePort.SetFileName(strImageFile);
			m_FilePort.SetConnected(TRUE);
			m_FilePort.SetPortState(PortState_Online);
			m_FilePort.SetWorkMode(m_WorkMode);
			m_FilePort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			// if
			if (1 == nImageType)  //MTP Image
			{
				CWpdDevice wpdDevice;
				wpdDevice.Init(m_hWnd,&m_bCancel,m_hLogFile,&m_Command);
				wpdDevice.SetWorkMode(WorkMode_ImageCopy);

				wpdDevice.SetMasterPort(&m_FilePort);
				wpdDevice.SetTargetPorts(&m_TargetPorts);
				wpdDevice.SetHashMethod(TRUE,hashMethod);
				wpdDevice.SetCompareParm(bCompare,CompareMethod_Hash);
				wpdDevice.SetSocket(m_ClientSocket,m_bServerFirst);

				bResult = wpdDevice.Start();
			}
			else
			{
				disk.SetMasterPort(&m_FilePort);
				disk.SetTargetPorts(&m_TargetPorts);
				disk.SetHashMethod(TRUE,hashMethod);
				disk.SetCompareParm(bCompare,compareMethod);
				disk.SetSocket(m_ClientSocket,m_bServerFirst);
				disk.SetCleanDiskFirst(bCleanDiskFirst,bCompareClean,seq,nCleanTimes,pCleanFillValues);
				delete []pCleanFillValues;

				bResult = disk.Start();
			}
			
			CString strHashValue;
			if (bResult)
			{

				MoveFileEx(strTempFile,strImageFile,MOVEFILE_REPLACE_EXISTING);

				int len = m_FilePort.GetHashLength();
				BYTE *pHash = new BYTE[len];
				ZeroMemory(pHash,len);
				m_FilePort.GetHash(pHash,len);
				for (int i = 0;i < len;i++)
				{
					CString strHash;
					strHash.Format(_T("%02X"),pHash[i]);

					strHashValue += strHash;
				}
				delete []pHash;
				strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
					,strWorkMode,m_FilePort.GetHashMethodString(),strHashValue);
				
			}
			else
			{
				DeleteFile(strTempFile);

				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_FilePort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		
		break;

	case WorkMode_FileCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("FileCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("FileCopy"),_T("En_Compare"),FALSE);
			CompareMethod compareMethod = (CompareMethod)m_Config.GetInt(_T("FileCopy"),_T("CompareMethod"),0);
			BOOL bCopyAllFiles = m_Config.GetBool(_T("FileCopy"),_T("En_CopyAllFiles"),TRUE);
			BOOL bCleanupTargets = m_Config.GetBool(_T("FileCopy"),_T("En_CleanupTargets"),FALSE);
			
			UINT nNumOfFolders = m_Config.GetUInt(_T("FileCopy"),_T("NumOfFolders"),0);
			UINT nNumOfFiles = m_Config.GetUInt(_T("FileCopy"),_T("NumOfFiles"),0);

			
			CStringArray filesArray,folderArray;
			CString strKey,strFile,strPath;

			if (bCopyAllFiles)
			{
				strPath = MASTER_PATH;

				folderArray.Add(strPath);
			}
			else
			{
				for (UINT i = 0;i < nNumOfFolders;i++)
				{
					strKey.Format(_T("Folder_%d"),i);
					strFile = m_Config.GetString(_T("FileCopy"),strKey);

					if (strFile.IsEmpty())
					{
						continue;
					}

					strPath = MASTER_PATH + strFile;

					if (PathFileExists(strPath))
					{
						folderArray.Add(strPath);
					}
					else
					{
						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("File Copy,folder %s doesn't exist."),strPath);
					}

				}

				for (UINT i = 0;i < nNumOfFiles;i++)
				{
					strKey.Format(_T("File_%d"),i);
					strFile = m_Config.GetString(_T("FileCopy"),strKey);

					if (strFile.IsEmpty())
					{
						continue;
					}

					strPath = MASTER_PATH + strFile;

					if (PathFileExists(strPath))
					{
						filesArray.Add(strPath);
					}
					else
					{
						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("File Copy,file %s doesn't exist."),strPath);
					}

				}
			}
			

			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetMasterPort(&m_MasterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,hashMethod);
			disk.SetCompareParm(bCompare,compareMethod);
			disk.SetFileAndFolder(bCleanupTargets,filesArray,folderArray);

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
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !!!"),strWorkMode);
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	case WorkMode_DiskFormat:
		{
			CString strVolumeLable = m_Config.GetString(_T("DiskFormat"),_T("VolumeLabel"));
			FileSystem fileSystem = (FileSystem)m_Config.GetUInt(_T("DiskFormat"),_T("FileSystem"),FileSystem_FAT32);
			UINT nClusterSize = m_Config.GetUInt(_T("DiskFormat"),_T("ClusterSize"),0);
			BOOL bQuickFormat = m_Config.GetBool(_T("DiskFormat"),_T("QuickFormat"),TRUE);

			// ���ö˿�״̬
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetFormatParm(strVolumeLable,fileSystem,nClusterSize,TRUE);

			bResult = disk.Start();

			if (bResult)
			{
				strMsg.Format(_T("%s Completed !!!"),strWorkMode);
			}
			else
			{
				// ����ȡһ������
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
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	case WorkMode_DifferenceCopy:
		{
			BOOL bUploadUserLog = m_Config.GetBool(_T("DifferenceCopy"),_T("En_UserLog"),FALSE);
			BOOL bComputeHash = m_Config.GetBool(_T("DifferenceCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("DifferenceCopy"),_T("En_Compare"),FALSE);
			BOOL bIncludeSubDir = m_Config.GetBool(_T("DifferenceCopy"),_T("En_IncludeSub"),FALSE);
			UINT nSourceType = m_Config.GetUInt(_T("DifferenceCopy"),_T("SourceType"),0);
			UINT nCompareRule = m_Config.GetUInt(_T("DifferenceCopy"),_T("CompareRule"),0);
			UINT nPkgLocation = m_Config.GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0);
			CString strPackageName = m_Config.GetString(_T("DifferenceCopy"),_T("PkgName"));
			CStringArray strArrayLog;
			
			if (bUploadUserLog)
			{
				UINT nNumOfLogPath = m_Config.GetBool(_T("DifferenceCopy"),_T("NumOfLogPath"),FALSE);

				CString strKey,strValue;

				for (UINT i = 0;i < nNumOfLogPath;i++)
				{
					strKey.Format(_T("LogPath_%d"),i);
					strValue = m_Config.GetString(_T("DifferenceCopy"),strKey);
				
					if (strValue.Find(_T(":")))
					{
						strArrayLog.Add(strValue);
					}
				}
			}


			CPort *masterPort = NULL;
			if (nSourceType == SourceType_Package)
			{
				if (nPkgLocation == PathType_Local)
				{
					masterPort = &m_MasterPort;

					strPackageName = MASTER_PATH + strPackageName;
				}
				else
				{
					masterPort = &m_FilePort;
				}
			}
			else
			{
				nPkgLocation = PathType_Local;
				masterPort = &m_MasterPort;
			}


			// ���ö˿�״̬
			masterPort->SetFileName(strPackageName);
			masterPort->SetConnected(TRUE);
			masterPort->SetPortState(PortState_Online);
			masterPort->SetHashMethod(hashMethod);
			masterPort->SetWorkMode(m_WorkMode);
			masterPort->SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetMasterPort(masterPort);
			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetHashMethod(bComputeHash,hashMethod);
			disk.SetCompareParm(bCompare,CompareMethod_Hash);
			disk.SetSocket(m_ClientSocket,nPkgLocation);
			disk.SetDiffCopyParm(nSourceType,nPkgLocation,nCompareRule,bUploadUserLog,strArrayLog,bIncludeSubDir);

			bResult = disk.Start();

			CString strHashValue;
			if (bResult)
			{
				if (bComputeHash)
				{
					int len = masterPort->GetHashLength();
					BYTE *pHash = new BYTE[len];
					ZeroMemory(pHash,len);
					masterPort->GetHash(pHash,len);
					for (int i = 0;i < len;i++)
					{
						CString strHash;
						strHash.Format(_T("%02X"),pHash[i]);

						strHashValue += strHash;
					}
					delete []pHash;
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,masterPort->GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !!!"),strWorkMode);
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = masterPort->GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);

		}
		break;

	case WorkMode_MTPCopy:
		{
			BOOL bComputeHash = m_Config.GetBool(_T("MTPCopy"),_T("En_ComputeHash"),FALSE);
			BOOL bCompare = m_Config.GetBool(_T("MTPCopy"),_T("En_Compare"),FALSE);

			// ���ö˿�״̬
			m_MasterPort.SetHashMethod(hashMethod);
			m_MasterPort.SetWorkMode(m_WorkMode);
			m_MasterPort.SetStartTime(m_StartTime);

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			CWpdDevice wpdDevice;
			wpdDevice.Init(m_hWnd,&m_bCancel,m_hLogFile,&m_Command);
			wpdDevice.SetWorkMode(WorkMode_MTPCopy);

			wpdDevice.SetMasterPort(&m_MasterPort);
			wpdDevice.SetTargetPorts(&m_TargetPorts);
			wpdDevice.SetHashMethod(bComputeHash,hashMethod);
			wpdDevice.SetCompareParm(bCompare,CompareMethod_Hash);

			bResult = wpdDevice.Start();

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
					strMsg.Format(_T("%s Completed !\r\nHashMethod=%s, HashValue=%s")
						,strWorkMode,m_MasterPort.GetHashMethodString(),strHashValue);
				}
				else
				{
					strMsg.Format(_T("%s Completed !!!"),strWorkMode);
				}
			}
			else
			{
				ErrorType errType = ErrorType_System;
				DWORD dwErrorCode = 0;
				errType = m_MasterPort.GetErrorCode(&dwErrorCode);

				if (dwErrorCode == 0)
				{
					// ����ȡһ������
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
				}

				if (errType == ErrorType_System)
				{
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}

			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	case WorkMode_Full_RW_Test:
		{
			BOOL bReadOnlyTest = m_Config.GetBool(_T("FullRWTest"),_T("En_ReadOnlyTest"),FALSE);
			BOOL bRetainOriginData = m_Config.GetBool(_T("FullRWTest"),_T("En_RetainOriginData"),FALSE);
			BOOL bFormatFinish = m_Config.GetBool(_T("FullRWTest"),_T("En_FormatFinished"),FALSE);
			BOOL bStopBad = m_Config.GetBool(_T("FullRWTest"),_T("En_StopBadBlock"),FALSE);

			// ���ö˿�״̬
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			if (bFormatFinish)
			{
				CString strVolumeLable = m_Config.GetString(_T("DiskFormat"),_T("VolumeLabel"));
				FileSystem fileSystem = (FileSystem)m_Config.GetUInt(_T("DiskFormat"),_T("FileSystem"),FileSystem_FAT32);
				UINT nClusterSize = m_Config.GetUInt(_T("DiskFormat"),_T("ClusterSize"),0);
				BOOL bQuickFormat = m_Config.GetBool(_T("DiskFormat"),_T("QuickFormat"),TRUE);

				disk.SetFormatParm(strVolumeLable,fileSystem,nClusterSize,TRUE);
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetFullRWTestParm(bReadOnlyTest,bRetainOriginData,bFormatFinish,bStopBad);

			bResult = disk.Start();

			if (bResult)
			{
				strMsg.Format(_T("%s Completed !!!"),strWorkMode);
			}
			else
			{
				// ����ȡһ������
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
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	case WorkMode_Fade_Picker:
		{
			BOOL bRetainOriginData = m_Config.GetBool(_T("FadePicker"),_T("En_RetainOriginData"),FALSE);
			BOOL bFormatFinish = m_Config.GetBool(_T("FadePicker"),_T("En_FormatFinished"),FALSE);

			// ���ö˿�״̬
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			if (bFormatFinish)
			{
				CString strVolumeLable = m_Config.GetString(_T("DiskFormat"),_T("VolumeLabel"));
				FileSystem fileSystem = (FileSystem)m_Config.GetUInt(_T("DiskFormat"),_T("FileSystem"),FileSystem_FAT32);
				UINT nClusterSize = m_Config.GetUInt(_T("DiskFormat"),_T("ClusterSize"),0);
				BOOL bQuickFormat = m_Config.GetBool(_T("DiskFormat"),_T("QuickFormat"),TRUE);

				disk.SetFormatParm(strVolumeLable,fileSystem,nClusterSize,TRUE);
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetFadePickerParm(bRetainOriginData,bFormatFinish);

			bResult = disk.Start();

			if (bResult)
			{
				strMsg.Format(_T("%s Completed !!!"),strWorkMode);
			}
			else
			{
				// ����ȡһ������
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
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	case WorkMode_Speed_Check:
		{
			BOOL bSpeedUpStandard = m_Config.GetBool(_T("SpeedCheck"),_T("En_SpeedUpStandard"),FALSE);
			double dbReadSpeed = m_Config.GetDouble(_T("SpeedCheck"),_T("ReadSpeed"),0.0);
			double dbWriteSpeed = m_Config.GetDouble(_T("SpeedCheck"),_T("WriteSpeed"),0.0);

			// ���ö˿�״̬
			m_MasterPort.Initial();

			POSITION pos = m_TargetPorts.GetHeadPosition();
			while (pos)
			{
				CPort *port = m_TargetPorts.GetNext(pos);
				if (port->IsConnected())
				{
					port->SetHashMethod(hashMethod);
					port->SetWorkMode(m_WorkMode);
					port->SetStartTime(m_StartTime);
				}
			}

			disk.SetTargetPorts(&m_TargetPorts);
			disk.SetSpeedCheckParm(bSpeedUpStandard,dbReadSpeed,dbWriteSpeed);

			bResult = disk.Start();

			if (bResult)
			{
				strMsg.Format(_T("%s Completed !!!"),strWorkMode);
			}
			else
			{
				// ����ȡһ������
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
					strMsg.Format(_T("%s Failed !\r\nSystem errorCode=%d,%s")
						,strWorkMode,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));
				}
				else
				{
					strMsg.Format(_T("%s Failed !\r\nCustom errorCode=0x%X,%s")
						,strWorkMode,dwErrorCode,GetCustomErrorMsg((CustomError)dwErrorCode));
				}
			}

			CUtils::WriteLogFile(m_hLogFile,TRUE,strMsg);
		}
		break;

	}

	m_strMsg = strMsg;
	m_bResult = bResult;

	KillTimer(TIMER_UPDATE_STATISTIC);

	m_bRunning = FALSE;

	PostMessage(WM_UPDATE_STATISTIC);

	if (!m_bCancel)
	{
		PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_START, BN_CLICKED), (LPARAM)m_hWnd); 
	}
	
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

		case CustomError_Speed_Too_Slow:
			strError = _T("Speed too slow.");
			break;

		case CustomError_Unrecognized_Partition:
			strError = _T("Unrecognized partition.");
			break;

		case CustomError_No_File_Select:
			strError = _T("No file has been selected.");
			break;

		case CustomError_Target_Small:
			strError = _T("Target is small.");
			break;

		case CustomError_Format_Error:
			strError = _T("Disk format error.");
			break;

		case CustomError_Get_Data_From_Server_Error:
			strError = _T("Get data from server error.");
			break;

		case CustomError_Image_Hash_Value_Changed:
			strError = _T("Image hash value was changed.");
			break;

		case CustomError_Enum_File_Failed:
			strError = _T("Enum file failed.");
			break;

		case CustomError_MTP_OpenDevice_Failed:
			strError = _T("MTP open device failed.");
			break;

		case CustomError_MTP_EnumStorageID_Failed:
			strError = _T("MTP enum storage id failed.");
			break;

		case CustomError_MTP_CreateFolder_Failed:
			strError = _T("MTP create folder failed.");
			break;

		case CustomError_MTP_NO_ObjectID:
			strError = _T("MTP no object id.");
			break;

		case CustomError_MTP_CreateDataStream_Failed:
			strError = _T("MTP create data stream failed.");
			break;

		case CustomError_MTP_WriteFile_Failed:
			strError = _T("MTP write file failed.");
			break;

		case CustomError_MTP_ReadFile_Failed:
			strError = _T("MTP read file failed.");
			break;

		case CustomError_ReadSpeed_Slow:
			strError = _T("Read speed slow.");
			break;

		case CustomError_WriteSpeed_Slow:
			strError = _T("Write speed slow.");
			break;

		case CustomError_Master_Target_Size_Not_Same:
			strError = _T("Not same size.");
			break;
	}

	return strError;
}


HBRUSH CUSBCopyDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  �ڴ˸��� DC ���κ�����
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

	case IDC_TEXT_LOGO:
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(58,95,205 ));
		break;
		
	}

	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	return hbr;
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
	// TODO: �ڴ����ר�ô����/����û���

	m_bRunning = TRUE;
	m_bCancel = TRUE;

	UpdatePortFrame(FALSE);

	if (m_ThreadListen && m_ThreadListen->m_hThread)
	{
		WaitForSingleObject(m_ThreadListen->m_hThread,INFINITE);
		delete m_ThreadListen;
		m_ThreadListen = NULL;
	}

	POSITION pos = m_TargetPorts.GetHeadPosition();

	while(pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);
		delete port;
	}

	m_TargetPorts.RemoveAll();

	if (m_bUpdate)
	{
		for (UINT i = 1;i < m_nPortNum;i++)
		{
			m_Command.Power(i,FALSE);
		}
	}
	else
	{
		m_Command.ResetPower();
	}

	if (m_hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hLogFile);
		m_hLogFile = INVALID_HANDLE_VALUE;
	}

	if (m_hBurninEvent != NULL)
	{
		CloseHandle(m_hBurninEvent);
		m_hBurninEvent = NULL;
	}

	for (UINT i = 0; i < m_nPortNum;i++)
	{
		m_PortFrames[i].DestroyWindow();
	}

	delete []m_PortFrames;

	if (m_bSockeConnected)
	{
		closesocket(m_ClientSocket);
	}

	return CDialogEx::DestroyWindow();
}

void CUSBCopyDlg::InitialMachine()
{
	// ����
	m_Command.SwitchScreen();

	//��ʼ���ϵ�
	m_Command.ResetPower();

	// �����NGFF�����ϵ磬ֻ���ڵ�START���ϵ�
	if (m_nMachineType != MT_NGFF)
	{
		// ȫ���ϵ�
		for (UINT i = 0; i < m_nPortNum;i++)
		{
			m_Command.Power(i,TRUE);
			Sleep(100);
		}
	}

// 	// ʹ����˸����
// 	m_Command.EnableFlashLight();

	CleanRecord();
}

DWORD CUSBCopyDlg::InitialMachineThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;
	pDlg->InitialMachine();

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
			if (!m_bRunning)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Start event click."),(BYTE)pBuffer[i]);
				keybd_event(VK_RETURN,0,0,0);
				keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
			}
			
		}
		else if (pBuffer[i] == 0x76)
		{
			if (GetDlgItem(IDC_BTN_START)->IsWindowEnabled() && ::GetActiveWindow() == m_hWnd)
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Stop event click."),(BYTE)pBuffer[i]);

				Sleep(2000);

				MSG msg;
				while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
				{
					DispatchMessage(&msg);
					TranslateMessage(&msg); 
				}

				PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_START, BN_CLICKED), (LPARAM)m_hWnd); 
			}		
		}
	}
	delete []pBuffer;

	return 0;
}

afx_msg LRESULT CUSBCopyDlg::OnResetMachienPort(WPARAM wParam, LPARAM lParam)
{
	InitialStatisticInfo();
	ResetPortFrame();
	ResetPortInfo();

	UpdatePortFrame(TRUE);

	// ��λ
	m_Command.ResetLight();

	m_ThreadListen = AfxBeginThread((AFX_THREADPROC)EnumDeviceThreadProc,this,0,CREATE_SUSPENDED);

	int nCount = 3;
	while (m_ThreadListen == NULL && nCount > 0)
	{
		DWORD dwLastError = GetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("CreateEnumDeviceThread failed(%d),%s"),nCount,CUtils::GetErrorMsg(dwLastError));

		Sleep(500);
		m_ThreadListen = AfxBeginThread((AFX_THREADPROC)EnumDeviceThreadProc,this,0,CREATE_SUSPENDED);

		nCount--;
	}

	if (m_ThreadListen)
	{
		m_ThreadListen->m_bAutoDelete = FALSE;
		m_ThreadListen->ResumeThread();
	}

	return 0;
}

BOOL CUSBCopyDlg::OnDeviceChange( UINT nEventType, DWORD_PTR dwData )
{
	PDEV_BROADCAST_HDR   pDevBroadcastHdr;   
	//���������Ϣƥ��,����guid��

	switch (nEventType)
	{		
	case DBT_DEVICEREMOVECOMPLETE:
	case DBT_DEVICEARRIVAL:
		pDevBroadcastHdr = (PDEV_BROADCAST_HDR)dwData;
		if (/*pDevBroadcastHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE || */pDevBroadcastHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
		{

		}
		break;
	default:
		break;
	}
	return TRUE;
}

void CUSBCopyDlg::EnumDevice()
{
	while (!m_bRunning)
	{
		if (m_bIsMTP)
		{
			if (!m_bRunning)
			{
				EnumWPD(&m_bRunning);
			}

			if (!m_bRunning)
			{
				MatchMTPDevice();
			}

			CleanupWPD();
		}
		else
		{

			if (!m_bRunning && m_nMachineType == MT_USB)
			{
				EnumUSB(&m_bRunning);
			}

			if (!m_bRunning)
			{
				EnumStorage(&m_bRunning);
			}

			if (!m_bRunning)
			{
				EnumVolume(&m_bRunning);
			}

			if (!m_bRunning)
			{
				MatchDevice();
			}

			CleanupStorage();
			CleanupVolume();
			CleanupUSB();
		}
		
	}
	
}

void CUSBCopyDlg::EnumMTPDevice()
{
	while (!m_bRunning)
	{
		if (!m_bRunning)
		{
			EnumWPD(&m_bRunning);
		}

		if (!m_bRunning)
		{
			MatchMTPDevice();
		}

		CleanupWPD();
	}
}

DWORD CUSBCopyDlg::EnumDeviceThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;

	pDlg->EnumDevice();	

	return 1;
}

void CUSBCopyDlg::MatchDevice()
{
	POSITION pos = m_TargetPorts.GetHeadPosition();
	CString strPath;
	int nConnectIndex = -1;
	int nCount = 0;
	CString strModel,strSN;
	while (pos && !m_bRunning)
	{
		PUSBDEVICEINFO pUsbDeviceInfo = NULL;
		PDEVICELIST pStorageList = NULL;
		PDEVICELIST pVolumeList = NULL;
		CPort *port;

		if (nCount == 0)
		{
			port = &m_MasterPort;
		}
		else
		{
			port = m_TargetPorts.GetNext(pos);
		}

		nCount++;

		strPath = port->GetPath1();
		nConnectIndex = port->GetConnectIndex1();
		pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);

		if (pUsbDeviceInfo == NULL)
		{
			strPath = port->GetPath2();
			nConnectIndex = port->GetConnectIndex2();
			pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);
		}

		if (pUsbDeviceInfo)
		{
			pStorageList = MatchStorageDeivceIDs(pUsbDeviceInfo->DeviceID);

			if (pStorageList != NULL)
			{
				if (pStorageList->pNext == NULL && m_nMachineType == MT_USB)
				{
					PUSBDEVIEINFO pUSB = MatchUSBDeviceID(pUsbDeviceInfo->DeviceID);

					if (pUSB)
					{
						CleanupStorageDeviceList(pStorageList);

						pStorageList = MatchStorageDeivceIDs(pUSB->pszDeviceID);

						CleanUSBDeviceInfo(pUSB);
					}
				}
				
			}

			if (pStorageList)
			{
				PDEVICELIST pListNode = pStorageList->pNext;

				if (pListNode == NULL && m_nMachineType != MT_TS)
				{
					port->Initial();
				}

				while (pListNode && !m_bRunning)
				{
					PSTORAGEDEVIEINFO pStorageDevInfo = (PSTORAGEDEVIEINFO)pListNode->pDevInfo;
					if (pStorageDevInfo)
					{
						DWORD dwErrorCode = 0;
						HANDLE hDevice = CDisk::GetHandleOnPhysicalDrive(pStorageDevInfo->nDiskNum,FILE_FLAG_OVERLAPPED,&dwErrorCode);

						if (hDevice != INVALID_HANDLE_VALUE)
						{
							ULONGLONG ullSectorNums = 0;
							DWORD dwBytesPerSector = 0;
							MEDIA_TYPE type;
							ullSectorNums = CDisk::GetNumberOfSectors(hDevice,&dwBytesPerSector,&type);

							// �����ĸ������ReadOnly
							if (port->GetPortNum() == 0)
							{
								if (m_WorkMode == WorkMode_FileCopy)
								{
									CDisk::SetDiskAtrribute(hDevice,FALSE,FALSE,&dwErrorCode);
								}
								else
								{
									CDisk::SetDiskAtrribute(hDevice,TRUE,FALSE,&dwErrorCode);
								}

							}
// 							else
// 							{
// 								CDisk::SetDiskAtrribute(hDevice,FALSE,FALSE,&dwErrorCode);
// 							}


							if (ullSectorNums > 0)
							{

								if (port->IsConnected())
								{									
									CloseHandle(hDevice);									
									break;
								}

								// һ���������м���volume
								CStringArray strVolumeArray;
								pVolumeList = MatchVolumeDeviceDiskNums(pStorageDevInfo->nDiskNum);

								if (pVolumeList)
								{
									PDEVICELIST pVolumeNode = pVolumeList->pNext;
									while (pVolumeNode)
									{
										PVOLUMEDEVICEINFO pVolumeInfo = (PVOLUMEDEVICEINFO)pVolumeNode->pDevInfo;
										if (pVolumeInfo)
										{
											CString strVolume(pVolumeInfo->pszVolumePath);
											strVolumeArray.Add(strVolume);

											if (port->GetPortNum() == 0 && m_WorkMode == WorkMode_FileCopy)
											{
												//��ĸ�̷����̷�
												strVolume += _T("\\");
												CDisk::ChangeLetter(strVolume,MASTER_PATH);
											}
										}

										pVolumeNode = pVolumeNode->pNext;
									}

									CleanupVolumeDeviceList(pVolumeList);
								}

								if (m_nMachineType == MT_TS)
								{
									TCHAR szModelName[256] = {NULL};
									TCHAR szSerialNumber[256] = {NULL};

									CDisk::GetTSModelNameAndSerialNumber(hDevice,szModelName,szSerialNumber,&dwErrorCode);

									strModel = CString(szModelName);
									strSN = CString(szSerialNumber);

									strModel.Trim();
									strSN.Trim();
								}
								else
								{

									PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
									DeviceDescriptor=(PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1];
									DeviceDescriptor->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;

									BOOL bOK = GetDriveProperty(hDevice,DeviceDescriptor);

									dwErrorCode = GetLastError();

									if (bOK)
									{
										LPCSTR vendorId = "";
										LPCSTR productId = "";

										//										LPCSTR serialNumber = "";

										if ((DeviceDescriptor->ProductIdOffset != 0) &&
											(DeviceDescriptor->ProductIdOffset != -1)) 
										{
											productId        = (LPCSTR)(DeviceDescriptor);
											productId       += (ULONG_PTR)DeviceDescriptor->ProductIdOffset;
										}

										if ((DeviceDescriptor->VendorIdOffset != 0) &&
											(DeviceDescriptor->VendorIdOffset != -1)) 
										{
											vendorId         = (LPCSTR)(DeviceDescriptor);
											vendorId        += (ULONG_PTR)DeviceDescriptor->VendorIdOffset;
										}

										strModel = CString(vendorId) + CString(productId);

										if (pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.iSerialNumber)
										{
											PSTRING_DESCRIPTOR_NODE StringDescs = pUsbDeviceInfo->StringDescs;
											UCHAR Index = pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.iSerialNumber;
											// Use an actual "int" here because it's passed as a printf * precision
											int descChars;  

											while (StringDescs)
											{
												if (StringDescs->DescriptorIndex == Index)
												{

													//
													// bString from USB_STRING_DESCRIPTOR isn't NULL-terminated, so 
													// calculate the number of characters.  
													// 
													// bLength is the length of the whole structure, not just the string.  
													// 
													// bLength is bytes, bString is WCHARs
													// 
													descChars = 
														( (int) StringDescs->StringDescriptor->bLength - 
														offsetof(USB_STRING_DESCRIPTOR, bString) ) /
														sizeof(WCHAR);
													//
													// Use the * precision and pass the number of characters just caculated.
													// bString is always WCHAR so specify widestring regardless of what TCHAR resolves to
													// 
													strSN.Format(_T("%.*ws"),
														descChars,
														StringDescs->StringDescriptor->bString);
												}

												StringDescs = StringDescs->Next;
											}
										}

										strModel.Trim();
										strSN.Trim();

										// �ж����ƶ�Ӳ�̻���U�̣�������ƶ�Ӳ�̿��Ի�ȡSN��Model
										if (type == FixedMedia && DeviceDescriptor->BusType == BusTypeUsb)
										{
											TCHAR szModelName[256] = {NULL};
											TCHAR szSerialNumber[256] = {NULL};

											if (CDisk::GetUsbHDDModelNameAndSerialNumber(hDevice,szModelName,szSerialNumber,&dwErrorCode))
											{
												CString strModelTemp = CString(szModelName);
												CString strSNTemp = CString(szSerialNumber);

												strModelTemp.Trim();
												strSNTemp.Trim();

												if (!strModelTemp.IsEmpty() && !strSNTemp.IsEmpty())
												{
													strModel = strModelTemp;
													strSN = strSNTemp;
												}
											}

										}

									}

									delete []DeviceDescriptor;

								}

								port->SetConnected(TRUE);
								port->SetDiskNum(pStorageDevInfo->nDiskNum);
								port->SetPortState(PortState_Online);
								port->SetBytesPerSector(dwBytesPerSector);
								port->SetTotalSize(ullSectorNums * dwBytesPerSector);
								port->SetVolumeArray(strVolumeArray);
								port->SetSN(strSN);
								port->SetModuleName(strModel);

								if ((pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB & 0x0300) == 0x0300)
								{
									port->SetUsbType(_T("3.0"));
								}
								else
								{
									port->SetUsbType(_T("2.0"));
								}

								CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Path=%s:%d,bcdUSB=%04X,Capacity=%I64d,ModelName=%s,SN=%s")
									,port->GetPortName(),pStorageDevInfo->nDiskNum,strPath,nConnectIndex,pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB
									,ullSectorNums * dwBytesPerSector,strModel,strSN);

								CloseHandle(hDevice);
								
								break;
							}
							else
							{
								CloseHandle(hDevice);
								port->Initial();
							}

						}
						else
						{
							port->Initial();
						}
					}
					else
					{
						port->Initial();
					}

					pListNode = pListNode->pNext;
				}				
				CleanupStorageDeviceList(pStorageList);

			}
			else
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,No disk"),port->GetPortName());
				port->Initial();

				// �����TF������������������ϵ磬�����USB����������Ҫ

				if (m_nMachineType == MT_TS && !m_bExportLog)
				{
					// �����ϵ�
					m_Command.Power(port->GetPortNum(),FALSE);
					Sleep(500);
					m_Command.Power(port->GetPortNum(),TRUE);
				}

			}

			CleanupInfo(pUsbDeviceInfo);
			pUsbDeviceInfo = NULL;
		}
		else
		{
			port->Initial();

			// �����TF������������������ϵ磬�����USB����������Ҫ

			if (m_nMachineType == MT_TS && !m_bExportLog)
			{
				// �����ϵ�
				m_Command.Power(port->GetPortNum(),FALSE);
				Sleep(500);
				m_Command.Power(port->GetPortNum(),TRUE);
			}
		}
	}
}

void CUSBCopyDlg::MatchMTPDevice()
{
	POSITION pos = m_TargetPorts.GetHeadPosition();
	CString strPath;
	int nConnectIndex = -1;
	int nCount = 0;
	CString strModel,strSN;
	while (pos && !m_bRunning)
	{
		PUSBDEVICEINFO pUsbDeviceInfo = NULL;
		PWPDDEVIEINFO pWPDDevInfo = NULL;
		CPort *port;

		if (nCount == 0)
		{
			port = &m_MasterPort;
		}
		else
		{
			port = m_TargetPorts.GetNext(pos);
		}

		nCount++;

		strPath = port->GetPath1();
		nConnectIndex = port->GetConnectIndex1();
		pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);

		if (pUsbDeviceInfo == NULL)
		{
			strPath = port->GetPath2();
			nConnectIndex = port->GetConnectIndex2();
			pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);
		}

		if (pUsbDeviceInfo)
		{
			pWPDDevInfo = MatchWPDDeviceID(pUsbDeviceInfo->DeviceID);

			if (pWPDDevInfo)
			{
				if (!port->IsConnected())
				{
					DWORD dwErrorCode = 0;
					CComPtr<IPortableDevice> pIPortableDevice;
					HRESULT hr = OpenDevice(&pIPortableDevice,pWPDDevInfo->pszDevicePath);

					if (SUCCEEDED(hr) && pIPortableDevice != NULL)
					{
						GetDeviceModelAndSerialNumber(pIPortableDevice,strModel.GetBuffer(MAX_PATH),strSN.GetBuffer(MAX_PATH));
						strModel.ReleaseBuffer();
						strSN.ReleaseBuffer();

						LPWSTR ppwszObjectIDs[NUM_OBJECTS_TO_REQUEST] = {NULL};
						DWORD dwObjects = EnumStorageIDs(pIPortableDevice,ppwszObjectIDs);

						ULONGLONG ullTotalSize = 0,ullFreeSize = 0;
						if (dwObjects > 0)
						{
							GetStorageFreeAndTotalCapacity(pIPortableDevice,ppwszObjectIDs[0],&ullFreeSize,&ullTotalSize);

							for (DWORD index = 0; index < dwObjects;index++)
							{
								//delete []ppwszObjectIDs[index];
								CoTaskMemFree(ppwszObjectIDs[index]);
								ppwszObjectIDs[index] = NULL;
							}
						}
						

						port->SetConnected(TRUE);
						port->SetDevicePath(CString(pWPDDevInfo->pszDevicePath));
						port->SetPortState(PortState_Online);
						port->SetTotalSize(ullTotalSize);
						port->SetCompleteSize(ullTotalSize - ullFreeSize);
						port->SetSN(strSN);
						port->SetModuleName(strModel);

						if ((pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB & 0x0300) == 0x0300)
						{
							port->SetUsbType(_T("3.0"));
						}
						else
						{
							port->SetUsbType(_T("2.0"));
						}

						CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Path=%s:%d,bcdUSB=%04X,Capacity=%I64d,FreeSize=%I64d,ModelName=%s,SN=%s")
							,port->GetPortName(),strPath,nConnectIndex,pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB
							,ullTotalSize,ullFreeSize,strModel,strSN);
					}
					else
					{
						port->Initial();
					}
				}

				CleanWPDDeviceInfo(pWPDDevInfo);
			}
			else
			{
				port->Initial();
			}

			CleanupInfo(pUsbDeviceInfo);
			pUsbDeviceInfo = NULL;
		}
		else
		{
			port->Initial();
		}
	}
}


afx_msg LRESULT CUSBCopyDlg::OnUpdateSoftware(WPARAM wParam, LPARAM lParam)
{
	m_bUpdate = TRUE;
	return 0;
}

void CUSBCopyDlg::BackupLogfile( HANDLE hFile ,DWORD dwFileSize)
{
	CString strBackipFile = m_strAppPath + LOG_FILE_BAK;

	if (PathFileExists(strBackipFile))
	{
		CFileStatus status;

		CFile::GetStatus(strBackipFile,status);

		if (status.m_size > 1 * 1024 * 1024 * 1024)
		{
			DeleteFile(strBackipFile);
		}
	}

	HANDLE hBackup = CreateFile(strBackipFile,
								GENERIC_WRITE | GENERIC_READ,
								FILE_SHARE_WRITE | FILE_SHARE_READ,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL);

	if (hBackup == INVALID_HANDLE_VALUE)
	{
		CString strMsg;
		strMsg.LoadString(IDS_MSG_CREATE_BACKUP_FILE_FAILED);
		MessageBox(strMsg);
		return;
	}

	LARGE_INTEGER liFile = {0};
	if (SetFilePointerEx(hBackup,liFile,0,FILE_END))
	{
		BYTE *pByte = new BYTE[dwFileSize];
		ZeroMemory(pByte,dwFileSize);

		DWORD dwRead = 0,dwWrite = 0;
		if (ReadFile(hFile,pByte,dwFileSize,&dwRead,NULL))
		{
			WriteFile(hBackup,pByte,dwRead,&dwWrite,NULL);
		}

		delete []pByte;
	}
	else
	{
		CString strMsg;
		strMsg.LoadString(IDS_MSG_SETFILEPOINT_FAILED);
		MessageBox(strMsg);
	}
	
	CloseHandle(hBackup);
	
}


afx_msg LRESULT CUSBCopyDlg::OnResetPower(WPARAM wParam, LPARAM lParam)
{
	//��ʼ���ϵ�
	m_Command.ResetPower();

	// ȫ���ϵ�
	for (UINT i = 0; i < m_nPortNum;i++)
	{
		m_Command.Power(i,TRUE);
		Sleep(100);
	}

// 	// ʹ����˸����
// 	m_Command.EnableFlashLight();
	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnBurnInTest(WPARAM wParam, LPARAM lParam)
{
	if (m_bLisence && GetDlgItem(IDC_BTN_START)->IsWindowEnabled())
	{
		AfxBeginThread((AFX_THREADPROC)BurnInTestThreadProc,this);
	}
	
	return 0;
}

CString CUSBCopyDlg::GetWorkModeString( WorkMode mode )
{
	CString strWorkMode;
	switch (mode)
	{
	case WorkMode_FullCopy:
		strWorkMode = _T("FULL COPY");
		break;
	case WorkMode_QuickCopy:
		strWorkMode = _T("QUICK COPY");
		break;
	case WorkMode_FileCopy:
		strWorkMode = _T("FILE COPY");
		break;
	case WorkMode_ImageCopy:
		strWorkMode = _T("IMAGE COPY");
		break;
	case WorkMode_ImageMake:
		strWorkMode = _T("IMAGE MAKE");
		break;
	case WorkMode_DiskClean:
		strWorkMode = _T("DISK CLEAN");
		break;

	case WorkMode_DiskCompare:
		strWorkMode = _T("DISK COMPARE");
		break;

	case WorkMode_DiskFormat:
		strWorkMode = _T("DISK FORMAT");
		break;

	case WorkMode_DifferenceCopy:
		strWorkMode = _T("DIFF. COPY");
		break;

	case WorkMode_MTPCopy:
		strWorkMode = _T("MTP COPY");
		break;

	case WorkMode_Full_RW_Test:
		strWorkMode = _T("FULL RW TEST");
		break;

	case WorkMode_Fade_Picker:
		strWorkMode = _T("FADE PICKER");
		break;

	case WorkMode_Speed_Check:
		strWorkMode = _T("SPEED CHECK");
		break;
	}

	return strWorkMode;
}

void CUSBCopyDlg::InitialBurnInTest(UINT cycle,UINT nCycleCount)
{
	CString strWorkMode,strWorkModeParm;
	CString strResText1,strResText2,strResText3;
	m_bIsMTP = FALSE;
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		{
			strResText1.LoadString(IDS_WORK_MODE_FULL_COPY);

			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("FullCopy"),_T("En_CleanDiskFirst"),FALSE))
			{
				strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

				strWorkModeParm = strResText3;
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText1;
			}

			if (m_Config.GetBool(_T("FullCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText2;
			}
		}
		break;

	case WorkMode_QuickCopy:
		{
			strResText1.LoadString(IDS_WORK_MODE_QUICK_COPY);
			strWorkMode = strWorkModeParm = strResText1;


			if (m_Config.GetBool(_T("QuickCopy"),_T("En_CleanDiskFirst"),FALSE))
			{
				strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

				strWorkModeParm = strResText3;
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText1;
			}

			if (m_Config.GetBool(_T("QuickCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm += _T(" + ");
				strWorkModeParm += strResText2;
			}
		}
		break;

	case WorkMode_ImageCopy:
		{

			strResText1.LoadString(IDS_WORK_MODE_IMAGE_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("ImageCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}

			// MTP Copy
			if (m_Config.GetUInt(_T("ImageCopy"),_T("ImageType"),0) == 1)
			{
				m_bIsMTP = TRUE;
				strResText3.LoadString(IDS_MTP_IMAGE);
				strWorkModeParm += _T(" - ");
				strWorkModeParm += strResText3;
			}
			else
			{
				if (m_Config.GetBool(_T("ImageCopy"),_T("En_CleanDiskFirst"),FALSE))
				{
					strResText3.LoadString(IDS_WORK_MODE_DISK_CLEAN);

					strWorkModeParm = strResText3 + _T(" + ") + strWorkModeParm;
				}
			}
		}
		break;

	case WorkMode_ImageMake:
		{
			strResText1.LoadString(IDS_WORK_MODE_IMAGE_MAKE);
			strWorkMode = strWorkModeParm = strResText1;

			UINT nSaveMode = m_Config.GetUInt(_T("ImageMake"),_T("SaveMode"),0);

			switch (nSaveMode)
			{
			case 0: //Full image
				strResText3.LoadString(IDS_FULL_IMAGE);

				break;

			case 1: //Quick image
				strResText3.LoadString(IDS_QUICK_IMAGE);
				break;

			case 2: //MTP image
				m_bIsMTP = TRUE;
				strResText3.LoadString(IDS_MTP_IMAGE);
				break;

			}

			strWorkModeParm += _T(" - ");
			strWorkModeParm += strResText3;
		}

		break;

	case WorkMode_DiskClean:
		{
			strResText1.LoadString(IDS_WORK_MODE_DISK_CLEAN);
			strWorkMode = strWorkModeParm = strResText1;

			CleanMode cleanMode = (CleanMode)m_Config.GetInt(_T("DiskClean"),_T("CleanMode"),0);
			switch (cleanMode)
			{
			case CleanMode_Full:
				strResText2.LoadString(IDS_FULL_CLEAN);
				break;

			case CleanMode_Quick:
				strResText2.LoadString(IDS_QUICK_CLEAN);
				break;

			case CleanMode_Safe:
				strResText2.LoadString(IDS_SAFE_CLEAN);
				break;
			}

			strWorkModeParm.Format(_T("%s - %s"),strResText1,strResText2);
		}

		break;

	case WorkMode_DiskCompare:
		{
			strResText1.LoadString(IDS_WORK_MODE_DISK_COMPARE);
			strWorkMode = strWorkModeParm = strResText1;

			CompareMode compareMode = (CompareMode)m_Config.GetInt(_T("DiskCompare"),_T("CompareMode"),0);

			switch (compareMode)
			{
			case CompareMode_Full:
				strResText2.LoadString(IDS_FULL_COMPARE);
				break;

			case CompareMode_Quick:
				strResText2.LoadString(IDS_QUICK_COMPARE);
				break;
			}

			strWorkModeParm.Format(_T("%s - %s"),strResText1,strResText2);
		}
		break;

	case WorkMode_FileCopy:
		{
			strResText1.LoadString(IDS_WORK_MODE_FILE_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("FileCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}
		}
		break;

	case WorkMode_DiskFormat:
		{
			strResText1.LoadString(IDS_WORK_MODE_DISK_FORMAT);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_DifferenceCopy:
		{
			strResText1.LoadString(IDS_WORK_MODE_DIFF_COPY);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_MTPCopy:
		{
			strResText1.LoadString(IDS_WORK_MODE_MTP_COPY);
			strWorkMode = strWorkModeParm = strResText1;

			if (m_Config.GetBool(_T("MTPCopy"),_T("En_Compare"),FALSE))
			{
				strResText2.LoadString(IDS_COMPARE);
				strWorkModeParm.Format(_T("%s + %s"),strResText1,strResText2);
			}

			m_bIsMTP = TRUE;
		}
		break;

	case WorkMode_Full_RW_Test:
		{
			strResText1.LoadString(IDS_WORK_MODE_FULL_RW_TEST);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_Fade_Picker:
		{
			strResText1.LoadString(IDS_WORK_MODE_FADE_PICKER);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;

	case WorkMode_Speed_Check:
		{
			strResText1.LoadString(IDS_WORK_MODE_SPEED_CHECK);
			strWorkMode = strWorkModeParm = strResText1;
		}
		break;
	}

	CString strBurnIn,strText;
	strBurnIn.LoadString(IDS_WORK_MODE_BURN_IN);

	strText.Format(_T("%s(%d/%d)  ---  %s"),strBurnIn,cycle + 1, nCycleCount,strWorkModeParm);

	SetDlgItemText(IDC_GROUP_WORK_MODE,strText);
}

void CUSBCopyDlg::BurnInTest()
{
	m_bBurnInTest = TRUE;
	m_bResult = TRUE;
	UINT nCycleCount = m_Config.GetUInt(_T("BurnIn"),_T("CycleCount"),1);
	UINT nFunctionNum = m_Config.GetUInt(_T("BurnIn"),_T("FunctionNum"),0);

	m_StartTime = CTime::GetCurrentTime();

	for (UINT cycle = 0;cycle < nCycleCount && m_bResult;cycle++)
	{

		CString strKey;
		for (UINT i = 0;i < nFunctionNum;i++)
		{
			WaitForSingleObject(m_hBurninEvent,INFINITE);

			if (!m_bResult)
			{
				SetEvent(m_hBurninEvent);
				break;
			}

			ResetPortFrame();
			ResetPortInfo();

			UpdatePortFrame(TRUE);

			// ��λ
			m_Command.ResetLight();

			strKey.Format(_T("Function_%d"),i);
			m_WorkMode = (WorkMode)m_Config.GetUInt(_T("BurnIn"),strKey,0);

			CUtils::WriteLogFile(m_hLogFile,FALSE,_T(""));
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Burn In Test - cycle %d - %s"),cycle+1,GetWorkModeString(m_WorkMode));

			SendMessage(WM_SET_BURN_IN_TEXT,cycle,nCycleCount);

			SendMessage(WM_COMMAND, MAKEWPARAM(IDC_BTN_START, BN_CLICKED), (LPARAM)m_hWnd);

		}
	}

	WaitForSingleObject(m_hBurninEvent,INFINITE);
	CString strResText;
	strResText.LoadString(IDS_MSG_BURNIN_TEST_COMPLETED);
	MessageBox(strResText);

	SetEvent(m_hBurninEvent);

	//��λ
	SendMessage(WM_RESET_MACHIEN_PORT);

	EnableControls(TRUE);
	GetDlgItem(IDC_BTN_START)->SetFocus();

	m_bRunning = FALSE;

	m_bBurnInTest = FALSE;

	SendMessage(WM_INIT_CURRENT_WORKMODE);
}

DWORD WINAPI CUSBCopyDlg::BurnInTestThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;
	pDlg->BurnInTest();

	return 1;
}


BOOL CUSBCopyDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: �ڴ����ר�ô����/����û���
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE 
			|| (pMsg->wParam == VK_RETURN && m_bRunning)
			|| (pMsg->wParam == VK_SPACE && m_bRunning))
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

DWORD CUSBCopyDlg::CreateSocketConnect()
{

	if (m_bSockeConnected)
	{
		return 0;
	}

	m_CSConnectSever.Lock();

	CString strIpAddr = m_Config.GetString(_T("RemoteServer"),_T("ServerIP"));
	UINT nPort = m_Config.GetUInt(_T("RemoteServer"),_T("ListenPort"),7788);

	USES_CONVERSION;
	char *pBuf = W2A(strIpAddr);

	m_ClientSocket = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED);

	if (m_ClientSocket == INVALID_SOCKET)
	{
		DWORD dwErrorCode = WSAGetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSASocket failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		m_CSConnectSever.Unlock();
		return dwErrorCode;
	}

	BOOL bKeepAlive = TRUE;
	if (setsockopt(m_ClientSocket,SOL_SOCKET,SO_KEEPALIVE,(char *)&bKeepAlive,sizeof(bKeepAlive)) == SOCKET_ERROR)
	{
		DWORD dwErrorCode = WSAGetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("setsockopt failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		closesocket(m_ClientSocket);

		m_CSConnectSever.Unlock();
		return dwErrorCode;
	}

	// set KeepAlive parameter
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.keepalivetime    = 500;  // 0.5s
	alive_in.keepaliveinterval  = 1000; //1s
	alive_in.onoff                       = TRUE;
	unsigned long ulBytesReturn = 0;
	int nRet = WSAIoctl(m_ClientSocket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	if (nRet == SOCKET_ERROR)
	{
		DWORD dwErrorCode = WSAGetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSAIoctl failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		closesocket(m_ClientSocket);

		m_CSConnectSever.Unlock();
		return dwErrorCode;
	}

	WSAAsyncSelect(m_ClientSocket,this->m_hWnd,WM_SOCKET_MSG,FD_CLOSE | FD_CONNECT);

	SOCKADDR_IN ServerAddr = {0};
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(pBuf);
	ServerAddr.sin_port = htons(nPort);

	//���ж�host�Ƿ����
	HOSTENT *host = gethostbyaddr((char *)&ServerAddr.sin_addr.s_addr,4,PF_INET);

	if (host == NULL)
	{
		DWORD dwErrorCode = WSAGetLastError();
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("gethostbyaddr(%s:%d) failed with system error code:%d,%s")
			,strIpAddr,nPort,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		closesocket(m_ClientSocket);

		m_CSConnectSever.Unlock();
		return dwErrorCode;
	}

	if (connect(m_ClientSocket,(PSOCKADDR)&ServerAddr,sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		DWORD dwErrorCode = WSAGetLastError();

		if (dwErrorCode != WSAEWOULDBLOCK)
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("connect server(%s:%d) failed with system error code:%d,%s")
				,strIpAddr,nPort,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

			closesocket(m_ClientSocket);

			m_CSConnectSever.Unlock();
			return dwErrorCode;
		}
		
	}

	// CUtils::WriteLogFile(m_hLogFile,TRUE,_T("connect server(%s:%d) success."),strIpAddr,nPort);

	m_CSConnectSever.Unlock();
	return 0;
}

BOOL CUSBCopyDlg::SyncTime()
{
	SYNC_TIME_IN syncTimeIn = {0};
	syncTimeIn.dwCmdIn = CMD_SYNC_TIME_IN;
	syncTimeIn.dwSizeSend = sizeof(SYNC_TIME_IN);

	DWORD dwLen = sizeof(SYNC_TIME_IN);
	DWORD dwErrorCode = 0;

	if (!Send(m_ClientSocket,(char*)&syncTimeIn,dwLen,NULL,&dwErrorCode))
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSASend() failed with system error code:%d,%s"),dwErrorCode,
			CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}
	

	SYNC_TIME_OUT syncTimeOut = {0};
	dwLen = sizeof(SYNC_TIME_OUT);
	if (!Recv(m_ClientSocket,(char *)&syncTimeOut,dwLen,NULL,&dwErrorCode))
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSARecv() failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return FALSE;
	}

	if (syncTimeOut.dwCmdOut == CMD_SYNC_TIME_OUT && dwLen == sizeof(SYNC_TIME_OUT))
	{
		SYSTEMTIME SystemTime;
		SystemTime.wYear = syncTimeOut.wYmdHMS[0];
		SystemTime.wMonth = syncTimeOut.wYmdHMS[1];
		SystemTime.wDay = syncTimeOut.wYmdHMS[2];
		SystemTime.wHour = syncTimeOut.wYmdHMS[3];
		SystemTime.wMinute = syncTimeOut.wYmdHMS[4];
		SystemTime.wSecond = syncTimeOut.wYmdHMS[5];
		SystemTime.wMilliseconds = 0; //����ʱ��ʱ����Ҫ
		SystemTime.wDayOfWeek = -1;  //����ʱ��ʱ����Ҫ

		SetLocalTime(&SystemTime);
	}
	else
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSARecv() data error"));

		return FALSE;
	}

	return TRUE;
}


afx_msg LRESULT CUSBCopyDlg::OnConnectSocket(WPARAM wParam, LPARAM lParam)
{
	DWORD dwErrorCode = CreateSocketConnect();

	/*
	if (m_bSockeConnected)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time with server..."));

		if (SyncTime())
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time success."));
			SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: YES"));
		}
		else
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time failed."));
			SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: NO"));

			m_bSockeConnected = FALSE;
		}
	}
	else
	{
		SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: NO"));
	}
	*/

	return dwErrorCode;
}


afx_msg LRESULT CUSBCopyDlg::OnDisconnectSocket(WPARAM wParam, LPARAM lParam)
{
	if (m_bSockeConnected)
	{
		closesocket(m_ClientSocket);

		m_ClientSocket = INVALID_SOCKET;
	}

	m_bSockeConnected = FALSE;

	SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: NO"));

	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnSocketMsg(WPARAM wParam, LPARAM lParam)
{
	WORD event = WSAGETSELECTEVENT(lParam);
	WORD err = WSAGETSELECTERROR(lParam);
	CString strIpAddr = m_Config.GetString(_T("RemoteServer"),_T("ServerIP"));
	UINT nPort = m_Config.GetUInt(_T("RemoteServer"),_T("ListenPort"),7788);

	switch (event)
	{
	case FD_CLOSE:
		m_bSockeConnected = FALSE;
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("disconnect server(%s:%d) with system error code:%d,%s.")
			,strIpAddr,nPort,err,CUtils::GetErrorMsg(err));
		SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: NO"));

		SetTimer(TIMER_CONNECT_SERVER,5000,NULL);
		break;

	case FD_CONNECT:
		
		if (err == 0)
		{
			KillTimer(TIMER_CONNECT_SERVER);
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("connect server(%s:%d) success."),strIpAddr,nPort);
			SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: YES"));

			m_bSockeConnected = TRUE;
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time with server..."));

			if (SyncTime())
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time success."));
			}
			else
			{
				CUtils::WriteLogFile(m_hLogFile,TRUE,_T("synchronize time failed."));
			}
		}
		else
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("connect server(%s:%d) failed failed with system error code:%d,%s")
				,strIpAddr,nPort,err,CUtils::GetErrorMsg(err));
			SetDlgItemText(IDC_TEXT_CONNECT,_T("CONNECTED: NO"));

			m_bSockeConnected = FALSE;
		}
		
		break;
	}
	return 0;
}

DWORD CUSBCopyDlg::UploadLog(CString strLogName,CString strLog)
{
	USES_CONVERSION;
	char *filename = W2A(strLogName);
	char *log = W2A(strLog);

	CMD_IN uploadLogIn = {0};

	DWORD dwLen = sizeof(CMD_IN) + strlen(filename) + 1 + strlen(log) + 1;
	
	uploadLogIn.dwCmdIn = CMD_UPLOAD_LOG_IN;
	uploadLogIn.dwSizeSend = dwLen;
	
	BYTE *pByte = new BYTE[dwLen];
	ZeroMemory(pByte,dwLen);

	memcpy(pByte,&uploadLogIn,sizeof(CMD_IN));
	memcpy(pByte + sizeof(CMD_IN),filename,strlen(filename));
	memcpy(pByte + sizeof(CMD_IN) + strlen(filename) + 1,log,strlen(log));
	pByte[dwLen - 1] = END_FLAG;

	DWORD dwErrorCode = 0;
	if (!Send(m_ClientSocket,(char *)pByte,dwLen,NULL,&dwErrorCode))
	{
		delete []pByte;

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSASend() failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return dwErrorCode;
	}
	delete []pByte;

	UPLOAD_LOG_OUT uploadLogOut = {0};
	dwLen = sizeof(UPLOAD_LOG_OUT);
	if (!Recv(m_ClientSocket,(char *)&uploadLogOut,dwLen,NULL,&dwErrorCode))
	{

		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("WSARecv() failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return dwErrorCode;
	}

	if (uploadLogOut.dwErrorCode != 0)
	{
		dwErrorCode = uploadLogOut.dwErrorCode;
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Recv() failed with system error code:%d,%s")
			,dwErrorCode,CUtils::GetErrorMsg(dwErrorCode));

		return dwErrorCode;
	}
		
	return 0;
	
}

CString CUSBCopyDlg::GetUploadLogString()
{
	CString strLog;

	//PortNum;PortType;MachineSN;AliasName;SerialNumber;Model;DataSize;Capacity;Function;StartTime;EndTime;
	//1 - HashMethod;HashValue;Speed;Result;ErrorType;ErrorCode
	//2 - BadBlocks;ReadSpeed;WriteSpeed;Result;ErrorType;ErrorCode

	CString strItem;

	CString strAliasName,strMachineSN;
	GetDlgItemText(IDC_TEXT_ALIAS,strAliasName);
	GetDlgItemText(IDC_TEXT_SN,strMachineSN);

	if (m_MasterPort.IsConnected())
	{
		strItem.Format(_T("%s;"),m_MasterPort.GetPortName());
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetPortTypeName());
		strLog += strItem;

		strItem.Format(_T("%s;"),strMachineSN);
		strLog += strItem;

		strItem.Format(_T("%s;"),strAliasName);
		strLog += strItem;


		strItem.Format(_T("%s;"),m_MasterPort.GetSN());
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetModuleName());
		strLog += strItem;

		strItem.Format(_T("%I64d;"),m_MasterPort.GetValidSize());
		strLog += strItem;

		strItem.Format(_T("%I64d;"),m_MasterPort.GetTotalSize());
		strLog += strItem;

		strItem.Format(_T("%s;"),GetWorkModeString(m_WorkMode));
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetStartTime().Format(_T("%Y-%m-%d %H:%M:%S")));
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetEndTime().Format(_T("%Y-%m-%d %H:%M:%S")));
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetHashMethodString());
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetHashString());
		strLog += strItem;

		strItem.Format(_T("%d;"),(int)m_MasterPort.GetRealSpeed());
		strLog += strItem;

		strItem.Format(_T("%s;"),m_MasterPort.GetResultString());
		strLog += strItem;

		DWORD dwErrorCode = 0;
		ErrorType errType = m_MasterPort.GetErrorCode(&dwErrorCode);

		strItem.Format(_T("%d;"),errType);
		strLog += strItem;

		strItem.Format(_T("%d\r\n"),dwErrorCode);
		strLog += strItem;
	}

	POSITION pos = m_TargetPorts.GetHeadPosition();

	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);

		if (port->IsConnected())
		{
			strItem.Format(_T("%s;"),port->GetPortName());
			strLog += strItem;

			strItem.Format(_T("%s;"),port->GetPortTypeName());
			strLog += strItem;

			strItem.Format(_T("%s;"),strMachineSN);
			strLog += strItem;

			strItem.Format(_T("%s;"),strAliasName);
			strLog += strItem;

			strItem.Format(_T("%s;"),port->GetSN());
			strLog += strItem;

			strItem.Format(_T("%s;"),port->GetModuleName());
			strLog += strItem;

			// ����ǿ���⣬������ʵ������
			if (m_WorkMode == WorkMode_Full_RW_Test
				|| m_WorkMode == WorkMode_Fade_Picker
				|| m_WorkMode == WorkMode_Speed_Check
				)
			{
				if (port->GetBadBlockCount() > 0)
				{
					CULLArray badBlocks;
					port->GetBadBlockArray(badBlocks);

					ULONGLONG ullSize = badBlocks.GetAt(0) * port->GetBytesPerSector();

					strItem.Format(_T("%I64d;"),ullSize);
				}
				else
				{
					strItem.Format(_T("%I64d;"),port->GetTotalSize());
				}
			}
			else
			{
				strItem.Format(_T("%I64d;"),port->GetValidSize());
			}
			strLog += strItem;

			strItem.Format(_T("%I64d;"),port->GetTotalSize());
			strLog += strItem;

			strItem.Format(_T("%s;"),GetWorkModeString(m_WorkMode));
			strLog += strItem;

			strItem.Format(_T("%s;"),port->GetStartTime().Format(_T("%Y-%m-%d %H:%M:%S")));
			strLog += strItem;

			strItem.Format(_T("%s;"),port->GetEndTime().Format(_T("%Y-%m-%d %H:%M:%S")));
			strLog += strItem;

			// ����ǿ���⣬������������ٶȡ�д�ٶ�
			if (m_WorkMode == WorkMode_Full_RW_Test
				|| m_WorkMode == WorkMode_Fade_Picker
				|| m_WorkMode == WorkMode_Speed_Check
				)
			{
				strItem.Format(_T("%d;"),port->GetBadBlockCount());
				strLog += strItem;

				strItem.Format(_T("%d;"),(int)port->GetRealSpeed(TRUE));
				strLog += strItem;

				strItem.Format(_T("%d;"),(int)port->GetRealSpeed(FALSE));
				strLog += strItem;
			}
			else
			{
				strItem.Format(_T("%s;"),port->GetHashMethodString());
				strLog += strItem;

				strItem.Format(_T("%s;"),port->GetHashString());
				strLog += strItem;

				strItem.Format(_T("%d;"),(int)port->GetRealSpeed());
				strLog += strItem;
			}

			strItem.Format(_T("%s;"),port->GetResultString());
			strLog += strItem;

			DWORD dwErrorCode = 0;
			ErrorType errType = port->GetErrorCode(&dwErrorCode);

			strItem.Format(_T("%d;"),errType);
			strLog += strItem;

			strItem.Format(_T("%d\r\n"),dwErrorCode);
			strLog += strItem;
		}
	}

	return strLog;
}

void CUSBCopyDlg::WriteUploadLog( CString strLog )
{
	CString strRecordFile;
	CTime time = CTime::GetCurrentTime();
	strRecordFile.Format(_T("%s\\RECORD_%s.txt"),m_strAppPath,time.Format(_T("%Y%m%d")));

	HANDLE hFile = CreateFile(strRecordFile,
							  GENERIC_READ | GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_WRITE,
							  NULL,
							  OPEN_ALWAYS,
							  0,
							  NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	SetFilePointer(hFile,0,NULL,FILE_END);

	USES_CONVERSION;
	char *buf = W2A(strLog);

	DWORD dwSize = 0;
	WriteFile(hFile,buf,strlen(buf),&dwSize,NULL);
	CloseHandle(hFile);
}

void CUSBCopyDlg::SetAllFailed(DWORD dwErrorCode,ErrorType errType)
{
	if (m_MasterPort.IsConnected())
	{
		m_MasterPort.SetResult(FALSE);
		m_MasterPort.SetPortState(PortState_Fail);

		if (dwErrorCode != 0)
		{
			m_MasterPort.SetErrorCode(errType,dwErrorCode);
		}
		
	}

	POSITION pos = m_TargetPorts.GetHeadPosition();

	while (pos)
	{
		CPort *port = m_TargetPorts.GetNext(pos);

		if (port->IsConnected())
		{
			port->SetResult(FALSE);
			port->SetPortState(PortState_Fail);

			if (dwErrorCode != 0)
			{
				port->SetErrorCode(errType,dwErrorCode);
			}
			
		}
	}
}

BOOL CUSBCopyDlg::IsLisence()
{
	CString strListenFile = m_strAppPath + _T("\\lisence.dat");

	HANDLE hFile = CreateFile(strListenFile,
							  GENERIC_READ,
							  FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  0,
							  NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("The lisence file doesn't exist."));
		return FALSE;
	}

	BYTE byFileKey[KEY_LEN] = {NULL};

	DWORD dwReadSize = 0;


	if (!ReadFile(hFile,byFileKey,KEY_LEN,&dwReadSize,NULL))
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Read lisence file failed."));
		CloseHandle(hFile);
		return FALSE;
	}

	if (dwReadSize != KEY_LEN)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Invalid lisence file."));
		CloseHandle(hFile);
		return FALSE;
	}

	CLisence lisence;
	
	lisence.GetLock();
	BYTE *key = lisence.GetKey();

	// �Ƚ�
	for (int i = 0; i < KEY_LEN - 8; i++)
	{
		if (key[i] != byFileKey[i])
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Invalid lisence."));
			CloseHandle(hFile);
			return FALSE;
		}
	}
	CloseHandle(hFile);
	return TRUE;
}

DWORD WINAPI CUSBCopyDlg::ConnectSocketThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;
	
	//pDlg->OnConnectSocket(0,0);
	pDlg->CreateSocketConnect();

	return 0;
}


void CUSBCopyDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	//CDialogEx::OnClose();
	return;
}


afx_msg LRESULT CUSBCopyDlg::OnUpdateFunction(WPARAM wParam, LPARAM lParam)
{
	char *function = (char *)wParam;

	if (function != NULL)
	{
		SetDlgItemText(IDC_TEXT_FUNCTION2,CString(function));
	}

	m_StartTimeTemp = CTime::GetCurrentTime();

	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnSetBurnInText(WPARAM wParam, LPARAM lParam)
{
	InitialBurnInTest(wParam,lParam);
	return 0;
}

DWORD WINAPI CUSBCopyDlg::StopThreadProc( LPVOID lpParm )
{
	CUSBCopyDlg *pDlg = (CUSBCopyDlg *)lpParm;

	pDlg->OnStop();

	return 1;
}


afx_msg LRESULT CUSBCopyDlg::OnInitCurrentWorkmode(WPARAM wParam, LPARAM lParam)
{
	InitialCurrentWorkMode();
	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnExportLogStart(WPARAM wParam, LPARAM lParam)
{
	m_bExportLog = TRUE;
	return 0;
}


afx_msg LRESULT CUSBCopyDlg::OnExportLogEnd(WPARAM wParam, LPARAM lParam)
{
	m_bExportLog = FALSE;
	return 0;
}

void CUSBCopyDlg::CleanRecord()
{
	CString strRecordFile = m_strAppPath + _T("\\RECORD_*.txt");

	CFileFind find;
	BOOL bFind = find.FindFile(strRecordFile);
	CTime time = CTime::GetCurrentTime();
	while (bFind)
	{
		bFind = find.FindNextFile();

		CFileStatus status;
		CFile::GetStatus(find.GetFilePath(),status);

		CTimeSpan span = time - status.m_mtime;

		//ɾ�����������log�ļ�
		if (span.GetDays() > 180)
		{
			DeleteFile(find.GetFilePath());
		}
	}

	find.Close();
}


afx_msg LRESULT CUSBCopyDlg::OnLoadConfig(WPARAM wParam, LPARAM lParam)
{
	m_strConfigPath = m_Config.GetPathName();
	InitialCurrentWorkMode();

	SetDlgItemText(IDC_TEXT_CONFIG,CUtils::GetFileTitle(m_strConfigPath));

	// ����������ص������ļ�
	CIni origin(m_strAppPath + CONFIG_NAME);
	origin.WriteString(_T("AppSetting"),_T("RecentConfig"),m_strConfigPath);
	return 0;
}
