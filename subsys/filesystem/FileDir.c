//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        FileDir.h
//!    \brief       File Directory System.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "types.h"

#include "Rtc.h"

#include "../../services/SysTime.h"

#include "../../drivers/flash/FlashMem/NandSimu.h"
#include "../memory/FlashMemMap.h"
#include "../memory/Flash.h"

#include "FileHeader.h"
#include "FileWriteRef.h"
#include "FileDir.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    FlashMemMapBlockType 	tBlock;
	UINT32					dwPage;
}FileDirFileIndexType;


//////////////////////////////////////////////////
// FILE WRITE STRCUTURE
//////////////////////////////////////////////////
typedef struct
{
    BOOL    fIsPageContainsHeader;
    UINT16  wHeaderSize;
    UINT32  dwDataBytesWritten;
    UINT32  dwDataBytesAvailable;
}__attribute__((__packed__)) FileWriteRefPage;

typedef struct
{
    UINT8                   bHeaderChunkStampPageRate;
	// track data chunk in case is required. bHeaderChunkStampPageRate != 0
	UINT32					dwChunkSequence;
	UINT8                  	bDataChunkCheckSum8;
}__attribute__((__packed__)) FileWriteRefChunk;

typedef struct
{
	UINT64					qwFileTotBytes;
	//UINT8                  	bTotDataCheckSum8;
}__attribute__((__packed__)) FileWriteRefClose;

typedef struct
{
	FileDirFileIndexType	tFileIndexStart;
	FileDirFileIndexType	tFileIndexCurrent;
	UINT32					dwPageIndex;
	FileWriteRefPage		tCurrentPageStat;
	FileWriteRefChunk		tChunkStat;
	FileWriteRefClose		tCloseStat;
}__attribute__((__packed__)) FileWriteRefOpenFileReference;
//////////////////////////////////////////////////
//////////////////////////////////////////////////



typedef struct
{
	// * * * * * * * * * * * * * * * * * * * *
	// GENERAL
    CHAR const * const              pcDirName;
	// * * * * * * * * * * * * * * * * * * * *
	// CONFIG
    FileDirectoryWriteTypeEnum      eWriteType;
    FlashMemMapEnum  				eMemoryMap;
	// mallest size the algorithm will navigate looking for files in memory.
	// this number has to be multiple of Page size.
	UINT16							wFileMemMapSmallestSlicePageAmount;
	// this number has to be multiple of Page size.
	//BOOL							fIsFileChunkHeaderStampEnabled;
	// a chunk header will be stamped every X size bytes in the file
	//UINT16							wFileChunkHeaderStampEverySizePages;
	// * * * * * * * * * * * * * * * * * * * *
	// STAT
	BOOL  							fIsLoaded;
	FileDirFileIndexType			tCurrentIndex;
	FileDirFileIndexType			tLastIndex;
	BOOL                            fIsWritingFile;
	FileWriteRefOpenFileReference   tFileWritingRef;
}FileDirType;

static FileDirType tDirArray[] =
{
    {
		// * * * * * * * * * * * * * * * * * * * *
		// GENERAL
        //pcDirName
        "FILE_DIR_SYS_INF",
		// * * * * * * * * * * * * * * * * * * * *
		// CONFIG
		// eWriteType                		eMemoryMap
		FILE_DIR_WRITE_TYPE_CIRCULAR,   	FLASH_MEM_MAP_FILE_SYS_INFO,
		// wFileMemMapSmallestSlicePageAmount	fIsFileChunkHeaderStampEnabled	wFileChunkHeaderStampEverySizePages
		2,										//FALSE,							0,
		// * * * * * * * * * * * * * * * * * * * *
		// STAT
		// fIsLoaded
		0,
		// tCurrentIndex
		{0},
		// tLastIndex
		{0},
		// fIsWritingFile
		0,
    },
	{
		// * * * * * * * * * * * * * * * * * * * *
		// GENERAL
        //pcDirName
        "DIR_TEST",
		// * * * * * * * * * * * * * * * * * * * *
		// CONFIG
		// eWriteType                		eMemoryMap
		FILE_DIR_WRITE_TYPE_LINEAR,   		FLASH_MEM_MAP_TEST_1,
		// wFileMemMapSmallestSlicePageAmount	fIsFileChunkHeaderStampEnabled	wFileChunkHeaderStampEverySizePages
		2,										//FALSE,							0,
		// * * * * * * * * * * * * * * * * * * * *
		// STAT
		// fIsLoaded
		0,
		// tCurrentIndex
		{0},
		// tLastIndex
		{0},
		// fIsWritingFile
		0,
    },
	{
		// * * * * * * * * * * * * * * * * * * * *
		// GENERAL
        //pcDirName
        "DIR_CONFIG_TEMP",
		// * * * * * * * * * * * * * * * * * * * *
		// CONFIG
		// eWriteType                		eMemoryMap
		FILE_DIR_WRITE_TYPE_LINEAR,   		FLASH_MEM_MAP_TEST_2,
		// wFileMemMapSmallestSlicePageAmount	fIsFileChunkHeaderStampEnabled	wFileChunkHeaderStampEverySizePages
		2,										//FALSE,							4,
		// * * * * * * * * * * * * * * * * * * * *
		// STAT
		// fIsLoaded
		0,
		// tCurrentIndex
		{0},
		// tLastIndex
		{0},
		// fIsWritingFile
		0,
    },
};

#define FILE_DIR_DIRECTORY_AMOUNT       ( sizeof(tDirArray) / sizeof( tDirArray[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL FileDirGetDirFileIndexFirst ( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex );
static BOOL FileDirGetDirFileIndexNext  ( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex );
static BOOL FileDirGetDirFileIndexIsLast( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex, BOOL *pfIsLast );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirModuleInit( void )
{
    FlashMemMapInitModule();

    if( FILE_DIR_DIRECTORY_AMOUNT != FILE_DIR_MAX )
    {
        while(TRUE)
        {

        }
    }

	// make sure wFileMemMapSmallestSlicePageAmount is divisible in equal parts for mem map
	for( FileDirectoryEnum eDir = 0; eDir < FILE_DIR_DIRECTORY_AMOUNT ; eDir++)
	{
		UINT32 dwMemMapSizeBytes = FlashMemMapSizeBytes( tDirArray[eDir].eMemoryMap );
		UINT32 dwMemMapSizePages = dwMemMapSizeBytes / FLASH_PAGE_SIZE_BYTES;
		// mem map should be divided in equal page size parts
		if( ( dwMemMapSizePages % tDirArray[eDir].wFileMemMapSmallestSlicePageAmount ) != 0 )
		{
			// catch this bug in development time
			while(TRUE)
			{
			}
		}
	}

	// calculate last directory file index
	// only for circular file type
	for( FileDirectoryEnum eDir = 0; eDir < FILE_DIR_DIRECTORY_AMOUNT ; eDir++)
	{
        if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
        {
            FlashMemMapBlockGetLast( tDirArray[eDir].eMemoryMap, &tDirArray[eDir].tLastIndex );
            FlashMemMapBlockPageGetLast( tDirArray[eDir].tLastIndex.tBlock.dwBlock, &tDirArray[eDir].tLastIndex.dwPage );

            // move x amount of pages in reverse since the count is starting from the last page.
            // start from 1 since the current page you are sitting should count too
            for( UINT32 dwPagCounter = 1 ; dwPagCounter < tDirArray[eDir].wFileMemMapSmallestSlicePageAmount ; dwPagCounter++ )
            {
                // move to next page and if fails, then first page of next block
                if( FlashMemMapBlockPageGetPrev( tDirArray[eDir].tLastIndex.tBlock.dwBlock, tDirArray[eDir].tLastIndex.dwPage, &tDirArray[eDir].tLastIndex.dwPage ) == FALSE )
                {
                    FlashMemMapBlockGetPrev( tDirArray[eDir].eMemoryMap, &tDirArray[eDir].tLastIndex.tBlock, &tDirArray[eDir].tLastIndex.tBlock );
                    FlashMemMapBlockPageGetLast( tDirArray[eDir].tLastIndex.tBlock.dwBlock, &tDirArray[eDir].tLastIndex.dwPage );
                }
            }
        }
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 FileDirCheckSum8( UINT8 bCurrentCheckSum8, UINT8 * pvBuffer, UINT32 dwBufferSize )
{
	if( pvBuffer != NULL )
	{
		for( UINT32 c = 0 ; c < dwBufferSize ; c++ )
		{
			bCurrentCheckSum8 +=  pvBuffer[c];
		}
	}

	return bCurrentCheckSum8;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 FileDirTimeStampToEpoch( FileHeaderTimeStamp *ptFileDateTime )
{
	UINT32 dwTimeEpoch = 0;

	if( ptFileDateTime != NULL )
	{
		// WARNING!! careful with this operation since both are different structures
		// but of same size and same order of list of variables plus both are __packed__
		// if any of this conditions is not true then this operation can not be executed this way.
		dwTimeEpoch = SysTimeDateTimeToEpochSeconds( (SysTimeDateTimeStruct *)ptFileDateTime );
	}

	return dwTimeEpoch;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirLoad( FileDirectoryEnum eDir, BOOL fLoad )
{
	BOOL fSuccess = FALSE;

	if( eDir < FILE_DIR_DIRECTORY_AMOUNT )
	{
		if( fLoad )
		{
			if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
			{
				FileHeaderFileStart tFileStart;
				BOOL fIsError			= FALSE;
				BOOL fIsLast 			= FALSE;
				BOOL fIsLatestFileFound = FALSE;
				FileHeaderTimeStamp		tFileTimeStampCurrent;
				// FileDirFileIndexType	tFileIndexCurrent;

				// default minimum time stamp
				tFileTimeStampCurrent.wYear 	= 2000;
				tFileTimeStampCurrent.bMonth	= 01;
				tFileTimeStampCurrent.bDay		= 01;
				tFileTimeStampCurrent.bHour		= 00;
				tFileTimeStampCurrent.bMinute	= 00;
				tFileTimeStampCurrent.bSecond	= 00;

				FileDirGetDirFileIndexFirst( eDir, &tDirArray[eDir].tCurrentIndex );

				do
				{
					// read file header
					if
					(
						FlashReadPage(
							tDirArray[eDir].tCurrentIndex.tBlock.bFlashId,
							tDirArray[eDir].tCurrentIndex.dwPage,
							sizeof(tFileStart),
							&tFileStart,
							sizeof(tFileStart)
						) == FALSE
					)
					{
						fIsError = TRUE;
					}
					else
					{
						// check if there is a valid File sys info header
						if
						(
							FileDirCheckSum8( 0, &tFileStart.tFileSysInfoWrap.tFileSysInfo, sizeof(tFileStart.tFileSysInfoWrap.tFileSysInfo) )
							==
							tFileStart.tFileSysInfoWrap.bHeaderCheckSum8
						)
						{
							if
							(
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysVer == FILE_SYS_VERSION ) &&
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[0] == FILE_SYS_SIGNATURE_BYTE ) &&
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[1] == FILE_SYS_SIGNATURE_BYTE ) &&
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[2] == FILE_SYS_SIGNATURE_BYTE ) &&
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[3] == FILE_SYS_SIGNATURE_BYTE ) &&
								( tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileHeaderType	  == FILE_HEADER_SYS_INFO_TYPE_FILE_NEW )
							)
							{
								// check open file time stamp
								if
								(
									FileDirCheckSum8( 0, &tFileStart.tFileInfoWrap.tFileInfo, sizeof(tFileStart.tFileInfoWrap.tFileInfo) )
									==
									tFileStart.tFileInfoWrap.bHeaderCheckSum8
								)
								{
									// extract time stamp
									// check if is smaller date time than the previous time stamp (what if there hasnt been extracted any time stamp?)
									if( FileDirTimeStampToEpoch( &tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen ) > FileDirTimeStampToEpoch( &tFileTimeStampCurrent ) )
									{
										// indicate at least one file found
										fIsLatestFileFound = TRUE;

										// keep track of the file index
										// memcpy( &tFileIndexCurrent, tDirArray[eDir].tCurrentIndex, sizeof(tFileIndexCurrent) );

										// copy time stamp to keep comparing
										memcpy( &tFileTimeStampCurrent, &tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen, sizeof(tFileTimeStampCurrent) );
									}
									else
									{
										// found date time that is from older file instead of newst.
										// time to stop loop
										break;
									}
								}
							}
						}
					}

					FileDirGetDirFileIndexNext( eDir, &tDirArray[eDir].tCurrentIndex );

					FileDirGetDirFileIndexIsLast( eDir, &tDirArray[eDir].tCurrentIndex, &fIsLast );
				}while( fIsLast == FALSE && fIsError == FALSE );

				if( fIsError == FALSE )
				{
					if( fIsLatestFileFound == FALSE )
					{
						// if no valid file found then use the first index of memory as current index
						FileDirGetDirFileIndexFirst( eDir, &tDirArray[eDir].tCurrentIndex );
					}
					else
                    {
                        // walk until this latest file ends
                        // 1.- check if file got closed correctly
                        //  if not closed correctly
                        //          if ascii type
                        //                 try to read until find pattern 0xFF
                        //          if bin type
                        //                  check header chunk page amount
                        //                      if 0 then this file can be overwritten(there is no way to recover data) so stay in this index for opening a new file
                        //                      if no 0 then check the next chunk header making sure Data Chunk sequence number is valid.
                        //                              if Data Chunk sequence number is NOT valid then stop there and go the next new file index.
                        //  else if closed correctly then
                        //          calculate number of pages to jump
                        //          after jumping pages, get next file index. and at that point will be the next open write file
					}

					tDirArray[eDir].fIsLoaded = TRUE;

					fSuccess = TRUE;
				}
			}
			else if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_LINEAR )
			{
				// point file to the first memory location always

				FileDirGetDirFileIndexFirst( eDir, &tDirArray[eDir].tCurrentIndex );

				tDirArray[eDir].fIsLoaded = TRUE;

				fSuccess = TRUE;
			}
		}
		else
		{
			tDirArray[eDir].fIsLoaded = FALSE;

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirIsLoaded( FileDirectoryEnum eDir )
{
	if( eDir < FILE_DIR_DIRECTORY_AMOUNT )
	{
		return tDirArray[eDir].fIsLoaded;
	}
	else
	{
		return FALSE;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirGetDirFileIndexFirst( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex )
{
	BOOL fSuccess = FALSE;

	if
	(
		( eDir < FILE_DIR_DIRECTORY_AMOUNT ) &&
		( ptCurrentIndex != NULL )
	)
	{
		fSuccess = TRUE;
		fSuccess&= ( FlashMemMapBlockGetFirst( tDirArray[eDir].eMemoryMap, &ptCurrentIndex->tBlock ) );
		fSuccess&= ( FlashMemMapBlockPageGetFirst( ptCurrentIndex->tBlock.dwBlock, &ptCurrentIndex->dwPage ) );
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirGetDirFileIndexNext( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex )
{
	BOOL fSuccess = FALSE;

	if
	(
		( eDir < FILE_DIR_DIRECTORY_AMOUNT ) &&
		( ptCurrentIndex != NULL )
	)
	{
		// only for circular file type
		if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
		{
			// move x amount of pages.
			for( UINT32 dwPagCounter = 0 ; dwPagCounter < tDirArray[eDir].wFileMemMapSmallestSlicePageAmount ; dwPagCounter++ )
			{
				// move to next page and if fails, then first page of next block
				if( FlashMemMapBlockPageGetNext( ptCurrentIndex->tBlock.dwBlock, ptCurrentIndex->dwPage, &ptCurrentIndex->dwPage ) == FALSE )
				{
					FlashMemMapBlockGetNext( tDirArray[eDir].eMemoryMap, &ptCurrentIndex->tBlock, &ptCurrentIndex->tBlock );
					FlashMemMapBlockPageGetFirst( ptCurrentIndex->tBlock.dwBlock, &ptCurrentIndex->dwPage );
				}
			}

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirGetDirFileIndexIsLast( FileDirectoryEnum eDir, FileDirFileIndexType *ptCurrentIndex, BOOL *pfIsLast )
{
	BOOL fSuccess = FALSE;

	if
	(
		( eDir < FILE_DIR_DIRECTORY_AMOUNT ) &&
		( ptCurrentIndex != NULL ) &&
		( pfIsLast != NULL )
	)
	{
		// only for circular file type
		if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
		{
			(*pfIsLast) = TRUE;
			// compare blocks
			(*pfIsLast)&= ( memcmp( &tDirArray[eDir].tLastIndex.tBlock, &ptCurrentIndex->tBlock, sizeof(ptCurrentIndex->tBlock) ) == 0 );
			// compare pages
			(*pfIsLast)&= ( tDirArray[eDir].tLastIndex.dwPage == ptCurrentIndex->dwPage );

			fSuccess = TRUE;
		}
		else if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_LINEAR )
		{
			// for linear, first is and last is the only index it can use for a new file
			FileDirFileIndexType tDirFileIndexFirst;

			FlashMemMapBlockGetFirst( tDirArray[eDir].eMemoryMap, &tDirFileIndexFirst.tBlock );
			FlashMemMapBlockPageGetFirst( tDirFileIndexFirst.tBlock.dwBlock, &tDirFileIndexFirst.dwPage );

			(*pfIsLast) = TRUE;
			// compare blocks
			(*pfIsLast)&= ( memcmp( &tDirFileIndexFirst.tBlock, &ptCurrentIndex->tBlock, sizeof(ptCurrentIndex->tBlock) ) == 0 );
			// compare pages
			(*pfIsLast)&= ( tDirFileIndexFirst.dwPage == ptCurrentIndex->dwPage );

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FileDirFileWriteOpenNew( FileDirectoryEnum eDir, FileWriteRefNewFileInputParams *ptParamsIn, FILE_W_REF * ptFileRef )
{
    BOOL fSuccess = FALSE;

	if( eDir < FILE_DIR_DIRECTORY_AMOUNT )
	{
        if( tDirArray[eDir].fIsLoaded )
        {
            if
            (
                ( ptParamsIn != NULL ) &&
                ( ptFileRef != NULL )
            )
            {
                ////////////////////////////////////////////
                FileHeaderFileStart     tFileStart;
                SysTimeDateTimeStruct   tDateTimeUtc;

                memset( &tFileStart, 0xFF, sizeof(tFileStart) );
                SysTimeGetDateTime( &tDateTimeUtc, TRUE );

                tFileStart.tFileSysInfoWrap.tFileSysInfo.bMemBlockStat[0]   = FILE_HEADER_BLOCK_STATUS_HEALTHY;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bMemBlockStat[1]   = FILE_HEADER_BLOCK_STATUS_HEALTHY;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysVer        = FILE_SYS_VERSION;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[0] = FILE_SYS_SIGNATURE_BYTE;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[1] = FILE_SYS_SIGNATURE_BYTE;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[2] = FILE_SYS_SIGNATURE_BYTE;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileSysSigByte[3] = FILE_SYS_SIGNATURE_BYTE;
                tFileStart.tFileSysInfoWrap.tFileSysInfo.bFileHeaderType    = FILE_HEADER_SYS_INFO_TYPE_FILE_NEW;
                tFileStart.tFileSysInfoWrap.bHeaderCheckSum8                = FileDirCheckSum8( 0, &tFileStart.tFileSysInfoWrap.tFileSysInfo, sizeof(tFileStart.tFileSysInfoWrap.tFileSysInfo) );

                tFileStart.tFileInfoWrap.tFileInfo.bHeaderChunkStampPageRate= ptParamsIn->bHeaderChunkStampPageRate;
                tFileStart.tFileInfoWrap.tFileInfo.fFileIsBin           = ptParamsIn->fFileIsBin;
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.wYear  = tDateTimeUtc.tDate.wYear;  // 0-9999
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.bMonth = tDateTimeUtc.tDate.wYear;  // 1-12
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.bDay   = tDateTimeUtc.tDate.wYear;  // 1-32
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.bHour  = tDateTimeUtc.tTime.bHour;  // 0-23
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.bMinute= tDateTimeUtc.tTime.bMinute;// 0-59
                tFileStart.tFileInfoWrap.tFileInfo.tFileTimeOpen.bSecond= tDateTimeUtc.tTime.bSecond;// 0-59
                tFileStart.tFileInfoWrap.tFileInfo.bExtension[0]        = ptParamsIn->bExtension[0];
                tFileStart.tFileInfoWrap.tFileInfo.bExtension[1]        = ptParamsIn->bExtension[1];
                tFileStart.tFileInfoWrap.tFileInfo.bExtension[2]        = ptParamsIn->bExtension[2];
                tFileStart.tFileInfoWrap.bHeaderCheckSum8               = FileDirCheckSum8( 0, &tFileStart.tFileInfoWrap.tFileInfo, sizeof(tFileStart.tFileInfoWrap.tFileInfo) );

                tFileStart.tFileChunkWrap.tFileChunk.dwChunkSequence    = 0;
                ////////////////////////////////////////////

                if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
                {

                }
                else if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_LINEAR )
                {
                    // prepare reference structure
                    memset( &tDirArray[eDir].tFileWritingRef, 0x00, sizeof(tDirArray[eDir].tFileWritingRef) );

                    memcpy( &tDirArray[eDir].tFileWritingRef.tFileIndexStart, &tDirArray[eDir].tCurrentIndex, sizeof(tDirArray[eDir].tFileWritingRef.tFileIndexStart) );
                    memcpy( &tDirArray[eDir].tFileWritingRef.tFileIndexCurrent, &tDirArray[eDir].tCurrentIndex, sizeof(tDirArray[eDir].tFileWritingRef.tFileIndexCurrent) );

                    tDirArray[eDir].tFileWritingRef.dwPageIndex                           = 0;

                    tDirArray[eDir].tFileWritingRef.tCurrentPageStat.fIsPageContainsHeader= TRUE;
                    tDirArray[eDir].tFileWritingRef.tCurrentPageStat.wHeaderSize          = sizeof(tFileStart);
                    tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesWritten   = 0;
                    tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable = FLASH_PAGE_SIZE_BYTES - sizeof(tFileStart);

                    tDirArray[eDir].tFileWritingRef.tChunkStat.bHeaderChunkStampPageRate  = ptParamsIn->bHeaderChunkStampPageRate;
                    tDirArray[eDir].tFileWritingRef.tChunkStat.dwChunkSequence            = 0;
                    tDirArray[eDir].tFileWritingRef.tChunkStat.bDataChunkCheckSum8        = 0;

                    tDirArray[eDir].tFileWritingRef.tCloseStat.qwFileTotBytes             = 0;

                    tDirArray[eDir].fIsWritingFile = TRUE;

                    if( FlashEraseBlock( (FlashIdEnum)tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.bFlashId, tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.dwBlock ) )
                    {
                        // write header
                        if
                        (
                            FlashWriteAtPageOffset
                            (
                                tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.bFlashId,
                                tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.dwPage,
                                0,
                                &tFileStart,
                                sizeof(tFileStart)
                            )
                        )
                        {
                            (*ptFileRef) = (FILE_W_REF)&tDirArray[eDir].tFileWritingRef;

                            tDirArray[eDir].fIsWritingFile = TRUE;

                            fSuccess = TRUE;
                        }
                    }
                }
            }
        }
	}

	return fSuccess;
}

// http://www.freertos.org/FreeRTOS-Plus/Fail_Safe_File_System/Fail_Safe_Embedded_File_System_demo.shtml

BOOL FileDirFileWriteAppendData( FileDirectoryEnum eDir, FILE_W_REF * ptFileRef, void *pvBuffer, UINT16 wBytesToSend, UINT16 *pwBytesSent )
{
    BOOL fSuccess = FALSE;

    if( eDir < FILE_DIR_DIRECTORY_AMOUNT )
	{
        if( tDirArray[eDir].fIsWritingFile )
        {
            if( (*ptFileRef) == &tDirArray[eDir].tFileWritingRef )
            {
                if
                (
                    ( pvBuffer != NULL ) &&
                    ( wBytesToSend != 0 )
                )
                {
                    if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_CIRCULAR )
                    {

                    }
                    else if( tDirArray[eDir].eWriteType == FILE_DIR_WRITE_TYPE_LINEAR )
                    {
                        UINT16 wBytesToWrite = 0 ;
                        BOOL    fIsError = FALSE;

                        for( UINT16 wIdx = 0 ; wIdx < wBytesToSend ; wIdx++ )
                        {
                            if( wBytesToSend > tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable )
                            {
                                wBytesToWrite = tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable;
                            }
                            else
                            {
                                wBytesToWrite = tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable;
                            }

                            if
                            (
                                FlashWriteAtPageOffset
                                (
                                    tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.bFlashId,
                                    tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.dwPage,
                                    tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesWritten, // offset to write
                                    &pvBuffer[wIdx],
                                    wBytesToWrite
                                )
                            )
                            {
                                // update vars
                                tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesWritten   += wBytesToWrite;
                                tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable -= wBytesToWrite;

                                tDirArray[eDir].tFileWritingRef.tChunkStat.bDataChunkCheckSum8         = FileDirCheckSum8( tDirArray[eDir].tFileWritingRef.tChunkStat.bDataChunkCheckSum8, &pvBuffer[wIdx], wBytesToWrite );

                                tDirArray[eDir].tFileWritingRef.tCloseStat.qwFileTotBytes             += wBytesToWrite;

                                wIdx += wBytesToWrite;
                            }
                            else
                            {
                                fIsError = TRUE;
                                break;
                            }

                            // check if current page is full
                            if( tDirArray[eDir].tFileWritingRef.tCurrentPageStat.dwDataBytesAvailable == 0 )
                            {
                                // get next page

                                // if page is last in subsector then get next subsector then first page of that subsector
                                BOOL fIsPageLastInBlockResult = FALSE;
                                FlashMemMapBlockPageIsLast
                                (
                                    tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.dwBlock,
                                    tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.dwPage,
                                    &fIsPageLastInBlockResult
                                );


                                if( fIsPageLastInBlockResult )
                                {

                                    // if last block of memory map then function below will return false
                                    if
                                    (
                                        FlashMemMapBlockGetNext
                                        (
                                            tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.bFlashId,
                                            &tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.dwBlock,
                                            &tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.dwBlock
                                        )
                                    )
                                    {
                                        tDirArray[eDir].tFileWritingRef.dwPageIndex++;

                                        FlashMemMapBlockPageGetFirst
                                        (
                                            tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.tBlock.dwBlock,
                                            &tDirArray[eDir].tFileWritingRef.tFileIndexCurrent.dwPage
                                        );

                                        // check if page requires chunk stamp
                                        if( tDirArray[eDir].tFileWritingRef.tChunkStat.bHeaderChunkStampPageRate > 0 )
                                        {
                                            if
                                            (
                                                ( ( tDirArray[eDir].tFileWritingRef.dwPageIndex % tDirArray[eDir].tFileWritingRef.tChunkStat.bHeaderChunkStampPageRate ) == 0 )
                                            )
                                            {
                                                // TODO: add chunk stamp here

                                                // reset chunk variables
                                                tDirArray[eDir].tFileWritingRef.tChunkStat.dwChunkSequence++;
                                                tDirArray[eDir].tFileWritingRef.tChunkStat.bDataChunkCheckSum8 = 0;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    BOOL FlashMemMapBlockPageGetNext	( UINT32 dwBlock, UINT32 dwPage, UINT32 *pdwPageResult );

                                    tDirArray[eDir].tFileWritingRef.dwPageIndex++;
                                    // check if next page is not the start page.

                                    // check if page requires chunk stamp
                                }
                            }
                        }

                        fSuccess = !fIsError;
                    }
                }
            }
        }
	}

	return fSuccess;
}


void FileDirTest( void )
{
    BOOL                fSuccess = FALSE;
    FileDirectoryEnum   eDir;

    FileDirModuleInit();

    eDir = FILE_DIR_FILE_SYS_SIGNATURE;
    fSuccess = FileDirLoad( eDir, TRUE );
    fSuccess = FileDirIsLoaded( eDir );
    fSuccess = FileDirLoad( eDir, FALSE );
    fSuccess = FileDirIsLoaded( eDir );

    eDir = FILE_DIR_TEST;
    fSuccess = FileDirLoad( eDir, TRUE );
    fSuccess = FileDirIsLoaded( eDir );
    fSuccess = FileDirLoad( eDir, FALSE );
    fSuccess = FileDirIsLoaded( eDir );

    eDir = FILE_DIR_CONFIG_TEMP;
    fSuccess = FileDirLoad( eDir, TRUE );
    fSuccess = FileDirIsLoaded( eDir );
    //fSuccess = FileDirLoad( eDir, FALSE );
    //fSuccess = FileDirIsLoaded( eDir );


    FileWriteRefNewFileInputParams tParamsIn;
    tParamsIn.bExtension[0] ='t';
    tParamsIn.bExtension[1] ='x';
    tParamsIn.bExtension[2] ='t';
    tParamsIn.bHeaderChunkStampPageRate = 2;
    tParamsIn.fFileIsBin = FALSE;

    FILE_W_REF xFileRef = NULL;

    fSuccess = FileDirFileWriteOpenNew( FILE_DIR_CONFIG_TEMP, &tParamsIn, &xFileRef );


}

//////////////////////////////////////////////////////////////////////////////////////////////////
