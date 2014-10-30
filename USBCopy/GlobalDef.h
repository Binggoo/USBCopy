#ifndef _GLOBAL_DEF_H
#define _GLOBAL_DEF_H

#include <Windows.h>

#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))

#define RANDOM_VALUE -1

#define BUF_SECTORS             (2 * 512) //512KB
#define MAX_COMPRESS_BUF        ((BUF_SECTORS + 8) * 512)
#define CLEAN_LENGTH           (1 * 1024 * 1024) //1M
#define BYTES_PER_SECTOR        512

#define ALLOC(dwBytes)			GlobalAlloc(GPTR,(dwBytes))
#define REALLOC(hMem, dwBytes)	GlobalReAlloc((hMem), (dwBytes), (GMEM_MOVEABLE|GMEM_ZEROINIT))
#define FREE(hMem)				GlobalFree((hMem))

#define DISK_PATH_LEN     20
#define IDENTIFY_BUFFER_SIZE  512
#define LEN_SN			20 //与硬盘的SN长度一致
#define LEN_MODEL		40 //与硬盘的model长度一致

#define IDE_ATAPI_IDENTIFY 0xA1   // 读取ATAPI设备的命令
#define IDE_ATA_IDENTIFY  0xEC   // 读取ATA设备的命

// 错误类型
typedef enum _ENUM_ERROR_TYPE
{
	ErrorType_System,
	ErrorType_Custom
}ErrorType;

//自定义错误
typedef enum _ENUM_CUSTOM_ERROR
{
	CustomError_OK = 0,
	CustomError_Cancel,
	CustomError_No_Master,
	CustomError_No_Target,
	CustomError_Compare_Failed,
	CustomError_All_Targets_Failed,
	CustomError_Master_Failed,
	CustomError_Image_Format_Error,
	CustomError_Compress_Error,
	CustomError_UnCompress_Error,
	CustomError_Speed_Too_Slow,
	CustomError_Unrecognized_Partition,
	CustomError_No_File_Select,
	CustomError_Target_Small,
	CustomError_Format_Error,
	CustomError_Get_Data_From_Server_Error,
	CustomError_Image_Hash_Value_Changed,
	CustomError_Enum_File_Failed

}CustomError;

// 工作模式
typedef enum _ENUM_WORK_MODE
{
	WorkMode_None = 0,
	WorkMode_FullCopy = 1,
	WorkMode_QuickCopy,
	WorkMode_FileCopy,
	WorkMode_DifferenceCopy,
	WorkMode_ImageCopy,
	WorkMode_ImageMake,
	WorkMode_DiskClean,
	WorkMode_DiskCompare,
	WorkMode_DiskFormat
}WorkMode;

// 端口状态
typedef enum _ENUM_PORT_STATE
{
	PortState_Offline = 0, //没有接入设备
	PortState_Online,    //有设备接入
	PortState_Pending,
	PortState_Active,    //正在工作中
	PortState_Stop, // 中间值
	PortState_Pass,
	PortState_Fail
}PortState;

typedef enum _ENUM_PORT_TYPE
{
	PortType_NULL = 0,
	PortType_MASTER_DISK,
	PortType_TARGET_DISK,
	PortType_TARGET_FILE,
	PortType_MASTER_FILE,
	PortType_SERVER
}PortType;

typedef enum _ENUM_CLEAN_MODE
{
	CleanMode_Full = 0,
	CleanMode_Quick,
	CleanMode_Safe
}CleanMode;

typedef enum _ENUM_COMPARE_MODE
{
	CompareMode_Full = 0,
	CompareMode_Quick
}CompareMode;

typedef enum _ENUM_SAVE_PATH
{
	PathType_Local = 0,
	PathType_Remote
}PathType;

typedef enum _ENUM_SOURCE_TYPE
{
	SourceType_Package = 0,
	SourceType_Master
}SourceType;

typedef enum _ENUM_COMPARE_RULE
{
	CompareRule_Fast = 0,
	CompareRule_Slow
}CompareRule;

typedef enum _ENUM_HASH_METHOD
{
	HashMethod_CHECKSUM32 = 0,
	HashMethod_CRC32,
	HashMethod_MD5
	
}HashMethod;

typedef enum _ENUM_HASH_LENGTH
{
	CHECKSUM32_LEN = 4,
	CRC32_LEN = 4,
	MD5_LEN = 16
}HashLen;

typedef enum _ENUM_LED_COLOR
{
	LedColor_Empty = 0,
	LedColor_Blue,
	LedColor_Green,
	LedColor_Red
}LedColor;

// 分区体系
typedef enum __ENUM_PARTITION_STYLE
{
	PartitionStyle_UNKNOWN,
	PartitionStyle_DOS,
	PartitionStyle_GPT,
	PartitionStyle_APPLE,
	PartitionStyle_BSD
}PartitionStyle;

typedef enum _ENUM_BOOT_SECTOR
{
	BootSector_UNKNOWN,
	BootSector_MBR,
	BootSector_DBR
}BootSector;

typedef enum _ENUM_FILE_SYSTEM
{
	FileSystem_UNKNOWN,
	FileSystem_EXTEND,
	FileSystem_FAT12,
	FileSystem_FAT16,
	FileSystem_FAT32,
	FileSystem_EXFAT,
	FileSystem_NTFS,
	FileSystem_EXT_X,
	FileSystem_EFI_PART
}FileSystem;

#pragma pack(push,1)
/****************************************************************************
*     分区表项结构(16字节)
****************************************************************************/
typedef struct _STRUCT_PARTITION_ENTRY
{
	UCHAR BootIndicator;  // 能否启动标志
	UCHAR StartHead;   // 该分区起始磁头号
	UCHAR StartSector;  // 起始柱面号高2位：6位起始扇区号
	UCHAR StartCylinder;  // 起始柱面号低8位
	UCHAR PartitionType;  // 分区类型
	UCHAR EndHead;   // 该分区终止磁头号
	UCHAR EndSector;   // 终止柱面号高2位：6位终止扇区号
	UCHAR EndCylinder;  // 终止柱面号低8位
	ULONG StartLBA;   // 起始扇区号
	ULONG TotalSector;  // 分区尺寸（总扇区数）
}PARTITION_ENTRY,*PPARTITION_ENTRY;

/****************************************************************************
*     主引导记录(MBR)结构 
****************************************************************************/
typedef struct _STRUCT_MASTER_BOOT_RECORD
{
	//UCHAR    BootCode[446];
	PARTITION_ENTRY  Partition[4];
	USHORT    Signature;
}MASTER_BOOT_RECORD,*PMASTER_BOOT_RECORD;

//EFI信息头
typedef struct _STRUCT_EFI_HEAD_INFO
{
	UCHAR Signature[8];                // 00~07 8
	ULONG RevisionNumber;              // 08~0B 4
	ULONG HeaderSize;                  // 0C~0F 4
	ULONG CRC32Checksum;               // 10~13 4
	ULONG Reserver;                    // 14~17 4
	ULONGLONG PrimaryLBA;              // 18~1F 8
	ULONGLONG BackupLBA;               // 20~27 8
	ULONGLONG FirstUsableLBA;          // 28~2F 8
	ULONGLONG LastUsableLBA;           // 30~37 8
	UCHAR DiskGUID[16];                // 38~47 16
	ULONGLONG PartitionEntryLBA;       // 48~4F 8
	ULONG MaxNumberOfPartitionEntries; // 50~53 4
	ULONG SizeOfPartitionEntry;        // 54~57 4
	ULONG PartitionEntryArrayCRC32;    // 58~5B 4
}EFI_HEAD,*PEFI_HEAD;

// GPT分区表项数据结构
typedef struct _STRUCT_GPT_PARTITION_ENTRY
{
	UCHAR PartitionTypeGUID[16];    // 00~0F 16
	UCHAR UniquePartitionGUID[16];  // 10~1F 16
	ULONGLONG StartingLBA;          // 20~27 8
	ULONGLONG EndingLBA;            // 28~2F 8
	ULONGLONG AttributeBits;        // 30~37 8
	UCHAR     PartitionName[72];     // 38~7F 72  Unicode
}GPT_PARTITION_ENTRY,*PGPT_PARTITION_ENTRY;

#pragma pack(pop)

// 有效数据
typedef struct _STRUCT_EFF_DATA
{
	ULONGLONG ullStartSector;
	ULONGLONG ullSectors;
	WORD      wBytesPerSector;
}EFF_DATA,*PEFF_DATA;

typedef CList<EFF_DATA,EFF_DATA&> EFF_LIST;
typedef CList<EFF_DATA,EFF_DATA&> *PEFF_LIST;

typedef struct _STRUCT_FAT_INFO
{
	WORD  wBytesPerSector;     // 0B~0C 2
	BYTE  bySectorsPerCluster;  // 0D~0D 1
	WORD  wReserverSectors;     // 0E~0F 2
	BYTE  byNumsOfFAT;          // 10~10 1
	DWORD dwVolumeLength;// 20~23 4
	DWORD dwFATLength;      // FAT32 20~23 4   FAT16 16~17 2
	WORD  wSectorsOfRoot;  // 此项只针对FAT12/16 11~12 2
	BYTE  byFATBit;
	ULONGLONG ullStartSector;
}FAT_INFO,*PFAT_INFO;

typedef struct _STRUCT_EXFAT_INFO
{
	ULONGLONG ullPartitionOffset; //40~47 8
	ULONGLONG ullVolumeLength;// 48~4F 8
	DWORD dwFatOffset;// 50~53 4
	DWORD dwFatLength;// 54~57 4
	DWORD dwClusterHeapOffset;// 58~5B 4
	DWORD dwClusterCount; //5C~5F 4
	DWORD dwRootDirectoryCluster; //60~63 4
	WORD wBytesPerSector;// 6C~6C 1    Power of 2. Minimum 9 (512 bytes per sector), maximum 12 (4096 bytes per sector)
	DWORD dwSectorsPerCluster;//6D~6D 1    Power of 2. Minimum 0 (1 sector per cluster), maximum 25 – BytesPerSectorShift, so max cluster size is 32 MB
	BYTE byNumsOfFat;//6E~6E 1 
}EXFAT_INFO,*PEXFAT_INFO;

typedef struct _STRUCT_NTFS_INFO
{
	WORD  wBytesPerSectors;           // 0B~0C 2
	BYTE  bySectorsPerCluster;        // 0D~0D 1
	ULONGLONG ullVolumeLength; // 28~2F 8
	ULONGLONG ullStartCluserOfMFT;    // 30~37 8
	BYTE  byMFTSize;                  // 40~40 1
	ULONGLONG ullStartSector;
}NTFS_INFO,*PNTFS_INFO;

typedef struct _STRUCT_SUPER_BLOCK_INFO
{
	DWORD dwBlocksOfVolume;   // 04~07 4
	DWORD dwStartBlockOfGroup0;   // 14~17 4
	DWORD dwBytesPerBlock;  // 18~1B 4  dwBytesPerBlock = 1024 << N;
	DWORD dwBlocksPerBlockGroup; // 20~23 4
	DWORD dwNodesPerBlockGroup;  // 28~2B
	DWORD dwSectorsPerBlock;
}SUPER_BLOCK_INFO,*PSUPER_BLOCK_INFO;

typedef struct _STRUCT_RANGE_FROM_TO
{
	ULONGLONG ullStartLBA;
	ULONGLONG ullEndingLBA;
}RANGE_FROM_TO,*PRANGE_FROM_TO;

// IMAGE define
#define LEN_DIGEST 64
#define SIZEOF_IMAGE_HEADER 1024
#define SIZEOF_IMAGE_DISCRP 256
#define IMAGE_FLAG "PY-IMG"
#define END_FLAG 0xED
#define START_FLAG 0xBE
#define IMAGE_VERSION 0x01
#define APP_VER_LENGTH						(16)	// this is the length of the FILE_HANDLER_VER
#define PKG_HEADER_SIZE  12
#define ZIP_VERSION   "1.28"
#define APP_VERSION   "1.0.0"

#pragma pack(push, 1)
typedef struct _STRUCT_IMAGE_HEADER
{
	char      szImageFlag[8];  //8
	ULONGLONG ullImageSize;    //8
	char	  szAppVersion[APP_VER_LENGTH]; //16
	DWORD	  dwImageVersion;  //4
	DWORD	  dwMaxSizeOfPackage; //4
	ULONGLONG ullCapacitySize; //8
	DWORD     dwBytesPerSector; //4
	char	  szZipVer[APP_VER_LENGTH]; //16
	ULONGLONG ullPkgCount; //8
	ULONGLONG ullValidSize; //8
	DWORD dwHashLen; //4
	DWORD	  dwHashType; //8
	BYTE	  byImageDigest[LEN_DIGEST]; // 64
	char      szDescription[SIZEOF_IMAGE_DISCRP]; //256
	BYTE      byReserved[SIZEOF_IMAGE_HEADER-8*5-4*5-APP_VER_LENGTH*2-LEN_DIGEST-SIZEOF_IMAGE_DISCRP-1];
	BYTE      byEnd; //1
}IMAGE_HEADER,*PIMAGE_HEADER;

#pragma pack(pop)


//////////////////////////////////////////////////////////////////////////
//网络部分
//////////////////////////////////////////////////////////////////////////

#ifndef DEF_COMMU_CMD
#define DEF_COMMU_CMD const unsigned long
#endif

#define MAX_BUFFER (512 + 12)*1024

#pragma pack(push, 1)
typedef struct _STRUCT_CMD_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
}CMD_IN,*PCMD_IN;

typedef struct _STRUCT_CMD_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
}CMD_OUT,*PCMD_OUT;

typedef struct _STRUCT_CMD_SYNC_TIME_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
}SYNC_TIME_IN,*PSYNC_TIME_IN;

typedef struct _STRUCT_CMD_SYNC_TIME_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	WORD wYmdHMS[6];
}SYNC_TIME_OUT,*PSYNC_TIME_OUT;

typedef struct _STRUCT_CMD_COPY_IMAGE_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStopFlag;
	char  *pszImageName;
	BYTE  byFlag;

}COPY_IMAGE_IN,*PCOPY_IMAGE_IN;

typedef struct _STRUCT_CMD_COPY_IMAGE_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
	BYTE  *pContext;
	BYTE  byFlag;
	
}COPY_IMAGE_OUT,*PCOPY_IMAGE_OUT;

typedef struct _STRUCT_CMD_MAKE_IMAGE_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStopFlag;
	char  *pszImageName;
	BYTE  *pContext;
	BYTE  byFlag;

}MAKE_IMAGE_IN,*PMAKE_IMAGE_IN;

typedef struct _STRUCT_CMD_MAKE_IMAGE_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
}MAKE_IMAGE_OUT,*PMAKE_IMAGE_OUT;

typedef struct _STRUCT_CMD_UPLOAD_LOG_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
	char  *pszImageName;
	BYTE  *pContext;
	BYTE  byFlag;

}UPLOAD_LOG_IN,*PUPLOAD_LOG_IN;

typedef struct _STRUCT_CMD_UPLOAD_LOG_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
}UPLOAD_LOG_OUT,*PUPLOAD_LOG_OUT;

typedef struct _STRUCT_CMD_QUERY_IMAGE_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
	char  *pszImageName;
	BYTE  byFlag;
}QUERY_IMAGE_IN,*PQUERY_IMAGE_IN;

typedef struct _STRUCT_CMD_QUERY_IMAGE_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
	BYTE  byHead[SIZEOF_IMAGE_HEADER];
	BYTE  byFlag;
}QUERY_IMAGE_OUT,*PQUERY_IMAGE_OUT;

// 2014-10-11 新增
typedef struct _STRUCT_CMD_DOWN_PKG_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
	char  *pszImageName;
	BYTE  byFlag;
}DOWN_PKG_IN,DOWN_LIST_IN,*PDOWN_PKG_IN,*PDOWN_LIST_IN;

typedef struct _STRUCT_CMD_DOWN_PKG_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
	char  *pszFileName;
	BYTE  *pContext;
	BYTE  byFlag;
}DOWN_PKG_OUT,*PDOWN_PKG_OUT;

typedef struct _STRUCT_CMD_DOWN_LIST_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
	BYTE  *pContext;
	BYTE  byFlag;
}DOWN_LIST_OUT,*PDOWN_LIST_OUT;

typedef struct _STRUCT_CMD_CUSTOM_LOG_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
	char  *pszImageName;
	BYTE  *pContext;
	BYTE  byFlag;
}CUSTOM_LOG_IN,*PCUSTOM_LOG_IN;

typedef struct _STRUCT_CMD_CUSTOM_LOG_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
}CUSTOM_LOG_OUT,*PCUSTOM_LOG_OUT;

typedef struct _STRUCT_CMD_QUERY_PKG_IN
{
	DWORD dwCmdIn;
	DWORD dwSizeSend;
	BYTE  byStop;
	char  *pszPkgName;
	BYTE  byFlag;
}QUERY_PKG_IN,QUERY_LIST_IN,*PQUERY_PKG_IN,*PQUERY_LIST_IN;

typedef struct _STRUCT_CMD_QUERY_PKG_OUT
{
	DWORD dwCmdOut;
	DWORD dwSizeSend;
	ErrorType errType;
	DWORD dwErrorCode;
	ULONGLONG ullFileSize;
	BYTE  byFlag;
}QUERY_PKG_OUT,QUERY_LIST_OUT,*PQUERY_PKG_OUT,*PQUERY_LIST_OUT;

#pragma pack(pop)

/************************************************************************/
/* COMMAND:CMD_SYNC_TIME
 * <Input>:
 * DEF_COMMU_CMD	CMD_SYNC_TIME_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD	CMD_SYNC_TIME_OUT;
 * WORD wYmdHMS[4]; //年月日时分秒*/
/************************************************************************/
DEF_COMMU_CMD CMD_SYNC_TIME_IN    = 0x43440000;
DEF_COMMU_CMD CMD_SYNC_TIME_OUT   = 0x43440100;

/************************************************************************/
/* COMMAND:CMD_COPY_IMAGE
 * <Input>:
 * DEF_COMMU_CMD CMD_COPY_IMAGE_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * BYTE byStop; //停止标志
 * char *pszImageName; //IMAGE名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_COPY_IMAGE_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * BYTE *pContext; // IMAGE内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED*/
/************************************************************************/
DEF_COMMU_CMD CMD_COPY_IMAGE_IN   = 0x43440001;
DEF_COMMU_CMD CMD_COPY_IMAGE_OUT  = 0x43440101;

/************************************************************************/
/* COMMAND:CMD_MAKE_IMAGE
 * <Input>:
 * DEF_COMMU_CMD CMD_MAKE_IMAGE_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * BYTE byStop; //停止标志
 * char *pszImageName; //IMAGE名称 0字符结尾
 * BYTE *pContext; //IMAGE内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_MAKE_IMAGE_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功 */
/************************************************************************/
DEF_COMMU_CMD CMD_MAKE_IMAGE_IN   = 0x43440002;
DEF_COMMU_CMD CMD_MAKE_IMAGE_OUT  = 0x43440102;

/************************************************************************/
/* COMMAND:CMD_UPLOAD_LOG
 * <Input>:
 * DEF_COMMU_CMD CMD_UPLOAD_LOG_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * char *pszLogName; //LOG名称，0字符结尾
 * BYTE *pContext; //LOG文件内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_UPLOAD_LOG_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功 */
/************************************************************************/
DEF_COMMU_CMD CMD_UPLOAD_LOG_IN   = 0x43440003;
DEF_COMMU_CMD CMD_UPLOAD_LOG_OUT  = 0x43440103;

/************************************************************************/
/* COMMAND:CMD_QUERY_IMAGE
 * <Input>:
 * DEF_COMMU_CMD CMD_QUERY_IMAGE_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * char *pszImageName; //IMAGE名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_QUERY_IMAGE_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * BYTE head[1024]; // 0字符结尾
/************************************************************************/
DEF_COMMU_CMD CMD_QUERY_IMAGE_IN  = 0x43440004;
DEF_COMMU_CMD CMD_QUERY_IMAGE_OUT = 0x43440104;

/************************************************************************/
/* COMMAND:CMD_SHOW_SCREEN
 * <Input>:
 * DEF_COMMU_CMD CMD_SHOW_SCREEN_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * BYTE *bitmap
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_SHOW_SCREEN_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
/************************************************************************/
DEF_COMMU_CMD CMD_SHOW_SCREEN_IN  = 0x43440005;
DEF_COMMU_CMD CMD_SHOW_SCREEN_OUT = 0x43440105;

// 2014-10-11 新增

/************************************************************************/
/* COMMAND:CMD_DOWN_PKG
 * <Input>:
 * DEF_COMMU_CMD CMD_DOWN_PKG_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * BYTE byStop; //停止标志
 * char *pszPkgName; //Package名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_DOWN_PKG_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * ULONGLONG ullFileSize;//Package文件大小
 * BYTE *pContext; // Package内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED*/
/************************************************************************/
DEF_COMMU_CMD CMD_DOWN_PKG_IN  = 0x43440006;
DEF_COMMU_CMD CMD_DOWN_PKG_OUT = 0x43440106;

/************************************************************************/
/* COMMAND:CMD_USER_LOG
 * <Input>:
 * DEF_COMMU_CMD CMD_USER_LOG_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * char *pszLogName; //LOG名称，0字符结尾
 * BYTE *pContext; //LOG文件内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_USER_LOG_IN;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功 */
/************************************************************************/
DEF_COMMU_CMD CMD_USER_LOG_IN   = 0x43440007;
DEF_COMMU_CMD CMD_USER_LOG_OUT  = 0x43440107;

/************************************************************************/
/* COMMAND:CMD_QUERY_PKG
 * <Input>:
 * DEF_COMMU_CMD CMD_QUERY_PKG_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * char *pszPkgName; //IMAGE名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_QUERY_PKG_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * ULONGLONG ullFileSize; //文件大小
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED */
/************************************************************************/
DEF_COMMU_CMD CMD_QUERY_PKG_IN  = 0x43440008;
DEF_COMMU_CMD CMD_QUERY_PKG_OUT = 0x43440108;

/************************************************************************/
/* COMMAND:CMD_DOWN_LIST
 * <Input>:
 * DEF_COMMU_CMD CMD_DOWN_LIST_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * BYTE byStop; //停止标志
 * char *pszPkgName; //Package名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_DOWN_LIST_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * ULONGLONG ullFileSize;//Package文件大小
 * BYTE *pContext; // Package内容
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED*/
/************************************************************************/
DEF_COMMU_CMD CMD_DOWN_LIST_IN  = 0x43440009;
DEF_COMMU_CMD CMD_DOWN_LIST_OUT = 0x43440109;

/************************************************************************/
/* COMMAND:CMD_QUERY_LIST
 * <Input>:
 * DEF_COMMU_CMD CMD_QUERY_LIST_IN;
 * DWORD dwSizeSend; //发送的字节数;
 * char *pszPkgName; //IMAGE名称
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED
 * <Output>:
 * DWORD dwSizeOfReturn; //返回数据总大小。
 * DEF_COMMU_CMD CMD_QUERY_LIST_OUT;
 * DWORD ErrorType; //错误类型
 * DWORD dwErrorCode; //错误代码，0表示成功
 * ULONGLONG ullFileSize; //文件大小
 * BYTE byFlag; //开始结束标志，开始：BE,结束：ED */
/************************************************************************/
DEF_COMMU_CMD CMD_QUERY_LIST_IN  = 0x4344000A;
DEF_COMMU_CMD CMD_QUERY_LIST_OUT = 0x4344010A;

#endif