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
#include "ModemFtp.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

#include "Led.h"

typedef enum
{
    MODEM_FTP_STATE_UNINITIALIZED = 0,
    
    MODEM_FTP_STATE_DISABLED,
    
    MODEM_FTP_STATE_CLOSE_IDLE,
    
    MODEM_FTP_STATE_OPEN_START,
    MODEM_FTP_STATE_OPEN_RUN_CLOSE,
    MODEM_FTP_STATE_OPEN_RUN_OPEN,
    MODEM_FTP_STATE_OPEN_RUN_TYPE,
    
    MODEM_FTP_STATE_OPEN_PASS,
    
    MODEM_FTP_STATE_OPEN_IDLE,
    
    MODEM_FTP_STATE_CLOSE_START,
    MODEM_FTP_STATE_CLOSE_RUN_CLOSE,
    
    MODEM_FTP_STATE_CLOSE_END,
    
    MODEM_FTP_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemFtpStateMachineName[MODEM_FTP_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "Ftp Disabled",
    "Ftp Close Idle",
    
    "Ftp Open Start",
    "Run Close",
    "Run Open",
    "Run Type",
    
    "Ftp Open Pass",
    
    "Ftp Open Idle",
    
    "Ftp Close Start",
    "Run Close",
    
    "Ftp Close End",
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
static BOOL                             gfIsUpdateConfigRequested;
static ModemFtpConnType                 gtRequestedFtpConnConfiguration;

static ModemFtpOperationTypeEnum        geOperationRequestingPermission;
static ModemFtpOperationTypeEnum        geOperationGranted;

//
static ModemDataType               *gpModemData;

// operation
static BOOL                         gfIsWaitingForNewCommand;
static ModemStateMachineType        gtStateMachine;
static ModemCommandSemaphoreEnum    geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpInit( void )
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
BOOL ModemFtpConnectSetConfig( ModemFtpConnType *ptFtpConn )
{
    BOOL fSuccess = FALSE;
    
    if( NULL != ptFtpConn )
    {        
        memcpy( &gtRequestedFtpConnConfiguration, ptFtpConn, sizeof(gtRequestedFtpConnConfiguration) );

        gfIsUpdateConfigRequested = TRUE;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

ModemFtpOperationTypeEnum ModemFtpConnectAllowedOperation   ( void )
{
    return geOperationGranted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpConnectRun( BOOL fConnectToServer, ModemFtpOperationTypeEnum eTransferType )
{
    if( gfIsWaitingForNewCommand )
    {
        if( eTransferType < MODEM_FTP_OPERATION_MAX )
        {
            if( fConnectToServer == TRUE ) // open connection
            {
                // if permit is available
                if( geOperationGranted           == MODEM_FTP_OPERATION_TYPE_NONE )
                {
                    geOperationRequestingPermission = eTransferType;
                    
                    // indicate a command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
                    
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_START );
                }
            }
            else    // close connection
            {
                // if permit is available                
                if
                (
                    ( geOperationGranted           == eTransferType )                            
                )
                {
                    geOperationRequestingPermission = eTransferType;

                    // indicate command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                }                
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpConnectIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpConnectIsConnected( void )
{
    BOOL fIsConnected = FALSE;
    
    if
    (
        ( geOperationGranted == MODEM_FTP_OPERATION_TYPE_PUT ) ||
        ( geOperationGranted == MODEM_FTP_OPERATION_TYPE_GET ) ||
        ( geOperationGranted == MODEM_FTP_OPERATION_TYPE_DELETE )
    )
    {
        fIsConnected = TRUE;
    }
    
    return fIsConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {
        case MODEM_FTP_STATE_UNINITIALIZED :
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_DISABLED );
            break;
        }
            
        case MODEM_FTP_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            // not allowed to run ftp operation
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // clear variables
                geOperationGranted              = MODEM_FTP_OPERATION_TYPE_NONE;
                geOperationRequestingPermission = MODEM_FTP_OPERATION_TYPE_NONE;

                // print message that power is on
                ModemConsolePrintf( "Modem Ftp Conn Disabled\r\n" );
            }
                        
            gfIsWaitingForNewCommand    = FALSE;
        
            if( ModemConnectIsConnected() )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_IDLE );
            }
            break;
        }
        
        case MODEM_FTP_STATE_CLOSE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                geOperationGranted              = MODEM_FTP_OPERATION_TYPE_NONE;
                geOperationRequestingPermission = MODEM_FTP_OPERATION_TYPE_NONE;
                // print message that power is on
                ModemConsolePrintf( "Modem Ftp Closed\r\n" );

                // set LED pattern
                LED_setLedBlinkPattern( LED1_POWER, LED_PATTERN_SLOW );
            }
            
            gfIsWaitingForNewCommand    = TRUE;
            
            if( ModemConnectIsConnected() == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_FTP_STATE_OPEN_START:
        {
            // set LED pattern
            LED_setLedBlinkPattern( LED1_POWER, LED_PATTERN_FAST );

            geOperationGranted = MODEM_FTP_OPERATION_TYPE_NONE;
            
            if( gfIsUpdateConfigRequested )
            {                
                memcpy( &gpModemData->tFtpServerConn, &gtRequestedFtpConnConfiguration, sizeof(gpModemData->tFtpServerConn) );
                
                gfIsUpdateConfigRequested = FALSE;
            }

            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_RUN_CLOSE );
            break;
        }
        
        case MODEM_FTP_STATE_OPEN_RUN_CLOSE:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 10000 ); // 10 seconds time out
                    ModemCommandProcessorSendAtCommand( "#FTPCLOSE" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_RUN_OPEN );
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
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
            }
            break;
        }
        
        case MODEM_FTP_STATE_OPEN_RUN_OPEN:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 15 * 1000 ); // 10 seconds time out
                    ModemCommandProcessorSendAtCommand
                    ( 
                        "#FTPOPEN=\"%s:%u\",\"%s\",\"%s\",0",
                        &gpModemData->tFtpServerConn.szAddress[0],
                        gpModemData->tFtpServerConn.dwPort,
                        &gpModemData->tFtpServerConn.szUserName[0],
                        &gpModemData->tFtpServerConn.szPassword[0]
                    );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_RUN_TYPE );
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
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
            }
            break;
        }
        
        case MODEM_FTP_STATE_OPEN_RUN_TYPE:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, ( 10 * 1000 ) );
                    ModemCommandProcessorSendAtCommand( "#FTPTYPE=0" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_PASS );
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
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_START );
            }
            break;
        }
        
        case MODEM_FTP_STATE_OPEN_PASS:
        {
            // set allowed operation       
            geOperationGranted              = geOperationRequestingPermission;
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_OPEN_IDLE );
            break;
        }
        
        case MODEM_FTP_STATE_OPEN_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                geOperationRequestingPermission = MODEM_FTP_OPERATION_TYPE_NONE;
                // print message that power is on
                ModemConsolePrintf( "Modem Ftp Open\r\n" );
            }
            
            gfIsWaitingForNewCommand    = TRUE;
            
            if( ModemConnectIsConnected() == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_DISABLED );
            }
            break;
        }
        
        case MODEM_FTP_STATE_CLOSE_START:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_RUN_CLOSE );
            break;
        }

        case MODEM_FTP_STATE_CLOSE_RUN_CLOSE:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 10 * 1000 ); // 10 seconds time out
                    ModemCommandProcessorSendAtCommand( "#FTPCLOSE" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_END );
                    break;
                }
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_END );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_END );
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
                        "ftp[%s] Error='%s'", 
                        cModemFtpStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_END );
            }
            break;
        }
    
        case MODEM_FTP_STATE_CLOSE_END:
        {            
            // clear variables
            geOperationGranted              = MODEM_FTP_OPERATION_TYPE_NONE;
            geOperationRequestingPermission = MODEM_FTP_OPERATION_TYPE_NONE;            

            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_CLOSE_IDLE );
            break;
        }
                   
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_STATE_UNINITIALIZED );            
            break;
        }
    }
}
