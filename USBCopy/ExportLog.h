#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CExportLog �Ի���

class CExportLog : public CDialogEx
{
	DECLARE_DYNAMIC(CExportLog)

public:
	CExportLog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CExportLog();

// �Ի�������
	enum { IDD = IDD_DIALOG_EXPORT_LOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
