#pragma once
#include "Ini.h"
#include "Port.h"
#include "afxcmn.h"

// CDiffCopySetting 对话框

class CDiffCopySetting : public CDialogEx
{
	DECLARE_DYNAMIC(CDiffCopySetting)

public:
	CDiffCopySetting(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDiffCopySetting();

// 对话框数据
	enum { IDD = IDD_DIALOG_DIFFC_SETTING };

	void SetConfig(CIni *pIni,CPort *port);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	CIni *m_pIni;
	CPort *m_MasterPort;
	CString m_strMasterPath;
	int m_nRadioSourceType;
	int m_nRadioCompareRule;
	int m_nRadioLocation;
	BOOL m_bCheckComputeHash;
	BOOL m_bCheckCompare;
	BOOL m_bCheckUpload;
	BOOL m_bCheckIncludeSub;
	CListCtrl m_ListCtrl;

	void InitialListCtrl();
	void UpdateListCtrl();
	BOOL IsAdded(CString strPath,CString strExt);

public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedCheckComputeHash();
	afx_msg void OnBnClickedCheckCompare();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnRemove();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheckUserLog();
	afx_msg void OnBnClickedRadioPkg();
	afx_msg void OnBnClickedRadioMaster();
};
