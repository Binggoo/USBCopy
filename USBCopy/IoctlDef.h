#ifndef _IOCTL_DEF_H_
#define _IOCTL_DEF_H_

#include <WinIoCtl.h>
#include <ntddscsi.h>

#define SPT_SENSE_LENGTH 32
#define SPTWB_DATA_LENGTH 512

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
	SCSI_PASS_THROUGH spt;
	ULONG             Filler;      // realign buffers to double word boundary
	UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
	UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
	SCSI_PASS_THROUGH_DIRECT sptd;
	ULONG             Filler;      // realign buffer to double word boundary
	UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

#pragma pack(push,1)
struct IDENTIFY_DEVICE
{
	WORD		GeneralConfiguration;					//0
	WORD		LogicalCylinders;						//1	Obsolete
	WORD		SpecificConfiguration;					//2
	WORD		LogicalHeads;							//3 Obsolete
	WORD		Retired1[2];							//4-5
	WORD		LogicalSectors;							//6 Obsolete
	DWORD		ReservedForCompactFlash;				//7-8
	WORD		Retired2;								//9
	CHAR		SerialNumber[20];						//10-19
	WORD		Retired3;								//20
	WORD		BufferSize;								//21 Obsolete
	WORD		Obsolute4;								//22
	CHAR		FirmwareRev[8];							//23-26
	CHAR		Model[40];								//27-46
	WORD		MaxNumPerInterupt;						//47
	WORD		Reserved1;								//48
	WORD		Capabilities1;							//49
	WORD		Capabilities2;							//50
	DWORD		Obsolute5;								//51-52
	WORD		Field88and7064;							//53
	WORD		Obsolute6[5];							//54-58
	WORD		MultSectorStuff;						//59
	DWORD		TotalAddressableSectors;				//60-61
	WORD		Obsolute7;								//62
	WORD		MultiWordDma;							//63
	WORD		PioMode;								//64
	WORD		MinMultiwordDmaCycleTime;				//65
	WORD		RecommendedMultiwordDmaCycleTime;		//66
	WORD		MinPioCycleTimewoFlowCtrl;				//67
	WORD		MinPioCycleTimeWithFlowCtrl;			//68
	WORD		Reserved2[6];							//69-74
	WORD		QueueDepth;								//75
	WORD		SerialAtaCapabilities;					//76
	WORD		SerialAtaAdditionalCapabilities;		//77
	WORD		SerialAtaFeaturesSupported;				//78
	WORD		SerialAtaFeaturesEnabled;				//79
	WORD		MajorVersion;							//80
	WORD		MinorVersion;							//81
	WORD		CommandSetSupported1;					//82
	WORD		CommandSetSupported2;					//83
	WORD		CommandSetSupported3;					//84
	WORD		CommandSetEnabled1;						//85
	WORD		CommandSetEnabled2;						//86
	WORD		CommandSetDefault;						//87
	WORD		UltraDmaMode;							//88
	WORD		TimeReqForSecurityErase;				//89
	WORD		TimeReqForEnhancedSecure;				//90
	WORD		CurrentPowerManagement;					//91
	WORD		MasterPasswordRevision;					//92
	WORD		HardwareResetResult;					//93
	WORD		AcoustricManagement;					//94
	WORD		StreamMinRequestSize;					//95
	WORD		StreamingTimeDma;						//96
	WORD		StreamingAccessLatency;					//97
	DWORD		StreamingPerformance;					//98-99
	ULONGLONG	MaxUserLba;								//100-103
	WORD		StremingTimePio;						//104
	WORD		Reserved3;								//105
	WORD		SectorSize;								//106
	WORD		InterSeekDelay;							//107
	WORD		IeeeOui;								//108
	WORD		UniqueId3;								//109
	WORD		UniqueId2;								//110
	WORD		UniqueId1;								//111
	WORD		Reserved4[4];							//112-115
	WORD		Reserved5;								//116
	DWORD		WordsPerLogicalSector;					//117-118
	WORD		Reserved6[8];							//119-126
	WORD		RemovableMediaStatus;					//127
	WORD		SecurityStatus;							//128
	WORD		VendorSpecific[31];						//129-159
	WORD		CfaPowerMode1;							//160
	WORD		ReservedForCompactFlashAssociation[7];	//161-167
	WORD		DeviceNominalFormFactor;				//168
	WORD		DataSetManagement;						//169
	WORD		AdditionalProductIdentifier[4];			//170-173
	WORD		Reserved7[2];							//174-175
	CHAR		CurrentMediaSerialNo[60];				//176-205
	WORD		SctCommandTransport;					//206
	WORD		ReservedForCeAta1[2];					//207-208
	WORD		AlignmentOfLogicalBlocks;				//209
	DWORD		WriteReadVerifySectorCountMode3;		//210-211
	DWORD		WriteReadVerifySectorCountMode2;		//212-213
	WORD		NvCacheCapabilities;					//214
	DWORD		NvCacheSizeLogicalBlocks;				//215-216
	WORD		NominalMediaRotationRate;				//217
	WORD		Reserved8;								//218
	WORD		NvCacheOptions1;						//219
	WORD		NvCacheOptions2;						//220
	WORD		Reserved9;								//221
	WORD		TransportMajorVersionNumber;			//222
	WORD		TransportMinorVersionNumber;			//223
	WORD		ReservedForCeAta2[10];					//224-233
	WORD		MinimumBlocksPerDownloadMicrocode;		//234
	WORD		MaximumBlocksPerDownloadMicrocode;		//235
	WORD		Reserved10[19];							//236-254
	WORD		IntegrityWord;							//255
};
#pragma	pack(pop)

enum COMMAND_TYPE
{
	CMD_TYPE_PHYSICAL_DRIVE = 0,
	CMD_TYPE_SCSI_MINIPORT,
	CMD_TYPE_SILICON_IMAGE,
	CMD_TYPE_SAT,				// SAT = SCSI_ATA_TRANSLATION
	CMD_TYPE_SUNPLUS,
	CMD_TYPE_IO_DATA,
	CMD_TYPE_LOGITEC,
	CMD_TYPE_JMICRON,
	CMD_TYPE_CYPRESS,
	CMD_TYPE_PROLIFIC,			// Not imprement
	CMD_TYPE_CSMI,				// CSMI = Common Storage Management Interface
	CMD_TYPE_CSMI_PHYSICAL_DRIVE, // CSMI = Common Storage Management Interface 
	CMD_TYPE_WMI,
	CMD_TYPE_DEBUG
};

//
// Command Descriptor Block constants.
//

#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10

#define SETBITON                             1
#define SETBITOFF                            0
//
// Mode Sense/Select page constants.
//

#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_SENSE_RETURN_ALL           0x3f
#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_DATA_COMPRESS         0x0f

//
// SCSI CDB operation codes
//

#define SCSIOP_TEST_UNIT_READY     0x00
#define SCSIOP_REZERO_UNIT         0x01
#define SCSIOP_REWIND              0x01
#define SCSIOP_REQUEST_BLOCK_ADDR  0x02
#define SCSIOP_REQUEST_SENSE       0x03
#define SCSIOP_FORMAT_UNIT         0x04
#define SCSIOP_READ_BLOCK_LIMITS   0x05
#define SCSIOP_REASSIGN_BLOCKS     0x07
#define SCSIOP_READ6               0x08
#define SCSIOP_RECEIVE             0x08
#define SCSIOP_WRITE6              0x0A
#define SCSIOP_PRINT               0x0A
#define SCSIOP_SEND                0x0A
#define SCSIOP_SEEK6               0x0B
#define SCSIOP_TRACK_SELECT        0x0B
#define SCSIOP_SLEW_PRINT          0x0B
#define SCSIOP_SEEK_BLOCK          0x0C
#define SCSIOP_PARTITION           0x0D
#define SCSIOP_READ_REVERSE        0x0F
#define SCSIOP_WRITE_FILEMARKS     0x10
#define SCSIOP_FLUSH_BUFFER        0x10
#define SCSIOP_SPACE               0x11
#define SCSIOP_INQUIRY             0x12
#define SCSIOP_VERIFY6             0x13
#define SCSIOP_RECOVER_BUF_DATA    0x14
#define SCSIOP_MODE_SELECT         0x15
#define SCSIOP_RESERVE_UNIT        0x16
#define SCSIOP_RELEASE_UNIT        0x17
#define SCSIOP_COPY                0x18
#define SCSIOP_ERASE               0x19
#define SCSIOP_MODE_SENSE          0x1A
#define SCSIOP_START_STOP_UNIT     0x1B
#define SCSIOP_STOP_PRINT          0x1B
#define SCSIOP_LOAD_UNLOAD         0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC  0x1C
#define SCSIOP_SEND_DIAGNOSTIC     0x1D
#define SCSIOP_MEDIUM_REMOVAL      0x1E
#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_SEEK                0x2B
#define SCSIOP_LOCATE              0x2B
#define SCSIOP_WRITE_VERIFY        0x2E
#define SCSIOP_VERIFY              0x2F
#define SCSIOP_SEARCH_DATA_HIGH    0x30
#define SCSIOP_SEARCH_DATA_EQUAL   0x31
#define SCSIOP_SEARCH_DATA_LOW     0x32
#define SCSIOP_SET_LIMITS          0x33
#define SCSIOP_READ_POSITION       0x34
#define SCSIOP_SYNCHRONIZE_CACHE   0x35
#define SCSIOP_COMPARE             0x39
#define SCSIOP_COPY_COMPARE        0x3A
#define SCSIOP_WRITE_DATA_BUFF     0x3B
#define SCSIOP_READ_DATA_BUFF      0x3C
#define SCSIOP_CHANGE_DEFINITION   0x40
#define SCSIOP_READ_SUB_CHANNEL    0x42
#define SCSIOP_READ_TOC            0x43
#define SCSIOP_READ_HEADER         0x44
#define SCSIOP_PLAY_AUDIO          0x45
#define SCSIOP_PLAY_AUDIO_MSF      0x47
#define SCSIOP_PLAY_TRACK_INDEX    0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME        0x4B
#define SCSIOP_LOG_SELECT          0x4C
#define SCSIOP_LOG_SENSE           0x4D


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

#endif