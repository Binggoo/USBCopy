// PortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "PortDlg.h"
#include "afxdialogex.h"
#include "Utils.h"

#define TIMER_UPDATE  1
// CPortDlg 对话框

IMPLEMENT_DYNAMIC(CPortDlg, CDialogEx)

CPortDlg::CPortDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPortDlg::IDD, pParent)
{
	m_Port = NULL;
}

CPortDlg::~CPortDlg()
{
}

void CPortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PIC_ICON, m_PictureCtrol);
	DDX_Control(pDX, IDC_PROGRESS1, m_ProgressCtrl);
}


BEGIN_MESSAGE_MAP(CPortDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CPortDlg 消息处理程序


BOOL CPortDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_Port != NULL && m_PortList != NULL);

	SetDlgItemText(IDC_GROUP_PORT,m_Port->GetPortName());
//	m_ProgressCtrl.ShowPercent(TRUE);
	m_ProgressCtrl.SetRange32(0,100);
	
	Initial();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CPortDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	CWnd *pWnd = GetWindow(GW_CHILD);
	while (pWnd)
	{
		ChangeSize(pWnd,cx,cy);
		pWnd = pWnd->GetWindow(GW_HWNDNEXT);
	}

	GetClientRect(&m_Rect);
}

void CPortDlg::ChangeSize( CWnd *pWnd,int cx, int cy )
{
	if (pWnd)
	{
		CRect rect;
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.left=rect.left*cx/m_Rect.Width();
		rect.right=rect.right*cx/m_Rect.Width();
		rect.top=rect.top*cy/m_Rect.Height();
		rect.bottom=rect.bottom*cy/m_Rect.Height();
		pWnd->MoveWindow(rect);
	}
}

void CPortDlg::SetPort(CPortCommand *pCommand, CPort *port,PortList *pPortList )
{
	m_pCommand = pCommand;
	m_Port = port;
	m_PortList = pPortList;
}

void CPortDlg::Update( BOOL bStart /*= TRUE*/ )
{
	if (bStart)
	{
		SetTimer(TIMER_UPDATE,1000,NULL);
	}
	else
	{
		KillTimer(TIMER_UPDATE);
	}
	
}


void CPortDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CString strSpeed,strSize;
	int iPercent = 0;
	switch (m_Port->GetPortState())
	{
	case PortState_Offline:
		if (m_Bitmap.m_hObject != NULL)
		{
			m_Bitmap.DeleteObject();
		}
		m_Bitmap.LoadBitmap(IDB_GRAY);

		strSpeed = _T("");
		strSize = _T("");
		iPercent = 0;

		break;

	case PortState_Online:
		if (m_Bitmap.m_hObject != NULL)
		{
			m_Bitmap.DeleteObject();
		}
		m_Bitmap.LoadBitmap(IDB_NORMAL);

		strSpeed = _T("");
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		iPercent = 0;

// 		if (m_Port->GetPortType() == PortType_MASTER_DISK)
// 		{
// 			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE,TRUE);
// 			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE,TRUE);
// 		}
// 		else
// 		{
// 			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE,FALSE);
// 			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE,FALSE);
// 		}

		break;

	case PortState_Active:
		if (m_Bitmap.m_hObject != NULL)
		{
			m_Bitmap.DeleteObject();
		}

		if (IsSlowest())
		{
			m_Bitmap.LoadBitmap(IDB_YELLOW);
		}
		else
		{
			m_Bitmap.LoadBitmap(IDB_NORMAL);
		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		iPercent = m_Port->GetPercent();

		break;

	case PortState_Pass:
		if (m_Bitmap.m_hObject != NULL)
		{
			m_Bitmap.DeleteObject();
		}
		m_Bitmap.LoadBitmap(IDB_GREEN);

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		iPercent = 100;

		if (m_Port->GetPortType() == PortType_MASTER_DISK)
		{
			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE,TRUE);
		}
		else
		{
			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE,FALSE);
		}
		

		break;

	case PortState_Fail:
		if (m_Bitmap.m_hObject != NULL)
		{
			m_Bitmap.DeleteObject();
		}
		m_Bitmap.LoadBitmap(IDB_RED);

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		iPercent = m_Port->GetPercent();

		if (m_Port->GetPortType() == PortType_MASTER_DISK)
		{
			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE,TRUE);
		}
		else
		{
			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE,FALSE);
		}

		break;
	}

	m_PictureCtrol.SetBitmap(m_Bitmap);
	SetDlgItemText(IDC_TEXT_SIZE,strSize);
	SetDlgItemText(IDC_TEXT_SPEED,strSpeed);
	m_ProgressCtrl.SetPos(iPercent);

	CDialogEx::OnTimer(nIDEvent);
}

void CPortDlg::Initial()
{
	if (m_Bitmap.m_hObject != NULL)
	{
		m_Bitmap.DeleteObject();
	}

	m_Bitmap.LoadBitmap(IDB_GRAY);
	m_PictureCtrol.SetBitmap(m_Bitmap);

	SetDlgItemText(IDC_TEXT_SPEED,_T(""));
	SetDlgItemText(IDC_TEXT_SIZE,_T(""));

	m_ProgressCtrl.SetPos(0);

	Invalidate(TRUE);
}

void CPortDlg::SetBitmap( UINT nResource )
{
	if (m_Bitmap.m_hObject != NULL)
	{
		m_Bitmap.DeleteObject();
	}

	m_Bitmap.LoadBitmap(nResource);
	m_PictureCtrol.SetBitmap(m_Bitmap);

	Invalidate(TRUE);
}

BOOL CPortDlg::IsSlowest()
{
	BOOL bSlow = TRUE;
	double dbSpeed = m_Port->GetRealSpeed();
	POSITION pos = m_PortList->GetHeadPosition();
	int nCount = 0;
	while(pos)
	{
		CPort *port = m_PortList->GetNext(pos);
		if (port->IsConnected() && port->GetPortState() == PortState_Active && port->GetPortNum() != m_Port->GetPortNum())
		{
			nCount++;
			if (port->GetRealSpeed() < dbSpeed)
			{
				bSlow = FALSE;
				break;
			}
		}
	}

	// 只有一个没有可比性
	if (nCount == 0)
	{
		bSlow = FALSE;
	}

	return bSlow;
}

CPort * CPortDlg::GetPort()
{
	return m_Port;
}
