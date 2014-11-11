#pragma once
#include "Ini.h"
#include "Port.h"

// CPackageName 对话框

class CPackageName : public CDialogEx
{
	DECLARE_DYNAMIC(CPackageName)

public:
	CPackageName(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPackageName();

// 对话框数据
	enum { IDD = IDD_DIALOG_PKG_NAME };

	void SetConfig(CIni *pIni,CPort *port,SOCKET sClient);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	CIni *m_pIni;
	SOCKET m_ClientSocket;
	CPort *m_MasterPort;

	CString m_strEditPackageName;

	BOOL QueryPackage(CString strPackageName,PDWORD pdwErrorCode);
	BOOL QueryChangeList(CString strChangeList,PDWORD pdwErrorCode);

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
