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

#ifndef _STR_QUEUE_CIR_H_
#define _STR_QUEUE_CIR_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	STR_QUEUE_CIR_CONSOLE_1 = 0,

	STR_QUEUE_CIR_MAX,	     // Total Number of QUEUES defined
}StrQueueCirEnum;

#define     STR_QUEUE_CIR_INVALID_QUEUE    (STR_QUEUE_CIR_MAX)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    StrQueueCirModuleInit             	( void );
BOOL    StrQueueCirInsertString           	( StrQueueCirEnum eStrQueueCir, CHAR *pcStringBuffer );
BOOL    StrQueueCirGetStringAtCurrentIndex  ( StrQueueCirEnum eStrQueueCir, CHAR *pcStringBuffer, UINT32 dwStringBufferSize );
BOOL    StrQueueCirGetCharFromCurrentString ( StrQueueCirEnum eStrQueueCir, UINT32 dwCurrentStringCharIndex, CHAR *pcChar );

BOOL 	StrQueueCirIndexMoveToNext			( StrQueueCirEnum eStrQueueCir );
BOOL 	StrQueueCirIndexMoveToPrevious		( StrQueueCirEnum eStrQueueCir );

BOOL  	StrQueueCirResetQueue          		( StrQueueCirEnum eStrQueueCir );

void 	StrQueueCirTest						( void );

#endif // _QUEUE_CIRCULAR_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
