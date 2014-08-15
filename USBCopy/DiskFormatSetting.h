#pragma once
#include "afxwin.h"
#include "Ini.h"

// CDiskFormatSetting �Ի���

class CDiskFormatSetting : public CDialogEx
{
	DECLARE_DYNAMIC(CDiskFormatSetting)

public:
	CDiskFormatSetting(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDiskFormatSetting();

// �Ի�������
	enum { IDD = IDD_DIALOG_DF_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
