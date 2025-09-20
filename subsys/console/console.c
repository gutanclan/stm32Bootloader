//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// System include files
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <stdio.h>          // Standard I/O library
#include <stdarg.h>         // For va_arg support
#include <string.h>         // String manipulation routines

#include "../../drivers/common/Types.h"
#include "../../drivers/common/Semaphore.h"
#include "../../drivers/common/timer.h"
#include "../../drivers/rtc/rtc.h"
#include "../../drivers/uart/usart.h"

#include "../iostream/ioStream.h"
#include "print.h"

#include "../../drivers/common/stringUtils.h"
#include "strQueueCir.h"
#include "strLineEditor.h"

#include "./Command/command.h"
#include "console.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Macros and Structures
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CONSOLE_TOKEN_STRING_ARG_INDICATOR  ('"')
#define CONSOLE_TOKEN_SEPARATOR             (' ')


////! Console prompt symbol to be printed
#define CONSOLE_STRING_PROMPT			    (">")
//
////! Console jump to a new line and prints the prompt
//#define CONSOLE_STRING_NEW_PROMPT		("\r\n> ")
//
////! Console jump back one position in the line removing a char at the same time
#define CONSOLE_STRING_BACKSPACE		    ("\b \b")
//
////! clears the line displayed in the console
//#define CONSOLE_STRING_CLEAR_LINE		("\x1b[2K\x1b[128D")
//
////! ASCII chars
#define CONSOLE_CHAR_LF						(0x0A)
#define CONSOLE_CHAR_CR						(0x0D)
#define CONSOLE_CHAR_BACKSPACE				(0x08)		// '\b'
#define CONSOLE_CHAR_TAB					(0x09)		// '\t'
#define CONSOLE_CHAR_ESCAPE					(0x1B)
#define CONSOLE_CHAR_DELETE                 (0x7F)
#define CONSOLE_CHAR_SPACE					(0x20)		// ' '
#define CONSOLE_CHAR_2						(0x32)
#define CONSOLE_CHAR_A						(0x41)
#define CONSOLE_CHAR_B						(0x42)
#define CONSOLE_CHAR_C						(0x43)
#define CONSOLE_CHAR_D						(0x44)
#define CONSOLE_CHAR_K						(0x4B)
#define CONSOLE_CHAR_SQUARE_BRACKET_LEFT	(0x5b)
#define CONSOLE_CHAR_STRING_ARG_INDICATOR   (0x60)      // '`'
#define CONSOLE_CHAR_SUB                    (0x0A)      // substitute

#define CONSOLE_STRING_CLEAR_LINE		    ("\x1b[2K\x1b[128D")

typedef struct
{
    BOOL                        fIsConsoleEnable;
    BOOL                        fInputAutoUpdateEnable;
    BOOL                        fPrintDbgEnable;
    BOOL                        fPrintfEnable;
	SemaphoreEnum			    eSemaphoreAccess;
    TIMER                       dwConsoleInactivityUpTimer;
}ConsoleStatType;

#define CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE  (5) // never change this const
#define CONSOLE_COMMAND_TOKEN_MAX               (8)
#define CONSOLE_PRINTF_OUTPUT_BUFFER            (512)

typedef struct
{
    //! Pointer to the console's command dictionary
	CommandDictionaryType		*psDictionary;
    CommandResultEnum		    eLastResult;

    // line buffer editor
    StrLineEditorEnum           eLineBufferEditor;
    //! Circular buffer
    StrQueueCirEnum             eCircularQueue;
    //! Historical record of the past key inputs - used for processing special keys from the console
    CHAR						cInputKeyHistory				[ CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE ];
    //! Represents a list of tokens parsed form the command line handler
	CHAR					    *pcTokens						[ CONSOLE_COMMAND_TOKEN_MAX ];
}ConsoleCommandType;

typedef struct
{
	// console string name
    const CHAR		    *   pcLabel;
    ConsoleStatType         tStatus;
    ConsoleCommandType      tInputCommand;
    IoStreamSourceEnum      eIoStream;
    CHAR                    cOutputBuffer                       [ CONSOLE_PRINTF_OUTPUT_BUFFER ];
} ConsoleType;

////////////////////////////////////////////////////////////////////////////////////////////////////
//// Local Variables
////////////////////////////////////////////////////////////////////////////////////////////////////

static ConsoleType	gtConsoleLookupArray[] =
{
	{
	    //	Console label
	    "SERIAL1",
	    // tStatus
        {
            //fIsConsoleEnable
            FALSE,
            //fInputAutoUpdateEnable
            FALSE,
            //fPrintDbgEnable
            FALSE,
            //fPrintfEnable
            FALSE,
            //eSemaphoreAccess
            SEMAPHORE_CONSOLE_OUTPUT_MAIN,
            //dwConsoleInactivityUpTimer
            0
        },
        // tCommand
        {
            // dictionary pointer
            NULL,
            // last result
            COMMAND_RESULT_UNKNOWN,
            // line buffer editor
            STR_LINE_EDITOR_1,
            // circular queue
            STR_QUEUE_CIR_CONSOLE_1

            // the elements left are arrays.
            //let the global structure to initialize this elements to zero.
        },
        //IoStreamSourceEnum      eIoStream;
        IO_STREAM_USER
        // leave buffer so that it gets initialized at zeros
    },
};

#define CONSOLE_ARRAY_MAX		( sizeof(gtConsoleLookupArray)/sizeof(gtConsoleLookupArray[0]) )

////////////////////////////////////////////////////////////////////////////////////////////////////
//// Local Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL     ConsolePortAcquire              ( ConsolePortEnum eConsolePort );
static BOOL     ConsolePortRelease              ( ConsolePortEnum eConsolePort );
static BOOL     ConsoleLineBufferAppendChar     ( ConsolePortEnum eConsolePort, CHAR cChar );
static BOOL     ConsoleLineBufferUnAppendChar   ( ConsolePortEnum eConsolePort );
static void     ConsoleCommandClearCurrent      ( ConsolePortEnum eConsolePort );
static void     ConsoleCommandDisplayPrevious   ( ConsolePortEnum eConsolePort );
static void     ConsoleCommandDisplayNext       ( ConsolePortEnum eConsolePort );
static void     ConsoleCommandProcess           ( ConsolePortEnum eConsolePort );

///////////////////////////////////// BODY OF THE LIBRARY //////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsolePortAcquire( ConsolePortEnum eConsolePort )
{
    BOOL fSuccess       = FALSE;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
    {
        fSuccess = SemaphoreTake( gtConsoleLookupArray[eConsolePort].tStatus.eSemaphoreAccess );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsolePortRelease( ConsolePortEnum eConsolePort )
{
    BOOL fSuccess       = FALSE;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
    {
        fSuccess = SemaphoreGive( gtConsoleLookupArray[eConsolePort].tStatus.eSemaphoreAccess );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleModuleInit( void )
{
    BOOL fSuccess       = FALSE;

    if( CONSOLE_ARRAY_MAX != CONSOLE_PORT_MAX )
    {
        while(1);
    }

    StrQueueCirModuleInit();
    StrLineEditorModuleInit();

    fSuccess = TRUE;

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleCommandDictionarySet( ConsolePortEnum eConsolePort, CommandDictionaryType *psDictionary )
{
	BOOL fSuccess = FALSE;

	if
	(
        ( eConsolePort < CONSOLE_ARRAY_MAX ) &&
        ( NULL != psDictionary )
    )
	{
		gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary = psDictionary;

        fSuccess = TRUE;
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ConsoleCommandDictionaryGet( ConsolePortEnum eConsolePort, CommandDictionaryType **psDictionary )
{
	if
	(
        ( eConsolePort < CONSOLE_ARRAY_MAX ) &&
        ( NULL != psDictionary )
    )
	{
		(*psDictionary) = gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleEnable( ConsolePortEnum eConsolePort, BOOL fEnable )
{
    BOOL fSuccess = FALSE;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
	{
	    gtConsoleLookupArray[eConsolePort].tStatus.fIsConsoleEnable = fEnable;
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleInputGetChar( ConsolePortEnum eConsolePort, CHAR *pcChar )
{
    BOOL                fSuccess = FALSE;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
	{
	    if( gtConsoleLookupArray[eConsolePort].eIoStream < IO_STREAM_MAX )
	    {
	        if( gtConsoleLookupArray[eConsolePort].tStatus.fIsConsoleEnable )
            {
                fSuccess = IoStreamInputGetChar( gtConsoleLookupArray[eConsolePort].eIoStream, pcChar );
            }
	    }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleInputProcessingAutoUpdateEnable( ConsolePortEnum eConsolePort, BOOL fAutoUpdateEnable )
{
    BOOL fSuccess = FALSE;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
    {
        gtConsoleLookupArray[eConsolePort].tStatus.fInputAutoUpdateEnable = fAutoUpdateEnable;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleInputProcessingAutoUpdateIsEnabled( ConsolePortEnum eConsolePort )
{
    if( eConsolePort < CONSOLE_ARRAY_MAX )
    {
        return gtConsoleLookupArray[eConsolePort].tStatus.fInputAutoUpdateEnable;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ConsoleInputProcessingAutoUpdate( void )
{
    ConsolePortEnum eConsolePort    = 0;
    CHAR			cChar		    = -1;

    for( eConsolePort = 0; eConsolePort < CONSOLE_ARRAY_MAX ; eConsolePort++ )
    {
        if( gtConsoleLookupArray[eConsolePort].tStatus.fInputAutoUpdateEnable )
        {
            // if console enabled, it will attempt to get the next char in the serial buffer
            if( ConsoleInputGetChar( eConsolePort, &cChar ) )
            {
                // process key
                ConsoleInputProcessingManualUpdate( eConsolePort, cChar );
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleInputProcessingManualUpdate( ConsolePortEnum eConsolePort, CHAR cChar )
{
    BOOL    fSuccess                = FALSE;
    UINT8   bInputKeyHistoryLenght;

    if( eConsolePort < CONSOLE_ARRAY_MAX )
	{
	    if( gtConsoleLookupArray[eConsolePort].tStatus.fIsConsoleEnable )
        {
            fSuccess = TRUE;

            /////////////////////////////////////////////////////////////////////
            // special case when user only press ESC char.
            // CONDITION: some key press like arrows up, down, etc etc.. are a sequence of ESC+CHARx+CHARy one after the other so
            // if only ESC is detected(pressing only ESC key), and a timer times out, 2 more characters should be added to keep the
            // format of ESC+CHARx+CHARy instead of only ESC.
            if(gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_ESCAPE)
            {
                // NOTE: considering average person type 200 chars per minute
                if( TimerUpTimerGetMs(gtConsoleLookupArray[eConsolePort].tStatus.dwConsoleInactivityUpTimer) > 200 )
                {
                    for( bInputKeyHistoryLenght = 0 ; bInputKeyHistoryLenght < (CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1) ; bInputKeyHistoryLenght++ )
                    {
                        gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght+1];
                    }
                    gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = 0x0;

                    for( bInputKeyHistoryLenght = 0 ; bInputKeyHistoryLenght < (CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1) ; bInputKeyHistoryLenght++ )
                    {
                        gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght+1];
                    }
                    gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = 0x0;
                }
            }
            /////////////////////////////////////////////////////////////////////



            // Restart inactivity timer
            gtConsoleLookupArray[eConsolePort].tStatus.dwConsoleInactivityUpTimer = TimerUpTimerStartMs();


            //////////////////////////////////////////////////////////////////////
            // Shift it into the KEY history
            for( bInputKeyHistoryLenght = 0 ; bInputKeyHistoryLenght < (CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1) ; bInputKeyHistoryLenght++ )
            {
                gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght+1];
            }
            gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[bInputKeyHistoryLenght] = cChar;
            //////////////////////////////////////////////////////////////////////

            if	// UP ARROW
            (
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-2] == CONSOLE_CHAR_ESCAPE) &&
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-1] == CONSOLE_CHAR_SQUARE_BRACKET_LEFT) &&
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_A)
            )
            {
                //ConsolePrintf( eConsolePort, "UP ARROW\r\n> " );
                ConsoleCommandDisplayPrevious( eConsolePort );
            }
            else if // DOWN ARROW
            (
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-2] == CONSOLE_CHAR_ESCAPE) &&
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-1] == CONSOLE_CHAR_SQUARE_BRACKET_LEFT) &&
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_B)
            )
            {
                //ConsolePrintf( eConsolePort, "DOWN ARROW\r\n> " );
                ConsoleCommandDisplayNext( eConsolePort );
            }
            else if // RIGHT ARROW
            (
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-2] == CONSOLE_CHAR_ESCAPE) &&
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-1] == CONSOLE_CHAR_SQUARE_BRACKET_LEFT) &&
                ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_C)
            )
            {
                // Do nothing
                //ConsolePrintf( eConsolePort, "RIGHT ARROW\r\n> " );
            }
            else if // LEFT ARROW
            (
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-2] == CONSOLE_CHAR_ESCAPE) &&
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-1] == CONSOLE_CHAR_SQUARE_BRACKET_LEFT) &&
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_D)
            )
            {
                // Do nothing
                //ConsolePrintf( eConsolePort, "LEFT ARROW\r\n> " );
            }
            ////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////
            // this section allow the exc char to escalate to index 1 of key history buffer without
            // letting other keys to be detected. For example In case of upper arrow, it has a
            // ESC+[+D.... being '[' and 'D' printable characters
            else if ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_ESCAPE )
            {
                // Do nothing
                // This will catch incomplete escape codes
            }
            else if ( gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-1] == CONSOLE_CHAR_ESCAPE )
            {
                // Do nothing
                // This will catch incomplete escape codes
            }
            ////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////
            else if // CLEAR LINE
            (
                (gtConsoleLookupArray[eConsolePort].tInputCommand.cInputKeyHistory[CONSOLE_COMMAND_INPUT_KEY_HISTORY_SIZE-1-0] == CONSOLE_CHAR_DELETE)
            )
            {
                //ConsolePrintf( eConsolePort, "CLR_LINE\r\n> " );
                // clear command
                ConsoleCommandClearCurrent( eConsolePort );
            }
            else if( cChar == CONSOLE_CHAR_TAB )
            {
                // Do nothing
                //ConsolePrintf( eConsolePort, "TAB\r\n> " );
            }
            else if( ( cChar >= 32) && (cChar <= 126) )	// Printable characters
            {
                //ConsolePrintf( eConsolePort, "PRT CHAR\r\n> " );
                ConsoleLineBufferAppendChar( eConsolePort, cChar );
            }
            else if( cChar == CONSOLE_CHAR_BACKSPACE )	// BACKSPACE
            {
                //ConsolePrintf( eConsolePort, "BACKSPACE\r\n> " );
                ConsoleLineBufferUnAppendChar( eConsolePort );
            }
            else if( cChar == CONSOLE_CHAR_CR )			// carriage return
            {
                //ConsolePrintf( eConsolePort, "ENTER\r\n> " );

                if( strlen( (const char *)StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) ) > 0 )
                {
                    // move line buffer to circular
                    StrQueueCirInsertString
                    (
                        gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue,
                        StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor )
                    );
                }

                //Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );
                ///////////////////////////////
                // this block can be commented and console will still be working
                // process command
                ConsoleCommandProcess( eConsolePort );
                // if  ConsoleCommandProcess commented, at least print back command
                //ConsolePrintf( eConsolePort, "\r\n" );
                //ConsolePrintf( eConsolePort, StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );
                ///////////////////////////////

                // reset line buffer
                StrLineEditorBufferReset( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor );

                // print jump line
                Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );
                //Printf( PRINT_STREAM_USER, TRUE, "\r\n" );
                //ConsolePrintf( eConsolePort, "\r\n" );
                // print propt
                //ConsolePrintf( eConsolePort, CONSOLE_STRING_PROMPT );
                Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_PROMPT );
            }
        }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ConsoleInputInactivityTimeMilliSeconds( ConsolePortEnum eConsolePort )
{
    if( eConsolePort < CONSOLE_ARRAY_MAX )
	{
        return TimerUpTimerGetMs( gtConsoleLookupArray[eConsolePort].tStatus.dwConsoleInactivityUpTimer );
	}

	return 0xFFffFFff;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ConsoleCommandClearCurrent( ConsolePortEnum eConsolePort )
{
    // reset line buffer
    StrLineEditorBufferReset( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor );
    // clear CONSOLE line
    Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_CLEAR_LINE );
    // print CONSOLE prompt at the beginning of the line
    Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_PROMPT );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleLineBufferAppendChar( ConsolePortEnum eConsolePort, CHAR cChar )
{
    BOOL fSuccess = FALSE;

    if( StrLineEditorBufferInsertChar( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor, cChar ) )
    {
        // display into the console the char appended
        fSuccess = PrintChar( gtConsoleLookupArray[eConsolePort].eIoStream, cChar );
        //ConsolePutChar( eConsolePort, TRUE, cChar );

        //fSuccess = TRUE;
    }

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConsoleLineBufferUnAppendChar( ConsolePortEnum eConsolePort )
{
	BOOL fSuccess = FALSE;

    if( StrLineEditorBufferDeleteChar( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor, NULL ) )
    {
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_BACKSPACE );

        fSuccess = TRUE;
    }

	return fSuccess;
}

void ConsoleCommandProcess( ConsolePortEnum eConsolePort )
{
    UINT8					bNumTokens				= 0;
    UINT16                  wStrLen                 = 0;
    UINT32					dwDictionaryEntryIndex	= 0;
    CommandArgsType		    tCommandArgs;


    wStrLen = strlen( (const char *)StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );

    if( wStrLen > 0 )
    {
        //ConsolePrintf( eConsolePort, "\r\n[%s] EXECUTING [%s]\r\n", gtConsoleLookupArray[eConsolePort].pcLabel, StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );
        //Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n[%s] EXECUTING [%s]\r\n", gtConsoleLookupArray[eConsolePort].pcLabel, StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );
        //PrintDebugf( PRINT_STREAM_USER, TRUE, "\r\n[%s] EXECUTING [%s]\r\n", gtConsoleLookupArray[eConsolePort].pcLabel, StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );

        // clear token array
        memset( &gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[0], 0, sizeof(gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens) );

        // convert string into tokens
        bNumTokens = StringUtilsStringToTokenArray
        (
            StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ),
            wStrLen,
            CONSOLE_TOKEN_SEPARATOR,
            TRUE,
            CONSOLE_TOKEN_STRING_ARG_INDICATOR,
            &gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[0],
            (sizeof(gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens)/sizeof(gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[0]))
        );

        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n\r\n" );

        if( NULL != gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary )
        {
            if( CommandIsCommandValid( gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary, gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[0], &dwDictionaryEntryIndex ) == TRUE )
            {
//                for(int c = 0  ; c<bNumTokens; c++)
//                {
//                    ConsolePrintf( eConsolePort, "token %d [%s]\r\n", (c+1), gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[c] );
//                }

                //Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );

                // prepare structure that will hold the arguments for the command function
                tCommandArgs.pcCommand      = gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary[dwDictionaryEntryIndex].pszCommandString;
                tCommandArgs.eConsolePort   = eConsolePort;
                tCommandArgs.bNumArgs	    = bNumTokens;
                tCommandArgs.pcArgArray	    = &gtConsoleLookupArray[eConsolePort].tInputCommand.pcTokens[0];

                // Dispatch this to the command handler
                gtConsoleLookupArray[eConsolePort].tInputCommand.eLastResult = (gtConsoleLookupArray[eConsolePort].tInputCommand.psDictionary[dwDictionaryEntryIndex].pfnCommandHandler)( &tCommandArgs );
            }
            else
            {
                gtConsoleLookupArray[eConsolePort].tInputCommand.eLastResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
            }

            // Print result to show if the command was correct or not
            //Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );
            if( gtConsoleLookupArray[eConsolePort].tInputCommand.eLastResult != COMMAND_RESULT_OK )
            {
                Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "ERROR " );
            }
            else
            {
                Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );
            }
            Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "%s\r\n", CommandResultGetStringPrt(gtConsoleLookupArray[eConsolePort].tInputCommand.eLastResult) );
        }
        else
        {
            //Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "\r\n" );
            Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "ERROR ");
            Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "No Command Dictionary Mounted!\r\n" );
            gtConsoleLookupArray[eConsolePort].tInputCommand.eLastResult = COMMAND_RESULT_UNKNOWN;
        }
    }
}

void ConsoleCommandDisplayNext( ConsolePortEnum eConsolePort )
{
    CHAR    cChar;
    UINT32  dwCurrentIndex = 0;

    // MOUNT PREVIOUS COMMAND
    // increment the History Index
    //StrQueueCirIndexMoveToNext( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue );
    if( StrQueueCirIndexMoveToNext( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue ) )
    {
        // clear line buffer to get overwritten by new string
        StrLineEditorBufferReset( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor );
        // update line buffer with new command
        dwCurrentIndex = 0;
        while( StrQueueCirGetCharFromCurrentString( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue, dwCurrentIndex, &cChar ) )
        {
            dwCurrentIndex++;
            StrLineEditorBufferInsertChar( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor, cChar );
        }

        // clear CONSOLE line
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_CLEAR_LINE );
        // print CONSOLE prompt at the beginning of the line
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_PROMPT );
        // print into CONSOLE the loaded command from history
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "%s", StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );
    }
}

void ConsoleCommandDisplayPrevious( ConsolePortEnum eConsolePort )
{
	CHAR    cChar;
    UINT32  dwCurrentIndex = 0;

    // MOUNT PREVIOUS COMMAND
    // increment the History Index
    //StrQueueCirIndexMoveToPrevious( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue );
    if( StrQueueCirIndexMoveToPrevious( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue ) )
    {
        // clear line buffer to get overwritten by new string
        StrLineEditorBufferReset( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor );
        // update line buffer with new command
        dwCurrentIndex = 0;
        while( StrQueueCirGetCharFromCurrentString( gtConsoleLookupArray[eConsolePort].tInputCommand.eCircularQueue, dwCurrentIndex, &cChar ) )
        {
            dwCurrentIndex++;
            StrLineEditorBufferInsertChar( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor, cChar );
        }

        // clear CONSOLE line
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_CLEAR_LINE );
        // print CONSOLE prompt at the beginning of the line
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, CONSOLE_STRING_PROMPT );
        // print into CONSOLE the loaded command from history
        Printf( gtConsoleLookupArray[eConsolePort].eIoStream, "%s", StrLineEditorEditedStringGetPointer( gtConsoleLookupArray[eConsolePort].tInputCommand.eLineBufferEditor ) );
    }
}


void ConsoleTest( void )
{
    ConsoleModuleInit();
    ConsoleInputProcessingAutoUpdateEnable( CONSOLE_PORT_MAIN, TRUE );

    ConsoleEnable( CONSOLE_PORT_MAIN, TRUE );
    //ConsolePrintfEnable( CONSOLE_PORT_MAIN, TRUE );

    ConsoleCommandDictionarySet( CONSOLE_PORT_MAIN, CommandDictionaryGetPointer( COMMAND_DICTIONARY_1 ) );

    //UINT32  dwDelayDuration_mSec= 50;
    UINT32  dwDelayDuration_mSec= 5000;
    TIMER   xTimer              = TimerDownTimerStartMs( dwDelayDuration_mSec );

    Printf( IO_STREAM_USER, "> " );

    while(1)
    {
        ConsoleInputProcessingAutoUpdate();

        if( TimerDownTimerIsExpired(xTimer) == TRUE )
        {
            // restart timer
            xTimer    = TimerDownTimerStartMs( dwDelayDuration_mSec );

            // print message
            //ConsolePrintf( CONSOLE_PORT_MAIN, "Test printf\r\n" );
        }
    }
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
