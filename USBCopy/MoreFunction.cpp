// MoreFunction.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "MoreFunction.h"
#include "afxdialogex.h"


// CMoreFunction �Ի���

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


// CMoreFunction ��Ϣ�������


BOOL CMoreFunction::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
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
	// �쳣: OCX ����ҳӦ���� FALSE
}


BOOL CMoreFunction::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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

	// TODO: �ڴ˴������Ϣ����������

	if (nState == WA_INACTIVE)
	{
		CDialogEx::OnOK();
	}
}


void CMoreFunction::OnBnClickedBtnSoftwareRecovery()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_SOFTWARERECOVERY,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnViewLog()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_VIEWLOG,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnBurnIn()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_BURNIN,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}


void CMoreFunction::OnBnClickedBtnDebug()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_COMMAND,MAKEWPARAM(ID_MOREFUNCTION_DEBUG,CN_COMMAND),(LPARAM)m_hParent);

	CDialogEx::OnOK();
}
