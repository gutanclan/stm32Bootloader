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
#include <stdlib.h>         // atoi()

#include "../Utils/types.h"
#include "../Utils/timer.h"
#include "../Utils/stateMachine.h"
#include "../Utils/crc.h"

#include "../IoStream/ioStream.h"

#include "yModem.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
    NOTES:
    * protocol should be receiver driven

*/
//////////////////////////////////////////////////////////////////////////////////////////////////

#define XYZ_MODEM_SOH 					(0x01)		// Start of Header
#define XYZ_MODEM_STX 					(0x02)		// Start transmission 1024
#define XYZ_MODEM_EOT 					(0x04)		// End of Transmission
#define XYZ_MODEM_ETB 					(0x17)		// End of Transmission Block (Return to Amulet OS mode)
#define XYZ_MODEM_CAN 					(0x18)		// Cancel (Force receiver to start sending C's)

#define XYZ_MODEM_ACK 					(0x06)		// Aknowledge
#define XYZ_MODEM_NAK 					(0x15)		// Not Aknowledge

#define XYZ_MODEM_ASCII_C 				(0x43)		// ASCII "C"
#define XYZ_MODEM_DATA_EOF_CTRL_Z 		(0x1A)

#define XYZ_MODEM_ON_ERROR_RETRY_MAX 	(10)

#define YZ_MODEM_PACKET_DATA_LEN_BYTES  (1024)
#define YZ_MODEM_PACKET_TOT_LEN_BYTES   (1+1+1+YZ_MODEM_PACKET_DATA_LEN_BYTES+2)

#define XYZ_MODEM_PART_TIME_OUT_MSEC 	(3000)
#define XYZ_MODEM_C_FLAG_TIME_OUT_MSEC 	(3000)
#define XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC 	(30000)

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	UINT8 	bFileName[30];
	UINT32	dwFileSize;
}YModemFileDescriptor;

typedef struct
{
	UINT8 	*pbPayload;
	UINT16	wPayloadSize;
	UINT16 	wPayloadRxIdx;
}YModemPacketPayload;

typedef struct
{
	UINT8 				bStart;
	UINT8 				bSeqNum;
	UINT8 				bNegSeqNum;
	YModemPacketPayload tPayload;
	UINT16 				wCrc32;
}YModemPacket;

typedef struct
{
    BOOL 			    fIsError;
    yModemErrorTypeEnum eErrorType;
}YModemError;

typedef struct
{
	UINT8  			bPacketSeqNumExpected;
	UINT16  		wPacketByteCounter;
    UINT16  		wPayloadCrcCalculated;
	UINT16  		wPacketErrorCounter;
	YModemError     tError;
}YModemPacketStatus;

typedef struct
{
	YModemPacketStatus	tStatus;
	YModemPacket 		tPacket;
}YModemPacketStructure;

const CHAR * const pcErrorNameTable[] =
{
    "NO_ERROR",
    "ERR_BUFFER_INVALID",
    "ERR_TIME_OUT",
    "ERR_RETRY_MAX",
    "ERR_STORAGE_SIZE",
    "ERR_DATA_SIZE",
    "ERR_NO_EOF",
    "ERR_DATA_TYPE",
    "ERR_RESP_INVALID",
    "ERR_UNKNOWN",
};

static YModemPacketStructure gtYModemPacket;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void     yModemResetAll              ( void );
static void     yModemPacketReset           ( BOOL fIsPacketReceivedValid );
static BOOL     yModemPacketByteAdd         ( UINT8 bByte );
static BOOL     yModemPacketIsComplete      ( void );
static BOOL     yModemPacketIsValid         ( void );
static BOOL     yModemIsTransferInitiated   ( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream );
static BOOL     yModemFileGetDescriptor     ( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, YModemFileDescriptor *ptFileDescriptor );
static BOOL     yModemFileIsAccepted        ( yModemTransferTypeEnum eType, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, IoStreamSourceEnum dataOutputStream, UINT32 dwFileSize, UINT32 dwMaxStorageSizeBytes );
static UINT32   yModemFileGetChunk          ( yModemTransferTypeEnum eType, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, UINT32 dwBytesReceivedTotal, UINT32 dwFileExpectedSize );
static BOOL     yModemFileIsEofReceived   	( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream );
static BOOL     yModemGetBlockZero          ( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream );
static void     yModemSetError              ( yModemErrorTypeEnum eError );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemConfigBuffer( UINT8 *pbBuffer, UINT16 wBufferSize )
{
	BOOL fSuccess = FALSE;

    fSuccess = TRUE;

	if( pbBuffer != NULL )
	{
		if( wBufferSize >= YZ_MODEM_PACKET_DATA_LEN_BYTES )
		{
			gtYModemPacket.tPacket.tPayload.pbPayload 		= pbBuffer;
			gtYModemPacket.tPacket.tPayload.wPayloadSize 	= YZ_MODEM_PACKET_DATA_LEN_BYTES;

			fSuccess = TRUE;
		}
		else
		{
			gtYModemPacket.tPacket.tPayload.pbPayload 		= NULL;
			gtYModemPacket.tPacket.tPayload.wPayloadSize 	= 0;
		}
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void yModemSetError( yModemErrorTypeEnum eError )
{
    if( eError >= Y_MODEM_ERROR_MAX )
    {
        eError = Y_MODEM_ERROR_UNKNOWN;
    }

    if( gtYModemPacket.tStatus.tError.fIsError == FALSE )
    {
        gtYModemPacket.tStatus.tError.fIsError  = TRUE;
        gtYModemPacket.tStatus.tError.eErrorType= eError;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemReceiveFile( yModemTransferTypeEnum eType, UINT32 dwMaxStorageSizeBytes, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, IoStreamSourceEnum dataOutputStream )
{
    BOOL fSuccess = FALSE;

    CHAR cChar;
    // before starting clear chars in streams
    while( IoStreamInputGetChar( protocolInputStream, &cChar ) == TRUE ){};

    yModemResetAll();

    // 0.- check if the other size is waiting for transmission
    if( yModemIsTransferInitiated( protocolInputStream, protocolOutputStream ) == TRUE )
    {
        YModemFileDescriptor tFileDescriptor;

        // 1.- get File information
        if( yModemFileGetDescriptor( protocolInputStream, protocolOutputStream, &tFileDescriptor ) == TRUE )
        {
            if( tFileDescriptor.dwFileSize > 0 )
            {
                if( tFileDescriptor.dwFileSize <= dwMaxStorageSizeBytes )
                {
                    // 2.- Accept File
                    if( yModemFileIsAccepted( eType, protocolInputStream, protocolOutputStream, dataOutputStream, tFileDescriptor.dwFileSize, dwMaxStorageSizeBytes ) == TRUE )
                    {
                        gtYModemPacket.tStatus.tError.eErrorType        = Y_MODEM_ERROR_NON;
                        gtYModemPacket.tStatus.tError.fIsError          = FALSE;

                        fSuccess = TRUE;
                    }
                    else
                    {
                        yModemSetError( Y_MODEM_ERROR_UNKNOWN );
                    }
                }
                else
                {
                    yModemSetError( Y_MODEM_ERROR_STORAGE_SIZE );
                }
            }
            else
            {
                yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
            }
        }
        else
        {
            yModemSetError( Y_MODEM_ERROR_RESPONSE_INVALID );
        }
    }

    // 3.- Terminate Transfer by receiving the final empty packet or BLOCK ZERO.
    if( fSuccess )
    {
        yModemResetAll();

        if( yModemIsTransferInitiated( protocolInputStream, protocolOutputStream ) )
        {
            // this step is additional just so that Tera Term terminates its protocol correctly.
            yModemGetBlockZero( protocolInputStream, protocolOutputStream );
        }

        gtYModemPacket.tStatus.tError.eErrorType        = Y_MODEM_ERROR_NON;
        gtYModemPacket.tStatus.tError.fIsError          = FALSE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void yModemResetAll( void )
{
    gtYModemPacket.tStatus.bPacketSeqNumExpected    = 0;

    // if packet valid this get reset. (Max 10 retries)
    gtYModemPacket.tStatus.wPacketErrorCounter      = 0;

    // on every packet received, all this vars should be cleared
    gtYModemPacket.tStatus.wPacketByteCounter       = 0;
    gtYModemPacket.tStatus.wPayloadCrcCalculated    = 0;
    gtYModemPacket.tPacket.tPayload.wPayloadRxIdx   = 0;
    gtYModemPacket.tStatus.tError.eErrorType        = Y_MODEM_ERROR_UNKNOWN;
    gtYModemPacket.tStatus.tError.fIsError          = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void yModemPacketReset( BOOL fIsPacketReceivedValid )
{
    gtYModemPacket.tStatus.wPacketByteCounter       = 0;
    gtYModemPacket.tStatus.wPayloadCrcCalculated    = 0;
    gtYModemPacket.tPacket.tPayload.wPayloadRxIdx   = 0;

    if( fIsPacketReceivedValid )
    {
        gtYModemPacket.tStatus.tError.eErrorType        = Y_MODEM_ERROR_UNKNOWN;
        gtYModemPacket.tStatus.tError.fIsError          = FALSE;
        gtYModemPacket.tStatus.wPacketErrorCounter  = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemPacketByteAdd( UINT8 bByte )
{
    BOOL fSuccess = FALSE;

    gtYModemPacket.tStatus.wPacketByteCounter++;

    switch( gtYModemPacket.tStatus.wPacketByteCounter )
    {
        case 1: // STX
        {
            gtYModemPacket.tPacket.bStart = bByte;
            fSuccess = TRUE;
            break;
        }
        case 2: // SeqNum
        {
            gtYModemPacket.tPacket.bSeqNum = bByte;
            fSuccess = TRUE;
            break;
        }
        case 3: // ~SeqNum
        {
            gtYModemPacket.tPacket.bNegSeqNum = bByte;
            fSuccess = TRUE;
            break;
        }

        default: // Data and out of range bytes
        {
            if( (gtYModemPacket.tStatus.wPacketByteCounter >= 4) && (gtYModemPacket.tStatus.wPacketByteCounter <= (YZ_MODEM_PACKET_DATA_LEN_BYTES+3)) )
            {
                if( gtYModemPacket.tPacket.tPayload.wPayloadRxIdx < gtYModemPacket.tPacket.tPayload.wPayloadSize )
                {
                    if( gtYModemPacket.tPacket.tPayload.pbPayload != NULL )
                    {
                        gtYModemPacket.tPacket.tPayload.pbPayload[gtYModemPacket.tPacket.tPayload.wPayloadRxIdx] = bByte;
                        gtYModemPacket.tStatus.wPayloadCrcCalculated = Crc16Update( gtYModemPacket.tStatus.wPayloadCrcCalculated, &bByte, 1 );
                        fSuccess = TRUE;
                    }
                    else
                    {
                        yModemSetError( Y_MODEM_ERROR_BUFFER_INVALID );
                    }
                }
                gtYModemPacket.tPacket.tPayload.wPayloadRxIdx++;
            }
            break;
        }
        case 1028: // CRC16 MSB
        {
            gtYModemPacket.tPacket.wCrc32 = (bByte << 8) & 0xFF00;
            fSuccess = TRUE;
            break;
        }
        case 1029: // CRC16 LSB
        {
            gtYModemPacket.tPacket.wCrc32 |= (bByte & 0xFF);
            fSuccess = TRUE;
            break;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemPacketIsComplete( void )
{
    return (gtYModemPacket.tStatus.wPacketByteCounter >= YZ_MODEM_PACKET_TOT_LEN_BYTES);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemPacketIsValid( void )
{
    BOOL fIsValid = TRUE;

    fIsValid &= ( gtYModemPacket.tStatus.wPacketByteCounter == YZ_MODEM_PACKET_TOT_LEN_BYTES );
    fIsValid &= ( gtYModemPacket.tPacket.tPayload.wPayloadRxIdx == YZ_MODEM_PACKET_DATA_LEN_BYTES );

    fIsValid &= ( gtYModemPacket.tPacket.bStart  == XYZ_MODEM_STX );
    fIsValid &= ( gtYModemPacket.tPacket.bSeqNum == (0xff-gtYModemPacket.tPacket.bNegSeqNum) );
    fIsValid &= ( gtYModemPacket.tPacket.bSeqNum == gtYModemPacket.tStatus.bPacketSeqNumExpected );
    fIsValid &= ( gtYModemPacket.tPacket.wCrc32  == gtYModemPacket.tStatus.wPayloadCrcCalculated );

    return fIsValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemIsTransferInitiated( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream )
{
    BOOL fSuccess = FALSE;
    UINT8 bByte;
    TIMER xTimerCTimeOut;
    TIMER xTimerResponseTimeOut;

    yModemPacketReset(TRUE);

    xTimerCTimeOut          = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_TIME_OUT_MSEC );
    xTimerResponseTimeOut   = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );

    // send C's and wait for start of sequence
    IoStreamOutputPutChar( protocolOutputStream, XYZ_MODEM_ASCII_C );

    // protection: stop sequence if time expired or packet max exceeded
    while
    (
        ( TimerDownTimerIsExpired(xTimerResponseTimeOut) == FALSE ) && ( gtYModemPacket.tStatus.wPacketByteCounter < YZ_MODEM_PACKET_TOT_LEN_BYTES )
    )
    {
        if( IoStreamInputGetChar(protocolInputStream, &bByte ) )
        {
            if( yModemPacketByteAdd( bByte ) )
            {
                if( bByte == XYZ_MODEM_STX )
                {
                    fSuccess = TRUE;
                    break;
                }
                else
                {
                    // ERROR
                    yModemSetError( Y_MODEM_ERROR_RESPONSE_INVALID );
                    break;
                }
            }
            else
            {
                // ERROR
                // accepting byte
                yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
                break;
            }
        }

        if( TimerDownTimerIsExpired(xTimerCTimeOut) == TRUE )
        {
            IoStreamOutputPutChar( protocolOutputStream, XYZ_MODEM_ASCII_C );
            xTimerCTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_TIME_OUT_MSEC );
        }
    }

    // IS ERROR?
    if( fSuccess == FALSE )
    {
        if( TimerDownTimerIsExpired(xTimerResponseTimeOut) )
        {
            yModemSetError( Y_MODEM_ERROR_TIME_OUT );
        }
        else
        {
            yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemFileGetDescriptor( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, YModemFileDescriptor *ptFileDescriptor )
{
    BOOL fSuccess = FALSE;

    TIMER   xTimerResponseTimeOut;
    UINT8   bByte;

    if( ptFileDescriptor != NULL )
    {
        // set time out before starting loop
		xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );

        // protection: stop sequence if time expired or packet max exceeded
        while
        (
            ( TimerDownTimerIsExpired(xTimerResponseTimeOut) == FALSE ) && ( gtYModemPacket.tStatus.wPacketByteCounter < YZ_MODEM_PACKET_TOT_LEN_BYTES )
        )
        {
            if( IoStreamInputGetChar(protocolInputStream, &bByte ) )
            {
                if( yModemPacketByteAdd( bByte ) )
                {
                    if( yModemPacketIsComplete() )
                    {
                        if( yModemPacketIsValid() )
                        {
                            // extract File Name and size
                            memset( &ptFileDescriptor->bFileName[0], 0, sizeof(ptFileDescriptor->bFileName) );
                            ptFileDescriptor->dwFileSize = 0;

                            // expected format <fileName><'\0'><fileSize><others>
                            strncpy( (char *)&ptFileDescriptor->bFileName[0], (const char *)&gtYModemPacket.tPacket.tPayload.pbPayload[0], (sizeof(ptFileDescriptor->bFileName)-1) );

                            CHAR * pcFileSizeIdx = strchr( (const char *)&gtYModemPacket.tPacket.tPayload.pbPayload[0], '\0')+1;
                            ptFileDescriptor->dwFileSize = atoi((const char *)pcFileSizeIdx);

                            // increment sequence number
                            gtYModemPacket.tStatus.bPacketSeqNumExpected++;

                            fSuccess = TRUE;

                            // return ACK
                            IoStreamOutputPutChar(protocolOutputStream, XYZ_MODEM_ACK );

                            break;
                        }
                        else
                        {
                            if( gtYModemPacket.tStatus.wPacketErrorCounter < XYZ_MODEM_ON_ERROR_RETRY_MAX )
                            {
                                // reset vars to accept new packet
                                yModemPacketReset(FALSE);
                                // retry
                                gtYModemPacket.tStatus.wPacketErrorCounter++;

                                // return ACK
                                IoStreamOutputPutChar(protocolOutputStream, XYZ_MODEM_NAK );
                            }
                            else
                            {
                                // ERROR too many retries
                                yModemSetError( Y_MODEM_ERROR_RETRY_MAX );
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // ERROR
                    break;
                }

                // restart timer if there is activity in the input stream
				xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );
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
BOOL yModemFileIsAccepted( yModemTransferTypeEnum eType, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, IoStreamSourceEnum dataOutputStream, UINT32 dwFileExpectedSize, UINT32 dwMaxStorageSizeBytes )
{
    BOOL fSuccess = FALSE;
    UINT32 dwBytesReceivedChunk = 0;
    UINT32 dwBytesReceivedTotal = 0;

    if( dwFileExpectedSize < dwMaxStorageSizeBytes )
    {
        if( yModemIsTransferInitiated( protocolInputStream, protocolOutputStream ) == TRUE )
        {
            while( dwBytesReceivedTotal < dwFileExpectedSize )
            {
                dwBytesReceivedChunk = yModemFileGetChunk( eType, protocolInputStream, protocolOutputStream, dwBytesReceivedTotal, dwFileExpectedSize );

                if( dwBytesReceivedChunk > 0 )
                {
                    dwBytesReceivedTotal = dwBytesReceivedTotal + dwBytesReceivedChunk;

                    if( dwBytesReceivedTotal > dwFileExpectedSize )
                    {
                        yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
                        // ERROR more file bytes than expected
                        break;
                    }

                    // send data received to out put data stream
                    for( UINT32 dwIdx = 0 ; dwIdx < dwBytesReceivedChunk ; dwIdx++ )
                    {
                        IoStreamOutputPutChar( dataOutputStream, gtYModemPacket.tPacket.tPayload.pbPayload[dwIdx] );
                    }
                }
                else
                {
                    yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
                    // ERROR
                    break;
                }

                if( yModemFileIsEofReceived( protocolInputStream, protocolOutputStream ) == TRUE )
                {
                    fSuccess = TRUE;
                    break;
                }
                else
                {
                    continue;
                }
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
UINT32 yModemFileGetChunk( yModemTransferTypeEnum eType, IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream, UINT32 dwBytesReceivedTotal, UINT32 dwFileExpectedSize )
{
    UINT32 dwBytesReceived = 0;

    TIMER   xTimerResponseTimeOut;
    UINT8   bByte;
    BOOL    fIsAsciiError = FALSE;

    if( dwBytesReceivedTotal < dwFileExpectedSize )
    {
        // set time out before starting loop
		xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );

        // protection: stop sequence if time expired or packet max exceeded
        while
        (
            ( TimerDownTimerIsExpired(xTimerResponseTimeOut) == FALSE ) && ( gtYModemPacket.tStatus.wPacketByteCounter < YZ_MODEM_PACKET_TOT_LEN_BYTES )
        )
        {
            if( IoStreamInputGetChar(protocolInputStream, &bByte ) )
            {
                if( yModemPacketByteAdd( bByte ) )
                {
                    if( yModemPacketIsComplete() )
                    {
                        if( yModemPacketIsValid() )
                        {
                            if( ( gtYModemPacket.tPacket.tPayload.wPayloadSize + dwBytesReceivedTotal ) > dwFileExpectedSize )
                            {
                                dwBytesReceived = dwFileExpectedSize - dwBytesReceivedTotal;
                            }
                            else
                            {
                                dwBytesReceived = gtYModemPacket.tPacket.tPayload.wPayloadSize;
                            }

                            // check if bytes received are within range of ASCII values
                            if( eType == Y_MODEM_TRANSFER_TYPE_ASCII )
                            {
                                for( UINT32 dwIdx = 0 ; dwIdx < dwBytesReceived ; dwIdx++ )
                                {
                                    if( gtYModemPacket.tPacket.tPayload.pbPayload[dwIdx] > 0x7F )
                                    {
                                        fIsAsciiError = TRUE;
                                        break;
                                    }
                                }
                            }

                            if( fIsAsciiError )
                            {
                                yModemSetError( Y_MODEM_ERROR_DATA_TYPE_MISMATCH );
                                // set invalid received bytes
                                dwBytesReceived = 0;
                                // ERROR terminate loop
                                break;
                            }
                            else
                            {
                                // increment sequence number
                                gtYModemPacket.tStatus.bPacketSeqNumExpected++;

                                // return ACK
                                IoStreamOutputPutChar(protocolOutputStream, XYZ_MODEM_ACK );
                                break;
                            }
                        }
                        else
                        {
                            if( gtYModemPacket.tStatus.wPacketErrorCounter < XYZ_MODEM_ON_ERROR_RETRY_MAX )
                            {
                                // clear variables and try again
                                yModemPacketReset(FALSE);

                                // retry
                                gtYModemPacket.tStatus.wPacketErrorCounter++;

                                // return ACK
                                IoStreamOutputPutChar(protocolOutputStream, XYZ_MODEM_NAK );
                            }
                            else
                            {
                                yModemSetError( Y_MODEM_ERROR_RETRY_MAX );
                                // set invalid received bytes
                                dwBytesReceived = 0;
                                // ERROR too many retries
                                break;
                            }
                        }
                    }
                }
                else
                {
                    yModemSetError( Y_MODEM_ERROR_UNKNOWN );
                    // set invalid received bytes
                    dwBytesReceived = 0;
                    // ERROR
                    break;
                }

                // restart timer if there is activity in the input stream
				xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );
            }
        }
    }

    if( dwBytesReceived == 0 )
    {
        if( gtYModemPacket.tStatus.tError.fIsError == FALSE )
        {
            if( TimerDownTimerIsExpired(xTimerResponseTimeOut) )
            {
                yModemSetError( Y_MODEM_ERROR_TIME_OUT );
            }
            else
            {
                yModemSetError( Y_MODEM_ERROR_DATA_SIZE_MISMATCH );
            }
        }
    }

    return dwBytesReceived;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemFileIsEofReceived( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream )
{
    BOOL fSuccess = FALSE;
    UINT8 bByte;
    TIMER xTimerResponseTimeOut;

    yModemPacketReset(TRUE);

    xTimerResponseTimeOut   = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );

    // send C's and wait for start of sequence
    // protection: stop sequence if time expired or packet max exceeded
    while
    (
        ( TimerDownTimerIsExpired(xTimerResponseTimeOut) == FALSE ) && ( gtYModemPacket.tStatus.wPacketByteCounter < YZ_MODEM_PACKET_TOT_LEN_BYTES )
    )
    {
        if( IoStreamInputGetChar( protocolInputStream, &bByte ) )
        {
            if( yModemPacketByteAdd( bByte ) )
            {
                if( bByte == XYZ_MODEM_EOT )
                {
                    // return ACK
                    IoStreamOutputPutChar( protocolOutputStream, XYZ_MODEM_ACK );

                    fSuccess = TRUE;
                    break;
                }
                else
                {
                    // terminate loop. another packet is about to be sent
                    break;
                }
            }
            else
            {
                yModemSetError( Y_MODEM_ERROR_UNKNOWN );
                // ERROR accepting byte
                break;
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
BOOL yModemGetBlockZero( IoStreamSourceEnum protocolInputStream, IoStreamSourceEnum protocolOutputStream )
{
    BOOL fSuccess = FALSE;

    TIMER   xTimerResponseTimeOut;
    UINT8   bByte;

    // set time out before starting loop
    xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );

    // protection: stop sequence if time expired or packet max exceeded
    while
    (
        ( TimerDownTimerIsExpired(xTimerResponseTimeOut) == FALSE ) && ( gtYModemPacket.tStatus.wPacketByteCounter < YZ_MODEM_PACKET_TOT_LEN_BYTES )
    )
    {
        if( IoStreamInputGetChar(protocolInputStream, &bByte ) )
        {
            if( yModemPacketByteAdd( bByte ) )
            {
                if( yModemPacketIsComplete() )
                {
                    if( yModemPacketIsValid() )
                    {
                        fSuccess = TRUE;

                        // return ACK
                        IoStreamOutputPutChar(protocolOutputStream, XYZ_MODEM_ACK );
                    }
                    break;
                }
            }
            else
            {
                // ERROR
                break;
            }

            // restart timer if there is activity in the input stream
            xTimerResponseTimeOut = TimerDownTimerStartMs( XYZ_MODEM_C_FLAG_MAX_WAIT_MSEC );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL yModemGetLastError( yModemErrorTypeEnum *peError )
{
    BOOL fSuccess = FALSE;

    if( peError != NULL )
    {
        (*peError) = gtYModemPacket.tStatus.tError.eErrorType;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * yModemGetErrorName( yModemErrorTypeEnum eError )
{
    if( eError < Y_MODEM_ERROR_MAX )
    {
        return &pcErrorNameTable[eError][0];
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
