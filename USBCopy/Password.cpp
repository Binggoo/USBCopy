// Password.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "Password.h"
#include "afxdialogex.h"


// CPassword 对话框

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
			strPassword = DEFAULT_PASSWORD;
		}
		m_strPassword = strPassword;
		CDialogEx::OnOK();
	}
	else
	{
		if (strPassword == m_strPassword || strPassword == DEFAULT_PASSWORD)
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
