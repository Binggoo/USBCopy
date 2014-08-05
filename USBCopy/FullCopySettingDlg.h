#pragma once
#include "Ini.h"

// CFullCopySettingDlg 对话框

class CFullCopySettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFullCopySettingDlg)

public:
	CFullCopySettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFullCopySettingDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_FC_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bComputeHash;
	BOOL m_bCompare;

	CIni *m_pIni;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedCheckComputeHash();
	afx_msg void OnBnClickedOk();
};
