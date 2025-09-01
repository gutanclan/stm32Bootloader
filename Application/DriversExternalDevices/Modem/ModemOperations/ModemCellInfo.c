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
#include "ModemCellInfo.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

typedef enum
{
    MODEM_CELL_INFO_STATE_UNINITIALIZED = 0,
    
    MODEM_CELL_INFO_STATE_DISABLED,
    MODEM_CELL_INFO_STATE_IDLE,
    
    MODEM_CELL_INFO_STATE_START,
    MODEM_CELL_INFO_STATE_RUN_SERVINFO,
    MODEM_CELL_INFO_STATE_RUN_SET_MONI,
    MODEM_CELL_INFO_STATE_RUN_GET_MONI,
    MODEM_CELL_INFO_STATE_CHECK_IF_INFO_COMPLETE,
    
    MODEM_CELL_INFO_STATE_PASS,
    MODEM_CELL_INFO_STATE_FAIL,
    MODEM_CELL_INFO_STATE_END,
    
    MODEM_CELL_INFO_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemCellInfoStateMachineName[MODEM_CELL_INFO_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "Cell Info Disabled",
    "Cell Info Idle",
    
    "Cell Info Start",
    "servinfo",
    "set moni",
    "Get moni",
    "Check if info complete",
    
    "Info Pass",
    "Info Fail",
    "Info End",
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
//static BOOL                     gfIsInfoReady;

//
static ModemDataType           *gpModemData;

// operation
static BOOL                     gfIsWaitingForNewCommand;
static ModemStateMachineType    gtStateMachine;
static ModemCommandSemaphoreEnum geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemCellInfoInit( void )
{
    // set state machine to a initial state.
    StateMachineInit( &gtStateMachine.tState );
    
    //ModemResponseModemDataSemaphoreReserve( &geSemaphore );
    //gpModemData = ModemResponseModemDataGetPtr( &geSemaphore );    
    gpModemData = ModemResponseModemDataGetPtr();
    //ModemResponseModemDataSemaphoreRelease( &geSemaphore );
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
void ModemCellInfoCheckRun( void )
{
    if( gfIsWaitingForNewCommand )
    {
        // indicate command will start running so more commands are not accepted
        gfIsWaitingForNewCommand = FALSE;
        
        StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_START );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCellInfoIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCellInfoIsReady( void )
{
    return gpModemData->tCellInfo.fIsInfoReady;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemCellInfoStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print state machine state
        ModemConsolePrintDbg( "MDM SM <%s>", cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {
        case MODEM_CELL_INFO_STATE_UNINITIALIZED :
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_DISABLED );
            break;
        }
            
        case MODEM_CELL_INFO_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            // not allowed to run operation
            //////////////////////////////////////////////////
        
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                gpModemData->tCellInfo.fIsInfoReady = FALSE;
                // clear data strings                
                gpModemData->tCellInfo.szOperatorName[0]= '\0';                
                gpModemData->tCellInfo.szLac[0]         = '\0';
                gpModemData->tCellInfo.szCellId[0]      = '\0';
            }

            gfIsWaitingForNewCommand     = FALSE;            
        
            if( ModemConnectIsConnected() )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_IDLE );
            }
            break;
        }
        
        case MODEM_CELL_INFO_STATE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand    = TRUE;
            
            if( ModemConnectIsConnected() == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_CELL_INFO_STATE_START:
        {
            gpModemData->tCellInfo.fIsInfoReady = FALSE;
            // clear data strings                
            gpModemData->tCellInfo.szOperatorName[0]= '\0';            
            gpModemData->tCellInfo.szLac[0]         = '\0';
            gpModemData->tCellInfo.szCellId[0]      = '\0';
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_RUN_SERVINFO );
            break;
        }
        
        case MODEM_CELL_INFO_STATE_RUN_SERVINFO:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#SERVINFO:", 1, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 3000 );
                    
                    ModemCommandProcessorSendAtCommand( "#SERVINFO" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "cellInfo[%s] Error='%s'", 
                        cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "Semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_RUN_SET_MONI );
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
                    "cellInfo[%s] Error='%s'", 
                    cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "time out" 
                );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
            }
            break;
        }
        
        case MODEM_CELL_INFO_STATE_RUN_SET_MONI:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 3000 ); // 3 secs since information returned is long
                    
                    ModemCommandProcessorSendAtCommand( "#MONI=0" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "cellInfo[%s] Error='%s'", 
                        cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_RUN_GET_MONI );
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
                    "cellInfo[%s] Error='%s'", 
                    cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "time out" 
                );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
            }
            break;
        }

        case MODEM_CELL_INFO_STATE_RUN_GET_MONI:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 ); // 5 secs since information returned is long
                    
                    ModemCommandProcessorSendAtCommand( "#MONI" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "cellInfo[%s] Error='%s'", 
                        cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_CHECK_IF_INFO_COMPLETE );
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
                    "cellInfo[%s] Error='%s'", 
                    cModemCellInfoStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "time out" 
                );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
            }
            break;
        }

        case MODEM_CELL_INFO_STATE_CHECK_IF_INFO_COMPLETE:
        {
            BOOL fIsInfoComplete = TRUE;
                         
            fIsInfoComplete &= ( gpModemData->tCellInfo.szOperatorName[0]   != '\0' );            
            fIsInfoComplete &= ( gpModemData->tCellInfo.szLac[0]            != '\0' );
            fIsInfoComplete &= ( gpModemData->tCellInfo.szCellId[0]         != '\0' );

            // log msg
            ModemEventLog(FALSE,"Operator=%s,Lac=%s,CellId=%s", gpModemData->tCellInfo.szOperatorName,gpModemData->tCellInfo.szLac,gpModemData->tCellInfo.szCellId );

            if( fIsInfoComplete )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_PASS );
            }
            else
            {            
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_FAIL );
            }
            break;
        }
        
        case MODEM_CELL_INFO_STATE_PASS:
        {
            ModemConsolePrintf("Obtain Cell Info Pass\r\n");

            gpModemData->tCellInfo.fIsInfoReady = TRUE;                
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_IDLE );
            break;
        }
        
        case MODEM_CELL_INFO_STATE_FAIL:
        {
            ModemConsolePrintf("Obtain Cell Info Fail\r\n");

            gpModemData->tCellInfo.fIsInfoReady = FALSE;
            // clear data strings                
            gpModemData->tCellInfo.szOperatorName[0]= '\0';            
            gpModemData->tCellInfo.szLac[0]         = '\0';
            gpModemData->tCellInfo.szCellId[0]      = '\0';
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_IDLE );
            break;
        }
        
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CELL_INFO_STATE_UNINITIALIZED );            
            break;
        }
    }
}

ModemDataCellInfoType  * ModemCellInfoGetInfoPtr        ( void )
{
    return &gpModemData->tCellInfo;    
}
