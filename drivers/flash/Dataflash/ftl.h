//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	Flash Translation Layer
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FTL_H_
#define _FTL_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    FTL_MEMORY_1 = 0,
    
	FTL_MEMORY_MAX,
}FtlMemoryEnum;

// this functions only provide pass fail.
// error handling should be handled automatically in the .c file

BOOL  ftlModuleInit			( void );

BOOL  ftlMount				( FtlMemoryEnum eMem );
BOOL  ftlUnmount			( FtlMemoryEnum eMem );

BOOL  ftlIsBusy             ( FtlMemoryEnum eMem );

BOOL  ftlWrite              ( FtlMemoryEnum eMem, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength );
BOOL  ftlWriteInPage        ( FtlMemoryEnum eMem, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength );
BOOL  ftlWritePage          ( FtlMemoryEnum eMem, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength );

BOOL  ftlRead               ( FtlMemoryEnum eMem, UINT32 dwAddress, UINT8 *pbPageData, UINT32 dwLength );
BOOL  ftlReadPage           ( FtlMemoryEnum eMem, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength );

BOOL  ftlEraseSector        ( FtlMemoryEnum eMem, UINT16 wSector );
BOOL  ftlEraseSubsector     ( FtlMemoryEnum eMem, UINT16 wSubsector );
BOOL  ftlEraseAll           ( FtlMemoryEnum eMem );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _FTL_H_