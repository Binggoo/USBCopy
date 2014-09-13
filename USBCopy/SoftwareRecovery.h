#pragma once
#include "afxcmn.h"


// CSoftwareRecovery 对话框

class CSoftwareRecovery : public CDialogEx
{
	DECLARE_DYNAMIC(CSoftwareRecovery)

public:
	CSoftwareRecovery(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSoftwareRecovery();

// 对话框数据
	enum { IDD = IDD_DIALOG_RESTORE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CListCtrl m_SoftwareList;
	CString   m_strAppPath;
	
	void InitialListCtrl();
	void AddListData();
public:
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedOk();
};
