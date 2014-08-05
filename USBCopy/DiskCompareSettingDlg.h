#pragma once
#include "Ini.h"

// CDiskCompareSettingDlg �Ի���

class CDiskCompareSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskCompareSettingDlg)

public:
	CDiskCompareSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDiskCompareSettingDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_CMP_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
private:
	int m_nCompareModeIndex;
	CIni *m_pIni;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
