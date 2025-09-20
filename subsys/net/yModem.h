//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _Y_MODEM_H_
#define _Y_MODEM_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    Y_MODEM_ERROR_NON       = 0,
    Y_MODEM_ERROR_BUFFER_INVALID,
    Y_MODEM_ERROR_TIME_OUT,
    Y_MODEM_ERROR_RETRY_MAX,
    Y_MODEM_ERROR_STORAGE_SIZE,
    Y_MODEM_ERROR_DATA_SIZE_MISMATCH,
    Y_MODEM_ERROR_NO_EOF,
    Y_MODEM_ERROR_DATA_TYPE_MISMATCH,
    Y_MODEM_ERROR_RESPONSE_INVALID,
    Y_MODEM_ERROR_UNKNOWN,

    Y_MODEM_ERROR_MAX
}yModemErrorTypeEnum;

typedef enum
{
    Y_MODEM_TRANSFER_TYPE_BIN = 0,
    Y_MODEM_TRANSFER_TYPE_ASCII,

    Y_MODEM_TRANSFER_TYPE_MAX
}yModemTransferTypeEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: if streams are files then , they should be ready for read/write before start transferring
BOOL 	yModemConfigBuffer     	( UINT8 *pbBuffer, UINT16 wBufferSize );
BOOL	yModemReceiveFile       ( yModemTransferTypeEnum eType, UINT32 dwMaxStorageSizeBytes, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, IoStreamSourceEnum dataOutputStream );

BOOL	yModemGetLastError      ( yModemErrorTypeEnum *peError );
CHAR *	yModemGetErrorName      ( yModemErrorTypeEnum eError );

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif
