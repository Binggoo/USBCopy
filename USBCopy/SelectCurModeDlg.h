#pragma once
#include "GlobalDef.h"
#include "Ini.h"

// CSelectCurModeDlg �Ի���

class CSelectCurModeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSelectCurModeDlg)

public:
	CSelectCurModeDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSelectCurModeDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_WORK_MODE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

private:
	WorkMode m_WorkMode;
	CIni *m_pIni;

public:
	void SetConfig(CIni *pIni);
	afx_msg void OnBnClickedButtonFullCopy();
	afx_msg void OnBnClickedButtonQuickCopy();
	afx_msg void OnBnClickedButtonDiskClean();
	afx_msg void OnBnClickedButtonMakeImage();
	afx_msg void OnBnClickedButtonCopyFromImage();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnFileCopy();
	afx_msg void OnBnClickedBtnDiskCompare();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
