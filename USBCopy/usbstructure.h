#ifndef _USB_STRUCTURE_H_H
#define _USB_STRUCTURE_H_H

#include <usb100.h>
//
// Structure used to build a linked list of String Descriptors
// retrieved from a device.
//

typedef struct _STRING_DESCRIPTOR_NODE
{
	struct _STRING_DESCRIPTOR_NODE *Next;
	UCHAR                           DescriptorIndex;
	USHORT                          LanguageID;
	USB_STRING_DESCRIPTOR           StringDescriptor[0];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;


//
// Structures assocated with TreeView items through the lParam.  When an item
// is selected, the lParam is retrieved and the structure it which it points
// is used to display information in the edit control.
//

typedef enum _USBDEVICEINFOTYPE
{
	HostControllerInfo,

	RootHubInfo,

	ExternalHubInfo,

	DeviceInfo

} USBDEVICEINFOTYPE, *PUSBDEVICEINFOTYPE;


typedef struct _USBHOSTCONTROLLERINFO
{
	USBDEVICEINFOTYPE                   DeviceInfoType;

	LIST_ENTRY                          ListEntry;

	PTSTR                               DriverKey;

	ULONG                               VendorID;

	ULONG                               DeviceID;

	ULONG                               SubSysID;

	ULONG                               Revision;

} USBHOSTCONTROLLERINFO, *PUSBHOSTCONTROLLERINFO;


typedef struct _USBROOTHUBINFO
{
	USBDEVICEINFOTYPE                   DeviceInfoType;

	PUSB_NODE_INFORMATION               HubInfo;

	PTSTR                               HubName;

	PUSB_HUB_CAPABILITIES               HubCaps;

	PUSB_HUB_CAPABILITIES_EX            HubCapsEx;

} USBROOTHUBINFO, *PUSBROOTHUBINFO;


typedef struct _USBEXTERNALHUBINFO
{
	USBDEVICEINFOTYPE                   DeviceInfoType;

	PUSB_NODE_INFORMATION               HubInfo;

	PTSTR                               HubName;

	PUSB_HUB_CAPABILITIES               HubCaps;

	PUSB_HUB_CAPABILITIES_EX            HubCapsEx;

	PUSB_NODE_CONNECTION_INFORMATION_EX ConnectionInfo;

	PUSB_DESCRIPTOR_REQUEST             ConfigDesc;

	PSTRING_DESCRIPTOR_NODE             StringDescs;

} USBEXTERNALHUBINFO, *PUSBEXTERNALHUBINFO;


typedef struct _USBDEVICEINFO
{
	USBDEVICEINFOTYPE                   DeviceInfoType;

	PUSB_NODE_CONNECTION_INFORMATION_EX ConnectionInfo;

	PUSB_DESCRIPTOR_REQUEST             ConfigDesc;

	PSTRING_DESCRIPTOR_NODE             StringDescs;

	//2010.09.20 Ìí¼ÓDeviceDescºÍDeviceID
	PTSTR		DeviceDesc;
	PTSTR       DeviceID;

} USBDEVICEINFO, *PUSBDEVICEINFO;

#endif