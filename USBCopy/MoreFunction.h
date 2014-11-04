#pragma once
#include "BtnST.h"

// CMoreFunction �Ի���

class CMoreFunction : public CDialogEx
{
	DECLARE_DYNAMIC(CMoreFunction)

public:
	CMoreFunction(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CMoreFunction();

// �Ի�������
	enum { IDD = IDD_DIALOG_MORE_FUNCTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

private:
	CButtonST m_BtnSoftwareRecovery;
	CButtonST m_BtnViewLog;
	//CButtonST m_BtnBurnIn;
	CButtonST m_BtnDebug;

	HWND m_hParent;

public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnBnClickedBtnSoftwareRecovery();
	afx_msg void OnBnClickedBtnViewLog();
	afx_msg void OnBnClickedBtnBurnIn();
	afx_msg void OnBnClickedBtnDebug();
};
