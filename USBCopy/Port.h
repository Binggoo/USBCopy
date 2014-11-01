#pragma once
#include "GlobalDef.h"
#include "DataQueue.h"

class CPort
{
public:
	CPort(void);
	~CPort(void);

private:

	// �����ļ�
	int m_iPortNum;
	PortType m_PortType;
	CString m_strPath1;
	CString m_strPath2;
	int m_iConnectIndex1;
	int m_iConnectIndex2;

	// �豸
	BOOL m_bConnected;
	int m_iDiskNum;
	CStringArray m_strVolumeArray;	
	ULONGLONG m_ullTotalSize;
	DWORD     m_dwBytesPerSector;
	CString m_strSN;
	CString m_strModuleName;
	WorkMode m_WorkMode;
	HashMethod m_HashMethod;
	BOOL m_bKickOff; // �޳���־

	// 2014-10-30 MTP ����
	CString m_strDevicePath;
	
	// ӳ��
	CString m_strFileName;

	// ��ֹʱ��
	CTime m_StartTime;
	CTime m_EndTime;

	// ����
	volatile ULONGLONG m_ullValidSize;
	volatile ULONGLONG m_ullCompleteSize;
	volatile double     m_dbUsedWaitTimeS;
	volatile double     m_dbUsedNoWaitTimeS;
	BYTE m_Hash[LEN_DIGEST];

	// ���
	ErrorType m_ErrorType;
	DWORD m_dwErrorCode;
	PortState m_PortState;
	BOOL m_bResult;

	// OVERLAPPED
	OVERLAPPED m_ReadOverlapped;
	OVERLAPPED m_WriteOverlapped;

public:
	void Initial();
	void Reset(); //ÿ�ο�ʼ֮ǰ�����һ������
	void Active();

	//�����ļ�
	void SetPortNum(int iPortNum);
	int GetPortNum();
	CString GetPortName();

	void SetPortType(PortType type);
	PortType GetPortType();
	CString GetPortTypeName();

	CString GetPath1();
	int GetConnectIndex1();
	void SetPath1AndIndex(CString strPath);

	CString GetPath2();
	int GetConnectIndex2();
	void SetPath2AndIndex(CString strPath);

	// �豸
	void SetConnected(BOOL bConnected);
	BOOL IsConnected();

	int GetDiskNum();
	void SetDiskNum(int iNum);
	
	void SetVolumeArray(const CStringArray& strVolumeArray);
	void GetVolumeArray(CStringArray& strVolumeArray);

	ULONGLONG GetTotalSize();
	void SetTotalSize(ULONGLONG ullSize);

	DWORD GetBytesPerSector();
	void SetBytesPerSector(DWORD dwValue);

	CString GetSN();
	void SetSN(CString strSN);

	CString GetModuleName();
	void SetModuleName(CString strName);

	void SetWorkMode(WorkMode workMode);
	WorkMode GetWorkMode();

	void SetHashMethod(HashMethod method);
	HashMethod GetHashMethod();
	CString GetHashMethodString();
	int GetHashLength();

	BOOL IsKickOff();
	void SetKickOff(BOOL bKickOff);

	// 2014-10-30 MTP ����
	CString GetDevicePath();
	void SetDevicePath(CString strDevicePath);

	//ӳ��
	CString GetFileName();
	void SetFileName(CString strFile);

	// ��ֹʱ��
	void SetStartTime(CTime time);
	CTime GetStartTime();

	void SetEndTime(CTime time);
	CTime GetEndTime();

	// ����
	ULONGLONG GetValidSize();
	void SetValidSize(ULONGLONG ullSize);

	void SetCompleteSize(ULONGLONG ullSize);
	void AppendCompleteSize(ULONGLONG ullSize);
	ULONGLONG GetCompleteSize();

	void SetUsedWaitTimeS(double dbTimeS);
	void AppendUsedWaitTimeS(double dbTimeS);
	double GetUsedWaitTimeS();

	void SetUsedNoWaitTimeS(double dbTimeS);
	void AppendUsedNoWaitTimeS(double dbTimeS);
	double GetUsedNoWaitTimeS();

	double GetAvgSpeed();
	CString GetAvgSpeedString();

	double GetRealSpeed();
	CString GetRealSpeedString();

	int GetPercent();
	CString GetPercentString();

	void SetHash(const BYTE *value,size_t length);
	void GetHash(PBYTE value,size_t length);
	CString GetHashString();


	// ���
	void SetPortState(PortState state);
	PortState GetPortState();

	void SetResult(BOOL bResult);
	BOOL GetResult();
	CString GetResultString();

	void SetErrorCode(ErrorType errType,DWORD dwErrorCode);
	ErrorType GetErrorCode(PDWORD pdwErrorCode);

	// OVERLAPPED
	LPOVERLAPPED GetOverlapped(BOOL bRead);

};

typedef CList<CPort*,CPort*> PortList;

