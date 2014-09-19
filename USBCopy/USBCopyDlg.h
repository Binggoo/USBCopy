
// USBCopyDlg.h : header file
//

#pragma once
#include "PortDlg.h"
#include "afxcmn.h"
#include "Ini.h"
#include "Port.h"
#include "SerialPort.h"
#include "PortCommand.h"
#include "BtnST.h"

#define CONFIG_NAME     _T("\\USBCopy.ini")
#define MACHINE_INFO    _T("\\MachineInfo.ini")
#define LOG_FILE     _T("\\USBCopy.log")
#define LOG_FILE_BAK     _T("\\USBCopy.log.bak")
#define MASTER_PATH  _T("M:\\")
#define RECODE_FILE  _T("\\record.txt");
#define LOGO_TS         _T("TF/SD DUPLICATOR")
#define LOGO_USB         _T("USB DUPLICATOR")

#define MAX_RUN_TIMES  500

#define TIMER_UPDATE_STATISTIC 1
#define TIMER_SEND_BITMAP      2
#define TIMER_LISENCE          3

// CUSBCopyDlg dialog
class CUSBCopyDlg : public CDialogEx
{
// Construction
public:
	CUSBCopyDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_USBCOPY_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	CProgressCtrlEx m_ProgressCtrl;
	CPortDlg      *m_PortFrames;
	CRect         m_Rect;
	CString       m_strAppPath;
	HANDLE        m_hLogFile;
	HANDLE        m_hEvent;
	BOOL          m_bLock;
	BOOL          m_bCancel;
	CString       m_strPassWord;

	UINT          m_nPortNum;
	CIni          m_Config;

	CPort         m_MasterPort;
	CPort         m_FilePort;
	PortList      m_TargetPorts;

	WorkMode      m_WorkMode;

	CSerialPort   m_SerialPort;
	CPortCommand  m_Command;

	CTime         m_StartTime;

	CFont         m_font;
	CFont         m_LogoFont;

	CString       m_strMsg;
	BOOL          m_bResult;
	BOOL          m_bVerify;
	BOOL          m_bRunning;
	BOOL          m_bUpdate;

	CWinThread    *m_ThreadListen;

	BOOL          m_bBurnInTest;

	// 网络部分
	SOCKET m_ClientSocket;
	BOOL   m_bSockeConnected;
	BOOL   m_bServerFirst;  // 仅仅在Copy Image时有用

	BOOL  m_bStart; //用于标记当前是开始状态还是结束状态

	BOOL  m_bLisence;

	BOOL  m_bIsUSB;

	enum
	{
		COLUMNS = 6,
		ROWS    = 3
	};

	void ChangeSize( CWnd *pWnd,int cx, int cy );

	void InitialMachine();

	void InitialPortFrame();
	void ResetPortFrame();
	void UpdatePortFrame(BOOL bStart);
	void EnableKickOff(BOOL bEnable);

	void InitialPortPath();
	void InitialSerialPort();
	void ResetPortInfo();

	void InitialCurrentWorkMode();
	void InitialStatisticInfo();
	void UpdateStatisticInfo();

	void InitialBurnInTest();

	// 网络部分
	BOOL CreateSocketConnect();
	BOOL SyncTime();

	void EnableControls(BOOL bEnable);

	BOOL IsReady();
	BOOL IsExistTarget();
	BOOL IsExistMaster();

	CString GetCustomErrorMsg(CustomError customError);
	CString GetWorkModeString(WorkMode mode);

	void OnStart();

	void EnumDevice();
	void MatchDevice();

	void BackupLogfile(HANDLE hFile,DWORD dwFileSize);

	static DWORD WINAPI StartThreadProc(LPVOID lpParm);
	static DWORD WINAPI InitialMachineThreadProc(LPVOID lpParm);
	static DWORD WINAPI EnumDeviceThreadProc(LPVOID lpParm);
	static DWORD WINAPI BurnInTestThreadProc(LPVOID lpParm);
	static DWORD WINAPI ConnectSocketThreadProc(LPVOID lpParm);

	void BurnInTest();
	DWORD UploadLog(CString strLogName,CString strLog);
	CString GetUploadLogString();
	void WriteUploadLog(CString strLog);
	void SetAllFailed(DWORD dwErrorCode,ErrorType errType);
	void OnStop();

	BOOL IsLisence();

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnSystem();
	afx_msg void OnBnClickedBtnWorkSelect();
	afx_msg void OnBnClickedBtnSetting();
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
protected:
	afx_msg LRESULT OnUpdateStatistic(WPARAM wParam, LPARAM lParam);
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
	afx_msg LRESULT OnSendVerifyStart(WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL DestroyWindow();
protected:
	afx_msg LRESULT OnComReceive(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetMachienPort(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg LRESULT OnUpdateSoftware(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetPower(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBurnInTest(WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	afx_msg LRESULT OnConnectSocket(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDisconnectSocket(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSocketMsg(WPARAM wParam, LPARAM lParam);
private:
	CButtonST m_BtnStart;
	CButtonST m_BtnLock;
	CButtonST m_BtnMenu;
	CButtonST m_BtnSetting;
	CButtonST m_BtnWorkMode;
};
