#pragma once
#include "Port.h"
#include "HashMethod.h"
#include "PortCommand.h"
#include "WPDFunction.h"

typedef CList<PObjectProperties,PObjectProperties> CListFileObjects;
typedef CList<CMapStringToString*,CMapStringToString*> CListMapString;
typedef CMap<CString,LPCTSTR,PObjectProperties,PObjectProperties> CMapObjectProerties;

class CWpdDevice
{
public:
	CWpdDevice(void);
	~CWpdDevice(void);

	// 公共方法
	void Init(HWND hWnd,LPBOOL lpCancel,HANDLE hLogFile,CPortCommand *pCommand);
	void SetMasterPort(CPort *port);
	void SetTargetPorts(PortList *pList);
	void SetHashMethod(BOOL bComputeHash,BOOL bHashVerify,HashMethod hashMethod);
	void SetWorkMode(WorkMode workMode);


	BOOL Start();


private:
	CPort    *m_MasterPort;
	PortList *m_TargetPorts;
	CDataQueueList m_DataQueueList;
	HWND     m_hWnd;
	HANDLE   m_hLogFile;
	LPBOOL   m_lpCancel;

	BOOL m_bComputeHash;
	BOOL m_bHashVerify;
	HashMethod m_HashMethod;
	CHashMethod *m_pMasterHashMethod;
	BYTE m_ImageHash[LEN_DIGEST];
	CString m_strMasterHash;

	WorkMode m_WorkMode;

	volatile UINT m_nCurrentTargetCount;

	CPortCommand *m_pCommand;

	ULONGLONG m_ullValidSize;

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

	// 线程
	static DWORD WINAPI ReadWpdFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI WriteWpdFilesThreadProc(LPVOID lpParm);
	static DWORD WINAPI VerifyWpdFilesThreadProc(LPVOID lpParm);

	BOOL OnCopyWpdFiles();

	BOOL ReadWpdFiles();
	BOOL WriteWpdFiles(CPort* port,CDataQueue *pDataQueue,CMapStringToString *pMap);
	BOOL VerifyWpdFiles(CPort *port,CHashMethod *pHashMethod,CMapStringToString *pMap);
};
