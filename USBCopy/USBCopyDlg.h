
// USBCopyDlg.h : header file
//

#pragma once
#include "PortDlg.h"
#include "afxcmn.h"
#include "Ini.h"
#include "Port.h"
#include "SerialPort.h"
#include "PortCommand.h"

#define CONFIG_NAME     _T("\\USBCopy.INI")
#define MACHINE_INFO    _T("\\MachineInfo.INI")
#define LOG_FILE     _T("\\USBCopy.LOG")

#define TIMER_UPDATE_STATISTIC 1
#define WM_UPDATE_STATISTIC (WM_USER + 1)
#define WM_RESET_MACHIEN_PORT (WM_USER + 2)
#define WM_COMPLETE (WM_USER + 3)

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

	CString       m_strMsg;
	BOOL          m_bResult;

	enum
	{
		COLUMNS = 6,
		ROWS    = 3
	};

	void ChangeSize( CWnd *pWnd,int cx, int cy );

	void InitailMachine();

	void InitialPortFrame();
	void ResetPortFrame();
	void UpdatePortFrame(BOOL bStart);

	void InitialPortPath();
	void InitialSerialPort();
	void ResetPortInfo();

	void InitialCurrentWorkMode();
	void InitialStatisticInfo();
	void UpdateStatisticInfo();

	void EnableControls(BOOL bEnable);

	BOOL IsReady();
	BOOL IsExistTarget();
	BOOL IsExistMaster();

	CString GetCustomErrorMsg(CustomError customError);

	void OnStart();

	static DWORD StartThreadProc(LPVOID lpParm);
	static DWORD InitialMachineThreadProc(LPVOID lpParm);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnSystem();
	afx_msg void OnBnClickedBtnWorkSelect();
	afx_msg void OnBnClickedBtnSetting();
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnStop();
protected:
	afx_msg LRESULT OnUpdateStatistic(WPARAM wParam, LPARAM lParam);
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
	afx_msg LRESULT OnSendFunctionText(WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL DestroyWindow();
protected:
	afx_msg LRESULT OnComReceive(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetMachienPort(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnComplete(WPARAM wParam, LPARAM lParam);
};
