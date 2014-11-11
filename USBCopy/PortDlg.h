#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Port.h"
#include "ProgressCtrlEx.h"
#include "PortCommand.h"
#include "Ini.h"

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

	BOOL m_bIsUSB;

	BOOL m_bOnlineCommand;
	BOOL m_bStopCommand;
	CIni *m_pIni;
	HANDLE m_hLogFile;
	volatile BOOL m_bEnableKickOff;
	UINT m_nKickOffCount;

	CToolTipCtrl m_Tooltips;

	CFont m_font;

	void ChangeSize( CWnd *pWnd,int cx, int cy );

	BOOL IsSlowest();

	void UpdateState();

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void SetPort(CIni *pIni,HANDLE hLogFile,CPortCommand *pCommand,CPort *port,PortList *pPortList,BOOL bIsUSB);
	CPort *GetPort();
	void Update(BOOL bStart = TRUE);
	void EnableKickOff(BOOL bEnable);
	void Initial();
	void SetBitmap(UINT nResource);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
