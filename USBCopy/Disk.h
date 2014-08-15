#pragma once
#include "Port.h"
#include "HashMethod.h"
#include "PortCommand.h"

#define WM_VERIFY_START (WM_USER + 101)

typedef CMap<CString,LPCTSTR,ULONGLONG,ULONGLONG> CMapStringToULL;

class CDisk
{
public:
	CDisk(void);
	~CDisk(void);

	//静态方法
	static HANDLE GetHandleOnPhysicalDrive(int iDiskNumber,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnVolume(TCHAR letter,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnFile(LPCTSTR lpszFileName,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static ULONGLONG GetNumberOfSectors(HANDLE hDevice,PDWORD pdwBytesPerSector);
	static void DeleteDirectory( LPCTSTR lpszPath );
	static BOOL CreateDisk(int disk,ULONG PartitionNumber);
	static BOOL DestroyDisk(int disk);
	static BOOL DestroyDisk(HANDLE hDisk);
	static BOOL ChangeLetter(LPCTSTR lpszVolumeName,LPCTSTR lpszVolumePath);
	static BOOL LockOnVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL UnLockVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL UnmountVolume(HANDLE hVolume,DWORD *pdwErrorCode);
	static BOOL IsVolumeUnmount(HANDLE hVolume);
	static BOOL SetDiskAtrribute(HANDLE hDisk,BOOL bReadOnly,BOOL bOffline,PDWORD pdwErrorCode);

	// 公共方法
	void Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile,CPortCommand *pCommand);
	void SetMasterPort(CPort *port);
	void SetTargetPorts(PortList *pList);
	void SetHashMethod(BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod);
	void SetWorkMode(WorkMode workMode);
	void SetCleanMode(CleanMode cleanMode,int nFillValue);
	void SetCompareMode(CompareMode compareMode);
	void SetFileAndFolder(const CStringArray &fileArray,const CStringArray &folderArray);
	void SetFormatParm(CString strVolumeLabel,FileSystem fileSystem,DWORD dwClusterSize,BOOL bQuickFormat);
	BOOL Start();

private:
	CPort    *m_MasterPort;
	PortList *m_TargetPorts;
	HWND     m_hWnd;
	HANDLE   m_hLogFile;
	LPBOOL   m_lpCancel;
	HANDLE   m_hMaster;
	
	ULONGLONG m_ullCapacity;
	ULONGLONG m_ullSectorNums;
	DWORD     m_dwBytesPerSector;
	ULONGLONG m_ullValidSize;

	BOOL m_bComputeHash;
	BOOL m_bHashVerify;
	HashMethod m_HashMethod;
	CHashMethod *m_pMasterHashMethod;
	BYTE m_ImageHash[LEN_DIGEST];
	CString m_strMasterHash;

	WorkMode m_WorkMode;

	CleanMode m_CleanMode;
	int       m_nFillValue;

	CompareMode m_CompareMode;

	EFF_LIST  m_EffList;

	CDataQueue m_CompressQueue;
	CDataQueueList m_DataQueueList;

	volatile UINT m_nCurrentTargetCount;

	CStringArray  m_FileArray;
	CStringArray  m_FodlerArray;

	CMapStringToULL m_MapFiles;

	BOOL m_bQuickFormat;
	FileSystem m_FileSystem;
	DWORD m_dwClusterSize;
	CString m_strVolumnLabel;

	CPortCommand *m_pCommand;

	BOOL m_bCompressComplete; //压缩线程和解压线程是否结束

	BOOL ReadSectors(HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff, LPOVERLAPPED lpOverlap,DWORD *pdwErrorCode);
	BOOL WriteSectors(HANDLE hDevice,ULONGLONG ullStartSector,DWORD dwSectors,DWORD dwBytesPerSector, LPBYTE lpSectBuff,LPOVERLAPPED lpOverlap, DWORD *pdwErrorCode);

	BOOL ReadFileAsyn(HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode);
	BOOL WriteFileAsyn(HANDLE hFile,ULONGLONG ullOffset,DWORD &dwSize,LPBYTE lpBuffer,LPOVERLAPPED lpOverlap,PDWORD pdwErrorCode);


	// 文件系统分析
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

	int EnumFile(CString strSource);

	BOOL QuickClean(CPort *port,PDWORD pdwErrorCode);

	// 线程
	static DWORD WINAPI ReadDiskThreadProc(LPVOID lpParm);
	static DWORD WINAPI ReadImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteDiskThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI VerifyThreadProc(LPVOID lpParm);
	static DWORD WINAPI CompressThreadProc(LPVOID lpParm);
	static DWORD WINAPI UnCompressThreadProc(LPVOID lpParm);
	static DWORD WINAPI CleanDiskThreadProc(LPVOID lpParm);

	static DWORD WINAPI ReadFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI VerifyFilesThreadProc(LPVOID lpParm);

	static DWORD WINAPI FormatDiskThreadProc(LPVOID lpParm);

	BOOL OnCopyDisk();
	BOOL OnCopyImage();
	BOOL OnMakeImage();
	BOOL OnCompareDisk();
	BOOL OnCleanDisk();
	BOOL OnCopyFiles();
	BOOL OnFormatDisk();

	BOOL ReadDisk();
	BOOL ReadImage();
	BOOL WriteDisk(CPort *port,CDataQueue *pDataQueue);
	BOOL WriteImage(CPort *port,CDataQueue *pDataQueue);
	BOOL Verify(CPort *port,CHashMethod *pHashMethod);
	BOOL Compress();
	BOOL Uncompress();
	BOOL CleanDisk(CPort *port);

	BOOL ReadFiles();
	BOOL WriteFiles(CPort *port,CDataQueue *pDataQueue);
	BOOL VerifyFiles(CPort *port,CHashMethod *pHashMethod);

	BOOL FormatDisk(CPort *port);

};

typedef struct _STRUCT_LPVOID_PARM
{
	LPVOID lpVoid1;   //CDisk
	LPVOID lpVoid2;   //CPort
	LPVOID lpVoid3;   //CHashMethod or CDataQueue
}VOID_PARM,*LPVOID_PARM;

//
// IOCTLs to query and modify attributes
// associated with the given disk. These
// are persisted within the registry.
//

#define IOCTL_DISK_GET_DISK_ATTRIBUTES      CTL_CODE(IOCTL_DISK_BASE, 0x003c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_SET_DISK_ATTRIBUTES      CTL_CODE(IOCTL_DISK_BASE, 0x003d, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define DISK_ATTRIBUTE_OFFLINE              0x0000000000000001
#define DISK_ATTRIBUTE_READ_ONLY            0x0000000000000002

//
// IOCTL_DISK_GET_DISK_ATTRIBUTES
//
// Input Buffer:
//     None
//
// Output Buffer:
//     Structure of type GET_DISK_ATTRIBUTES
//

typedef struct _GET_DISK_ATTRIBUTES {

	//
	// Specifies the size of the
	// structure for versioning.
	//
	ULONG Version;

	//
	// For alignment purposes.
	//
	ULONG Reserved1;

	//
	// Specifies the attributes
	// associated with the disk.
	//
	ULONGLONG Attributes;

} GET_DISK_ATTRIBUTES, *PGET_DISK_ATTRIBUTES;

//
// IOCTL_DISK_SET_DISK_ATTRIBUTES
//
// Input Buffer:
//     Structure of type SET_DISK_ATTRIBUTES
//
// Output Buffer:
//     None
//

typedef struct _SET_DISK_ATTRIBUTES {

	//
	// Specifies the size of the
	// structure for versioning.
	//
	ULONG Version;

	//
	// Indicates whether to remember
	// these settings across reboots
	// or not.
	//
	BOOLEAN Persist;

	//
	// Indicates whether the ownership
	// taken earlier is being released.
	//
	BOOLEAN RelinquishOwnership;

	//
	// For alignment purposes.
	//
	BOOLEAN Reserved1[2];

	//
	// Specifies the new attributes.
	//
	ULONGLONG Attributes;

	//
	// Specifies the attributes
	// that are being modified.
	//
	ULONGLONG AttributesMask;

	//
	// Specifies an identifier to be
	// associated  with  the caller.
	// This setting is not persisted
	// across reboots.
	//
	GUID Owner;

} SET_DISK_ATTRIBUTES, *PSET_DISK_ATTRIBUTES;