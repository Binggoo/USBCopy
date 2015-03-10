#pragma once


// CSaveConfig 对话框

class CSaveConfig : public CDialogEx
{
	DECLARE_DYNAMIC(CSaveConfig)

public:
	CSaveConfig(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSaveConfig();

// 对话框数据
	enum { IDD = IDD_DIALOG_SAVE_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

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
