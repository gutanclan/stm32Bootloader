//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        System.c
//!    \brief       Functions for the main System.
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <string.h>

// STM Library includes
								// NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"		// Macro: assert_param() for other STM libraries.

// RTOS Library includes
// RTOS Library includes
#include "FreeRTOS.h"
#include "task.h"

// PCOM General Library includes
#include "Types.h"				// Timer depends on Types
#include "Timer.h"				// task depends on Timer for vApplicationTickHook

// PCOM Project Targets
#include "Target.h"				// Hardware target specifications

#include "Rtc.h"
#include "Usart.h"
#include "Spi.h"
#include "Console.h"
#include "Bluetooth.h"
#include "Led.h"

#include "UserIf.h"
// PCOM Library includes
#include "System.h"


///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

static BOOL SystemLowPowerModeStop		( void );
static void SystemLowPowerModeStandBy	( void );

///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\fn			BOOL SystemClockInitialize( void )
//!
//!	\brief		Setup the microcontroller system clock for the application.
//!
//!	\return		void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SystemClockInitialize( void )
{
	BOOL				fSuccess = FALSE;
    RCC_ClocksTypeDef	tRCC_Clocks;

	RCC_DeInit();											// Resets the RCC clock configuration to the default reset state.
	RCC_HSEConfig( RCC_HSE_ON );							// Configures the External High Speed oscillator.
    fSuccess = RCC_WaitForHSEStartUp();						// Waits on HSERDY flag to be set.

	if( fSuccess == TRUE )
	{
		/*  This group includes the following functions:
			- void FLASH_SetLatency(uint32_t FLASH_Latency)
			To correctly read data from FLASH memory, the number of wait states (LATENCY) 
			must be correctly programmed according to the frequency of the CPU clock 
			(HCLK) and the supply voltage of the device.
		 +-------------------------------------------------------------------------------------+     
		 | Latency       |                HCLK clock frequency (MHz)                           |
		 |               |---------------------------------------------------------------------|     
		 |               | voltage range  | voltage range  | voltage range   | voltage range   |
		 |               | 2.7 V - 3.6 V  | 2.4 V - 2.7 V  | 2.1 V - 2.4 V   | 1.8 V - 2.1 V   |
		 |---------------|----------------|----------------|-----------------|-----------------|              
		 |0WS(1CPU cycle)|0 < HCLK <= 30  |0 < HCLK <= 24  |0 < HCLK <= 18   |0 < HCLK <= 16   |
		 |---------------|----------------|----------------|-----------------|-----------------|   
		 |1WS(2CPU cycle)|30 < HCLK <= 60 |24 < HCLK <= 48 |18 < HCLK <= 36  |16 < HCLK <= 32  | 
		 |---------------|----------------|----------------|-----------------|-----------------|   
		 |2WS(3CPU cycle)|60 < HCLK <= 90 |48 < HCLK <= 72 |36 < HCLK <= 54  |32 < HCLK <= 48  |
		 |---------------|----------------|----------------|-----------------|-----------------| 
		 |3WS(4CPU cycle)|90 < HCLK <= 120|72 < HCLK <= 96 |54 < HCLK <= 72  |48 < HCLK <= 64  |
		 |---------------|----------------|----------------|-----------------|-----------------| 
		 |4WS(5CPU cycle)|      NA        |96 < HCLK <= 120|72 < HCLK <= 90  |64 < HCLK <= 80  |
		 |---------------|----------------|----------------|-----------------|-----------------| 
		 |5WS(6CPU cycle)|      NA        |      NA        |90 < HCLK <= 108 |80 < HCLK <= 96  | 
		 |---------------|----------------|----------------|-----------------|-----------------| 
		 |6WS(7CPU cycle)|      NA        |      NA        |108 < HCLK <= 120|96 < HCLK <= 112 | 
		 |---------------|----------------|----------------|-----------------|-----------------| 
		 |7WS(8CPU cycle)|      NA        |      NA        |     NA          |112 < HCLK <= 120| 
		 |***************|****************|****************|*****************|*****************|*****************************+
		 |               | voltage range  | voltage range  | voltage range   | voltage range   | voltage range 2.7 V - 3.6 V |
		 |               | 2.7 V - 3.6 V  | 2.4 V - 2.7 V  | 2.1 V - 2.4 V   | 1.8 V - 2.1 V   | with External Vpp = 9V      |
		 |---------------|----------------|----------------|-----------------|-----------------|-----------------------------| 
		 |Max Parallelism|      x32       |               x16                |       x8        |          x64                |              
		 |---------------|----------------|----------------|-----------------|-----------------|-----------------------------|   
		 |PSIZE[1:0]     |      10        |               01                 |       00        |           11                |
		 +-------------------------------------------------------------------------------------------------------------------+  
		 */
		// Enable Prefetch Buffer
        FLASH_PrefetchBufferCmd(ENABLE);
       
        // Sets the code latency value: FLASH Two Latency cycles
        FLASH_SetLatency(FLASH_Latency_3);		
				
		/*
		* @param  PLLM: specifies the division factor for PLL VCO input clock
		*          This parameter must be a number between 0 and 63.
		* @note   You have to set the PLLM parameter correctly to ensure that the VCO input
		*         frequency ranges from 1 to 2 MHz. It is recommended to select a frequency
		*         of 2 MHz to limit PLL jitter.
		*  
		* @param  PLLN: specifies the multiplication factor for PLL VCO output clock
		*          This parameter must be a number between 192 and 432.
		* @note   You have to set the PLLN parameter correctly to ensure that the VCO
		*         output frequency is between 192 and 432 MHz.
		*   
		* @param  PLLP: specifies the division factor for main system clock (SYSCLK)
		*          This parameter must be a number in the range {2, 4, 6, or 8}.
		* @note   You have to set the PLLP parameter correctly to not exceed 120 MHz on
		*         the System clock frequency.
		*  
		* @param  PLLQ: specifies the division factor for OTG FS, SDIO and RNG clocks
		*          This parameter must be a number between 4 and 15.
		* @note   If the USB OTG FS is used in your application, you have to set the
		*         PLLQ parameter correctly to have 48 MHz clock for the USB. However,
		*         the SDIO and RNG need a frequency lower than or equal to 48 MHz to work
		*         correctly.
		*/
        UINT32 PLLM = 8;	// M(Div) output Freq = HSE/M	= 16 / 16 = 1   MHz
		UINT32 PLLN = 240;	// N(Mult)output Freq = M * N	= 1 * 240 = 240 MHz
		UINT32 PLLP = 2;	// P(Div) output Freq = N / P	= 240 / 2 = 120 MHz
		UINT32 PLLQ = 5;	// Q(Div) output Freq = N / Q	= 240 / 5 = 48  MHz
		RCC_PLLConfig( RCC_PLLSource_HSE , PLLM, PLLN, PLLP, PLLQ );
		RCC_PLLCmd( ENABLE );
        // Checks whether the specified RCC flag is set or not
        // Wait till PLL is ready
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

		// NOTE!! Depends on the RCC_SYSCLKConfig() Clock source selected is the clock speed you have to divide in the following 
		// three instructions. RCC_HCLKConfig(),RCC_PCLK1Config(), RCC_PCLK2Config()
		// If PLL is enabled, then the Clock signal to be divided is the one comming out of the PLLP clock.
		// NOTE!! APB1 or PCLK1  should not run more than 30 MHz
        // NOTE!! APB2 or PCLK2  should not run more than 60 MHz
		RCC_HCLKConfig( RCC_SYSCLK_Div1 );					// Configures the AHB clock (HCLK).AHB = 120M/1 = 120 MHz
        RCC_PCLK1Config( RCC_HCLK_Div4 );					// Configures the Low Speed APB clock (PCLK1). APB1 = HCLK / 4 = 120 / 4 = 30 MHz
        RCC_PCLK2Config( RCC_HCLK_Div2 );					// Configures the High Speed APB clock (PCLK2). APB2 = HCLK / 2 = 120 / 2 = 60 MHz

		// Configures the system clock Mux (SYSCLK).
        /*	A switch from one clock source to another occurs only if the target
		*	clock source is ready (clock stable after startup delay or PLL locked). 
		*   If a clock source which is not yet ready is selected, the switch will
		*   occur when the clock source will be ready.*/
		RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );			

		// Get System Clock Source
        // Wait till the clock source configured previously is used as system clock source.
        UINT32 CLOCK_SOURCE_HSI = 0x00;
        UINT32 CLOCK_SOURCE_HSE = 0x04;
        UINT32 CLOCK_SOURCE_PLL = 0x08;        
        while(RCC_GetSYSCLKSource() != CLOCK_SOURCE_PLL);


		// For debugging: you can see the current clock speeds from here.
		RCC_GetClocksFreq( &tRCC_Clocks );
		
		// NOTE: If using FREERTOS make sure to update configCPU_CLOCK_HZ with AHB clock speed!!!(FreeRTOSConfig.h)		
        SystemCoreClock	= tRCC_Clocks.SYSCLK_Frequency;		// Update SystemCoreClock variable according to Clock Register Values		

		/* Flash 2 wait state. */
		*( volatile unsigned long  * )0x40022000 = 0x01;

		// NOTE: NVIC priority group has to be configured for FreeRTOS.		
        /* Set the Vector Table base address at 0x08000000. */
		NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

		NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	}

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\fn			void SystemTickInitialize( void )
//!
//!	\brief		Initializes the sysTick to 1 ms interrupts
//!
//!	\return		void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SystemTickInitialize( void )
{
	BOOL fSuccess = FALSE;

	////////////////////////////////////////////////////////////////////////////
	// Setup SysTick Timer for 1 msec interrupts.
	////////////////////////////////////////////////////////////////////////////
	// To adjust the SysTick time base, use the following formula:
	//
    //     Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
    //
	//	- Reload Value is the parameter to be passed for SysTick_Config() function
    //  - Reload Value should not exceed 0xFFFFFF
	//
    //   You can change the SysTick IRQ priority by calling the
    //   NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function
    //   call. The NVIC_SetPriority() is defined inside the core_cm3.h file.
	//
	////////////////////////////////////////////////////////////////////////////
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );

	//if( SysTick_Config( SystemCoreClock / 8 / 1000 ) == 0 )
    if( SysTick_Config( SystemCoreClock / 1000 ) == 0 )
	{
		//SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

		fSuccess = TRUE;
	}
    else
    {
        // ERROR: If you reach here, the sysTick couldn't be configured
        while( 1 )
        {}
    }

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\fn			void SystemGetClockFreq( RCC_ClocksTypeDef *ptSystemClocks )
//!
//!	\brief		Returns a struct with the clock speed of SysClock, AHB, APB1 and APB2
//!
//!	\return		void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SystemGetClockFreq( RCC_ClocksTypeDef *ptSystemClocks )
{
	if( ptSystemClocks != NULL )
	{
		RCC_GetClocksFreq( ptSystemClocks );
	}
}

void SystemShutDownPeripherals( UINT32 dwWkUpInterruptMaskEnable )
{    
    ////////////////////////////////////////////////////////////////////////////
    // Shutdown all peripherals (low level drivers)
    ////////////////////////////////////////////////////////////////////////////

    // * * * * * * * * * * * * * * * * 
    // USART
    // * * * * * * * * * * * * * * * * 
    // first peripheral to turn off in order to not let the user type more commands
    UsartPortClose( TARGET_USART_PORT_TO_CONSOLE );
    UsartPortClose( TARGET_USART_PORT_TO_MODEM );
    UsartPortClose( TARGET_USART_PORT_TO_BLUETOOTH );

    // * * * * * * * * * * * * * * * * 
    // stop parallel tasks
    // * * * * * * * * * * * * * * * * 
    // Put all tasks that may be turning on GPIOs into a safe state and 
    // suspend the tasks (or stop the scheduler) before sleeping.
    LedTaskEnable( FALSE );

    // * * * * * * * * * * * * * * * * 
    // cut power from other regulators or transistors
    // * * * * * * * * * * * * * * * * 
    // Power line SUP_3V3
        // Dataflash
        // btooth
        // temp sensor[not used]
        // mpu9250 [not used]
        // adc buffers
    GpioWrite(GPIO_SUPPORT_EN, LOW);
    // Power line 3V8 and MDM_3V3(modem)
    GpioWrite(GPIO_MODEM_PWR_EN_OUT, LOW);       

    // * * * * * * * * * * * * * * * * 
    // SPI
    // * * * * * * * * * * * * * * * * 
    SpiBusPowerOnAll( FALSE );                    

    // * * * * * * * * * * * * * * * * 
    // ADC
    // * * * * * * * * * * * * * * * * 
    // Disable ADC to consume less current
    ADC_Cmd( ADC1, DISABLE );
    ADC_Cmd( ADC2, DISABLE );
    ADC_Cmd( ADC3, DISABLE );    

    // * * * * * * * * * * * * * * * * 
    // gpios "outputs" ->low	
    // * * * * * * * * * * * * * * * *
    for( GpioPortEnum eGpioPort = 0; eGpioPort < GPIO_PORT_MAX ; eGpioPort++ )
    {
        for( UINT8 bPinNumber = 0; bPinNumber < 16 ; bPinNumber++ )
        {               
            // ignore JTAG pins
            if
            (
                 ( ( eGpioPort == GPIO_PORT_A ) && ( bPinNumber == 13 ) ) ||
                 ( ( eGpioPort == GPIO_PORT_A ) && ( bPinNumber == 14 ) ) ||
                 ( ( eGpioPort == GPIO_PORT_A ) && ( bPinNumber == 15 ) ) ||
                 ( ( eGpioPort == GPIO_PORT_B ) && ( bPinNumber == 3 ) ) ||
                 ( ( eGpioPort == GPIO_PORT_B ) && ( bPinNumber == 4 ) )
            )
            {
                // do nothing
            }
            else
            {
                // 3rd arg, FALSE = LOW, TRUE = HIGH
                GpioOutputWrite( eGpioPort, bPinNumber, FALSE );
            }
        }
    }

    // * * * * * * * * * * * * * * * * 
    // enable wkup interruptions
    // * * * * * * * * * * * * * * * * 
    // ################################################################
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1 ) != 0 )
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, TRUE );
    }
    else
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, FALSE );
    }
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2 ) != 0 )
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, TRUE );
    }
    else
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, FALSE );
    }

    // set Rx pin as interrupt pin used to wake up from sleep.
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_CONSOLE_RX ) != 0 )
    {
        UsartConsoleRxInterrEnable( TRUE );    
    }
    else
    {
        UsartConsoleRxInterrEnable( FALSE );    
    }

    // set configuration variables for the [wake up on time match]
    // will wake up when seconds == 0   
    //RTC_WakeUpCmd(ENABLE);

    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_TIME_MATCH ) != 0 )
    {
        RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_SECOND, 0 );
    }
    else
    {
        RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );
    }

    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_PA0_WKUP ) != 0 )
    {
        ///////////////////////////////        
//        // Clear RCC flags to be able to determine on Wake up the source or the interruption
//        RCC_ClearFlag();
//
//        // Clear Wakeup flag
//        PWR_ClearFlag( PWR_FLAG_WU );
//
//        // Clear StandBy flag
//        PWR_ClearFlag( PWR_FLAG_SB );
//
//        while( PWR_GetFlagStatus( PWR_FLAG_SB ) == SET );
//
//        PWR_WakeUpPinCmd(ENABLE);
        ///////////////////////////////        

        RtcInterruptPinA0Enable( TRUE );        
    }
    else
    {
        //PWR_WakeUpPinCmd(DISABLE);
        RtcInterruptPinA0Enable( FALSE );
    }
    
    // ################################################################
}

void SystemStartUpPeripherals( UINT32 dwWkUpInterruptMaskEnable )
{
    ////////////////////////////////////////////////////////////////////////////
    // Wake up all peripherals (low level drivers)
    ////////////////////////////////////////////////////////////////////////////
    //RCC_ClearFlag();                                            // Clears the RCC reset flags

    // * * * * * * * * * * * * * * * * 
    // disable wkup interruptions
    // * * * * * * * * * * * * * * * *        
    // set Rx pin as interrupt pin used to wake up from sleep.
//    UsartConsoleRxInterrEnable( FALSE );
//    // disable wakeup on time match
//    RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );
//    RtcInterruptPinA0Enable( FALSE );    
//    // keep reed switched enabled since they are required all the time to trigger actions
//    UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, TRUE );
//    UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, TRUE );

    // ################################################################
    // set Rx pin as interrupt pin used to wake up from sleep.
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_CONSOLE_RX ) != 0 )
    {
        UsartConsoleRxInterrEnable( TRUE );    
    }
    else
    {
        UsartConsoleRxInterrEnable( FALSE );    
    }

    // set configuration variables for the [wake up on time match]
    // will wake up when seconds == 0        
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_TIME_MATCH ) != 0 )
    {
        RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_SECOND, 0 );
    }
    else
    {
        RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );
    }

    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_PA0_WKUP ) != 0 )
    {
        //PWR_WakeUpPinCmd(ENABLE);
        RtcInterruptPinA0Enable( TRUE );
    }
    else
    {
        //PWR_WakeUpPinCmd(DISABLE);
        RtcInterruptPinA0Enable( FALSE );
    }    

    ///  enable disable reed switches
    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1 ) != 0 )
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, TRUE );
    }
    else
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, FALSE );
    }

    if( ( dwWkUpInterruptMaskEnable & SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2 ) != 0 )
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, TRUE );
    }
    else
    {
        UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, FALSE );
    }    
    // ################################################################


    // * * * * * * * * * * * * * * * * 
    // gpios 
    // * * * * * * * * * * * * * * * *
    // stay the same.. they will be set high as other 
    // the peripherals start using them.        

    // * * * * * * * * * * * * * * * * 
    // ADC
    // * * * * * * * * * * * * * * * *     
    ADC_Cmd( ADC1, ENABLE );
    ADC_Cmd( ADC2, ENABLE );
    ADC_Cmd( ADC3, ENABLE );    

    // * * * * * * * * * * * * * * * * 
    // SPI
    // * * * * * * * * * * * * * * * * 
    SpiBusPowerOnAll( TRUE );                

    // * * * * * * * * * * * * * * * * 
    // bring power back to regulators or transistors
    // * * * * * * * * * * * * * * * * 
    // Power line SUP_3V3
        // Dataflash
        // btooth
        // temp sensor[not used]
        // mpu9250 [not used]
        // adc buffers
    // important power line to allow the use of dataflash!!
    GpioWrite(GPIO_PIN_SPI1_ACCEL_CS, HIGH);
    // WARNING!! before turning on datatflash, set chip select high.
    GpioWrite(GPIO_PIN_SPI1_DATAFLASH_CS, HIGH);
    // Dataflash always in use to store data from sensors
    GpioWrite(GPIO_SUPPORT_EN, HIGH);
    // Wait for rail to stabilize before continuing
    TimerTaskDelayMs( 100 );
    // Power line 3V8 and MDM_3V3 (modem)
    // this line will be turned on only if modem is required to be used.

    // * * * * * * * * * * * * * * * * 
    // start parallel tasks
    // * * * * * * * * * * * * * * * *     
    LedTaskEnable( TRUE );

    // * * * * * * * * * * * * * * * * 
    // USART
    // * * * * * * * * * * * * * * * * 
    // last peripheral to turn off in order to let the user type commands
    UsartPortOpen( TARGET_USART_PORT_TO_CONSOLE );
    UsartPortOpen( TARGET_USART_PORT_TO_MODEM );
    UsartPortOpen( TARGET_USART_PORT_TO_BLUETOOTH );    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL SystemSleep( BOOL fStandbySleep )
//!
//! \brief  Put the system to sleep
//!
//! \param[in]  fStandbySleep - TRUE = sleep in stanby mode, FLASE = sleep in stop mode
//!
//! \return BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SystemSleep( SystemSleepModeEnum eSleepMode )
{
	BOOL            fSuccess                        = FALSE;
    static UINT64   qwTotalAwakeTimeTicksPrevious   = 0;

    if( eSleepMode < SYSTEM_SLEEP_MODE_MAX )
    {                       
        fSuccess = TRUE;

        UINT32 dwInterruptMask;

        // enable the interruptions required for the type of system sleep
        if( eSleepMode == SYSTEM_SLEEP_MODE_STAND_BY )
        {
            dwInterruptMask = 0;                        
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_PA0_WKUP;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2;                        
        }
        else
        {            
            dwInterruptMask = 0;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_CONSOLE_RX;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_TIME_MATCH;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_PA0_WKUP;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2;            
        }

        SystemShutDownPeripherals( dwInterruptMask );

        // #########################################################################
        ////////////////////////////////////////////////////////////////////////////
        // Put system to sleep
        ////////////////////////////////////////////////////////////////////////////

        if( eSleepMode == SYSTEM_SLEEP_MODE_STAND_BY )
        {
            SystemLowPowerModeStandBy();

            // Should never return from standby mode
            SystemSoftwareReset();
        }
        else if( eSleepMode == SYSTEM_SLEEP_MODE_STOP )
        {
            fSuccess &= SystemLowPowerModeStop();
        }

        // #########################################################################
        
        // enable the interruptions required after waking up
        if( eSleepMode == SYSTEM_SLEEP_MODE_STAND_BY )
        {
            // don't care, should go back to this point
            dwInterruptMask = 0;  
        }
        else
        {            
            dwInterruptMask = 0;            
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1;
            dwInterruptMask |= SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2;            
        }

        SystemStartUpPeripherals( dwInterruptMask );

    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL SystemLowPowerModeStop( void )
//!
//! \brief  Switch the system into Stop mode
//!
//! \return BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
/*	Stop mode
	==========
	In Stop mode, all clocks in the 1.2V domain are stopped, the PLL, the HSI,
	and the HSE RC oscillators are disabled. Internal SRAM and register contents
	are preserved.
	The voltage regulator can be configured either in normal or low-power mode.
	To minimize the consumption In Stop mode, FLASH can be powered off before
	entering the Stop mode. It can be switched on again by software after exiting
	the Stop mode using the PWR_FlashPowerDownCmd() function.

    - Entry:
      - The Stop mode is entered using the PWR_EnterSTOPMode(PWR_Regulator_LowPower,)
        function with regulator in LowPower or with Regulator ON.
    - Exit:
      - Any EXTI Line (Internal or External) configured in Interrupt/Event mode.
*/
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SystemLowPowerModeStop( void )
{
    BOOL fSuccess = TRUE;

    // All interruption flags must be cleared!!!
	// RCC flags
    RCC_ClearFlag();

    ////////////////////////////////////////////////////////////
    // Power down *EVERYTHING*
    ////////////////////////////////////////////////////////////
   
    // Disables SysTick interrupts
    //taskDISABLE_INTERRUPTS();
    //__disable_irq(); 
    portENTER_CRITICAL();
	   	
    //vPortSuppressTicksAndSleep( portMAX_DELAY );
    //portSUPPRESS_TICKS_AND_SLEEP();

//	NVIC_InitTypeDef    NVIC_InitStructure;
//	// Config Commons
//	NVIC_InitStructure.NVIC_IRQChannel                      = RTC_Alarm_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 15;
//    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
//    NVIC_Init( &NVIC_InitStructure );

	//PWR_WakeUpPinCmd(ENABLE);

	// Switch SysClock source to HSI
    RCC_SYSCLKConfig( RCC_SYSCLKSource_HSI );
    while( RCC_GetSYSCLKSource() != 0x00 );     // 0x00: HSI used as system clock

	// Disables the main PLL (In this case PLL is NOT RUNNING)
	RCC_PLLCmd( DISABLE );

	// External High Speed oscillator DISABLED
    RCC_HSEConfig( RCC_HSE_OFF );
    ////////////////////////////////////////////////////////////

	// ########################################################
    PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI );		// Code sits here until interrupt occurs
    // ########################################################


    ////////////////////////////////////////////////////////////
    // Power up *EVERYTHING*
    ////////////////////////////////////////////////////////////
    // Restore SysClock and other clocks
	//RCC_PLLCmd( ENABLE );

	SystemClockInitialize();     

	PWR_WakeUpPinCmd(DISABLE);
	// Config Commons
//	NVIC_InitStructure.NVIC_IRQChannel                      = RTC_Alarm_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 15;
//    NVIC_InitStructure.NVIC_IRQChannelCmd					= DISABLE;
//    NVIC_Init( &NVIC_InitStructure );

    // Enables SysTick interrupts
	//taskENABLE_INTERRUPTS();
	//__enable_irq();
    portEXIT_CRITICAL();

    ////////////////////////////////////////////////////////////

	RCC_ClearFlag();                                            // Clears the RCC reset flags
    ////////////////////////////////////////////////////////////

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void SystemLowPowerModeStandBy( void )
//!
//! \brief  Switch the system into Stand by mode
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
/*	Standby mode
	============
	The Standby mode allows to achieve the lowest power consumption. It is based
	on the Cortex-M3 deepsleep mode, with the voltage regulator disabled.
	The 1.2V domain is consequently powered off. The PLL, the HSI oscillator and
	the HSE oscillator are also switched off. SRAM and register contents are lost
	except for the RTC registers, RTC backup registers, backup SRAM and Standby
	circuitry.

	The voltage regulator is OFF.

    - Entry:
      - The Standby mode is entered using the PWR_EnterSTANDBYMode() function.
    - Exit:
      - WKUP pin rising edge, RTC alarm (Alarm A and Alarm B), RTC wakeup,
        tamper event, time-stamp event, external reset in NRST pin, IWDG reset.
*/
//////////////////////////////////////////////////////////////////////////////////////////////////
void SystemLowPowerModeStandBy( void )
{
//	// Clear RCC flags to be able to determine on Wake up the source or the interruption
	RCC_ClearFlag();
//
//	// Clear Wakeup flag
//    PWR_ClearFlag( PWR_FLAG_WU );
//
//    // Clear StandBy flag
//    PWR_ClearFlag( PWR_FLAG_SB );
//
//    while( PWR_GetFlagStatus( PWR_FLAG_SB ) == SET );

	// ########################################################
    PWR_EnterSTANDBYMode();											// Code sits here waiting for an interruption to occur.
    // ########################################################

    // After interruption, system restarts in main, therefore you will never reach this point!!
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void SystemSoftwareReset( void )
//!
//! \brief  Software reset
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SystemSoftwareReset( void )
{
	// The function initiates a system reset request to reset the MCU.
	NVIC_SystemReset();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\fn			void SystemWatchdogRestart( void )
//!
//!	\brief		Reset system watchdog timer
//!
//!	\return		void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SystemWatchdogRestart( void )
{
    // Not implement yet.  TODO
}

// EOF //