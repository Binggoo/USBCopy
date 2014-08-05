#pragma once
#include "Ini.h"
#include "afxwin.h"

// CDiskCleanSettingDlg �Ի���

class CDiskCleanSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskCleanSettingDlg)

public:
	CDiskCleanSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDiskCleanSettingDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_DC_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
private:
	CIni *m_pIni;
	CComboBox m_ComboBoxFullCleanValue;
	int m_nRadioSelectIndex;
public:
	virtual BOOL OnInitDialog();
	
	afx_msg void OnBnClickedRadioFullClean();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioQuickClean();
	afx_msg void OnBnClickedRadioSafeClean();
};
