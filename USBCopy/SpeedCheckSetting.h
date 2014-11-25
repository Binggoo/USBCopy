#pragma once
#include "Ini.h"

// CSpeedCheckSetting 对话框

class CSpeedCheckSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CSpeedCheckSetting)

public:
	CSpeedCheckSetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSpeedCheckSetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_SPEED_CHECK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	BOOL m_bCheckSpeedUpStandard;
	double m_dbEditReadSpeed;
	double m_dbEditWriteSpeed;

	CIni *m_pIni;
public:
	afx_msg void OnBnClickedCheckSpeedUpStandard();
	afx_msg void OnBnClickedOk();

	void SetConfig(CIni *pIni);
};
