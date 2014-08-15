#pragma once
#include "Ini.h"
#include "afxwin.h"
#include "GlobalDef.h"

// CBurnIn �Ի���

class CBurnIn : public CDialogEx
{
	DECLARE_DYNAMIC(CBurnIn)

public:
	CBurnIn(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CBurnIn();

// �Ի�������
	enum { IDD = IDD_DIALOG_BURN_IN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
