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
#include "./strLineEditor.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define STR_LINE_BUFFER_1_SIZE_BYTES                        (128)
static CHAR	gcLineBuffer1[STR_LINE_BUFFER_1_SIZE_BYTES];

typedef struct
{
    CHAR *          pcBuffer;		    /* body of queue                */
    UINT32 	        dwBufferSize;       /* max size*/

    // Indicates the position to insert a new char
	UINT32	        dwCursorIndex;
	UINT32	        dwStringEndIndex;
}StrLineEditorType;

// make sure is inside printabel chars
#define STR_LINE_EDITOR_CHAR_SMALLEST_VALID                 (32)    // 'space'
#define STR_LINE_EDITOR_CHAR_GREATEST_VALID                 (126)   // '~'

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// This table must be in the same order as QueueEnum in queue.h
static StrLineEditorType gtLineEditorLookupArray[STR_LINE_EDITOR_MAX] =
{
    //	*pcBuffer						buffer SIZE             OTHERS
    {   &gcLineBuffer1[0],              sizeof(gcLineBuffer1),  0,0  },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
};

#define STR_LINE_BUFFER_ARRAY_MAX	( sizeof(gtLineEditorLookupArray) / sizeof( gtLineEditorLookupArray[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorModuleInit( void )
{
    BOOL fSuccess = FALSE;

    if( STR_LINE_EDITOR_MAX != STR_LINE_BUFFER_ARRAY_MAX )
    {
        // definition mismatch error
        while(0);
    }
    else
    {
        fSuccess = TRUE;

        /*for( UINT32 dwECounter = 0 ; dwECounter < STR_LINE_BUFFER_ARRAY_MAX ; dwECounter++ )
        {
            fSuccess &= StrLineEditorBufferReset( dwECounter );
        }*/
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorBufferReset( StrLineEditorEnum eLineEditor )
{
	BOOL fSuccess = FALSE;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer)
    )
    {
		memset( &gtLineEditorLookupArray[eLineEditor].pcBuffer[0], 0, gtLineEditorLookupArray[eLineEditor].dwBufferSize );

		gtLineEditorLookupArray[eLineEditor].dwCursorIndex 	    = 0;
		gtLineEditorLookupArray[eLineEditor].dwStringEndIndex 	= 0;

		fSuccess = TRUE;
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorIsCharValid( CHAR cChar )
{
	if
	(
		( cChar >= STR_LINE_EDITOR_CHAR_SMALLEST_VALID ) &&
		( cChar <= STR_LINE_EDITOR_CHAR_GREATEST_VALID )
	)
	{
		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorCursorMoveForward( StrLineEditorEnum eLineEditor )
{
	BOOL fSuccess = FALSE;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer)
    )
    {
		if( gtLineEditorLookupArray[eLineEditor].dwCursorIndex < gtLineEditorLookupArray[eLineEditor].dwStringEndIndex )
		{
            gtLineEditorLookupArray[eLineEditor].dwCursorIndex++;

            fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorCursorMoveBackward( StrLineEditorEnum eLineEditor )
{
	BOOL fSuccess = FALSE;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer)
    )
    {
		if( gtLineEditorLookupArray[eLineEditor].dwCursorIndex > 0 )
        {
			gtLineEditorLookupArray[eLineEditor].dwCursorIndex--;

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorBufferInsertChar( StrLineEditorEnum eLineEditor, CHAR cChar )
{
	BOOL 	fSuccess = FALSE;
	UINT16 	wIndex = 0;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer)
    )
    {
		// validate char
		if( StrLineEditorIsCharValid( cChar ) == FALSE )
		{
			cChar = '?';
		}

        if( gtLineEditorLookupArray[eLineEditor].dwCursorIndex < (gtLineEditorLookupArray[eLineEditor].dwBufferSize-1) )
        {
            // shift chars to right from the cursor new char
            if( gtLineEditorLookupArray[eLineEditor].dwCursorIndex <= (gtLineEditorLookupArray[eLineEditor].dwBufferSize-3) )
            {
                for( wIndex = gtLineEditorLookupArray[eLineEditor].dwStringEndIndex ; gtLineEditorLookupArray[eLineEditor].dwCursorIndex > wIndex ; wIndex-- )
                {
                    gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex+1] = gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex];
                }
            }

            // insert char at cursor new char
            gtLineEditorLookupArray[eLineEditor].pcBuffer[gtLineEditorLookupArray[eLineEditor].dwCursorIndex] = cChar;

            gtLineEditorLookupArray[eLineEditor].dwCursorIndex++;

            // stringEnd only walk if less than buffer size -2
            if( gtLineEditorLookupArray[eLineEditor].dwStringEndIndex <= (gtLineEditorLookupArray[eLineEditor].dwBufferSize-2) )
            {
                gtLineEditorLookupArray[eLineEditor].dwStringEndIndex++;
            }

            fSuccess = TRUE;
        }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorBufferDeleteChar( StrLineEditorEnum eLineEditor, CHAR *pcChar )
{
	BOOL 	fSuccess = FALSE;
	UINT16 	wIndex = 0;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer)
    )
    {
		// if string end is align to cursor
		if( gtLineEditorLookupArray[eLineEditor].dwCursorIndex > 0 )
        {
            // retrieve deleted char
            if( NULL != pcChar )
            {
                *pcChar = gtLineEditorLookupArray[eLineEditor].pcBuffer[gtLineEditorLookupArray[eLineEditor].dwCursorIndex-1];
            }

            // shirt chars to left from the cursor new char
            for( wIndex = (gtLineEditorLookupArray[eLineEditor].dwCursorIndex-1) ; wIndex < gtLineEditorLookupArray[eLineEditor].dwStringEndIndex ; wIndex++ )
            {
                gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex] = gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex+1];
            }

            // decrement string End if grater than 0
            gtLineEditorLookupArray[eLineEditor].dwStringEndIndex--;
            gtLineEditorLookupArray[eLineEditor].dwCursorIndex--;

            fSuccess = TRUE;
        }
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorBufferGetCopy( StrLineEditorEnum eLineEditor, CHAR *pcBuffer, UINT16 wBufferSize )
{
	BOOL fSuccess = FALSE;

	if
	(
        ( NULL != pcBuffer ) &&
		( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
		( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer) &&
		( wBufferSize > 0 )
	)
	{
		// make sure buffer size is big enough to hold the copy of string
		UINT16 wIndex = 0;

		for( wIndex = 0 ; wIndex < (wBufferSize-1) ; wIndex++ )
		{
			if( gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex] == '\0' )
			{
				break;
			}

			// copy char
			pcBuffer[wIndex] = gtLineEditorLookupArray[eLineEditor].pcBuffer[wIndex];
		}

		pcBuffer[wIndex] = '\0';

		fSuccess = TRUE;
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * StrLineEditorEditedStringGetPointer( StrLineEditorEnum eLineEditor )
{
	if( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX )
    {
        return &gtLineEditorLookupArray[eLineEditor].pcBuffer[0];
    }
    else
    {
        return NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StrLineEditorCursorGetCurrentIndex( StrLineEditorEnum eLineEditor, UINT16 *pwCursorCurrentIndex )
{
    BOOL 	fSuccess = FALSE;

	if
    (
        ( eLineEditor < STR_LINE_BUFFER_ARRAY_MAX ) &&
        ( NULL != gtLineEditorLookupArray[eLineEditor].pcBuffer) &&
        ( NULL != pwCursorCurrentIndex )
    )
    {
        fSuccess = TRUE;
        (*pwCursorCurrentIndex) = gtLineEditorLookupArray[eLineEditor].dwCursorIndex;
    }

    return fSuccess;
}



void StrLineEditorTest( void )
{
    // declare number of elements per queue
    //StrLineEditorSetBufferPointer( STR_LINE_EDITOR_1, &gcLineBuffer1[0], STR_LINE_BUFFER_1_SIZE_BYTES );

    CHAR cChar;
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );

    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'a' );
    cChar = '\0';
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, &cChar );

    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'a' );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'b' );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'c' ); //test insert limit
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );// test delete limit

    // now test move limits
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'a' );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'b' );
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );// test limit forward
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );// test limit back

    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL );
    StrLineEditorCursorMoveForward( STR_LINE_EDITOR_1 );
    StrLineEditorBufferDeleteChar( STR_LINE_EDITOR_1, NULL ); // here the string should be clear and cursor at 0


    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'a' );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'b' );
    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );
    StrLineEditorCursorMoveBackward( STR_LINE_EDITOR_1 );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'c' );
    StrLineEditorBufferInsertChar( STR_LINE_EDITOR_1, 'd' ); // this should end overwriting a and b
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
//

