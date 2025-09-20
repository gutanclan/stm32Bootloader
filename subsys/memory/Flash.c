//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Flash.h
//!    \brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "types.h"

#include "../../drivers/flash/FlashMem/NandSimu.h"
#include "Flash.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    // flash information
	UINT8  	bFlashId;
}FlashType;

static BOOL (*f_ptr_WriteAtPageOffst)   ( UINT8 bFlashId, UINT32 dwPageNumber, UINT16 wPageByteOffset, UINT8 *pbDataBuffer, UINT32 dwDataLength );
static BOOL (*f_ptr_ReadAtByte)         ( UINT8 bFlashId, UINT32 dwByteAddress, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize );
static BOOL (*f_ptr_EraseBlock)         ( UINT8 bFlashId, UINT16 wBlock );
static BOOL (*f_ptr_EraseAll)           ( UINT8 bFlashId );

static const FlashType  gtFlashArray[] =
{
    // NAND_SIMU_ARRAY_ID_INVALID,
	NAND_SIMU_ARRAY_ID_1,
	NAND_SIMU_ARRAY_ID_2,
};

#define FLASH_AMOUNT     ( sizeof(gtFlashArray) / sizeof( gtFlashArray[0] ) )


//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void FlashInitModule( void )
{
    if( FLASH_AMOUNT != FLASH_MAX )
    {
        while(1)
        {
            // catch this bug during development time
        }
    }

	f_ptr_WriteAtPageOffst  = &NandSimuWriteAtPageOffset;
	f_ptr_ReadAtByte	    = &NandSimuReadAtByteAddress;
	f_ptr_EraseBlock	    = &NandSimuEraseSubsector;
	f_ptr_EraseAll		    = &NandSimuEraseAll;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashIsFlashValid( FlashIdEnum eFlashId )
{
	//if( ( eFlashId > FLASH_ID_NOT_USED ) && ( eFlashId < FLASH_AMOUNT ) )
    if( eFlashId < FLASH_AMOUNT )
	{
		return TRUE;
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
BOOL FlashWriteAtPageOffset( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT16 wPageByteOffset, UINT8 *pbDataBuffer, UINT32 dwDataLength )
{
	BOOL fSuccess = FALSE;

    if( FlashIsFlashValid( eFlashId ) )
    {
		if( dwPageNumber < FLASH_NUM_PAGES )
		{
			fSuccess = (*f_ptr_WriteAtPageOffst)( gtFlashArray[eFlashId].bFlashId, dwPageNumber, wPageByteOffset, pbDataBuffer, dwDataLength );
		}
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashReadPage( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize )
{
    BOOL fSuccess = FALSE;

    if( FlashIsFlashValid( eFlashId ) )
    {
		if( dwPageNumber < FLASH_NUM_PAGES )
		{
			fSuccess = (*f_ptr_ReadAtByte)
			(
				gtFlashArray[eFlashId].bFlashId,
				dwPageNumber*FLASH_PAGE_SIZE_BYTES, // convert page address to byte address
				dwBytesToRead,
				pbDataBuffer,
				dwBufferSize
			);
		}
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashErasePage( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT8 *pbRecreateBlockBuffer, UINT32 dwRecreateBlockBufferSize )
{
	BOOL 	fSuccess 			= FALSE;
	UINT32 	dwSs 				= 0;
	UINT32 	dwPageOffsetIntoSs 	= 0;
	UINT32 	dwSs1stPageNumber 	= 0;

    if( FlashIsFlashValid( eFlashId ) )
    {
		if( dwPageNumber < FLASH_NUM_PAGES )
		{
			if
			(
				( pbRecreateBlockBuffer != NULL ) &&
				( dwRecreateBlockBufferSize == FLASH_BLOCK_SIZE_BYTES )
			)
			{
				dwSs 				= dwPageNumber / FLASH_PAGES_PER_BLOCK;
				dwPageOffsetIntoSs 	= dwPageNumber % (FLASH_PAGES_PER_BLOCK);
				dwSs1stPageNumber	= dwSs * FLASH_PAGES_PER_BLOCK;

				// read block
				fSuccess = (*f_ptr_ReadAtByte)
				(
					gtFlashArray[eFlashId].bFlashId,
					(dwSs*FLASH_BLOCK_SIZE_BYTES), // convert page address to byte address
					dwRecreateBlockBufferSize,
					&pbRecreateBlockBuffer[0],
					dwRecreateBlockBufferSize
				);

				// erase block
				fSuccess &= FlashEraseBlock( eFlashId, dwSs );

				// erase page in block buffer
				memset( &pbRecreateBlockBuffer[(dwPageOffsetIntoSs*FLASH_PAGE_SIZE_BYTES)], 0xFF, FLASH_PAGE_SIZE_BYTES );

                UINT32 dwPage;
                UINT32 dwBufferByteOffset;
				// write edited block buffer back to flash
				for( UINT32 dwPageAtBlock = 0; dwPageAtBlock < FLASH_PAGES_PER_BLOCK; dwPageAtBlock++ )
				{
				    dwPage              = (dwSs1stPageNumber + dwPageAtBlock);
				    dwBufferByteOffset  = (dwPageAtBlock*FLASH_PAGE_SIZE_BYTES);
					fSuccess &= (*f_ptr_WriteAtPageOffst)( gtFlashArray[eFlashId].bFlashId, dwPage, 0, &pbRecreateBlockBuffer[dwBufferByteOffset], FLASH_PAGE_SIZE_BYTES );
				}
			}
		}
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashEraseBlock( FlashIdEnum eFlashId, UINT16 wBlock )
{
    BOOL fSuccess = FALSE;

    if( FlashIsFlashValid( eFlashId ) )
    {
        if
        (
            ( wBlock <= FLASH_BLOCK_MAX ) &&
            ( wBlock >= FLASH_BLOCK_MIN )
        )
        {
            fSuccess = (*f_ptr_EraseBlock)( gtFlashArray[eFlashId].bFlashId, wBlock );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FlashEraseAll( FlashIdEnum eFlashId )
{
	BOOL fSuccess = FALSE;

	if( FlashIsFlashValid( eFlashId ) )
	{
		fSuccess = (*f_ptr_EraseAll)( gtFlashArray[eFlashId].bFlashId );
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
