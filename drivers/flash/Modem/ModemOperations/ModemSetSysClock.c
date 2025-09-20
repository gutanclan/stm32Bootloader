/** C Header ******************************************************************

*******************************************************************************/

#include <assert.h>
#include <string.h>     /* memset() */
#include <stdio.h>      /* printf() */
#include <ctype.h>      /* isprint() */
#include <stdlib.h>

#include <stdarg.h>         // For va_arg support

#include "Types.h"

// PCOM Project Targets
#include "Target.h"             // Hardware target specifications

#include "Asc.h"
#include "Config.h"
#include "Debug.h"
#include "General.h"
#include "Gpio.h"

//#include "Modem.h"
//#include "Module.h"     /* Common task control structure definition */
#include "StringTable.h"
#include "Timer.h"
#include "Usart.h"
#include "Datalog.h"
#include "Control.h"
#include "Console.h"


#include "ModemPower.h"
#include "ModemConnect.h"
#include "ModemSetSysClock.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

typedef enum
{
    MODEM_SET_SYS_CLOCK_STATE_UNINITIALIZED = 0,
    
    MODEM_SET_SYS_CLOCK_STATE_DISABLED,
    
    MODEM_SET_SYS_CLOCK_STATE_IDLE,
    
    MODEM_SET_SYS_CLOCK_STATE_START,
    MODEM_SET_SYS_CLOCK_STATE_RUN_GET_TIME,
    MODEM_SET_SYS_CLOCK_STATE_END,
    
    MODEM_SET_SYS_CLOCK_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemSetSysClkStateMachineName[MODEM_SET_SYS_CLOCK_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "setSysClk Disabled",
    
    "setSysClk Idle",
    
    "setSysClk Start",
    "setSysClk get time",
    "setSysClk End",
};


////////////////////////////////////////////////////////////////////////////////////////////////////

//
static ModemDataType                   *gpModemData;

// operation
static BOOL                             gfIsWaitingForNewCommand;
static ModemStateMachineType            gtStateMachine;
static ModemCommandSemaphoreEnum        geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////
    
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemSetSysClockInit( void )
{
    // set state machine to a initial state.
    StateMachineInit( &gtStateMachine.tState );       
    
    gpModemData = ModemResponseModemDataGetPtr();
    
    if( gpModemData == NULL )
    {
        // catch this bug on development time
        while(1);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemSetSysClockSetDeltaTimeCorrection( UINT8 bDeltaSecondsTimeCorrection )
{
    if( gpModemData != NULL )
    {
        gpModemData->tSysClock.bDeltaSecondsConfigured = bDeltaSecondsTimeCorrection;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemSetSysClockRun( void )
{
    if( gfIsWaitingForNewCommand )
    {
        // indicate command will start running so more commands are not accepted
        gfIsWaitingForNewCommand = FALSE;
        
        StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_START );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemSetSysClockIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemSetSysClockIsTimeCorrected( void )
{
    return gpModemData->tSysClock.fIsTimeUpdated;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemSetSysClockStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemSetSysClkStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {
        case MODEM_SET_SYS_CLOCK_STATE_UNINITIALIZED:
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_DISABLED );
            break;
        }
        
        case MODEM_SET_SYS_CLOCK_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            // not allowed to run operation
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand                = FALSE;
            gpModemData->tSysClock.fIsTimeUpdated   = FALSE;
        
            if( ModemConnectIsConnected() )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_IDLE );
            }
            break;
        }
        
        case MODEM_SET_SYS_CLOCK_STATE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand    = TRUE;
            
            if( ModemConnectIsConnected() == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_SET_SYS_CLOCK_STATE_START:
        {
            gpModemData->tSysClock.fIsTimeUpdated = FALSE;
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_RUN_GET_TIME );
            break;
        }
        
        case MODEM_SET_SYS_CLOCK_STATE_RUN_GET_TIME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CCLK:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    
                    ModemCommandProcessorSendAtCommand( "#CCLK?" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "sysClk[%s] Error='%s'", 
                        cModemSetSysClkStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_END );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_END );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_END );
                }
                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                break;
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                        "sysClk[%s] Error='%s'", 
                        cModemSetSysClkStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_END );
            }
            break;
        }
        
        case MODEM_SET_SYS_CLOCK_STATE_END:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_IDLE );
            break;
        }
        
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_SET_SYS_CLOCK_STATE_UNINITIALIZED );            
            break;
        }
    }
}

ModemDataSysClockType  * ModemSetSysClockGetInfoPtr       ( void )
{
    return &gpModemData->tSysClock;
}