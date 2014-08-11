// ScanningDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ScanningDlg.h"
#include "afxdialogex.h"
#include "EnumUSB.h"
#include "EnumStorage.h"
#include "Utils.h"
#include "Disk.h"

// CScanningDlg �Ի���

IMPLEMENT_DYNAMIC(CScanningDlg, CDialogEx)

CScanningDlg::CScanningDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CScanningDlg::IDD, pParent)
{
	m_pTargetPortList = NULL;
	m_nWaitTimeS = 0;
	m_bBeginning = TRUE;
	m_pIni = NULL;
	m_dwCurrentTime = 0;
	m_dwOldTime = 0;
	m_hLogFile = INVALID_HANDLE_VALUE;
}

CScanningDlg::~CScanningDlg()
{
}

void CScanningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CScanningDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CScanningDlg ��Ϣ�������


BOOL CScanningDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pTargetPortList && m_pIni);
	ASSERT(m_hLogFile != INVALID_HANDLE_VALUE);

	SetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE,GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)|WS_EX_LAYERED);

	SetLayeredWindowAttributes(RGB(255,255,255),200,LWA_ALPHA);

	m_font.CreatePointFont(200,_T("Arial"));

	GetDlgItem(IDC_TEXT_SCAN)->SetFont(&m_font);

	m_WorkMode = (WorkMode)m_pIni->GetUInt(_T("Option"),_T("FunctionMode"),1);

	if (m_WorkMode == WorkMode_DiskFormat)
	{
		::PostMessage(GetParent()->GetSafeHwnd(),WM_RESET_POWER,0,0);
	}

	CString strText;
	if (m_bBeginning)
	{
		m_nWaitTimeS = m_pIni->GetUInt(_T("Option"),_T("ScanDiskTimeS"),30);
		strText.Format(_T("Scanning disk ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
		SetTimer(1,3000,NULL);
		m_dwOldTime = m_dwCurrentTime = GetTickCount();
	}
	else
	{
		m_nWaitTimeS = m_pIni->GetUInt(_T("Option"),_T("SpinDownTimeS"),3);
		strText.Format(_T("Power off ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
		SetTimer(1,1000,NULL);
		m_dwOldTime = m_dwCurrentTime = GetTickCount();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CScanningDlg::SetDeviceInfoList(CPort *pMaster,PortList *pTargetPortList )
{
	m_pMasterPort = pMaster;
	m_pTargetPortList = pTargetPortList;
}

void CScanningDlg::SetBegining( BOOL bBeginning /*= TRUE*/ )
{
	m_bBeginning = bBeginning;
}


void CScanningDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CString strText;
	m_dwCurrentTime = GetTickCount();
	if (m_bBeginning)
	{
		m_nWaitTimeS -= (m_dwCurrentTime - m_dwOldTime)/1000;

		m_dwOldTime = m_dwCurrentTime;

		if (m_nWaitTimeS < 0)
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Scanning disk end."));
			KillTimer(1);
			EndDialog(0);
			return;
		}
		strText.Format(_T("Scanning disk ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Scanning disk......"));

		EnumStorage();
		EnumVolume();
		ScanningDevice();
		CleanupStorage();
		CleanupVolume();

		if (IsAllConnected())
		{
			CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Scanning disk end."));
			KillTimer(1);
			EndDialog(0);
			return;
		}
	}
	else
	{
		m_nWaitTimeS -= (m_dwCurrentTime - m_dwOldTime)/1000;

		m_dwOldTime = m_dwCurrentTime;

		if (m_nWaitTimeS < 0)
		{
			KillTimer(nIDEvent);
			EndDialog(0);
			return;
		}

		strText.Format(_T("Power off ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
	}

	CDialogEx::OnTimer(nIDEvent);
}


HBRUSH CScanningDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  �ڴ˸��� DC ���κ�����
	if (pWnd->GetDlgCtrlID() == IDC_TEXT_SCAN)
	{
		pDC->SetTextColor(RGB(0,0,255));
		pDC->SetBkMode(TRANSPARENT);
	}

	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	return hbr;
}

void CScanningDlg::ScanningDevice()
{
	POSITION pos = m_pTargetPortList->GetHeadPosition();
	CString strPath;
	int nConnectIndex = -1;
	int nCount = 0;
	CString strModel,strSN;
	while (pos)
	{
		PUSBDEVICEINFO pUsbDeviceInfo = NULL;
		PDEVICELIST pStorageList = NULL;
		PDEVICELIST pVolumeList = NULL;
		CPort *port;

		if (nCount == 0)
		{
			port = m_pMasterPort;

			if (m_WorkMode == WorkMode_DiskClean || m_WorkMode == WorkMode_ImageCopy || m_WorkMode == WorkMode_DiskFormat)
			{
				port->Initial();
				nCount++;
				continue;
			}
		}
		else
		{
			port = m_pTargetPortList->GetNext(pos);
			if (m_WorkMode == WorkMode_ImageMake)
			{
				port->Initial();
				nCount++;
				continue;
			}
		}

		nCount++;

		strPath = port->GetPath1();
		nConnectIndex = port->GetConnectIndex1();
		pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);

		if (pUsbDeviceInfo == NULL)
		{
			strPath = port->GetPath2();
			nConnectIndex = port->GetConnectIndex2();
			pUsbDeviceInfo = GetHubPortDeviceInfo(strPath.GetBuffer(),nConnectIndex);
		}

		if (pUsbDeviceInfo)
		{
			pStorageList = MatchStorageDeivceIDs(pUsbDeviceInfo->DeviceID);

			if (pStorageList)
			{
				PDEVICELIST pListNode = pStorageList->pNext;
				while (pListNode)
				{
					PSTORAGEDEVIEINFO pStorageDevInfo = (PSTORAGEDEVIEINFO)pListNode->pDevInfo;
					if (pStorageDevInfo)
					{
						DWORD dwErrorCode = 0;
						HANDLE hDevice = CDisk::GetHandleOnPhysicalDrive(pStorageDevInfo->nDiskNum,FILE_FLAG_OVERLAPPED,&dwErrorCode);

						if (hDevice != INVALID_HANDLE_VALUE)
						{

							switch (m_WorkMode)
							{
							case WorkMode_FullCopy:
							case WorkMode_QuickCopy:
							case WorkMode_ImageCopy:
							case WorkMode_DiskClean:
								// ȥ������readonly���Բ���offline
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,FALSE,TRUE,&dwErrorCode);
								}
								break;

							case WorkMode_DiskCompare:
								// ֻ��
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,TRUE,FALSE,&dwErrorCode);
								}
								break;

							case WorkMode_FileCopy:
							case WorkMode_DiskFormat:
								// ȥ��ֻ��������online
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,FALSE,FALSE,&dwErrorCode);
								}
							}

							ULONGLONG ullSectorNums = 0;
							DWORD dwBytesPerSector = 0;
							ullSectorNums = CDisk::GetNumberOfSectors(hDevice,&dwBytesPerSector);
							CloseHandle(hDevice);

							if (ullSectorNums > 0)
							{
								// һ���������м���volume
								CStringArray strVolumeArray;
								pVolumeList = MatchVolumeDeviceDiskNums(pStorageDevInfo->nDiskNum);

								if (pVolumeList)
								{
									PDEVICELIST pVolumeNode = pVolumeList->pNext;
									while (pVolumeNode)
									{
										PVOLUMEDEVICEINFO pVolumeInfo = (PVOLUMEDEVICEINFO)pVolumeNode->pDevInfo;
										if (pVolumeInfo)
										{
											strVolumeArray.Add(pVolumeInfo->pszVolumePath);
										}

										pVolumeNode = pVolumeNode->pNext;
									}

									CleanupVolumeDeviceList(pVolumeList);
								}

								port->SetConnected(TRUE);
								port->SetDiskNum(pStorageDevInfo->nDiskNum);
								port->SetPortState(PortState_Online);
								port->SetBytesPerSector(dwBytesPerSector);
								port->SetTotalSize(ullSectorNums * dwBytesPerSector);
								port->SetVolumeArray(strVolumeArray);

								CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Path=%s:%d,bcdUSB=%04X,Capacity=%I64d,ModelName=%s,SN=%s")
									,port->GetPortName(),pStorageDevInfo->nDiskNum,strPath,nConnectIndex,pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB
									,ullSectorNums * dwBytesPerSector,strModel,strSN);

								break;
							}
							else
							{
								port->Initial();
								
							}
							
						}
						else
						{
							port->Initial();
						}
					}
					else
					{
						port->Initial();
					}

					pListNode = pListNode->pNext;
				}				
				CleanupStorageDeviceList(pStorageList);

			}
			else
			{
				port->Initial();
			}

			CleanupInfo(pUsbDeviceInfo);
			pUsbDeviceInfo = NULL;
		}
		else
		{
			port->Initial();
		}
	}
}

BOOL CScanningDlg::IsAllConnected()
{
	WorkMode workMode = (WorkMode)m_pIni->GetUInt(_T("Option"),_T("FunctionMode"),1);
	switch (workMode)
	{
	case WorkMode_ImageCopy:
	case WorkMode_DiskClean:
	case WorkMode_DiskFormat:
		break;

	case WorkMode_ImageMake:
		return m_pMasterPort->IsConnected();

	default:
		if (!m_pMasterPort->IsConnected())
		{
			return FALSE;
		}
		break;

	}

	POSITION pos = m_pTargetPortList->GetHeadPosition();
	BOOL bConnect = TRUE;
	CPort *pPort;
	while (pos)
	{
		pPort = m_pTargetPortList->GetNext(pos);
		if (!pPort->IsConnected())
		{
			bConnect = FALSE;
			break;
		}
	}
	return bConnect;
}

void CScanningDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}

void CScanningDlg::SetLogFile( HANDLE hFile )
{
	m_hLogFile = hFile;
}


BOOL CScanningDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: �ڴ����ר�ô����/����û���
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
