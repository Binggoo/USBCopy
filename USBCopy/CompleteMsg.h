#pragma once
#include "PortCommand.h"
#include "Ini.h"

// CCompleteMsg �Ի���

class CCompleteMsg : public CDialogEx
{
	DECLARE_DYNAMIC(CCompleteMsg)

public:
	CCompleteMsg(CIni *pIni,CPortCommand *pCommand,CString strMessage,BOOL bPass,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CCompleteMsg();

// �Ի�������
	enum { IDD = IDD_DIALOG_MSG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
