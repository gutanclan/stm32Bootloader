//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! DATA LINK PROTOCOL
//!
//! NOTE: this protocol is based on the principals of HDLC(High Level Data Link Control) protocol.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

// RTOS Library includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "Timer.h"

#include "./DlProtocolFrame.h"
#include "../Utils/QueueX.h"
#include "Usart.h"

#include "../Utils/ByteBitField.h"

#include "DlProtocol.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

// comment the define below to use unprotected queues
#define PROTOCOL_USE_PROTECTED_QUEUES                   (1)
#define PROTOCOL_PROTECTED_QUEUE_TICK_WAIT              (0)

#define PROTOCOL_TX_ERROR_MAX                           (10)
#define PROTOCOL_RX_ERROR_MAX                           (10)

#define PROTOCOL_FLAG_BYTE_ESCAPE                       (0x7C) // 124
#define PROTOCOL_FLAG_BYTE_SOT                          (0x7D) // 125

#define FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE            (0)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE            (1)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE            (3)

#define FRAME_FIELD_CNTRL_S_TYPE_ACK_VALUE              (0) // originally HDLC S-Frame/RR response
#define FRAME_FIELD_CNTRL_S_TYPE_NACK_VALUE             (2) // originally HDLC S-Frame/REJ response

#define FRAME_FIELD_CNTRL_U_TYPE_CONNECT_VALUE          (1)
#define FRAME_FIELD_CNTRL_U_TYPE_CONNECT_ACK_VALUE      (2)
#define FRAME_FIELD_CNTRL_U_TYPE_CONNECT_NACK_VALUE     (3)
// not used                                             (4)
#define FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_VALUE       (5)
#define FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_ACK_VALUE   (6)
// not used                                             (7)
#define FRAME_FIELD_CNTRL_U_TYPE_GENERIC_ERROR_VALUE    (8)

#define FRAME_FIELD_CNTRL_U_TYPE_SEQ_NUM_RESET_VALUE    (9)

// BIT OFFSET
#define FRAME_FIELD_ADDRESS_IS_8BIT_BIT_OFFSET          (0)
#define FRAME_FIELD_ADDRESS_ADDR_BIT_OFFSET             (1)

#define FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_OFFSET       (0)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_OFFSET       (0)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_U_BIT_OFFSET       (0)

#define FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_OFFSET (1)
#define FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_OFFSET (5)
#define FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_OFFSET       (1)
#define FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_OFFSET (5)
#define FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_OFFSET   (2)
#define FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_OFFSET   (5)

// BITS PER FIELD
#define FRAME_FIELD_ADDRESS_IS_8BIT_BIT_SIZE            (1)
#define FRAME_FIELD_ADDRESS_ADDR_BIT_SIZE               (7)

#define FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_SIZE         (1)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_SIZE         (2)
#define FRAME_FIELD_CNTRL_FRAME_TYPE_U_BIT_SIZE         (2)

#define FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_SIZE   (3)
#define FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_SIZE   (3)
#define FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_SIZE         (3)
#define FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_SIZE   (3)
#define FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_SIZE     (2)
#define FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_SIZE     (3)

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    DL_PROTOCOL_CONN_ADDR_MASTER = 0,
    DL_PROTOCOL_CONN_ADDR_SLAVE,
    
    DL_PROTOCOL_CONN_ADDR_MAX
}DlProtocolConnectionAddressEnum;

typedef struct
{
    UINT8   bAddress;
    BOOL    fIs8Bit;
}DlProtocolFrameFieldAddressBitFields;

typedef struct
{
    UINT8   bSeqNumReceive;
    UINT8   bSeqNumTransmit;
}DlProtocolFrameFieldControlIFrameBitFields;

typedef struct
{
    UINT8   bSeqNumReceive;
    UINT8   bType;
}DlProtocolFrameFieldControlSFrameBitFields;

typedef struct
{
    UINT8   bType;
}DlProtocolFrameFieldControlUFrameBitFields;

//////////////////////////////////////////////////////////////////////////////////////////////////

static UsartPortsEnum           geUsartPort     = USART6_PORT; //USART6 = CAM
static UsartPortsEnum           geUsartPortDbg  = USART5_PORT; //USART5 = INT B console     TARGET_USART_PORT_TO_INT_B_CONSOLE;

static BOOL                     gfDbgEnable     = FALSE;

static BOOL                     gfProtocolIsUsingPort = FALSE;

static TIMER                    gtSlaveConnectTimeOutTimer;
static TIMER                    gtSlaveSendConnectRequestTimer;
static BOOL                     gfSlaveIsConnected = FALSE;
static BOOL                     gfSlaveIsConnecting = FALSE;

static BOOL                     gfMasterIsListeningForConnections = FALSE;
static BOOL                     gfMasterIsConnected = FALSE;

static UINT8                    gbConnectionAddress = 0;

static UINT8                    gbContinuousTxErrorPacketCounter = 0;
static UINT8                    gbContinuousRxErrorPacketCounter = 0;
static BOOL                     gfSequenceNumberResetRequested = FALSE;

static UINT8                    gbISUFrameRxBuffer[DL_PROTOC_BUFFER_RX_FRAME_NO_SOT_MAX_BYTE_SIZE];     // linear
static DlProtocolFrame          gtISUFrameRx    =  { 0 };

static UINT16                   gwISUFrameRxBufferWalker = 0;
static BOOL                     gfISUFrameRxEscapeByteFound = FALSE;
static UINT16                   gwISUFrameRxCrc16 = 0;
static UINT8                    gbIFrameRxSequenceNum = 0;

static BOOL                     gfIsPutBufferBusy = FALSE;
static DlProtocolFrame          gtSUFrameTx     = { 0 };
static UINT8                    gbIFrameTxDataBuffer[DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE];
static DlProtocolFrame          gtIFrameTx = 
{ 
    // bAddress
    0,
    // bControl
    0,
    // tData
    {
        // pbArray
        &gbIFrameTxDataBuffer[0],
        // wArrMax
        sizeof(gbIFrameTxDataBuffer),
        // wDataLen
        0
    },
    // wCrc16
    0
};

static BOOL                     gfIFrameTxIsSent = FALSE;
static BOOL                     gfIFrameTxIsPending = FALSE;
static UINT32                   gdwIFrameTxRetryTimeOutMSec = 500;
//static UINT32                   gdwIFrameTxRetryTimeOutMSec = 100;
static TIMER                    gtIFrameTxRetryTimeOutTimer;
static UINT8                    gbIFrameTxSequenceNum = 0;

#ifdef PROTOCOL_USE_PROTECTED_QUEUES
static xQueueHandle             tQueueTxSuFrame;
static xQueueHandle             tQueueRxIFrameDataField;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

static void     PrintDbgFrame                   ( DlProtocolFrame *ptFrame, BOOL fIsIncomingFrame );

static void     UpdateSenderState               ( void );
static void     UpdateReceiverState             ( UINT8 bByte );
static BOOL     SendFrameDataByte               ( UINT8 bByte );
static BOOL     SendFrame                       ( DlProtocolFrame *ptFrame );

static BOOL     SendEscapedValidSizeRawBuffer   ( UINT8 *pbBuffer, UINT16 wBufferSize );

static BOOL     ParseFrame                      ( UINT8 *pbFrameBuffer, UINT16 dwBytesReceived, DlProtocolFrame *ptISUFrameResult );
static UINT16   FrameGetCrc16                   ( DlProtocolFrame *ptISUFrame );
static UINT16   UpdateCRC16                     ( UINT16 crcX, UINT8 data );

static BOOL     FieldControlSetType             ( UINT8 bControlFieldType, UINT8 *pbControlFieldResultByte );
static UINT8    FieldControlGetType             ( UINT8 bByte );
static BOOL     FieldAddressMergeParts          ( DlProtocolFrameFieldAddressBitFields *ptAddressParts, UINT8 *pbByteResult );
static BOOL     FieldAddressSeparateParts       ( UINT8 bByte, DlProtocolFrameFieldAddressBitFields *ptAddressPartsResult );
static BOOL     FieldControlIFrameSeparateParts ( UINT8 bByte, DlProtocolFrameFieldControlIFrameBitFields *ptControlIFramePartsResult );
static BOOL     FieldControlIFrameMergeParts    ( DlProtocolFrameFieldControlIFrameBitFields *ptControlIFrameParts, UINT8 *pbByteResult );
static BOOL     FieldControlSFrameSeparateParts ( UINT8 bByte, DlProtocolFrameFieldControlSFrameBitFields *ptControlSFramePartsResult );
static BOOL     FieldControlSFrameMergeParts    ( DlProtocolFrameFieldControlSFrameBitFields *ptControlSFrameParts, UINT8 *pbByteResult );
static BOOL     FieldControlUFrameSeparateParts ( UINT8 bByte, DlProtocolFrameFieldControlUFrameBitFields *ptControlUFramePartsResult );
static BOOL     FieldControlUFrameMergeParts    ( DlProtocolFrameFieldControlUFrameBitFields *ptControlUFrameParts, UINT8 *pbByteResult );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolInitModule( void )
{
#ifdef PROTOCOL_USE_PROTECTED_QUEUES

    memset( &tQueueTxSuFrame, 0 , sizeof(tQueueTxSuFrame) );
    memset( &tQueueRxIFrameDataField, 0 , sizeof(tQueueRxIFrameDataField) );

    tQueueTxSuFrame         = xQueueCreate( DL_PROTOC_QUEUE_RX_NUM_OF_S_U_FRAME_ELEMENTS, sizeof( DlProtocolFrame ) );
    
    if( tQueueTxSuFrame == NULL )
    {
        // catch this problem during testing time
        while(1){};
    }

    tQueueRxIFrameDataField = xQueueCreate( DL_PROTOC_BUFFER_RX_RAW_DATA_BYTE_SIZE, sizeof( UINT8 ) );

    if( tQueueRxIFrameDataField == NULL )
    {
        // catch this problem during testing time
        while(1){};
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolDbgEnable( BOOL fEnable )
{
    gfDbgEnable = fEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolIsDbgEnable( void )
{
    return gfDbgEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolDbgPortConfig( UsartPortsEnum eUsartPort )
{
    /*
        NOTE: invalid usart ports are allowed so that in order to not let 
        protocol keep parsing usart bytes a USART MAX(or invalid usart) can be set
    */
    if( eUsartPort > USART_PORT_TOTAL )
    {
        eUsartPort = USART_PORT_TOTAL;
    }

    geUsartPortDbg = eUsartPort;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UsartPortsEnum DlProtocolGetDbgPort( void )
{
    return geUsartPortDbg;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolPortConfig( UsartPortsEnum eUsartPort )
{
    /*
        NOTE: invalid usart ports are allowed so that in order to not let 
        protocol keep parsing usart bytes a USART MAX(or invalid usart) can be set
    */
    if( eUsartPort > USART_PORT_TOTAL )
    {
        eUsartPort = USART_PORT_TOTAL;
    }

    geUsartPort = eUsartPort;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UsartPortsEnum DlProtocolGetPortConfig( void )
{
    return geUsartPort;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolUsePort( BOOL fUsePort )
{
    gfProtocolIsUsingPort = fUsePort;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolIsUsingPort( void )
{
    return gfProtocolIsUsingPort;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolMasterListenForConnection( BOOL fListen )
{
    BOOL fSuccess = FALSE;
                
    if
    (
        (gfSlaveIsConnecting == FALSE) &&
        (gfSlaveIsConnected == FALSE)
    )
    {
        gbConnectionAddress = DL_PROTOCOL_CONN_ADDR_MASTER;
                
        if( fListen )
        {
            if( gfMasterIsListeningForConnections == FALSE )
            {
                if( gfMasterIsConnected )
                {
                    // already connected
                    fSuccess = TRUE;
                }
                else
                {
                    // clear QUEUES
                    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                    xQueueReset( tQueueTxSuFrame );
                    xQueueReset( tQueueRxIFrameDataField );
                    #else
                    QueueXReset( QUEUE_INDEX_S_U_FRAME );
                    QueueXReset( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
                    #endif
                    // restart buffer and other variables
                    gfISUFrameRxEscapeByteFound = FALSE;
                    gwISUFrameRxCrc16 = 0;
                    gwISUFrameRxBufferWalker = 0;
                    // reset seq num
                    gbIFrameRxSequenceNum = 0;
                    gbIFrameTxSequenceNum = 0;
                    // reset error counters
                    gbContinuousRxErrorPacketCounter = 0;
                    gbContinuousTxErrorPacketCounter = 0;

                    // start listening
                    gfMasterIsListeningForConnections = TRUE;

                    fSuccess = TRUE;
                }
            }
            else
            {
                // already listening
                fSuccess = TRUE;
            }
        }
        else // stop connection
        {
            // if connected
            if( gfMasterIsConnected )
            {
                // disconnect best effort. send 1 packet only
                DlProtocolFrame                             tUFrameTx;
                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                
                // stamp Control field->"Frame bits" to make it U-Frame
                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                tFieldAddressTx.bAddress = gbConnectionAddress;
                tFieldAddressTx.fIs8Bit  = TRUE;
                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_VALUE;
                
                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                
                tUFrameTx.tData.pbArray = NULL;
                tUFrameTx.tData.wArrMax = 0;
                tUFrameTx.tData.wDataLen = 0;
                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                
                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                #else
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                #endif

                // wait for disconnect or if not response then assume disconnected
                TIMER tDisconnectSendMsgDownTimer;
                
                tDisconnectSendMsgDownTimer = TimerDownTimerStartMs(500);
                
                // wait for DISCONNECT message to be sent
                while( gfMasterIsConnected )
                {
                    if( TimerDownTimerIsExpired(tDisconnectSendMsgDownTimer) )
                    {
                        gfMasterIsConnected = FALSE;
                    }
                }; 
            }

            // call reset other variables
            if( gfMasterIsListeningForConnections )
            {
                gfMasterIsListeningForConnections = FALSE;
            }
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolMasterIsListening( void )
{
    return gfMasterIsListeningForConnections;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolMasterIsClientConnected( void )
{
    return gfMasterIsConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolSlaveConnect( UINT16 wTimeOutMSec )
{
    // slave continuously send connection requests until time out
    BOOL fSuccess = FALSE;
            
    if
    (
        (gfMasterIsListeningForConnections == FALSE) &&
        (gfMasterIsConnected == FALSE)
    )
    {
        gbConnectionAddress = DL_PROTOCOL_CONN_ADDR_SLAVE;
        
        if( gfSlaveIsConnecting )
        {
            // already connection procedure started
            fSuccess = TRUE;
        }
        else
        {
            if( gfSlaveIsConnected )
            {
                // already connected
            }
            else // not connected and not connecting running
            {
                // clear QUEUES
                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                xQueueReset( tQueueTxSuFrame );
                xQueueReset( tQueueRxIFrameDataField );
                #else
                QueueXReset( QUEUE_INDEX_S_U_FRAME );
                QueueXReset( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
                #endif
                // restart buffer and other variables
                gfISUFrameRxEscapeByteFound = FALSE;
                gwISUFrameRxCrc16 = 0;
                gwISUFrameRxBufferWalker = 0;
                // reset seq num
                gbIFrameRxSequenceNum = 0;
                gbIFrameTxSequenceNum = 0;
                // reset error counters
                gbContinuousRxErrorPacketCounter = 0;
                gbContinuousTxErrorPacketCounter = 0;


                DlProtocolFrame                             tUFrameTx;
                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                
                // stamp Control field->"Frame bits" to make it U-Frame
                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                tFieldAddressTx.bAddress = gbConnectionAddress;
                tFieldAddressTx.fIs8Bit  = TRUE;
                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_VALUE;
                
                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                
                tUFrameTx.tData.pbArray = NULL;
                tUFrameTx.tData.wArrMax = 0;
                tUFrameTx.tData.wDataLen = 0;
                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                
                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                #else
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                #endif
            
                // start connection procedure
                gfSlaveIsConnecting = TRUE;
                gtSlaveConnectTimeOutTimer = TimerDownTimerStartMs(wTimeOutMSec);
                gtSlaveSendConnectRequestTimer = TimerDownTimerStartMs(100);
            }
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolSlaveDisconnect( void )
{
    BOOL fSuccess = FALSE;
                
    if
    (
        (gfMasterIsListeningForConnections == FALSE) &&
        (gfMasterIsConnected == FALSE)
    )
    {
        if( gfSlaveIsConnecting )
        {
            // stop connection attempts
            gfSlaveIsConnecting = FALSE;
            gfSlaveIsConnected = FALSE;
            
            fSuccess = TRUE;
        }
        else // if no connection attempt running
        {
            if( gfSlaveIsConnected )
            {
                // send disconnect request 3 times
                // disconnect best effort.
                DlProtocolFrame                             tUFrameTx;
                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                
                // stamp Control field->"Frame bits" to make it U-Frame
                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                tFieldAddressTx.bAddress = gbConnectionAddress;
                tFieldAddressTx.fIs8Bit  = TRUE;
                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_VALUE;
                
                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                
                tUFrameTx.tData.pbArray = NULL;
                tUFrameTx.tData.wArrMax = 0;
                tUFrameTx.tData.wDataLen = 0;
                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                
                // add frame to "send s-u-frames" queue
                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                #else
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                #endif
                
                TIMER tDisconnectSendMsgDownTimer;

                tDisconnectSendMsgDownTimer = TimerDownTimerStartMs(500);

                // wait for DISCONNECT message to be sent
                while( gfSlaveIsConnected )
                {
                    if( TimerDownTimerIsExpired(tDisconnectSendMsgDownTimer) )
                    {
                        gfSlaveIsConnected = FALSE;
                    }
                };
                
                fSuccess = TRUE;
            }
            else
            {
                // already disconnected
                fSuccess = TRUE;
            }
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolSlaveIsConnecting( void )
{
    return gfSlaveIsConnecting;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolSlaveIsConnected( void )
{
    return gfSlaveIsConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolIsPutBufferBusy( void )
{
    return gfIsPutBufferBusy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 DlProtocolGetBufferSubPacketCount( UINT8 *pbBuffer, UINT16 wBufferSize )
{
    UINT16 wSubPacketCount  = 0;
    UINT16 wRawBytesToSendCounter = 0;
    
    if( ( pbBuffer != NULL ) && ( wBufferSize > 0 ) )
    {
        for( UINT16 wIdx = 0 ; wIdx < wBufferSize ; wIdx++ )
        {            
            wRawBytesToSendCounter++;

            if
            (
                ( pbBuffer[wIdx] == PROTOCOL_FLAG_BYTE_ESCAPE ) ||
                ( pbBuffer[wIdx] == PROTOCOL_FLAG_BYTE_SOT ) 
            )
            {
                wRawBytesToSendCounter++;
            }

            //if( wRawBytesToSendCounter < DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
            if( wRawBytesToSendCounter < gtIFrameTx.tData.wArrMax )
            {
                // check future byte to see if it fits in frame
                if( (wIdx+1) < wBufferSize )
                {
                    if
                    (
                        ( pbBuffer[wIdx+1] == PROTOCOL_FLAG_BYTE_ESCAPE ) ||
                        ( pbBuffer[wIdx+1] == PROTOCOL_FLAG_BYTE_SOT ) 
                    )
                    {
                        // escape byte found. does it fit?
                        //if( (wRawBytesToSendCounter+2) > DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
                        if( (wRawBytesToSendCounter+2) < gtIFrameTx.tData.wArrMax )
                        {
                            // if doesnt fit dont include it in current packet. count it on the next one
                            // count subpacket and reset byte counter
                            wSubPacketCount++;
                            wRawBytesToSendCounter = 0;
                            continue;
                        }
                    }
                }
                else
                {
                    // if no more bytes to process, close frame and send it
                    // (count one more frame)
                    wSubPacketCount++;
                    // terminate loop since it was the last byte to process
                    break;
                }
            }
            //else if( wRawBytesToSendCounter == DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
            else if( wRawBytesToSendCounter == gtIFrameTx.tData.wArrMax )
            {
                // count subpacket and reset byte counter
                wRawBytesToSendCounter = 0;
                wSubPacketCount++;
            }
        }
    }

    return wSubPacketCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SendEscapedValidSizeRawBuffer( UINT8 *pbBuffer, UINT16 wBufferSize )
{
    BOOL    fSuccess = FALSE;
    TIMER   tTimeOutTimer;
    
    if( ( pbBuffer != NULL ) && ( wBufferSize > 0 ) )
    //&& ( wBufferSize <= DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE ) )
    {
        //if( wBufferSize <= sizeof(gbIFrameTxDataBuffer) )
        if( ( gtIFrameTx.tData.pbArray != NULL ) && ( wBufferSize <= gtIFrameTx.tData.wArrMax ) )
        {
            if( gfIFrameTxIsPending == FALSE )
            {
                gfIFrameTxIsSent = FALSE;

                // Prepare I Frame for the next chunk of data
                DlProtocolFrameFieldAddressBitFields        tAddress;
                DlProtocolFrameFieldControlIFrameBitFields  tControlIFrame;

                // SET ADDRESS FIELD
                tAddress.bAddress = gbConnectionAddress;
                tAddress.fIs8Bit  = TRUE;
                FieldAddressMergeParts( &tAddress, &gtIFrameTx.bAddress );
                // SET CONTROL FIELD
                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE, &gtIFrameTx.bControl );
                tControlIFrame.bSeqNumTransmit = gbIFrameTxSequenceNum;
                tControlIFrame.bSeqNumReceive = 0; // not used
                FieldControlIFrameMergeParts( &tControlIFrame, &gtIFrameTx.bControl );
                // SET DATA FIELD
                //#warning this section has been modified with no testing but it seems to be breaking protocol
                //memcpy( &gbIFrameTxDataBuffer[0], &pbBuffer[0], wBufferSize );
                //if( ( gtIFrameTx.tData.pbArray != NULL ) && ( wBufferSize <= gtIFrameTx.tData.wArrMax ) )
                {
                    // update buffer
                    memcpy( &gtIFrameTx.tData.pbArray[0], &pbBuffer[0], wBufferSize );
                    gtIFrameTx.tData.wDataLen= wBufferSize;
                }
//                else
//                {
//                }
//                gtIFrameTx.tData.pbArray = &gbIFrameTxDataBuffer[0];
//                gtIFrameTx.tData.wDataLen= wBufferSize;
//                gtIFrameTx.tData.wArrMax = sizeof(gbIFrameTxDataBuffer);

                // SET CRC FIELD
                gtIFrameTx.wCrc16 = FrameGetCrc16( &gtIFrameTx );
    
                // to start transmission set Flag gfIFrameTxIsPending to TRUE
                gfIFrameTxIsPending = TRUE;

                // loop until packet is send or time expires or send operation fails (check for disconnect)
                // when send operation fails at max retries, the driver gets disconnected automatically
            
                // waiting time per iframe 2 second max
                // NOTE: ideally it should stop after MAX retries, but in case of but there is this time out.
                tTimeOutTimer = TimerDownTimerStartMs( 3000 );
            
                do
                {
                    // check if no pending packet
                    if( gfIFrameTxIsPending == FALSE )
                    {
                        if( gfSlaveIsConnected | gfMasterIsConnected )
                        {
                            // PASS
                            fSuccess = TRUE;
                            break;
                        }
                        else
                        {
                            // ERROR
                            break;
                        }
                        break;
                    }
                    else if( TimerDownTimerIsExpired( tTimeOutTimer ) )
                    {
                        // cancel last packet transmission
                        if( gfSlaveIsConnected )
                        {
                            gfSlaveIsConnected = FALSE;
                        }
                        else if( gfMasterIsConnected )
                        {
                            gfMasterIsConnected = FALSE;
                            gfMasterIsListeningForConnections = TRUE;
                        }

                        gfIFrameTxIsPending = FALSE;
                        gfIFrameTxIsSent    = FALSE;
                        break;
                    }
                }while(TRUE);
            }
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolPutBuffer( UINT8 *pbBuffer, UINT16 wBufferSize, UINT16 wTimeOutMSec )
{
    BOOL    fSuccess                = FALSE;
    UINT16  wRawBytesToSendCounter;
    TIMER   tTimeOutTimer;
    UINT16  wStartIdx;
    
    if( ( pbBuffer != NULL ) && ( wBufferSize > 0 ) && ( gfSlaveIsConnected | gfMasterIsConnected ) )
    {
        if( gfIsPutBufferBusy == FALSE )
        {
            gfIsPutBufferBusy = TRUE;
            
            // start timer
            tTimeOutTimer = TimerDownTimerStartMs( wTimeOutMSec );
            
            wRawBytesToSendCounter = 0;
            wStartIdx = 0;
            
            for( UINT16 wIdx = 0 ; wIdx < wBufferSize ; wIdx++ )
            {            
                wRawBytesToSendCounter++;

                if
                (
                    ( pbBuffer[wIdx] == PROTOCOL_FLAG_BYTE_ESCAPE ) ||
                    ( pbBuffer[wIdx] == PROTOCOL_FLAG_BYTE_SOT ) 
                )
                {
                    wRawBytesToSendCounter++;
                }

                //if( wRawBytesToSendCounter < DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
                if( wRawBytesToSendCounter < gtIFrameTx.tData.wArrMax )
                {
                    // check future byte to see if it fits in frame
                    if( (wIdx+1) < wBufferSize )
                    {
                        if
                        (
                            ( pbBuffer[wIdx+1] == PROTOCOL_FLAG_BYTE_ESCAPE ) ||
                            ( pbBuffer[wIdx+1] == PROTOCOL_FLAG_BYTE_SOT ) 
                        )
                        {
                            // escape byte found. does it fit?
                            //if( (wRawBytesToSendCounter+2) > DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
                            if( (wRawBytesToSendCounter+2) > gtIFrameTx.tData.wArrMax )
                            {
                                // if doesnt fit dont include it in current packet. count it on the next one
                                ////////////////////////////////////////////////////////////////////////
                                ////////////////////////////////////////////////////////////////////////
                                // send subpacket and reset byte counter
                                if( SendEscapedValidSizeRawBuffer( &pbBuffer[wStartIdx], ( (wIdx+1) - (wStartIdx+1) + 1 ) ) )
                                {
                                    // update start index
                                    wStartIdx = wIdx+1;
                                    // PASS
                                    // still more bytes to process
                                    wRawBytesToSendCounter = 0;
                                    continue;
                                }
                                else
                                {
                                    // FAIL TO SEND PACKET
                                    // change status to disconnected
                                    if( gfSlaveIsConnected )
                                    {
                                        gfSlaveIsConnected = FALSE;
                                    }
                                    else if( gfMasterIsConnected )
                                    {
                                        gfMasterIsConnected = FALSE;
                                        gfMasterIsListeningForConnections = TRUE;
                                    }
                                    // cancel last packet transmission
                                    gfIFrameTxIsPending = FALSE;
                                    gfIFrameTxIsSent    = FALSE;
                                    break;
                                }
                                ////////////////////////////////////////////////////////////////////////
                                ////////////////////////////////////////////////////////////////////////
                            }
                        }
                    }
                    else
                    {
                        // if no more bytes to process, close frame and send it
                        // wSubPacketCount++;
                        // terminate loop since it was the last byte to process
                        if( SendEscapedValidSizeRawBuffer( &pbBuffer[wStartIdx], ( (wIdx+1) - (wStartIdx+1) + 1 ) ) )
                        {
                            // update start index
                            wStartIdx = wIdx+1;

                            // if PASS
                            fSuccess = TRUE;
                            break;
                        }
                        else
                        {
                            // FAIL TO SEND PACKET
                            // change status to disconnected
                            if( gfSlaveIsConnected )
                            {
                                gfSlaveIsConnected = FALSE;
                            }
                            else if( gfMasterIsConnected )
                            {
                                gfMasterIsConnected = FALSE;
                                gfMasterIsListeningForConnections = TRUE;
                            }
                            // cancel last packet transmission
                            gfIFrameTxIsPending = FALSE;
                            gfIFrameTxIsSent    = FALSE;
                            break;
                        }
                    }
                }
                //else if( wRawBytesToSendCounter == DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE )
                else if( wRawBytesToSendCounter == gtIFrameTx.tData.wArrMax )
                {
                    // count subpacket and reset byte counter
                    if( SendEscapedValidSizeRawBuffer( &pbBuffer[wStartIdx], ( (wIdx+1) - (wStartIdx+1) + 1 ) ) )
                    {
                        // update start index
                        wStartIdx = wIdx+1;

                        // if PASS
                        if( (wIdx+1) < wBufferSize )
                        {
                            wRawBytesToSendCounter = 0;
                            continue;
                        }
                        else
                        {
                            // no more bytes to send
                            fSuccess = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        // FAIL TO SEND PACKET
                        // change status to disconnected
                        if( gfSlaveIsConnected )
                        {
                            gfSlaveIsConnected = FALSE;
                        }
                        else if( gfMasterIsConnected )
                        {
                            gfMasterIsConnected = FALSE;
                            gfMasterIsListeningForConnections = TRUE;
                        }
                        // cancel last packet transmission
                        gfIFrameTxIsPending = FALSE;
                        gfIFrameTxIsSent    = FALSE;
                        break;
                    }
                }

                // check if time expires                
                if( TimerDownTimerIsExpired( tTimeOutTimer ) )
                {
                    if( gfSlaveIsConnected )
                    {
                        gfSlaveIsConnected = FALSE;
                    }
                    else if( gfMasterIsConnected )
                    {
                        gfMasterIsConnected = FALSE;
                        gfMasterIsListeningForConnections = TRUE;
                    }

                    gfIFrameTxIsPending = FALSE;
                    gfIFrameTxIsSent    = FALSE;
                    break;
                }
            }
            
            gfIsPutBufferBusy = FALSE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolIsDataAvailable( void )
{
    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
    return ( uxQueueMessagesWaiting( tQueueRxIFrameDataField ) != 0 );
    #else
    return !QueueXIsEmpty( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
    #endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DlProtocolReadData( UINT8 *pbBuffer, UINT16 wBufferSize, UINT16 *pdwBytesRead )
{
    BOOL fSuccess = FALSE;
    UINT16 wIdx;
    
    if( (pbBuffer != NULL) && (wBufferSize > 0) )
    {
        // bytes read is optional so that this function can be used to empty rx buffer
        wIdx = 0;
        
        do
        {
            #ifdef PROTOCOL_USE_PROTECTED_QUEUES
            if( xQueueReceive( tQueueRxIFrameDataField, &pbBuffer[wIdx], PROTOCOL_PROTECTED_QUEUE_TICK_WAIT ) )
            #else
            if( QueueXRemove( QUEUE_INDEX_DL_PROTOCOL_RX_DATA, &pbBuffer[wIdx], 1 ) )
            #endif
            {
                wIdx++;
                // if at least 1 char received then success
                fSuccess = TRUE;
            }
            else
            {
                break;
            }
        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
        }while( (wIdx < wBufferSize) && (uxQueueMessagesWaiting( tQueueRxIFrameDataField ) != 0) );
        #else
        }while( (wIdx < wBufferSize) && (QueueXIsEmpty(QUEUE_INDEX_DL_PROTOCOL_RX_DATA) == FALSE) );
        #endif
        
        if( pdwBytesRead != NULL )
        {
            (*pdwBytesRead) = wIdx;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SendFrame( DlProtocolFrame *ptFrame )
{
    BOOL fSuccess = FALSE;
    
    if( ptFrame != NULL )
    {
        {
            fSuccess = TRUE;
        
            fSuccess &= UsartPutChar( geUsartPort, PROTOCOL_FLAG_BYTE_SOT, 10 );
                
            /////////////////////////////////
            fSuccess &= SendFrameDataByte( ptFrame->bAddress );
            fSuccess &= SendFrameDataByte( ptFrame->bControl );
        
            if( (ptFrame->tData.wDataLen > 0) && (ptFrame->tData.pbArray != NULL) )
            {
                for( UINT16 x = 0 ; x < ptFrame->tData.wDataLen ; x++ )
                {
                    fSuccess &= SendFrameDataByte( ptFrame->tData.pbArray[x] );
                }
            }
        
            fSuccess &= SendFrameDataByte( (ptFrame->wCrc16 >> 8) );        // MSB
            fSuccess &= SendFrameDataByte( (ptFrame->wCrc16 & 0xFF) );      // LSB
            /////////////////////////////////
        
            fSuccess &= UsartPutChar( geUsartPort, PROTOCOL_FLAG_BYTE_SOT, 10 );
        
            PrintDbgFrame(ptFrame, FALSE);
        
            if( fSuccess == FALSE )
            {
                //System.out.println( "ERROR: O " + tFrame.toString() );
            }
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PrintDbgFrame( DlProtocolFrame *ptFrame, BOOL fIsIncomingFrame )
{
    if( gfDbgEnable )
    {
        if( ptFrame != NULL )
        {
            if( fIsIncomingFrame )
            {
                UsartPrintf( geUsartPortDbg, "I [ " );
            }
            else
            {
                UsartPrintf( geUsartPortDbg, "O [ " );
            }

            // address
            UsartPrintf( geUsartPortDbg, "ADD:0x%02X, ", ptFrame->bAddress );

            // control
            switch( FieldControlGetType( ptFrame->bControl ) )
            {
                case FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE:
                    UsartPrintf( geUsartPortDbg, "CNT-Tp:I, ");
                    DlProtocolFrameFieldControlIFrameBitFields tFieldControlIFrame;
                    FieldControlIFrameSeparateParts( ptFrame->bControl, &tFieldControlIFrame );
                    UsartPrintf( geUsartPortDbg, "CNT-Rsn:%02d, ", tFieldControlIFrame.bSeqNumReceive );
                    UsartPrintf( geUsartPortDbg, "CNT-Ssn:%02d, ", tFieldControlIFrame.bSeqNumTransmit );
                    break;
                case FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE:
                    UsartPrintf( geUsartPortDbg, "CNT-Tp:S, ");
                    DlProtocolFrameFieldControlSFrameBitFields  tFieldControlSFrame;
                    FieldControlSFrameSeparateParts( ptFrame->bControl, &tFieldControlSFrame );
                    UsartPrintf( geUsartPortDbg, "CNT-Rsn:%02d, ", tFieldControlSFrame.bSeqNumReceive );
                    UsartPrintf( geUsartPortDbg, "CNT-Typ:%02d, ", tFieldControlSFrame.bType );
                    break;
                case FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE:
                    UsartPrintf( geUsartPortDbg, "CNT-Tp:U, ");
                    DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrame;
                    FieldControlUFrameSeparateParts( ptFrame->bControl, &tFieldControlUFrame );
                    UsartPrintf( geUsartPortDbg, "CNT-Typ:%02d, ", tFieldControlUFrame.bType );
                    break;
                 default:
                    UsartPrintf( geUsartPortDbg, "CNT-Typ:ERR, " );
                    break;
            }

            // data
            if( (ptFrame->tData.wDataLen > 0) && (ptFrame->tData.pbArray != NULL) )
            {
                for( UINT16 x = 0 ; x < ptFrame->tData.wDataLen ; x++ )
                {
                    if( FieldControlGetType( ptFrame->bControl ) == FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE )
                    {
                        if( (ptFrame->tData.pbArray[x] >= 32) && (ptFrame->tData.pbArray[x] <= 126) )
                        {
                            UsartPrintf( geUsartPortDbg, "D%02d:'%c', ", x, ptFrame->tData.pbArray[x] );
                        }
                        else
                        {
                            UsartPrintf( geUsartPortDbg, "D%02d:0x%02X, ", x, ptFrame->tData.pbArray[x] );
                        }
                    }
                    else
                    {
                        UsartPrintf( geUsartPortDbg, "D%02d:0x%02X, ", x, ptFrame->tData.pbArray[x] );
                    }
                }
            }

            // crc16
            UsartPrintf( geUsartPortDbg, "CRM:0x%02X, ", (ptFrame->wCrc16 >> 8) );
            UsartPrintf( geUsartPortDbg, "CRM:0x%02X ]\r\n", (ptFrame->wCrc16 & 0xFF) );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SendFrameDataByte( UINT8 bByte )
{
    BOOL fSuccess = FALSE;
            
    switch( bByte )
    {
        case PROTOCOL_FLAG_BYTE_ESCAPE:
        case PROTOCOL_FLAG_BYTE_SOT:
            bByte = (bByte ^ PROTOCOL_FLAG_BYTE_ESCAPE);
            fSuccess = UsartPutChar( geUsartPort, PROTOCOL_FLAG_BYTE_ESCAPE, 10 );
            fSuccess &= UsartPutChar( geUsartPort, bByte, 10 );
            break;
        default:
            fSuccess = UsartPutChar( geUsartPort, bByte, 10 );
            break;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void DlProtocolUpdate( void )
{
    CHAR cChar;
    
    if( gfProtocolIsUsingPort )
    {
        while( UsartGetChar( geUsartPort, &cChar, 0 ) )
        {
            // process received bytes
            UpdateReceiverState( cChar );
        }
    
        // sends bytes from pending packet
        UpdateSenderState();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////                    
void UpdateSenderState( void )
{
    //######################################################
    // check if handshake procedure is running.
    // handshake procedure from slave should send every x period mSecs a connection request
    if( gfSlaveIsConnecting == TRUE )
    {
        if( TimerDownTimerIsExpired( gtSlaveSendConnectRequestTimer ) )
        {
            // send connection request packet
            DlProtocolFrame                             tUFrameTx;
            DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
            DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
            
            // stamp Control field->"Frame bits" to make it U-Frame
            FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

            tFieldAddressTx.bAddress = gbConnectionAddress;
            tFieldAddressTx.fIs8Bit  = TRUE;
            tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_VALUE;
            
            FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
            FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
            
            tUFrameTx.tData.pbArray = NULL;
            tUFrameTx.tData.wArrMax = 0;
            tUFrameTx.tData.wDataLen = 0;
            tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
            
            #ifdef PROTOCOL_USE_PROTECTED_QUEUES
            xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
            #else
            QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
            #endif
            
            // restart timer
            gtSlaveSendConnectRequestTimer = TimerDownTimerStartMs(100);
        }
        
        if( TimerDownTimerIsExpired( gtSlaveConnectTimeOutTimer ) )
        {
            // send connection request packet
            // send connection closed( in case the last second Master got
            //      connection request from slave and openned the connection
            DlProtocolFrame                             tUFrameTx;
            DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
            DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
            
            // stamp Control field->"Frame bits" to make it U-Frame
            FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

            tFieldAddressTx.bAddress = gbConnectionAddress;
            tFieldAddressTx.fIs8Bit  = TRUE;
            tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_VALUE;
            
            FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
            FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
            
            tUFrameTx.tData.pbArray = NULL;
            tUFrameTx.tData.wArrMax = 0;
            tUFrameTx.tData.wDataLen = 0;
            tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
            
            #ifdef PROTOCOL_USE_PROTECTED_QUEUES
            xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
            xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
            xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
            #else
            QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
            QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
            QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
            #endif
            
            gfSlaveIsConnecting = FALSE;
            gfSlaveIsConnected = FALSE;
        }
    }
    //######################################################
    

    // I frame pending to be send...
    // S and U frames take priority over I frame
    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
    if( uxQueueMessagesWaiting( tQueueTxSuFrame ) != 0 )
    {
        // S-U Frames best effort...no retry
        if( xQueueReceive( tQueueTxSuFrame, &gtSUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT ) == pdTRUE )
        {
            SendFrame( &gtSUFrameTx );
        }
    }
    #else
    if( QueueXIsEmpty( QUEUE_INDEX_S_U_FRAME ) == FALSE )
    {
        // S-U Frames best effort...no retry
        QueueXRemove( QUEUE_INDEX_S_U_FRAME, &gtSUFrameTx, sizeof(gtSUFrameTx) );
        SendFrame( &gtSUFrameTx );
    }
    #endif
    else // I-Frame
    {
        if( gfIFrameTxIsPending )
        {
            if( gfIFrameTxIsSent == FALSE )
            {
                // SEND
                SendFrame( &gtIFrameTx );
                gtIFrameTxRetryTimeOutTimer = TimerDownTimerStartMs( gdwIFrameTxRetryTimeOutMSec );
                gfIFrameTxIsSent = TRUE;
            }
            else
            {
                // wait until receive response or time out
                if( TimerDownTimerIsExpired( gtIFrameTxRetryTimeOutTimer ) )
                {
                    gbContinuousTxErrorPacketCounter++;

                    if( gfSequenceNumberResetRequested )
                    {
                        gfSequenceNumberResetRequested = FALSE;
                        
                        // crc16 re-calc for current I-Frame with the new sequence number
                        DlProtocolFrameFieldControlIFrameBitFields tIFrameControlField;
                        FieldControlIFrameSeparateParts( gtIFrameTx.bControl, &tIFrameControlField );
                        tIFrameControlField.bSeqNumTransmit = gbIFrameTxSequenceNum;
                        FieldControlIFrameMergeParts( &tIFrameControlField, &gtIFrameTx.bControl );
                        gtIFrameTx.wCrc16 = FrameGetCrc16( &gtIFrameTx );
                    }
                    
                    // Re-SEND
                    SendFrame( &gtIFrameTx );
                    gtIFrameTxRetryTimeOutTimer = TimerDownTimerStartMs( gdwIFrameTxRetryTimeOutMSec );
                    gfIFrameTxIsSent = TRUE;
                }
            }

            // if 10 errors in a row, throw away the connection
            if( gbContinuousTxErrorPacketCounter >= PROTOCOL_TX_ERROR_MAX )
            {
                gbContinuousTxErrorPacketCounter = 0;

                gfIFrameTxIsPending = FALSE;
                gfIFrameTxIsSent    = FALSE;

                if( gfMasterIsConnected )
                {
                    gfMasterIsConnected = FALSE;
                    gfMasterIsListeningForConnections = TRUE;
                }
                else if( gfSlaveIsConnected )
                {
                    gfSlaveIsConnected = FALSE;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateReceiverState( UINT8 bByte )
{
    if( bByte == PROTOCOL_FLAG_BYTE_SOT )
    {
        if( gfDbgEnable )
        {
            UsartPrintf( geUsartPortDbg, "SOT found\r\n" );
        }
        
        // check current buffer and see if there is a valid frame.
        if( gwISUFrameRxBufferWalker > 0 )
        {
            if( ParseFrame( &gbISUFrameRxBuffer[0], gwISUFrameRxBufferWalker, &gtISUFrameRx ) )
            {
                // System.out.println( "I " + tFrame.toString() );
                PrintDbgFrame( &gtISUFrameRx, TRUE );
                
                if( gtISUFrameRx.wCrc16 == gwISUFrameRxCrc16 )
                {

                    // check if is data frame
                    switch( FieldControlGetType( gtISUFrameRx.bControl ) )
                    {

                        case FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE:
                        {
                            if( gfSlaveIsConnected | gfMasterIsConnected )
                            {
                                // check if sequence number is the expected one
                                DlProtocolFrameFieldControlIFrameBitFields tControlIFramePartsResult;
                                FieldControlIFrameSeparateParts( gtISUFrameRx.bControl, &tControlIFramePartsResult );
                                
                                if( tControlIFramePartsResult.bSeqNumTransmit == gbIFrameRxSequenceNum )
                                {
                                    // add RAW DATA bytes to receive(Rx) queue
                                    // NOTE: if data from this frame fits into queue then 
                                    // add it to queue and send back ACK otherwise 
                                    // send NACK and dont add data from this frame to queue
                                    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                    UINT32 dwQueueMax = DL_PROTOC_BUFFER_RX_RAW_DATA_BYTE_SIZE;
                                    UINT32 dwItemCount = uxQueueMessagesWaiting( tQueueRxIFrameDataField );
                                    UINT32 dwSpaceAvail = dwQueueMax - dwItemCount;
                                    #else
                                    UINT32 dwQueueMax = QueueXGetMaxItems( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
                                    UINT32 dwItemCount = QueueXGetItemCount( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
                                    UINT32 dwSpaceAvail = dwQueueMax - dwItemCount;
                                    #endif

                                    if( gtISUFrameRx.tData.wDataLen <= dwSpaceAvail )
                                    {
                                        // reset error
                                        gbContinuousRxErrorPacketCounter = 0;

                                        for( UINT16 wIdx = 0; wIdx < gtISUFrameRx.tData.wDataLen; wIdx++ )
                                        {
                                            #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                            xQueueSend( tQueueRxIFrameDataField, &gtISUFrameRx.tData.pbArray[wIdx], PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                            #else
                                            QueueXAdd( QUEUE_INDEX_DL_PROTOCOL_RX_DATA, &gtISUFrameRx.tData.pbArray[wIdx], 1 );
                                            #endif
                                        }
                                    
                                        // send ACK back
                                        DlProtocolFrame                             tSFrameTx;
                                        DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                        DlProtocolFrameFieldControlSFrameBitFields  tFieldControlSFrameTx;
                                    
                                        // stamp Control field->"Frame bits" to make it S-Frame
                                        FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE, &tSFrameTx.bControl );

                                        tFieldAddressTx.bAddress = gbConnectionAddress;
                                        tFieldAddressTx.fIs8Bit  = TRUE;
                                        tFieldControlSFrameTx.bSeqNumReceive = gbIFrameRxSequenceNum;
                                        tFieldControlSFrameTx.bType = FRAME_FIELD_CNTRL_S_TYPE_ACK_VALUE;
                                    
                                        FieldAddressMergeParts( &tFieldAddressTx, &tSFrameTx.bAddress );
                                        FieldControlSFrameMergeParts( &tFieldControlSFrameTx, &tSFrameTx.bControl );
                                    
                                        tSFrameTx.tData.pbArray = NULL;
                                        tSFrameTx.tData.wArrMax = 0;
                                        tSFrameTx.tData.wDataLen = 0;
                                        tSFrameTx.wCrc16 = FrameGetCrc16( &tSFrameTx );
                                    
                                        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                        xQueueSend( tQueueTxSuFrame, &tSFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                        #else
                                        QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tSFrameTx, sizeof(tSFrameTx) );
                                        #endif
                                    
                                        // increment sequence number
                                        gbIFrameRxSequenceNum++;
                                        gbIFrameRxSequenceNum = gbIFrameRxSequenceNum % 8;
                                    }
                                    else
                                    {
                                        // data doesn't fit in queue... return error until data can fit
                                        // send NACK back
                                        DlProtocolFrame                             tSFrameTx;
                                        DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                        DlProtocolFrameFieldControlSFrameBitFields  tFieldControlSFrameTx;

                                        // stamp Control field->"Frame bits" to make it S-Frame
                                        FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE, &tSFrameTx.bControl );

                                        tFieldAddressTx.bAddress = gbConnectionAddress;
                                        tFieldAddressTx.fIs8Bit  = TRUE;
                                        tFieldControlSFrameTx.bSeqNumReceive = gbIFrameRxSequenceNum;
                                        tFieldControlSFrameTx.bType = FRAME_FIELD_CNTRL_S_TYPE_NACK_VALUE;
                                    
                                        FieldAddressMergeParts( &tFieldAddressTx, &tSFrameTx.bAddress );
                                        FieldControlSFrameMergeParts( &tFieldControlSFrameTx, &tSFrameTx.bControl );
                                    
                                        tSFrameTx.tData.pbArray = NULL;
                                        tSFrameTx.tData.wArrMax = 0;
                                        tSFrameTx.tData.wDataLen = 0;
                                        tSFrameTx.wCrc16 = FrameGetCrc16( &tSFrameTx );
                                    
                                        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                        xQueueSend( tQueueTxSuFrame, &tSFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                        #else
                                        QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tSFrameTx, sizeof(tSFrameTx) );
                                        #endif
                                    }
                                }
                                else
                                {
                                    UINT8 bIFrameRxSequenceNumPrevious = 0;
                                    if( gbIFrameRxSequenceNum == 0 )
                                    {
                                        bIFrameRxSequenceNumPrevious = 7;
                                    }
                                    else
                                    {
                                        bIFrameRxSequenceNumPrevious = gbIFrameRxSequenceNum-1;
                                    }
                                 
                                    // condition for when ACK response got sent to remote but 
                                    // lost or corrupted while 
                                    // locally seq num incremented
                                    // remote didnt increment
                                    if( tControlIFramePartsResult.bSeqNumTransmit == bIFrameRxSequenceNumPrevious )
                                    {
                                        // PREVIOUS PACKET RE-ACKed
                                        
                                        // send ACK back
                                        DlProtocolFrame                             tSFrameTx;
                                        DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                        DlProtocolFrameFieldControlSFrameBitFields  tFieldControlSFrameTx;
                                        
                                        // stamp Control field->"Frame bits" to make it U-Frame
                                        FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE, &tSFrameTx.bControl );

                                        tFieldAddressTx.bAddress = gbConnectionAddress;
                                        tFieldAddressTx.fIs8Bit  = TRUE;
                                        tFieldControlSFrameTx.bSeqNumReceive = bIFrameRxSequenceNumPrevious;
                                        tFieldControlSFrameTx.bType = FRAME_FIELD_CNTRL_S_TYPE_ACK_VALUE;
                                        
                                        FieldAddressMergeParts( &tFieldAddressTx, &tSFrameTx.bAddress );
                                        FieldControlSFrameMergeParts( &tFieldControlSFrameTx, &tSFrameTx.bControl );
                                        
                                        tSFrameTx.tData.pbArray = NULL;
                                        tSFrameTx.tData.wArrMax = 0;
                                        tSFrameTx.tData.wDataLen = 0;
                                        tSFrameTx.wCrc16 = FrameGetCrc16( &tSFrameTx );
                                        
                                        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                        xQueueSend( tQueueTxSuFrame, &tSFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                        #else
                                        QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tSFrameTx, sizeof(tSFrameTx) );
                                        #endif
                                    }
                                    else
                                    {
                                        gbContinuousRxErrorPacketCounter++;

                                        // SEQ NUM OUT OF SYNC
                                        
                                        // if seq num out of sync 
                                        // send U-frame reset
                                        // wait for remote u-frame request for RESET (at that point remote already reset 
                                        //      seq num even no uframe ACK has been sent back from local)
                                        // after sending ack...expect from remote start sending data with seq num = 0
                                        // what if ack gets lost?
                                        // communications wont continue until ACK received.
                                        //  if ACK gets lost remote will time out and reset request for RESET
                                        //  local will reset again and send a new ACK
                                        // adjust the sequence number in the frame   
                                        
                                        gbIFrameRxSequenceNum = 0;
                                        gbIFrameTxSequenceNum = 0;
                                        
                                        DlProtocolFrame                             tUFrameTx;
                                        DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                        DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                        
                                        // stamp Control field->"Frame bits" to make it U-Frame
                                        FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                        tFieldAddressTx.bAddress = gbConnectionAddress;
                                        tFieldAddressTx.fIs8Bit  = TRUE;
                                        tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_SEQ_NUM_RESET_VALUE;
                                        
                                        FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                        FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                        
                                        tUFrameTx.tData.pbArray = NULL;
                                        tUFrameTx.tData.wArrMax = 0;
                                        tUFrameTx.tData.wDataLen = 0;
                                        tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                        
                                        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                        xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                        #else
                                        QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                        #endif
                                    }
                                }
                            }
                            break;
                        }// END case FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE:
                            
                        case FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE:
                        {
                            if( gfSlaveIsConnected | gfMasterIsConnected )
                            {
                                // get a frame that indicates if retransmission is required
                                // check if is an ACK or ERROR
                                DlProtocolFrameFieldControlSFrameBitFields  tFieldControlSFrameRx;
                                
                                FieldControlSFrameSeparateParts( gtISUFrameRx.bControl, &tFieldControlSFrameRx );
                                
                                if( tFieldControlSFrameRx.bSeqNumReceive == gbIFrameTxSequenceNum )
                                {
                                    if( tFieldControlSFrameRx.bType == FRAME_FIELD_CNTRL_S_TYPE_ACK_VALUE )
                                    {
                                        // data received correctly in the other end. 
                                        // clear flags to allow sending more data
                                        // gbContinuousRxErrorPacketCounter = 0;
                                        gbContinuousTxErrorPacketCounter = 0;
                                        
                                        gbIFrameTxSequenceNum++;
                                        gbIFrameTxSequenceNum = gbIFrameTxSequenceNum % 8;
                                        
                                        gfIFrameTxIsPending = FALSE;
                                        // gfIFrameTxIsSent    = TRUE;
                                    }
                                    else
                                    {
                                        //gbContinuousRxErrorPacketCounter++;
                                        gbContinuousTxErrorPacketCounter++;
                                        
                                        // ERROR: NACK or not supported response returned.
                                        // force a re-send
                                        gfIFrameTxIsSent = FALSE;
                                    }
                                }
                                else
                                {
                                    // ERROR: sequence number out of sync. Do nothing.
                                    //gbContinuousRxErrorPacketCounter++;
                                    gbContinuousTxErrorPacketCounter++;
                                }
                            }
                            break;
                        } // END case FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE:
                        
                        
                        case FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE:
                        {
                            DlProtocolFrameFieldAddressBitFields        tAddressRx;
                            DlProtocolFrameFieldControlUFrameBitFields  tControlUFrameRx;
                            
                            FieldAddressSeparateParts( gtISUFrameRx.bAddress, &tAddressRx );
                            FieldControlUFrameSeparateParts( gtISUFrameRx.bControl, &tControlUFrameRx );
                        
                            switch( tControlUFrameRx.bType )
                            {
                                case FRAME_FIELD_CNTRL_U_TYPE_CONNECT_VALUE: // only slaves receive this type of packet
                                    if( gbConnectionAddress == DL_PROTOCOL_CONN_ADDR_MASTER )
                                    {
                                        if( tAddressRx.bAddress == DL_PROTOCOL_CONN_ADDR_SLAVE )
                                        {
                                            if( gfMasterIsListeningForConnections )
                                            {
                                                // clear QUEUES
                                                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                                xQueueReset( tQueueTxSuFrame );
                                                xQueueReset( tQueueRxIFrameDataField );
                                                #else
                                                QueueXReset( QUEUE_INDEX_S_U_FRAME );
                                                QueueXReset( QUEUE_INDEX_DL_PROTOCOL_RX_DATA );
                                                #endif

                                                // return ACK and reset sequence number or packet variables
                                                DlProtocolFrame                             tUFrameTx;
                                                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                                
                                                // stamp Control field->"Frame bits" to make it U-Frame
                                                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                                tFieldAddressTx.bAddress = gbConnectionAddress;
                                                tFieldAddressTx.fIs8Bit  = TRUE;
                                                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_ACK_VALUE;
                                                
                                                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                                
                                                tUFrameTx.tData.pbArray = NULL;
                                                tUFrameTx.tData.wArrMax = 0;
                                                tUFrameTx.tData.wDataLen = 0;
                                                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                                
                                                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                                #else
                                                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                                #endif
                                                
                                                // connection established!!
                                                gfMasterIsListeningForConnections = FALSE;
                                                gfMasterIsConnected = TRUE;
                                                
                                                gbContinuousRxErrorPacketCounter = 0;
                                                gbContinuousTxErrorPacketCounter = 0;
                                                
                                                // reset packet frame variables
                                                gbIFrameRxSequenceNum = 0;
                                                gbIFrameTxSequenceNum = 0;
                                                
                                                // reset buffer vars
                                                gfISUFrameRxEscapeByteFound = FALSE;
                                                gwISUFrameRxCrc16 = 0;
                                                gwISUFrameRxBufferWalker = 0;
                                            }
                                            else if( gfMasterIsConnected )
                                            {
                                                // return ACK but don't reset sequence number or packet variables
                                                DlProtocolFrame                             tUFrameTx;
                                                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                                
                                                // stamp Control field->"Frame bits" to make it U-Frame
                                                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                                tFieldAddressTx.bAddress = gbConnectionAddress;
                                                tFieldAddressTx.fIs8Bit  = TRUE;
                                                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_ACK_VALUE;
                                                
                                                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                                
                                                tUFrameTx.tData.pbArray = NULL;
                                                tUFrameTx.tData.wArrMax = 0;
                                                tUFrameTx.tData.wDataLen = 0;
                                                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                                
                                                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                                #else
                                                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                                #endif
                                                
                                                // connection established!!
                                                gfMasterIsListeningForConnections = FALSE;
                                            }
                                            else
                                            {
                                                // return NACK
                                                DlProtocolFrame                             tUFrameTx;
                                                DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                                DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                                
                                                // stamp Control field->"Frame bits" to make it U-Frame
                                                FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                                tFieldAddressTx.bAddress = gbConnectionAddress;
                                                tFieldAddressTx.fIs8Bit  = TRUE;
                                                tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_NACK_VALUE;
                                                
                                                FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                                FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                                
                                                tUFrameTx.tData.pbArray = NULL;
                                                tUFrameTx.tData.wArrMax = 0;
                                                tUFrameTx.tData.wDataLen = 0;
                                                tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                                
                                                #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                                xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                                #else
                                                QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                                #endif
                                            }
                                        }
                                        else
                                        {
                                            // return NACK for connection
                                            DlProtocolFrame                             tUFrameTx;
                                            DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                            DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                            
                                            // stamp Control field->"Frame bits" to make it U-Frame
                                            FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                            tFieldAddressTx.bAddress = gbConnectionAddress;
                                            tFieldAddressTx.fIs8Bit  = TRUE;
                                            tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_NACK_VALUE;
                                            
                                            FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                            FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                            
                                            tUFrameTx.tData.pbArray = NULL;
                                            tUFrameTx.tData.wArrMax = 0;
                                            tUFrameTx.tData.wDataLen = 0;
                                            tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                            
                                            #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                            xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                            #else
                                            QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                            #endif
                                        }
                                    }
                                    else
                                    {
                                        // return NACK for connection
                                        DlProtocolFrame                             tUFrameTx;
                                        DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                        DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                        
                                        // stamp Control field->"Frame bits" to make it U-Frame
                                        FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                        tFieldAddressTx.bAddress = gbConnectionAddress;
                                        tFieldAddressTx.fIs8Bit  = TRUE;
                                        tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_CONNECT_NACK_VALUE;
                                        
                                        FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                        FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                        
                                        tUFrameTx.tData.pbArray = NULL;
                                        tUFrameTx.tData.wArrMax = 0;
                                        tUFrameTx.tData.wDataLen = 0;
                                        tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                        
                                        #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                        xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                        #else
                                        QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                        #endif
                                    }
                                    break;
                                    
                                case FRAME_FIELD_CNTRL_U_TYPE_CONNECT_ACK_VALUE:
                                    if( gbConnectionAddress == DL_PROTOCOL_CONN_ADDR_SLAVE )
                                    {
                                        if( tAddressRx.bAddress == DL_PROTOCOL_CONN_ADDR_MASTER )
                                        {
                                            if( gfSlaveIsConnecting )
                                            {
                                                // connection established!!
                                                gfSlaveIsConnecting = FALSE;
                                                gfSlaveIsConnected = TRUE;
                                                
                                                gbContinuousRxErrorPacketCounter = 0;
                                                gbContinuousTxErrorPacketCounter = 0;
                                                
                                                // reset packet frame variables
                                                gbIFrameRxSequenceNum = 0;
                                                gbIFrameTxSequenceNum = 0;
                                                
                                                // reset buffer vars
                                                gfISUFrameRxEscapeByteFound = FALSE;
                                                gwISUFrameRxCrc16 = 0;
                                                gwISUFrameRxBufferWalker = 0;
                                            }
                                        }
                                    }
                                    break;

                                case FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_VALUE:
                                {
                                    // this can be requested by master or slave
                                    // return DISCONNECT ACK and reset sequence number or packet variables
                                    DlProtocolFrame                             tUFrameTx;
                                    DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                                    DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                                    
                                    // stamp Control field->"Frame bits" to make it U-Frame
                                    FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                                    tFieldAddressTx.bAddress = gbConnectionAddress;
                                    tFieldAddressTx.fIs8Bit  = TRUE;
                                    tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_ACK_VALUE;
                                    
                                    FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                                    FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                                    
                                    tUFrameTx.tData.pbArray = NULL;
                                    tUFrameTx.tData.wArrMax = 0;
                                    tUFrameTx.tData.wDataLen = 0;
                                    tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                                    
                                    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                                    xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                    xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                    xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                                    #else
                                    QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                    QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                    QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                                    #endif
                                    
                                    // connection closed!!
                                    if( gfMasterIsConnected )
                                    {
                                        gfMasterIsListeningForConnections = TRUE;
                                        gfMasterIsConnected = FALSE;
                                    }
                                    else if( gfSlaveIsConnected )
                                    {
                                        gfSlaveIsConnected = FALSE;
                                    }
                                    
                                    gbContinuousRxErrorPacketCounter = 0;
                                    gbContinuousTxErrorPacketCounter = 0;
                                    
                                    // reset packet frame variables
                                    gbIFrameRxSequenceNum = 0;
                                    gbIFrameTxSequenceNum = 0;
                                    
                                    // reset buffer vars
                                    gfISUFrameRxEscapeByteFound = FALSE;
                                    gwISUFrameRxCrc16 = 0;
                                    gwISUFrameRxBufferWalker = 0;
                                    break;
                                }

                                case FRAME_FIELD_CNTRL_U_TYPE_DISCONNECT_ACK_VALUE:
                                    // connection closed!!
                                    if( gfMasterIsConnected )
                                    {
                                        gfMasterIsListeningForConnections = TRUE;
                                        gfMasterIsConnected = FALSE;
                                    }
                                    else if( gfSlaveIsConnected )
                                    {
                                        gfSlaveIsConnected = FALSE;
                                    }
                                    break;

                                case FRAME_FIELD_CNTRL_U_TYPE_SEQ_NUM_RESET_VALUE:
                                    // adjust the sequence number in the frame   
                                    gbIFrameRxSequenceNum = 0;
                                    gbIFrameTxSequenceNum = 0;
                                    
                                    gfSequenceNumberResetRequested = TRUE;
                                    break;
                            }
                            break;
                        }// END case FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE:

                        default:
                        {
                            break;
                        }// EDN default:
                    }// END switch()
                }
                else
                {
                    #warning This packet response should be used during protocol debugging. remove response after debug ok
                    #if(1)
                    // bad crc. unreliable frame type
                    // return generic error
                    DlProtocolFrame                             tUFrameTx;
                    DlProtocolFrameFieldAddressBitFields        tFieldAddressTx;
                    DlProtocolFrameFieldControlUFrameBitFields  tFieldControlUFrameTx;
                    
                    // stamp Control field->"Frame bits" to make it U-Frame
                    FieldControlSetType( FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE, &tUFrameTx.bControl );

                    tFieldAddressTx.bAddress = gbConnectionAddress;
                    tFieldAddressTx.fIs8Bit  = TRUE;
                    tFieldControlUFrameTx.bType = FRAME_FIELD_CNTRL_U_TYPE_GENERIC_ERROR_VALUE;
                    
                    FieldAddressMergeParts( &tFieldAddressTx, &tUFrameTx.bAddress );
                    FieldControlUFrameMergeParts( &tFieldControlUFrameTx, &tUFrameTx.bControl );
                    
                    tUFrameTx.tData.pbArray = NULL;
                    tUFrameTx.tData.wArrMax = 0;
                    tUFrameTx.tData.wDataLen = 0;
                    tUFrameTx.wCrc16 = FrameGetCrc16( &tUFrameTx );
                    
                    #ifdef PROTOCOL_USE_PROTECTED_QUEUES
                    xQueueSend( tQueueTxSuFrame, &tUFrameTx, PROTOCOL_PROTECTED_QUEUE_TICK_WAIT );
                    #else
                    QueueXAdd( QUEUE_INDEX_S_U_FRAME, &tUFrameTx, sizeof(tUFrameTx) );
                    #endif
                    #endif

                    gbContinuousRxErrorPacketCounter++;
                    //System.out.println("crc missmatch");
                    if( gfDbgEnable )
                    {
                        UsartPrintf( geUsartPortDbg, "bad crc\r\n" );
                    }
                }
            }
            else
            {
                //System.out.println("Frame parsing error");
                if( gfDbgEnable )
                {
                    UsartPrintf( geUsartPortDbg, "parsing error\r\n" );
                }
            }
        }
        
        // System.out.println("all vars reset");
        if( gfDbgEnable )
        {
            UsartPrintf( geUsartPortDbg, "vars reset\r\n" );
        }
        
        // restart buffer and other variables
        gfISUFrameRxEscapeByteFound = FALSE;
        gwISUFrameRxCrc16 = 0;
        gwISUFrameRxBufferWalker = 0;
    }
    else
    {
        ///////////////////////////////////////
        // 1.- check bytes should be escaped
        ///////////////////////////////////////
        if( gfISUFrameRxEscapeByteFound == FALSE )
        {
            if( bByte == PROTOCOL_FLAG_BYTE_ESCAPE )
            {
                gfISUFrameRxEscapeByteFound = TRUE;
                // do not include this byte in the array
            }
        }
        else if( gfISUFrameRxEscapeByteFound == TRUE )
        {
            gfISUFrameRxEscapeByteFound = FALSE;

            // format escaped byte. Apply XOR
            bByte = (bByte ^ PROTOCOL_FLAG_BYTE_ESCAPE);
        }
        ///////////////////////////////////////

        // if no escape byte, record byte in buffer
        if( gfISUFrameRxEscapeByteFound == FALSE )
        {
            if( gwISUFrameRxBufferWalker < sizeof(gbISUFrameRxBuffer) )    
            {
                gbISUFrameRxBuffer[gwISUFrameRxBufferWalker] = bByte;

                gwISUFrameRxBufferWalker++;

                // start calculating gwISUFrameRxCrc16 after 3 bytes received (crc16 offset + EOT byte)
                if(gwISUFrameRxBufferWalker >= 3)
                {
                    // gwISUFrameRxCrc16 = crc_update(gwISUFrameRxCrc16, gbISUFrameRxBuffer[gwISUFrameRxBufferWalker-3]);
                    gwISUFrameRxCrc16 = UpdateCRC16(gwISUFrameRxCrc16, gbISUFrameRxBuffer[gwISUFrameRxBufferWalker-3]);
                }
            }
            else
            {
                // Rx buffer overflow
                if( gfDbgEnable )
                {
                    UsartPrintf( geUsartPortDbg, "Rx buffer overflow\r\n" );
                }

                // restart buffer and other variables
                gfISUFrameRxEscapeByteFound = FALSE;
                gwISUFrameRxCrc16 = 0;
                gwISUFrameRxBufferWalker = 0;
            }
        }
    }
    
    // if 10 errors in a row, throw away the connection
    if( gbContinuousRxErrorPacketCounter >= PROTOCOL_RX_ERROR_MAX )
    {
        gbContinuousRxErrorPacketCounter = 0;

        gfIFrameTxIsPending = FALSE;
        gfIFrameTxIsSent = FALSE;

        if( gfMasterIsConnected )
        {
            gfMasterIsConnected = FALSE;
            gfMasterIsListeningForConnections = TRUE;
        }
        else if( gfSlaveIsConnected )
        {
            gfSlaveIsConnected = FALSE;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 FrameGetCrc16( DlProtocolFrame *ptISUFrame ) 
{
    UINT16 wCrc16 = 0;
    
//    wCrc16 = crc_update( wCrc16, ptISUFrame->bAddress );
//    wCrc16 = crc_update( wCrc16, ptISUFrame->bControl );
    
    wCrc16 = UpdateCRC16( wCrc16, ptISUFrame->bAddress );
    wCrc16 = UpdateCRC16( wCrc16, ptISUFrame->bControl );
    
    if( ptISUFrame->tData.pbArray != NULL )
    {
        for( UINT16 wIdx = 0 ; wIdx < ptISUFrame->tData.wDataLen ; wIdx++ )
        {
            //wCrc16 = crc_update( wCrc16, ptISUFrame->tData.pbArray[wIdx] );
            wCrc16 = UpdateCRC16( wCrc16, ptISUFrame->tData.pbArray[wIdx] );
        }
    }
    
    return wCrc16;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!  Taken from Y modem.c  crc16
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 UpdateCRC16(UINT16 crcIn, UINT8 byte)
{
    UINT32 crc = crcIn;
    UINT32 in = byte | 0x100;

    do
    {
        crc <<= 1;
        in <<= 1;
        
        if(in & 0x100)
          ++crc;

        if(crc & 0x10000)
          crc ^= 0x1021;
    }while(!(in & 0x10000));

    return crc & 0xffffu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ParseFrame( UINT8 *pbFrameBuffer, UINT16 dwBytesReceived, DlProtocolFrame *ptISUFrameResult )
{
    BOOL    fSuccess = FALSE;
    
    if
    ( 
        ( pbFrameBuffer != NULL ) &&
        ( dwBytesReceived >= 4 ) && // minimum bytes are 4
        ( dwBytesReceived <= DL_PROTOC_BUFFER_RX_FRAME_NO_SOT_MAX_BYTE_SIZE ) && // max bytes 
        ( ptISUFrameResult != NULL )
    )
    {
        ptISUFrameResult->bAddress = pbFrameBuffer[0];
        ptISUFrameResult->bControl = pbFrameBuffer[1];
        ptISUFrameResult->wCrc16 = 0;
        ptISUFrameResult->wCrc16 |= (pbFrameBuffer[dwBytesReceived-2] << 8);
        ptISUFrameResult->wCrc16 |= pbFrameBuffer[dwBytesReceived-1];
        
        switch( FieldControlGetType(ptISUFrameResult->bControl) )
        {
            case FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE:
                ptISUFrameResult->tData.pbArray = &pbFrameBuffer[2];
                ptISUFrameResult->tData.wArrMax = dwBytesReceived - 4;  // 2bytes CNTRL+ADDRS 2Bytes CRC16
                ptISUFrameResult->tData.wDataLen = dwBytesReceived - 4;       // 2bytes CNTRL+ADDRS 2Bytes CRC16
                fSuccess = TRUE;
                break;
            case FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE:
                ptISUFrameResult->tData.pbArray = NULL;
                ptISUFrameResult->tData.wDataLen = 0;
                fSuccess = TRUE;
                break;
            case FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE:
                ptISUFrameResult->tData.pbArray = NULL;
                ptISUFrameResult->tData.wDataLen = 0;
                fSuccess = TRUE;
                break;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldAddressSeparateParts( UINT8 bByte, DlProtocolFrameFieldAddressBitFields *ptAddressPartsResult )
{
    BOOL    fSuccess = FALSE;
    UINT8   bByteResult;
    
    if( ptAddressPartsResult != NULL )
    {
        bByteResult = 0;
        
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_ADDRESS_IS_8BIT_BIT_OFFSET, FRAME_FIELD_ADDRESS_IS_8BIT_BIT_SIZE, &bByteResult );
        if( bByteResult == 0 )
        {
            ptAddressPartsResult->fIs8Bit = TRUE;
        }
        else
        {
            ptAddressPartsResult->fIs8Bit = FALSE;
        }
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_ADDRESS_ADDR_BIT_OFFSET, FRAME_FIELD_ADDRESS_ADDR_BIT_SIZE, &bByteResult );
        ptAddressPartsResult->bAddress = bByteResult;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldAddressMergeParts( DlProtocolFrameFieldAddressBitFields *ptAddressParts, UINT8 *pbByteResult )
{
    BOOL fSuccess = FALSE;
    UINT8 bIs8Bit = 0; // this field is inverted
    if
    ( 
        (ptAddressParts != NULL) &&
        (pbByteResult != NULL)
    )
    {
        if( ptAddressParts->fIs8Bit )
        {
            bIs8Bit = 0;
        }
        else
        {
            bIs8Bit = 1;
        }

        fSuccess = TRUE;
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_ADDRESS_IS_8BIT_BIT_OFFSET, FRAME_FIELD_ADDRESS_IS_8BIT_BIT_SIZE, bIs8Bit );
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_ADDRESS_ADDR_BIT_OFFSET, FRAME_FIELD_ADDRESS_ADDR_BIT_SIZE, ptAddressParts->bAddress );
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 FieldControlGetType( UINT8 bByte )
{
    UINT8 bControlFieldType;
    
    bControlFieldType = 0xFF;
    ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_OFFSET, FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_SIZE, &bControlFieldType );
    if( bControlFieldType != FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE )
    {
        // check the other types.. it uses 2 bits for the remaining types
        // S type and U type can be extracted with the same Offset and bit size
        ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_OFFSET, FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_SIZE, &bControlFieldType );
        
        if
        (
            ( bControlFieldType != FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE ) &&
            ( bControlFieldType != FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE )
        )
        {
            // INVALID TYPE
            bControlFieldType = 0xFF;
        }
    }
    
    return bControlFieldType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlSetType( UINT8 bControlFieldType, UINT8 *pbControlFieldResultByte )
{
    BOOL fSuccess = FALSE;
    
    if( pbControlFieldResultByte != NULL )
    {
        switch(bControlFieldType)
        {
            case FRAME_FIELD_CNTRL_FRAME_TYPE_I_VALUE:
                fSuccess = ByteBitFieldSetBitsAtOffset
                ( 
                    pbControlFieldResultByte, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_OFFSET, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_I_BIT_SIZE, 
                    bControlFieldType 
                );
                break;
            case FRAME_FIELD_CNTRL_FRAME_TYPE_S_VALUE:
                fSuccess = ByteBitFieldSetBitsAtOffset
                ( 
                    pbControlFieldResultByte, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_OFFSET, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_S_BIT_SIZE, 
                    bControlFieldType 
                );
                break;
            case FRAME_FIELD_CNTRL_FRAME_TYPE_U_VALUE:
                fSuccess = ByteBitFieldSetBitsAtOffset
                ( 
                    pbControlFieldResultByte, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_U_BIT_OFFSET, 
                    FRAME_FIELD_CNTRL_FRAME_TYPE_U_BIT_SIZE, 
                    bControlFieldType 
                );
                break;
            default: // INVALID
                break;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlIFrameSeparateParts( UINT8 bByte, DlProtocolFrameFieldControlIFrameBitFields *ptControlIFramePartsResult )
{
    BOOL    fSuccess = FALSE;
    UINT8   bByteResult;
    
    if( ptControlIFramePartsResult != NULL )
    {
        bByteResult = 0;
        
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_OFFSET, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_SIZE, &bByteResult );
        ptControlIFramePartsResult->bSeqNumReceive = bByteResult;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_OFFSET, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_SIZE, &bByteResult );
        ptControlIFramePartsResult->bSeqNumTransmit = bByteResult;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlIFrameMergeParts( DlProtocolFrameFieldControlIFrameBitFields *ptControlIFrameParts, UINT8 *pbByteResult )
{
    BOOL fSuccess = FALSE;
    
    if
    (
        ( ptControlIFrameParts != NULL ) &&
        ( pbByteResult != NULL )
    )
    {
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_OFFSET, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_RX_BIT_SIZE, ptControlIFrameParts->bSeqNumReceive );
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_OFFSET, FRAME_FIELD_CNTRL_I_FRAME_SEQ_NUM_TX_BIT_SIZE, ptControlIFrameParts->bSeqNumTransmit );
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlSFrameSeparateParts( UINT8 bByte, DlProtocolFrameFieldControlSFrameBitFields *ptControlSFramePartsResult )
{
    BOOL    fSuccess = FALSE;
    UINT8   bByteResult;
    
    if( ptControlSFramePartsResult != NULL )
    {
        bByteResult = 0;
        
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_OFFSET, FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_SIZE, &bByteResult );
        ptControlSFramePartsResult->bSeqNumReceive = bByteResult;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_OFFSET, FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_SIZE, &bByteResult );
        ptControlSFramePartsResult->bType = bByteResult;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlSFrameMergeParts( DlProtocolFrameFieldControlSFrameBitFields *ptControlSFrameParts, UINT8 *pbByteResult )
{
    BOOL fSuccess = FALSE;
    
    if
    (
        ( ptControlSFrameParts != NULL ) &&
        ( pbByteResult != NULL )
    )
    {
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_OFFSET, FRAME_FIELD_CNTRL_S_FRAME_SEQ_NUM_RX_BIT_SIZE, ptControlSFrameParts->bSeqNumReceive );
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_OFFSET, FRAME_FIELD_CNTRL_S_FRAME_TYPE_BIT_SIZE, ptControlSFrameParts->bType );
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlUFrameSeparateParts( UINT8 bByte, DlProtocolFrameFieldControlUFrameBitFields *ptControlUFramePartsResult )
{
    BOOL    fSuccess = FALSE;
    UINT8   bByteResult;
    
    if( ptControlUFramePartsResult != NULL )
    {
        bByteResult = 0;
        
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_OFFSET, FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_SIZE, &bByteResult );
        ptControlUFramePartsResult->bType = bByteResult;
        fSuccess &= ByteBitFieldGetBitsAtOffset( bByte, FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_OFFSET, FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_SIZE, &bByteResult );
        ptControlUFramePartsResult->bType |= (bByteResult << 2);
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FieldControlUFrameMergeParts( DlProtocolFrameFieldControlUFrameBitFields *ptControlUFrameParts, UINT8 *pbByteResult )
{
    BOOL fSuccess = FALSE;
    
    if
    (
        ( ptControlUFrameParts != NULL ) &&
        ( pbByteResult != NULL )
    )
    {
        fSuccess = TRUE;
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_OFFSET, FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_SIZE, ptControlUFrameParts->bType );
        fSuccess &= ByteBitFieldSetBitsAtOffset( pbByteResult, FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_OFFSET, FRAME_FIELD_CNTRL_U_FRAME_TYPE_MSB_BIT_OFFSET, (ptControlUFrameParts->bType >> FRAME_FIELD_CNTRL_U_FRAME_TYPE_LSB_BIT_SIZE) );
    }
    
    return fSuccess;
}
