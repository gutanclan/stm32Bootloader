
// C Library includes

#include <stdio.h>          // Standard I/O library

#include <string.h>         // String manipulation routines

#include "./types.h"
#include "./timer.h"
#include "./stateMachine.h"



BOOL StateMachineInit( StateMachineType *ptStateMachineState )
{
    BOOL fSuccess = FALSE;

    if( NULL != ptStateMachineState )
    {
        memset( ptStateMachineState, 0, STATE_MACHINE_STRUCT_SIZE_BYTES );

        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL StateMachineUpdate( StateMachineType *ptStateMachineState )
{
    BOOL fSuccess = FALSE;

    if( NULL != ptStateMachineState )
    {
        //indicates  if is first entry
        if( ptStateMachineState->bStatePrevious ==  ptStateMachineState->bStateCurrent )
        {
            ptStateMachineState->fStateIsFirstEntry = FALSE;
        }
        else
        {
            // reset variables
            ptStateMachineState->fTimeOutEnabled = FALSE;
            ptStateMachineState->xTimeOutTimer   = 0;
            // first entry
            ptStateMachineState->fStateIsFirstEntry = TRUE;
        }

        ptStateMachineState->bStatePrevious = ptStateMachineState->bStateCurrent;

        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL StateMachineIsFirtEntry( StateMachineType *ptStateMachineState )
{
    BOOL fIsFirtEntry = FALSE;

    if( NULL != ptStateMachineState )
    {
        fIsFirtEntry = ptStateMachineState->fStateIsFirstEntry;
    }

    return fIsFirtEntry;
}

BOOL StateMachineSetTimeOut( StateMachineType *ptStateMachineState, UINT32 dwStateTimeOut_mSec )
{
    BOOL fSuccess = FALSE;

    if( NULL != ptStateMachineState )
    {
        ptStateMachineState->fTimeOutEnabled = TRUE;
        ptStateMachineState->xTimeOutTimer   = TimerDownTimerStartMs( dwStateTimeOut_mSec );

        fSuccess = TRUE;
    }

    return fSuccess;
}



BOOL StateMachineIsTimeOut( StateMachineType *ptStateMachineState )
{
    BOOL fIsTimeOut = FALSE;

    if( NULL != ptStateMachineState )
    {
        if( ptStateMachineState->fTimeOutEnabled )
        {
            fIsTimeOut = TimerDownTimerIsExpired( ptStateMachineState->xTimeOutTimer );
        }
    }

    return fIsTimeOut;
}

BOOL StateMachineChangeState( StateMachineType *ptStateMachineState, UINT8 bNewState )
{
    BOOL fSuccess = FALSE;

    if( NULL != ptStateMachineState )
    {
        ptStateMachineState->bStateCurrent = bNewState;

        fSuccess = TRUE;
    }

    return fSuccess;
}
