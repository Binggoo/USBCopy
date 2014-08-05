#pragma once


// CPassword 对话框
#define DEFAULT_PASSWORD  _T("123456")

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

public:
	virtual BOOL OnInitDialog();

	void SetPassword(CString strPassword);
	CString GetPassword();
	afx_msg void OnBnClickedOk();
};
