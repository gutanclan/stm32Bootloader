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

#include "../../drivers/common/types.h"
//#include "../DriversMcuPeripherals/usart.h"
#include "../serial/serial.h"
#include "ioStream.h"

static const CHAR * const pcLabelArray[] =
{
	"SERIAL",
	"FILE"
};

typedef struct
{
	// TYPES OF Streams Supported
	IoStreamSourceTypeEnum 		eStreamSouceType;

	// enum for that particular stream for Example:
	// if stream is a usart port then go to Usart.h and grab
	// one of the port enums and use it to set the value of
	// this param
	UINT8						bStreamEnum;
}IoStreamControlType;

typedef struct
{
	IoStreamControlType	tIn;
	IoStreamControlType	tOut;
}IoStreamType;

static IoStreamType	gtIoStreamLookUpArray[] =
{
	// IO_STREAM_USER
	{
		//tIn
		{
			// eStreamSouceType
			IO_STREAM_SOURCE_TYPE_SERIAL_PORT,
			// bStreamEnum
			SERIAL_PORT_MAIN
		},
		//tOut
		{
			// eStreamSouceType
			IO_STREAM_SOURCE_TYPE_SERIAL_PORT,
			// bStreamEnum
			SERIAL_PORT_MAIN
		}
	},

	//////////////////////////////
	// ADD MORE STREAMS HERE
	//////////////////////////////
};

#define IO_STREAM_ARRAY_SIZE       ( sizeof(gtIoStreamLookUpArray)/sizeof(gtIoStreamLookUpArray[0]) )

////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL IoStreamInit( void )
{
	if( IO_STREAM_MAX != IO_STREAM_ARRAY_SIZE )
	{
		while(1);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// INPUT
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL IoStreamInputGetChar( IoStreamSourceEnum eIoStream, CHAR *pcChar )
{
	BOOL 			fSuccess = FALSE;
	UsartPortEnum 	ePort;

	if( eIoStream < IO_STREAM_MAX )
	{
		switch( gtIoStreamLookUpArray[eIoStream].tIn.eStreamSouceType )
		{
			case IO_STREAM_SOURCE_TYPE_NON:
				break;
			case IO_STREAM_SOURCE_TYPE_SERIAL_PORT:
				ePort = (SerialPortEnum) gtIoStreamLookUpArray[eIoStream].tIn.bStreamEnum;
				fSuccess = SerialGetChar( ePort, pcChar );
				break;
			case IO_STREAM_SOURCE_TYPE_FILE:
				break;
		}
	}

	return fSuccess;
}

BOOL IoStreamInputGetBuffer( IoStreamSourceEnum eIoStream, void *pvBuffer, UINT16 wBufferSize, UINT16 *pwBytesReceived, BOOL *pfIsEndOfFile )
{
	BOOL 			fSuccess = FALSE;
	UsartPortEnum 	ePort;

	if( eIoStream < IO_STREAM_MAX )
	{
		switch( gtIoStreamLookUpArray[eIoStream].tIn.eStreamSouceType )
		{
			case IO_STREAM_SOURCE_TYPE_NON:
				break;
			case IO_STREAM_SOURCE_TYPE_SERIAL_PORT:
				ePort = (SerialPortEnum) gtIoStreamLookUpArray[eIoStream].tIn.bStreamEnum;
				fSuccess = SerialGetBuffer( ePort, pvBuffer, wBufferSize, pwBytesReceived );

				// for usart this var always false
				if( pfIsEndOfFile != NULL )
				{
					(*pfIsEndOfFile) = FALSE;
				}
				break;
			case IO_STREAM_SOURCE_TYPE_FILE:
				break;
		}
	}

	return fSuccess;
}

IoStreamSourceTypeEnum IoStreamInputGetType( IoStreamSourceEnum eIoStream )
{
	if( eIoStream < IO_STREAM_MAX )
	{
		return gtIoStreamLookUpArray[eIoStream].tIn.eStreamSouceType;
	}
	else
	{
		return INPUT_STREAM_SOURCE_INVALID;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// OUTPUT
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL IoStreamOutputPutChar( IoStreamSourceEnum eIoStream, CHAR cChar )
{
	BOOL 			fSuccess = FALSE;
	SerialPortEnum 	ePort;

	if( eIoStream < IO_STREAM_MAX )
	{
		switch( gtIoStreamLookUpArray[eIoStream].tOut.eStreamSouceType )
		{
			case IO_STREAM_SOURCE_TYPE_NON:
				break;
			case IO_STREAM_SOURCE_TYPE_SERIAL_PORT:
				ePort = (SerialPortEnum) gtIoStreamLookUpArray[eIoStream].tOut.bStreamEnum;
				fSuccess = SerialPutChar( ePort, cChar );
				break;
			case IO_STREAM_SOURCE_TYPE_FILE:
				break;
		}
	}

	return fSuccess;
}

BOOL IoStreamOutputPutBuffer( IoStreamSourceEnum eIoStream, void *pvBuffer, UINT16 wBytesToSend, UINT16 *pwBytesSent, BOOL *pfIsEndOfFile )
{
	BOOL 			fSuccess = FALSE;
	SerialPortEnum 	ePort;

	if( eIoStream < IO_STREAM_MAX )
	{
		switch( gtIoStreamLookUpArray[eIoStream].tIn.eStreamSouceType )
		{
			case IO_STREAM_SOURCE_TYPE_NON:
				break;
			case IO_STREAM_SOURCE_TYPE_SERIAL_PORT:
				ePort = (SerialPortEnum) gtIoStreamLookUpArray[eIoStream].tOut.bStreamEnum;
				fSuccess = SerialPutBuffer( ePort, pvBuffer, wBytesToSend );

				if( pwBytesSent != NULL )
				{
					if( fSuccess )
					{
						(*pwBytesSent) = wBytesToSend;
					}
					else
					{
						(*pwBytesSent) = 0;
					}
				}

				// for usart this var always false
				if( pfIsEndOfFile != NULL )
				{
					(*pfIsEndOfFile) = FALSE;
				}
				break;
			case IO_STREAM_SOURCE_TYPE_FILE:
				break;
		}
	}

	return fSuccess;
}

IoStreamSourceTypeEnum IoStreamOuputGetType( IoStreamSourceEnum eIoStream )
{
	if( eIoStream < IO_STREAM_MAX )
	{
		return gtIoStreamLookUpArray[eIoStream].tOut.eStreamSouceType;
	}
	else
	{
		return INPUT_STREAM_SOURCE_INVALID;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////

CHAR *  IoStreamTypeGetName( IoStreamSourceTypeEnum eIoStreamSourceType )
{
	if( eIoStreamSourceType < IO_STREAM_SOURCE_TYPE_MAX )
	{
		return &pcLabelArray[eIoStreamSourceType][0];
	}
	else
	{
		return NULL;
	}
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
