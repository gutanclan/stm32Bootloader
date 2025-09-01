//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \note
//!
//! \author
//! \date
//!
//////////////////////////////////////////////////////////////////////////////////////////////////


// C Library includes

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../inc/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../Utils/Types.h"
#include "../Utils/Queue.h"
#include "../Utils/timer.h"
#include "../Targets/Target_CurrentTarget.h"

#include "../DriversMcuPeripherals/gpio.h"
#include "../DriversMcuPeripherals/usart.h"
#include "serial.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SERIAL_QUEUE_ACTION_SEND,
    SERIAL_QUEUE_ACTION_RECEIVE,

    SERIAL_QUEUE_ACTION_MAX,
}SerialQueueActionEnum;

typedef enum
{
    SERIAL_QUEUE_TYPE_RX,
    SERIAL_QUEUE_TYPE_TX,

    SERIAL_QUEUE_TYPE_MAX,
}SerialQueueTypeEnum;

typedef struct
{
    UsartPortEnum               eUsart;
    USART_TypeDef               *ptUsartRegPointer;
    UINT8                       bIrqN;
}SerialUsartPortCommonType;

typedef struct
{
    QueueEnum                   eQueue;
    GpioPortEnum                eGpioPort;
    UINT8                       bGpioPin;
    GpioAlternativeFunctionEnum eAf;
}SerialStreamType;

typedef struct
{
    TIMER   tTimerUpTime;
    BOOL    fIsTimerStarted;
    UINT16  wRxEventCountCurrent;
    UINT16  wRxEventCountPrevious;
    BOOL    fIsClientConnected;
}SerialClientDetectType;

typedef struct
{
    const CHAR                  *const  pcPortName;
    const SerialUsartPortCommonType     tPortCommon;
    SerialPortConfigType                tPortConfig;
    const SerialStreamType              tRx;
    const SerialStreamType              tTx;
    SerialClientDetectType              tClientDetect;
}SerialType;

static SerialType     gtSerialLookupArray[] =
{
    {
        // ### PERIPH
        // Name
        "MAIN",
        // ### COMMON
        // usart                ptUsartRegPointer   irq
        { USART_PORT_3,         USART3,              USART3_IRQn },
        // ### PORT CONFIG
		// dwBaudRate		eData                 		eParity				eStop
        { 115200,       	USART_WORD_LENGTH_8_BITS,	USART_PARITY_NON,   USART_STOP_BIT_1 },
        // ### PIN CONFIG
        // pin rx
        {
            // queue
            QUEUE_USART3_RX,
            // port
            GPIO_PORT_C,
            // pin
            11,
            // af
            GPIO_ALT_FUNC_USART1_USART3
        },
        // pin tx
        {
            // queue
            QUEUE_USART3_TX,
            // port
            GPIO_PORT_C,
            // pin
            10,
            // af
            GPIO_ALT_FUNC_USART1_USART3
        },
        // tClientDetect
        {
            0,
            FALSE,
            0,
            0,
            FALSE
        }
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
};

#define SERIAL_ARRAY_MAX	( sizeof(gtSerialLookupArray) / sizeof( gtSerialLookupArray[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL SerialPortInterruptionsEnabled  ( SerialPortEnum ePort, BOOL fIsEnabled );
static BOOL SerialQueueAccess               ( SerialPortEnum ePort, SerialQueueTypeEnum eQueueType, SerialQueueActionEnum eQueueAction, CHAR *pcChar );
static void SerialCommonIrqHandler          ( SerialPortEnum ePort );

// void        USART1_IRQHandler      ( void );
// void        USART2_IRQHandler      ( void );
void        USART3_IRQHandler      ( void );
// void        UART4_IRQHandler       ( void );
// void        UART5_IRQHandler       ( void );
// void        USART6_IRQHandler      ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialModuleInit( void )
{
	BOOL fSuccess = FALSE;

    if( SERIAL_PORT_MAX != SERIAL_ARRAY_MAX )
    {
        // definition mismatch error
        while(0);
    }
    else
    {
        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialPortEnable( SerialPortEnum ePort, BOOL fEnable )
{
	BOOL                fSuccess = FALSE;
    NVIC_InitTypeDef    NVIC_InitStructure;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( fEnable )
        {
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // QUEUES
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            QueueReset( gtSerialLookupArray[ePort].tRx.eQueue );
            QueueReset( gtSerialLookupArray[ePort].tTx.eQueue );

            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // GPIO RX PIN
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

            if( GpioPortClockIsEnabled( gtSerialLookupArray[ePort].tRx.eGpioPort ) == FALSE )
            {
                GpioPortClockEnable( gtSerialLookupArray[ePort].tRx.eGpioPort, fEnable );
            }
            // prepare pin for input RX
            GpioSpeedSetVal( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin, GPIO_OSPEED_25MHZ );
            GpioPuPdSetVal( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin, GPIO_PUPD_NOPUPD );
            // change gpio as Alternative function
            GpioModeSetAf( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin, gtSerialLookupArray[ePort].tRx.eAf );


            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // GPIO TX PIN
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

            if( GpioPortClockIsEnabled( gtSerialLookupArray[ePort].tTx.eGpioPort ) == FALSE )
            {
                GpioPortClockEnable( gtSerialLookupArray[ePort].tTx.eGpioPort, fEnable );
            }
            // prepare pin for output TX
            GpioSpeedSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_OSPEED_25MHZ );
            GpioPuPdSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_PUPD_NOPUPD );
            GpioOTypeSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_OTYPE_PUSH_PULL );
            GpioOutputRegWrite( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_LOGIC_HIGH );
            GpioModeSetOutput( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin );
            // change gpio as Alternative function
            GpioModeSetAf( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, gtSerialLookupArray[ePort].tTx.eAf );

            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // USART
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

			// clock
			UsartPortClockEnable( gtSerialLookupArray[ePort].tPortCommon.eUsart, fEnable );

			// configure serial
			// WARNING!! configuration changes should be performed only with usart disabled.
			/*
			Follow the order(otherwise there could be garbage in the first rx char):
			1.- stop bits
			2.- word len
			3.- parity
			4.- mode
			5.- hardware flow control (not implemented in code but not required since by default is off)
			6.- baud rate
			*/
            UsartPortSetStopBit( gtSerialLookupArray[ePort].tPortCommon.eUsart, gtSerialLookupArray[ePort].tPortConfig.eStop );
            UsartPortSetWordLength( gtSerialLookupArray[ePort].tPortCommon.eUsart, gtSerialLookupArray[ePort].tPortConfig.eData );
            UsartPortSetParity( gtSerialLookupArray[ePort].tPortCommon.eUsart, gtSerialLookupArray[ePort].tPortConfig.eParity );
            UsartPortSetBaud( gtSerialLookupArray[ePort].tPortCommon.eUsart, gtSerialLookupArray[ePort].tPortConfig.dwBaudRate );
            UsartPortSetMode( gtSerialLookupArray[ePort].tPortCommon.eUsart, USART_MODE_RXTX );

            //////////////////////////////////////////////////////////////////
            /* Enable the selected USART by setting the UE bit in the CR1 register */
            // gtSerialLookupArray[ePort].tPortCommon.ptUsart->CR1 |= USART_CR1_UE;
            UsartPortEnable( gtSerialLookupArray[ePort].tPortCommon.eUsart, fEnable );
            //////////////////////////////////////////////////////////////////

            // last thing to enable .- interruptions
            SerialPortInterruptionsEnabled( ePort, fEnable );
        }
        else
        {
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // USART
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

            SerialPortInterruptionsEnabled(ePort,fEnable);

            //////////////////////////////////////////////////////////////////
            /* Disable the selected USART by clearing the UE bit in the CR1 register */
            //gtSerialLookupArray[ePort].tPortCommon.ptUsart->CR1 &= (uint16_t)~((uint16_t)USART_CR1_UE);
            UsartPortEnable( gtSerialLookupArray[ePort].tPortCommon.eUsart, fEnable );
            //////////////////////////////////////////////////////////////////

            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // GPIO RX PIN
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            GpioModeSetInput( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin );
            GpioPuPdSetVal( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin, GPIO_PUPD_P_DOWN );
            GpioSpeedSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_OSPEED_2MHZ );


            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            // GPIO TX PIN
            // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
            GpioModeSetInput( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin );
            GpioPuPdSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_PUPD_P_DOWN );
            GpioSpeedSetVal( gtSerialLookupArray[ePort].tTx.eGpioPort, gtSerialLookupArray[ePort].tTx.bGpioPin, GPIO_OSPEED_2MHZ );
        }
    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialPortIsEnabled( SerialPortEnum ePort )
{
	BOOL fSuccess = FALSE;

	if( ePort < SERIAL_ARRAY_MAX )
    {
		fSuccess = UsartPortIsEnabled( gtSerialLookupArray[ePort].tPortCommon.eUsart );
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialPortSetConfig( SerialPortEnum ePort, SerialPortConfigType *ptPortConfig )
{
	BOOL fSuccess = FALSE;
    BOOL fIsPortEnabled = FALSE;

	if( ePort < SERIAL_ARRAY_MAX )
    {
        if( ptPortConfig != NULL )
        {
            fIsPortEnabled = SerialPortIsEnabled(ePort);

            // disable usart before changing configuration
            if( fIsPortEnabled )
            {
                SerialPortEnable( ePort, FALSE );
            }

            // set configuration
            memcpy( &gtSerialLookupArray[ePort].tPortConfig, ptPortConfig, sizeof(gtSerialLookupArray[ePort].tPortConfig) );

            // enable usart (if it was enabled before)
            // when enabling again, usart will be reconfigured.
            if( fIsPortEnabled )
            {
                SerialPortEnable( ePort, TRUE );
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
BOOL SerialPortInterruptionsEnabled( SerialPortEnum ePort, BOOL fIsEnabled )
{
    BOOL fSuccess = FALSE;
    NVIC_InitTypeDef    NVIC_InitStructure;
    FunctionalState     eEnableDisableState;

    eEnableDisableState = DISABLE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( fIsEnabled == TRUE )
        {
            eEnableDisableState = ENABLE;
        }

        ////////////////////////////////////////////////////////////////////////////
        // #### INTERRUPT ENABLE ####
        ////////////////////////////////////////////////////////////////////////////
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 0x01;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 0x0F;
        NVIC_InitStructure.NVIC_IRQChannel                      = gtSerialLookupArray[ePort].tPortCommon.bIrqN;
        NVIC_InitStructure.NVIC_IRQChannelCmd                   = eEnableDisableState;
        NVIC_Init( &NVIC_InitStructure );

        ////////////////////////////////////////////////////////////////////////////
        // #### USART ENABLE ####
        ////////////////////////////////////////////////////////////////////////////
        // clear interrupt flags before enabling interrupts
        USART_ClearFlag( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_RXNE );
        USART_ClearFlag( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE );
        // Enable Rx interruptions
        USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_RXNE, eEnableDisableState );
        USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_ERR, eEnableDisableState );
        // NOTE: TX interruption enabled only when is time to transmit

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialPortGetConfig( SerialPortEnum ePort, SerialPortConfigType *ptPortConfig )
{
	BOOL fSuccess = FALSE;

	if( ePort < SERIAL_ARRAY_MAX )
    {
		if( ptPortConfig != NULL )
		{
			// WARNING!! size to be used should be of destination pointer not source.
			memcpy( ptPortConfig, &gtSerialLookupArray[ePort].tPortConfig, sizeof(gtSerialLookupArray[ePort].tPortConfig) );

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
BOOL SerialPutChar( SerialPortEnum ePort, CHAR cChar )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_TX, SERIAL_QUEUE_ACTION_SEND, &cChar ) == TRUE )
        {
            fSuccess = TRUE;

            USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE, ENABLE );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialGetChar( SerialPortEnum ePort, CHAR *pcChar )
{
    BOOL fSuccess = FALSE;
    CHAR cChar;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_RX, SERIAL_QUEUE_ACTION_RECEIVE, &cChar ) == TRUE )
        {
            fSuccess = TRUE;

            if( NULL != pcChar )
            {
                (*pcChar) = cChar;
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
BOOL SerialPutBuffer( SerialPortEnum ePort, const void *const pvBuffer, UINT16 wBufferSize )
{
    BOOL    fSuccess     = FALSE;
    UINT8  *pbByteBuffer = NULL;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( UsartPortIsEnabled(ePort) )
        {
            // cast void pointer to byte pointer
            pbByteBuffer = (UINT8 *)(pvBuffer);

            for( UINT16 wBufferIndex = 0; wBufferIndex < wBufferSize; wBufferIndex++ )
            {
                if( SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_TX, SERIAL_QUEUE_ACTION_SEND, &pbByteBuffer[wBufferIndex] ) == FALSE )
                {
                    /* Cannot fit any more in the queue.  Try turning the Tx on to clear some space. */
                    USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE, ENABLE );

                    TimerBlockingDelayMs( 1*4 );

                    // best effort to add the failed character
                    SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_TX, SERIAL_QUEUE_ACTION_SEND, &pbByteBuffer[wBufferIndex] );
                }
            }

            USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE, ENABLE );

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
BOOL SerialGetBuffer( SerialPortEnum ePort, void *pvBuffer, UINT16 wBufferSize, UINT16 *pwBytesReceived )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if
        (
            ( pvBuffer != NULL ) &&
            ( wBufferSize > 0 ) &&
            ( pwBytesReceived != NULL )
        )
        {
            for( (*pwBytesReceived) = 0; (*pwBytesReceived) < wBufferSize ; (*pwBytesReceived)++ )
            {
                if( SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_RX, SERIAL_QUEUE_ACTION_RECEIVE, &pvBuffer[(*pwBytesReceived)] ) == FALSE )
                {
                    break;
                }
                else
                {
                    // at least one char received in buffer
                    fSuccess = TRUE;
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
// interruptions enabled: USART_IT_TXE: Transmit Data Register empty interrupt
// handler will get interruption when Tx data reg is empty(finished sending the character)
// interruptions enabled: USART_IT_RXNE: Receive Data register not empty interrupt
void SerialCommonIrqHandler( SerialPortEnum ePort )
{
    CHAR cChar = 0;

    // interruption tx
    if( USART_GetITStatus( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE ) == SET )
    {
        // The interrupt was caused by the THR becoming empty.
        // check if there are any more characters to transmit
        if( SerialQueueAccess( ePort, SERIAL_QUEUE_TYPE_TX, SERIAL_QUEUE_ACTION_RECEIVE, &cChar ) == TRUE )
        {
            // A character was retrieved from the buffer so can be sent to the THR now
            USART_SendData( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, cChar );
        }
        else
        {
            USART_ITConfig( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_TXE, DISABLE );
        }
    }

    // USART_IT_RXNE : Received Data Ready to be Read
    if( USART_GetITStatus( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer, USART_IT_RXNE ) == SET )
    {
        cChar = USART_ReceiveData( gtSerialLookupArray[ePort].tPortCommon.ptUsartRegPointer );

        QueueSend( gtSerialLookupArray[ePort].tRx.eQueue, &cChar );

        gtSerialLookupArray[ePort].tClientDetect.wRxEventCountCurrent++;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
// void USART1_IRQHandler( void )
// {
// //     SerialCommonIrqHandler( USART_PORT_2 );
// }
// void USART2_IRQHandler( void )
// {
// //     SerialCommonIrqHandler( USART_PORT_2 );
// }
void USART3_IRQHandler( void )
{
     SerialCommonIrqHandler( USART_PORT_3 );
}
// void UART4_IRQHandler( void )
// {
// //     SerialCommonIrqHandler( USART_PORT_2 );
// }void UART5_IRQHandler( void )
// {
// //     SerialCommonIrqHandler( USART_PORT_2 );
// }
// void USART6_IRQHandler( void )
// {
// //     SerialCommonIrqHandler( USART_PORT_2 );
// }

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialQueueAccess( SerialPortEnum ePort, SerialQueueTypeEnum eQueueType, SerialQueueActionEnum eQueueAction, CHAR *pcChar )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        switch( eQueueType )
        {
            //////////////////////////////////////////////////////////////////////////////////
            case SERIAL_QUEUE_TYPE_RX:
                // give preference to RX SEND to queue
                switch( eQueueAction )
                {
                    case SERIAL_QUEUE_ACTION_SEND:
                        fSuccess = QueueSend( gtSerialLookupArray[ePort].tRx.eQueue, pcChar );
                        break;
                    case SERIAL_QUEUE_ACTION_RECEIVE:
                        fSuccess = QueueReceive( gtSerialLookupArray[ePort].tRx.eQueue, pcChar, 1 );
                        break;
                    default:
                        break;
                }
                break;
            //////////////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////////////
            case SERIAL_QUEUE_TYPE_TX:
                switch( eQueueAction )
                {
                    case SERIAL_QUEUE_ACTION_SEND:
                        fSuccess = QueueSend( gtSerialLookupArray[ePort].tTx.eQueue, pcChar );
                        break;
                    case SERIAL_QUEUE_ACTION_RECEIVE:
                        fSuccess = QueueReceive( gtSerialLookupArray[ePort].tTx.eQueue, pcChar, 1 );
                        break;
                    default:
                        break;
                }
                break;
            //////////////////////////////////////////////////////////////////////////////////

            default:
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
BOOL SerialQueueClear( SerialPortEnum ePort, SerialQueueTypeEnum eQueueType )
{
    BOOL    fSuccess     = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        switch( eQueueType )
        {
            case SERIAL_QUEUE_TYPE_RX:
                fSuccess = QueueReset( gtSerialLookupArray[ePort].tRx.eQueue );
                break;
            case SERIAL_QUEUE_TYPE_TX:
                fSuccess = QueueReset( gtSerialLookupArray[ePort].tTx.eQueue );
                break;
            default:
                break;
        }
    }

    return fSuccess;
}


void SerialPortIsClientConnectedUpdate( void )
{
    GpioLogicEnum eLogic;

    for( SerialPortEnum ePort = 0 ; ePort < SERIAL_PORT_MAX ; ePort++ )
    {
        // check if Rx voltage is 0
        GpioInputRegRead( gtSerialLookupArray[ePort].tRx.eGpioPort, gtSerialLookupArray[ePort].tRx.bGpioPin, &eLogic );

        if( eLogic == GPIO_LOGIC_LOW )
        {
            // check if client got disconnected
            if( gtSerialLookupArray[ePort].tClientDetect.fIsTimerStarted == FALSE )
            {
                // start timer
                gtSerialLookupArray[ePort].tClientDetect.fIsTimerStarted = TRUE;

                gtSerialLookupArray[ePort].tClientDetect.tTimerUpTime = TimerUpTimerStartMs();

                gtSerialLookupArray[ePort].tClientDetect.wRxEventCountPrevious = gtSerialLookupArray[ePort].tClientDetect.wRxEventCountCurrent;
            }
            else
            {
                // is time to check?
                if( TimerUpTimerGetMs( gtSerialLookupArray[ePort].tClientDetect.tTimerUpTime ) > 100 )
                {
                    // is usart having Rx activity?
                    if( gtSerialLookupArray[ePort].tClientDetect.wRxEventCountPrevious == gtSerialLookupArray[ePort].tClientDetect.wRxEventCountCurrent )
                    {
                        // count of rx events is same
                        // no activity detected
                        gtSerialLookupArray[ePort].tClientDetect.fIsClientConnected = FALSE;
                    }
                    else // having activity
                    {
                        // reset vars
                        gtSerialLookupArray[ePort].tClientDetect.fIsTimerStarted = FALSE;
                    }
                }
            }
        }
        else // if HIGH
        {
            gtSerialLookupArray[ePort].tClientDetect.fIsClientConnected = TRUE;
            // reset vars
            gtSerialLookupArray[ePort].tClientDetect.fIsTimerStarted = FALSE;
        }
    }// end for loop
}

BOOL SerialPortIsClientConnected( SerialPortEnum ePort )
{
    if( ePort < SERIAL_ARRAY_MAX )
    {
        return gtSerialLookupArray[ePort].tClientDetect.fIsClientConnected;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * SerialPortGetName( SerialPortEnum ePort )
{
    if( ePort < SERIAL_ARRAY_MAX )
    {
        return &gtSerialLookupArray[ePort].pcPortName[0];
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
BOOL SerialGetUsart( SerialPortEnum ePort, UsartPortEnum *peUsart )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( peUsart!= NULL )
        {
            if( gtSerialLookupArray[ePort].tPortCommon.eUsart < USART_PORT_TOTAL )
            {
                (*peUsart) = gtSerialLookupArray[ePort].tPortCommon.eUsart;
            }
            else
            {
                (*peUsart) = USART_PORT_INVALID;
            }
        }
    }

    return fSuccess;
}

BOOL SerialGetGpioRx( SerialPortEnum ePort, GpioPortEnum *peGpioPort, UINT8 *pbGpioPin )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( ( peGpioPort!= NULL ) && ( pbGpioPin!= NULL ) )
        {
            (*peGpioPort)   = gtSerialLookupArray[ePort].tRx.eGpioPort;
            (*pbGpioPin)    = gtSerialLookupArray[ePort].tRx.bGpioPin;
        }
    }

    return fSuccess;
}

BOOL SerialGetGpioTx( SerialPortEnum ePort, GpioPortEnum *peGpioPort, UINT8 *pbGpioPin )
{
    BOOL fSuccess = FALSE;

    if( ePort < SERIAL_ARRAY_MAX )
    {
        if( ( peGpioPort!= NULL ) && ( pbGpioPin!= NULL ) )
        {
            (*peGpioPort)   = gtSerialLookupArray[ePort].tTx.eGpioPort;
            (*pbGpioPin)    = gtSerialLookupArray[ePort].tTx.bGpioPin;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SerialTest( void )
{
    CHAR cChar;

    SerialPortConfigType tPortConfig;
    tPortConfig.dwBaudRate = 115200;
    tPortConfig.eData = USART_WORD_LENGTH_8_BITS;
    tPortConfig.eParity = USART_PARITY_NON;
    tPortConfig.eStop = USART_STOP_BIT_1;

    SerialModuleInit();
    SerialPortSetConfig( SERIAL_PORT_MAIN, &tPortConfig );
    SerialPortEnable( SERIAL_PORT_MAIN, TRUE );

    while(1)
    {
        SerialPortIsClientConnectedUpdate();

        if( SerialGetChar( SERIAL_PORT_MAIN, &cChar ) )
        {
            SerialPutChar( SERIAL_PORT_MAIN, cChar );
        }
    }
}
