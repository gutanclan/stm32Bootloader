//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		Flash.h
//!	\brief
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _FLASH_H_
#define _FLASH_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: This Generic Flash driver assumes all the flash chips are the same component of same sizes of memory.
//////////////////////////////////////////////////////////////////////////////////////////////////

#define FLASH_NUM_BLOCKS                (NAND_SIMU_NUM_SUBSECTORS)
#define FLASH_NUM_PAGES                 (NAND_SIMU_NUM_PAGES)
#define FLASH_NUM_BYTES_TOT             (NAND_SIMU_NUM_BYTES_TOT)

#define FLASH_PAGE_SIZE_BYTES           (NAND_SIMU_PAGE_SIZE_BYTES)
#define FLASH_BLOCK_SIZE_BYTES		    (NAND_SIMU_SUBSECTOR_SIZE_BYTES)

#define FLASH_PAGES_PER_BLOCK		    (FLASH_BLOCK_SIZE_BYTES/FLASH_PAGE_SIZE_BYTES)

#define FLASH_BLOCK_MAX       			(NAND_SIMU_NUM_SUBSECTORS-1)
#define FLASH_BLOCK_MIN       			(0)

typedef enum
{
    // FLASH_ID_NOT_USED       = -1,

    FLASH_ID_NAND_SIMU_1    = 0,
	FLASH_ID_NAND_SIMU_2,

    FLASH_MAX,
}FlashIdEnum;

void   FlashInitModule      ( void );

// WRITE
BOOL FlashWriteAtPageOffset ( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT16 wPageByteOffset, UINT8 *pbDataBuffer, UINT32 dwDataLength );

// READ
BOOL FlashReadPage			( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize );

// ERASE
BOOL FlashErasePage    		( FlashIdEnum eFlashId, UINT32 dwPage, UINT8 *pbRecreateBlockBuffer, UINT32 dwRecreateBlockBufferSize );
BOOL FlashEraseBlock   		( FlashIdEnum eFlashId, UINT16 wBlock );
BOOL FlashEraseAll          ( FlashIdEnum eFlashId );

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _FLASH_H_

//! @}

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
