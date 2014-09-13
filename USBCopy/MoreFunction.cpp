// MoreFunction.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "MoreFunction.h"
#include "afxdialogex.h"


// CMoreFunction 对话框

IMPLEMENT_DYNAMIC(CMoreFunction, CDialogEx)

CMoreFunction::CMoreFunction(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMoreFunction::IDD, pParent)
{
	if (pParent != NULL)
	{
		m_hParent = pParent->GetSafeHwnd();
	}
}

CMoreFunction::~CMoreFunction()
{
}

void CMoreFunction::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMoreFunction, CDialogEx)
	ON_WM_KILLFOCUS()
	ON_WM_SHOWWINDOW()
	ON_WM_SETCURSOR()
	ON_WM_ACTIVATE()
	ON_BN_CLICKED(IDC_BTN_SOFTWARE_RECOVERY, &CMoreFunction::OnBnClickedBtnSoftwareRecovery)
	ON_BN_CLICKED(IDC_BTN_VIEW_LOG, &CMoreFunction::OnBnClickedBtnViewLog)
	ON_BN_CLICKED(IDC_BTN_BURN_IN, &CMoreFunction::OnBnClickedBtnBurnIn)
	ON_BN_CLICKED(IDC_BTN_DEBUG, &CMoreFunction::OnBnClickedBtnDebug)
END_MESSAGE_MAP()


// CMoreFunction 消息处理程序


BOOL CMoreFunction::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_BtnSoftwareRecovery.SubclassDlgItem(IDC_BTN_SOFTWARE_RECOVERY,this);
	m_BtnSoftwareRecovery.SetFlat(FALSE);
	m_BtnSoftwareRecovery.SetBitmaps(IDB_RESTORE,RGB(255,255,255));

	m_BtnViewLog.SubclassDlgItem(IDC_BTN_VIEW_LOG,this);
	m_BtnViewLog.SetFlat(FALSE);
	m_BtnViewLog.SetBitmaps(IDB_VIEW_LOG,RGB(255,255,255));

	m_BtnBurnIn.SubclassDlgItem(IDC_BTN_BURN_IN,this);
	m_BtnBurnIn.SetFlat(FALSE);
	m_BtnBurnIn.SetBitmaps(IDB_BURN_IN,RGB(255,255,255));

	m_BtnDebug.SubclassDlgItem(IDC_BTN_DEBUG,this);
	m_BtnDebug.SetFlat(FALSE);
	m_BtnDebug.SetBitmaps(IDB_DEBUG,RGB(255,255,255));

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


BOOL CMoreFunction::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_SOFTWARE_RECOVERY:
		case IDC_BTN_VIEW_LOG:
		case IDC_BTN_BURN_IN:
		case IDC_BTN_DEBUG:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CMoreFunction::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 在此处添加消息处理程序代码

	if (nState == WA_INACTIVE)
	{
		CDialogEx::OnOK();
	}
}


void CMoreFunction::OnBnClickedBtnSoftwareRecovery()
{
	// TODO: 在此添加控件通知处理程序代码
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_SOFTWARERECOVERY,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnViewLog()
{
	// TODO: 在此添加控件通知处理程序代码
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_VIEWLOG,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnBurnIn()
{
	// TODO: 在此添加控件通知处理程序代码
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_BURNIN,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnDebug()
{
	// TODO: 在此添加控件通知处理程序代码
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_DEBUG,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}
