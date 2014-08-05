#pragma once
#include "Ini.h"


// CImageCopySetting 对话框

class CImageCopySetting : public CDialogEx
{
	DECLARE_DYNAMIC(CImageCopySetting)

public:
	CImageCopySetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CImageCopySetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_IC_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	int m_nRadioSelectIndex;
	BOOL m_bCheckCompare;
	CIni *m_pIni;
public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedOk();
};
