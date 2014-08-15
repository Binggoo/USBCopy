#pragma once
#include "Ini.h"
#include "afxwin.h"
#include "GlobalDef.h"

// CBurnIn 对话框

class CBurnIn : public CDialogEx
{
	DECLARE_DYNAMIC(CBurnIn)

public:
	CBurnIn(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CBurnIn();

// 对话框数据
	enum { IDD = IDD_DIALOG_BURN_IN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
private:
	CComboBox m_ComboBoxCycleCount;
	CListBox m_ListBoxFunction;
	CComboBox m_ComboBoxFunction;

	CIni *m_pIni;

	CString GetWorkModeString(WorkMode workMode);

public:
	afx_msg void OnBnClickedBtnDel();
	afx_msg void OnBnClickedBtnUp();
	afx_msg void OnBnClickedBtnDown();
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedOk();

	void SetConfig(CIni *pIni);
	afx_msg void OnCbnEditchangeComboCycleCount();
};
