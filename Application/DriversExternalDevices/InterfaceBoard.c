//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        InterfaceBoard.c
//!    \brief       Interface Board Wrapper GPIO output/input control module header.
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <string.h>
#include <stdio.h>

// Rowley includes
#include "Assert.h"

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// RTOS Library includes

// PCOM General Library includes
#include "Types.h"
#include "Target.h"             // Hardware target definitions

#include "Timer.h"
#include "Gpio.h"
#include "Adc.h"
#include "Usart.h"
#include "PortEx.h"
#include "Spi.h"
#include "AdcExternal.h"
#include "InterfaceBoard.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// remapping  array from interface board enum to GPIO enum
static GpioPinNamesEnum gtInterfaceBoardLookupArray[INT_B_PIN_ENUM_TOTAL] =
{
    // Inteface board enum              gpio enum    

//	  INT_B_PIN_EXT_ADC_TERTIARY,
	/* INT_B_PIN_EXT_GPIO_PA2*/         GPIO_EXT_GPIO_PA2,
    /* INT_B_PIN_WKUP_INT*/             GPIO_WAKEUP_INT,
    /* INT_B_PIN_EXT_GPIO_PD14*/        GPIO_EXT_GPIO_PD14,    
//    INT_B_PIN_EXT_USART1_RX,
    /* INT_B_PIN_EXT_GPIO_PD1*/         GPIO_EXT_GPIO_PD1,
    /* INT_B_PIN_EXT_GPIO_PE9*/         GPIO_EXT_GPIO_PE9, 
    /* INT_B_PIN_EXT_GPIO_PE13*/        GPIO_EXT_GPIO_PE13,

    /* INT_B_PIN_EXT_GPIO_PA4*/         GPIO_EXT_GPIO_PA4, 
    /* INT_B_PIN_EXT_GPIO_PB1*/         GPIO_EXT_GPIO_PB1,
    /* INT_B_PIN_EXT_SPI_MOSI*/         GPIO_PIN_SPI2_MOSI,
    /* INT_B_PIN_EXT_GPIO_PB9*/         GPIO_EXT_GPIO_PB9,
    /* INT_B_PIN_EXT_GPIO_PB7*/         GPIO_EXT_GPIO_PB7,    
//    INT_B_PIN_EXT_UART6_TX,
    /* INT_B_PIN_EXT_GPIO_PA11*/        GPIO_EXT_GPIO_PA11,
    /* INT_B_PIN_EXT_GPIO_PC11*/        GPIO_EXT_GPIO_PC11,

//    INT_B_PIN_EXT_ADC_PRIMARY,
//    INT_B_PIN_EXT_ADC_SECONDARY,          
    /* INT_B_PIN_EXT_GPIO_PA3*/         GPIO_EXT_GPIO_PA3,
    /* INT_B_PIN_POWER_CONTROL*/        GPIO_POWER_CONTROL,
    /* INT_B_PIN_EXT_GPIO_PD15*/        GPIO_EXT_GPIO_PD15,    
//    INT_B_PIN_EXT_UART1_TX,
#if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
    /*INT_B_EXT_GPIO_PB5  */            GPIO_EXT_GPIO_PB5,
#else
    /* INT_B_PIN_CHARGE_CONNECTED*/     GPIO_CHARGE_CONNECTED,
#endif
    /* INT_B_PIN_PWR_CTRL_ON_OFF*/      GPIO_PWR_CTRL_ON_OFF,
    /* INT_B_PIN_EXT_GPIO_PE11*/        GPIO_EXT_GPIO_PE11,
    /* INT_B_PIN_EXT_GPIO_PE14*/        GPIO_EXT_GPIO_PE14,

    /* INT_B_PIN_EXT_GPIO_PB0*/         GPIO_EXT_GPIO_PB0,
    /* INT_B_PIN_EXT_SPI_CLK*/          GPIO_PIN_SPI2_CLK,
    /* INT_B_PIN_EXT_SPI_MISO*/         GPIO_PIN_SPI2_MOSI,
    /* INT_B_PIN_EXT_GPIO_PB8*/         GPIO_EXT_GPIO_PB8,
    /* INT_B_PIN_EXT_GPIO_PB6*/         GPIO_EXT_GPIO_PB6,    
//    INT_B_PIN_EXT_UART6_RX,
    /* INT_B_PIN_EXT_GPIO_PA12*/        GPIO_EXT_GPIO_PA12,
    /* INT_B_PIN_EXT_GPIO_PC10*/        GPIO_EXT_GPIO_PC10
};
    
#define INT_B_NUMBER_OF_PINS     ( sizeof(gtInterfaceBoardLookupArray) / sizeof( gtInterfaceBoardLookupArray[0] ) )


static BOOL gfIsIntBoardEnabled = FALSE;
static BOOL gfIsCamEnabled = FALSE;

///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBInit( void )
//!
//! \brief  Initializes all the Interface board gpios as inputs if they havent been initialized by 
//!         the GPIO module.
//!
//! \return BOOLEAN
//!         - TRUE if operation successfully
//!         - FALSE if error on the operation
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBInit( void )
{
    BOOL fSuccess       = TRUE;
    BOOL fInitResult;

    if( INT_B_NUMBER_OF_PINS != INT_B_PIN_ENUM_TOTAL )
    {
        // GPIO pin definition error
        assert( 0 );
    }

    // init gpio pins as inputs if they haven't been initialized.
    for( UINT16 counter = 0 ; counter < INT_B_NUMBER_OF_PINS ; counter++ )
    {
        fInitResult    = FALSE;

        GpioIsPinInit( gtInterfaceBoardLookupArray[counter], &fInitResult );

        if( FALSE == fInitResult )
        {
            // initialize by default as input.            
            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[counter], GPIO_MODE_INPUT, 0, GPIO_OUTPUT_TYPE_PUSH_PULL );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void IntBoardEnable( BOOL fEnable )
//!
//! \brief  enable power lines in the interface board and set pins to initial state depending on their use
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void IntBoardEnable( BOOL fEnable )
{
    BOOL fSuccess       = FALSE;    

    if( fEnable )
    {        
        if( gfIsIntBoardEnabled == FALSE )
        {
            /////////////////////////////////////////////////////
            // before enabling 3v3, make sure other pins are in a desired initial OFF state        

            // 3v3 power line asserted
            // ###########################################################
            // ###########################################################
            // pin asserted
            GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_3V3_PWR_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
            TimerTaskDelayMs( 100 ); 
            // ###########################################################
            // ###########################################################

            // other power lines
            GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_2V5_PWR_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );     
            // NOTE:  This regulator takes from 7ms up to 12ms to stabilize

            // this should only be done if the configuration type for the channels requires it. 
            // For example.. channel with config mode rtd doenst require 12 v in the sensor power to take a reading.                
            // make sure 12 v power line is not enabled 
            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_12V_POwER_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );       

            // config spi 
            // chip selects first            
            // pin output with open drain. low = ground, high = resistor pull (up or down)        
//            GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_PORTEX], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );       
//            GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_ADC],    GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );       

            // set clck mosi and miso as alternative functions
            SpiBusPowerOn( SPI_BUS_2, TRUE );        

            // adc ex
            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_CONFIG_A],               GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_CONFIG_B],               GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );

            // unasserted pins
            // change open drain pin config to push pull to drive hard
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_1_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_2_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_3_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_4_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_5_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//            fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_6_ENABLE],               GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
        
            // port ex
                // uses only spi
            // configure pins as output low
            PortExSetInitialPinState();
        
            // checks if is possible to talk to the chip
            AdcSetStartUpState();
            /////////////////////////////////////////////////////        

            gfIsIntBoardEnabled = TRUE;
        }
    }
    else
    {        
        // first power down cam in case is enabled
        // this should take care of turning off usart

        IntBoardCamEnable( FALSE );
        // camera related pin stats        
        // usart are inputs
        // Cam EN .. FLOATING(open drain)...with pull up
        // 422 rec EN ..FLOATING(open drain)...with pull up        
        // 422 driv En... output low
        // cam select a/b ...FLOATING(open drain)...with pull down


        // then, power down all other stuff        

        // remove high voltage from sensors
        fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_12V_POwER_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );       
        // make sure the power line has completily no power
        TimerTaskDelayMs( 500 ); 

        // disable adcex channels
        // adc ex        
        // un asserted pins
        // change pin output config to push pull to drive hard
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_1_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_2_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_3_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_4_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_5_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_6_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        TimerTaskDelayMs( 1 ); 
        // once pin state is guaranteed, change pin config as open drain for soft drive
        // pin output with open drain. low = ground, high = resistor pull (up or down)
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_1_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_2_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_3_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_4_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_5_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_6_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );        
        
        // channel configuration not required any more
        fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_CONFIG_A], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
        fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_ANALOG_CH_CONFIG_B], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );               

        // spi
        // set clck and mosi to output low        
        SpiBusPowerOn( SPI_BUS_2, FALSE );         
        // change pin output config to push pull to drive hard
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_ADC], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_PORTEX], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        TimerTaskDelayMs( 1 ); 
        // once pin state is guaranteed, change pin config as open drain for soft drive
        // pin output with open drain. low = ground, high = resistor pull (up or down)
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_ADC],      GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_SPI_CS_PORTEX],   GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );

        fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_2V5_PWR_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );                       
        // pin un asserted
        fSuccess &= GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_3V3_PWR_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );                    

        gfIsIntBoardEnabled = FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardIsEnabled( void )
//!
//! \brief  Check if interface board power lines are enabled.
//!
//! \return BOOL
//!         TRUE if interface board power lines are enabled, false otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardIsEnabled( void )
{
    BOOL            fIsEnable           = FALSE;
//    BOOL            fPwr2v5LogicLevel   = 0;
//    BOOL            fPwr3v3LogicLevel   = 0;    
//    BOOL            fPwr12v0LogicLevel  = 0;    
//    GpioModeEnum    eMode2v5            = GPIO_MODE_INPUT;
//    GpioModeEnum    eMode3v3            = GPIO_MODE_INPUT;
//    GpioModeEnum    eMode12v0           = GPIO_MODE_INPUT;
//
//    // make sure is output mode    
//    IntBoardGpioGetPinMode( INT_B_2V5_PWR_ENABLE,   &eMode2v5 );
//    IntBoardGpioGetPinMode( INT_B_3V3_PWR_ENABLE,   &eMode3v3 );
//    IntBoardGpioGetPinMode( INT_B_12V_POwER_ENABLE, &eMode12v0 );
//    // make sure is enable disable
//    IntBoardGpioPinRead( INT_B_2V5_PWR_ENABLE,   &fPwr2v5LogicLevel  );    
//    IntBoardGpioPinRead( INT_B_3V3_PWR_ENABLE,   &fPwr3v3LogicLevel  );        
//    IntBoardGpioPinRead( INT_B_12V_POwER_ENABLE, &fPwr12v0LogicLevel );        
//    // 3v3 is inverted
//    if ( ( eMode2v5 == GPIO_MODE_OUTPUT ) &&
//         ( eMode3v3 == GPIO_MODE_OUTPUT ) &&
//         ( eMode12v0 == GPIO_MODE_OUTPUT ) &&
//         ( fPwr3v3LogicLevel == FALSE ) &&  // 3v3 line is inverted
//         ( fPwr2v5LogicLevel == TRUE ) && 
//         ( fPwr12v0LogicLevel == TRUE ) )
//    { 
//        fIsEnable = TRUE;
//    }

    fIsEnable = gfIsIntBoardEnabled;

    return fIsEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardGpioSetPinMode()
//!
//! \brief  Initializes a pin as input or output.
//!
//! \return BOOLEAN
//!         - TRUE if operation successfully
//!         - FALSE if error on the operation
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardGpioSetPinMode( const IntBoardGpioPinEnum ePinName, const GpioModeEnum eMode, const LOGIC eLogic, GpioOutputTypeEnum eOutputType )
{
    BOOL fSuccess = FALSE;    

    if( ePinName < INT_B_NUMBER_OF_PINS )
    {
        fSuccess = GpioInitSinglePin( gtInterfaceBoardLookupArray[ePinName], eMode, eLogic, eOutputType );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardGpioGetPinMode()
//!
//! \brief  Returns the mode of a pin. input, output, Alternative function and adc modes
//!
//! \return BOOLEAN
//!         - TRUE if no error on the operation
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardGpioGetPinMode( const IntBoardGpioPinEnum ePinName, GpioModeEnum *peMode )
{
    BOOL fSuccess = FALSE;

    if( ePinName < INT_B_NUMBER_OF_PINS )
    {
        if( NULL != peMode )
        {
            fSuccess = GpioGetPinMode( gtInterfaceBoardLookupArray[ePinName], peMode );            
        }        
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardGpioOutputWrite( const IntBoardGpioPinEnum ePinName, BOOL fLogicLevel )
//!
//! \brief  changes the logic of the interface board Gpio OUTPUT pin. 
//!         LOGIC param can be HIGH or LOW
//!
//! \return BOOLEAN
//!         - TRUE if operation successfully
//!         - FALSE if error on the operation
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardGpioOutputWrite( const IntBoardGpioPinEnum ePinName, const BOOL fLogicLevel )
{
    BOOL            fSuccess = FALSE;
    GpioModeEnum    eMode;

    if( ePinName < INT_B_NUMBER_OF_PINS )
    {
        GpioGetPinMode( gtInterfaceBoardLookupArray[ePinName], &eMode );

        if( eMode == GPIO_MODE_OUTPUT )
        {
            fSuccess = GpioWrite( gtInterfaceBoardLookupArray[ePinName], fLogicLevel );                        
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardGpioPinRead()
//!
//! \brief  Read the logic status of the pin.
//!
//! \return BOOLEAN
//!         - TRUE if operation successfully
//!         - FALSE if error on the operation
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardGpioPinRead( const IntBoardGpioPinEnum ePinName, BOOL *pfLogicLevel )
{
    BOOL            fSuccess = FALSE;
    GpioModeEnum    eMode;

    if( ePinName < INT_B_NUMBER_OF_PINS )
    {
        GpioGetPinMode( gtInterfaceBoardLookupArray[ePinName], &eMode );

        if( ( eMode == GPIO_MODE_INPUT ) || ( eMode == GPIO_MODE_OUTPUT ) || ( eMode == GPIO_MODE_AF ) )
        {
            fSuccess = GpioRead( gtInterfaceBoardLookupArray[ePinName], pfLogicLevel );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardAdcPwrRailRead( const IntBoardAdcPowerRailEnum eAdcPwrRail, SINGLE *psgAdcResultMilliVolts )
//!
//! \brief  Read a Power Rail Voltage from interface board
//!
//! \return BOOLEAN
//!         - TRUE if operation successfully
//!         - FALSE if error on the operation
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardAdcPwrRailRead( const IntBoardAdcPowerRailEnum eAdcPwrRail, UINT16 *pwAdcReadingRaw, SINGLE *psgAdcResultMilliVolts )
{
    BOOL            fSuccess    = FALSE;
    AdcSensorEnum   eAdcSensor  = 0;
    UINT16          wAdcRaw     = 0;
            
    // make sure Adc is enabled in the interface board
    BOOL fLogicLevel = FALSE;
    IntBoardGpioPinRead( INT_B_PWR_SCALED_ADC_ENABLE, &fLogicLevel );

    if( fLogicLevel == 1 )
    {
        // validate power line
        switch( eAdcPwrRail )
        {
            case INT_B_ADC_PWR_RAIL_PRIMARY:
            fSuccess = TRUE;            
            eAdcSensor = ADC_SENSOR_EXT_PRIMARY_BATTERY_VOLTAGE;
            break;
        case INT_B_ADC_PWR_RAIL_SECONDARY:
            fSuccess = TRUE;            
            eAdcSensor = ADC_SENSOR_EXT_SECONDARY;
            break;
        case INT_B_ADC_PWR_RAIL_TERTIARY:   
            fSuccess = TRUE;            
            eAdcSensor = ADC_SENSOR_EXT_TERTIARY;
            break;        
        }

        if( fSuccess )
        {                
            // perform adc reading
            fSuccess &= AdcInternalGetReadingRaw( eAdcSensor, 1, &wAdcRaw );

            if( NULL != pwAdcReadingRaw )
            {
                *pwAdcReadingRaw = wAdcRaw;
            }
            // convert to volts
            if( NULL != psgAdcResultMilliVolts )
            {
                *psgAdcResultMilliVolts = AdcInternalRawToMilliVolts( eAdcSensor, wAdcRaw );
            }
        }
    }    

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardIsGpioOnIntBoard( const GpioPinNamesEnum )
//!
//! \brief  indicates if the gpio is wired to the int board
//!
//! \return BOOLEAN
//!         - TRUE if is connected
//!         - FALSE if otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardIsGpioOnIntBoard( const GpioPinNamesEnum eGpioPin )
{
    BOOL fSuccess = FALSE;
    
    for( IntBoardGpioPinEnum eIntbPinCounter = 0 ; eIntbPinCounter < INT_B_NUMBER_OF_PINS ; eIntbPinCounter++ )
    {
        if( gtInterfaceBoardLookupArray[ eIntbPinCounter ] == eGpioPin )
        {
            fSuccess = TRUE;
            break;
        }
    }

    return fSuccess;    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void IntBoardCamEnable( BOOL fEnable )
//!
//! \brief  enables disables the pwr on the camera
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void IntBoardCamEnable( BOOL fEnable )
{
    GPIO_InitTypeDef     GPIO_InitStructure;

    if( fEnable )
    {
        if( IntBoardIsEnabled() )
        {          
            if( gfIsCamEnabled == FALSE )
            {
                // pin stats
                // usart are inputs
                // Cam EN .. FLOATING(open drain)...with pull up
                // 422 rec EN ..FLOATING(open drain)...with pull up        
                // 422 driv En... output low
                // cam select a/b ...FLOATING(open drain)...with pull down

                // start turning on lines                   
                GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_SEL_A_B], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
                // power on cam 
                // assert pin
                // change pin output config to push pull to drive hard
                //GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
                // pin output with open drain. low = ground, high = resistor pull (up or down)
                GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_OPEN_DRAIN );

                TimerTaskDelayMs(100);

                // usart on
                UsartPortOpen( TARGET_USART_PORT_CAMERA );

                // enable rx tx
                // assert
                // change pin output config to push pull to drive hard
                //GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_RECEIVER_OUT_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );                   
                // pin output with open drain. low = ground, high = resistor pull (up or down)
                GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_RECEIVER_OUT_ENABLE], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        
                GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_DRIVER_OUT_ENABLE],    GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );       

                TimerTaskDelayMs( 10 ); 

                gfIsCamEnabled = TRUE;
            }
        }
    }
    else
    {   
        // change pin output config to push pull to drive hard
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_RECEIVER_OUT_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        TimerTaskDelayMs( 1 ); 
        // once pin state is guaranteed, change pin config as open drain for soft drive
        // pin output with open drain. low = ground, high = resistor pull (up or down)
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_RECEIVER_OUT_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );        

        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAM_RS422_DRIVER_OUT_ENABLE],   GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );

        // set all the pins as inputs and open drains for the cam enable and receiver drive
        // usart off
        UsartPortClose( TARGET_USART_PORT_CAMERA );

        // change pin output config to push pull to drive hard
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_SEL_A_B], GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        TimerTaskDelayMs( 1 ); 
        // once pin state is guaranteed, change pin config as open drain for soft drive        
        // pin output with open drain. low = ground, high = resistor pull (up or down)
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_SEL_A_B],GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );

        // change pin output config to push pull to drive hard
//        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );        
//        TimerTaskDelayMs( 1 ); 
        // once pin state is guaranteed, change pin config as open drain for soft drive        
        // pin output with open drain. low = ground, high = resistor pull (up or down)
        GpioInitSinglePin( gtInterfaceBoardLookupArray[INT_B_CAMERA_ENABLE], GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_OPEN_DRAIN );
        

        // pin stats
        // usart are inputs
        // Cam EN .. FLOATING(open drain)...with pull up
        // 422 rec EN ..FLOATING(open drain)...with pull up        
        // 422 driv En... output low
        // cam select a/b ...FLOATING(open drain)...with pull down

        gfIsCamEnabled = FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL IntBoardIsCamEnabled( void )
//!
//! \brief  indicates if pwr on the camera is enabled disabled
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IntBoardIsCamEnabled( void )
{
    return gfIsCamEnabled;
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
