#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CExportLog 对话框

class CExportLog : public CDialogEx
{
	DECLARE_DYNAMIC(CExportLog)

public:
	CExportLog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CExportLog();

// 对话框数据
	enum { IDD = IDD_DIALOG_EXPORT_LOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CComboBox m_ComboBoxDate;
	CListCtrl m_ListCtrl;
	CString m_strAppPath;
	CStringArray m_strFileArray;

	void InitialListCtrl();
	void AddListData(DWORD days);
public:
	afx_msg void OnBnClickedBtnDel();
	afx_msg void OnBnClickedOk();

	void GetFileArray(CStringArray &strArray);
	afx_msg void OnCbnEditchangeComboDate();
	afx_msg void OnCbnSelchangeComboDate();
};
