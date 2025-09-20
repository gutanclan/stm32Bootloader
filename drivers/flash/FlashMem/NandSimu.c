//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        NandSimu.h
//!    \brief       Generic Nand Flash Driver.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "types.h"

#include "NandSimu.h"

#include "../../common/semaphore.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

//typedef UINT8 *         SEMAPHORE_KEY_HOLDER;
//static volatile BOOL    gfSemaphoreIsOperationLocked    = FALSE;
//static const UINT8      gbSemaphoreOperationLockKeyVal  = 0xAF;

static UINT8 bNandMem[NAND_SIMU_ARRAY_ID_MAX][NAND_SIMU_NUM_BYTES_TOT];

typedef struct
{
	CHAR	*		pcName;
    SemaphoreEnum   eSemaphoreId;
}NandSimuStruct;

static NandSimuStruct tNandSimuArray[] =
{
    // Name			eSemaphoreId
    { "NAND_SIM1", 	SEMAPHORE_NAND_SIMU_ID_1 },
	{ "NAND_SIM2", 	SEMAPHORE_NAND_SIMU_ID_2 },
};

#define NAND_SIMU_ARRAY_MAX     ( sizeof(tNandSimuArray) / sizeof( tNandSimuArray[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL NandSimuModuleInit( void )
{
    NandSimuEraseAll(NAND_SIMU_ARRAY_ID_1);
    NandSimuEraseAll(NAND_SIMU_ARRAY_ID_2);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!     Write bytes at page. It can start at page offset.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NandSimuWriteAtPageOffset( NandSimuArrayIdEnum eArrId, UINT32 dwPage, UINT16 wPageOffset, UINT8 *pbDataBuffer, UINT32 dwDataLength )
{
    BOOL                    fSuccess = FALSE;
    UINT8                   bStatus;

    if( eArrId < NAND_SIMU_ARRAY_MAX )
    {
        if
        (
            ( dwPage < NAND_SIMU_NUM_PAGES ) &&
            ( pbDataBuffer != NULL ) &&
            ( dwDataLength > 0 ) &&
            ( wPageOffset <= 255 ) &&
            ( dwDataLength <= NAND_SIMU_PAGE_SIZE_BYTES )
        )
        {
            // make sure the data to write fits withing the page range
            if( ( ( wPageOffset + dwDataLength ) - 1 ) <= NAND_SIMU_PAGE_SIZE_BYTES )
            {
                ////////////////////////////////////////////
                if( SemaphoreTake( tNandSimuArray[eArrId].eSemaphoreId ) )
                {
                    UINT32 dwPageByteStart 	= dwPage * NAND_SIMU_PAGE_SIZE_BYTES;
                    dwPageByteStart = dwPageByteStart + wPageOffset;
                    UINT32 dwPageByteEnd 	= ( dwPageByteStart + dwDataLength ) - 1;
                    UINT32 dwBuffCounter 	= 0;

                    for( UINT32 dwByte = dwPageByteStart ; dwByte <= dwPageByteEnd ; dwByte++ )
                    {
                        bNandMem[eArrId][dwByte] &= pbDataBuffer[dwBuffCounter];
                        dwBuffCounter++;
                    }

                    SemaphoreGive( tNandSimuArray[eArrId].eSemaphoreId );
                }

                fSuccess = TRUE;
                ////////////////////////////////////////////
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NandSimuWriteAtPage( NandSimuArrayIdEnum eArrId, UINT32 dwPage, UINT8 *pbDataBuffer, UINT32 dwDataLength )
{
    BOOL                    fSuccess = FALSE;
    UINT8                   bStatus;

    if( eArrId < NAND_SIMU_ARRAY_MAX )
    {
        if
        (
            ( dwPage < NAND_SIMU_NUM_PAGES ) &&
            ( pbDataBuffer != NULL ) &&
            ( dwDataLength > 0 ) &&
            ( dwDataLength <= NAND_SIMU_PAGE_SIZE_BYTES )
        )
        {
            ////////////////////////////////////////////
            if( SemaphoreTake( tNandSimuArray[eArrId].eSemaphoreId ) )
			{
				UINT32 dwPageByteStart 	= dwPage * NAND_SIMU_PAGE_SIZE_BYTES;
				UINT32 dwPageByteEnd 	= dwPageByteStart + dwDataLength - 1;
				UINT32 dwBuffCounter 	= 0;

				for( UINT32 dwByte = dwPageByteStart ; dwByte <= dwPageByteEnd ; dwByte++ )
				{
					bNandMem[eArrId][dwByte] &= pbDataBuffer[dwBuffCounter];
					dwBuffCounter++;
				}

				SemaphoreGive( tNandSimuArray[eArrId].eSemaphoreId );
			}

			fSuccess = TRUE;
            ////////////////////////////////////////////
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NandSimuReadAtByteAddress( NandSimuArrayIdEnum eArrId, UINT32 dwByteAddress, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize )
{
    BOOL                    fSuccess = FALSE;
    UINT32                  dwBytesToStoreInBuffer;

    if( eArrId < NAND_SIMU_ARRAY_MAX )
    {
        if
        (
            ( dwByteAddress < NAND_SIMU_NUM_BYTES_TOT ) &&
            ( pbDataBuffer != NULL ) &&
            ( dwBufferSize > 0 ) &&
            ( dwBytesToRead > 0 ) &&
            // byte offset + data is inside writeable range
            ( ( (dwByteAddress-1) + dwBytesToRead ) < NAND_SIMU_NUM_BYTES_TOT )
        )
        {
            ////////////////////////////////////////////
            if( SemaphoreTake( tNandSimuArray[eArrId].eSemaphoreId ) )
			{
				if( dwBytesToRead > dwBufferSize )
				{
					dwBytesToRead = dwBufferSize;
				}

				memcpy( pbDataBuffer, &bNandMem[eArrId][dwByteAddress], dwBytesToRead );

				SemaphoreGive( tNandSimuArray[eArrId].eSemaphoreId );
			}

			fSuccess = TRUE;
            ////////////////////////////////////////////
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NandSimuEraseAll( NandSimuArrayIdEnum eArrId )
{
    BOOL                    fSuccess = FALSE;
    UINT8                   bStatus;

    if( eArrId < NAND_SIMU_ARRAY_MAX )
    {
        ////////////////////////////////////////////
        if( SemaphoreTake( tNandSimuArray[eArrId].eSemaphoreId ) )
        {
			for( UINT32 dwByte = 0 ; dwByte < NAND_SIMU_NUM_BYTES_TOT ; dwByte++ )
			{
				bNandMem[eArrId][dwByte] = 0xff;
			}

            SemaphoreGive( tNandSimuArray[eArrId].eSemaphoreId );

            fSuccess = TRUE;
        }
        ////////////////////////////////////////////
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL NandSimuEraseSubsector( NandSimuArrayIdEnum eArrId, UINT16 wSubsector )
{
    BOOL                    fSuccess = FALSE;
    UINT8                   bStatus;

    if( eArrId < NAND_SIMU_ARRAY_MAX )
    {
        if( wSubsector < NAND_SIMU_NUM_SUBSECTORS )
        {
            ////////////////////////////////////////////
            if( SemaphoreTake( tNandSimuArray[eArrId].eSemaphoreId ) )
			{
				UINT32 dwSubsectorByteStart = wSubsector * NAND_SIMU_SUBSECTOR_SIZE_BYTES;
				UINT32 dwSubsectorByteEnd 	= ( wSubsector * NAND_SIMU_SUBSECTOR_SIZE_BYTES ) + NAND_SIMU_SUBSECTOR_SIZE_BYTES - 1 ;

				for( UINT32 dwByte = dwSubsectorByteStart ; dwByte <= dwSubsectorByteEnd ; dwByte++ )
				{
					bNandMem[eArrId][dwByte] = 0xff;
				}

				SemaphoreGive( tNandSimuArray[eArrId].eSemaphoreId );

				fSuccess = TRUE;
			}
            ////////////////////////////////////////////
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////


