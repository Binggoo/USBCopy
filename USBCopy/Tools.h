#pragma once
#include "BtnST.h"
// CTools 对话框

class CTools : public CDialogEx
{
	DECLARE_DYNAMIC(CTools)

public:
	CTools(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CTools();

// 对话框数据
	enum { IDD = IDD_DIALOG_TOOLS };

private:
	CButtonST m_BtnDiskFormat;
	CButtonST m_BtnFullRWTest;
	CButtonST m_BtnFakePicker;
	CButtonST m_BtnSpeedCheck;
	CButtonST m_BtnBurnIn;

	HWND m_hParent;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnBnClickedBtnDiskFormart();
	afx_msg void OnBnClickedBtnFullRwTest();
	afx_msg void OnBnClickedBtnFakePicker();
	afx_msg void OnBnClickedBtnSpeedCheck();
	afx_msg void OnBnClickedBtnBurnIn();
};
