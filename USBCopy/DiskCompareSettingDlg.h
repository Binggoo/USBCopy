#pragma once
#include "Ini.h"

// CDiskCompareSettingDlg 对话框

class CDiskCompareSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskCompareSettingDlg)

public:
	CDiskCompareSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDiskCompareSettingDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_CMP_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	int m_nCompareModeIndex;
	CIni *m_pIni;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
