//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        BKP_SRAM.h
//!    \brief       BKP_SRAM module header.
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
#include "BkpSram.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Enums Structures and variables
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// NOTE!! Notice that this refers to "BKPSRAM"!!!!
//////////////////////////////////////////////////

static UINT8 *gpbBkpSram = (UINT8 *)BKP_SRAM_START_ADDR;

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
BOOL BkpSramEnable( BOOL fEnable )
{
	if( fEnable )
	{

	    // Enable the PWR clock
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
        // Enable access to the backup domain
        PWR_BackupAccessCmd(ENABLE);

        // Enable the Backup SRAM low power Regulator to retain it's content in VBAT mode
        //PWR_BackupRegulatorCmd(ENABLE);
        /* Wait until the Backup SRAM low power Regulator is ready */
        //while(PWR_GetFlagStatus(PWR_FLAG_BRR) == RESET){}

        // Enable backup SRAM Clock
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);



		// Initialize the RCC clock for the SRAM
		// RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, ENABLE );
	}
	else
	{
	    // Enable access to the backup domain
        PWR_BackupAccessCmd(DISABLE);

		RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, DISABLE );
	}

	return TRUE;
}

BOOL BkpSramIsEnabled( void )
{
	BOOL fEnabled = FALSE;

	if
	(
		// Enable the PWR clock
		( ( RCC->APB1ENR & RCC_APB1Periph_PWR ) > 0 ) &&
		// Enable access to the backup domain
		( ( ( PWR->CR >> 8 ) & 1 ) > 0 ) &&
		// Enable backup SRAM Clock
		( ( RCC->AHB1ENR & RCC_AHB1Periph_BKPSRAM ) > 0 )
	)
	{
		fEnabled = TRUE;
	}

	return fEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BkpSramRead( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
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
BOOL BkpSramRead( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess = FALSE;
    UINT8   *pbBuffer;

    if( BkpSramIsEnabled() )
    {
        if( vpBuffer != NULL )
        {
            // cast pointer
            pbBuffer = vpBuffer;

            // Sanitation (CAREFUL WITH THIS CONDITIONS!!)
            fSuccess = TRUE;
            fSuccess &= ( dwBufferSize <= BKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
            fSuccess &= ( ( dwStartOffset + dwBufferSize ) <= ( BKP_SRAM_SIZE - 1) );       // data to write is inside sram area

            if( fSuccess )
            {
                memset( &pbBuffer[0], 0, dwBufferSize );

                memcpy( &pbBuffer[0], &gpbBkpSram[dwStartOffset], dwBufferSize );
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BkpSramWrite( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
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
BOOL BkpSramWrite( UINT32 dwStartOffset, void *vpBuffer, UINT32 dwBufferSize )
{
    BOOL    fSuccess = FALSE;
    UINT8   *pbBuffer;

    if( BkpSramIsEnabled() )
    {
        if( vpBuffer != NULL )
        {
            // cast pointer
            pbBuffer = vpBuffer;

            // Sanitation (CAREFUL WITH THIS CONDITIONS!!)
            fSuccess = TRUE;
            fSuccess &= ( dwBufferSize <= BKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
            fSuccess &= ( ( dwStartOffset + dwBufferSize ) <= ( BKP_SRAM_SIZE - 1) );       // data to write is inside sram area

            if( fSuccess )
            {
                memcpy( &gpbBkpSram[dwStartOffset], &pbBuffer[0], dwBufferSize );
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL BkpSramErase( UINT32 dwStartOffset, UINT32 dwEraseSize )
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
BOOL BkpSramErase( UINT32 dwStartOffset, UINT32 dwEraseSize )
{
    BOOL    fSuccess = FALSE;

    if( BkpSramIsEnabled() )
    {
        // Sanitation (CAREFUL WITH THIS CONDITIONS!!)
        fSuccess = TRUE;
        fSuccess &= ( dwEraseSize <= BKP_SRAM_SIZE );                                  // Buffer size smaller than the size of sram
        fSuccess &= ( ( dwStartOffset + dwEraseSize ) <= ( BKP_SRAM_SIZE - 1) );       // data to write is inside sram area

        if( fSuccess )
        {
            memset( &gpbBkpSram[dwStartOffset], 0, dwEraseSize );
        }
    }

    return fSuccess;
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////

