//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! this module give usart a more advance priting PRINTF() and allows to change stream from usart to file and vice versa
//! file will have read and write functions
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _IO_STREAM_H_
#define _IO_STREAM_H_

typedef enum
{
	IO_STREAM_SOURCE_TYPE_NON = 0,
	IO_STREAM_SOURCE_TYPE_SERIAL_PORT,
	IO_STREAM_SOURCE_TYPE_FILE,
	
	IO_STREAM_SOURCE_TYPE_MAX
}IoStreamSourceTypeEnum;

#define INPUT_STREAM_SOURCE_INVALID (IO_STREAM_SOURCE_TYPE_MAX)

typedef enum
{
	IO_STREAM_USER,
	
	IO_STREAM_MAX
}IoStreamSourceEnum;

#define IO_STREAM_INVALID (IO_STREAM_MAX)

BOOL	IoStreamInit						( void );

//////////////////////////////////////////////////////////////////////////////////////////////////
// INPUT
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    IoStreamInputGetChar          		( IoStreamSourceEnum eIoStream, CHAR *pcChar );
BOOL    IoStreamInputGetBuffer        		( IoStreamSourceEnum eIoStream, void *pvBuffer, UINT16 wBufferSize, UINT16 *pwBytesReceived, BOOL *pfIsEndOfFile );
IoStreamSourceTypeEnum IoStreamInputGetType	( IoStreamSourceEnum eIoStream );

//////////////////////////////////////////////////////////////////////////////////////////////////
// OUTPUT
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    IoStreamOutputPutChar          		( IoStreamSourceEnum eIoStream, CHAR cChar );
BOOL    IoStreamOutputPutBuffer        		( IoStreamSourceEnum eIoStream, void *pvBuffer,UINT16 wBytesToSend, UINT16 *pwBytesSent, BOOL *pfIsEndOfFile );
IoStreamSourceTypeEnum IoStreamOuputGetType	( IoStreamSourceEnum eIoStream );

//////////////////////////////////////////////////////////////////////////////////////////////////

CHAR *  IoStreamSourceTypeGetName        	( IoStreamSourceTypeEnum eIoStreamSourceType );

#endif // _CONSOLE_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
