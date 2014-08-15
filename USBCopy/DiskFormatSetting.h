#pragma once
#include "afxwin.h"
#include "Ini.h"

// CDiskFormatSetting 对话框

class CDiskFormatSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskFormatSetting)

public:
	CDiskFormatSetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDiskFormatSetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_DF_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	CString m_strEditLabel;
	CComboBox m_ComboBoxFileSystem;
	CComboBox m_ComboBoxClusterSize;
	BOOL m_bCheckQuickFormat;

	CIni *m_pIni;
public:
	virtual BOOL OnInitDialog();

	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEditLabel();
};
