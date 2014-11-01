#pragma once
#include "Port.h"
#include "HashMethod.h"
#include "PortCommand.h"
#include "IoctlDef.h"

typedef CMap<CString,LPCTSTR,ULONGLONG,ULONGLONG> CMapStringToULL;
typedef CMap<CPort *,CPort *,CStringArray*,CStringArray*> CMapPortStringArray;

class CDisk
{
public:
	CDisk(void);
	~CDisk(void);

	//静态方法
	static HANDLE GetHandleOnPhysicalDrive(int iDiskNumber,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnVolume(TCHAR letter,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static HANDLE GetHandleOnFile(LPCTSTR lpszFileName,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,PDWORD pdwErrorCode);
	static ULONGLONG GetNumberOfSectors(HANDLE hDevice,PDWORD pdwBytesPerSector,MEDIA_TYPE *type);
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
	static BOOL GetTSModelNameAndSerialNumber(HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode);
	static BOOL GetDiskModelNameAndSerialNumber(HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode);
	static BOOL GetUsbHDDModelNameAndSerialNumber(HANDLE hDevice,LPTSTR lpszModulName,LPTSTR lpszSerialNum,DWORD *pdwErrorCode);
	// 公共方法
	void Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile,CPortCommand *pCommand,UINT nBlockSectors);
	void SetMasterPort(CPort *port);
	void SetTargetPorts(PortList *pList);
	void SetHashMethod(BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod);
	void SetWorkMode(WorkMode workMode);
	void SetCleanMode(CleanMode cleanMode,int nFillValue);
	void SetCompareMode(CompareMode compareMode);
	void SetFileAndFolder(const CStringArray &fileArray,const CStringArray &folderArray);
	void SetFormatParm(CString strVolumeLabel,FileSystem fileSystem,DWORD dwClusterSize,BOOL bQuickFormat);
	void SetMakeImageParm(int compressLevel);
	BOOL Start();

	void SetSocket(SOCKET sClient,BOOL bServerFirst);

	// 2014-10-14 增量拷贝新增
	void SetDiffCopyParm(UINT nSourceType,BOOL bServer,UINT nCompareRule,BOOL bUpload,const CStringArray &logArray,BOOL bIncludeSunDir);

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

	CMapStringToULL m_MapCopyFiles;

	BOOL m_bQuickFormat;
	FileSystem m_FileSystem;
	DWORD m_dwClusterSize;
	CString m_strVolumnLabel;

	CPortCommand *m_pCommand;

	BOOL m_bCompressComplete; //压缩线程和解压线程是否结束

	SOCKET m_ClientSocket;
	BOOL   m_bServerFirst;

	int m_iCompressLevel;

	UINT m_nBlockSectors;

	// 20140-10-14 新增增量拷贝
	CMapPortStringArray m_MapPortStringArray;
	CStringArray m_ArrayLogPath;
	BOOL m_bUploadUserLog;
	BOOL m_bIncludeSubDir;
	UINT m_nCompareRule; //0 - FileSize, 1 - HashValue
	UINT m_nSourceType;  //0 - Package, 1 - Master
	CStringArray m_ArrayDelFiles;
	CStringArray m_ArrayDelFolders;
	CMapStringToULL m_MapSizeSourceFiles;
	CMapStringToULL m_MapSizeDestFiles;
	CMapStringToString m_MapHashSourceFiles;
	CMapStringToString m_MapHashDestFiles;
	CStringArray m_ArraySameFile;
	ULONGLONG m_ullHashFilesSize;
	CMapStringToString m_MapSourceFolders;
	CMapStringToString m_MapDestFolders;

	BOOL ReadSectors(HANDLE hDevice,
		ULONGLONG ullStartSector,
		DWORD dwSectors,
		DWORD dwBytesPerSector, 
		LPBYTE lpSectBuff, 
		LPOVERLAPPED lpOverlap,
		DWORD *pdwErrorCode,
		DWORD dwTimeOut = 10000);
	BOOL WriteSectors(HANDLE hDevice,
		ULONGLONG ullStartSector,
		DWORD dwSectors,
		DWORD dwBytesPerSector, 
		LPBYTE lpSectBuff,
		LPOVERLAPPED lpOverlap, 
		DWORD *pdwErrorCode,
		DWORD dwTimeOut = 10000);

	BOOL ReadFileAsyn(HANDLE hFile,
		ULONGLONG ullOffset,
		DWORD &dwSize,
		LPBYTE lpBuffer,
		LPOVERLAPPED lpOverlap,
		PDWORD pdwErrorCode,
		DWORD dwTimeOut = 10000);
	BOOL WriteFileAsyn(HANDLE hFile,
		ULONGLONG ullOffset,
		DWORD &dwSize,
		LPBYTE lpBuffer,
		LPOVERLAPPED lpOverlap,
		PDWORD pdwErrorCode,
		DWORD dwTimeOut= 10000);


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

	void AddDataQueueList(PDATA_INFO dataInfo);
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

	// 2014-10-14 增量拷贝新增
	static DWORD WINAPI SearchUserLogThreadProc(LPVOID lpParm);
	static DWORD WINAPI DeleteChangeThreadProc(LPVOID lpParm);
	static DWORD WINAPI EnumFileThreadProc(LPVOID lpParm);
	static DWORD WINAPI ComputeHashThreadProc(LPVOID lpParm);

	BOOL OnCopyDisk();
	BOOL OnCopyImage();
	BOOL OnMakeImage();
	BOOL OnCompareDisk();
	BOOL OnCleanDisk();
	BOOL OnCopyFiles();
	BOOL OnFormatDisk();
	// 2014-10-14 增量拷贝新增
	BOOL OnDiffCopy();

	BOOL ReadDisk();
	BOOL ReadImage();
	BOOL WriteDisk(CPort *port,CDataQueue *pDataQueue);
	BOOL WriteImage(CPort *port,CDataQueue *pDataQueue);
	BOOL Verify(CPort *port,CHashMethod *pHashMethod);
	BOOL Compress();
	BOOL Uncompress();
	BOOL CleanDisk(CPort *port);

	BOOL ReadFiles();
	BOOL ReadLocalFiles();
	BOOL WriteFiles(CPort *port,CDataQueue *pDataQueue);
	BOOL VerifyFiles(CPort *port,CHashMethod *pHashMethod);

	BOOL FormatDisk(CPort *port);

	BOOL ReadLocalImage();
	BOOL ReadRemoteImage();

	BOOL WriteLocalImage(CPort *port,CDataQueue *pDataQueue);
	BOOL WriteRemoteImage(CPort *port,CDataQueue *pDataQueue);

	// 2014-10-14 增量拷贝新增
	BOOL SearchUserLog(CPort *port);
	void SearchUserLog(CString strPath,CString strType,CStringArray *pArray);
	void CleanMapPortStringArray();
	BOOL UploadUserLog();
	BOOL ReadRemoteFiles();
	BOOL DownloadChangeList();
	BOOL QueryChangeList();
	BOOL DeleteChange(CPort *port);
	int EnumFile(CString strPath,CString strVolume,CMapStringToULL *pMap,CMapStringToString *pArray);

	BOOL EnumFileSize(CPort *port,CMapStringToULL *pMap,CMapStringToString *pArray);
	BOOL ComputeHashValue(CPort *port,CMapStringToString *pMap);
	void CompareFileSize();
	void CompareHashValue();

	static TCHAR *ConvertSENDCMDOUTPARAMSBufferToString(const DWORD *dwDiskData, DWORD nFirstIndex, DWORD nLastIndex);
	static DWORD ScsiCommand( HANDLE hDevice,
		void *pCdb,
		UCHAR ucCdbLength,
		void *pDataBuffer,
		ULONG ulDataLength,
		UCHAR ucDirection ,
		void *pSenseBuffer,
		UCHAR ucSenseLength,
		ULONG ulTimeout );

	static DWORD DoIdentifyDeviceSat(HANDLE hDevice, BYTE target, IDENTIFY_DEVICE* data, COMMAND_TYPE type);
};

typedef struct _STRUCT_LPVOID_PARM
{
	LPVOID lpVoid1;   //CDisk
	LPVOID lpVoid2;   //CPort
	LPVOID lpVoid3;   //CHashMethod or CDataQueue
	LPVOID lpVoid4;   //Other
}VOID_PARM,*LPVOID_PARM;

