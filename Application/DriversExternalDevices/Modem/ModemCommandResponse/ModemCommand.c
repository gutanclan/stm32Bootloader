#include <stdio.h>          // Standard I/O library
#include <string.h>
#include <stdarg.h>         // For va_arg support
#include <__vfprintf.h>     // For printf-esque support
#include "Types.h"
#include "../Utils/StringUtils.h"
#include "../PCOM/Console.h"
#include "../PCOM/Usart.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////

static ModemDataType           *gpModemData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandModuleInit( void )
{
    gpModemData = ModemResponseModemDataGetPtr();    

    if( gpModemData == NULL )
    {
        // catch this bug on development time
        while(1);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemCommandUpdateResponseReceived( void )
{    
    if( gpModemData->tCommandResponse.fIsProcessingCommandResponse )
    {
        // check if complete response found    
        UINT8 bResponseExpectedMask = 0;
        UINT8 bResponseReceivedMask = 0;
        UINT8 bHold                 = 0;

        // set response EXPECTED bit mask
        bHold = 0;
        bHold |=gpModemData->tCommandResponse.tResponse.tOk.fIsOkExpected           << 0;    
        bHold |=gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordExpected << 1;
        bResponseExpectedMask |= bHold;


        // set response RECEIVED bit mask    
//        // check if error
//        if( gpModemData->tCommandResponse.tResponse.fIsError )
//        {
//            // force to terminate processing of response
//            if( gpModemData->tCommandResponse.tResponse.tOk.fIsOkExpected )
//            {
//                gpModemData->tCommandResponse.tResponse.tOk.fIsOkReceived           = TRUE;
//            }
//            if( gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordExpected )
//            {
//                gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordReceived = TRUE;
//                gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordReceivedCounter = gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordExpectedCounter;
//            }
//        }
        
        // check for ok
        if( gpModemData->tCommandResponse.tResponse.tOk.fIsOkReceived )
        {
            bResponseReceivedMask = gpModemData->tCommandResponse.tResponse.tOk.fIsOkReceived << 0;
        }

        // check for keyword
        if( gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordReceived )
        {            
            if( gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordReceivedCounter == gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordExpectedCounter )
            {
                bResponseReceivedMask |= gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordReceived << 1;
            }            
        }
        
        if( bResponseExpectedMask == bResponseReceivedMask )
        {
            gpModemData->tCommandResponse.tResponse.fIsCompleteResponseReceived = TRUE;
        }
        else
        {
            if( gpModemData->tCommandResponse.tResponse.fIsError )
            {
                gpModemData->tCommandResponse.tResponse.fIsCompleteResponseReceived = TRUE;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemCommandProcessorCountKeywordMatch( CHAR *pszReceivedKeyWord )
{
    // check if expecting for this response
    if( gpModemData->tCommandResponse.fIsProcessingCommandResponse ) 
    {
        if( gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordExpected ) 
        {
            if( StringUtilsStringEqual( pszReceivedKeyWord, &gpModemData->tCommandResponse.tResponse.tKeyWord.cKeywordExpectedBuffer[0] ) == TRUE )
            {
                gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordReceived = TRUE;
                gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordReceivedCounter++;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
// semaphore between sending a command and waiting for response. 
// semaphore should be used so that nowhere else in the system another command will be send at the same time.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandProcessorReserve( ModemCommandSemaphoreEnum *peSemaphoreResult )
{
    BOOL fSuccess = FALSE;

    if( NULL != peSemaphoreResult )
    {
        if( gpModemData->tCommandResponse.fIsProcessingCommandResponse == FALSE )
        {            
            (*peSemaphoreResult) = MODEM_COMMAND_SEMAPHORE_RESERVED;
            gpModemData->tCommandResponse.fIsProcessingCommandResponse = TRUE;
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
// semaphore between sending a command and waiting for response. 
// semaphore should be used so that nowhere else in the system another command will be send at the same time.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandProcessorRelease( ModemCommandSemaphoreEnum *peSemaphoreResult )
{
    BOOL fSuccess = FALSE;

    if( NULL != peSemaphoreResult )
    {
        if( (*peSemaphoreResult) == MODEM_COMMAND_SEMAPHORE_RESERVED )
        {            
            (*peSemaphoreResult) = MODEM_COMMAND_SEMAPHORE_FREE;
            gpModemData->tCommandResponse.fIsProcessingCommandResponse = FALSE;
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemCommandProcessorResetResponse( void )
{
    memset( &gpModemData->tCommandResponse.tResponse, 0, sizeof(gpModemData->tCommandResponse.tResponse) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandProcessorSetExpectedResponse( BOOL fIsResponseKeywordExpected, CHAR *pcExpectedResponseKeyword, UINT8 bExpectedResponseKeywordAmount, BOOL fIsOkExpected )
{
    if
    (
        ( fIsResponseKeywordExpected == FALSE ) &&
        ( fIsOkExpected == FALSE )
    )
    {
        return FALSE;
    }
    else
    {        
        if        
        (
            ( fIsResponseKeywordExpected ) &&
            ( NULL == pcExpectedResponseKeyword )
        )
        {
            return FALSE;
        }
        else
        {
            if        
            (
                ( fIsResponseKeywordExpected ) &&
                ( bExpectedResponseKeywordAmount == 0 )
            )
            {
                return FALSE;
            }
            else
            {
                if( fIsResponseKeywordExpected )
                {
                    strncpy
                    ( 
                        &gpModemData->tCommandResponse.tResponse.tKeyWord.cKeywordExpectedBuffer[0], 
                        pcExpectedResponseKeyword, 
                        (sizeof(gpModemData->tCommandResponse.tResponse.tKeyWord.cKeywordExpectedBuffer)-1) 
                    );

                    
                    gpModemData->tCommandResponse.tResponse.tKeyWord.fIsKeyWordExpected        = fIsResponseKeywordExpected;
                    gpModemData->tCommandResponse.tResponse.tKeyWord.dwKeyWordExpectedCounter  = bExpectedResponseKeywordAmount;
                }

                if( fIsOkExpected )
                {
                    gpModemData->tCommandResponse.tResponse.tOk.fIsOkExpected                   = fIsOkExpected;
                }

                return TRUE;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandProcessorIsResponseComplete( void )
{
    return gpModemData->tCommandResponse.tResponse.fIsCompleteResponseReceived;
}

BOOL ModemCommandProcessorIsError( void )
{
    return gpModemData->tCommandResponse.tResponse.fIsError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemCommandIsProcessorBusy( void )
{
    return gpModemData->tCommandResponse.fIsProcessingCommandResponse;
}