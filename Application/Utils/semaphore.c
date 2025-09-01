//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        queue.h
//!    \brief       queue module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "./Types.h"
#include "./timer.h"
#include "./semaphore.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    volatile BOOL fIsSemaphoreTaken;
}SemaphoreType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// This table must be in the same order as SemaphoreEnum in semaphore.h
static volatile SemaphoreType gtSemaphoreLookupArray[] =
{
	// SEMAPHORE_USART3_RX
	FALSE,
	// SEMAPHORE_USART3_TX
	FALSE,
	// SEMAPHORE_CONSOLE_OUTPUT_MAIN,
	FALSE,
	// SEMAPHORE_NAND_SIMU_ID_1,
	FALSE,
	// SEMAPHORE_NAND_SIMU_ID_2,
	FALSE,
};

#define SEMAPHORE_ARRAY_MAX	( sizeof(gtSemaphoreLookupArray) / sizeof( gtSemaphoreLookupArray[0] ) )

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL SemaphoreModuleInit( void )
//!
//! \brief  Initialize semaphore variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SemaphoreModuleInit( void )
{
    BOOL fSuccess = FALSE;

    if( SEMAPHORE_MAX != SEMAPHORE_ARRAY_MAX )
    {
        // definition mismatch error
        while(0);
    }
    else
    {
        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL SemaphoreTake( SemaphoreEnum eSemaphore, UINT32 dwWaitingTime_mSec )
//!
//! \brief  function to obtain a semaphore
//!
//! \param[in]  eSemaphore      	Semaphore selected
//! \param[in]  dwWaitingTime_mSec  mSec to wait for semaphore to be available
//!
//! \return
//!         - TRUE if the semaphore was obtained. FALSE if dwWaitingTime_mSec expired without the
//!				semaphore becoming available.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SemaphoreTake( SemaphoreEnum eSemaphore )
{
    BOOL 	fIsSemaphoreObtained 	= FALSE;

	if( eSemaphore < SEMAPHORE_ARRAY_MAX )
	{
        if( gtSemaphoreLookupArray[eSemaphore].fIsSemaphoreTaken == FALSE )
        {
            // update semaphore status
            gtSemaphoreLookupArray[eSemaphore].fIsSemaphoreTaken = TRUE;

            // update result
            fIsSemaphoreObtained = TRUE;
        }
	}

    return fIsSemaphoreObtained;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL SemaphoreGive( SemaphoreEnum eSemaphore )
//!
//! \brief  return the semaphore
//!
//! \param[in]  eSemaphore      Semaphore selected
//!
//! \return
//!         - TRUE if is free
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SemaphoreGive( SemaphoreEnum eSemaphore )
{
    BOOL 	fIsSemaphoreReturned 	= FALSE;

	if( eSemaphore < SEMAPHORE_ARRAY_MAX )
	{
		if( gtSemaphoreLookupArray[eSemaphore].fIsSemaphoreTaken )
		{
			// update semaphore status
			gtSemaphoreLookupArray[eSemaphore].fIsSemaphoreTaken = FALSE;

			// update result ( if semaphore is not used indicate semaphore is free )
            fIsSemaphoreReturned = TRUE;
		}
	}

    return fIsSemaphoreReturned;
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
//

