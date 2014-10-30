#pragma once
#include "Ini.h"
#include "PortCommand.h"
#include "BtnST.h"

// CSystemMenu 对话框

class CSystemMenu : public CDialogEx
{
	DECLARE_DYNAMIC(CSystemMenu)

public:
	CSystemMenu(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSystemMenu();

// 对话框数据
	enum { IDD = IDD_DIALOG_SYSTEM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni,CPortCommand *pCommand,BOOL bConnected,BOOL bLisence);

private:
	CIni *m_pIni;
	CPortCommand *m_pCommand;
	CString m_strAppPath;
	UINT m_nTargetNum;
	BOOL m_bSocketConnected;
	BOOL m_bLisence;

	CButtonST m_BtnUpdate;
	CButtonST m_BtnSetting;
	CButtonST m_BtnSyncImage;
	CButtonST m_BtnImageManager;
	CButtonST m_BtnExport;
	CButtonST m_BtnReturn;
	CButtonST m_BtnMore;
	CButtonST m_BtnPackage;

public:
	afx_msg void OnBnClickedButtonUpdate();
	afx_msg void OnBnClickedButtonRestore();
	afx_msg void OnBnClickedButtonGlobal();
	afx_msg void OnBnClickedButtonSynchImage();
	afx_msg void OnBnClickedButtonImageManager();
	afx_msg void OnBnClickedButtonViewLog();
	afx_msg void OnBnClickedButtonExportLog();
	afx_msg void OnBnClickedButtonBurnIn();
	afx_msg void OnBnClickedButtonDebug();
	afx_msg void OnBnClickedButtonReturn();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedBtnMore();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
