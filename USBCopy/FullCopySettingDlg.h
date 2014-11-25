#pragma once
#include "Ini.h"

// CFullCopySettingDlg �Ի���

class CFullCopySettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFullCopySettingDlg)

public:
	CFullCopySettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFullCopySettingDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_FC_SETTING };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bComputeHash;
	BOOL m_bCompare;
	BOOL m_bAllowCapGap;
	BOOL m_bCleanDiskFirst;
	CComboBox m_ComboBoxCapGap;
	CComboBox m_ComboBoxCleanTimes;
	CString m_strFillValues;
	int m_nCompareMethodIndex;
	BOOL m_bCompareClean;

	int m_nCompareCleanSeqIndex;

	CIni *m_pIni;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedCheckComputeHash();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnEditchangeComboCapGap();
	afx_msg void OnBnClickedCheckCapaGap();
	afx_msg void OnBnClickedCheckCleanDisk();
	afx_msg void OnCbnSelchangeComboCleanTimes();
	afx_msg void OnBnClickedRadioHashCompare();
};
