//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        NandSimu.h
//!    \brief       Generic Nand Flash Driver.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NAND_SIMU_H
#define NAND_SIMU_H

//////////////////////////////////////////////////////////////////////////////////////////////////

#define NAND_SIMU_NUM_SUBSECTORS             (10)     /* 4096 subsectors of 4kBytes */
#define NAND_SIMU_NUM_PAGES                  (40)     /* 65536 pages of 256 bytes */
#define NAND_SIMU_NUM_BYTES_TOT              (10240)  /* 128 MBits => 16MBytes */

#define NAND_SIMU_PAGE_SIZE_BYTES            (256)    /* 65536 pages of 256 bytes */
#define NAND_SIMU_SUBSECTOR_SIZE_BYTES       (1024)   /* 4096 subsectors of 4kBytes */


//#define NAND_SIMU_NUM_SUBSECTORS             (3)     /* 4096 subsectors of 4kBytes */
//#define NAND_SIMU_NUM_PAGES                  (9)     /* 65536 pages of 256 bytes */
//#define NAND_SIMU_NUM_BYTES_TOT              (18)  /* 128 MBits => 16MBytes */
//
//#define NAND_SIMU_PAGE_SIZE_BYTES            (2)    /* 65536 pages of 256 bytes */
//#define NAND_SIMU_SUBSECTOR_SIZE_BYTES       (6)   /* 4096 subsectors of 4kBytes */

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    NAND_SIMU_ARRAY_ID_1 = 0,
	NAND_SIMU_ARRAY_ID_2,

    NAND_SIMU_ARRAY_ID_MAX,
} NandSimuArrayIdEnum;

#define NAND_SIMU_ARRAY_ID_INVALID (NAND_SIMU_ARRAY_ID_MAX)

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL NandSimuModuleInit		    ( void );

// WRITE
BOOL NandSimuWriteAtPage        ( NandSimuArrayIdEnum eArrId, UINT32 dwPage, UINT8 *pbDataBuffer, UINT32 dwDataLength );
BOOL NandSimuWriteAtPageOffset  ( NandSimuArrayIdEnum eArrId, UINT32 dwPage, UINT16 wPageOffset, UINT8 *pbDataBuffer, UINT32 dwDataLength );

// READ
BOOL NandSimuReadAtByteAddress  ( NandSimuArrayIdEnum eArrId, UINT32 dwByteAddress, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize );

// ERASE
BOOL NandSimuEraseAll			( NandSimuArrayIdEnum eArrId );
BOOL NandSimuEraseSubsector		( NandSimuArrayIdEnum eArrId, UINT16 wSubsector );

#endif /* NAND_SIMU_H */


//////////////////////////////////////////////////////////////////////////////////////////////////
