#pragma once
#include "PortCommand.h"
#include "Ini.h"

// CCompleteMsg 对话框

class CCompleteMsg : public CDialogEx
{
	DECLARE_DYNAMIC(CCompleteMsg)

public:
	CCompleteMsg(CIni *pIni,CPortCommand *pCommand,CString strMessage,BOOL bPass,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CCompleteMsg();

// 对话框数据
	enum { IDD = IDD_DIALOG_MSG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bPass;
	CFont m_font;
	CString m_strMessage;
	BOOL  m_bStop;
	CWinThread *m_ThreadBuzzer;
	CIni *m_pIni;

	void Buzzer();

	static DWORD WINAPI BuzzerThreadProc(LPVOID lpParm);

	CPortCommand *m_pCommand;
public:
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual BOOL DestroyWindow();
};
