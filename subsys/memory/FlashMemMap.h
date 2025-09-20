//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		FlashMemMap.h
//!	\brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FLASH_MEM_MAP_H_
#define _FLASH_MEM_MAP_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    UINT16  wCurrentIndex;
}FlashMemMapIndexType;

typedef struct
{
	FlashMemMapIndexType	tMemMapIndex;
    UINT8   				bFlashId;
    UINT32  				dwBlock;
}FlashMemMapBlockType;

typedef enum
{
    FLASH_MEM_MAP_FILE_SYS_INFO = 0,

    FLASH_MEM_MAP_TEST_1,

    FLASH_MEM_MAP_TEST_2,

    FLASH_MEM_MAP_MAX,
}FlashMemMapEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////

void    FlashMemMapInitModule    	( void );
UINT32  FlashMemMapSizeBytes        ( FlashMemMapEnum eMem );

// block size navigation
BOOL    FlashMemMapBlockGetFirst	( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlockResult );
BOOL    FlashMemMapBlockIsFirst		( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, BOOL *pfBlockIsFirstResult );
BOOL    FlashMemMapBlockGetLast		( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlockResult );
BOOL    FlashMemMapBlockIsLast		( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, BOOL *pfBlockIsFirstResult );
BOOL    FlashMemMapBlockGetNext		( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, FlashMemMapBlockType *ptBlockResult );
BOOL    FlashMemMapBlockGetPrev		( FlashMemMapEnum eMem, FlashMemMapBlockType *ptBlock, FlashMemMapBlockType *ptBlockResult );

// page size navigation within a block
BOOL FlashMemMapBlockPageGetFirst	( UINT32 dwBlock, UINT32 *pdwPageResult );
BOOL FlashMemMapBlockPageIsFirst	( UINT32 dwBlock, UINT32 dwPage, BOOL *pfIsPageFirstInBlockResult );
BOOL FlashMemMapBlockPageGetLast	( UINT32 dwBlock, UINT32 *pdwPageResult );
BOOL FlashMemMapBlockPageIsLast		( UINT32 dwBlock, UINT32 dwPage, BOOL *pfIsPageLastInBlockResult );
BOOL FlashMemMapBlockPageGetNext	( UINT32 dwBlock, UINT32 dwPage, UINT32 *pdwPageResult );
BOOL FlashMemMapBlockPageGetPrev	( UINT32 dwBlock, UINT32 dwPage, UINT32 *pdwPageResult );

// void FlashMemMapBlockTest		    ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _FLASH_MEM_MAP_H_

//! @}

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
