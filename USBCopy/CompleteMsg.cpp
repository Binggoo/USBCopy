// CompleteMsg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "CompleteMsg.h"
#include "afxdialogex.h"
#include "USBCopyDlg.h"

// CCompleteMsg �Ի���

IMPLEMENT_DYNAMIC(CCompleteMsg, CDialogEx)

CCompleteMsg::CCompleteMsg(CString strMessage,BOOL bPass,CWnd* pParent /*=NULL*/)
	: CDialogEx(CCompleteMsg::IDD, pParent)
{
	m_strMessage = strMessage;
	m_bPass = bPass;
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


// CCompleteMsg ��Ϣ�������


BOOL CCompleteMsg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��

	SetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE,GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)|WS_EX_LAYERED);

	SetLayeredWindowAttributes(RGB(255,255,255),200,LWA_ALPHA);

	m_font.CreatePointFont(180,_T("Arial"));

	GetDlgItem(IDC_TEXT_MSG)->SetFont(&m_font);

	SetDlgItemText(IDC_TEXT_MSG,m_strMessage);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


BOOL CCompleteMsg::DestroyWindow()
{
	// TODO: �ڴ����ר�ô����/����û���
	::PostMessage(GetParent()->GetSafeHwnd(),WM_RESET_MACHIEN_PORT,0,0);

	return CDialogEx::DestroyWindow();
}


HBRUSH CCompleteMsg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  �ڴ˸��� DC ���κ�����
	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_TEXT_MSG:
		pDC->SetBkMode(TRANSPARENT);

		if (m_bPass)
		{
			pDC->SetTextColor(RGB(0,139,0));
		}
		else
		{
			pDC->SetTextColor(RGB(255,0,0));
		}
		break;
	}

	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	return hbr;
}


BOOL CCompleteMsg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: �ڴ����ר�ô����/����û���

	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_LBUTTONDOWN)
	{
		CDialogEx::OnOK();
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CCompleteMsg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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
