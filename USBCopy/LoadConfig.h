#pragma once
#include "afxcmn.h"

// CLoadConfig 对话框

class CLoadConfig : public CDialogEx
{
	DECLARE_DYNAMIC(CLoadConfig)

public:
	CLoadConfig(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLoadConfig();

// 对话框数据
	enum { IDD = IDD_DIALOG_LOAD_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	CListCtrl m_ListCtrl;
	CString m_strAppPath;
	CString m_strConfigName;

	void InitialListCtrl();
	void AddListData();
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnDel();
	afx_msg void OnBnClickedOk();

	CString GetPathName();
};
