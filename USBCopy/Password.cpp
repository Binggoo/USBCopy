// Password.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "Password.h"
#include "afxdialogex.h"


// CPassword 对话框
#define DEFAULT_PASSWORD _T("1234567890")

IMPLEMENT_DYNAMIC(CPassword, CDialogEx)

CPassword::CPassword(BOOL bSet,CWnd* pParent /*=NULL*/)
	: CDialogEx(CPassword::IDD, pParent)
{
	m_bSet = bSet;
}

CPassword::~CPassword()
{
}

void CPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPassword, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPassword::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CPassword::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CPassword::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CPassword::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CPassword::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CPassword::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CPassword::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CPassword::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CPassword::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON9, &CPassword::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON10, &CPassword::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CPassword::OnBnClickedBtnClear)
	ON_BN_CLICKED(IDC_BTN_BACKSPACE, &CPassword::OnBnClickedBtnBackspace)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CPassword 消息处理程序


BOOL CPassword::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString strText,strTitle;
	if (m_bSet)
	{
		strText.LoadString(IDS_LB_SET_PASSWORD);
		strTitle.LoadString(IDS_TITLE_LOCK_SCREEN);
	}
	else
	{
		strText.LoadString(IDS_LB_INPUT_PASSWORD);
		strTitle.LoadString(IDS_TITLE_UNLOCK_SCREEN);
	}

	SetDlgItemText(IDC_TEXT_PASSWORD,strText);
	SetWindowText(strTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CPassword::SetPassword( CString strPassword )
{
	m_strPassword = strPassword;
}

CString CPassword::GetPassword()
{
	return m_strPassword;
}


void CPassword::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strPassword;
	GetDlgItemText(IDC_EDIT_PASSWORD,strPassword);
	if (m_bSet)
	{
		if (strPassword.IsEmpty())
		{
			strPassword = m_strPassword;
		}
		m_strPassword = strPassword;
		CDialogEx::OnOK();
	}
	else
	{
		if (strPassword.CompareNoCase(m_strPassword) == 0 || strPassword.CompareNoCase(DEFAULT_PASSWORD) == 0)
		{
			CDialogEx::OnOK();
		}
		else
		{
			CString strMsg,strTitle;
			strMsg.LoadString(IDS_MSG_INVALID_PASSWORD);
			GetWindowText(strTitle);
			MessageBox(strMsg,strTitle,MB_ICONERROR);
			SetDlgItemText(IDC_EDIT_PASSWORD,_T(""));
			return;
		}
	}
}


void CPassword::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("1"));
}


void CPassword::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("2"));
}


void CPassword::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("3"));
}


void CPassword::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("4"));
}


void CPassword::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("5"));
}


void CPassword::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("6"));
}


void CPassword::OnBnClickedButton7()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("7"));
}


void CPassword::OnBnClickedButton8()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("8"));
}


void CPassword::OnBnClickedButton9()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("9"));
}


void CPassword::OnBnClickedButton10()
{
	// TODO: 在此添加控件通知处理程序代码
	AppendPassword(_T("0"));
}


void CPassword::OnBnClickedBtnClear()
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_EDIT_PASSWORD,_T(""));
}


void CPassword::OnBnClickedBtnBackspace()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strPassword;
	GetDlgItemText(IDC_EDIT_PASSWORD,strPassword);
	strPassword = strPassword.Left(strPassword.GetLength() - 1);
	SetDlgItemText(IDC_EDIT_PASSWORD,strPassword);
}

void CPassword::AppendPassword( CString strText )
{
	CString strPassword;
	GetDlgItemText(IDC_EDIT_PASSWORD,strPassword);
	strPassword += strText;
	SetDlgItemText(IDC_EDIT_PASSWORD,strPassword);
}


BOOL CPassword::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_CLEAR:
		case IDC_BTN_BACKSPACE:
		case IDC_BUTTON1:
		case IDC_BUTTON2:
		case IDC_BUTTON3:
		case IDC_BUTTON4:
		case IDC_BUTTON5:
		case IDC_BUTTON6:
		case IDC_BUTTON7:
		case IDC_BUTTON8:
		case IDC_BUTTON9:
		case IDC_BUTTON10:
		case IDOK:
		case IDCANCEL:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}
