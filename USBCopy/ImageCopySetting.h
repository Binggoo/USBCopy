#pragma once
#include "Ini.h"


// CImageCopySetting �Ի���

class CImageCopySetting : public CDialogEx
{
	DECLARE_DYNAMIC(CImageCopySetting)

public:
	CImageCopySetting(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CImageCopySetting();

// �Ի�������
	enum { IDD = IDD_DIALOG_IC_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
