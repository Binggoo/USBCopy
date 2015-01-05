#pragma once
#include "afxcmn.h"
#include "Ini.h"
#include "PortCommand.h"
// CImageManager 对话框

class CImageManager : public CDialogEx
{
	DECLARE_DYNAMIC(CImageManager)

public:
	CImageManager(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CImageManager();

// 对话框数据
	enum { IDD = IDD_DIALOG_IMAGE_MANAGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DWORD EnumImage();
	void InitailListCtrl();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void SetConfig(CIni *pIni,CPortCommand *pCommand,UINT nMachineType);
	
private:
	CListCtrl m_ListImages;
	CIni *m_pIni;
	CPortCommand *m_pCommand;
	UINT m_nTargetNum;
	UINT m_nMachineType;

	CString m_strImagePath;
	CString m_strTotalSpace;
	CString m_strUsedSpace;
	CString m_strTotalImages;

	void RunSynchImage();

	static DWORD WINAPI RunSynchImageThreadProc(LPVOID lpParm);

public:
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedBtnSync();
};
