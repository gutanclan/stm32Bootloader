//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \note     Puracom revisions were made for hardware targetting support. Across all platforms,
//!           it is taken that COM0 is assigned to the console at 115200-8,N,1 and COM1 assigned
//!           to the WiFi TCP/IP serial stream at 115200-8,N,1.
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
#include "../../hal/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../common/types.h"
#include "../common/Queue.h"
#include "../common/Target_CurrentTarget.h"

#include "gpio.h"
#include "usart.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

// - - - - - - - - - - - - - - - - - - -

#define USART_CR1_REG_PARITY_CONTROL_ENABLE_MASK    (0x0400)
#define USART_CR1_REG_PARITY_SELECTION_MASK         (0x0200)
#define USART_CR2_REG_STOP_BITS_MASK                (0x3000)
#define USART_CR1_REG_M_MASK                        (0x1000)
#define USART_CR1_REG_RXTX_MASK                     (0x000C)

static const CHAR * UsartModeNameArray[] =
{
	"NON",
	"RX",
	"TX",
	"RXTX",
};
static const CHAR * UsartWordLenNameArray[] =
{
	"8_BIT",
	"9_BIT",
};
static const CHAR * UsartStopBitNameArray[] =
{
	"1_0_BIT",
	"0_5_BIT",
	"2_0_BIT",
	"1_5_BIT",
};
static const CHAR * UsartParityNameArray[] =
{
	"NON",
	"ODD",
	"EVEN",
};

typedef struct
{
    USART_TypeDef               *ptUsart;
    UINT32                      dwRcc;
}UsartPortCommonType;

typedef struct
{
    const CHAR                  *const  pcPortName;
    const UsartPortCommonType           tPortCommon;
}UsartType;

static UsartType     gtUsartLookupArray[USART_PORT_TOTAL] =
{
    {
        // ### PERIPH
        // Name
        "USART3",
        // ### COMMON
        // usart        rcc
        { USART3,       RCC_APB1Periph_USART3 },
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
};

#define USART_ARRAY_MAX	( sizeof(gtUsartLookupArray) / sizeof( gtUsartLookupArray[0] ) )

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsartModuleInit( void )
{
    BOOL fSuccess = FALSE;

    if( USART_ARRAY_MAX != USART_PORT_TOTAL )
    {
        while(TRUE)
        {
            // constants has to match size
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsartPortClockEnable( UsartPortEnum ePort, BOOL fEnable )
{
    BOOL fSuccess = FALSE;

    if(ePort < USART_ARRAY_MAX)
    {
        switch(gtUsartLookupArray[ePort].tPortCommon.dwRcc)
        {
            case RCC_APB1Periph_USART2:
            case RCC_APB1Periph_USART3:
            case RCC_APB1Periph_UART4:
            case RCC_APB1Periph_UART5:
                if( fEnable )
                {
                    RCC->APB1ENR |= gtUsartLookupArray[ePort].tPortCommon.dwRcc;
                }
                else{
                    RCC->APB1ENR &= ~gtUsartLookupArray[ePort].tPortCommon.dwRcc;
                }
                fSuccess = TRUE;
                break;

            case RCC_APB2Periph_USART1:
            case RCC_APB2Periph_USART6:
                if( fEnable )
                {
                    RCC->APB2ENR |= gtUsartLookupArray[ePort].tPortCommon.dwRcc;
                }
                else{
                    RCC->APB2ENR &= ~gtUsartLookupArray[ePort].tPortCommon.dwRcc;
                }
                fSuccess = TRUE;
                break;
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
BOOL UsartPortClockIsEnabled( UsartPortEnum ePort )
{
    BOOL fEnabled = FALSE;

    if( ePort < USART_ARRAY_MAX)
    {
        switch(gtUsartLookupArray[ePort].tPortCommon.dwRcc)
        {
            case RCC_APB1Periph_USART2:
            case RCC_APB1Periph_USART3:
            case RCC_APB1Periph_UART4:
            case RCC_APB1Periph_UART5:
                fEnabled = ( ( RCC->APB1ENR & gtUsartLookupArray[ePort].tPortCommon.dwRcc ) > 0 );
                break;

            case RCC_APB2Periph_USART1:
            case RCC_APB2Periph_USART6:
                fEnabled = ( ( RCC->APB2ENR & gtUsartLookupArray[ePort].tPortCommon.dwRcc ) > 0 );
                break;
            default:
                break;
        }
    }

    return fEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsartPortEnable( UsartPortEnum ePort, BOOL fEnable )
{
    BOOL    fSuccess            = FALSE;
    UINT32  dwReg1BitMask       = 0;
    UINT32  dwRegCurrentStat    = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( ( fEnable == TRUE ) || ( fEnable == FALSE ) )
        {
            dwReg1BitMask  = USART_CR1_UE;

            // invert mask to set all regs that are not related to the pin selected
            dwReg1BitMask  = ~dwReg1BitMask;

            dwRegCurrentStat = gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1;

            // clear pin reg
            dwRegCurrentStat = dwRegCurrentStat & dwReg1BitMask;

            // set value
            dwReg1BitMask = fEnable << 13;

            // set new mode
            dwRegCurrentStat = ( dwRegCurrentStat | dwReg1BitMask );

            // set the reg
            gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 = dwRegCurrentStat;

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
BOOL UsartPortIsEnabled( UsartPortEnum ePort )
{
    BOOL fIsEnabled = FALSE;

    if( ePort < USART_ARRAY_MAX )
    {
        fIsEnabled = ( ( gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 & USART_CR1_UE ) > 0 );
    }

    return fIsEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! http://www.kaltpost.de/?page_id=167
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
/*
Next we need to setup the USART. First we are going to define the baud rate to use. This is done by using the USARTs BRR register. Since the STM32 has a fractional generator any baud rate must be derived from the systems bus clock. The value to store in the BRR register simply could by calculated by this simple formula:

USART_BRR = [BUS_FREQUENCY_in_Hz] / [BAUD_RATE]

To setup the USART for 38400 bauds, we do the following in our code:

    // Configure BRR by deviding the bus clock with the baud rate
    USART1->BRR = 8000000/38400;
The last thing we have to do is to enable the UART, the TX unit of the URAT and the RX unit of the USART. This is done by setting the corresponding flags on th config register 1 of the UART1:

USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
Thats it. Your USART is now ready to communicate with 38400 bauds at 8 data bits, no flow control and one stop-bit.
*/
BOOL UsartPortSetBaud( UsartPortEnum ePort, UINT32 dwBaudRate )
{
    BOOL    fSuccess            = FALSE;

    FLOAT   sgTemp              = 0.0;
    UINT32  dwMant              = 0x00;
    UINT32  dwFrac              = 0x00;
    UINT32  dwBrrReg            = 0x00;

    UINT32  dwApbClock          = 0x00;
    RCC_ClocksTypeDef RCC_ClocksStatus;

    if( ePort < USART_ARRAY_MAX )
    {
        // Get bus clock freq
        RCC_GetClocksFreq( &RCC_ClocksStatus );

        if
        (
            ( gtUsartLookupArray[ePort].tPortCommon.ptUsart == USART1 ) ||
            ( gtUsartLookupArray[ePort].tPortCommon.ptUsart == USART6 )
        )
        {
            dwApbClock = RCC_ClocksStatus.PCLK2_Frequency;
        }
        else
        {
            dwApbClock = RCC_ClocksStatus.PCLK1_Frequency;
        }

        sgTemp = dwApbClock / ( 16.0 * dwBaudRate );

        dwMant = (UINT32)sgTemp;
        // (fractionalPart) * 16 is to convert the fractional part into hex val
        dwFrac = (UINT32)( (sgTemp - dwMant) * 16 );

        dwMant = dwMant << 4;

        dwBrrReg = dwMant | dwFrac;

        gtUsartLookupArray[ePort].tPortCommon.ptUsart->BRR = dwBrrReg;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsartPortGetBaud( UsartPortEnum ePort, FLOAT *psgBaudRate )
{
    BOOL    fSuccess            = FALSE;
    FLOAT   sgTemp              = 0.0;
    UINT32  dwMant              = 0x00;
    UINT32  dwFrac              = 0x00;
    UINT32  dwBrrReg            = 0x00;

    UINT32  dwApbClock          = 0x00;
    RCC_ClocksTypeDef RCC_ClocksStatus;

    if( ePort < USART_ARRAY_MAX )
    {
        if( psgBaudRate != NULL )
        {
            // Get bus clock freq
            RCC_GetClocksFreq( &RCC_ClocksStatus );

            if
            (
                ( gtUsartLookupArray[ePort].tPortCommon.ptUsart == USART1 ) ||
                ( gtUsartLookupArray[ePort].tPortCommon.ptUsart == USART6 )
            )
            {
                dwApbClock = RCC_ClocksStatus.PCLK2_Frequency;
            }
            else
            {
                dwApbClock = RCC_ClocksStatus.PCLK1_Frequency;
            }

            // read baud rate register
            dwBrrReg = gtUsartLookupArray[ePort].tPortCommon.ptUsart->BRR;

            dwMant = dwBrrReg & 0xfff0;
            dwFrac = dwBrrReg & 0x000f;

            sgTemp = (dwMant >> 4);

            sgTemp = sgTemp + ( dwFrac / 16.0 );

            (*psgBaudRate) = dwApbClock / ( 16.0 * sgTemp );

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
BOOL UsartPortSetParity( UsartPortEnum ePort, UsartParityEnum eParity )
{
    BOOL    fSuccess            = FALSE;
    UINT32  dwReg2BitMask       = 0;
    UINT32  dwRegCurrentStat    = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( eParity < USART_PARITY_MAX )
        {
            dwReg2BitMask  = USART_CR1_REG_PARITY_CONTROL_ENABLE_MASK | USART_CR1_REG_PARITY_SELECTION_MASK;

            // invert mask to set all regs that are not related to the pin selected
            dwReg2BitMask  = ~dwReg2BitMask;

            dwRegCurrentStat = gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1;

            // clear pin reg
            dwRegCurrentStat = dwRegCurrentStat & dwReg2BitMask;

            // set new value for bits
            if( eParity == USART_PARITY_NON )
            {
                dwReg2BitMask  = 0;
            }
            if( eParity == USART_PARITY_EVEN )
            {
                dwReg2BitMask  = USART_CR1_REG_PARITY_CONTROL_ENABLE_MASK;
            }
            if( eParity == USART_PARITY_ODD )
            {
                dwReg2BitMask  = USART_CR1_REG_PARITY_CONTROL_ENABLE_MASK | USART_CR1_REG_PARITY_SELECTION_MASK;
            }

            // set new mode
            dwRegCurrentStat = ( dwRegCurrentStat | dwReg2BitMask );

            // set the reg
            gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 = dwRegCurrentStat;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsartPortGetParity( UsartPortEnum ePort, UsartParityEnum *peParity )
{
    BOOL    fSuccess  = FALSE;

    if( ePort < USART_ARRAY_MAX )
    {
        if( peParity != NULL )
        {
            if( (gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 & USART_CR1_REG_PARITY_CONTROL_ENABLE_MASK) > 0 )
            {
                // enabled. now find what type of parity is selected
                if( (gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 & USART_CR1_REG_PARITY_SELECTION_MASK) == 0 )
                {
                    // 0 = even
                    (*peParity) = USART_PARITY_EVEN;
                }
                else
                {
                    // 1 = ODD
                    (*peParity) = USART_PARITY_ODD;
                }
            }
            else
            {
                // disabled
                (*peParity) = USART_PARITY_NON;
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
BOOL UsartPortSetStopBit( UsartPortEnum ePort, UsartStopBitEnum eStopBit )
{
    BOOL    fSuccess            = FALSE;
    UINT32  dwReg2BitMask       = 0;
    UINT32  dwRegCurrentStat    = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( eStopBit < USART_STOP_BIT_MAX )
        {
            dwReg2BitMask  = USART_CR2_REG_STOP_BITS_MASK;

            // invert mask to set all regs that are not related to the pin selected
            dwReg2BitMask  = ~dwReg2BitMask;

            dwRegCurrentStat = gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR2;

            // clear pin reg
            dwRegCurrentStat = dwRegCurrentStat & dwReg2BitMask;

            // set value
            dwReg2BitMask = eStopBit << 12;

            // set new mode
            dwRegCurrentStat = ( dwRegCurrentStat | dwReg2BitMask );

            // set the reg
            gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR2 = dwRegCurrentStat;

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
BOOL UsartPortGetStopBit( UsartPortEnum ePort, UsartStopBitEnum *peStopBit )
{
    BOOL    fSuccess        = FALSE;
    UINT32  dwReg2BitMask   = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( peStopBit != NULL )
        {
            dwReg2BitMask = (gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR2 & USART_CR2_REG_STOP_BITS_MASK);

            // bit offset move to the start right of the uint32
            dwReg2BitMask = dwReg2BitMask >> 12;

            (*peStopBit) = dwReg2BitMask;

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
BOOL UsartPortSetWordLength( UsartPortEnum ePort, UsartWordLengthEnum eWordLen )
{
    BOOL    fSuccess            = FALSE;
    UINT32  dwReg1BitMask       = 0;
    UINT32  dwRegCurrentStat    = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( eWordLen < USART_WORD_LENGTH_MAX )
        {
            dwReg1BitMask  = USART_CR1_REG_M_MASK;

            // invert mask to set all regs that are not related to the pin selected
            dwReg1BitMask  = ~dwReg1BitMask;

            dwRegCurrentStat = gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1;

            // clear pin reg
            dwRegCurrentStat = dwRegCurrentStat & dwReg1BitMask;

            // set value
            dwReg1BitMask = eWordLen << 12;

            // set new mode
            dwRegCurrentStat = ( dwRegCurrentStat | dwReg1BitMask );

            // set the reg
            gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 = dwRegCurrentStat;

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
BOOL UsartPortGetWordLength( UsartPortEnum ePort, UsartWordLengthEnum *peWordLen )
{
    BOOL    fSuccess        = FALSE;
    UINT32  dwReg1BitMask   = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( peWordLen != NULL )
        {
            dwReg1BitMask = (gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 & USART_CR1_REG_M_MASK);

            // bit offset move to the start right of the uint32
            dwReg1BitMask = dwReg1BitMask >> 12;

            (*peWordLen) = dwReg1BitMask;

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
BOOL UsartPortSetMode( UsartPortEnum ePort, UsartModeEnum eMode )
{
    BOOL    fSuccess            = FALSE;
    UINT32  dwReg2BitMask       = 0;
    UINT32  dwRegCurrentStat    = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( eMode < USART_MODE_MAX )
        {
            dwReg2BitMask  = USART_CR1_REG_RXTX_MASK;

            // invert mask to set all regs that are not related to the pin selected
            dwReg2BitMask  = ~dwReg2BitMask;

            dwRegCurrentStat = gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1;

            // clear pin reg
            dwRegCurrentStat = dwRegCurrentStat & dwReg2BitMask;

            // set value
            dwReg2BitMask = eMode << 2;

            // set new mode
            dwRegCurrentStat = ( dwRegCurrentStat | dwReg2BitMask );

            // set the reg
            gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 = dwRegCurrentStat;

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
BOOL UsartPortGetMode( UsartPortEnum ePort, UsartModeEnum *peMode )
{
    BOOL    fSuccess        = FALSE;
    UINT32  dwReg2BitMask   = 0;

    if( ePort < USART_ARRAY_MAX )
    {
        if( peMode != NULL )
        {
            dwReg2BitMask = (gtUsartLookupArray[ePort].tPortCommon.ptUsart->CR1 & USART_CR1_REG_RXTX_MASK);

            // bit offset move to the start right of the uint32
            dwReg2BitMask = dwReg2BitMask >> 2;

            (*peMode) = dwReg2BitMask;

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
CHAR * UsartPortGetModeName( UsartModeEnum eMode )
{
    if( eMode < USART_MODE_MAX )
    {
        return &UsartModeNameArray[eMode][0];
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
CHAR * UsartPortGetWordLenName( UsartWordLengthEnum eWordLen )
{
    if( eWordLen < USART_WORD_LENGTH_MAX )
    {
        return &UsartWordLenNameArray[eWordLen][0];
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
CHAR * UsartPortGetStopBitName( UsartStopBitEnum eStopBit )
{
    if( eStopBit < USART_STOP_BIT_MAX )
    {
        return &UsartStopBitNameArray[eStopBit][0];
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
CHAR * UsartPortGetParityName( UsartParityEnum eParity )
{
    if( eParity < USART_PARITY_MAX )
    {
        return &UsartParityNameArray[eParity][0];
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
CHAR * UsartPortGetName( UsartPortEnum ePort )
{
    if( ePort < USART_ARRAY_MAX )
    {
        return &gtUsartLookupArray[ePort].pcPortName[0];
    }
    else
    {
        return NULL;
    }
}
