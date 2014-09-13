#pragma once
#include "afxcmn.h"


// CSoftwareRecovery �Ի���

class CSoftwareRecovery : public CDialogEx
{
	DECLARE_DYNAMIC(CSoftwareRecovery)

public:
	CSoftwareRecovery(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSoftwareRecovery();

// �Ի�������
	enum { IDD = IDD_DIALOG_RESTORE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
