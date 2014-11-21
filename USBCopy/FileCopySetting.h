#pragma once
#include "Ini.h"
#include "Port.h"
#include "afxcmn.h"

// CFileCopySetting �Ի���

class CFileCopySetting : public CDialogEx
{
	DECLARE_DYNAMIC(CFileCopySetting)

public:
	CFileCopySetting(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFileCopySetting();

// �Ի�������
	enum { IDD = IDD_DIALOG_FILE_COPY_SETTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()

private:
	CIni *m_pIni;
	CPort *m_MasterPort;
	CString m_strMasterPath;

	CListCtrl m_ListCtrl;
	BOOL m_bCheckComputeHash;
	BOOL m_bCheckCompare;
	int m_nCompareMethodIndex; // 0 - hash�ȶԣ� 1 - byte �ȶ�

	void InitialListCtrl();

	void UpdateListCtrl();

	BOOL IsAdded(CString strItem);

	static DWORD WINAPI UpdateListCtrlThreadProc(LPVOID lpParm);
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

public:
	virtual BOOL OnInitDialog();

	void SetConfig(CIni *pIni,CPort *port);
	
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedBtnAddFolder();
	afx_msg void OnBnClickedBtnAddFile();
	afx_msg void OnBnClickedBtnDel();
	afx_msg void OnBnClickedCheckComputeHash();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioHashCompare();
};
