
/*****************************************************************************
Copyright (c) Phiyo Technology 
Module Name:  
DeviceEnum.cpp
Created on:
2011-9-9
Author:
Administrator
*****************************************************************************/

/******************************************************************************
* include files
******************************************************************************/
#include "stdafx.h"

#include <initguid.h>
#include <winioctl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#pragma comment(lib,"Setupapi.lib")

#include "EnumUSB.h"


#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4200) // named type definition in parentheses
#pragma warning(disable:4213) // named type definition in parentheses
#pragma warning(disable:4267) // named type definition in parentheses


PTSTR GetDriverKeyName (
	HANDLE  Hub,
	ULONG   ConnectionIndex
	);

PUSB_DESCRIPTOR_REQUEST
	GetConfigDescriptor (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex
	);

BOOL
	AreThereStringDescriptors (
	PUSB_DEVICE_DESCRIPTOR          DeviceDesc,
	PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
	);

PSTRING_DESCRIPTOR_NODE
	GetAllStringDescriptors (
	HANDLE                          hHubDevice,
	ULONG                           ConnectionIndex,
	PUSB_DEVICE_DESCRIPTOR          DeviceDesc,
	PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
	);

PSTRING_DESCRIPTOR_NODE
	GetStringDescriptor (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex,
	USHORT  LanguageID
	);

PSTRING_DESCRIPTOR_NODE
	GetStringDescriptors (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex,
	ULONG   NumLanguageIDs,
	USHORT  *LanguageIDs,
	PSTRING_DESCRIPTOR_NODE StringDescNodeTail
	);

PTSTR 
	DriverKeyNameToDeviceDesc (
	__in PCTSTR DriverName, 
	BOOLEAN DeviceId
	);


//*****************************************************************************
//
// WideStrToMultiStr()
//
//*****************************************************************************

PTSTR WideStrToMultiStr (__in LPCWSTR WideStr)
{
	// Is there a better way to do this?
#if defined(_UNICODE) //  If this is built for UNICODE, just clone the input
	ULONG nChars;
	PTSTR RetStr;

	nChars = wcslen(WideStr) + 1;
	RetStr = (PTSTR)ALLOC(nChars * sizeof(TCHAR));
	if (RetStr == NULL)
	{
		return NULL;
	}
	_tcscpy_s(RetStr, nChars, WideStr);
	return RetStr;


#else //  convert
	ULONG nBytes;
	PTSTR MultiStr;

	// Get the length of the converted string
	//
	nBytes = WideCharToMultiByte(
		CP_ACP,
		0,
		WideStr,
		-1,
		NULL,
		0,
		NULL,
		NULL);

	if (nBytes == 0)
	{
		return NULL;
	}

	// Allocate space to hold the converted string
	//
	MultiStr = (PTSTR)ALLOC(nBytes);

	if (MultiStr == NULL)
	{
		return NULL;
	}

	// Convert the string
	//
	nBytes = WideCharToMultiByte(
		CP_ACP,
		0,
		WideStr,
		-1,
		MultiStr,
		nBytes,
		NULL,
		NULL);

	if (nBytes == 0)
	{
		FREE(MultiStr);
		MultiStr = NULL;
		return NULL;
	}
	return MultiStr;
#endif

}

//*****************************************************************************
//
// GetDriverKeyName()
//
//*****************************************************************************

PTSTR GetDriverKeyName (
	HANDLE  Hub,
	ULONG   ConnectionIndex
	)
{
	BOOL                                success;
	ULONG                               nBytes;
	USB_NODE_CONNECTION_DRIVERKEY_NAME  driverKeyName;
	PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyNameW;
	PTSTR                               driverKeyNameA;

	driverKeyNameW = NULL;
	driverKeyNameA = NULL;

	// Get the length of the name of the driver key of the device attached to
	// the specified port.
	//
	driverKeyName.ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(Hub,
		IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
		&driverKeyName,
		sizeof(driverKeyName),
		&driverKeyName,
		sizeof(driverKeyName),
		&nBytes,
		NULL);

	if (!success)
	{
		OOPS();
		goto GetDriverKeyNameError;
	}

	// Allocate space to hold the driver key name
	//
	nBytes = driverKeyName.ActualLength;

	if (nBytes <= sizeof(driverKeyName))
	{
		OOPS();
		goto GetDriverKeyNameError;
	}

	driverKeyNameW = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)ALLOC(nBytes);

	if (driverKeyNameW == NULL)
	{
		OOPS();
		goto GetDriverKeyNameError;
	}

	// Get the name of the driver key of the device attached to
	// the specified port.
	//
	driverKeyNameW->ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(Hub,
		IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
		driverKeyNameW,
		nBytes,
		driverKeyNameW,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		OOPS();
		goto GetDriverKeyNameError;
	}

	// Convert the driver key name
	//
	driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName);

	// All done, free the uncoverted driver key name and return the
	// converted driver key name
	//
	FREE(driverKeyNameW);
	driverKeyNameW = NULL;

	return driverKeyNameA;


GetDriverKeyNameError:
	// There was an error, free anything that was allocated
	//
	if (driverKeyNameW != NULL)
	{
		FREE(driverKeyNameW);
		driverKeyNameW = NULL;
	}

	return NULL;
}

//*****************************************************************************
//
// GetConfigDescriptor()
//
// hHubDevice - Handle of the hub device containing the port from which the
// Configuration Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the Configuration Descriptor will be requested.
//
// DescriptorIndex - Configuration Descriptor index, zero based.
//
//*****************************************************************************

PUSB_DESCRIPTOR_REQUEST
	GetConfigDescriptor (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex
	)
{
	BOOL    success;
	ULONG   nBytes;
	ULONG   nBytesReturned;

	UCHAR   configDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) +
		sizeof(USB_CONFIGURATION_DESCRIPTOR)];

	PUSB_DESCRIPTOR_REQUEST         configDescReq;
	PUSB_CONFIGURATION_DESCRIPTOR   configDesc;


	// Request the Configuration Descriptor the first time using our
	// local buffer, which is just big enough for the Cofiguration
	// Descriptor itself.
	//
	nBytes = sizeof(configDescReqBuf);

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)configDescReqBuf;
	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Zero fill the entire request structure
	//
	memset(configDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
		| DescriptorIndex;

	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		configDescReq,
		nBytes,
		configDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	if (!success)
	{
		OOPS();
		return NULL;
	}

	if (nBytes != nBytesReturned)
	{
		OOPS();
		return NULL;
	}

	if (configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
	{
		OOPS();
		return NULL;
	}

	// Now request the entire Configuration Descriptor using a dynamically
	// allocated buffer which is sized big enough to hold the entire descriptor
	//
	nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + configDesc->wTotalLength;

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)ALLOC(nBytes);

	if (configDescReq == NULL)
	{
		OOPS();
		return NULL;
	}

	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
		| DescriptorIndex;

	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		configDescReq,
		nBytes,
		configDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	if (!success)
	{
		OOPS();
		FREE(configDescReq);
		configDescReq = NULL;
		return NULL;
	}

	if (nBytes != nBytesReturned)
	{
		OOPS();
		FREE(configDescReq);
		configDescReq = NULL;
		return NULL;
	}

	if (configDesc->wTotalLength != (nBytes - sizeof(USB_DESCRIPTOR_REQUEST)))
	{
		OOPS();
		FREE(configDescReq);
		configDescReq = NULL;
		return NULL;
	}

	return configDescReq;
}


//*****************************************************************************
//
// AreThereStringDescriptors()
//
// DeviceDesc - Device Descriptor for which String Descriptors should be
// checked.
//
// ConfigDesc - Configuration Descriptor (also containing Interface Descriptor)
// for which String Descriptors should be checked.
//
//*****************************************************************************

BOOL
	AreThereStringDescriptors (
	PUSB_DEVICE_DESCRIPTOR          DeviceDesc,
	PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
	)
{
	PUCHAR                  descEnd;
	PUSB_COMMON_DESCRIPTOR  commonDesc;

	//
	// Check Device Descriptor strings
	//

	if (DeviceDesc->iManufacturer ||
		DeviceDesc->iProduct      ||
		DeviceDesc->iSerialNumber
		)
	{
		return TRUE;
	}


	//
	// Check the Configuration and Interface Descriptor strings
	//

	descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

	commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

	while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
		(PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
	{
		switch (commonDesc->bDescriptorType)
		{
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
			{
				OOPS();
				break;
			}
			if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration)
			{
				return TRUE;
			}
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;

		case USB_INTERFACE_DESCRIPTOR_TYPE:
			if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
				commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR2))
			{
				OOPS();
				break;
			}
			if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface)
			{
				return TRUE;
			}
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;

		default:
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;
		}
		break;
	}

	return FALSE;
}


//*****************************************************************************
//
// GetAllStringDescriptors()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptors will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptors will be requested.
//
// DeviceDesc - Device Descriptor for which String Descriptors should be
// requested.
//
// ConfigDesc - Configuration Descriptor (also containing Interface Descriptor)
// for which String Descriptors should be requested.
//
//*****************************************************************************

PSTRING_DESCRIPTOR_NODE
	GetAllStringDescriptors (
	HANDLE                          hHubDevice,
	ULONG                           ConnectionIndex,
	PUSB_DEVICE_DESCRIPTOR          DeviceDesc,
	PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
	)
{
	PSTRING_DESCRIPTOR_NODE supportedLanguagesString;
	PSTRING_DESCRIPTOR_NODE stringDescNodeTail;
	ULONG                   numLanguageIDs;
	USHORT                  *languageIDs;

	PUCHAR                  descEnd;
	PUSB_COMMON_DESCRIPTOR  commonDesc;

	//
	// Get the array of supported Language IDs, which is returned
	// in String Descriptor 0
	//
	supportedLanguagesString = GetStringDescriptor(hHubDevice,
		ConnectionIndex,
		0,
		0);

	if (supportedLanguagesString == NULL)
	{
		return NULL;
	}

	numLanguageIDs = (supportedLanguagesString->StringDescriptor->bLength - 2) / 2;

	//languageIDs = &supportedLanguagesString->StringDescriptor->bString[0];
	languageIDs = (USHORT *)&supportedLanguagesString->StringDescriptor->bString[0];

	stringDescNodeTail = supportedLanguagesString;

	//
	// Get the Device Descriptor strings
	//

	if (DeviceDesc->iManufacturer)
	{
		stringDescNodeTail = GetStringDescriptors(hHubDevice,
			ConnectionIndex,
			DeviceDesc->iManufacturer,
			numLanguageIDs,
			languageIDs,
			stringDescNodeTail);
	}

	if (DeviceDesc->iProduct)
	{
		stringDescNodeTail = GetStringDescriptors(hHubDevice,
			ConnectionIndex,
			DeviceDesc->iProduct,
			numLanguageIDs,
			languageIDs,
			stringDescNodeTail);
	}

	if (DeviceDesc->iSerialNumber)
	{
		stringDescNodeTail = GetStringDescriptors(hHubDevice,
			ConnectionIndex,
			DeviceDesc->iSerialNumber,
			numLanguageIDs,
			languageIDs,
			stringDescNodeTail);
	}


	//
	// Get the Configuration and Interface Descriptor strings
	//

	descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

	commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

	while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
		(PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
	{
		switch (commonDesc->bDescriptorType)
		{
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
			if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
			{
				OOPS();
				break;
			}
			if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration)
			{
				stringDescNodeTail = GetStringDescriptors(
					hHubDevice,
					ConnectionIndex,
					((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration,
					numLanguageIDs,
					languageIDs,
					stringDescNodeTail);
			}
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;

		case USB_INTERFACE_DESCRIPTOR_TYPE:
			if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
				commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR2))
			{
				OOPS();
				break;
			}
			if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface)
			{
				stringDescNodeTail = GetStringDescriptors(
					hHubDevice,
					ConnectionIndex,
					((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface,
					numLanguageIDs,
					languageIDs,
					stringDescNodeTail);
			}
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;

		default:
			//(PUCHAR)commonDesc += commonDesc->bLength;
			commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
			continue;
		}
		break;
	}

	return supportedLanguagesString;
}


//*****************************************************************************
//
// GetStringDescriptor()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptor will be requested.
//
// DescriptorIndex - String Descriptor index.
//
// LanguageID - Language in which the string should be requested.
//
//*****************************************************************************

PSTRING_DESCRIPTOR_NODE
	GetStringDescriptor (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex,
	USHORT  LanguageID
	)
{
	BOOL    success;
	ULONG   nBytes;
	ULONG   nBytesReturned;

	UCHAR   stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) +
		MAXIMUM_USB_STRING_LENGTH];

	PUSB_DESCRIPTOR_REQUEST stringDescReq;
	PUSB_STRING_DESCRIPTOR  stringDesc;
	PSTRING_DESCRIPTOR_NODE stringDescNode;

	nBytes = sizeof(stringDescReqBuf);

	stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
	stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq+1);

	// Zero fill the entire request structure
	//
	memset(stringDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	stringDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8)
		| DescriptorIndex;

	stringDescReq->SetupPacket.wIndex = LanguageID;

	stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		stringDescReq,
		nBytes,
		stringDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	//
	// Do some sanity checks on the return from the get descriptor request.
	//

	if (!success)
	{
		OOPS();
		return NULL;
	}

	if (nBytesReturned < 2)
	{
		OOPS();
		return NULL;
	}

	if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
	{
		OOPS();
		return NULL;
	}

	if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
	{
		OOPS();
		return NULL;
	}

	if (stringDesc->bLength % 2 != 0)
	{
		OOPS();
		return NULL;
	}

	//
	// Looks good, allocate some (zero filled) space for the string descriptor
	// node and copy the string descriptor to it.
	//

	stringDescNode = (PSTRING_DESCRIPTOR_NODE)ALLOC(sizeof(STRING_DESCRIPTOR_NODE) +
		stringDesc->bLength);

	if (stringDescNode == NULL)
	{
		OOPS();
		return NULL;
	}

	stringDescNode->DescriptorIndex = DescriptorIndex;
	stringDescNode->LanguageID = LanguageID;

	memcpy(stringDescNode->StringDescriptor,
		stringDesc,
		stringDesc->bLength);

	return stringDescNode;
}


//*****************************************************************************
//
// GetStringDescriptors()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptor will be requested.
//
// DescriptorIndex - String Descriptor index.
//
// NumLanguageIDs -  Number of languages in which the string should be
// requested.
//
// LanguageIDs - Languages in which the string should be requested.
//
//*****************************************************************************

PSTRING_DESCRIPTOR_NODE
	GetStringDescriptors (
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex,
	UCHAR   DescriptorIndex,
	ULONG   NumLanguageIDs,
	USHORT  *LanguageIDs,
	PSTRING_DESCRIPTOR_NODE StringDescNodeTail
	)
{
	ULONG i;

	for (i=0; i<NumLanguageIDs; i++)
	{
		StringDescNodeTail->Next = GetStringDescriptor(hHubDevice,
			ConnectionIndex,
			DescriptorIndex,
			*LanguageIDs);

		if (StringDescNodeTail->Next)
		{
			StringDescNodeTail = StringDescNodeTail->Next;
		}

		LanguageIDs++;
	}

	return StringDescNodeTail;
}


//*****************************************************************************
//
// DriverKeyNameToDeviceDesc()
//
// Returns the Device Description of the DevNode with the matching DriverName.
// Returns NULL if the matching DevNode is not found.
//
// The caller should copy the returned string buffer instead of just saving
// the pointer value.  (Dynamically allocate return buffer instead?)
//
//*****************************************************************************

PTSTR DriverKeyNameToDeviceDesc (__in PCTSTR DriverName, BOOLEAN DeviceId)
{
	DEVINST     devInst;
	DEVINST     devInstNext;
	CONFIGRET   cr;
	ULONG       walkDone = 0;
	ULONG       len;

	//static TCHAR buf[MAX_DEVICE_ID_LEN];  // (Dynamically size this instead?)
	PTSTR		buf;
	// Get Root DevNode
	//
	cr = CM_Locate_DevNode(&devInst,
		NULL,
		0);

	if (cr != CR_SUCCESS)
	{
		return NULL;
	}

	//alloc a memory space to hold the request data
	buf = (PTSTR)ALLOC(MAX_DEVICE_ID_LEN);
	if(buf == NULL)
	{
		return NULL;
	}

	// Do a depth first search for the DevNode with a matching
	// DriverName value
	//
	while (!walkDone)
	{
		// Get the DriverName value
		//
		//len = sizeof(buf) / sizeof(buf[0]);
		len = MAX_DEVICE_ID_LEN;
		cr = CM_Get_DevNode_Registry_Property(devInst,
			CM_DRP_DRIVER,
			NULL,
			buf,
			&len,
			0);

		// If the DriverName value matches, return the DeviceDescription
		//
		if (cr == CR_SUCCESS && _tcsicmp(DriverName, buf) == 0)
		{
			//len = sizeof(buf) / sizeof(buf[0]);
			len = MAX_DEVICE_ID_LEN;

			if (DeviceId)
			{
				cr = CM_Get_Device_ID(devInst,
					buf,
					len,
					0);
			}
			else
			{
				cr = CM_Get_DevNode_Registry_Property(devInst,
					CM_DRP_DEVICEDESC,
					NULL,
					buf,
					&len,
					0);
			}

			if (cr == CR_SUCCESS)
			{
				return buf;
			}
			else
			{
				FREE(buf);
				buf = NULL;
				return NULL;
			}
		}

		// This DevNode didn't match, go down a level to the first child.
		//
		cr = CM_Get_Child(&devInstNext,
			devInst,
			0);

		if (cr == CR_SUCCESS)
		{
			devInst = devInstNext;
			continue;
		}

		// Can't go down any further, go across to the next sibling.  If
		// there are no more siblings, go back up until there is a sibling.
		// If we can't go up any further, we're back at the root and we're
		// done.
		//
		for (;;)
		{
			cr = CM_Get_Sibling(&devInstNext,
				devInst,
				0);

			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
				break;
			}

			cr = CM_Get_Parent(&devInstNext,
				devInst,
				0);


			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
			}
			else
			{
				walkDone = 1;
				break;
			}
		}
	}

	//2011.09.20 如果buf已申请到空间，需要销毁
	if(buf)
	{
		FREE(buf);
		buf = NULL;
	}

	return NULL;
}

/******************************************************************************
* GetHubPortDeviceInfo
*  获取连接到Hub端口的设备信息
*      PTSTR pszRootHubDevPath     RootHub 设备路径
*      ULONG ConnectionIndex       连接端口索引
******************************************************************************/
PUSBDEVICEINFO
	GetHubPortDeviceInfo(PTSTR pszRootHubDevPath,
	ULONG ConnectionIndex)
{
	/******************************************************************************
	* 枚举RootHub设备，获得RootHub设备句柄以及Root设备信息
	******************************************************************************/
	HANDLE      hHubDevice;
	BOOL        success;

	PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = NULL;
	PUSB_NODE_CONNECTION_INFORMATION_EX connectionInfoEx  = NULL;
	PUSB_DESCRIPTOR_REQUEST             configDesc  = NULL;
	PSTRING_DESCRIPTOR_NODE             stringDescs  = NULL;
	PUSBDEVICEINFO                      info  = NULL;

	PTSTR       driverKeyName   = NULL;
	PTSTR       deviceDesc      = NULL;
	PTSTR       deviceId        = NULL;
	ULONG       nBytes;
	ULONG       nBytesEx;


	// Try to hub the open device
	hHubDevice = CreateFile(pszRootHubDevPath,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hHubDevice == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(_T("CreateFile error"));
		OutputDebugString(pszRootHubDevPath);
		return NULL;
	}

	// Allocate space to hold the connection info for this port.
	// For now, allocate it big enough to hold info for 30 pipes.
	//
	// Endpoint numbers are 0-15.  Endpoint number 0 is the standard
	// control endpoint which is not explicitly listed in the Configuration
	// Descriptor.  There can be an IN endpoint and an OUT endpoint at
	// endpoint numbers 1-15 so there can be a maximum of 30 endpoints
	// per device configuration.
	//
	// Should probably size this dynamically at some point.
	//
	nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) +
		sizeof(USB_PIPE_INFO) * 30;

	connectionInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)ALLOC(nBytesEx);

	if (connectionInfoEx == NULL)
	{
		goto EnumHubPortError;
	}

	//
	// Now query USBHUB for the USB_NODE_CONNECTION_INFORMATION_EX structure
	// for this port.  This will tell us if a device is attached to this
	// port, among other things.
	//
	connectionInfoEx->ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
		connectionInfoEx,
		nBytesEx,
		connectionInfoEx,
		nBytesEx,
		&nBytesEx,
		NULL);
	if (!success)
	{
		OutputDebugString(_T("DeviceIoControl(IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX)"));

		// Try using IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
		// instead of IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
		//
		nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
			sizeof(USB_PIPE_INFO) * 30;


		connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);

		connectionInfo->ConnectionIndex = ConnectionIndex;

		success = DeviceIoControl(hHubDevice,
			IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
			connectionInfo,
			nBytes,
			connectionInfo,
			nBytes,
			&nBytes,
			NULL);

		if (!success)
		{
			OutputDebugString(_T("DeviceIoControl(IOCTL_USB_GET_NODE_CONNECTION_INFORMATION)"));

			goto EnumHubPortError;
		}

		// Copy IOCTL_USB_GET_NODE_CONNECTION_INFORMATION into
		// IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX structure.
		//
		connectionInfoEx->ConnectionIndex = connectionInfo->ConnectionIndex;
		connectionInfoEx->DeviceDescriptor = connectionInfo->DeviceDescriptor;
		connectionInfoEx->CurrentConfigurationValue = connectionInfo->CurrentConfigurationValue;
		connectionInfoEx->Speed = connectionInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;
		connectionInfoEx->DeviceIsHub = connectionInfo->DeviceIsHub;
		connectionInfoEx->DeviceAddress = connectionInfo->DeviceAddress;
		connectionInfoEx->NumberOfOpenPipes = connectionInfo->NumberOfOpenPipes;
		connectionInfoEx->ConnectionStatus = connectionInfo->ConnectionStatus;

		memcpy(&connectionInfoEx->PipeList[0],
			&connectionInfo->PipeList[0],
			sizeof(USB_PIPE_INFO) * 30);

		FREE(connectionInfo);
		connectionInfo = NULL;
	}

	// If there is a device connected, get the Device Description
	//
	deviceDesc = NULL;
	if (connectionInfoEx->ConnectionStatus != NoDeviceConnected)
	{
		driverKeyName = GetDriverKeyName(hHubDevice,
			ConnectionIndex);

		if (driverKeyName)
		{
			deviceDesc = DriverKeyNameToDeviceDesc(driverKeyName, FALSE);
			deviceId = DriverKeyNameToDeviceDesc(driverKeyName, TRUE);

			FREE(driverKeyName);
			driverKeyName = NULL;
		}
	}
	else
	{
		// No Device Connected
		goto EnumHubPortError;
	}

	// If there is a device connected to the port, try to retrieve the
	// Configuration Descriptor from the device.
	//
	if (connectionInfoEx->ConnectionStatus == DeviceConnected)
	{
		configDesc = GetConfigDescriptor(hHubDevice,
			ConnectionIndex,
			0);
	}
	else
	{
		configDesc = NULL;
	}

	if (configDesc != NULL && AreThereStringDescriptors(&connectionInfoEx->DeviceDescriptor,
		(PUSB_CONFIGURATION_DESCRIPTOR)(configDesc + 1)))
	{
		stringDescs = GetAllStringDescriptors(hHubDevice,
			ConnectionIndex,
			&connectionInfoEx->DeviceDescriptor,
			(PUSB_CONFIGURATION_DESCRIPTOR)(configDesc + 1));
	}
	else
	{
		stringDescs = NULL;
	}


	info = (PUSBDEVICEINFO) ALLOC(sizeof(USBDEVICEINFO));

	if (info == NULL)
	{
		goto EnumHubPortError;
	}

	info->DeviceInfoType = DeviceInfo;
	info->ConnectionInfo = connectionInfoEx;
	info->ConfigDesc = configDesc;
	info->StringDescs = stringDescs;
	//2011.09.20 USBDEVICEINFO 结构体中增加了 DeviceDesc和DeviceID
	info->DeviceDesc = deviceDesc;
	info->DeviceID = deviceId;

	if (hHubDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hHubDevice);
	}
	return info;

EnumHubPortError:

	FREE(connectionInfoEx);
	connectionInfoEx = NULL;

	if (configDesc)
	{
		FREE(configDesc);
		configDesc = NULL;
	}

	if (stringDescs != NULL)
	{
		PSTRING_DESCRIPTOR_NODE Next;

		do
		{

			Next = stringDescs->Next;
			FREE(stringDescs);
			stringDescs = Next;

		}
		while (stringDescs != NULL);
	}

	//2011.09.20 deviceDesc,deviceId 得到的是动态内存，需要销毁
	if(deviceDesc)
	{
		FREE(deviceDesc);
		deviceDesc = NULL;
	}

	if(deviceId)
	{
		FREE(deviceId);
		deviceId = NULL;
	}

	if (hHubDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hHubDevice);
	}
	return NULL;

}
//*****************************************************************************
//
// CleanupInfo
//
//*****************************************************************************

VOID
	CleanupInfo(PVOID info)
{

	if (info)
	{
		PTSTR DriverKey = NULL;
		PUSB_NODE_INFORMATION HubInfo = NULL;
		PUSB_HUB_CAPABILITIES HubCaps = NULL;
		PUSB_HUB_CAPABILITIES_EX HubCapsEx = NULL;
		PTSTR HubName = NULL;
		PUSB_NODE_CONNECTION_INFORMATION_EX ConnectionInfoEx = NULL;
		PUSB_DESCRIPTOR_REQUEST ConfigDesc = NULL;
		PSTRING_DESCRIPTOR_NODE StringDescs = NULL;

		//2011.09.20 DeviceInfo中增加了DeviceDesc 和 DeviceID
		PTSTR DeviceDesc = NULL;
		PTSTR DeviceID = NULL;

		ConnectionInfoEx = ((PUSBDEVICEINFO) info)->ConnectionInfo;
		ConfigDesc = ((PUSBDEVICEINFO) info)->ConfigDesc;
		StringDescs = ((PUSBDEVICEINFO) info)->StringDescs;

		//2011.09.20 DeviceInfo中增加了DeviceDesc 和 DeviceID
		DeviceDesc = ((PUSBDEVICEINFO) info)->DeviceDesc;
		DeviceID = ((PUSBDEVICEINFO) info)->DeviceID;

		if (ConfigDesc)
		{
			FREE(ConfigDesc);
			ConfigDesc = NULL;
		}

		if (StringDescs)
		{
			PSTRING_DESCRIPTOR_NODE Next;

			do
			{

				Next = StringDescs->Next;
				FREE(StringDescs);
				StringDescs = Next;

			}
			while (StringDescs);
		}

		if (ConnectionInfoEx)
		{
			FREE(ConnectionInfoEx);
			ConnectionInfoEx = NULL;
		}

		////2011.09.20 DeviceInfo中增加了DeviceDesc 和 DeviceID,需要销毁
		if (DeviceDesc)
		{
			FREE(DeviceDesc);
			DeviceDesc = NULL;
		}

		if (DeviceID)
		{
			FREE(DeviceID);
			DeviceID = NULL;
		}

		FREE(info);
		info = NULL;
	}
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
