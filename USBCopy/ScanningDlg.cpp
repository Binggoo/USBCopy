// ScanningDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ScanningDlg.h"
#include "afxdialogex.h"
#include "EnumUSB.h"
#include "EnumStorage.h"
#include "Utils.h"
#include "Disk.h"
#include "WPDFunction.h"

// CScanningDlg 对话框

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

	m_bStop = FALSE;
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


// CScanningDlg 消息处理程序


BOOL CScanningDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pTargetPortList && m_pIni);
	ASSERT(m_hLogFile != INVALID_HANDLE_VALUE);

	SetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE,GetWindowLong(this->GetSafeHwnd(),GWL_EXSTYLE)|WS_EX_LAYERED);

	SetLayeredWindowAttributes(RGB(255,255,255),200,LWA_ALPHA);

	m_font.CreatePointFont(200,_T("Arial"));

	GetDlgItem(IDC_TEXT_SCAN)->SetFont(&m_font);

	CString strText;
	if (m_bBeginning)
	{
		m_nWaitTimeS = m_pIni->GetUInt(_T("Option"),_T("ScanDiskTimeS"),30);
		strText.Format(_T("Scanning device ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
		SetTimer(1,3000,NULL);
		m_dwOldTime = m_dwCurrentTime = GetTickCount();

		AfxBeginThread((AFX_THREADPROC)EnumDeviceThreadProc,this);
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
	// 异常: OCX 属性页应返回 FALSE
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
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CString strText;
	m_dwCurrentTime = GetTickCount();
	if (m_bBeginning)
	{
		m_nWaitTimeS -= (m_dwCurrentTime - m_dwOldTime)/1000;

		m_dwOldTime = m_dwCurrentTime;

		if (m_nWaitTimeS < 0)
		{
			KillTimer(1);

			m_bStop = TRUE;

			return;
		}
		strText.Format(_T("Scanning device ...... %d s"),m_nWaitTimeS);
		SetDlgItemText(IDC_TEXT_SCAN,strText);
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

	// TODO:  在此更改 DC 的任何特性
	if (pWnd->GetDlgCtrlID() == IDC_TEXT_SCAN)
	{
		pDC->SetTextColor(RGB(0,0,255));
		pDC->SetBkMode(TRANSPARENT);
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
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

			// 不需要母盘
			if (m_WorkMode == WorkMode_DiskClean 
				|| m_WorkMode == WorkMode_ImageCopy 
				|| m_WorkMode == WorkMode_DiskFormat
				|| (m_WorkMode == WorkMode_DifferenceCopy 
				&& m_pIni->GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Package 
				&& m_pIni->GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0) == PathType_Remote) 
				|| m_WorkMode == WorkMode_Full_RW_Test 
				|| m_WorkMode == WorkMode_Fade_Picker 
				|| m_WorkMode == WorkMode_Speed_Check 
				)
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
							case WorkMode_DiskFormat:
								// 去掉子盘readonly属性并且offline
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,FALSE,TRUE,&dwErrorCode);
								}
								break;

							case WorkMode_DiskCompare:
								// 只读
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,TRUE,FALSE,&dwErrorCode);
								}
								break;

							case WorkMode_FileCopy:
							case WorkMode_DifferenceCopy:
								// 去掉只读，并且online
								if (port->GetPortNum() != 0)
								{
									CDisk::SetDiskAtrribute(hDevice,FALSE,FALSE,&dwErrorCode);
								}
								break;
							}

							ULONGLONG ullSectorNums = 0;
							DWORD dwBytesPerSector = 0;
							MEDIA_TYPE type;
							ullSectorNums = CDisk::GetNumberOfSectors(hDevice,&dwBytesPerSector,&type);

							if (ullSectorNums > 0)
							{
								// 一个磁盘中有几个volume
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
											CString strVolume(pVolumeInfo->pszVolumePath);
											strVolumeArray.Add(strVolume);
										}

										pVolumeNode = pVolumeNode->pNext;
									}

									CleanupVolumeDeviceList(pVolumeList);
								}

								if (m_nMachineType == MT_TS)
								{
									TCHAR szModelName[256] = {NULL};
									TCHAR szSerialNumber[256] = {NULL};

									CDisk::GetTSModelNameAndSerialNumber(hDevice,szModelName,szSerialNumber,&dwErrorCode);

									strModel = CString(szModelName);
									strSN = CString(szSerialNumber);

									strModel.Trim();
									strSN.Trim();
								}
								else
								{
									PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
									DeviceDescriptor=(PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1];
									DeviceDescriptor->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 512 - 1;

									BOOL bOK = GetDriveProperty(hDevice,DeviceDescriptor);

									dwErrorCode = GetLastError();

									if (bOK)
									{
										LPCSTR vendorId = "";
										LPCSTR productId = "";

//										LPCSTR serialNumber = "";

										if ((DeviceDescriptor->ProductIdOffset != 0) &&
											(DeviceDescriptor->ProductIdOffset != -1)) 
										{
											productId        = (LPCSTR)(DeviceDescriptor);
											productId       += (ULONG_PTR)DeviceDescriptor->ProductIdOffset;
										}

										if ((DeviceDescriptor->VendorIdOffset != 0) &&
											(DeviceDescriptor->VendorIdOffset != -1)) 
										{
											vendorId         = (LPCSTR)(DeviceDescriptor);
											vendorId        += (ULONG_PTR)DeviceDescriptor->VendorIdOffset;
										}

// 										if ((DeviceDescriptor->SerialNumberOffset != 0) &&
// 											(DeviceDescriptor->SerialNumberOffset != -1)) 
// 										{
// 											serialNumber     = (LPCSTR)(DeviceDescriptor);
// 											serialNumber    += (ULONG_PTR)DeviceDescriptor->SerialNumberOffset;
// 										}

//										strSN = CString(serialNumber);
										strModel = CString(vendorId) + CString(productId);

										if (pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.iSerialNumber)
										{
											PSTRING_DESCRIPTOR_NODE StringDescs = pUsbDeviceInfo->StringDescs;
											UCHAR Index = pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.iSerialNumber;
											// Use an actual "int" here because it's passed as a printf * precision
											int descChars;  

											while (StringDescs)
											{
												if (StringDescs->DescriptorIndex == Index)
												{

													//
													// bString from USB_STRING_DESCRIPTOR isn't NULL-terminated, so 
													// calculate the number of characters.  
													// 
													// bLength is the length of the whole structure, not just the string.  
													// 
													// bLength is bytes, bString is WCHARs
													// 
													descChars = 
														( (int) StringDescs->StringDescriptor->bLength - 
														offsetof(USB_STRING_DESCRIPTOR, bString) ) /
														sizeof(WCHAR);
													//
													// Use the * precision and pass the number of characters just caculated.
													// bString is always WCHAR so specify widestring regardless of what TCHAR resolves to
													// 
													strSN.Format(_T("%.*ws"),
														descChars,
														StringDescs->StringDescriptor->bString);
												}

												StringDescs = StringDescs->Next;
											}
										}

										strModel.Trim();
										strSN.Trim();

										// 判断是移动硬盘还是U盘，如果是移动硬盘可以获取SN和Model
										if (type == FixedMedia && DeviceDescriptor->BusType == BusTypeUsb)
										{
											TCHAR szModelName[256] = {NULL};
											TCHAR szSerialNumber[256] = {NULL};

											if (CDisk::GetUsbHDDModelNameAndSerialNumber(hDevice,szModelName,szSerialNumber,&dwErrorCode))
											{
												CString strModelTemp = CString(szModelName);
												CString strSNTemp = CString(szSerialNumber);

												strModelTemp.Trim();
												strSNTemp.Trim();

												if (!strModelTemp.IsEmpty() && !strSNTemp.IsEmpty())
												{
													strModel = strModelTemp;
													strSN = strSNTemp;
												}
											}

										}

									}

									delete []DeviceDescriptor;

								}

								port->SetConnected(TRUE);
								port->SetDiskNum(pStorageDevInfo->nDiskNum);
								port->SetPortState(PortState_Online);
								port->SetBytesPerSector(dwBytesPerSector);
								port->SetTotalSize(ullSectorNums * dwBytesPerSector);
								port->SetVolumeArray(strVolumeArray);
								port->SetSN(strSN);
								port->SetModuleName(strModel);

								if ((pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB & 0x0300) == 0x0300)
								{
									port->SetUsbType(_T("3.0"));
								}
								else
								{
									port->SetUsbType(_T("2.0"));
								}

								CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Disk %d,Path=%s:%d,bcdUSB=%04X,Capacity=%I64d,ModelName=%s,SN=%s")
									,port->GetPortName(),pStorageDevInfo->nDiskNum,strPath,nConnectIndex,pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB
									,ullSectorNums * dwBytesPerSector,strModel,strSN);

								CloseHandle(hDevice);
								break;
							}
							else
							{
								CloseHandle(hDevice);
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
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
	case WorkMode_QuickCopy:
	case WorkMode_FileCopy:
	case WorkMode_MTPCopy:
		// 必须要有母盘
		if (!m_pMasterPort->IsConnected())
		{
			return FALSE;
		}
		break;

	case WorkMode_ImageMake:
		// 只要求有母盘
		return m_pMasterPort->IsConnected();

	case WorkMode_DifferenceCopy:
		if (m_pIni->GetUInt(_T("DifferenceCopy"),_T("SourceType"),0) == SourceType_Master
			|| m_pIni->GetUInt(_T("DifferenceCopy"),_T("PkgLocation"),0) == PathType_Local)
		{
			// 需要母盘
			if (!m_pMasterPort->IsConnected())
			{
				return FALSE;
			}
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

void CScanningDlg::SetConfig( CIni *pIni,WorkMode workMode ,int nMachineType)
{
	m_pIni = pIni;
	m_WorkMode = workMode;
	m_nMachineType = nMachineType;
}

void CScanningDlg::SetLogFile( HANDLE hFile )
{
	m_hLogFile = hFile;
}


BOOL CScanningDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CScanningDlg::ScanningMTPDevice()
{
	POSITION pos = m_pTargetPortList->GetHeadPosition();
	CString strPath;
	int nConnectIndex = -1;
	int nCount = 0;
	CString strModel,strSN;
	while (pos)
	{
		PUSBDEVICEINFO pUsbDeviceInfo = NULL;
		PWPDDEVIEINFO pWPDDevInfo = NULL;
		CPort *port;

		if (nCount == 0)
		{
			port = m_pMasterPort;

			// 不需要母盘
			if (m_WorkMode == WorkMode_ImageCopy)
			{
				port->Initial();
				nCount++;
				continue;
			}
		}
		else
		{
			port = m_pTargetPortList->GetNext(pos);

			//不需要子盘
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
			pWPDDevInfo = MatchWPDDeivceID(pUsbDeviceInfo->DeviceID);

			if (pWPDDevInfo)
			{
				DWORD dwErrorCode = 0;
				CComPtr<IPortableDevice> pIPortableDevice;
				HRESULT hr = OpenDevice(&pIPortableDevice,pWPDDevInfo->pszDevicePath);

				if (SUCCEEDED(hr) && pIPortableDevice != NULL)
				{
					GetDeviceModelAndSerialNumber(pIPortableDevice,strModel.GetBuffer(MAX_PATH),strSN.GetBuffer(MAX_PATH));
					strModel.ReleaseBuffer();
					strSN.ReleaseBuffer();

					LPWSTR ppwszObjectIDs[NUM_OBJECTS_TO_REQUEST] = {NULL};
					DWORD dwObjects = EnumStorageIDs(pIPortableDevice,ppwszObjectIDs);

					ULONGLONG ullTotalSize = 0,ullFreeSize = 0;
					if (dwObjects > 0)
					{
						GetStorageFreeAndTotalCapacity(pIPortableDevice,ppwszObjectIDs[0],&ullFreeSize,&ullTotalSize);

						for (DWORD index = 0; index < dwObjects;index++)
						{
							//delete []ppwszObjectIDs[index];
							CoTaskMemFree(ppwszObjectIDs[index]);
							ppwszObjectIDs[index] = NULL;
						}
					}


					port->SetConnected(TRUE);
					port->SetDevicePath(CString(pWPDDevInfo->pszDevicePath));
					port->SetPortState(PortState_Online);
					port->SetTotalSize(ullTotalSize);
					port->SetCompleteSize(ullTotalSize - ullFreeSize);
					port->SetSN(strSN);
					port->SetModuleName(strModel);

					if ((pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB & 0x0300) == 0x0300)
					{
						port->SetUsbType(_T("3.0"));
					}
					else
					{
						port->SetUsbType(_T("2.0"));
					}

					CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Port %s,Path=%s:%d,bcdUSB=%04X,Capacity=%I64d,FreeSize=%I64d,ModelName=%s,SN=%s")
						,port->GetPortName(),strPath,nConnectIndex,pUsbDeviceInfo->ConnectionInfo->DeviceDescriptor.bcdUSB
						,ullTotalSize,ullFreeSize,strModel,strSN);
				}
				else
				{
					port->Initial();
				}

				CleanWPDDeviceInfo(pWPDDevInfo);
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

void CScanningDlg::EnumDevice()
{
	while (!m_bStop)
	{
		CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Scanning device......"));

		if (m_WorkMode == WorkMode_MTPCopy)
		{
			EnumWPD(&m_bStop);
			ScanningMTPDevice();
			CleanupWPD();
		}
		else
		{
			EnumStorage(&m_bStop);
			EnumVolume(&m_bStop);
			ScanningDevice();
			CleanupStorage();
			CleanupVolume();
		}

		if (IsAllConnected())
		{
			break;
		}
	}

	CUtils::WriteLogFile(m_hLogFile,TRUE,_T("Scanning device end."));
	KillTimer(1);
	
	PostMessage(WM_CLOSE);

	return;
}

DWORD WINAPI CScanningDlg::EnumDeviceThreadProc( LPVOID lpParm )
{
	CScanningDlg *pDlg = (CScanningDlg *)lpParm;

	pDlg->EnumDevice();

	return 1;
}
