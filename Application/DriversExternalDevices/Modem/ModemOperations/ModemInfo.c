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
#include "ModemInfo.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

typedef enum
{
    MODEM_INFO_STATE_UNINITIALIZED = 0,
    
    MODEM_INFO_STATE_DISABLED,
    MODEM_INFO_STATE_IDLE,
    
    MODEM_INFO_STATE_START,
    MODEM_INFO_STATE_RUN_MANUF_NAME,
    MODEM_INFO_STATE_RUN_MODEL,
    MODEM_INFO_STATE_RUN_FW_VER,
    MODEM_INFO_STATE_RUN_CGSN,
    
    MODEM_INFO_STATE_PASS,
    MODEM_INFO_STATE_FAIL,
    MODEM_INFO_STATE_END,
    
    MODEM_INFO_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemInfoStateMachineName[MODEM_INFO_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "Info Disabled",
    "Info Idle",
    
    "Info Start",
    "Get Manuf Name",
    "Get Model",
    "Get Fw Ver",
    "Get Imei",
    
    "Info Pass",
    "Info Fail",
    "Info End",
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
static BOOL                     gfIsInfoExtracted;

//
static ModemDataType   *gpModemData;

// operation
static BOOL                     gfIsWaitingForNewCommand;
static ModemStateMachineType    gtStateMachine;
static ModemCommandSemaphoreEnum geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemInfoInit( void )
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
void ModemInfoRun( void )
{
    if( gfIsWaitingForNewCommand )
    {
        // indicate command will start running so more commands are not accepted
        gfIsWaitingForNewCommand = FALSE;
        
        StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_START );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemInfoIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemInfoIsExtracted( void )
{
    return gfIsInfoExtracted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemInfoStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {        
        case MODEM_INFO_STATE_UNINITIALIZED :
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_DISABLED );
            break;
        }
            
        case MODEM_INFO_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            // not allowed to run config operation
            //////////////////////////////////////////////////
        
            gfIsWaitingForNewCommand    = FALSE;
        
            if( ModemPowerIsPowerEnabled() )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_IDLE );
            }
            break;
        }
        
        case MODEM_INFO_STATE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand    = TRUE;
            
            if( ModemPowerIsPowerEnabled() == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_INFO_STATE_START:
        {
            gfIsInfoExtracted = FALSE;
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_RUN_MANUF_NAME );
            break;
        }
        
        case MODEM_INFO_STATE_RUN_MANUF_NAME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CGMI:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    ModemCommandProcessorSendAtCommand( "#CGMI" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_RUN_MODEL );
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
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
            }
            break;
        }
            
        case MODEM_INFO_STATE_RUN_MODEL:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CGMM:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    ModemCommandProcessorSendAtCommand( "#CGMM" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_RUN_FW_VER );
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
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
            }
            break;
        }
            
        case MODEM_INFO_STATE_RUN_FW_VER:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CGMR:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    ModemCommandProcessorSendAtCommand( "#CGMR" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_RUN_CGSN );
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
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
            }
            break;
        }
        
        
        case MODEM_INFO_STATE_RUN_CGSN:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CGSN:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    ModemCommandProcessorSendAtCommand( "#CGSN" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_PASS );
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
                        "info[%s] Error='%s'", 
                        cModemInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_FAIL );
            }
            break;
        }
        
        case MODEM_INFO_STATE_PASS:
        {   
            gfIsInfoExtracted = TRUE;

            ModemConsolePrintf( "Modem Info pass\r\n" );
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_END );
            break;
        }
        
        case MODEM_INFO_STATE_FAIL:
        {
            gfIsInfoExtracted = FALSE;

            ModemConsolePrintf( "Modem Info fail\r\n" );
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_END );
            break;
        }
        
        case MODEM_INFO_STATE_END:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_IDLE );
            
            break;
        }
                   
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_INFO_STATE_UNINITIALIZED );            
            break;
        }
    }
}

ModemDataModemInfoType  * ModemGetModemInfoPtr        ( void )
{
    return &gpModemData->tModemInfo;    
}