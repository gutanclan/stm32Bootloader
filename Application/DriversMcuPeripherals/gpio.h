//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Gpio.h
//!    \brief       GPIO output/input control module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _GPIO_H_
#define _GPIO_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
 // Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

// GPIO Alternative Function
typedef enum
{
    // ALTERNATIVE FUNCTIONS
    GPIO_ALT_FUNC_SYS = 0,
    GPIO_ALT_FUNC_TIM1_TIM2,
    GPIO_ALT_FUNC_TIM3_TIM5,
    GPIO_ALT_FUNC_TIM8_TIM11,
    GPIO_ALT_FUNC_I2C1_I2C3,
    GPIO_ALT_FUNC_SPI1_SPI2,
    GPIO_ALT_FUNC_SPI3,
    GPIO_ALT_FUNC_USART1_USART3,
    GPIO_ALT_FUNC_USART4_USART6,
    GPIO_ALT_FUNC_CAN1_CAN2_TIM12_TIM14,
    GPIO_ALT_FUNC_OTGFS_OTGHS,
    GPIO_ALT_FUNC_ETH,
    GPIO_ALT_FUNC_FSMC_SDIO_OTGHS,
    GPIO_ALT_FUNC_DCMI,
    GPIO_ALT_FUNC_14_UNDEFINED,
    GPIO_ALT_FUNC_EVENTOUT,

    GPIO_ALT_FUNC_MAX,
}GpioAlternativeFunctionEnum;

#define GPIO_ALT_FUNC_UNKNOWN     (GPIO_ALT_FUNC_MAX)

// GPIO CONFIGURATIONS
typedef enum
{
    GPIO_MODE_INPUT		    = 0,
	GPIO_MODE_OUTPUT	    = 1,
	GPIO_MODE_AF		    = 2,
	GPIO_MODE_ANALOG        = 3,

    GPIO_MODE_MAX,
}GpioModeEnum;

typedef enum
{
    GPIO_OTYPE_PUSH_PULL    = 0,
	GPIO_OTYPE_OPEN_DRAIN   = 1,

    GPIO_OTYPE_MAX,
}GpioOTypeEnum;

typedef enum
{
    GPIO_OSPEED_2MHZ        = 0,
	GPIO_OSPEED_25MHZ       = 1,
	GPIO_OSPEED_50MHZ       = 2,
	GPIO_OSPEED_100MHZ      = 3,    // on 30 uF. Read notes on datasheet for more info.

    GPIO_OSPEED_MAX,
}GpioOSpeedEnum;

typedef enum
{
    GPIO_PUPD_NOPUPD        = 0,
	GPIO_PUPD_P_UP          = 1,
	GPIO_PUPD_P_DOWN        = 2,

    GPIO_PUPD_MAX
}GpioPuPdEnum;

typedef enum
{
    GPIO_LOGIC_LOW	        = 0,
    GPIO_LOGIC_HIGH         = 1,

    GPIO_LOGIC_MAX
}GpioLogicEnum;

// exception case used when pin is input instead of output
#define GPIO_LOGIC_NOT_REQUIRED     (GPIO_LOGIC_MAX)

typedef struct
{
    GpioPuPdEnum                ePupd;
    GpioOSpeedEnum              eSpeed;
    GpioOTypeEnum               eOType;
    GpioModeEnum                eMode;
    GpioAlternativeFunctionEnum eOptionalAf;
}GpioConfigType;

typedef enum
{
    GPIO_PORT_A     = 0,
	GPIO_PORT_B     = 1,
    GPIO_PORT_C		= 2,
    GPIO_PORT_D     = 3,
    GPIO_PORT_E     = 4,

    GPIO_PORT_MAX   = 5,
}GpioPortEnum;

#define GPIO_PORT_INVALID		    (GPIO_PORT_MAX)

#define GPIO_PORT_PIN_AMOUNT		(16)
#define GPIO_PIN_INVALID		    (GPIO_PORT_PIN_AMOUNT)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL 	GpioModuleInit				( void );   // this function should enable all gpio rcc clocks..so they can be read write

BOOL 	GpioPortClockEnable         ( GpioPortEnum ePort, BOOL fEnable );
BOOL 	GpioPortClockIsEnabled      ( GpioPortEnum ePort );

BOOL 	GpioOTypeSetVal            	( GpioPortEnum ePort, UINT8 bPinNumber, GpioOTypeEnum eOType );
BOOL 	GpioOTypeGetVal            	( GpioPortEnum ePort, UINT8 bPinNumber, GpioOTypeEnum *peOTypeResult );
CHAR * 	GpioOTypeGetName           	( GpioOTypeEnum eOType );

BOOL 	GpioPuPdSetVal             	( GpioPortEnum ePort, UINT8 bPinNumber, GpioPuPdEnum ePuPd );
BOOL 	GpioPuPdGetVal             	( GpioPortEnum ePort, UINT8 bPinNumber, GpioPuPdEnum  *pePuPdResult );
CHAR * 	GpioPuPdGetName            	( GpioPuPdEnum ePuPd );

BOOL 	GpioSpeedSetVal            	( GpioPortEnum ePort, UINT8 bPinNumber, GpioOSpeedEnum eSpeed );
BOOL 	GpioSpeedGetVal            	( GpioPortEnum ePort, UINT8 bPinNumber, GpioOSpeedEnum  *peSpeed );
CHAR * 	GpioSpeedGetName           	( GpioOSpeedEnum eSpeed );

BOOL 	GpioInputRegRead			( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum *peLogic );
BOOL 	GpioOutputRegRead       	( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum *peLogic );
BOOL 	GpioOutputRegWrite      	( GpioPortEnum ePort, UINT8 bPinNumber, GpioLogicEnum eLogic );
CHAR * 	GpioLogicGetName           	( GpioLogicEnum eLogic );

BOOL 	GpioModeGetVal             	( GpioPortEnum ePort, UINT8 bPinNumber, GpioModeEnum *peModeResult );
CHAR * 	GpioModeGetName           	( GpioModeEnum eMode );

BOOL 	GpioModeSetInput            ( GpioPortEnum ePort, UINT8 bPinNumber );
BOOL 	GpioModeSetOutput           ( GpioPortEnum ePort, UINT8 bPinNumber );
BOOL 	GpioModeSetAnalog           ( GpioPortEnum ePort, UINT8 bPinNumber );
BOOL 	GpioModeSetAf               ( GpioPortEnum ePort, UINT8 bPinNumber, GpioAlternativeFunctionEnum eAfType );
CHAR*   GpioAfGetName     			( GpioAlternativeFunctionEnum eAfType );
BOOL    GpioAfGetType               ( GpioPortEnum ePort, UINT8 bPinNumber, GpioAlternativeFunctionEnum *peAfType );

BOOL    GpioConfigSet               ( GpioPortEnum ePort, UINT8 bPinNumber, const GpioConfigType * const peConfig );

void    GpioTest                    ( void );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _GPIO_B_H_




