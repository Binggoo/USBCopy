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
	int m_nRadioPriorityIndex;
	BOOL m_bCheckCompare;
	CIni *m_pIni;
	int m_nRadioImageTypeIndex;
	BOOL m_bCleanDiskFirst;
	CComboBox m_ComboBoxCleanTimes;
	CString m_strFillValues;

	int m_nCompareMethodIndex;
	BOOL m_bCompareClean;
	int m_nCompareCleanSeqIndex;

public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioDiskImage();
	afx_msg void OnBnClickedRadioMtpImage();
	afx_msg void OnBnClickedCheckCleanDisk();
	afx_msg void OnCbnSelchangeComboCleanTimes();
	afx_msg void OnBnClickedCheckCompare();
};
