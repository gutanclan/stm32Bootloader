//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Gpio.c
//!    \brief       GPIO output/input control.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../inc/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../Utils/Types.h"
#include "../Targets/Target_CurrentTarget.h"

#include "gpio.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

static const CHAR * GpioAlternativeFunctionNameArray[GPIO_ALT_FUNC_MAX] =
{
	"SYS",
	"TIM1:2",
	"TIM3:5",
	"TIM8:11",
	"I2C1:3",
	"SPI1:2",
	"SPI3",
	"USART1:3",
	"USART4:6",
	"CAN1:2_TIM12:14",
	"OTGFS_OTGHS",
	"ETH",
	"FSMC_SDIO_OTGHS",
	"DCMI",
	"14_UNDEF",
	"EVENTOUT"
};

static const CHAR * GpioModeNameArray[GPIO_MODE_MAX] =
{
	"INPUT",
	"OUTPUT",
	"AF",
	"ANALOG",
};

static const CHAR * GpioOTypeNameArray[GPIO_OTYPE_MAX] =
{
	"P_PULL",
	"O_DRAIN",
};

static const CHAR * GpioSpeedNameArray[GPIO_OSPEED_MAX] =
{
	"2MHZ",
	"25MHZ",
	"50MHZ",
	"100MHZ",
};

static const CHAR * GpioPuPdNameArray[GPIO_PUPD_MAX] =
{
	"NO_PUPD",
	"P_UP",
	"P_DOWN",
};

static const CHAR * GpioLogicNameArray[GPIO_LOGIC_MAX] =
{
	"LOW",
	"HIGH",
};

static const GPIO_TypeDef * gpPortPointerArray[GPIO_PORT_MAX] =
{
	GPIOA,
	GPIOB,
	GPIOC,
	GPIOD,
	GPIOE,
};

static const UINT32 gdwPinNumberToBitOffsetArray[GPIO_PORT_PIN_AMOUNT] =
{
	GPIO_Pin_0,
	GPIO_Pin_1,
	GPIO_Pin_2,
	GPIO_Pin_3,
	GPIO_Pin_4,
	GPIO_Pin_5,
	GPIO_Pin_6,
	GPIO_Pin_7,
	GPIO_Pin_8,
	GPIO_Pin_9,
	GPIO_Pin_10,
	GPIO_Pin_11,
	GPIO_Pin_12,
	GPIO_Pin_13,
	GPIO_Pin_14,
	GPIO_Pin_15
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL GpioPortGetAhbx                 ( GpioPortEnum ePort, UINT32 *pdwPortAhbx );
static BOOL GpioModeSet						( GpioPortEnum ePort, UINT8 bPinNumber, GpioModeEnum eNewMode );

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL GpioModuleInit( void )
//!
//! \brief  Initialize GPIO variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModuleInit( void )
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
GPIO_TypeDef * GpioGetPortPointer( GpioPortEnum ePort )
{
	GPIO_TypeDef * ptPortX = NULL;

	if( ePort < GPIO_PORT_MAX )
	{
		ptPortX = (GPIO_TypeDef *)gpPortPointerArray[ePort];
	}

	return ptPortX;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioPortClockEnable( GpioPortEnum ePort, BOOL fEnable )
{
	//GPIO_TypeDef * 	ptPortX 	= NULL;
	UINT32          dwPortAhbx  = 0;
	BOOL			fSuccess 	= FALSE;
	FunctionalState NewState;

    if( GpioPortGetAhbx( ePort, &dwPortAhbx ) )
    {
        if( fEnable )
		{
			NewState = ENABLE;
		}
		else
		{
			NewState = DISABLE;
		}
		RCC_AHB1PeriphClockCmd( dwPortAhbx, NewState );

		fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioPortGetAhbx( GpioPortEnum ePort, UINT32 *pdwPortAhbx )
{
    BOOL fSuccess = FALSE;

    if( pdwPortAhbx != NULL )
    {
        switch(ePort)
        {
            case GPIO_PORT_A:
                (*pdwPortAhbx) = RCC_AHB1Periph_GPIOA;
                fSuccess = TRUE;
                break;
            case GPIO_PORT_B:
                (*pdwPortAhbx) = RCC_AHB1Periph_GPIOB;
                fSuccess = TRUE;
                break;
            case GPIO_PORT_C:
                (*pdwPortAhbx) = RCC_AHB1Periph_GPIOC;
                fSuccess = TRUE;
                break;
            case GPIO_PORT_D:
                (*pdwPortAhbx) = RCC_AHB1Periph_GPIOD;
                fSuccess = TRUE;
                break;
            case GPIO_PORT_E:
                (*pdwPortAhbx) = RCC_AHB1Periph_GPIOE;
                fSuccess = TRUE;
                break;

            case GPIO_PORT_MAX:
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
BOOL GpioPortClockIsEnabled( GpioPortEnum ePort )
{
	UINT32          dwPortAhbx  = 0;
	BOOL			fEnabled 	= FALSE;

    if( GpioPortGetAhbx( ePort, &dwPortAhbx ) )
    {
		if( ( RCC->AHB1ENR & dwPortAhbx ) > 0 )
		{
			fEnabled = TRUE;
		}
	}

	return fEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioOTypeSetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioOTypeEnum eOType )
{
	UINT32  dwReg1BitMask  = 0;
    UINT16  wOType          = 0;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( eOType < GPIO_OTYPE_MAX )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg1BitMask  = ( (UINT32)0x01 << (bPinNumber) );

				// invert mask to set all regs that are not related to the pin selected
				dwReg1BitMask  = ~dwReg1BitMask;

				wOType = ptPortX->OTYPER;

				// clear pin reg
				wOType = wOType & dwReg1BitMask;

				// set new mode
				wOType = wOType | ( (UINT32)eOType << (bPinNumber) );

				// set the reg
				ptPortX->OTYPER = wOType;

				fSuccess = TRUE;
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
BOOL GpioOTypeGetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioOTypeEnum *peOTypeResult )
{
    UINT32          dwReg1BitMask   = 0;
	BOOL	        fSuccess 	    = FALSE;
	GPIO_TypeDef * 	ptPortX         = NULL;

	if( peOTypeResult != NULL )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg1BitMask   = ( (UINT32)0x01 << (bPinNumber) );

                // get bits from reg
                dwReg1BitMask   = ( ptPortX->OTYPER & dwReg1BitMask );

                // adjust back pin offset
                dwReg1BitMask   = dwReg1BitMask >> (bPinNumber);

				(*peOTypeResult)= dwReg1BitMask;

				fSuccess = TRUE;
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
CHAR * 	GpioOTypeGetName( GpioOTypeEnum eOType )
{
	if( eOType < GPIO_OTYPE_MAX )
	{
		return (CHAR *)GpioOTypeNameArray[eOType];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioPuPdSetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioPuPdEnum ePuPd )
{
	UINT32  dwReg2BitMask;
    UINT32  dwPuPdCurrentStat;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( ePuPd < GPIO_PUPD_MAX )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg2BitMask  = ( (UINT32)0x03 << (bPinNumber * 2) );

				// invert mask to set all regs that are not related to the pin selected
				dwReg2BitMask  = ~dwReg2BitMask;

				dwPuPdCurrentStat = ptPortX->PUPDR;

				// clear pin reg
				dwPuPdCurrentStat = dwPuPdCurrentStat & dwReg2BitMask;

				// set new mode
				dwPuPdCurrentStat = dwPuPdCurrentStat | ( (UINT32)ePuPd << (bPinNumber * 2) );

				// set the reg
				ptPortX->PUPDR = dwPuPdCurrentStat;

				fSuccess = TRUE;
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
BOOL GpioPuPdGetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioPuPdEnum  *pePuPdResult )
{
	UINT32  dwReg2BitMask;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( pePuPdResult != NULL )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg2BitMask   = ( (UINT32)0x03 << (bPinNumber * 2) );

                // get bits from reg
                dwReg2BitMask   = (ptPortX->PUPDR & dwReg2BitMask);

                // adjust back pin offset
                dwReg2BitMask   = dwReg2BitMask >> (bPinNumber * 2);

				(*pePuPdResult) = dwReg2BitMask;

				fSuccess = TRUE;
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
CHAR * GpioPuPdGetName( GpioPuPdEnum ePuPd )
{
	if( ePuPd < GPIO_PUPD_MAX )
	{
		return (CHAR *)GpioPuPdNameArray[ePuPd];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioSpeedSetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioOSpeedEnum eSpeed )
{
	UINT32  dwReg2BitMask;
    UINT32  dwSpeedCurrentStat;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( eSpeed < GPIO_OSPEED_MAX )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg2BitMask  = ( (UINT32)0x03 << (bPinNumber * 2) );

				// invert mask to set all regs that are not related to the pin selected
				dwReg2BitMask  = ~dwReg2BitMask;

				dwSpeedCurrentStat = ptPortX->OSPEEDR;

				// clear pin reg
				dwSpeedCurrentStat = dwSpeedCurrentStat & dwReg2BitMask;

				// set new mode
				dwSpeedCurrentStat = dwSpeedCurrentStat | ( (UINT32)eSpeed << (bPinNumber * 2) );

				// set the reg
				ptPortX->OSPEEDR = dwSpeedCurrentStat;

				fSuccess = TRUE;
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
BOOL GpioSpeedGetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioOSpeedEnum *peSpeed )
{
	UINT32  dwReg2BitMask;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( peSpeed != NULL )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg2BitMask  = ( (UINT32)0x03 << (bPinNumber * 2) );

                // get bits from reg
                dwReg2BitMask  = (ptPortX->OSPEEDR & dwReg2BitMask);

                // adjust back pin offset
                dwReg2BitMask   = dwReg2BitMask >> (bPinNumber * 2);

				(*peSpeed) = dwReg2BitMask;

				fSuccess = TRUE;
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
CHAR * 	GpioSpeedGetName( GpioOSpeedEnum eSpeed )
{
	if( eSpeed < GPIO_OSPEED_MAX )
	{
		return (CHAR *)GpioSpeedNameArray[eSpeed];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioInputRegRead( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum *peLogic )
{
    BOOL fSuccess  = FALSE;
    GPIO_TypeDef * 	ptPortX 	= NULL;

	if( NULL != peLogic )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				(*peLogic) = GPIO_ReadInputDataBit(  ptPortX, gdwPinNumberToBitOffsetArray[bPinNumber] );

				fSuccess = TRUE;
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
BOOL GpioOutputRegRead( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum *peLogic )
{
	BOOL fSuccess  = FALSE;
    GPIO_TypeDef * 	ptPortX 	= NULL;

	if( NULL != peLogic )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				(*peLogic) = (GpioLogicEnum) GPIO_ReadOutputDataBit( ptPortX, gdwPinNumberToBitOffsetArray[bPinNumber] );

				fSuccess = TRUE;
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
BOOL GpioOutputRegWrite( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum eLogic )
{
	BOOL fSuccess  = FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	if( eLogic < GPIO_LOGIC_MAX )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				GPIO_WriteBit( ptPortX, gdwPinNumberToBitOffsetArray[bPinNumber], eLogic );

				fSuccess = TRUE;
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
CHAR * GpioLogicGetName( GpioLogicEnum eLogic )
{
	if( eLogic < GPIO_LOGIC_MAX )
	{
		return (CHAR *)GpioLogicNameArray[eLogic];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModeGetVal( GpioPortEnum ePort, UINT8 bPinNumber, GpioModeEnum *peModeResult )
{
    UINT32  dwReg2BitMask;
	BOOL	fSuccess 		= FALSE;
	GPIO_TypeDef * 	ptPortX 	= NULL;


	if( peModeResult != NULL )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
				dwReg2BitMask  = ( (UINT32)0x03 << (bPinNumber * 2) );

				(*peModeResult) = ( ptPortX->MODER & dwReg2BitMask ) >> (bPinNumber * 2);

				fSuccess = TRUE;
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
CHAR * GpioModeGetName( GpioModeEnum eMode )
{
	if( eMode < GPIO_MODE_MAX )
	{
		return (CHAR *)GpioModeNameArray[eMode];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModeSet( GpioPortEnum ePort, UINT8 bPinNumber, GpioModeEnum eNewMode )
{
	BOOL 			fSuccess 	= FALSE;
	UINT32  		dwReg2BitMask;
    UINT32  		dwModeCurrentStat;
	GPIO_TypeDef * 	ptPortX 	= NULL;

	ptPortX = GpioGetPortPointer(ePort);

	if( ptPortX != NULL )
	{
		if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
		{
			if( eNewMode < GPIO_MODE_MAX )
			{
				dwReg2BitMask  = ( (UINT32)0x03 << (bPinNumber * 2) );

				// invert mask to set all regs that are not related to the pin selected
				dwReg2BitMask  = ~dwReg2BitMask;

				dwModeCurrentStat = ptPortX->MODER;

				// clear pin reg
				dwModeCurrentStat = dwModeCurrentStat & dwReg2BitMask;

				// set new mode
				dwModeCurrentStat = dwModeCurrentStat | ( (UINT32)eNewMode << (bPinNumber * 2) );

				// set the reg
				ptPortX->MODER = dwModeCurrentStat;

				fSuccess = TRUE;
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
BOOL GpioModeSetInput( GpioPortEnum ePort, UINT8 bPinNumber )
{
    BOOL fSuccess = FALSE;

    fSuccess = GpioModeSet( ePort, bPinNumber, GPIO_MODE_INPUT );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModeSetOutput( GpioPortEnum ePort, UINT8 bPinNumber )
{
    BOOL fSuccess = FALSE;

    fSuccess = GpioModeSet( ePort, bPinNumber, GPIO_MODE_OUTPUT );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModeSetAnalog( GpioPortEnum ePort, UINT8 bPinNumber )
{
    BOOL fSuccess = FALSE;

    fSuccess = GpioModeSet( ePort, bPinNumber, GPIO_MODE_ANALOG );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioModeSetAf( GpioPortEnum ePort, UINT8 bPinNumber, GpioAlternativeFunctionEnum eAfType )
{
    BOOL fSuccess = FALSE;
    UINT32  dwReg4BitMask;
    UINT32  dwCurrentStat;
    GPIO_TypeDef * 	ptPortX 	= NULL;

	ptPortX = GpioGetPortPointer(ePort);

    if( ptPortX != NULL )
    {
        if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
        {
            if( eAfType < GPIO_ALT_FUNC_MAX )
            {
                // set alternative function mode
                fSuccess = GpioModeSet( ePort, bPinNumber, GPIO_MODE_AF );

                if( fSuccess )
                {
                    if( bPinNumber < 8 )
                    {
                        // modify dwAFRL

                        dwReg4BitMask  = ( (UINT32)0x0F << (bPinNumber * 4) );

                        // invert mask to set all regs that are not related to the pin selected
                        dwReg4BitMask  = ~dwReg4BitMask;

                        dwCurrentStat = ptPortX->AFR[0];

                        // clear pin reg
                        dwCurrentStat = dwCurrentStat & dwReg4BitMask;

                        // set new mode
                        dwCurrentStat = dwCurrentStat | ( (UINT32)eAfType << (bPinNumber * 4) );

                        // set the reg
                        ptPortX->AFR[0] = dwCurrentStat;
                    }
                    else{
                        // modify dwAFRH

                        dwReg4BitMask  = ( (UINT32)0x0F << ( (bPinNumber%8) * 4) );

                        // invert mask to set all regs that are not related to the pin selected
                        dwReg4BitMask  = ~dwReg4BitMask;

                        dwCurrentStat = ptPortX->AFR[1];

                        // clear pin reg
                        dwCurrentStat = dwCurrentStat & dwReg4BitMask;

                        // set new mode
                        dwCurrentStat = dwCurrentStat | ( (UINT32)eAfType << ((bPinNumber%8) * 4) );

                        // set the reg
                        ptPortX->AFR[1] = dwCurrentStat;
                    }
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
BOOL GpioAfGetType( GpioPortEnum ePort, UINT8 bPinNumber, GpioAlternativeFunctionEnum *peAfType )
{
    BOOL fSuccess = FALSE;
    GPIO_TypeDef * 	ptPortX 	= NULL;
    UINT32 dwReg4BitMask;

    if( peAfType != NULL )
	{
		ptPortX = GpioGetPortPointer(ePort);

		if( ptPortX != NULL )
		{
			if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
			{
			    if( (bPinNumber) < 8 )
                {
                    dwReg4BitMask  = ( (UINT32)0x0F << (bPinNumber * 4) );

                    // get bits from reg
                    dwReg4BitMask   = (ptPortX->AFR[0] & dwReg4BitMask);

                    // adjust back pin offset
                    dwReg4BitMask   = dwReg4BitMask >> (bPinNumber * 4);

                    (*peAfType) = dwReg4BitMask;

                    fSuccess = TRUE;
                }
                else{

                    dwReg4BitMask  = ( (UINT32)0x0F << ((bPinNumber%8) * 4) );

                    // get bits from reg
                    dwReg4BitMask   = (ptPortX->AFR[1] & dwReg4BitMask);

                    // adjust back pin offset
                    dwReg4BitMask   = dwReg4BitMask >> ((bPinNumber%8) * 4);

                    (*peAfType) = dwReg4BitMask;

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
CHAR* GpioAfGetName( GpioAlternativeFunctionEnum eAfType )
{
	if( eAfType < GPIO_ALT_FUNC_MAX )
	{
		return (CHAR*)GpioAlternativeFunctionNameArray[eAfType];
	}
	else
	{
		return (CHAR*)"UNKNOWN";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GpioConfigSet( GpioPortEnum ePort, UINT8 bPinNumber, const GpioConfigType * const peConfig )
{
    BOOL fSuccess = FALSE;

    if( ePort < GPIO_PORT_MAX )
    {
        if( bPinNumber < GPIO_PORT_PIN_AMOUNT )
        {
            if( peConfig != NULL )
            {
                fSuccess = TRUE;
                fSuccess &= (peConfig->ePupd < GPIO_ALT_FUNC_MAX);
                fSuccess &= (peConfig->eSpeed < GPIO_ALT_FUNC_MAX);
                fSuccess &= (peConfig->eOType < GPIO_ALT_FUNC_MAX);
                fSuccess &= (peConfig->eMode < GPIO_ALT_FUNC_MAX);
                fSuccess &= (peConfig->eOptionalAf < GPIO_ALT_FUNC_MAX);

                if( fSuccess )
                {
                    GpioOTypeSetVal( ePort, bPinNumber, peConfig->eOType );
                    GpioPuPdSetVal( ePort, bPinNumber, peConfig->ePupd );
                    GpioSpeedSetVal( ePort, bPinNumber, peConfig->eSpeed );

                    switch( peConfig->eMode )
                    {
                        case GPIO_MODE_INPUT:
                            GpioModeSetInput( ePort, bPinNumber );
                            break;
                        case GPIO_MODE_OUTPUT:
                            GpioModeSetOutput( ePort, bPinNumber );
                            break;
                        case GPIO_MODE_ANALOG:
                            GpioModeSetAnalog( ePort, bPinNumber );
                            break;
                        case GPIO_MODE_AF:
                            GpioModeSetAf( ePort, bPinNumber, peConfig->eOptionalAf );
                            break;
                        default:
                            fSuccess = FALSE;
                            break;
                    }
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
void GpioTest( void )
{
    GpioModuleInit();

    GpioPortClockEnable( GPIO_PORT_D , TRUE );
    GpioOTypeSetVal( GPIO_PORT_D, 12, GPIO_OTYPE_PUSH_PULL );
    GpioPuPdSetVal( GPIO_PORT_D, 12, GPIO_PUPD_NOPUPD );
    GpioSpeedSetVal( GPIO_PORT_D, 12, GPIO_OSPEED_2MHZ );
    GpioModeSetOutput( GPIO_PORT_D, 12 );


    UINT32 dwDelay = 0;
    dwDelay = 1000000;

    while(1)
    {
        GpioOutputRegWrite( GPIO_PORT_D, 12, GPIO_LOGIC_LOW );

        dwDelay = 1000000;
        while( dwDelay-- ){}

        GpioOutputRegWrite( GPIO_PORT_D, 12, GPIO_LOGIC_HIGH );

        dwDelay = 1000000;
        while( dwDelay-- ){}
    }
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
