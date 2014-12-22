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

	m_nMachineType = MT_TS;

	m_nKickOffCount = 0;

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

	m_font.CreatePointFont(85,_T("Arial"));

	GetDlgItem(IDC_TEXT_SN)->SetFont(&m_font);
	
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
		//rect.left=rect.left*cx/m_Rect.Width();
		rect.right=rect.right*cx/m_Rect.Width();
		rect.top=rect.top*cy/m_Rect.Height();
		rect.bottom=rect.bottom*cy/m_Rect.Height();
		pWnd->MoveWindow(rect);
	}
}

void CPortDlg::SetPort(CIni *pIni,HANDLE hLogFile,CPortCommand *pCommand, CPort *port,PortList *pPortList,int nMachineType)
{
	m_pCommand = pCommand;
	m_Port = port;
	m_PortList = pPortList;
	m_pIni = pIni;
	m_hLogFile = hLogFile;
	m_nMachineType = nMachineType;
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

	switch (m_nMachineType)
	{
	case MT_TS:
		nID = IDB_GRAY;
		break;

	case MT_USB:
		nID = IDB_USB_GRAY;
		break;

	case MT_NGFF:
		nID = IDB_NGFF_GRAY;
		break;

	}

	m_Bitmap.LoadBitmap(nID);
	m_PictureCtrol.SetBitmap(m_Bitmap);

	SetDlgItemText(IDC_TEXT_SPEED_R,_T(""));
	SetDlgItemText(IDC_TEXT_SPEED_W,_T(""));
	SetDlgItemText(IDC_TEXT_SIZE,_T(""));
	SetDlgItemText(IDC_TEXT_SN,_T(""));

	m_ProgressCtrl.SetPos(0);

	m_bOnlineCommand = FALSE;
	m_bStopCommand = FALSE;

	m_nKickOffCount = 0;

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

	UINT nKickOffTimes = m_pIni->GetUInt(_T("Option"),_T("KickOffTimeS"),5);

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

				m_nKickOffCount++;

				if (m_nKickOffCount >= nKickOffTimes)
				{
					m_Port->SetKickOff(TRUE);
					m_nKickOffCount = 0;
				}
				
			}
			else
			{
				m_nKickOffCount = 0;
			}
		}

		if (bCheckAbsoluteSpeed)
		{
			if (dbSpeed < nAbsolteSpeed)
			{
				m_nKickOffCount++;

				if (m_nKickOffCount >= nKickOffTimes)
				{
					m_Port->SetKickOff(TRUE);
					m_nKickOffCount = 0;
				}
				
			}
			else
			{
				m_nKickOffCount = 0;
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
	CString strSpeed,strSize,strSN,strTips,strUSBType,strReadSpeed,strWriteSpeed,strBadBlock,strRealSize;
	CULLArray ullBadBlockArray;
	int iPercent = 0;
	UINT nID  = IDB_GRAY;
	switch (m_Port->GetPortState())
	{
	case PortState_Offline:

		switch (m_nMachineType)
		{
		case MT_TS:
			nID = IDB_GRAY;
			break;

		case MT_USB:
			nID = IDB_USB_GRAY;
			break;

		case MT_NGFF:
			nID = IDB_NGFF_GRAY;
			break;

		}

		strSpeed = _T("");
		strSize = _T("");
		strSN = _T("");
		strTips = _T("Offline");
		strUSBType = _T("");
		iPercent = 0;

		if (m_bOnlineCommand)
		{
			m_pCommand->RedGreenLight(m_Port->GetPortNum(),FALSE);
			m_bOnlineCommand = FALSE;
		}
		SetDlgItemText(IDC_TEXT_SPEED_R,strSpeed);

		break;

	case PortState_Online:

		switch (m_nMachineType)
		{
		case MT_TS:
			nID = IDB_NORMAL;
			break;

		case MT_USB:
			nID = IDB_USB_NORMAL;
			break;

		case MT_NGFF:
			nID = IDB_NGFF_NORMAL;
			break;

		}

		strSpeed = _T("");
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		strTips.Format(_T("Online\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());
		iPercent = 0;
		strUSBType = m_Port->GetUsbType();

		if (!m_bOnlineCommand)
		{
			m_pCommand->RedGreenLight(m_Port->GetPortNum(),TRUE);
			m_bOnlineCommand = TRUE;
		}
		SetDlgItemText(IDC_TEXT_SPEED_R,strSpeed);

		break;

	case PortState_Active:
	case PortState_Stop:
		if (IsSlowest())
		{
			switch (m_nMachineType)
			{
			case MT_TS:
				nID = IDB_YELLOW;
				break;

			case MT_USB:
				nID = IDB_USB_YELLOW;
				break;

			case MT_NGFF:
				nID = IDB_NGFF_YELLOW;
				break;

			}

		}
		else
		{
			switch (m_nMachineType)
			{
			case MT_TS:
				nID = IDB_NORMAL;
				break;

			case MT_USB:
				nID = IDB_USB_NORMAL;
				break;

			case MT_NGFF:
				nID = IDB_NGFF_NORMAL;
				break;

			}
			
		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = m_Port->GetPercent();
		strUSBType = m_Port->GetUsbType();

		strTips.Format(_T("Running\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());
		if (m_Port->GetWorkMode() == WorkMode_Full_RW_Test 
			|| m_Port->GetWorkMode() == WorkMode_Fade_Picker 
			|| m_Port->GetWorkMode() == WorkMode_Speed_Check
			)
		{
			strReadSpeed.Format(_T("R:%s"),m_Port->GetRealSpeedString(TRUE));
			strWriteSpeed.Format(_T("W:%s"),m_Port->GetRealSpeedString(FALSE));

			SetDlgItemText(IDC_TEXT_SPEED_R,strReadSpeed);
			SetDlgItemText(IDC_TEXT_SPEED_W,strWriteSpeed);
		}
		else
		{
			SetDlgItemText(IDC_TEXT_SPEED_R,strSpeed);
		}

		break;

	case PortState_Pass:
		switch (m_nMachineType)
		{
		case MT_TS:
			nID = IDB_GREEN;
			break;

		case MT_USB:
			nID = IDB_USB_GREEN;
			break;

		case MT_NGFF:
			nID = IDB_NGFF_GREEN;
			break;

		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = 100;
		strUSBType = m_Port->GetUsbType();

		strTips.Format(_T("PASS\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());

		if (!m_bStopCommand)
		{	
			m_pCommand->GreenLight(m_Port->GetPortNum(),TRUE);

			// 如果是NGFF,红灯亮起的时候需要断电
			if (m_nMachineType == MT_NGFF)
			{
				Sleep(100);
				m_pCommand->Power(m_Port->GetPortNum(),FALSE);
			}

			m_bStopCommand = TRUE;
		}

		if (m_Port->GetWorkMode() == WorkMode_Full_RW_Test 
			|| m_Port->GetWorkMode() == WorkMode_Fade_Picker)
		{
			SetDlgItemText(IDC_TEXT_SPEED_R,strSize);
			SetDlgItemText(IDC_TEXT_SPEED_W,_T(""));
		}
		else if (m_Port->GetWorkMode() == WorkMode_Speed_Check)
		{
			strReadSpeed.Format(_T("R:%s"),m_Port->GetRealSpeedString(TRUE));
			strWriteSpeed.Format(_T("W:%s"),m_Port->GetRealSpeedString(FALSE));

			SetDlgItemText(IDC_TEXT_SPEED_R,strReadSpeed);
			SetDlgItemText(IDC_TEXT_SPEED_W,strWriteSpeed);
		}
		else
		{
			SetDlgItemText(IDC_TEXT_SPEED_R,strSpeed);
		}

		break;

	case PortState_Fail:

		switch (m_nMachineType)
		{
		case MT_TS:
			nID = IDB_RED;
			break;

		case MT_USB:
			nID = IDB_USB_RED;
			break;

		case MT_NGFF:
			nID = IDB_NGFF_RED;
			break;

		}

		strSpeed = m_Port->GetRealSpeedString();
		strSize = CUtils::AdjustFileSize(m_Port->GetTotalSize());
		strSN = m_Port->GetSN();
		iPercent = m_Port->GetPercent();
		strTips.Format(_T("FAIL\r\nModel:%s\r\nSN:%s\r\n"),m_Port->GetModuleName(),m_Port->GetSN());
		strUSBType = m_Port->GetUsbType();

		if (!m_bStopCommand)
		{
			m_pCommand->RedLight(m_Port->GetPortNum(),TRUE);

			// 如果是NGFF,红灯亮起的时候需要断电
			if (m_nMachineType == MT_NGFF)
			{
				Sleep(100);
				m_pCommand->Power(m_Port->GetPortNum(),FALSE);
			}

			m_bStopCommand = TRUE;
		}

		if (m_Port->GetWorkMode() == WorkMode_Full_RW_Test
			|| m_Port->GetWorkMode() == WorkMode_Fade_Picker )
		{
			int nBadCount = m_Port->GetBadBlockCount();

			if (nBadCount > 0)
			{
				m_Port->GetBadBlockArray(ullBadBlockArray);

				strBadBlock.Format(_T("B:%d"),nBadCount);
				strRealSize = CUtils::AdjustFileSize(ullBadBlockArray.GetAt(0) * m_Port->GetBytesPerSector());

				SetDlgItemText(IDC_TEXT_SPEED_R,strRealSize);
				SetDlgItemText(IDC_TEXT_SPEED_W,strBadBlock);
			}
			else
			{
				strReadSpeed.Format(_T("R:%s"),m_Port->GetRealSpeedString(TRUE));
				strWriteSpeed.Format(_T("W:%s"),m_Port->GetRealSpeedString(FALSE));

				SetDlgItemText(IDC_TEXT_SPEED_R,strReadSpeed);
				SetDlgItemText(IDC_TEXT_SPEED_W,strWriteSpeed);
			}
			
		}
		else if (m_Port->GetWorkMode() == WorkMode_Speed_Check)
		{
			strReadSpeed.Format(_T("R:%s"),m_Port->GetRealSpeedString(TRUE));
			strWriteSpeed.Format(_T("W:%s"),m_Port->GetRealSpeedString(FALSE));

			SetDlgItemText(IDC_TEXT_SPEED_R,strReadSpeed);
			SetDlgItemText(IDC_TEXT_SPEED_W,strWriteSpeed);

			iPercent = 100;
		}
		else
		{
			SetDlgItemText(IDC_TEXT_SPEED_R,strSpeed);
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
	SetDlgItemText(IDC_TEXT_SN,strSN);
	m_ProgressCtrl.SetPos(iPercent);
	

	if (!strUSBType.IsEmpty())
	{
		CString strGroupText;
		strGroupText.Format(_T("%s    %s"),m_Port->GetPortName(),strUSBType);

		SetDlgItemText(IDC_GROUP_PORT,strGroupText);
	}
	else
	{
		SetDlgItemText(IDC_GROUP_PORT,m_Port->GetPortName());
	}
	
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
