//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		FileHeader.h
//!	\brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FILE_HEADER_H_
#define _FILE_HEADER_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define FILE_SYS_VERSION                        (1)
#define FILE_SYS_SIGNATURE_BYTE                 (0x55)

#define FILE_HEADER_SYS_INFO_TYPE_EMPTY         (0xFF)
#define FILE_HEADER_SYS_INFO_TYPE_FILE_NEW      (0x01)
#define FILE_HEADER_SYS_INFO_TYPE_FILE_CHUNK    (0x02)

#define FILE_HEADER_BLOCK_STATUS_HEALTHY        (0xFF)

//////////////////////////////////////////////////////////////////////////////////////////////////

// Header: Time stamp
typedef struct
{
    UINT16  wYear;  // 0-9999
    UINT8   bMonth; // 1-12
    UINT8   bDay;   // 1-32
    UINT8   bHour;  // 0-23
    UINT8   bMinute;// 0-59
    UINT8   bSecond;// 0-59
}__attribute__((__packed__)) FileHeaderTimeStamp;

///////////////////////////////////////////////////////////////////////////
// FILE HEADER SECTION: GENERAL FILE SYS INFO
///////////////////////////////////////////////////////////////////////////
typedef struct
{
	// * * * * * * * * * * * * * * * * * * * *
	// if any of the bytes(bMemBlockStat) is different than 0xFF then is bad block
    UINT8               bMemBlockStat[2];
	UINT8               bFileSysVer;
    UINT8               bFileSysSigByte[4];
    UINT8               bFileHeaderType;
	// * * * * * * * * * * * * * * * * * * * *
    // UINT8               bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionSysInfoType;

typedef struct
{
	FileHeaderSectionSysInfoType	tFileSysInfo;
    UINT8               			bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionSysInfoWrapType;

///////////////////////////////////////////////////////////////////////////
// FILE HEADER SECTION: CLOSE FILE
///////////////////////////////////////////////////////////////////////////

// Header: Close
typedef struct
{
	// * * * * * * * * * * * * * * * * * * * *
    FileHeaderTimeStamp tFileTimeClose;
    BOOL                fIsOpen;
    UINT64              qwFileTotSizeBytes;
    UINT8               bTotalDataFileCheckSum8;
	// * * * * * * * * * * * * * * * * * * * *
	//UINT8               bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionCloseType;

typedef struct
{
	FileHeaderSectionCloseType	tFileClose;
	UINT8               		bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionCloseWrapType;

///////////////////////////////////////////////////////////////////////////
// FILE HEADER SECTION: CHUNK FILE
///////////////////////////////////////////////////////////////////////////

typedef struct
{
	// * * * * * * * * * * * * * * * * * * * *
	UINT32					dwChunkSequence;
	BOOL					fIsWrittingChunk;
	UINT8                  	bDataChunkCheckSum8;
	// * * * * * * * * * * * * * * * * * * * *
	// UINT8                  	bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionFileDataChunkType;

typedef struct
{
	FileHeaderSectionFileDataChunkType	tFileChunk;
	UINT8                  				bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionFileDataChunkWrapType;

///////////////////////////////////////////////////////////////////////////
// FILE HEADER SECTION: FILE INFO
///////////////////////////////////////////////////////////////////////////

typedef struct
{
	// * * * * * * * * * * * * * * * * * * * *
    BOOL                    fFileIsBin;
    FileHeaderTimeStamp     tFileTimeOpen;
    UINT8                   bExtension[3];
    UINT8                   bHeaderChunkStampPageRate;
	// * * * * * * * * * * * * * * * * * * * *
	// UINT8                  	bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionFileInfoType;

typedef struct
{
	FileHeaderSectionFileInfoType	tFileInfo;
	UINT8                  			bHeaderCheckSum8;
}__attribute__((__packed__)) FileHeaderSectionFileInfoWrapType;

///////////////////////////////////////////////////////////////////////////
// FILE HEADER TYPES
///////////////////////////////////////////////////////////////////////////

typedef struct
{
	FileHeaderSectionSysInfoWrapType	tFileSysInfoWrap;
    // FileHeaderSectionSysInfoType		tFileSysInfo;

	FileHeaderSectionFileInfoWrapType	tFileInfoWrap;
	// FileHeaderSectionFileInfoType	tFileInfo;

	FileHeaderSectionCloseWrapType		tFileCloseWrap;
    // FileHeaderSectionCloseType		tFileClose;

	FileHeaderSectionFileDataChunkWrapType	tFileChunkWrap;
	//FileHeaderSectionFileDataChunkType	tFileChunk;

}__attribute__((__packed__)) FileHeaderFileStart;

typedef struct
{
    FileHeaderSectionSysInfoWrapType	tFileSysInfoWrap;
    // FileHeaderSectionSysInfoType		tFileSysInfo;

	FileHeaderSectionFileDataChunkWrapType	tFileChunkWrap;
	//FileHeaderSectionFileDataChunkType	tFileChunk;

}__attribute__((__packed__)) FileHeaderFileContinue;

///////////////////////////////////////////////////////////////////////////
// TYPES UNION
///////////////////////////////////////////////////////////////////////////

typedef union
{
    FileHeaderFileStart         tFileStart;
    FileHeaderFileContinue      tFileContinue;
}FileHeaderTypes;

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _FILE_HEADER_H_

//! @}

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

