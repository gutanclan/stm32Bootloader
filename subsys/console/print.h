//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _PRINT_H_
#define _PRINT_H_

typedef enum
{
	PRINT_STREAM_USER = IO_STREAM_USER,

	PRINT_STREAM_MAX
}PrintStreamEnum;

#define PRINT_STREAM_INVALID (PRINT_STREAM_MAX)

void 	PrintSetBuffer					( CHAR *pcBuffer, UINT16 wBufferSize );

INT32  	Printf							( PrintStreamEnum ePrintStream, const char *format, ... );
BOOL    PrintChar                       ( PrintStreamEnum ePrintStream, CHAR cChar );
BOOL    PrintBuffer                     ( PrintStreamEnum ePrintStream, CHAR *pvBuffer, UINT16 wBufferSize );

void    PrintDebugfEnable          		( BOOL fPrintDebugfEnable );
BOOL    PrintDebugfIsEnabled       		( void );
INT32  	PrintDebugf						( PrintStreamEnum ePrintStream, BOOL fEnable, const char *format, ... );

#endif // _PRINT_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
