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
	int m_nCompareModeIndex; // 0 - ȫ�̱ȶԣ� 1 - ���ٱȶ�
	CIni *m_pIni;
	int m_nCompareMethodIndex; // 0 - hash�ȶԣ� 1 - byte �ȶ�
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
