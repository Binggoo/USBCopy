#pragma once
#include "Disk.h"
#include "WPDFunction.h"

typedef CList<PObjectProperties,PObjectProperties> CListFileObjects;
typedef CList<CMapStringToString*,CMapStringToString*> CListMapString;
typedef CMap<CString,LPCTSTR,PObjectProperties,PObjectProperties> CMapObjectProerties;

class CWpdDevice : public CDisk
{
public:
	CWpdDevice(void);
	~CWpdDevice(void);

	// 公共方法
	void Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile,CPortCommand *pCommand);
	void SetMasterPort(CPort *port);
	void SetTargetPorts(PortList *pList);
	void SetHashMethod(BOOL bComputeHash,HashMethod hashMethod);
	void SetCompareParm(BOOL bCompare,CompareMethod method);
	void SetWorkMode(WorkMode workMode);
	void SetSocket(SOCKET sClient,BOOL bServerFirst);
	void SetMakeImageParm(BOOL bCompress,int compressLevel);

	BOOL Start();


private:
	CPort    *m_MasterPort;
	PortList *m_TargetPorts;
	CDataQueueList m_DataQueueList;
	CDataQueue m_CompressQueue;
	HWND     m_hWnd;
	HANDLE   m_hLogFile;
	LPBOOL   m_lpCancel;

	BOOL m_bComputeHash;
	BOOL m_bCompare;
	HashMethod m_HashMethod;
	CHashMethod *m_pMasterHashMethod;
	BYTE m_ImageHash[LEN_DIGEST];
	CString m_strMasterHash;

	WorkMode m_WorkMode;

	CPortCommand *m_pCommand;

	// 2014-11-14 增加MTP Image Copy
	BOOL m_bCompressComplete; //压缩线程和解压线程是否结束
	BOOL m_bDataCompress;

	//映像拷贝参数
	SOCKET m_ClientSocket;
	BOOL   m_bServerFirst;
	int    m_iCompressLevel;

	ULONGLONG m_ullValidSize;

	// Image文件中的一些有用值
	HANDLE    m_hImage;
	ULONGLONG m_ullImageSize;
	DWORD     m_dwPkgOffset;

	CListFileObjects m_MasterFolderObjectsList;
	CListFileObjects m_MasterFileObjectsList;
	CListMapString m_TargetMapStringFileObjectsList;
	CMapObjectProerties m_MapObjectProerties;

	void CleanupObjectProperties();
	void EnumAllObjects();
	void RecursiveEnumerate(LPCWSTR szObjectID,IPortableDeviceContent* pContent,int level);
	HRESULT CleanupDeviceStorage(IPortableDevice *pDevice,LPCWSTR lpszObjectID);


	void SetValidSize(ULONGLONG ullSize);
	bool IsAllFailed(ErrorType &errType,PDWORD pdwErrorCode);

	void AddDataQueueList(PDATA_INFO dataInfo);
	bool IsReachLimitQty(int limit);
	UINT GetCurrentTargetCount();

	bool IsTargetsReady();

	// 线程
	static DWORD WINAPI ReadWpdFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteWpdFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI VerifyWpdFilesThreadProc(LPVOID lpParm);

	// 2014-11-14 增加MTP Image Copy
	static DWORD WINAPI ReadImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteImageThreadProc(LPVOID lpParm);
	static DWORD WINAPI CompressThreadProc(LPVOID lpParm);
	static DWORD WINAPI UnCompressThreadProc(LPVOID lpParm);

	BOOL OnCopyWpdFiles();
	BOOL OnMakeImage();
	BOOL OnCopyImage();

	BOOL ReadWpdFiles();
	BOOL WriteWpdFiles(CPort* port,CDataQueue *pDataQueue,CMapStringToString *pMap);
	BOOL VerifyWpdFiles(CPort *port,CHashMethod *pHashMethod,CMapStringToString *pMap);

	// 2014-11-14 增加MTP Image Copy
	BOOL ReadImage();
	BOOL ReadLocalImage();
	BOOL ReadRemoteImage();

	BOOL WriteImage(CPort* port,CDataQueue *pDataQueue);
	BOOL WriteLocalImage(CPort *port,CDataQueue *pDataQueue);
	BOOL WriteRemoteImage(CPort *port,CDataQueue *pDataQueue);

	BOOL Compress();
	BOOL Uncompress();
};
