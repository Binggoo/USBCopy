#pragma once
#include "Port.h"
#include "HashMethod.h"

#define WM_SEND_FUNCTION_TEXT (WM_USER + 101)

class CDisk
{
public:
	CDisk(void);
	~CDisk(void);

	//��̬����
	static HANDLE GetHandleOnPhysicalDrive(int iDiskNumber,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnVolume(TCHAR letter,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnFile(LPCTSTR lpszFileName,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static ULONGLONG GetNumberOfSectors(HANDLE hDevice,PDWORD pdwBytesPerSector);
	static void DeleteDirectory( LPCTSTR lpszPath );
	static BOOL CreateDisk(int disk,ULONG PartitionNumber);
	static BOOL DestroyDisk(int disk);
	static BOOL ChangeLetter(LPCTSTR lpszVolumeName,LPCTSTR lpszVolumePath);
	static BOOL LockOnVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL UnLockVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL UnmountVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL IsVolumeUnmount(HANDLE hVolume);

	// ��������
	void Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile);
	void SetMasterPort(CPort *port);
	void SetTargetPorts(PortList *pList);
	void SetHashMethod(BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod);
	void SetWorkMode(WorkMode workMode);
	void SetCleanMode(CleanMode cleanMode,int nFillValue);
	void SetCompareMode(CompareMode compareMode);
	BOOL Start();

private:
	CPort    *m_MasterPort;
	PortList *m_TargetPorts;
	HWND     m_hWnd;
	HANDLE   m_hLogFile;
	LPBOOL   m_lpCancel;
	HANDLE   m_hMaster;
	CCriticalSection m_CS;
	
	ULONGLONG m_ullCapacity;
	ULONGLONG m_ullSectorNums;
	DWORD     m_dwBytesPerSector;
	ULONGLONG m_ullValidSize;

	BOOL m_bComputeHash;
	BOOL m_bHashVerify;
	HashMethod m_HashMethod;
	CHashMethod *m_pMasterHashMethod;
	BYTE m_ImageHash[LEN_DIGEST];

	WorkMode m_WorkMode;

	CleanMode m_CleanMode;
	int       m_nFillValue;

	CompareMode m_CompareMode;

	EFF_LIST  m_EffList;

	CDataQueue m_CompressQueue;
	CDataQueueList m_DataQueueList;

	volatile UINT m_nCurrentTargetCount;

	BOOL ReadSectors(HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff, LPOVERLAPPED lpOverlap,DWORD *pdwErrorCode);
	BOOL WriteSectors(HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff,LPOVERLAPPED lpOverlap, DWORD *pdwErrorCode);

	BOOL ReadFileAsyn(HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode);
	BOOL WriteFileAsyn(HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode);


	// �ļ�ϵͳ����
	BootSector GetBootSectorType(const PBYTE pXBR);
	PartitionStyle GetPartitionStyle(const PBYTE pByte,BootSector bootSector);
	FileSystem GetFileSystem(const PBYTE pDBR,ULONGLONG ullStartSector);
	BOOL BriefAnalyze();
	BOOL AppendEffDataList(const PBYTE pDBR,FileSystem fileSystem,ULONGLONG ullStartSector,ULONGLONG ullMasterSectorOffset,ULONGLONG ullSectors);
	ULONGLONG ReadOffset(const PBYTE pByte,DWORD offset,BYTE bytes);
	BOOL IsFATValidCluster(const PBYTE pByte,DWORD cluster,BYTE byFATBit);
	BOOL IsNTFSValidCluster(const PBYTE pByte,DWORD cluster);
	BOOL IsExtXValidBlock(const PBYTE pByte,DWORD block);

	void SetValidSize(ULONGLONG ullSize);
	ULONGLONG GetValidSize();
	bool IsAllFailed(ErrorType &errType,PDWORD pdwErrorCode);

	void AddDataQueueList(DATA_INFO dataInfo);
	bool IsReachLimitQty(int limit);

	BOOL QuickClean(CPort *port,PDWORD pdwErrorCode);

	// �߳�
	static DWORD WINAPI ReadDiskThreadProc(LPVOID lpParm);
	static DWORD WINAPI ReadImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteDiskThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI VerifyThreadProc(LPVOID lpParm);
	static DWORD WINAPI CompressThreadProc(LPVOID lpParm);
	static DWORD WINAPI UnCompressThreadProc(LPVOID lpParm);
	static DWORD WINAPI CleanDiskThreadProc(LPVOID lpParm);

	BOOL OnCopyDisk();
	BOOL OnCopyImage();
	BOOL OnMakeImage();
	BOOL OnCompareDisk();
	BOOL OnCleanDisk();

	BOOL ReadDisk();
	BOOL ReadImage();
	BOOL WriteDisk(CPort *port,CDataQueue *pDataQueue);
	BOOL WriteImage(CPort *port,CDataQueue *pDataQueue);
	BOOL Verify(CPort *port,CHashMethod *pHashMethod);
	BOOL Compress();
	BOOL Uncompress();
	BOOL CleanDisk(CPort *port);

};

typedef struct _STRUCT_LPVOID_PARM
{
	LPVOID lpVoid1;   //CDisk
	LPVOID lpVoid2;   //CPort
	LPVOID lpVoid3;   //CHashMethod or CDataQueue
}VOID_PARM,*LPVOID_PARM;

