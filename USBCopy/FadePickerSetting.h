#pragma once
#include "Ini.h"

// CFadePickerSetting �Ի���

class CFadePickerSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CFadePickerSetting)

public:
	CFadePickerSetting(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFadePickerSetting();

// �Ի�������
	enum { IDD = IDD_DIALOG_FADE_PICKER };

	void SetConfig(CIni *pIni);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bCheckRetainOriginData;
	BOOL m_bCheckFormatFinished;
	CIni *m_pIni;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheckRetaiOriginData();
	afx_msg void OnBnClickedCheckFormat();
	afx_msg void OnBnClickedOk();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
