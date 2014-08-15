#pragma once

// CPassword 对话框

class CPassword : public CDialogEx
{
	DECLARE_DYNAMIC(CPassword)

public:
	CPassword(BOOL bSet,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPassword();

// 对话框数据
	enum { IDD = IDD_DIALOG_PASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bSet;
	CString m_strPassword;

	void AppendPassword(CString strText);

public:
	virtual BOOL OnInitDialog();

	void SetPassword(CString strPassword);
	CString GetPassword();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton10();
	afx_msg void OnBnClickedBtnClear();
	afx_msg void OnBnClickedBtnBackspace();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
