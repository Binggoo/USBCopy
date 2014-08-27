#pragma once
#include "Ini.h"

// CImageNameDlg 对话框

class CImageNameDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CImageNameDlg)

public:
	CImageNameDlg(BOOL bImageMake,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CImageNameDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_IMAGE_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni,SOCKET sClient);
	void SetMakeImage(BOOL bImageMake = TRUE);
	afx_msg void OnBnClickedOk();

	BOOL QueryImage(CString strImageName,PDWORD pdwErrorCode);

	BOOL GetServerFirst();

private:
	CIni *m_pIni;
	CString m_strEditImageName;
	BOOL  m_bImageMake;
	SOCKET m_ClientSocket;

	BOOL   m_bServerFirst; // 0 - 本地，1 - 远端
};
