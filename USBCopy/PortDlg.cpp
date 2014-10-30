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

	m_bOnlineCommand = FALSE;
	m_bStopCommand = FALSE;

	m_bEnableKickOff = FALSE;

	m_bIsUSB = FALSE;

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

	m_Tooltips.Create(this,TTS_ALWAYSTIP);
	m_Tooltips.SetMaxTipWidth(200);
	m_Tooltips.AddTool(&m_PictureCtrol,_T("Offline"));
	
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

void CPortDlg::SetPort(CIni *pIni,HANDLE hLogFile,CPortCommand *pCommand, CPort *port,PortList *pPortList,BOOL bIsUSB)
{
	m_pCommand = pCommand;
	m_Port = port;
	m_PortList = pPortList;
	m_pIni = pIni;
	m_hLogFile = hLogFile;
	m_bIsUSB = bIsUSB;
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
		UpdateState();
	}
	
}


void CPortDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	UpdateState();

	CDialogEx::OnTimer(nIDEvent);
}

void CPortDlg::Initial()
{
	if (m_Bitmap.m_hObject != NULL)
	{
		m_Bitmap.DeleteObject();
	}

	UINT nID = IDB_GRAY;

	if (m_bIsUSB)
	{
		nID = IDB_USB_GRAY;
	}

	m_Bitmap.LoadBitmap(nID);
	m_PictureCtrol.SetBitmap(m_Bitmap);

	SetDlgItemText(IDC_TEXT_SPEED,_T(""));
	SetDlgItemText(IDC_TEXT_SIZE,_T(""));
	SetDlgItemText(IDC_TEXT_SN,_T(""));

	m_ProgressCtrl.SetPos(0);

	m_bOnlineCommand = FALSE;
	m_bStopCommand = FALSE;
	m_nCount = 0;

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
	// 排除母盘比较
	if (m_Port->GetPortNum() == 0 && m_Port->GetPortType() == PortType_MASTER_DISK)
	{
		return FALSE;
	}

	BOOL bCheckRelativeSpeed = m_pIni->GetBool(_T("Option"),_T("En_RelativeSpeed"),FALSE);
	UINT nRelativeSpeed = m_pIni->GetUInt(_T("Option"),_T("RelativeSpeed"),60);

	BOOL bCheckAbsoluteSpeed = m_pIni->GetBool(_T("Option"),_T("En_AbsoluteSpeed"),FALSE);
	UINT nAbsolteSpeed = m_pIni->GetUInt(_T("Option"),_T("AbsoluteSpeed"),5);

	BOOL bSlow = TRUE;
	double dbSpeed = m_Port->GetRealSpeed();
	POSITION pos = m_PortList->GetHeadPosition();
	double dbTotolSpeed = dbSpeed;
	int nCount = 0;
	while(pos)
	{
		CPort *port = m_PortList->GetNext(pos);
		if (port->IsConnected() && port->GetPortState() == PortState_Active && port->GetPortNum() != m_Port->GetPortNum())
		{
			nCount++;
			dbTotolSpeed += port->GetRealSpeed();
			if (port->GetRealSpeed() < dbSpeed)
			{
				bSlow = FALSE;
			}
		}
	}

	// 只有一个没有可比性
	if (nCount == 0)
	{
		bSlow = FALSE;
	}

	if (bSlow && m_bEnableKickOff)
	{
		if (bCheckRelativeSpeed)
		{
			double dbAvgSpeed = dbTotolSpeed / (nCount + 1);
			if (dbSpeed < dbAvgSpeed * nRelativeSpeed / 100)
			{
				m_nCount++;

				if (m_nCount >= 10)
				{
					m_Port->SetKickOff(TRUE);
				}
				
			}
			else
			{
				m_nCount = 0;
			}
		}

		if (bCheckAbsoluteSpeed)
		{
			if (dbSpeed < nAbsolteSpeed)
			{
				m_nCount++;

				if (m_nCount >= 10)
				{
					m_Port->SetKickOff(TRUE);
				}
				
			}
			else
			{
				m_nCount = 0;
			}
		}
	}

	return bSlow;
}

CPort * CPortDlg::GetPort()
{
	return m_Port;
}

void CPortDlg::UpdateState()
{
	CString strSpeed,strSize,strSN,strTips;
	int iPercent = 0;
	UINT nID  = IDB_GRAY;
	switch (m_Port->GetPortState())
	{
	case PortState_Offline:
		nID = IDB_GRAY;

		if (m_bIsUSB)
		{
			nID = IDB_USB_GRAY;
		}
		strSpeed = _T("");
		strSize = _T("");
		strSN = _T("");
		strTips = _T("Offline");
		iPercent = 0;

		if (m_bOnlineCommand)
		{
			m_pCommand->RedGreenLight(m_Port->GetPortNum(),FALSE);
			m_bOnlineCommand = FALSE;
		}

		break;

	case PortState_Online:
		nID = IDB_NORMAL;

		if (m_bIsUSB)
		{
			nID = IDB_USB_NORMAL;
		}

		strSpeed = _T("");
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		strTips.Format(_T("Online\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());
		iPercent = 0;

		if (!m_bOnlineCommand)
		{
			m_pCommand->RedGreenLight(m_Port->GetPortNum(),TRUE);
			m_bOnlineCommand = TRUE;
		}

		break;

	case PortState_Active:
	case PortState_Stop:
		if (IsSlowest())
		{
			nID = IDB_YELLOW;
			if (m_bIsUSB)
			{
				nID = IDB_USB_YELLOW;
			}

		}
		else
		{
			nID = IDB_NORMAL;
			if (m_bIsUSB)
			{
				nID = IDB_USB_NORMAL;
			}
			
		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = m_Port->GetPercent();

		strTips.Format(_T("Running\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());

		break;

	case PortState_Pass:
		nID = IDB_GREEN;

		if (m_bIsUSB)
		{
			nID = IDB_USB_GREEN;
		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = 100;

		strTips.Format(_T("PASS\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());

		if (!m_bStopCommand)
		{	
			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE);
			m_bStopCommand = TRUE;
		}

		break;

	case PortState_Fail:

		nID = IDB_RED;

		if (m_bIsUSB)
		{
			nID = IDB_USB_RED;
		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = m_Port->GetPercent();
		strTips.Format(_T("FAIL\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());

		if (!m_bStopCommand)
		{
			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE);
			m_bStopCommand = TRUE;
		}

		break;
	}

	if (m_Bitmap.m_hObject != NULL)
	{
		m_Bitmap.DeleteObject();
	}

	m_Bitmap.LoadBitmap(nID);
	m_PictureCtrol.SetBitmap(m_Bitmap);
	SetDlgItemText(IDC_TEXT_SIZE,strSize);
	SetDlgItemText(IDC_TEXT_SPEED,strSpeed);
	SetDlgItemText(IDC_TEXT_SN,strSN);
	m_ProgressCtrl.SetPos(iPercent);

	m_Tooltips.UpdateTipText(strTips,&m_PictureCtrol);
}

void CPortDlg::EnableKickOff(BOOL bEnable)
{
	m_bEnableKickOff = bEnable;
}

BOOL CPortDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类

	m_Tooltips.RelayEvent(pMsg);
	
	return CDialogEx::PreTranslateMessage(pMsg);
}
