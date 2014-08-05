#include "StdAfx.h"
#include "Port.h"


CPort::CPort(void)
{
	m_iPortNum = 0;
	m_strPath1 = _T("");
	m_strPath2 = _T("");
	m_iConnectIndex1 = 0;
	m_iConnectIndex2 = 0;
	m_PortType = PortType_NULL;

	memset(&m_Overlapped,0,sizeof(m_Overlapped));
	m_Overlapped.hEvent = CreateEvent(NULL,FALSE,TRUE,NULL);

	Initial();
}


CPort::~CPort(void)
{
	if (m_Overlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_Overlapped.hEvent);
	}
}

void CPort::Initial()
{
	// 设备
	m_bConnected = FALSE;
	m_iDiskNum = 0;
	m_strVolumeArray.RemoveAll();
	m_ullTotalSize = 0;
	m_dwBytesPerSector = 0;
	m_strSN = _T("");
	m_strModuleName = _T("");
	m_WorkMode = WorkMode_None;

	// 映像
	m_strFileName = _T("");

	// 起止时间
	m_StartTime = m_EndTime = CTime::GetCurrentTime();

	// 计算
	m_ullValidSize = 0;
	m_ullCompleteSize = 0;
	m_dbUsedWaitTimeS = 0.0;
	m_dbUsedNoWaitTimeS = 0.0;
	memset(m_Hash,0,LEN_DIGEST);

	// 结果
	m_PortState = PortState_Offline;
	m_bResult = FALSE;
	m_dwErrorCode = 0;
	m_ErrorType = ErrorType_System;
	
}

void CPort::Reset()
{
	// 起止时间
	m_StartTime = m_EndTime = CTime::GetCurrentTime();

	// 计算
	m_ullValidSize = 0;
	m_ullCompleteSize = 0;
	m_dbUsedWaitTimeS = 0.0;
	m_dbUsedNoWaitTimeS = 0.0;
	memset(m_Hash,0,LEN_DIGEST);

	// 结果
	m_PortState = PortState_Offline;
	m_bResult = TRUE;
	m_dwErrorCode = 0;
	m_ErrorType = ErrorType_System;
}

void CPort::SetPortNum( int iPortNum )
{
	m_iPortNum = iPortNum;
}

int CPort::GetPortNum()
{
	return m_iPortNum;
}

CString CPort::GetPortName()
{
	CString strPortName;
	if (m_PortType == PortType_MASTER_DISK)
	{
		strPortName.Format(_T("M-1"));
	}
	else if (m_PortType == PortType_TARGET_DISK)
	{
		strPortName.Format(_T("T-%d"),m_iPortNum);
	}

	return strPortName;
}

void CPort::SetPortType( PortType type )
{
	m_PortType = type;
}

PortType CPort::GetPortType()
{
	return m_PortType;
}

CString CPort::GetPortTypeName()
{
	CString strPortTypeName;
	switch(m_PortType)
	{
	case PortType_MASTER_DISK:
		strPortTypeName = _T("Master Disk");
		break;

	case PortType_TARGET_DISK:
		strPortTypeName = _T("Target Disk");
		break;

	case PortType_MASTER_FILE:
		strPortTypeName = _T("Master Image");
		break;

	case PortType_TARGET_FILE:
		strPortTypeName = _T("Target Image");
		break;
	}

	return strPortTypeName;
}

CString CPort::GetPath1()
{
	return m_strPath1;
}

int CPort::GetConnectIndex1()
{
	return m_iConnectIndex1;
}

void CPort::SetPath1AndIndex( CString strPath )
{
	int pos = strPath.ReverseFind(_T(':'));
	m_strPath1 = strPath.Left(pos);

	m_iConnectIndex1= _ttoi(strPath.Right(strPath.GetLength() - pos -1));
}

CString CPort::GetPath2()
{
	return m_strPath2;
}

int CPort::GetConnectIndex2()
{
	return m_iConnectIndex2;
}

void CPort::SetPath2AndIndex( CString strPath )
{
	int pos = strPath.ReverseFind(_T(':'));
	m_strPath2 = strPath.Left(pos);

	m_iConnectIndex2= _ttoi(strPath.Right(strPath.GetLength() - pos -1));
}

void CPort::SetConnected( BOOL bConnected )
{
	m_bConnected = bConnected;
}

BOOL CPort::IsConnected()
{
	return m_bConnected;
}

int CPort::GetDiskNum()
{
	return m_iDiskNum;
}

void CPort::SetDiskNum( int iNum )
{
	m_iDiskNum = iNum;
}

void CPort::SetVolumeArray( const CStringArray& strVolumeArray )
{
	m_strVolumeArray.Copy(strVolumeArray);
}

void CPort::GetVolumeArray( CStringArray& strVolumeArray )
{
	strVolumeArray.Copy(m_strVolumeArray);
}

ULONGLONG CPort::GetTotalSize()
{
	return m_ullTotalSize;
}

void CPort::SetTotalSize( ULONGLONG ullSize )
{
	m_ullTotalSize = ullSize;
}

DWORD CPort::GetBytesPerSector()
{
	return m_dwBytesPerSector;
}

void CPort::SetBytesPerSector( DWORD dwValue )
{
	m_dwBytesPerSector = dwValue;
}

CString CPort::GetSN()
{
	return m_strSN;
}

void CPort::SetSN( CString strSN )
{
	m_strSN = strSN;
}

CString CPort::GetModuleName()
{
	return m_strModuleName;
}

void CPort::SetModuleName( CString strName )
{
	m_strModuleName = strName;
}

void CPort::SetWorkMode( WorkMode workMode )
{
	m_WorkMode = workMode;
}

WorkMode CPort::GetWorkMode()
{
	return m_WorkMode;
}

void CPort::SetHashMethod(HashMethod method)
{
	m_HashMethod = method;
}

HashMethod CPort::GetHashMethod()
{
	return m_HashMethod;
}

CString CPort::GetHashMethodString()
{
	CString strHashMethod;
	switch (m_HashMethod)
	{
	case HashMethod_CHECKSUM32:
		strHashMethod = _T("CHECKSUM32");
		break;

	case HashMethod_CRC32:
		strHashMethod = _T("CRC32");
		break;

	case HashMethod_MD5:
		strHashMethod = _T("MD5");
		break;
	}
	return strHashMethod;
}

int CPort::GetHashLength()
{
	int len = 0;
	switch (m_HashMethod)
	{
	case HashMethod_MD5:
		len = 16;
		break;

	case HashMethod_CRC32:
	case HashMethod_CHECKSUM32:
		len = 4;
		break;
	}

	return len;
}

CString CPort::GetFileName()
{
	return m_strFileName;
}

void CPort::SetFileName( CString strFile )
{
	m_strFileName = strFile;
}

void CPort::SetStartTime( CTime time )
{
	m_StartTime = time;
}

CTime CPort::GetStartTime()
{
	return m_StartTime;
}

void CPort::SetEndTime( CTime time )
{
	m_EndTime = time;
}

CTime CPort::GetEndTime()
{
	return m_EndTime;
}

ULONGLONG CPort::GetValidSize()
{
	return m_ullValidSize;
}

void CPort::SetValidSize( ULONGLONG ullSize )
{
	m_ullValidSize = ullSize;
}

void CPort::SetCompleteSize( ULONGLONG ullSize )
{
	m_ullCompleteSize = ullSize;
}

void CPort::AppendCompleteSize( ULONGLONG ullSize )
{
	m_ullCompleteSize += ullSize;
}

ULONGLONG CPort::GetCompleteSize()
{
	return m_ullCompleteSize;
}

void CPort::SetUsedWaitTimeS( double dbTimeS )
{
	m_dbUsedWaitTimeS = dbTimeS;
}

void CPort::AppendUsedWaitTimeS( double dbTimeS )
{
	m_dbUsedWaitTimeS += dbTimeS;
}

double CPort::GetUsedWaitTimeS()
{
	return m_dbUsedWaitTimeS;
}

void CPort::SetUsedNoWaitTimeS( double dbTimeS )
{
	m_dbUsedNoWaitTimeS = dbTimeS;
}

void CPort::AppendUsedNoWaitTimeS( double dbTimeS )
{
	m_dbUsedNoWaitTimeS += dbTimeS;
}

double CPort::GetUsedNoWaitTimeS()
{
	return m_dbUsedNoWaitTimeS;
}

double CPort::GetAvgSpeed()
{
	double dbSpeed = 0.0;
	if (m_dbUsedWaitTimeS > 0)
	{
		dbSpeed = (m_ullCompleteSize / 1024. / 1024.) / m_dbUsedWaitTimeS;
	}
	return dbSpeed;
}

CString CPort::GetAvgSpeedString()
{
	CString strSpeed;
	if (m_dbUsedWaitTimeS > 0)
	{
		int speed = 0;
		if (m_ullCompleteSize / 1024 / 1024 / 1024)
		{
			speed = (int)((m_ullCompleteSize / 1024 / 1024 / 1024) / m_dbUsedWaitTimeS);
			strSpeed.Format(_T("%dGB/s"),speed);
		}
		else if (m_ullCompleteSize / 1024 / 1024)
		{
			speed = (int)((m_ullCompleteSize / 1024 / 1024 ) / m_dbUsedWaitTimeS);
			strSpeed.Format(_T("%dMB/s"),speed);
		}
		else if (m_ullCompleteSize / 1024)
		{
			speed = (int)((m_ullCompleteSize / 1024) / m_dbUsedWaitTimeS);
			strSpeed.Format(_T("%dKB/s"),speed);
		}
		else
		{
			speed = (int)(m_ullCompleteSize / m_dbUsedWaitTimeS);
			strSpeed.Format(_T("%dB/s"),speed);
		}
	}
	
	return strSpeed;
}

double CPort::GetRealSpeed()
{
	double dbSpeed = 0.0;
	if (m_dbUsedNoWaitTimeS > 0)
	{
		dbSpeed = (m_ullCompleteSize / 1024. / 1024.) / m_dbUsedNoWaitTimeS;
	}
	return dbSpeed;
}

CString CPort::GetRealSpeedString()
{
	CString strSpeed;
	if (m_dbUsedNoWaitTimeS > 0)
	{
		int speed = 0;
		if ((speed = (int)((m_ullCompleteSize / 1024 / 1024 / 1024) / m_dbUsedNoWaitTimeS)))
		{
			strSpeed.Format(_T("%dGB/s"),speed);
		}
		else if ((speed = (int)((m_ullCompleteSize / 1024 / 1024 ) / m_dbUsedNoWaitTimeS)))
		{
			strSpeed.Format(_T("%dMB/s"),speed);
		}
		else if ((speed = (int)((m_ullCompleteSize / 1024) / m_dbUsedNoWaitTimeS)))
		{
			strSpeed.Format(_T("%dKB/s"),speed);
		}
		else
		{
			speed = (int)(m_ullCompleteSize / m_dbUsedNoWaitTimeS);
			strSpeed.Format(_T("%dB/s"),speed);
		}
	}
	return strSpeed;
}

int CPort::GetPercent()
{
	int percent = 0;
	if (m_ullValidSize > 0)
	{
		percent = int(m_ullCompleteSize * 100 / m_ullValidSize);
	}
	return percent;
}

CString CPort::GetPercentString()
{
	CString strPercent;
	strPercent.Format(_T("%d%%"),GetPercent());

	return strPercent;
}

void CPort::SetHash( const BYTE *value,size_t length )
{
	ASSERT(length <= LEN_DIGEST);
	memset(m_Hash,0,LEN_DIGEST);
	memcpy(m_Hash,value,length);
}

void CPort::GetHash( PBYTE value,size_t length )
{
	ASSERT(length <= LEN_DIGEST);
	memcpy(value,m_Hash,length);
}

void CPort::SetPortState( PortState state )
{
	m_PortState = state;
}

PortState CPort::GetPortState()
{
	return m_PortState;
}

void CPort::SetResult( BOOL bResult )
{
	m_bResult = bResult;
}

BOOL CPort::GetResult()
{
	return m_bResult;
}

void CPort::SetErrorCode( ErrorType errType,DWORD dwErrorCode )
{
	m_ErrorType = errType;
	m_dwErrorCode = dwErrorCode;
}

ErrorType CPort::GetErrorCode( PDWORD pdwErrorCode )
{
	*pdwErrorCode = m_dwErrorCode;
	return m_ErrorType;
}

LPOVERLAPPED CPort::GetOverlapped()
{
	return &m_Overlapped;
}


