//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Backup.h
//!    \brief       Backup SRAM module header.
//!
//!    \author      Puracom Inc.
//!    \date
//!
//!    \notes
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <string.h>

// STM Library includes
								// NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"		// Macro: assert_param() for other STM libraries.

// RTOS Library includes

// PCOM General Library includes
#include "Types.h"				// Several redefinitions of variables

// PCOM Project Targets

// PCOM Driver Library includes
#include "Backup.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Enums Structures and variables
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// NOTE!! Notice that this refers to "BKPSRAM"!!!!
//////////////////////////////////////////////////
#define BCKP_SRAM_ADDR	((uint32_t)0x40024000)
#define BCKP_SRAM_SIZE	((uint32_t)(0x40024FFF-0x40024000+1))    // 4K

static UINT8 *gpbBkpSRam = (UINT8 *)BCKP_SRAM_ADDR;

///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void BackupInitialize( void )
//!
//! \brief  Initialize the RCC clock for the SRAM
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BackupInitialize( void )
{
    // Initialize the RCC clock for the SRAM
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, ENABLE );

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BackupSramRead( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
//!
//! \brief  Read a section of the SRAM
//!
//! \param[in]  dwStartOffset   memory offset to start read/write. [0...(SRAM_SIZE-1)]
//!
//! \param[in]  vpBuffer        pointer to buffer
//!
//! \param[in]  dwBufferSize    size of the buffer. Should not exceed the size of SRAM_SIZE
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BackupSramRead( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess = TRUE;
    UINT8   *pbBuffer;

    if( vpBuffer != NULL )
    {
        // cast pointer
        pbBuffer = vpBuffer;

        // Sanitation (CAREFUL WITH THIS CONDITIONS!!)
        fSuccess &= ( dwBufferSize <= BCKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
        fSuccess &= ( ( dwStartOffset + dwBufferSize ) <= ( BCKP_SRAM_SIZE - 1) );       // data to write is inside sram area

        if( fSuccess )
        {
            memset( &pbBuffer[0], 0, dwBufferSize );

            memcpy( &pbBuffer[0], &gpbBkpSRam[dwStartOffset], dwBufferSize );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BackupSramWrite( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
//!
//! \brief  Write data from a buffer into the SRAM
//!
//! \param[in]  dwStartOffset   memory offset to start read/write. [0...(SRAM_SIZE-1)]
//!
//! \param[in]  vpBuffer        pointer to buffer
//!
//! \param[in]  dwBufferSize    size of the buffer. Should not exceed the size of SRAM_SIZE
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BackupSramWrite( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess = TRUE;
    UINT8   *pbBuffer;

    if( vpBuffer != NULL )
    {
        // cast pointer
        pbBuffer = vpBuffer;

        // Sanitation (CAREFUL WITH THIS CONDITIONS!!)
        fSuccess &= ( dwBufferSize <= BCKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
        fSuccess &= ( ( dwStartOffset + dwBufferSize ) <= ( BCKP_SRAM_SIZE - 1) );       // data to write is inside sram area

        if( fSuccess )
        {
            memcpy( &gpbBkpSRam[dwStartOffset], &pbBuffer[0], dwBufferSize );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BackupSramErase( UINT32 dwStartOffset, UINT32 dwEraseSize )
//!
//! \brief  Erase a specified amount of bytes in the sram memory
//!
//! \param[in]  dwStartOffset   memory offset to start read/write. [0...(SRAM_SIZE-1)]
//!
//! \param[in]  dwEraseSize     size =  amount of bytes to erase from the start offset
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BackupSramErase( UINT32 dwStartOffset, UINT32 dwEraseSize )
{
    BOOL    fSuccess = TRUE;

	// Sanitation (CAREFUL WITH THIS CONDITIONS!!)
	fSuccess &= ( dwEraseSize <= BCKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
	fSuccess &= ( ( dwStartOffset + dwEraseSize ) <= ( BCKP_SRAM_SIZE - 1) );       // data to write is inside sram area

	if( fSuccess )
	{
		memset( &gpbBkpSRam[dwStartOffset], 0, dwEraseSize );
	}

    return fSuccess;
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////

