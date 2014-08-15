#pragma once
#include <WinIoCtl.h>

#define START_SECTOR 2048

class Fs
{
public:
	Fs();
	~Fs(void);

	BOOL Init(HANDLE hDisk,DWORD dwClusterSize,const char* lpszVolumeLable);
	BOOL InitialDisk();
	BOOL PartitionDisk();
	DWORD GetErrorCode();

	virtual BOOL FormatPartition();

protected:
	HANDLE m_hDisk;
	OVERLAPPED m_WriteOverlap;
	OVERLAPPED m_ReadOverlap;
	DWORD m_dwErrorCode;
	DWORD m_dwBytesPerSector;
	ULONGLONG m_ullSectorNum;
	DISK_GEOMETRY_EX m_DiskGeometry;
	DWORD m_dwClusterSize;
	char m_szVolumeLable[12];

	BOOL ReadSectors(ULONGLONG ullStartSector,DWORD dwSectors,LPBYTE lpSectBuff);
	BOOL WriteSectors(ULONGLONG ullStartSector,DWORD dwSectors,LPBYTE lpSectBuff);
};

