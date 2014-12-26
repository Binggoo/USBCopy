#include "StdAfx.h"
#include "WPDFunction.h"
#include "Utils.h"

void GetStringProperty(
	IPortableDeviceValues*  pProperties,
	REFPROPERTYKEY          key,
	LPWSTR                 pszString)
{
	PWSTR   pszValue = NULL;
	pszString[0] = NULL;
	HRESULT hr = pProperties->GetStringValue(key,&pszValue);
	if (SUCCEEDED(hr))
	{
		// Get the length of the string value so we
		// can output <empty string value> if one
		// is encountered.
		size_t len = wcslen(pszValue) + 1;
		wcscpy_s(pszString,len,pszValue);
	}

	// Free the allocated string returned from the
	// GetStringValue method
	CoTaskMemFree(pszValue);
	pszValue = NULL;
}

ULONGLONG GetUnsignedLargeIntergerProperty(
	IPortableDeviceValues*  pProperties,
	REFPROPERTYKEY          key)
{
	ULONGLONG ullValue;
	HRESULT hr = pProperties->GetUnsignedLargeIntegerValue(key,&ullValue);
	if (SUCCEEDED(hr))
	{
		return ullValue;
	}

	return 0;
}

/************************************************************************/
/* Open Device Function
 * IPortableDevice** ppDevice : IPortableDevice interface
 * LPCWSTR wszPnPDeviceID : DeviceID 
 * return : success S_OK                                                */
/************************************************************************/
HRESULT OpenDevice( IPortableDevice** ppDevice,LPCWSTR wszPnPDeviceID )
{
	HRESULT                hr                 = S_OK;
	IPortableDeviceValues* pClientInformation = NULL;
	IPortableDevice*       pDevice            = NULL;

	if ((wszPnPDeviceID == NULL) || (ppDevice == NULL))
	{
		hr = E_INVALIDARG;

		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));

		return hr;
	}

	// CoCreate an IPortableDeviceValues interface to hold the client information.
	hr = CoCreateInstance(CLSID_PortableDeviceValues,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IPortableDeviceValues,
		(VOID**) &pClientInformation);
	if (SUCCEEDED(hr))
	{
		HRESULT ClientInfoHR = S_OK;

		// Attempt to set all properties for client information. If we fail to set
		// any of the properties below it is OK. Failing to set a property in the
		// client information isn't a fatal error.
		ClientInfoHR = pClientInformation->SetStringValue(WPD_CLIENT_NAME, CLIENT_NAME);
		if (FAILED(ClientInfoHR))
		{
			// Failed to set WPD_CLIENT_NAME
			DbgPrint((_T("! Failed to set WPD_CLIENT_NAME, hr = 0x%lx"),ClientInfoHR));
		}

		ClientInfoHR = pClientInformation->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, CLIENT_MAJOR_VER);
		if (FAILED(ClientInfoHR))
		{
			// Failed to set WPD_CLIENT_MAJOR_VERSION
			DbgPrint((_T("! Failed to set WPD_CLIENT_MAJOR_VERSION, hr = 0x%lx"),ClientInfoHR));
		}

		ClientInfoHR = pClientInformation->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, CLIENT_MINOR_VER);
		if (FAILED(ClientInfoHR))
		{
			// Failed to set WPD_CLIENT_MINOR_VERSION
			DbgPrint((_T("! Failed to set WPD_CLIENT_MINOR_VERSION, hr = 0x%lx"),ClientInfoHR));
		}

		ClientInfoHR = pClientInformation->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, CLIENT_REVISION);
		if (FAILED(ClientInfoHR))
		{
			// Failed to set WPD_CLIENT_REVISION
			DbgPrint((_T("! Failed to set WPD_CLIENT_REVISION, hr = 0x%lx"),ClientInfoHR));
		}

		ClientInfoHR = pClientInformation->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION);
		if (FAILED(ClientInfoHR))
		{
			// Failed to set WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE
			DbgPrint((_T("! Failed to set WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, hr = 0x%lx"),ClientInfoHR));
		}
	}
	else
	{
		// Failed to CoCreateInstance CLSID_PortableDeviceValues for client information
		DbgPrint((_T("! Failed to CoCreateInstance CLSID_PortableDeviceValues for client information, hr = 0x%lx"),hr));
	}

	if (SUCCEEDED(hr))
	{
		// CoCreate an IPortableDevice interface
		hr = CoCreateInstance(CLSID_PortableDevice,//CLSID_PortableDeviceFTM (new) or CLSID_PortableDevice (old)
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IPortableDevice,
			(VOID**) &pDevice);

		if (SUCCEEDED(hr))
		{
			// Attempt to open the device using the PnPDeviceID string given
			// to this function and the newly created client information.
			// Note that we're attempting to open the device the first 
			// time using the default (read/write) access. If this fails
			// with E_ACCESSDENIED, we'll attempt to open a second time
			// with read-only access.
			hr = pDevice->Open(wszPnPDeviceID, pClientInformation);
			if (hr == E_ACCESSDENIED)
			{
				// Attempt to open for read-only access
				pClientInformation->SetUnsignedIntegerValue(
					WPD_CLIENT_DESIRED_ACCESS,
					GENERIC_READ);
				hr = pDevice->Open(wszPnPDeviceID, pClientInformation);
			}
			if (SUCCEEDED(hr))
			{
				// The device successfully opened, obtain an instance of the Device into
				// ppDevice so the caller can be returned an opened IPortableDevice.
				hr = pDevice->QueryInterface(IID_IPortableDevice, (VOID**)ppDevice);
				if (FAILED(hr))
				{
					// Failed to QueryInterface the opened IPortableDevice
					DbgPrint((_T("! Failed to QueryInterface the opened IPortableDevice, hr = 0x%lx"),hr));
				}
			}
		}
		else
		{
			// Failed to CoCreateInstance CLSID_PortableDevice
			DbgPrint((_T("! Failed to CoCreateInstance CLSID_PortableDevice, hr = 0x%lx"),hr));
		}
	}

	// Release the IPortableDevice when finished
	if (pDevice != NULL)
	{
		pDevice->Release();
		pDevice = NULL;
	}

	// Release the IPortableDeviceValues that contains the client information when finished
	if (pClientInformation != NULL)
	{
		pClientInformation->Release();
		pClientInformation = NULL;
	}

	return hr;
}

/************************************************************************/
/* Close Device Function                                                */
/************************************************************************/
HRESULT CloseDevice(IPortableDevice* pDevice)
{
	HRESULT                hr                 = S_OK;
	if (pDevice == NULL)
	{
		hr = E_INVALIDARG;

		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));

		return hr;
	}

	hr = pDevice->Close();

	return hr;
}

/************************************************************************/
/* GetDeviceModelAndSerialNumber Function                               */
/************************************************************************/
void GetDeviceModelAndSerialNumber( IPortableDevice* pDevice,LPWSTR pwszModelName,LPWSTR pwszSerialNumber )
{
	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));
		return;
	}

	HRESULT                               hr               = S_OK;
	CComPtr<IPortableDeviceProperties>    pProperties;
	CComPtr<IPortableDeviceValues>        pObjectProperties;
	CComPtr<IPortableDeviceContent>       pContent;
	CComPtr<IPortableDeviceKeyCollection> pPropertiesToRead;

	// 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pDevice->Content(&pContent);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 2) Get an IPortableDeviceProperties interface from the IPortableDeviceContent interface
	// to access the property-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pContent->Properties(&pProperties);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceProperties from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 3) CoCreate an IPortableDeviceKeyCollection interface to hold the the property keys
	// we wish to read.
	//<SnippetContentProp1>
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pPropertiesToRead));
	if (SUCCEEDED(hr))
	{
		// 4) Populate the IPortableDeviceKeyCollection with the keys we wish to read.
		// NOTE: We are not handling any special error cases here so we can proceed with
		// adding as many of the target properties as we can.
		if (pPropertiesToRead != NULL)
		{
			HRESULT hrTemp = S_OK;
			hrTemp = pPropertiesToRead->Add(WPD_DEVICE_MODEL);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_DEVICE_MODEL to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_DEVICE_SERIAL_NUMBER);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_NAME to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

		}
	}
	//</SnippetContentProp1>
	// 5) Call GetValues() passing the collection of specified PROPERTYKEYs.
	//<SnippetContentProp2>
	if (SUCCEEDED(hr))
	{
		hr = pProperties->GetValues(WPD_DEVICE_OBJECT_ID,         // The object whose properties we are reading
			pPropertiesToRead,   // The properties we want to read
			&pObjectProperties); // Driver supplied property values for the specified object
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get all properties for object '%ws', hr= 0x%lx"), WPD_DEVICE_OBJECT_ID, hr));
		}
	}
	//</SnippetContentProp2>
	// 6) Display the returned property values to the user
	if (SUCCEEDED(hr))
	{
		GetStringProperty(pObjectProperties,WPD_DEVICE_MODEL,pwszModelName);
		GetStringProperty(pObjectProperties,WPD_DEVICE_SERIAL_NUMBER,pwszSerialNumber);
	}
}

void GetStorageFreeAndTotalCapacity( IPortableDevice* pDevice,LPCTSTR wszObjectID,PULONGLONG pullFreeSize,PULONGLONG pullTotalSize )
{
	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));
		return;
	}

	HRESULT                               hr               = S_OK;
	CComPtr<IPortableDeviceProperties>    pProperties;
	CComPtr<IPortableDeviceValues>        pObjectProperties;
	CComPtr<IPortableDeviceContent>       pContent;
	CComPtr<IPortableDeviceKeyCollection> pPropertiesToRead;

	// 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pDevice->Content(&pContent);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 2) Get an IPortableDeviceProperties interface from the IPortableDeviceContent interface
	// to access the property-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pContent->Properties(&pProperties);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceProperties from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 3) CoCreate an IPortableDeviceKeyCollection interface to hold the the property keys
	// we wish to read.
	//<SnippetContentProp1>
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pPropertiesToRead));
	if (SUCCEEDED(hr))
	{
		// 4) Populate the IPortableDeviceKeyCollection with the keys we wish to read.
		// NOTE: We are not handling any special error cases here so we can proceed with
		// adding as many of the target properties as we can.
		if (pPropertiesToRead != NULL)
		{
			HRESULT hrTemp = S_OK;
			hrTemp = pPropertiesToRead->Add(WPD_STORAGE_CAPACITY);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_STORAGE_CAPACITY to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_STORAGE_FREE_SPACE_IN_BYTES);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_STORAGE_FREE_SPACE_IN_BYTES to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

		}
	}
	//</SnippetContentProp1>
	// 5) Call GetValues() passing the collection of specified PROPERTYKEYs.
	//<SnippetContentProp2>
	if (SUCCEEDED(hr))
	{
		hr = pProperties->GetValues(wszObjectID,         // The object whose properties we are reading
			pPropertiesToRead,   // The properties we want to read
			&pObjectProperties); // Driver supplied property values for the specified object
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get all properties for object '%ws', hr= 0x%lx"), wszObjectID, hr));
		}
	}
	//</SnippetContentProp2>
	// 6) Display the returned property values to the user
	if (SUCCEEDED(hr))
	{
		*pullFreeSize = GetUnsignedLargeIntergerProperty(pObjectProperties,WPD_STORAGE_FREE_SPACE_IN_BYTES);
		*pullTotalSize = GetUnsignedLargeIntergerProperty(pObjectProperties,WPD_STORAGE_CAPACITY);
	}
}

DWORD EnumStorageIDs( IPortableDevice* pDevice,LPWSTR *ppwszObjectIDs )
{
	HRESULT                               hr = S_OK;
	CComPtr<IPortableDeviceContent>       pContent;
	CComPtr<IEnumPortableDeviceObjectIDs> pEnumObjectIDs;

	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));
		return 0;
	}

	// Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	hr = pDevice->Content(&pContent);
	// Enumerate content starting from the "DEVICE" object.
	if (SUCCEEDED(hr))
	{
		// Get an IEnumPortableDeviceObjectIDs interface by calling EnumObjects with the
		// specified parent object identifier.
		hr = pContent->EnumObjects(0,               // Flags are unused
			WPD_DEVICE_OBJECT_ID,     // Starting from the passed in object
			NULL,            // Filter is unused
			&pEnumObjectIDs);
		if (SUCCEEDED(hr))
		{
			DWORD  cFetched = 0;
			PWSTR  szObjectIDArray[NUM_OBJECTS_TO_REQUEST] = {0};
			hr = pEnumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST,   // Number of objects to request on each NEXT call
				szObjectIDArray,          // Array of PWSTR array which will be populated on each NEXT call
				&cFetched);               // Number of objects written to the PWSTR array
			if (SUCCEEDED(hr))
			{
				// Traverse the results of the Next() operation and recursively enumerate
				// Remember to free all returned object identifiers using CoTaskMemFree()
				for (DWORD dwIndex = 0; dwIndex < cFetched; dwIndex++)
				{
// 					size_t len = wcslen(szObjectIDArray[dwIndex]) + 1;
// 					*ppwszObjectIDs[dwIndex] = new WCHAR[len];
// 					wcscpy_s(*ppwszObjectIDs[dwIndex],len,szObjectIDArray[dwIndex]);
// 
// 					// Free allocated PWSTRs after the recursive enumeration call has completed.
// 					CoTaskMemFree(szObjectIDArray[dwIndex]);
// 					szObjectIDArray[dwIndex] = NULL;

					ppwszObjectIDs[dwIndex] = szObjectIDArray[dwIndex];
				}

				return cFetched;
			}
		}
		else
		{
			DbgPrint((_T("! Failed to get IEnumPortableDeviceObjectIDs from IPortableDeviceContent, hr = 0x%lx"),hr));
		}
	}
	else
	{
		DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
	}

	return 0;
}

HRESULT ReadWpdFile( IStream *pStream,LPVOID lpBuffer,DWORD &dwNumberOfBytesToRead)
{
	HRESULT hr = S_OK;
	DWORD dwReaded = 0;
	hr = pStream->Read(lpBuffer,dwNumberOfBytesToRead,&dwReaded);

	if (SUCCEEDED(hr))
	{
		dwNumberOfBytesToRead = dwReaded;
	}
	
	return hr;
}

HRESULT WriteWpdFile( IStream *pStream,LPCVOID lpBuffer,DWORD &dwNumberOfBytesToWrite)
{
	HRESULT hr = S_OK;

	DWORD dwWritten = 0;
	hr = pStream->Write(lpBuffer,dwNumberOfBytesToWrite,&dwWritten);

	if (SUCCEEDED(hr))
	{
		dwNumberOfBytesToWrite = dwWritten;
	}
	
	return hr;
}

HRESULT DeleteWpdFile( IPortableDevice *pDevice,LPCTSTR wszObjectID ,DWORD dwRecursion /*= PORTABLE_DEVICE_DELETE_NO_RECURSION*/)
{
	HRESULT                                       hr               = S_OK;
	CComPtr<IPortableDeviceContent>               pContent;
	CComPtr<IPortableDevicePropVariantCollection> pObjectsToDelete;
	CComPtr<IPortableDevicePropVariantCollection> pObjectsFailedToDelete;

	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));
		hr = E_INVALIDARG;
		return hr;
	}

	// 1) get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pDevice->Content(&pContent);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 2) CoCreate an IPortableDevicePropVariantCollection interface to hold the the object identifiers
	// to delete.
	//
	// NOTE: This is a collection interface so more than 1 object can be deleted at a time.
	//       This sample only deletes a single object.
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pObjectsToDelete));
		if (SUCCEEDED(hr))
		{
			if (pObjectsToDelete != NULL)
			{
				PROPVARIANT pv = {0};
				PropVariantInit(&pv);

				// Initialize a PROPVARIANT structure with the object identifier string
				// that the user selected above. Notice we are allocating memory for the
				// PWSTR value.  This memory will be freed when PropVariantClear() is
				// called below.
				pv.vt      = VT_LPWSTR;
				pv.pwszVal = AtlAllocTaskWideString(wszObjectID);
				if (pv.pwszVal != NULL)
				{
					// Add the object identifier to the objects-to-delete list
					// (We are only deleting 1 in this example)
					hr = pObjectsToDelete->Add(&pv);
					if (SUCCEEDED(hr))
					{
						// Attempt to delete the object from the device
						hr = pContent->Delete(dwRecursion,  // Deleting with no recursion
							pObjectsToDelete,               // Object(s) to delete
							&pObjectsFailedToDelete);       // Object(s) that failed to delete (we are only deleting 1, so we can pass NULL here)
						if (SUCCEEDED(hr))
						{
							// An S_OK return lets the caller know that the deletion was successful
							if (hr == S_OK)
							{
								DbgPrint((_T("The object '%ws' was deleted from the device."), wszObjectID));
							}

							// An S_FALSE return lets the caller know that the deletion failed.
							// The caller should check the returned IPortableDevicePropVariantCollection
							// for a list of object identifiers that failed to be deleted.
							else
							{
								DbgPrint((_T("The object '%ws' failed to be deleted from the device."), wszObjectID));
							}
						}
						else
						{
							DbgPrint((_T("! Failed to delete an object from the device, hr = 0x%lx"),hr));
						}
					}
					else
					{
						DbgPrint((_T("! Failed to delete an object from the device because we could no add the object identifier string to the IPortableDevicePropVariantCollection, hr = 0x%lx"),hr));
					}
				}
				else
				{
					hr = E_OUTOFMEMORY;
					DbgPrint((_T("! Failed to delete an object from the device because we could no allocate memory for the object identifier string, hr = 0x%lx"),hr));
				}

				// Free any allocated values in the PROPVARIANT before exiting
				PropVariantClear(&pv);
			}
			else
			{
				DbgPrint((_T("! Failed to delete an object from the device because we were returned a NULL IPortableDevicePropVariantCollection interface pointer, hr = 0x%lx"),hr));
			}
		}
		else
		{
			DbgPrint((_T("! Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection, hr = 0x%lx"),hr));
		}
	}

	return hr;
}

HRESULT CreateFolder( IPortableDevice *pDevice,LPCWSTR pszParentObjectID,LPCWSTR pszFolderName,LPWSTR pwszNewObjectID )
{
	HRESULT                             hr = S_OK;
	CComPtr<IPortableDeviceValues>      pObjectProperties;
	CComPtr<IPortableDeviceContent>     pContent;

	pwszNewObjectID[0] = NULL;
	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));

		hr = E_INVALIDARG;
		return hr;
	}

	// 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.	
	hr = pDevice->Content(&pContent);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
	}

	// 2) Get the properties that describe the object being created on the device
	if (SUCCEEDED(hr))
	{
		// CoCreate an IPortableDeviceValues interface to hold the the object information
		hr = CoCreateInstance(CLSID_PortableDeviceValues,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pObjectProperties));
		if (SUCCEEDED(hr))
		{
			if (pObjectProperties != NULL)
			{
				// Set the WPD_OBJECT_PARENT_ID
				if (SUCCEEDED(hr))
				{
					hr = pObjectProperties->SetStringValue(WPD_OBJECT_PARENT_ID, pszParentObjectID);
					if (FAILED(hr))
					{
						DbgPrint((_T("! Failed to set WPD_OBJECT_PARENT_ID, hr = 0x%lx"),hr));
					}
				}

				// Set the WPD_OBJECT_NAME.
				if (SUCCEEDED(hr))
				{
					hr = pObjectProperties->SetStringValue(WPD_OBJECT_NAME, pszFolderName);
					if (FAILED(hr))
					{
						DbgPrint((_T("! Failed to set WPD_OBJECT_NAME, hr = 0x%lx"),hr));
					}
				}

				// Set the WPD_OBJECT_CONTENT_TYPE to WPD_CONTENT_TYPE_FOLDER because we are
				// creating contact content on the device.
				if (SUCCEEDED(hr))
				{
					hr = pObjectProperties->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FOLDER);
					if (FAILED(hr))
					{
						DbgPrint((_T("! Failed to set WPD_OBJECT_CONTENT_TYPE to WPD_CONTENT_TYPE_FOLDER, hr = 0x%lx"),hr));
					}
				}
			}
			else
			{
				hr = E_UNEXPECTED;
				DbgPrint((_T("! Failed to create property information because we were returned a NULL IPortableDeviceValues interface pointer, hr = 0x%lx"),hr));
			}

		}
		else
		{
			DbgPrint((_T("! Failed to CoCreateInstance CLSID_PortableDeviceValues, hr = 0x%lx"),hr));
		}
	}

	// 3) Transfer the content to the device by creating a properties-only object
	if (SUCCEEDED(hr))
	{
		PWSTR pszNewlyCreatedObject = NULL;
		hr = pContent->CreateObjectWithPropertiesOnly(pObjectProperties,    // Properties describing the object data
			&pszNewlyCreatedObject);
		if (SUCCEEDED(hr))
		{
			size_t len = wcslen(pszNewlyCreatedObject) + 1;

			wcscpy_s(pwszNewObjectID,len,pszNewlyCreatedObject);

			DbgPrint((_T("The folder was created on the device.The newly created object's ID is '%ws'"),pszNewlyCreatedObject));
		}
		else
		{
			DbgPrint((_T("! Failed to create a new folder on the device, hr = 0x%lx"),hr));
		}

		// Free the object identifier string returned from CreateObjectWithPropertiesOnly
		CoTaskMemFree(pszNewlyCreatedObject);
		pszNewlyCreatedObject = NULL;
	}

	return hr;
}

HRESULT GetObjectProperties( IPortableDevice *pDevice,LPCWSTR wszObjectID,PObjectProperties pObjectProperties )
{
	HRESULT                               hr               = S_OK;
	CComPtr<IPortableDeviceProperties>    pProperties;
	CComPtr<IPortableDeviceValues>        pObjectPropertieValues;
	CComPtr<IPortableDeviceContent>       pContent;
	CComPtr<IPortableDeviceKeyCollection> pPropertiesToRead;

	if (pDevice == NULL)
	{
		DbgPrint((_T("! A NULL IPortableDevice interface pointer was received")));

		hr = E_INVALIDARG;
		return hr;
	}

	// 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pDevice->Content(&pContent);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 2) Get an IPortableDeviceProperties interface from the IPortableDeviceContent interface
	// to access the property-specific methods.
	if (SUCCEEDED(hr))
	{
		hr = pContent->Properties(&pProperties);
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get IPortableDeviceProperties from IPortableDevice, hr = 0x%lx"),hr));
		}
	}

	// 3) CoCreate an IPortableDeviceKeyCollection interface to hold the the property keys
	// we wish to read.
	//<SnippetContentProp1>
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pPropertiesToRead));
	if (SUCCEEDED(hr))
	{
		// 4) Populate the IPortableDeviceKeyCollection with the keys we wish to read.
		// NOTE: We are not handling any special error cases here so we can proceed with
		// adding as many of the target properties as we can.
		if (pPropertiesToRead != NULL)
		{
			HRESULT hrTemp = S_OK;
			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_ID);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_ID to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_PARENT_ID);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_PARENT_ID to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_NAME);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_NAME to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_ORIGINAL_FILE_NAME);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_ORIGINAL_FILE_NAME to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_FORMAT);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_FORMAT to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_CONTENT_TYPE);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_CONTENT_TYPE to IPortableDeviceKeyCollection, hr= 0x%lx\n"), hrTemp));
			}


			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_SIZE);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_SIZE to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}
		}
	}
	//</SnippetContentProp1>
	// 5) Call GetValues() passing the collection of specified PROPERTYKEYs.
	//<SnippetContentProp2>
	if (SUCCEEDED(hr))
	{
		hr = pProperties->GetValues(wszObjectID,         // The object whose properties we are reading
			pPropertiesToRead,   // The properties we want to read
			&pObjectPropertieValues); // Driver supplied property values for the specified object
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get all properties for object '%ws', hr= 0x%lx"), wszObjectID, hr));
		}
	}
	//</SnippetContentProp2>
	// 6) Display the returned property values to the user
	if (SUCCEEDED(hr))
	{
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_ID,&pObjectProperties->pwszObjectID);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_PARENT_ID,&pObjectProperties->pwszObjectParentID);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_NAME,&pObjectProperties->pwszObjectName);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME,&pObjectProperties->pwszObjectOringalFileName);
		pObjectPropertieValues->GetGuidValue(WPD_OBJECT_FORMAT,&pObjectProperties->guidObjectFormat);
		pObjectPropertieValues->GetGuidValue(WPD_OBJECT_CONTENT_TYPE,&pObjectProperties->guidObjectContentType);
		pObjectPropertieValues->GetUnsignedLargeIntegerValue(WPD_OBJECT_PARENT_ID,&pObjectProperties->ullObjectSize);
	}

	return hr;
}

HRESULT GetObjectProperties( IPortableDeviceContent* pContent,LPCWSTR wszObjectID,PObjectProperties pObjectProperties )
{
	HRESULT                               hr               = S_OK;
	CComPtr<IPortableDeviceProperties>    pProperties;
	CComPtr<IPortableDeviceValues>        pObjectPropertieValues;
	CComPtr<IPortableDeviceKeyCollection> pPropertiesToRead;

	// 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
	// access the content-specific methods.

	// 2) Get an IPortableDeviceProperties interface from the IPortableDeviceContent interface
	// to access the property-specific methods.
	hr = pContent->Properties(&pProperties);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IPortableDeviceProperties from IPortableDevice, hr = 0x%lx"),hr));
	}

	// 3) CoCreate an IPortableDeviceKeyCollection interface to hold the the property keys
	// we wish to read.
	//<SnippetContentProp1>
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pPropertiesToRead));
	if (SUCCEEDED(hr))
	{
		// 4) Populate the IPortableDeviceKeyCollection with the keys we wish to read.
		// NOTE: We are not handling any special error cases here so we can proceed with
		// adding as many of the target properties as we can.
		if (pPropertiesToRead != NULL)
		{
			HRESULT hrTemp = S_OK;
			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_ID);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_ID to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_PARENT_ID);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_PARENT_ID to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_NAME);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_NAME to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_ORIGINAL_FILE_NAME);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_ORIGINAL_FILE_NAME to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_FORMAT);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_FORMAT to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}

			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_CONTENT_TYPE);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_CONTENT_TYPE to IPortableDeviceKeyCollection, hr= 0x%lx\n"), hrTemp));
			}


			hrTemp = pPropertiesToRead->Add(WPD_OBJECT_SIZE);
			if (FAILED(hrTemp))
			{
				DbgPrint((_T("! Failed to add WPD_OBJECT_SIZE to IPortableDeviceKeyCollection, hr= 0x%lx"), hrTemp));
			}
		}
	}
	//</SnippetContentProp1>
	// 5) Call GetValues() passing the collection of specified PROPERTYKEYs.
	//<SnippetContentProp2>
	if (SUCCEEDED(hr))
	{
		hr = pProperties->GetValues(wszObjectID,         // The object whose properties we are reading
			pPropertiesToRead,   // The properties we want to read
			&pObjectPropertieValues); // Driver supplied property values for the specified object
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to get all properties for object '%ws', hr= 0x%lx"), wszObjectID, hr));
		}
	}
	//</SnippetContentProp2>
	// 6) Display the returned property values to the user
	if (SUCCEEDED(hr))
	{
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_ID,&pObjectProperties->pwszObjectID);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_PARENT_ID,&pObjectProperties->pwszObjectParentID);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_NAME,&pObjectProperties->pwszObjectName);
		pObjectPropertieValues->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME,&pObjectProperties->pwszObjectOringalFileName);
		pObjectPropertieValues->GetGuidValue(WPD_OBJECT_FORMAT,&pObjectProperties->guidObjectFormat);
		pObjectPropertieValues->GetGuidValue(WPD_OBJECT_CONTENT_TYPE,&pObjectProperties->guidObjectContentType);
		pObjectPropertieValues->GetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE,&pObjectProperties->ullObjectSize);
	}

	return hr;
}

HRESULT CreateDataOutStream( IPortableDevice *pDevice, LPCWSTR pwszParentObjectID, PObjectProperties pObjectProperties, IPortableDeviceDataStream **ppDataStream, PDWORD pdwOptimalWriteBufferSize )
{
	HRESULT                             hr = S_OK;
	CComPtr<IPortableDeviceValues>      pObjectPropertiesValues;
	CComPtr<IPortableDeviceContent>     pContent;
	CComPtr<IStream>                    pTempStream;  // Temporary IStream which we use to QI for IPortableDeviceDataStream

	hr = pDevice->Content(&pContent);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));

		return hr;
	}

	hr = CoCreateInstance(CLSID_PortableDeviceValues,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pObjectPropertiesValues));

	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to CoCreateInstance CLSID_PortableDeviceValues, hr = 0x%lx"),hr));
		return hr;
	}

	if (pObjectProperties == NULL)
	{
		hr = E_UNEXPECTED;
		DbgPrint((_T("! Failed to create property information because we were returned a NULL IPortableDeviceValues interface pointer, hr = 0x%lx"),hr));

		return hr;
	}

	// 设置属性
	hr = pObjectPropertiesValues->SetStringValue(WPD_OBJECT_PARENT_ID, pwszParentObjectID);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_PARENT_ID, hr = 0x%lx"),hr));

		return hr;
	}

	hr = pObjectPropertiesValues->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, pObjectProperties->ullObjectSize);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_SIZE, hr = 0x%lx"),hr));

		return hr;
	}

	hr = pObjectPropertiesValues->SetStringValue(WPD_OBJECT_NAME, pObjectProperties->pwszObjectName);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_NAME, hr = 0x%lx"),hr));

		return hr;
	}

	hr = pObjectPropertiesValues->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, pObjectProperties->pwszObjectOringalFileName);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_ORIGINAL_FILE_NAME, hr = 0x%lx"),hr));

		return hr;
	}

	// 以下2个属性可以不设置
	hr = pObjectPropertiesValues->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, pObjectProperties->guidObjectContentType);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_CONTENT_TYPE, hr = 0x%lx"),hr));
	}

	hr = pObjectPropertiesValues->SetGuidValue(WPD_OBJECT_FORMAT, pObjectProperties->guidObjectFormat);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to set WPD_OBJECT_FORMAT, hr = 0x%lx"),hr));
	}

	hr = pContent->CreateObjectWithPropertiesAndData(pObjectPropertiesValues,    // Properties describing the object data
		&pTempStream,              // Returned object data stream (to transfer the data to)
		pdwOptimalWriteBufferSize,    // Returned optimal buffer size to use during transfer
		NULL);

	// Once we have a the IStream returned from CreateObjectWithPropertiesAndData,
	// QI for IPortableDeviceDataStream so we can use the additional methods
	// to get more information about the object (i.e. The newly created object
	// identifier on the device)
	if (SUCCEEDED(hr))
	{
		hr = pTempStream->QueryInterface(IID_PPV_ARGS(ppDataStream));
		if (FAILED(hr))
		{
			DbgPrint((_T("! Failed to QueryInterface for IPortableDeviceDataStream, hr = 0x%lx"),hr));
		}
	}
	else
	{
		DbgPrint((_T("! Failed to CreateObjectWithPropertiesAndData, hr = 0x%lx"),hr));
	}

	return hr;
}

HRESULT CreateDataInStream( IPortableDevice *pDevice,LPCWSTR lpszObjectID,IStream **ppDataStream ,PDWORD pdwOptimalWriteBufferSize)
{
	HRESULT                            hr                   = S_OK;
	CComPtr<IPortableDeviceContent>    pContent;
	CComPtr<IPortableDeviceResources>  pResources;

	hr = pDevice->Content(&pContent);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx"),hr));

		return hr;
	}

	hr = pContent->Transfer(&pResources);
	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IPortableDeviceResources from IPortableDeviceContent, hr = 0x%lx"),hr));

		return hr;
	}

	hr = pResources->GetStream(lpszObjectID,             // Identifier of the object we want to transfer
		WPD_RESOURCE_DEFAULT,    // We are transferring the default resource (which is the entire object's data)
		STGM_READ,               // Opening a stream in READ mode, because we are reading data from the device.
		pdwOptimalWriteBufferSize,  // Driver supplied optimal transfer size
		ppDataStream);

	if (FAILED(hr))
	{
		DbgPrint((_T("! Failed to get IStream (representing object data on the device) from IPortableDeviceResources, hr = 0x%lx"),hr));
		return hr;
	}

	if (ppDataStream == NULL)
	{
		hr = E_UNEXPECTED;
		DbgPrint((_T("! Failed to get IStream from IPortableDeviceResources,because we were returned a NULL IStream interface pointer, hr = 0x%lx"),hr));
	}

	return hr;
}
