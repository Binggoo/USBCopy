#pragma once


// CPassword �Ի���
#define DEFAULT_PASSWORD  _T("123456")

class CPassword : public CDialogEx
{
	DECLARE_DYNAMIC(CPassword)

public:
	CPassword(BOOL bSet,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPassword();

// �Ի�������
	enum { IDD = IDD_DIALOG_PASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
