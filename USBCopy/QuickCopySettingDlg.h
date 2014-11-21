#pragma once
#include "Ini.h"
#include "afxwin.h"

// CQuickCopySettingDlg 对话框

class CQuickCopySettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CQuickCopySettingDlg)

public:
	CQuickCopySettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CQuickCopySettingDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_QC_SETTING };

	void SetConfig(CIni *pIni);

private:
	CIni *m_pIni;

	BOOL m_bComputeHash;
	BOOL m_bEnCompare;
	BOOL m_bEnMutiMBRSupported;
	CComboBox m_ComboBox;
	BOOL m_bCleanDiskFirst;
	CComboBox m_ComboBoxCleanTimes;
	CString m_strFillValues;

	int m_nCompareMethodIndex;
	BOOL m_bCompareClean;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

	
public:
	afx_msg void OnBnClickedCheckMutiMbr();
	afx_msg void OnBnClickedOk();

	
	afx_msg void OnBnClickedCheckQsComputeHash();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedCheckCleanDisk();
	afx_msg void OnCbnSelchangeComboCleanTimes();
	afx_msg void OnBnClickedRadioHashCompare();
};
