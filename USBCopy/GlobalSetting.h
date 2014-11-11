#pragma once
#include "afxwin.h"
#include "Ini.h"
#include "afxcmn.h"

// CGlobalSetting 对话框

class CGlobalSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CGlobalSetting)

public:
	CGlobalSetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CGlobalSetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_GLOBAL_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni,BOOL bConnected);

private:
	CComboBox m_ComboBoxScanTime;
	CComboBox m_ComboBoxBlockSectors;
	CComboBox m_ComboBoxKickOffTimeInterval;
	BOOL m_bCheckRelativeSpeed;
	UINT m_nRelativeSpeed;
	BOOL m_bCheckAbsoluteSpeed;
	UINT m_nAbsolteSpeed;
	BOOL m_bCheckUploadLog;
	int m_iHashMethod;
	CString m_strEditAlias;

	BOOL m_bSocketConnected;

	CIni *m_pIni;
	UINT m_nListenPort;
	BOOL m_bCheckShowCursor;
	BOOL m_bCheckBeep;
	CString m_strEditServerIP;
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnEditchangeComboScanDiskTime();
	afx_msg void OnCbnEditchangeComboDelayOffTime();
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnCbnEditchangeComboKickOffTimeInterval();
};
