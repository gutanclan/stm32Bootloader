#include <stdio.h>          // Standard I/O library
#include <string.h>
#include <stdarg.h>         // For va_arg support
#include <__vfprintf.h>     // For printf-esque support
#include "Types.h"
#include "Rtc.h"
#include "../Utils/StringUtils.h"
#include "../PCOM/Console.h"
#include "../PCOM/Usart.h"
#include "../PCOM/Datalog.h"
#include "../PCOM/LogFormat.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"

#define MODEM_COMMAND_USER_CONSOLE_USART                                    (USART1_PORT)
#define MODEM_COMMAND_OUTPUT_USART                                          (USART2_PORT)
#define MODEM_COMMAND_RESPONSE_BUFFER_SIZE_BYTES                            (100)
#define MODEM_COMMAND_RESPONSE_TOKEN_MAX                                    (8)

static const CHAR cModemConsolePrintPrefix[]    = "MDM TX:";
static const CHAR cModemConsolePrintSufix[]     = "\r\n";
static const CHAR cModemCommandPrefix[]         = "\rAT";
static const CHAR cModemCommandSufix[]          = "\r";

static BOOL                         gfTxSemaphoreTaken;
static BOOL                         gfModemConsolePrintfEnable;
static BOOL                         gfModemConsolePrintDbgEnable;
static BOOL                         gfModemEventLogEnable;
static BOOL                         gfPrintRxEnabled;
static BOOL                         gfPrintTxEnabled;
static BOOL                         gfAutoUpdateEnabled;

static CHAR                         gcRawResponseBuffer                     [ MODEM_COMMAND_RESPONSE_BUFFER_SIZE_BYTES ];    
static UINT16                       gwRawResponseBufferSize_Byte            = MODEM_COMMAND_RESPONSE_BUFFER_SIZE_BYTES;
static UINT16                       gwRawResponseBufferInsertNewCharIndex;  
static CHAR                        *gpcRawResponseTokens                    [ MODEM_COMMAND_RESPONSE_TOKEN_MAX ];

#define MODEM_COMMAND_OUTPUT_BUFFER_SIZE_BYTES          (256)
static CHAR                         gcOutputBuffer                          [ MODEM_COMMAND_OUTPUT_BUFFER_SIZE_BYTES ];
#define MODEM_ERROR_LOG_BUFFER_SIZE_BYTES               (256)
static CHAR                         gcEventLogBuffer                        [ MODEM_ERROR_LOG_BUFFER_SIZE_BYTES ];

///////////////////////////////////////////////////////////////////////////////////////////////////////////

static void ModemProcessInconmmingString( void );
static BOOL ModemLineBufferAppendChar   ( CHAR cChar );
static BOOL ModemTxSemaphoreTake        ( void );
static BOOL ModemTxSemaphoreRelease     ( void );

///////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemTxSemaphoreTake( void )
{
    BOOL fSuccess = FALSE;

    if( gfTxSemaphoreTaken == FALSE )
    {
        gfTxSemaphoreTaken  = TRUE;
        fSuccess            = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemTxSemaphoreRelease( void )
{
    BOOL fSuccess = FALSE;

    if( gfTxSemaphoreTaken == TRUE )
    {
        gfTxSemaphoreTaken  = FALSE;
        fSuccess            = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemModuleInit( void )
{
    ModemCommandModuleInit();
    ModemResponseModuleInit();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemRxProcessKeyAutoUpdateEnable( BOOL fAutoUpdateEnable )
{
    gfAutoUpdateEnabled = fAutoUpdateEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemRxProcessKeyAutoUpdate( void )
{
    if( gfAutoUpdateEnabled )
    {
        CHAR cChar = 0;

        while( UsartGetNB( MODEM_COMMAND_OUTPUT_USART, &cChar ) )
        {
            ModemRxProcessKeyManualUpdate( cChar );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemRxProcessKeyManualUpdate( CHAR cChar )
{
    if( ( cChar >= 32 ) && ( cChar <= 126 ) )  // Printable characters
    {
        ModemLineBufferAppendChar( cChar );                          
    }    
    //else if( cChar == 0x0D )            // ENTER   NOTE (0x0D) defined in console too..!!!!!
    else if( cChar == 0x0A )            // ENTER   NOTE (0x0D) defined in console too..!!!!!
    {
        if( gwRawResponseBufferInsertNewCharIndex > 0 )
        {
            // print if enabled
            if( gfPrintRxEnabled )
            {
                //ConsolePrintf( CONSOLE_PORT_USART, "MDM RX[%s]\r\n", &gcRawResponseBuffer[0] );            
                ModemConsolePrintDbg( "MDM RX [%s]", &gcRawResponseBuffer[0] );
            }
            
            ModemProcessInconmmingString();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemLineBufferAppendChar( CHAR cChar )
{
    BOOL fSuccess = FALSE;

    if( gwRawResponseBufferInsertNewCharIndex < (gwRawResponseBufferSize_Byte-1) ) // -1 to have at least the last char '\0'
    {        
        gcRawResponseBuffer[gwRawResponseBufferInsertNewCharIndex] = cChar;
        gwRawResponseBufferInsertNewCharIndex++;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemProcessInconmmingString( void )
{
    UINT8                           bNumTokens              = 0;
    UINT16                          wStrLen                 = 0;
    UINT32                          dwDictionaryEntryIndex  = 0;
    ModemResponseHandlerArgsType    tModemResponseArgs;
    
    wStrLen = strlen( &gcRawResponseBuffer[0] );

    if( wStrLen > 0 )
    {
        // convert string into tokens
        bNumTokens = StringUtilsStringToTokenArray
        (
            &gcRawResponseBuffer[0],
            wStrLen,
            ' ',
            TRUE,
            '"',
            &gpcRawResponseTokens[0],
            (sizeof(gpcRawResponseTokens)/sizeof(gpcRawResponseTokens[0]))
        );

        // prepare structure that will hold the arguments for the command function
        tModemResponseArgs.eConsolePort = CONSOLE_PORT_USART;
        tModemResponseArgs.bNumArgs     = bNumTokens;
        tModemResponseArgs.pcArgArray   = (const CHAR **)(&gpcRawResponseTokens[0]);

        if( ModemResponseIsResponseListed( ModemResponseHandlerGetResponseListPtr(), gpcRawResponseTokens[0], &dwDictionaryEntryIndex ) == TRUE )
        {
            // increment counter for expected keyword
            ModemCommandProcessorCountKeywordMatch( (CHAR *)tModemResponseArgs.pcArgArray[0] );

            // Dispatch this to the matching response handler
            (ModemResponseHandlerGetResponseListPtr()[dwDictionaryEntryIndex].pfnResponseHandler)( &tModemResponseArgs );
        }
        
        // after processing last command, if waiting for expected response, check if received
        ModemCommandUpdateResponseReceived();
                
        
        // after processing
        // clear token array             
        memset( &gpcRawResponseTokens[0], 0, sizeof(gpcRawResponseTokens) );
        // reset line buffer
        memset( &gcRawResponseBuffer[0], 0, gwRawResponseBufferSize_Byte );
        gwRawResponseBufferInsertNewCharIndex = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemTxPutBuffer( void *pvBuffer, UINT16 wBufferSize )
{
    BOOL fSuccess = FALSE;       
    
    if( ModemTxSemaphoreTake() )
    {
        // send to modem
        fSuccess = UsartPutBuffer( MODEM_COMMAND_OUTPUT_USART, pvBuffer, wBufferSize );        

        ModemTxSemaphoreRelease();
    }    
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ModemTxSendAtCommand( const char *format, ... )
{
    va_list		args;
    
    UINT16      wBufferLen;
    UINT16      wCharsWritten;
    UINT16      wStrLen;
        
    wBufferLen = sizeof(gcOutputBuffer);

    va_start( args, format );

    wCharsWritten = vsnprintf(&gcOutputBuffer[0], sizeof(gcOutputBuffer), format, args);

    ModemTxPutBuffer( (void *)&cModemCommandPrefix[0], strlen(cModemCommandPrefix) );

    if( wCharsWritten > 0  )
    {
        if( wCharsWritten >= wBufferLen )
        {
            gcOutputBuffer[wBufferLen-1] = '\0';
        }
        
        ModemTxPutBuffer( &gcOutputBuffer[0], wCharsWritten );    

        if( gfPrintTxEnabled )
        {
            ModemConsolePrintDbg( "MDM TX [%s]", gcOutputBuffer );
        }        
    }
    else
    {
        if( gfPrintTxEnabled )
        {
            ModemConsolePrintDbg( "MDM TX [AT]" );
        }        
    }

    ModemTxPutBuffer( (void *)&cModemCommandSufix[0], strlen(cModemCommandSufix) );

    va_end( args );        

    return wCharsWritten;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConsolePrintRxEnable( BOOL fPrintRxEnable )
{
   gfPrintRxEnabled = fPrintRxEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConsolePrintRxIsEnabled( void )
{
    return gfPrintRxEnabled;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConsolePrintTxEnable( BOOL fPrintTxEnable )
{   
    gfPrintTxEnabled = fPrintTxEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConsolePrintTxIsEnabled( void )
{
    return gfPrintTxEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConsolePrintfEnable( BOOL fPrintfEnable )
{
    gfModemConsolePrintfEnable = fPrintfEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConsolePrintfIsEnabled             ( void )
{
    return gfModemConsolePrintfEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void    ModemConsolePrintDbgEnable          ( BOOL fPrintfEnable )
{
    gfModemConsolePrintDbgEnable = fPrintfEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    ModemConsolePrintDbgIsEnabled       ( void )
{
    return gfModemConsolePrintDbgEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void    ModemEventLogEnable                 ( BOOL fEventLogEnable )
{
    gfModemEventLogEnable = fEventLogEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    ModemEventLogIsEnabled              ( void )
{
    return gfModemEventLogEnable;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 ModemEventLog( BOOL fIsError, const char *format, ... )
{
    va_list		args;
    
    UINT16      wCharsWritten = 0;
    UINT16      wBufferLen;    
    UINT16      wStrLen;
    
    if( gfModemEventLogEnable )
    {
        wBufferLen = sizeof(gcEventLogBuffer);

        va_start( args, format );

        wCharsWritten = vsnprintf(&gcEventLogBuffer[0], (sizeof(gcEventLogBuffer)-1), format, args);    

        if( wCharsWritten > 0  )
        {
            if( wCharsWritten > (wBufferLen-1) )
            {
                // update chars to be written to a valid value
                wCharsWritten = wBufferLen-1;

                gcEventLogBuffer[wBufferLen-1] = '\0';
            }
                
            LogFormatEventDataType eMsgType;

            if( fIsError )
            {            
                eMsgType = LOG_FORMAT_EVENT_DATA_TYPE_ERROR;
            }
            else
            {            
                eMsgType = LOG_FORMAT_EVENT_DATA_TYPE_EVENT;
            }

            LogFormatLogEventData
            ( 
                DATALOG_FILE_EVENT, 
                eMsgType, 
                "Modem", 
                gcEventLogBuffer
            );
        }    

        va_end( args );        
    }

    return wCharsWritten;
}

//BOOL ModemConsolePrintf( const char *format, ... )
//{
//    va_list		args;    
//
//    if( gfModemConsolePrintfEnable )
//    {
//        va_start( args, format );
//        ConsolePrintf( CONSOLE_PORT_USART, format, args );       
//        va_end( args );
//    }
//}


//BOOL ModemConsolePrintDbg( const char *format, ... )
//{
//    va_list		args;    
//    RtcDateTimeStruct		tDateTime;
//	CHAR					cTimestampBuffer[32];
//
//    UINT16 wCharsWrittenTime;
//    UINT16 wCharsWrittenString;
//
//    // Declare static so that it is in the BSS section instead of on the Stack
//	static char cConsolePrintfArray[1024];    
//
//    if( gfModemConsolePrintDbgEnable )
//    {
//        RtcDateTimeGet(&tDateTime);
//
//        wCharsWrittenTime = snprintf( &cTimestampBuffer[0], sizeof(cTimestampBuffer), "{%04u-%02u-%02u %02u:%02u:%02u} ",
//				( tDateTime.bYear + RTC_CENTURY ),
//				tDateTime.eMonth,
//				tDateTime.bDate,
//				tDateTime.bHour,
//				tDateTime.bMinute,
//				tDateTime.bSecond
//				);
//
//        va_start( args, format );
//        
//        wCharsWrittenString = vsnprintf(&cConsolePrintfArray[0], sizeof(cConsolePrintfArray), format, args);
//
//        ConsolePutBuffer( CONSOLE_PORT_USART, cTimestampBuffer, wCharsWrittenTime );       
//        ConsolePutBuffer( CONSOLE_PORT_USART, cConsolePrintfArray, wCharsWrittenString );       
//        va_end( args );
//    }
//}