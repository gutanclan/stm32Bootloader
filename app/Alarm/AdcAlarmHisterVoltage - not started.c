//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        strQueueCir.h
//!    \brief       queue module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "../Utils/Types.h"
#include "./strQueueCir.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    CHAR *          pcBuffer;		    /* body of queue                */
    const UINT32 	dwBufferSize;       /* max size*/

    // Indicates the position to insert a new string
    UINT32	        dwNewStringIndex;
	// Indicates the amount of string inside the buffer
    UINT8	        bStringCounter;
	// Holds an index to access the strings in the buffer. By using the arrows, the index change, therefore you can access different string
	UINT32	        dwStringBrowserBufferIndex;
}CirQueueType;

// make sure this
#define STR_QUEUE_CIR_STRING_DELIMITER_START 				(0x01)
#define STR_QUEUE_CIR_STRING_DELIMITER_END 					(0xFF)
#define STR_QUEUE_CIR_CHAR_SMALLEST_VALID                   (32)    // 'space'
#define STR_QUEUE_CIR_CHAR_GREATEST_VALID                   (126)   // '~'


// declare number of elements per queue
#define STR_QUEUE_CIR_CONSOLE_1_BUFFER_SIZE_BYTES           (15)

// declare buffer of required type
static CHAR	gcQueue1Buffer[STR_QUEUE_CIR_CONSOLE_1_BUFFER_SIZE_BYTES];

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// This table must be in the same order as QueueEnum in queue.h
static CirQueueType gtQueueLookupArray[] =
{
    //	*pcBuffer						MAX SIZE
    {   &gcQueue1Buffer[0],             STR_QUEUE_CIR_CONSOLE_1_BUFFER_SIZE_BYTES   },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
};

#define STR_QUEUE_CIR_ARRAY_MAX	( sizeof(gtQueueLookupArray) / sizeof( gtQueueLookupArray[0] ) )

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

static UINT32 StrQueueCirBufferGetNextIndex( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentIndex );
static UINT32 StrQueueCirBufferGetPreviousIndex( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentIndex );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void QueueModuleInit( void )
//!
//! \brief  Initialize queue variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrQueueCirModuleInit( void )
{
    BOOL fSuccess = FALSE;

    if( STR_QUEUE_CIR_MAX != STR_QUEUE_CIR_ARRAY_MAX )
    {
        // definition mismatch error
        while(0);
    }
    else
    {
        fSuccess = TRUE;

        for( UINT32 dwQCounter = 0 ; dwQCounter < STR_QUEUE_CIR_ARRAY_MAX ; dwQCounter++ )
        {
            fSuccess &= StrQueueCirResetQueue( dwQCounter );
        }
    }

    return fSuccess;
}

BOOL StrQueueCirResetQueue( StrQueueCirEnum eStrQueueCir )
{
    BOOL fSuccess = FALSE;

    if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
    {
        // memset 0 to buffer
        memset( &gtQueueLookupArray[eStrQueueCir].pcBuffer[0], 0, gtQueueLookupArray[eStrQueueCir].dwBufferSize );

        gtQueueLookupArray[eStrQueueCir].dwNewStringIndex           = 0;

        gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex = 0;

        gtQueueLookupArray[eStrQueueCir].bStringCounter             = 0;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL StrQueueCirInsertString( QueueEnum eQueue, const void * pvItemToQueue )
//!
//! \brief  send an item to the queue
//!
//! \param[in]  eQueue          queue selected
//! \param[in]  pvItemToQueue   pointer to item to be queued
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrQueueCirInsertString( StrQueueCirEnum eStrQueueCir, CHAR *pcStringBuffer )
{
    BOOL 	fSuccess 	= FALSE;
	UINT32 	dwStringLen = 0;
	CHAR    cChar;

    if( NULL != pcStringBuffer )
    {
        if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
        {
			dwStringLen = strlen( (const char *)pcStringBuffer );

			// make sure the string is smaller than the circular buffer(-2 for the reserved 2 delimiter bytes START/END)
			dwStringLen = ( (dwStringLen) < (gtQueueLookupArray[eStrQueueCir].dwBufferSize-2) ) ? dwStringLen : (gtQueueLookupArray[eStrQueueCir].dwBufferSize - 2);

            if( dwStringLen > 0 )
            {
                ///////////////////////////////////////////////////////
                // 	START DELIMITER CHAR
                ///////////////////////////////////////////////////////
                // If there is a START delimiter at the "about to be over written character", meaning, a string is going to be overwritten, then decrement string counter
                if( gtQueueLookupArray[eStrQueueCir].pcBuffer[gtQueueLookupArray[eStrQueueCir].dwNewStringIndex] == STR_QUEUE_CIR_STRING_DELIMITER_START )
                {
                    gtQueueLookupArray[eStrQueueCir].bStringCounter--;
                }

                // stamp the new String with the START delimiter
                gtQueueLookupArray[eStrQueueCir].pcBuffer[ gtQueueLookupArray[eStrQueueCir].dwNewStringIndex ] = STR_QUEUE_CIR_STRING_DELIMITER_START;

                // set the string access index at the last inserted string (at START byte)
                gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex = gtQueueLookupArray[eStrQueueCir].dwNewStringIndex;

                // increment by 1 the index
                gtQueueLookupArray[eStrQueueCir].dwNewStringIndex = StrQueueCirBufferGetNextIndex( eStrQueueCir, gtQueueLookupArray[eStrQueueCir].dwNewStringIndex );

                ///////////////////////////////////////////////////////

                ///////////////////////////////////////////////////////
                // 	STRING CHARS
                ///////////////////////////////////////////////////////
                // fill up the rest with the string data
                for( UINT32 dwCharCounter = 0 ; dwCharCounter < dwStringLen ; dwCharCounter++ )
                {
                    // If there is a START delimiter at the "about to be over written character", meaning, a string is going to be overwritten, then decrement string counter
                    if( gtQueueLookupArray[eStrQueueCir].pcBuffer[gtQueueLookupArray[eStrQueueCir].dwNewStringIndex] == STR_QUEUE_CIR_STRING_DELIMITER_START )
                    {
                        gtQueueLookupArray[eStrQueueCir].bStringCounter--;
                    }

                    cChar = pcStringBuffer[ dwCharCounter ];
                    // make sure char is a printable character, otherwise substitute it for a printable one
                    if
                    (
                        ( cChar < STR_QUEUE_CIR_CHAR_SMALLEST_VALID ) ||
                        ( cChar > STR_QUEUE_CIR_CHAR_GREATEST_VALID )
                    )
                    {
                        // substitute it
                        cChar = '?';
                    }

                    // copy char
                    gtQueueLookupArray[eStrQueueCir].pcBuffer[ gtQueueLookupArray[eStrQueueCir].dwNewStringIndex ] = cChar;

                    // increment by 1 the index
                    gtQueueLookupArray[eStrQueueCir].dwNewStringIndex = StrQueueCirBufferGetNextIndex( eStrQueueCir, gtQueueLookupArray[eStrQueueCir].dwNewStringIndex );
                }

                ///////////////////////////////////////////////////////
                // 	END DELIMITER CHAR
                ///////////////////////////////////////////////////////
                // If there is a START delimiter at the "about to be over written character", meaning, a string is going to be overwritten, then decrement string counter
                if( gtQueueLookupArray[eStrQueueCir].pcBuffer[gtQueueLookupArray[eStrQueueCir].dwNewStringIndex] == STR_QUEUE_CIR_STRING_DELIMITER_START )
                {
                    gtQueueLookupArray[eStrQueueCir].bStringCounter--;
                }

                // stamp the new String with the END delimiter
                gtQueueLookupArray[eStrQueueCir].pcBuffer[ gtQueueLookupArray[eStrQueueCir].dwNewStringIndex ] = STR_QUEUE_CIR_STRING_DELIMITER_END;

                // increment string counter since we finished adding the new string
                gtQueueLookupArray[eStrQueueCir].bStringCounter++;

                // set the new string index ready for the next insertion
                // increment by 1 the index
                gtQueueLookupArray[eStrQueueCir].dwNewStringIndex = StrQueueCirBufferGetNextIndex( eStrQueueCir, gtQueueLookupArray[eStrQueueCir].dwNewStringIndex );
                ///////////////////////////////////////////////////////

                fSuccess = TRUE;
            }
		}
	}

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL QueueReceive( QueueEnum eQueue, void * pvBuffer, UINT32 dwBufferSize )
//!
//! \brief  return and remove item from queue
//!
//! \param[in]  eQueue          queue selected
//! \param[in]  pvItemToQueue   pointer to item to be dequeued
//! \param[in]  dwBufferSize    size of buffer to hold the item to be dequeued
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrQueueCirGetStringAtCurrentIndex( StrQueueCirEnum eStrQueueCir, CHAR *pcStringBuffer, UINT32 dwStringBufferSize )
{
	BOOL fSuccess = FALSE;
	if
	(
		( pcStringBuffer != NULL ) &&
		( dwStringBufferSize > 0 )
	)
    {
		UINT32 dwIndex          = 0;

		if( gtQueueLookupArray[eStrQueueCir].bStringCounter != 0 )
        {
            // start the copy of characters from the string
            // start copying at START+1 index and end before copying END char
            // dwStringBrowserBufferIndex should guarantee it always points to a complete string(START/END).
            // string of size 0 should not be allowed to be in buffer
            do
            {
                pcStringBuffer[dwIndex] = gtQueueLookupArray[eStrQueueCir].pcBuffer[ StrQueueCirBufferGetNextIndex( eStrQueueCir, dwIndex + gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex ) ];

                if( pcStringBuffer[dwIndex] == STR_QUEUE_CIR_STRING_DELIMITER_END )
                {
                    break;
                }

                // next character
                dwIndex++;

            }while( dwIndex < (dwStringBufferSize-1) );
        }

		// set the last char as termination of string;
		pcStringBuffer[dwIndex] = '\0';

		fSuccess = TRUE;
	}

	return fSuccess;
}

// this function is meant to be used in a loop starting at CharIndex 0.
// if returns false, it means no more chars available for the current string
BOOL StrQueueCirGetCharFromCurrentString( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentStringCharIndex, CHAR *pcChar )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX ) &&
        ( NULL != pcChar )
    )
    {
        if( gtQueueLookupArray[eStrQueueCir].bStringCounter != 0 )
        {
            UINT32 dwIndex = gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex + 1 + dwCurrentStringCharIndex;

            dwIndex = ( dwIndex % gtQueueLookupArray[eStrQueueCir].dwBufferSize );

            if
            (
               gtQueueLookupArray[eStrQueueCir].pcBuffer[ dwIndex ] != STR_QUEUE_CIR_STRING_DELIMITER_END
            )
            {
                (*pcChar)= gtQueueLookupArray[eStrQueueCir].pcBuffer[ dwIndex ];
                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}

// returns next index
UINT32 StrQueueCirBufferGetNextIndex( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentIndex )
{
	if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
    {
		dwCurrentIndex = (dwCurrentIndex+1) % gtQueueLookupArray[eStrQueueCir].dwBufferSize;
	}
	else
    {
        dwCurrentIndex = 0;
    }

	return dwCurrentIndex;
}

// returns prev index
UINT32 StrQueueCirBufferGetPreviousIndex( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentIndex )
{
	if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
    {
        if( dwCurrentIndex == 0 )
        {
            dwCurrentIndex = gtQueueLookupArray[eStrQueueCir].dwBufferSize - 1;
        }
        else
        {
            dwCurrentIndex = (dwCurrentIndex-1);
        }
	}
	else
    {
        dwCurrentIndex = 0;
    }

	return dwCurrentIndex;
}

//*************
BOOL StrQueueCirIndexMoveToNext( StrQueueCirEnum eStrQueueCir )
{
	BOOL fSuccess = FALSE;

	if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
    {
        if( gtQueueLookupArray[eStrQueueCir].bStringCounter != 0 )
        {
            // move to prev index in the history buffer until find a START
            do
            {
                gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex = StrQueueCirBufferGetPreviousIndex( eStrQueueCir, gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex );
            }while( gtQueueLookupArray[eStrQueueCir].pcBuffer[ gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex] != STR_QUEUE_CIR_STRING_DELIMITER_START );
        }

		fSuccess = TRUE;
	}

	return fSuccess;
}

//*************
BOOL StrQueueCirIndexMoveToPrevious( StrQueueCirEnum eStrQueueCir )
{
	BOOL fSuccess = FALSE;

	if( eStrQueueCir < STR_QUEUE_CIR_ARRAY_MAX )
    {
        if( gtQueueLookupArray[eStrQueueCir].bStringCounter != 0 )
        {
            // move to prev index in the history buffer until find a START
            do
            {
                gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex = StrQueueCirBufferGetNextIndex( eStrQueueCir, gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex );
            }while( gtQueueLookupArray[eStrQueueCir].pcBuffer[gtQueueLookupArray[eStrQueueCir].dwStringBrowserBufferIndex] != STR_QUEUE_CIR_STRING_DELIMITER_START );
        }

		fSuccess = TRUE;
	}

	return fSuccess;
}

void StrQueueCirTest( void )
{
	CHAR szStr1[] = "test1";
	CHAR szStr2[] = "test2";
	CHAR szStr3[] = "test3";
	CHAR cTestBuffer[20];

	// test 1
	// push buttons with no string added
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToNext( STR_QUEUE_CIR_CONSOLE_1 );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToPrevious( STR_QUEUE_CIR_CONSOLE_1 );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, NULL );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	// test 2
	// add 1 string
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"hey how 1234567890 abcdefghijklmn" );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	StrQueueCirResetQueue( STR_QUEUE_CIR_CONSOLE_1 );

    memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"hello world" );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToNext( STR_QUEUE_CIR_CONSOLE_1 );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToPrevious( STR_QUEUE_CIR_CONSOLE_1 );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, NULL );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	// request string
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"str 1" );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"str 2" );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"str 3" );
	StrQueueCirInsertString( STR_QUEUE_CIR_CONSOLE_1, (CHAR *)"str 4" );

	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToNext( STR_QUEUE_CIR_CONSOLE_1 );

    UINT32 dwCurrentStringCharIndex = 0;
	CHAR cChar = '\0';
	while( StrQueueCirGetCharFromCurrentString( STR_QUEUE_CIR_CONSOLE_1, dwCurrentStringCharIndex, &cChar ) )
    {
        dwCurrentStringCharIndex++;
        // print character here
        // reset char
        cChar = '\0';
    }

	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToNext( STR_QUEUE_CIR_CONSOLE_1 );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );

	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToPrevious( STR_QUEUE_CIR_CONSOLE_1 );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	memset( &cTestBuffer[0], 0, sizeof(cTestBuffer) );
	StrQueueCirIndexMoveToPrevious( STR_QUEUE_CIR_CONSOLE_1 );
	StrQueueCirGetStringAtCurrentIndex( STR_QUEUE_CIR_CONSOLE_1, &cTestBuffer[0], sizeof(cTestBuffer) );
	// next string
	// request string
    // prev string
    // request string

    // test 3
    // add strings until overlap one of them
    // request string
	// next string
	// request string
    // prev string
    // request string
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
//

