#pragma once
#include "Ini.h"

// CMTPCopySetting 对话框

class CMTPCopySetting : public CDialogEx
{
	DECLARE_DYNAMIC(CMTPCopySetting)

public:
	CMTPCopySetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CMTPCopySetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_MTP_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bComputeHash;
	BOOL m_bCompare;

	CIni *m_pIni;

	int m_nCompareMethodIndex;

public:
	virtual BOOL OnInitDialog();


	afx_msg void OnBnClickedCheckComputeHash();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedOk();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedRadioHashCompare();
};
