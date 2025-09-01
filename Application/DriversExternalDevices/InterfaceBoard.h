//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        InterfaceBoard.h
//!    \brief       Interface Board Wrapper GPIO output/input control module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _INTERFACE_BOARD_H_
#define _INTERFACE_BOARD_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "Types.h"
#include "Console.h"
#include "stm32f2xx_gpio.h"
#include "Gpio.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: the following pins are not listed to limit the control over them.     
    //       This is done to avoid having control from different modules since 
    //       the control of the gpios can turn to be confusing.
    // ADC module
    /* INT_B_PIN_EXT_ADC_TERTIARY*/ 
    /* INT_B_PIN_EXT_ADC_PRIMARY*/  
    /* INT_B_PIN_EXT_ADC_SECONDARY*/

    // USART module
    /* INT_B_PIN_EXT_USART1_RX*/    
    /* INT_B_PIN_EXT_UART6_TX*/     
    /* INT_B_PIN_EXT_UART1_TX*/     
    /* INT_B_PIN_EXT_UART6_RX*/  
typedef enum
{            
#if defined(BUILD_TARGET_AFTI_MAINBOARD)    
//   
//	INT_B_PIN_EXT_ADC_TERTIARY,     // INT_B_12V_BAT_SCALED
	/*INT_B_PIN_EXT_GPIO_PA2*/      INT_B_12V_POwER_ENABLE  = 0, 
    /*INT_B_PIN_WKUP_INT*/          INT_B_BUTTON_REED_1,    
    /*INT_B_PIN_EXT_GPIO_PD14*/     INT_B_EXT_GPIO_PD14,
//    INT_B_PIN_EXT_USART1_RX*/     INT_B_EXT_UART1_RX,
    /*INT_B_PIN_EXT_GPIO_PD1*/      INT_B_BUTTON_REED_2,
    /*INT_B_PIN_EXT_GPIO_PE9*/      INT_B_EXT_GPIO_PE9,
    /*INT_B_PIN_EXT_GPIO_PE13*/     INT_B_PWR_SCALED_ADC_ENABLE,

    /*INT_B_PIN_EXT_GPIO_PA4*/      INT_B_SPI_CS_ADC,
    /*INT_B_PIN_EXT_GPIO_PB1*/      INT_B_ANALOG_CH_3_ENABLE,
    /*INT_B_PIN_EXT_SPI_MOSI*/      INT_B_EXT_SPI_MOSI,
    /*INT_B_PIN_EXT_GPIO_PB9*/      INT_B_ANALOG_CH_4_ENABLE,
    /*INT_B_PIN_EXT_GPIO_PB7*/      INT_B_ANALOG_CH_1_ENABLE,
//    INT_B_PIN_EXT_UART6_TX*/      INT_B_EXT_UART6_TX,
    /*INT_B_PIN_EXT_GPIO_PA11*/     INT_B_SPI_CS_PORTEX,
    /*INT_B_PIN_EXT_GPIO_PC11*/     INT_B_CAMERA_SEL_A_B,

        
//    INT_B_PIN_EXT_ADC_PRIMARY*/   INT_B_IS_12V_BAT_SCALED,
//    INT_B_PIN_EXT_ADC_SECONDARY*/ INT_B_SOLAR_IN_SCALED,
    /*INT_B_PIN_EXT_GPIO_PA3*/      INT_B_2V5_PWR_ENABLE,
    /*INT_B_PIN_POWER_CONTROL*/     INT_B_3V3_PWR_ENABLE,
    /*INT_B_PIN_EXT_GPIO_PD15*/     INT_B_ANALOG_CH_5_ENABLE,
//    INT_B_PIN_EXT_UART1_TX*/      INT_B_EXT_UART1_TX,
#if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
    /*INT_B_EXT_GPIO_PB5*/          INT_B_EXT_GPIO_PB5,
#else
    /*INT_B_PIN_CHARGE_CONNECTED*/  INT_B_EXT_GPIO_PB5,
#endif
    /*INT_B_PIN_PWR_CTRL_ON_OFF*/   INT_B_EXT_GPIO_PD0,
    /*INT_B_PIN_EXT_GPIO_PE11*/     INT_B_CAM_RS422_DRIVER_OUT_ENABLE,
    /*INT_B_PIN_EXT_GPIO_PE14*/     INT_B_CAM_RS422_RECEIVER_OUT_ENABLE,

    /*INT_B_PIN_EXT_GPIO_PB0*/      INT_B_ANALOG_CH_6_ENABLE,
    /*INT_B_PIN_EXT_SPI_CLK*/       INT_B_EXT_SPI_CLK,
    /*INT_B_PIN_EXT_SPI_MISO*/      INT_B_EXT_SPI_MISO,
    /*INT_B_PIN_EXT_GPIO_PB8*/      INT_B_ANALOG_CH_2_ENABLE,
    /*INT_B_PIN_EXT_GPIO_PB6*/      INT_B_ANALOG_CH_CONFIG_B,
//    INT_B_PIN_EXT_UART6_RX*/      INT_B_EXT_UART6_RX,
    /*INT_B_PIN_EXT_GPIO_PA12*/     INT_B_ANALOG_CH_CONFIG_A,
    /*INT_B_PIN_EXT_GPIO_PC10*/     INT_B_CAMERA_ENABLE,
    
#endif    
    INT_B_PIN_ENUM_TOTAL,	// Total Number of Interface board GPIO pins defined    
}IntBoardGpioPinEnum;

typedef enum
{
    INT_B_ADC_PWR_RAIL_PRIMARY    = 0,
    INT_B_ADC_PWR_RAIL_SECONDARY,
    INT_B_ADC_PWR_RAIL_TERTIARY,    

    INT_B_ADC_PWR_RAIL_ENUM_TOTAL,	
}IntBoardAdcPowerRailEnum;


//////////////////////////////////////////////////////////////////////////////////////////////////
// Exported Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL 	IntBoardInit            ( void );
void 	IntBoardEnable          ( BOOL fEnable );
BOOL 	IntBoardIsEnabled       ( void );

// interface board GPIO
// this functions only allow to set pin as INPUT or OUTPUT.
// this functions doesn't control pins form the MAX6957 (Use PortEx.h instead)
BOOL    IntBoardGpioSetPinMode  ( const IntBoardGpioPinEnum ePinName, const GpioModeEnum eMode, const LOGIC eLogic, GpioOutputTypeEnum eOutputType );
BOOL 	IntBoardGpioGetPinMode	( const IntBoardGpioPinEnum ePinName, GpioModeEnum *eMode );
BOOL    IntBoardGpioOutputWrite ( const IntBoardGpioPinEnum ePinName, const BOOL fLogicLevel );
BOOL    IntBoardGpioPinRead     ( const IntBoardGpioPinEnum ePinName, BOOL *pfLogicLevel );

// interface board ADC 
// this functions doesn't control pins form the AD7795 (Use AdcEx.h instead)
BOOL    IntBoardAdcPwrRailRead  ( const IntBoardAdcPowerRailEnum eAdcPwrRail, UINT16 *pwAdcReadingRaw, SINGLE *psgAdcResultMilliVolts );
    
// interface board ADC_EXT (Adc extension port)
// use AdcEx 

// interface board GPIO_EXT (Gpio extension port)
// use PortEx

// interface board CAM
// Not implemented yet!
void    IntBoardCamEnable       ( BOOL fEnable );

// WARNING! now has only support for 1 camera
BOOL    IntBoardIsCamEnabled    ( void );

// WARNING!! Gpio Argument is an Enum from gpio module
BOOL    IntBoardIsGpioOnIntBoard( const GpioPinNamesEnum eGpioPin );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _INTERFACE_BOARD_H_





