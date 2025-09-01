//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdarg.h>         // For va_arg support
#include <stdio.h>

#include "../Utils/Types.h"
#include "../DriversMcuPeripherals/Rtc.h"
#include "../IoStream/ioStream.h"
#include "./print.h"

// make sure is inside printabel chars
#define PRINT_CHAR_SMALLEST_VALID                 (32)    // 'space'
#define PRINT_CHAR_GREATEST_VALID                 (126)   // '~'

static CHAR    *gpcBuffer 			= NULL;
static UINT16 	gwBufferSize 		= 0;
static BOOL 	gfPrintDebugfEnable = FALSE;

static BOOL PrintIsCharPrintable( CHAR cChar );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PrintSetBuffer( CHAR *pcBuffer, UINT16 wBufferSize )
{
	BOOL fSuccess = FALSE;

	// DO NOT SET BUFFER IF ENUMS DOESN"T MATCH
	if( PRINT_STREAM_MAX != IO_STREAM_MAX )
	{
		while(1);
	}

	if( pcBuffer != NULL )
	{
		if( wBufferSize > 0 )
		{
			gpcBuffer 		= pcBuffer;
			gwBufferSize 	= wBufferSize;

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 Printf( PrintStreamEnum ePrintStream, const char *format, ... )
{
	BOOL		fSuccess		= FALSE;
	UINT16 		wBytesSent;
    UINT16      wStrLen;
	IoStreamSourceEnum eIoStream;
	va_list		args;
	//va_list		args_copy;

	if( ePrintStream < IO_STREAM_MAX )
	{
	    //if( fEnable )
	    {
			if( gpcBuffer != NULL )
			{
				// Grab variable argument list
				va_start( args, format );

				wStrLen = vsnprintf
				(
					(char *)gpcBuffer,
					gwBufferSize,
					format,
					args
				);

				// make sure the size is not greater than the buffer
				// if len equal or greater than size, add the '\0' at the last char of string
				if( wStrLen >= gwBufferSize )
				{
					wStrLen = gwBufferSize-1;
					gpcBuffer[gwBufferSize-1] = '\0';
				}

				eIoStream = (IoStreamSourceEnum)ePrintStream;
				fSuccess = IoStreamOutputPutBuffer( eIoStream, gpcBuffer, wStrLen, &wBytesSent, NULL );

				// Finish up
				va_end( args );
			}
        }
	}

	if( fSuccess )
	{
		return (INT32)wBytesSent;
	}
	else
	{
		return -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PrintIsCharPrintable( CHAR cChar )
{
	if
	(
		( cChar >= PRINT_CHAR_SMALLEST_VALID ) &&
		( cChar <= PRINT_CHAR_GREATEST_VALID )
	)
	{
		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PrintChar( PrintStreamEnum ePrintStream, CHAR cChar )
{
    BOOL		        fSuccess		= FALSE;
    IoStreamSourceEnum  eIoStream;

    if( ePrintStream < IO_STREAM_MAX )
	{
	    //if( fEnable )
	    {
	        if( PrintIsCharPrintable(cChar) == FALSE )
            {
                cChar = ".";
            }

	        eIoStream = (IoStreamSourceEnum)ePrintStream;
            fSuccess = IoStreamOutputPutChar( eIoStream, cChar );
	    }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PrintBuffer( PrintStreamEnum ePrintStream, CHAR *pvBuffer, UINT16 wBufferSize )
{
    BOOL		        fSuccess		= FALSE;
    IoStreamSourceEnum  eIoStream;

    if( ePrintStream < IO_STREAM_MAX )
	{
	    //if( fEnable )
	    {
	        for( UINT16 c = 0 ; c < wBufferSize ; c++ )
            {
                //CHAR cChar = ((CHAR*)pvBuffer)[c];
                if( PrintIsCharPrintable( pvBuffer[c] ) == FALSE )
                {
                    pvBuffer[c] = ".";
                }
            }

	        eIoStream = (IoStreamSourceEnum)ePrintStream;
            fSuccess = IoStreamOutputPutBuffer( eIoStream, pvBuffer, wBufferSize, NULL, NULL );
	    }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PrintDebugfEnable( BOOL fPrintDebugfEnable )
{
	gfPrintDebugfEnable = fPrintDebugfEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PrintDebugfIsEnabled( void )
{
	return gfPrintDebugfEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 PrintDebugf( PrintStreamEnum ePrintStream, BOOL fEnable, const char *format, ... )
{
	BOOL		fSuccess		= FALSE;
	UINT16 		wBytesSent;
    UINT16      wStrLen;
	va_list		args;
	//va_list		args_copy;

	IoStreamSourceEnum  eIoStream;
	RtcDateTimeStruct   tRtc;

	if( ePrintStream < IO_STREAM_MAX )
	{
	    if( fEnable && gfPrintDebugfEnable )
	    {
			if( gpcBuffer != NULL )
			{
			    eIoStream = (IoStreamSourceEnum)ePrintStream;

			    /////////////////////////////////////////////////
			    /////////////////////////////////////////////////
			    // print time first
			    RtcDateTimeStructInit( &tRtc );
			    RtcDateTimeGet( &tRtc );

			    wStrLen = snprintf
			    (
                    (char *)gpcBuffer,
					gwBufferSize,
					"[%04u-%02u-%02u %02u:%02u:%02u] ",
					(tRtc.bYear + RTC_CENTURY_OFFSET),
					tRtc.eMonth,
					tRtc.bDate,
					tRtc.bHour,
					tRtc.bMinute,
					tRtc.bSecond
                );

                // make sure the size is not greater than the buffer
				// if len equal or greater than size, add the '\0' at the last char of string
				if( wStrLen >= gwBufferSize )
				{
					wStrLen = gwBufferSize-1;
					gpcBuffer[gwBufferSize-1] = '\0';
				}

				fSuccess = IoStreamOutputPutBuffer( eIoStream, gpcBuffer, wStrLen, &wBytesSent, NULL );

                /////////////////////////////////////////////////
			    /////////////////////////////////////////////////
				// Grab variable argument list
				va_start( args, format );

				wStrLen = vsnprintf
				(
					(char *)gpcBuffer,
					gwBufferSize,
					format,
					args
				);

				// make sure the size is not greater than the buffer
				// if len equal or greater than size, add the '\0' at the last char of string
				if( wStrLen >= gwBufferSize )
				{
					wStrLen = gwBufferSize-1;
					gpcBuffer[gwBufferSize-1] = '\0';
				}

                // add at the end \r\n
				if( wStrLen < (gwBufferSize-2) )
				{
                    gpcBuffer[wStrLen] = '\r';
                    wStrLen++;
                    gpcBuffer[wStrLen] = '\n';
                    wStrLen++;
				}
				else
                {
                    wStrLen = gwBufferSize-3;
                    gpcBuffer[wStrLen] = '\r';
                    wStrLen++;
                    gpcBuffer[wStrLen] = '\n';
                    wStrLen++;
                }

				fSuccess = IoStreamOutputPutBuffer( eIoStream, gpcBuffer, wStrLen, &wBytesSent, NULL );

				// Finish up
				va_end( args );
				/////////////////////////////////////////////////
			    /////////////////////////////////////////////////
			}
        }
	}

	if( fSuccess )
	{
		return (INT32)wBytesSent;
	}
	else
	{
		return -1;
	}
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////

