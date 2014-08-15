// CompleteMsg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "CompleteMsg.h"
#include "afxdialogex.h"

// CCompleteMsg 对话框

IMPLEMENT_DYNAMIC(CCompleteMsg, CDialogEx)

CCompleteMsg::CCompleteMsg(CIni *pIni,CPortCommand *pCommand,CString strMessage,BOOL bPass,CWnd* pParent /*=NULL*/)
	: CDialogEx(CCompleteMsg::IDD, pParent)
{
	m_strMessage = strMessage;
	m_bPass = bPass;
	m_pCommand = pCommand;
	m_pIni = pIni;

	m_strMessage.Trim();

	if (m_strMessage.IsEmpty())
	{
		m_strMessage.LoadString(IDS_MSG_COMPLETE);
	}
	m_bStop = FALSE;
	m_ThreadBuzzer = NULL;
}

CCompleteMsg::~CCompleteMsg()
{
}

void CCompleteMsg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCompleteMsg, CDialogEx)
	ON_WM_CTLCOLOR()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CCompleteMsg 消息处理程序


BOOL CCompleteMsg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pCommand);

	SetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE,GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)|WS_EX_LAYERED);

	SetLayeredWindowAttributes(RGB(255,255,255),200,LWA_ALPHA);

	m_font.CreatePointFont(160,_T("Arial"));

	GetDlgItem(IDC_TEXT_MSG)->SetFont(&m_font);

	SetDlgItemText(IDC_TEXT_MSG,m_strMessage);

	BOOL bBeep = m_pIni->GetBool(_T("Option"),_T("BeepWhenFinish"),TRUE);

	if (bBeep)
	{
		m_ThreadBuzzer = AfxBeginThread((AFX_THREADPROC)BuzzerThreadProc,this,0,CREATE_SUSPENDED);
		m_ThreadBuzzer->m_bAutoDelete = FALSE;
		m_ThreadBuzzer->ResumeThread();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

HBRUSH CCompleteMsg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_TEXT_MSG:
		pDC->SetBkMode(TRANSPARENT);

		if (m_bPass)
		{
			//pDC->SetTextColor(RGB(0,139,0));
			hbr = CreateSolidBrush(RGB(0,255,0));
		}
		else
		{
			//pDC->SetTextColor(RGB(255,0,0));
			hbr = CreateSolidBrush(RGB(255,0,0));
		}
		break;
	}

	

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


BOOL CCompleteMsg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类

	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_LBUTTONDOWN)
	{
		CDialogEx::OnOK();
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CCompleteMsg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_TEXT_MSG:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}

void CCompleteMsg::Buzzer()
{
	m_pCommand->Buzzer(TRUE);

	int time = 5;
	while (!m_bStop && time > 0)
	{
		Sleep(1000);
		time--;
	}

	m_pCommand->Buzzer(FALSE);
}

DWORD WINAPI CCompleteMsg::BuzzerThreadProc( LPVOID lpParm )
{
	CCompleteMsg *pDlg = (CCompleteMsg *)lpParm;
	pDlg->Buzzer();
	return 1;
}

BOOL CCompleteMsg::DestroyWindow()
{
	// TODO: 在此添加专用代码和/或调用基类

	m_bStop = TRUE;

	if (m_ThreadBuzzer)
	{
		WaitForSingleObject(m_ThreadBuzzer->m_hThread,INFINITE);
		delete m_ThreadBuzzer;
		m_ThreadBuzzer = NULL;
	}
	
	return CDialogEx::DestroyWindow();
}
