#pragma once
#include "Ini.h"

// CSpeedCheckSetting �Ի���

class CSpeedCheckSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CSpeedCheckSetting)

public:
	CSpeedCheckSetting(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSpeedCheckSetting();

// �Ի�������
	enum { IDD = IDD_DIALOG_SPEED_CHECK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
