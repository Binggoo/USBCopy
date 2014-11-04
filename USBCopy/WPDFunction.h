#ifndef _WPD_FUNCTION_H_
#define _WPD_FUNCTION_H_

#ifndef IID_PPV_ARGS
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), (static_cast<IUnknown *>(*(ppType)),reinterpret_cast<void**>(ppType))
#endif

#define NUM_OBJECTS_TO_REQUEST 10

typedef struct _STRUCT_OBJECT_PROPERTIES
{
	LPWSTR pwszObjectID;
	LPWSTR pwszObjectParentID;
	LPWSTR pwszObjectName;
	LPWSTR pwszObjectOringalFileName;
	GUID   guidObjectContentType;
	GUID   guidOjectFormat;
	ULONGLONG ullObjectSize;
	int level;
}ObjectProperties,*PObjectProperties;

#include <PortableDeviceApi.h>  // Include this header for Windows Portable Device API interfaces
#include <PortableDevice.h>     // Include this header for Windows Portable Device definitions
#pragma comment(lib,"PortableDeviceGuids.lib")

HRESULT OpenDevice(IPortableDevice** ppDevice,LPCWSTR wszPnPDeviceID);
HRESULT CloseDevice(IPortableDevice* pDevice);
void GetDeviceModelAndSerialNumber(IPortableDevice* pDevice,LPWSTR pwszModelName,LPWSTR pwszSerialNumber);
void GetStorageFreeAndTotalCapacity(IPortableDevice* pDevice,LPCTSTR wszObjectID,PULONGLONG pullFreeSize,PULONGLONG pullTotalSize);
DWORD EnumStorageIDs(IPortableDevice* pDevice,LPWSTR **ppwszObjectIDs);

HRESULT ReadWpdFile(IStream *pStream,LPVOID lpBuffer,DWORD &dwNumberOfBytesToRead);
HRESULT WriteWpdFile(IStream *pStream,LPCVOID lpBuffer,DWORD &dwNumberOfBytesToWrite);
HRESULT DeleteWpdFile(IPortableDevice *pDevice,LPCTSTR wszObjectID,DWORD dwRecursion = PORTABLE_DEVICE_DELETE_NO_RECURSION);
HRESULT CreateFolder(IPortableDevice *pDevice,LPCWSTR pszParentObjectID,LPCWSTR pszFolderName,LPWSTR pwszNewObjectID);
HRESULT GetObjectProperties(IPortableDevice *pDevice,LPCWSTR wszObjectID,PObjectProperties pObjectProperties);
HRESULT GetObjectProperties(IPortableDeviceContent* pContent,LPCWSTR wszObjectID,PObjectProperties pObjectProperties);
HRESULT CreateDataOutStream(IPortableDevice *pDevice,
							LPCWSTR pwszParentObjectID,
							PObjectProperties pObjectProperties,
							IPortableDeviceDataStream **ppDataStream,
							PDWORD pdwOptimalWriteBufferSize);

HRESULT CreateDataInStream(IPortableDevice *pDevice,LPCWSTR lpszObjectID,IStream **ppDataStream,PDWORD pdwOptimalWriteBufferSize);
#endif