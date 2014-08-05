#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Port.h"
#include "ProgressCtrlEx.h"
#include "PortCommand.h"

// CPortDlg 对话框

class CPortDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPortDlg)

public:
	CPortDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPortDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_PORT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	CStatic m_PictureCtrol;
	CProgressCtrlEx m_ProgressCtrl;
	CBitmap       m_Bitmap;
	CRect   m_Rect;
	CPort   *m_Port;
	PortList *m_PortList;
	CPortCommand *m_pCommand;

	void ChangeSize( CWnd *pWnd,int cx, int cy );

	BOOL IsSlowest();

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void SetPort(CPortCommand *pCommand,CPort *port,PortList *pPortList);
	CPort *GetPort();
	void Update(BOOL bStart = TRUE);
	void Initial();
	void SetBitmap(UINT nResource);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
