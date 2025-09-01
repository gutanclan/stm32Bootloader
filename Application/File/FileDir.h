//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		FileDir.h
//!	\brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FILE_DIR_H_
#define _FILE_DIR_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef void * FILE_W_REF;
// typedef UINT32   TIMER;

typedef struct
{
	BOOL	fFileIsBin;
	UINT8   bExtension[3];
	UINT8   bHeaderChunkStampPageRate;
}__attribute__((__packed__)) FileWriteRefNewFileInputParams;

typedef enum
{
    // Contains File system information
    FILE_DIR_FILE_SYS_SIGNATURE = 0,

    // Test
    FILE_DIR_TEST,

    // Configuration Temp
    FILE_DIR_CONFIG_TEMP,

    FILE_DIR_MAX,
}FileDirectoryEnum;

typedef enum
{
    FILE_DIR_WRITE_TYPE_LINEAR = 0,
    FILE_DIR_WRITE_TYPE_CIRCULAR,

    FILE_DIR_WRITE_TYPE_MAX
}FileDirectoryWriteTypeEnum;

// Sets Directories to default state(uninitialized)
BOOL 	FileDirModuleInit           ( void );

// check whole directory to find the last file that got closed or incomplete (in the case of circular directory. for series then the first page should contain file header info)
// to start writing from there.
BOOL 	FileDirLoad            	    ( FileDirectoryEnum eDir, BOOL fLoad );
BOOL 	FileDirIsLoaded        	    ( FileDirectoryEnum eDir );

UINT32 	FileDirTimeStampToEpoch	    ( FileHeaderTimeStamp *ptFileDateTime );
UINT8   FileDirCheckSum8            ( UINT8 bCurrentCheckSum8, UINT8 * pvBuffer, UINT32 dwBufferSize );

//////////////////////////////////////
// WRITE FUNCTIONS
// for write operations, only 1 instance is allowed per directory.
BOOL    FileDirFileWriteOpenNew     ( FileDirectoryEnum eDir, FileWriteRefNewFileInputParams *ptNewFileWriteParams, FILE_W_REF * ptFileRef );
BOOL    FileDirFileWriteAppendData  ( FileDirectoryEnum eDir, FILE_W_REF * ptFileRef, void *pvBuffer,UINT16 wBytesToSend, UINT16 *pwBytesSent );
BOOL    FileDirFileWriteClose       ( FileDirectoryEnum eDir, FILE_W_REF * ptFileRef );


// READ FUNCTIONS
//BOOL    FileDirFileReadOpen         ( FileDirectoryEnum eDir );
//BOOL    FileDirFileReadGetNextData  ( FileDirectoryEnum eDir );
//////////////////////////////////////

void 	FileDirTest				    ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _FILE_HEADER_H_

