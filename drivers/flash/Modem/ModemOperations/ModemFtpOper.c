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
#include "ModemFtpOper.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

typedef enum
{
    MODEM_FTP_OPERATION_STATE_UNINITIALIZED = 0,
    
    MODEM_FTP_OPERATION_STATE_DISABLED,
    
    /////////////////////////////////
    // Ftp put    
    MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE,
    MODEM_FTP_OPERATION_PUT_STATE_START,
    MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_SET_NAME,    
    
    MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_DATA_IDLE,
    MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA_COMMAND,
    MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA,

    MODEM_FTP_OPERATION_PUT_STATE_END_SUCCEED,
    MODEM_FTP_OPERATION_PUT_STATE_END_FAILED,
    MODEM_FTP_OPERATION_PUT_STATE_END_DELAY,
    /////////////////////////////////
    
    /////////////////////////////////
    // Ftp Get
    MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_IDLE,
    MODEM_FTP_OPERATION_GET_STATE_START,
    MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_GET_SIZE,
    MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_SET_NAME,        
    
    MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_DATA_IDLE,
    MODEM_FTP_OPERATION_GET_STATE_REQUEST_OBTAIN_DATA,
    MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA,    
    MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA_IS_EOF,

    MODEM_FTP_OPERATION_GET_STATE_END_SUCCEED,
    MODEM_FTP_OPERATION_GET_STATE_END_FAILED,
    MODEM_FTP_OPERATION_GET_STATE_END_DELAY,
    /////////////////////////////////

    /////////////////////////////////
    // Ftp Delete
    MODEM_FTP_OPERATION_DEL_STATE_WAIT_FOR_FILE_IDLE,
    MODEM_FTP_OPERATION_DEL_STATE_START,
    MODEM_FTP_OPERATION_DEL_STATE_DELETE_FILE,
      
    MODEM_FTP_OPERATION_DEL_STATE_END_SUCCEED,
    MODEM_FTP_OPERATION_DEL_STATE_END_FAILED,
    MODEM_FTP_OPERATION_DEL_STATE_END_DELAY,
    /////////////////////////////////

    MODEM_FTP_OPERATION_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemFtpOperStateMachineName[MODEM_FTP_OPERATION_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "Ftp operations Disabled",
    
    //////////////////////////
    // Ftp put
    "Ftp put wait for file idle",
    "Ftp put start",
    "Ftp put set name",
    
    "Ftp put wait for data idle",
    "Ftp put append data command",
    "Ftp put append data",

    "Ftp put succeed",
    "Ftp put failed",
    "Ftp put complete",
    //////////////////////////

    //////////////////////////
    // Ftp get
    "Ftp get wait for file idle",
    "Ftp get start",
    "Ftp get check size",
    "Ftp get set name",    
    
    "Ftp get wait for data idle",   
    "Ftp get request obtain data", 
    "Ftp get obtain data",    
    "Ftp get is EOF",

    "Ftp get succeed",
    "Ftp get failed",
    "Ftp get complete",
    //////////////////////////

    //////////////////////////
    // Ftp del
    "Ftp del wait for file idle",
    "Ftp del start",
    "Ftp del Delete file",

    "Ftp del succeed",
    "Ftp del failed",
    "Ftp del complete",
    //////////////////////////
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
static BOOL                     gfIsSetFileSucceed;
static BOOL                     gfIsSendDataSucceed;
//static BOOL                     gfIsOperationComplete;
static BOOL                     gfIsOperationSucceed;

static UINT8                    gbFtpPutReadyToSendDataPromptIndex;
static CHAR                     gszFileName[ 200 ];
static UINT32                   gdwFileSpace;

static BOOL                     gfFtpGetEofReached;
static UINT32                   gdwFtpGetBufferWalkerIndex;

static UINT8                    gbDataBuffer[ 512 ];
static UINT16                   gwDataLen;
static UINT32                   gdwTranferedBytes;

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
void ModemFtpOperationInit( void )
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

// ###############################################################################################
// FTP PUT
// ###############################################################################################
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpOperationPutSetFile( CHAR *pszFileName, UINT32 dwFileSize )
{
    if( gfIsWaitingForNewCommand )
    {
        // only if waiting for file put
        if( gtStateMachine.tState.bStateCurrent == MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE )
        {
            if( NULL != pszFileName )
            {
                if( dwFileSize > 0 )
                {
                    // set file operation information
                    strncpy( &gszFileName[0], pszFileName, ( sizeof(gszFileName) - 1 ) );
                    gdwFileSpace = dwFileSize;
                
                    // indicate command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_START );
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationPutSetFileIsDone( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationPutSetFileIsSucceed( void )
{
    return gfIsSetFileSucceed;
}

void ModemFtpOperationPutSendBufferGetInfo( UINT8 **pbBuffer, UINT32 *pdwBufferSize )
{
    // check all pointer holders are provided
    if
    (
        ( NULL != pbBuffer ) &&
        ( NULL != pdwBufferSize )
    )
    {
        (*pbBuffer)     = &gbDataBuffer[0];
        (*pdwBufferSize)= sizeof(gbDataBuffer);
    }
}

void  ModemFtpOperationPutSendData( UINT16 wBytesRead )
{
    if( gfIsWaitingForNewCommand )
    {
        // only if waiting for file put
        if( gtStateMachine.tState.bStateCurrent == MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_DATA_IDLE )
        {
            if
            (
                ( wBytesRead > 0 ) &&
                ( wBytesRead <= sizeof(gbDataBuffer) )
            )
            {
                if( ( gdwTranferedBytes + wBytesRead ) <= gdwFileSpace )                    
                {
                    gwDataLen = wBytesRead;

                    // indicate command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA_COMMAND );
                }
                else
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                }
            }
            else
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
            }
        }
    }
}

BOOL ModemFtpOperationPutSendDataIsDone( void )
{
    return gfIsWaitingForNewCommand;
}

BOOL ModemFtpOperationPutSendDataIsSucceed( void )
{
    return gfIsSendDataSucceed;
}

// ###############################################################################################
// FTP GET
// ###############################################################################################
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpOperationGetSetFile( CHAR *pszFileName, UINT32 dwAllocMaxSize )
{
    if( gfIsWaitingForNewCommand )
    {
        // only if waiting for file put
        if( gtStateMachine.tState.bStateCurrent == MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_IDLE )
        {
            if( NULL != pszFileName )
            {
                if( dwAllocMaxSize > 0 )
                {
                    // set file operation information
                    strncpy( &gszFileName[0], pszFileName, ( sizeof(gszFileName) - 1 ) );
                    gdwFileSpace = dwAllocMaxSize;
                
                    // indicate command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_START );
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationGetSetFileIsDone( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationGetSetFileIsSucceed( void )
{
    return gfIsSetFileSucceed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpOperationGetReceiveData( void )
{
    if( gfIsWaitingForNewCommand )
    {
        // only if waiting for file get
        if( gtStateMachine.tState.bStateCurrent == MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_DATA_IDLE )
        {            
            // only if EOF hasn't been found                                
            if( gfFtpGetEofReached == FALSE )
            {
                // if max alloc size hasn't been exceeded
                if( gdwTranferedBytes < gdwFileSpace )
                {
                    // indicate command will start running so more commands are not accepted
                    gfIsWaitingForNewCommand = FALSE;
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_REQUEST_OBTAIN_DATA );
                }
                else
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                }
            }   
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationGetReceiveDataIsDone( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationGetReceiveDataIsSucceed( void )
{
    return gfIsSendDataSucceed;
}


BOOL ModemFtpOperationGetIsFileExceedAllocSize( void )
{
    return ( gpModemData->tFtpGet.dwFileSize > gdwFileSpace );
}

UINT32 ModemFtpOperationGetFileSize( void )
{
    return gpModemData->tFtpGet.dwFileSize;
}

void ModemFtpOperationGetReceiverBufferGetInfo( UINT8 **pbBuffer, UINT32 *pdwBufferSize, UINT32 *pdwBytesRead, BOOL *pfEofReached )
{
    // check all pointer holders are provided
    if
    (
        ( NULL != pbBuffer ) &&
        ( NULL != pdwBufferSize ) &&
        ( NULL != pdwBytesRead ) &&
        ( NULL != pfEofReached )
    )
    {
        (*pbBuffer)     = &gbDataBuffer[0];
        (*pdwBufferSize)= sizeof(gbDataBuffer);
        (*pdwBytesRead) = gwDataLen;
        (*pfEofReached) = gfFtpGetEofReached;
    }
}


// ###############################################################################################
// FTP DELETE
// ###############################################################################################
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpOperationDeleteFile( CHAR *pszFileName )
{
    if( gfIsWaitingForNewCommand )
    {
        // only if waiting for file put
        if( gtStateMachine.tState.bStateCurrent == MODEM_FTP_OPERATION_DEL_STATE_WAIT_FOR_FILE_IDLE )
        {
            if( NULL != pszFileName )
            {                
                // set file operation information
                strncpy( &gszFileName[0], pszFileName, ( sizeof(gszFileName) - 1 ) );                
            
                // indicate command will start running so more commands are not accepted
                gfIsWaitingForNewCommand = FALSE;
            
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_START );                
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationDeleteFileIsDone( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemFtpOperationDeleteFileIsSucceed( void )
{
    return gfIsOperationSucceed;
}




// ###############################################################################################
// FTP <<< STATE MACHINE >>>
// ###############################################################################################
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemFtpOperationStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {
        case MODEM_FTP_OPERATION_STATE_UNINITIALIZED:
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            break;
        }
        
        
        case MODEM_FTP_OPERATION_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            // not allowed to run config operation
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message 
                ModemConsolePrintf( "Modem Ftp Oper Disabled\r\n" );
            }

            gfIsWaitingForNewCommand    = FALSE;
        
            if
            ( 
                ( ModemFtpConnectIsConnected() == TRUE ) &&
                ( ModemFtpConnectAllowedOperation() == MODEM_FTP_OPERATION_TYPE_PUT )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE );
            }
            
            if
            ( 
                ( ModemFtpConnectIsConnected() == TRUE ) &&
                ( ModemFtpConnectAllowedOperation() == MODEM_FTP_OPERATION_TYPE_GET )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_IDLE );
            }
            
            if
            ( 
                ( ModemFtpConnectIsConnected() == TRUE ) &&
                ( ModemFtpConnectAllowedOperation() == MODEM_FTP_OPERATION_TYPE_DELETE )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_WAIT_FOR_FILE_IDLE );
            }
            break;
        }
        
        


        // ###############################################################################################
        // FTP PUT
        // ###############################################################################################
        case MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message 
                ModemConsolePrintf( "Modem Ftp Put Idle\r\n" );
            }

            gfIsWaitingForNewCommand    = TRUE;
            
            if
            (
                ( ModemFtpConnectIsConnected() == FALSE ) ||
                ( ModemFtpConnectAllowedOperation() != MODEM_FTP_OPERATION_TYPE_PUT )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_FTP_OPERATION_PUT_STATE_START:
        {
            // clear first all variables related to operation status
            ////////////////////////////////////
            gfIsSetFileSucceed  = 0;
            gfIsSendDataSucceed = 0;
            
            gwDataLen           = 0;
            gdwTranferedBytes   = 0;

            //gfIsOperationComplete=0;
            gfIsOperationSucceed= 0;            
            ////////////////////////////////////
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_SET_NAME );
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_SET_NAME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                    // the arg 1 = command mode / 0 = online mode
                    // WARNING!! File name max char = 200. From Telit datasheet
                    ModemCommandProcessorSendAtCommand( "#FTPPUT=\"%s\",1", &gszFileName[0] );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_DATA_IDLE );
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_DATA_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand    = TRUE;
            gfIsSetFileSucceed = TRUE;            
            
            if
            (
                ( ModemFtpConnectIsConnected() == FALSE ) ||
                ( ModemFtpConnectAllowedOperation() != MODEM_FTP_OPERATION_TYPE_PUT )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA_COMMAND:
        {
            CHAR cChar;

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                gfIsSendDataSucceed = FALSE;                
                gbFtpPutReadyToSendDataPromptIndex = 0;

                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    
                    // ########################################
                    // special case where we are only expecting for ">" simbol
                    //ModemCommandProcessorSetExpectedResponse( TRUE, "#CGMM:", 1, TRUE );
                    // disabled automatic input processing.
                    // WARNING! don't forget to enable it again after receiving '>' or changing state in the state machine
                    ModemRxProcessKeyAutoUpdateEnable( FALSE );
                    // ########################################
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
                    ModemCommandProcessorSendAtCommand( "#FTPAPPEXT=%lu,%u", gwDataLen, ((gdwTranferedBytes+gwDataLen) == gdwFileSpace) );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                    break;
                }
            }
            
            if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
            {
                ModemRxProcessKeyManualUpdate( cChar );
                
                if
                (
                    ( gbFtpPutReadyToSendDataPromptIndex == 0 ) &&
                    ( cChar == '>' )
                )
                {
                    gbFtpPutReadyToSendDataPromptIndex++;
                }

                if
                (
                    ( gbFtpPutReadyToSendDataPromptIndex == 1 ) &&
                    ( cChar == ' ' )
                )
                {
                    gbFtpPutReadyToSendDataPromptIndex++;
                }

                if( gbFtpPutReadyToSendDataPromptIndex == 2 )
                {
                    // send '\r' to continue processing other commands since after '>' simple no other chars are coming
                    ModemRxProcessKeyManualUpdate( '\r' );
                    
                    // ########################################
                    // enable automatic input processing
                    ModemRxProcessKeyAutoUpdateEnable( TRUE );
                    // ########################################
                    
                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA );
                    break;
                }
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // ########################################
                // enable automatic input processing
                ModemRxProcessKeyAutoUpdateEnable( TRUE );
                // ########################################
                
                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_APPEND_DATA:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
                    
                    // send data
                    if( ModemTxPutBuffer( &gbDataBuffer[0], gwDataLen ) == FALSE )
                    {
                        // Set ERROR
                        ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                        // before change state always release semaphore
                        ModemCommandProcessorRelease( &geSemaphore );
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                        break;
                    }
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
                }
                else
                {                
                    gfIsSendDataSucceed = TRUE;
                    gdwTranferedBytes = gdwTranferedBytes + gwDataLen;                
                    // decide if need to send more or is finished
                    if( gdwTranferedBytes == gdwFileSpace )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_SUCCEED );                    
                    }
                    else
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_DATA_IDLE );                    
                    }
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_FAILED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_END_SUCCEED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = TRUE;
            //StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE );            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_DELAY );            
            break;
        }
        
        case MODEM_FTP_OPERATION_PUT_STATE_END_FAILED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = FALSE;
            //StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE );            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_END_DELAY );            
            break;
        }

        case MODEM_FTP_OPERATION_PUT_STATE_END_DELAY:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // set time out 
                StateMachineSetTimeOut( &gtStateMachine.tState, ( 1000 ) );
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {                
                //gfIsOperationComplete   = TRUE;            
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_PUT_STATE_WAIT_FOR_FILE_IDLE );            
            }            
            break;
        }


        // ###############################################################################################
        // FTP GET
        // ###############################################################################################
        case MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message 
                ModemConsolePrintf( "Modem Ftp Get Idle\r\n" );
            }

            gfIsWaitingForNewCommand    = TRUE;
            
            if
            (
                ( ModemFtpConnectIsConnected() == FALSE ) ||
                ( ModemFtpConnectAllowedOperation() != MODEM_FTP_OPERATION_TYPE_GET )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            }
            break;
        }
         
        case MODEM_FTP_OPERATION_GET_STATE_START:
        {
            // clear first all variables related to operation status
            ////////////////////////////////////
            gfIsSetFileSucceed  = 0;
            gfIsSendDataSucceed = 0;
            gfFtpGetEofReached  = 0;
            
            gwDataLen           = 0;
            gdwTranferedBytes   = 0;

            //gfIsOperationComplete=0;
            gfIsOperationSucceed= 0;            
            ////////////////////////////////////
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_GET_SIZE );
            break;
        }
        
        case MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_GET_SIZE:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#FTPFSIZE:", 1, TRUE );
                    
                    //gpModemData->tFtpGet.dwFileSize = 0xffFFffFF;
                    gpModemData->tFtpGet.dwFileSize = 0x00;

                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                    // the arg 1 = command mode / 0 = online mode
                    // WARNING!! File name max char = 200. From Telit datasheet
                    ModemCommandProcessorSendAtCommand( "#FTPFSIZE=\"%s\"", &gszFileName[0] );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "file get size error"
                    );
                }
                else
                {                
                    if( gpModemData->tFtpGet.dwFileSize <= gdwFileSpace )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_SET_NAME );
                    }
                    else
                    {                    
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    }
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
            }
            break;
        }

        case MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_SET_NAME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                    /* file name , view type...0= text or 1= hex */
                    ModemCommandProcessorSendAtCommand( "#FTPGETPKT=\"%s\",0", &gszFileName[0] );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_DATA_IDLE );
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_DATA_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            gfIsWaitingForNewCommand    = TRUE;
            gfIsSetFileSucceed = TRUE;            
            
            if
            (
                ( ModemFtpConnectIsConnected() == FALSE ) ||
                ( ModemFtpConnectAllowedOperation() != MODEM_FTP_OPERATION_TYPE_GET )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            }
            break;
        }
                
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // WARNING!! this state(REQUEST_OBTAIN_DATA) and (OBTAIN_DATA) attempt to use 
        // ModemRxProcessKeyAutoUpdateEnable( FALSE ) to process chars manually.
        // don't forget to set back to TRUE if going to any state different than these 2 states
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        case MODEM_FTP_OPERATION_GET_STATE_REQUEST_OBTAIN_DATA:
        {
            CHAR cChar;

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                gfIsSendDataSucceed         = FALSE;
                gwDataLen                   = 0;
                gfFtpGetEofReached          = 0;
                gdwFtpGetBufferWalkerIndex  = 0;

                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#FTPRECV:", 1, FALSE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
                    
                    // get data internally
                    // ########################################
                    // WARNING! don't forget to enable it again after receiving "OK" or changing state in the state machine
                    ModemRxProcessKeyAutoUpdateEnable( FALSE );
                    // ########################################

                    ModemCommandProcessorSendAtCommand( "#FTPRECV=%d", sizeof(gbDataBuffer) );                    
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
            {
                // after recording in buffer incoming chars
                ModemRxProcessKeyManualUpdate( cChar );
            }

            if( ModemCommandProcessorIsResponseComplete() )
            {
                if( ModemCommandProcessorIsError() )
                {
                    // ########################################
                    // enable automatic input processing
                    ModemRxProcessKeyAutoUpdateEnable( TRUE );
                    // ########################################                    

                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                }
                else
                {                
                    gwDataLen = gpModemData->tFtpGet.dwBytesToReceive;                

                    if( (gdwTranferedBytes + gwDataLen) <= gdwFileSpace )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA );
                    }
                    else
                    {
                        // ########################################
                        // enable automatic input processing
                        ModemRxProcessKeyAutoUpdateEnable( TRUE );
                        // ########################################                    

                        // Set ERROR
                        ModemEventLog
                (
                    TRUE,
                            "ftpOper[%s] Error='%s'", 
                            cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                            "more incoming data than expected" 
                        );
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    }
                }
                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                break;
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // ########################################
                // enable automatic input processing
                ModemRxProcessKeyAutoUpdateEnable( TRUE );
                // ########################################                    

                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "ftpOper[%s] Error='%s'", 
                    cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
            }
            break;
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // WARNING!! this state(REQUEST_OBTAIN_DATA) and (OBTAIN_DATA) attempt to use 
        // ModemRxProcessKeyAutoUpdateEnable( FALSE ) to process chars manually.
        // don't forget to set back to TRUE if going to any state different than these 2 states
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        case MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA:
        {
            CHAR cChar;

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 15 * 1000 );
                    
                    // get data internally
                    // ########################################
                    // WARNING! don't forget to enable it again after receiving "OK" or changing state in the state machine
                    ModemRxProcessKeyAutoUpdateEnable( FALSE );
                    // ########################################
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
            {
                // only copy to buffer the amount of bytes expected from file
                if( gdwFtpGetBufferWalkerIndex < gwDataLen )
                {
                    gbDataBuffer[gdwFtpGetBufferWalkerIndex] = cChar;
                    
                    gdwFtpGetBufferWalkerIndex++;
                    gdwTranferedBytes++;
                }
                else
                {
                    // after recording in buffer incoming chars
                    ModemRxProcessKeyManualUpdate( cChar );
                }
                                
                if( ModemCommandProcessorIsResponseComplete() )
                {                                        
                    // ########################################
                    // enable automatic input processing
                    ModemRxProcessKeyAutoUpdateEnable( TRUE );
                    // ########################################                    

                    if( ModemCommandProcessorIsError() )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    }
                    else
                    {                
                        gfIsSendDataSucceed         = TRUE;
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA_IS_EOF );
                    }
                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    break;
                }
            }            

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // ########################################
                // enable automatic input processing
                ModemRxProcessKeyAutoUpdateEnable( TRUE );
                // ########################################

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
            }
            break;
        }        

        case MODEM_FTP_OPERATION_GET_STATE_OBTAIN_DATA_IS_EOF:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( TRUE, "#FTPGETPKT:", 1, TRUE );
                    
                    gpModemData->tFtpGet.fEofFound = FALSE;

                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                    
                    ModemCommandProcessorSendAtCommand( "#FTPGETPKT?" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {
                // update local vars
                gfFtpGetEofReached = gpModemData->tFtpGet.fEofFound;                                               

                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
                }
                else
                {                
                    if( gfFtpGetEofReached == TRUE )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_SUCCEED );
                    }
                    else
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_DATA_IDLE );
                    }
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_FAILED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_GET_STATE_END_SUCCEED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = TRUE;
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_DELAY );            
            break;
        }
        
        case MODEM_FTP_OPERATION_GET_STATE_END_FAILED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = FALSE;
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_END_DELAY );            
            break;
        }

        case MODEM_FTP_OPERATION_GET_STATE_END_DELAY:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // set time out 
                StateMachineSetTimeOut( &gtStateMachine.tState, ( 1000 ) );
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {                
                //gfIsOperationComplete   = TRUE;            
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_GET_STATE_WAIT_FOR_FILE_IDLE );            
            }            
            break;
        }

        // ###############################################################################################
        // FTP DELETE
        // ###############################################################################################        
        case MODEM_FTP_OPERATION_DEL_STATE_WAIT_FOR_FILE_IDLE:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message 
                ModemConsolePrintf( "Modem Ftp Delete Idle\r\n" );
            }

            gfIsWaitingForNewCommand    = TRUE;
            
            if
            (
                ( ModemFtpConnectIsConnected() == FALSE ) ||
                ( ModemFtpConnectAllowedOperation() != MODEM_FTP_OPERATION_TYPE_DELETE )
            )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_DISABLED );
            }
            break;
        }
        
        case MODEM_FTP_OPERATION_DEL_STATE_START:
        {
            // clear first all variables related to operation status
            ////////////////////////////////////
            gfIsSetFileSucceed  = 0;
            gfIsSendDataSucceed = 0;
            gfFtpGetEofReached  = 0;
            
            gwDataLen           = 0;
            gdwTranferedBytes   = 0;

            //gfIsOperationComplete=0;
            gfIsOperationSucceed= 0;            
            ////////////////////////////////////
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_DELETE_FILE );
            break;
        }

        case MODEM_FTP_OPERATION_DEL_STATE_DELETE_FILE:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();                    
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                    
                    ModemCommandProcessorSendAtCommand( "#FTPDELE=\"%s\"", &gszFileName[0] );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_FAILED );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_FAILED );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_SUCCEED );
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
                        "ftpOper[%s] Error='%s'", 
                        cModemFtpOperStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_FAILED );
            }
            break;
        }
      
        
        case MODEM_FTP_OPERATION_DEL_STATE_END_SUCCEED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = TRUE;
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_DELAY );            
            break;
        }
        
        case MODEM_FTP_OPERATION_DEL_STATE_END_FAILED:
        {
            //gfIsOperationComplete   = TRUE;
            gfIsOperationSucceed    = FALSE;
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_END_DELAY );            
            break;
        }

        case MODEM_FTP_OPERATION_DEL_STATE_END_DELAY:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // set time out 
                StateMachineSetTimeOut( &gtStateMachine.tState, ( 1000 ) );
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {                
                //gfIsOperationComplete   = TRUE;            
                StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_DEL_STATE_WAIT_FOR_FILE_IDLE );            
            }            
            break;
        }


        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_FTP_OPERATION_STATE_UNINITIALIZED );            
            break;
        }
    }
}
