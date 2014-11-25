#pragma once
#include "Ini.h"

// CFullRWTest 对话框

class CFullRWTest : public CDialogEx
{
	DECLARE_DYNAMIC(CFullRWTest)

public:
	CFullRWTest(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFullRWTest();

// 对话框数据
	enum { IDD = IDD_DIALOG_FULL_RW_TEST };

	void SetConfig(CIni *pIni);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	BOOL m_bCheckRetainOriginData;
	BOOL m_bCheckFormatFinish;
	BOOL m_bCheckStopBad;
	BOOL m_bCheckReadOnlyTest;

	CIni *m_pIni;
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheckRetaiOriginData();
	afx_msg void OnBnClickedCheckFormat();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedCheckReadOnly();
};
