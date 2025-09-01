//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        strQueueCir.h
//!    \brief       Circular queue to store strings. module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes      Considerations:
//!					* Stores characters only. Valid characters from ascii table [32 to 126]
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _STR_LINE_EDITOR_H_
#define _STR_LINE_EDITOR_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	STR_LINE_EDITOR_1 = 0,

	STR_LINE_EDITOR_MAX,	     // Total Number of String editor BUFFERs defined
}StrLineEditorEnum;

#define     STR_LINE_EDITOR_INVALID    (STR_LINE_EDITOR_MAX)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    StrLineEditorModuleInit                 ( void );

//BOOL    StrLineEditorSetBufferPointer           ( StrLineEditorEnum eLineEditor, CHAR *pcBuffer, UINT16 wBufferSize );
BOOL    StrLineEditorBufferInsertChar			( StrLineEditorEnum eLineEditor, CHAR cChar );
BOOL    StrLineEditorBufferDeleteChar			( StrLineEditorEnum eLineEditor, CHAR *pcChar );
BOOL	StrLineEditorBufferReset			    ( StrLineEditorEnum eLineEditor );

BOOL	StrLineEditorCursorMoveForward		    ( StrLineEditorEnum eLineEditor );
BOOL	StrLineEditorCursorMoveBackward		    ( StrLineEditorEnum eLineEditor );
BOOL	StrLineEditorCursorGetCurrentIndex		( StrLineEditorEnum eLineEditor, UINT16 *pwCursorCurrentIndex );

BOOL	StrLineEditorEditedStringGetCopy	    ( StrLineEditorEnum eLineEditor, CHAR *pcBuffer, UINT16 wBufferSize );
CHAR *  StrLineEditorEditedStringGetPointer	    ( StrLineEditorEnum eLineEditor );

#endif // _COMMAND_EDITOR_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
