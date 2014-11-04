#pragma once
#include "Port.h"
#include "Ini.h"

// CScanningDlg �Ի���

class CScanningDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CScanningDlg)

public:
	CScanningDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CScanningDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_SCAN };

	void SetDeviceInfoList(CPort *pMaster,PortList *pTargetPortList);
	void SetBegining(BOOL bBeginning = TRUE);
	void SetConfig(CIni *pIni,WorkMode workMode,BOOL bIsUSB);
	void SetLogFile(HANDLE hFile);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

	void ScanningDevice();
	void ScanningMTPDevice();
	BOOL IsAllConnected();

private:
	PortList *m_pTargetPortList;
	CPort    *m_pMasterPort;
	int m_nWaitTimeS;
	BOOL m_bBeginning;
	CFont m_font;
	CIni *m_pIni;
	DWORD m_dwOldTime;
	DWORD m_dwCurrentTime;
	HANDLE m_hLogFile;
	WorkMode m_WorkMode;
	BOOL m_bIsUSB;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
