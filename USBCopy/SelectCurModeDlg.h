#pragma once
#include "GlobalDef.h"
#include "Ini.h"
#include "BtnST.h"

// CSelectCurModeDlg 对话框

class CSelectCurModeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSelectCurModeDlg)

public:
	CSelectCurModeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSelectCurModeDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_WORK_MODE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	WorkMode m_WorkMode;
	CIni *m_pIni;

	CFont m_font;

	CButtonST m_BtnFullCopy;
	CButtonST m_BtnQuickCopy;
	CButtonST m_BtnFileCopy;
	CButtonST m_BtnImageCopy;
	CButtonST m_BtnImageMake;
	CButtonST m_BtnDiskCompare;
	CButtonST m_BtnTools;
	CButtonST m_BtnDiskClean;
	CButtonST m_BtnReturn;
	CButtonST m_BtnDiffCopy;
	CButtonST m_BtnMTPCopy;

public:
	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedButtonFullCopy();
	afx_msg void OnBnClickedButtonQuickCopy();
	afx_msg void OnBnClickedButtonDiskClean();
	afx_msg void OnBnClickedButtonMakeImage();
	afx_msg void OnBnClickedButtonCopyFromImage();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnFileCopy();
	afx_msg void OnBnClickedBtnDiskCompare();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedBtnTools();
	afx_msg void OnBnClickedBtnReturn();
	afx_msg void OnBnClickedBtnDifferenceCopy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedBtnMtpCopy();
protected:
	afx_msg LRESULT OnWorkModeSelect(WPARAM wParam, LPARAM lParam);
};
