#pragma once
#include "afxwin.h"
#include "Ini.h"

// CImageMakeSetting 对话框

class CImageMakeSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CImageMakeSetting)

public:
	CImageMakeSetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CImageMakeSetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_IM_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	int m_nRadioSelectIndex;
	BOOL m_bCheckSupportMutiMBR;
	CComboBox m_ComboBoxMBRLBA;
	CIni *m_pIni;
	int m_nRadioSaveModeIndex;
public:
	virtual BOOL OnInitDialog();

	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedCheckMutiMbr();
	afx_msg void OnBnClickedOk();
};
