#pragma once
#include "Ini.h"
#include "afxwin.h"

// CDiskCleanSettingDlg 对话框

class CDiskCleanSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskCleanSettingDlg)

public:
	CDiskCleanSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDiskCleanSettingDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_DC_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	CIni *m_pIni;
	CComboBox m_ComboBoxFullCleanValue;
	int m_nRadioSelectIndex;
public:
	virtual BOOL OnInitDialog();
	
	afx_msg void OnBnClickedRadioFullClean();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioQuickClean();
	afx_msg void OnBnClickedRadioSafeClean();
};
