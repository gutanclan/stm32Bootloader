//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        usart.c
//!    \brief       Usart module header
//!
//!    \author
//!    \date
//!
//!    \defgroup
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _USART_H_
#define _USART_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Library includes
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    USART_PORT_3, // main console

    USART_PORT_TOTAL,
}UsartPortEnum;

#define USART_PORT_INVALID (USART_PORT_TOTAL)

typedef enum
{
    USART_PARITY_NON = 0,
    USART_PARITY_ODD,
    USART_PARITY_EVEN,

    USART_PARITY_MAX,
}UsartParityEnum;

typedef enum
{
    USART_STOP_BIT_1 = 0,
    USART_STOP_BIT_0_5,
    USART_STOP_BIT_2,
    USART_STOP_BIT_1_5,

    USART_STOP_BIT_MAX,
}UsartStopBitEnum;

typedef enum
{
    USART_WORD_LENGTH_8_BITS = 0,
    USART_WORD_LENGTH_9_BITS,

    USART_WORD_LENGTH_MAX,
}UsartWordLengthEnum;

typedef enum
{
    USART_MODE_NONE = 0,
    USART_MODE_RX,
    USART_MODE_TX,
    USART_MODE_RXTX,

    USART_MODE_MAX
}UsartModeEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    UsartModuleInit         ( void );

BOOL 	UsartPortClockEnable    ( UsartPortEnum ePort, BOOL fEnable );
BOOL 	UsartPortClockIsEnabled ( UsartPortEnum ePort );

BOOL    UsartPortEnable         ( UsartPortEnum ePort, BOOL fEnable );
BOOL    UsartPortIsEnabled      ( UsartPortEnum ePort );

BOOL    UsartPortSetBaud        ( UsartPortEnum ePort, UINT32 dwBaudRate );
BOOL    UsartPortGetBaud        ( UsartPortEnum ePort, FLOAT *psgBaudRate );

BOOL    UsartPortSetMode        ( UsartPortEnum ePort, UsartModeEnum eMode );
BOOL    UsartPortGetMode        ( UsartPortEnum ePort, UsartModeEnum *peMode );

BOOL    UsartPortSetWordLength  ( UsartPortEnum ePort, UsartWordLengthEnum eWordLen );
BOOL    UsartPortGetWordLength  ( UsartPortEnum ePort, UsartWordLengthEnum *peWordLen );

BOOL    UsartPortSetStopBit     ( UsartPortEnum ePort, UsartStopBitEnum eStopBit );
BOOL    UsartPortGetStopBit     ( UsartPortEnum ePort, UsartStopBitEnum *peStopBit );

BOOL    UsartPortSetParity      ( UsartPortEnum ePort, UsartParityEnum eParity );
BOOL    UsartPortGetParity      ( UsartPortEnum ePort, UsartParityEnum *peParity );


CHAR *  UsartPortGetModeName    ( UsartModeEnum eMode );
CHAR *  UsartPortGetWordLenName ( UsartWordLengthEnum eWordLen );
CHAR *  UsartPortGetStopBitName ( UsartStopBitEnum eStopBit );
CHAR *  UsartPortGetParityName  ( UsartParityEnum eParity );

CHAR *  UsartPortGetName        ( UsartPortEnum ePort );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif


