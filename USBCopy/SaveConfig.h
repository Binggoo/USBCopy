#pragma once


// CSaveConfig �Ի���

class CSaveConfig : public CDialogEx
{
	DECLARE_DYNAMIC(CSaveConfig)

public:
	CSaveConfig(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSaveConfig();

// �Ի�������
	enum { IDD = IDD_DIALOG_SAVE_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bCheckOverwrite;
	CString m_strEditConfigName;

	CString m_strSourcePath;
	CString m_strDestPath;

public:
	afx_msg void OnBnClickedOk();

	void SetSourcePath(CString strSourcePath);
	void SetDestPath(CString strDestPath);
};
