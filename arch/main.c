//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../hal/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../drivers/common/Types.h"
#include "../drivers/common/Queue.h"
#include "../drivers/common/Target_CurrentTarget.h"

//// Low level drivers
#include "../drivers/common/sysTick.h"
#include "../drivers/common/timer.h"

#include "../drivers/gpio/gpio.h"
#include "../drivers/uart/usart.h"
#include "../drivers/rtc/rtc.h"

#include "../drivers/flash/FlashMem/NandSimu.h"
#include "../subsys/memory/Flash.h"

#include "../subsys/serial/serial.h"

#include "../subsys/iostream/ioStream.h"
//
#include "../subsys/console/strLineEditor.h"
#include "../subsys/console/print.h"
#include "../subsys/console/console.h"
#include "../subsys/console/Command/command.h"

#include "../services/SysBoot.h"
#include "../services/SysTime.h"

// FreeRTOS includes
#include "../FreeRTOS/include/FreeRTOS.h"
#include "../FreeRTOS/include/task.h"
#include "../app/FreeRTOSHandlers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

static void mainHardwareSetup       ( void );
static void mainApplicationInit     ( void );
static void mainApplicationTask     ( void *pvParameters );

void HardFault_Handler              ( void );
void PendSV_Handler                 ( void );
void SVC_Handler                    ( void );
void UsageFault_Handler             ( void );
void MemManage_Handler              ( void );

////////////////////////////////////////////////////////////////////////////////////////////////////

static CHAR gcBuffer[256];

////////////////////////////////////////////////////////////////////////////////////////////////////

// Simple test task for FreeRTOS isolation
void vTestTask(void *pvParameters)
{
    // Configure pin D12 as output for LED
    GpioPortClockEnable(GPIO_PORT_D, TRUE);
    GpioModeSetOutput(GPIO_PORT_D, 12);
    
    // Initialize LED to OFF state
    GpioOutputRegWrite(GPIO_PORT_D, 12, GPIO_LOGIC_LOW);
    
    while(1)
    {
        // Toggle LED on pin D12
        GpioLogicEnum currentState;
        GpioOutputRegRead(GPIO_PORT_D, 12, &currentState);
        
        if(currentState == GPIO_LOGIC_LOW)
        {
            GpioOutputRegWrite(GPIO_PORT_D, 12, GPIO_LOGIC_HIGH);
        }
        else
        {
            GpioOutputRegWrite(GPIO_PORT_D, 12, GPIO_LOGIC_LOW);
        }
        
        // Delay 1 second between blinks
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main( void )
{
    // EXPERIMENT 1: Minimal FreeRTOS test
    // Skip all application initialization to isolate FreeRTOS issues

    // Essential: Initialize system clock for FreeRTOS (following wd3 pattern)
    // Note: Your project uses SPL, not HAL like iotSeedFirmware
    SystemInit();  // This should set SystemCoreClock

    // Additional clock initialization from working wd3 project
    RCC_ClocksTypeDef tRCC_Clocks;
    RCC_GetClocksFreq(&tRCC_Clocks);
    SystemCoreClock = tRCC_Clocks.SYSCLK_Frequency;
    SystemCoreClockUpdate();

    // Only initialize essential hardware
    mainHardwareSetup();    // DISABLED for isolation test
    mainApplicationInit();  // DISABLED for isolation test

    // Create main application task
    BaseType_t xResult1 = xTaskCreate(mainApplicationTask, "MainApp", 512, NULL, 2, NULL);
    
    // Create test task (optional - can be removed later)
    BaseType_t xResult2 = xTaskCreate(vTestTask, "Test", 256, NULL, 1, NULL);

    if (xResult1 != pdPASS || xResult2 != pdPASS) {
        // Task creation failed
        while(1) {
            // Breakpoint here if task creation fails
        }
    }

    // Start the FreeRTOS scheduler - this should not return
    vTaskStartScheduler();

    /////////////////////////////////////////////
    // Should never reach here
    /////////////////////////////////////////////
    while(1)
    {
        // Breakpoint here - scheduler failed to start
    }
}

void mainHardwareSetup( void )
{
    // SysTickModuleInit();    // Temporarily disabled - FreeRTOS will handle SysTick

    //RtcInitialize( RTC_SOURCE_CLOCK_LSI );
    //BOOL fSuccess = RtcIsRtcConfigured();
    SysTimeInit();


    GpioModuleInit();

    //AdcModuleInit();
    AdcModuleInit();
    //AdcTest();

    QueueModuleInit();

    UsartModuleInit();

    //StrQueueCirModuleInit();
//    StrQueueCirTest();

    //jnkj
    //StrLineEditorModuleInit();


    //StrLineEditorTest();

    NandSimuModuleInit();
    FlashInitModule();

    SpiModuleInit();
}

void mainApplicationInit( void )
{
    SerialModuleInit();
    SerialPortConfigType tPortConfig;
    tPortConfig.dwBaudRate = 115200;
    tPortConfig.eData = USART_WORD_LENGTH_8_BITS;
    tPortConfig.eParity = USART_PARITY_NON;
    tPortConfig.eStop = USART_STOP_BIT_1;

    SerialPortSetConfig( SERIAL_PORT_MAIN, &tPortConfig );
    SerialPortEnable( SERIAL_PORT_MAIN, TRUE );

    IoStreamInit();

    PrintSetBuffer( &gcBuffer[0], sizeof(gcBuffer) );

    ConsoleModuleInit();

    DevInfoModuleInit();

    SysBootInit();
}

void mainApplicationTask( void *pvParameters )
{
    PrintDebugfEnable( TRUE );

    ConsoleInputProcessingAutoUpdateEnable( CONSOLE_PORT_MAIN, TRUE );

    ConsoleEnable( CONSOLE_PORT_MAIN, TRUE );
    //ConsolePrintfEnable( CONSOLE_PORT_MAIN, TRUE );

    ConsoleCommandDictionarySet( CONSOLE_PORT_MAIN, CommandDictionaryGetPointer( COMMAND_DICTIONARY_1 ) );

    //UINT32  dwDelayDuration_mSec= 50;
    UINT32  dwDelayDuration_mSec= 5000;
    TIMER   xTimer              = TimerDownTimerStartMs( dwDelayDuration_mSec );

    BOOL fIsClientConnectedPreviousState = FALSE;

    while(1)
    {
        SysBootUpdate();
        ConsoleInputProcessingAutoUpdate();
        SerialPortIsClientConnectedUpdate();

        if( fIsClientConnectedPreviousState != SerialPortIsClientConnected( SERIAL_PORT_MAIN ) )
        {
            fIsClientConnectedPreviousState = SerialPortIsClientConnected( SERIAL_PORT_MAIN );

            // if new state is connected then print prompt
            if( fIsClientConnectedPreviousState )
            {
                Printf( IO_STREAM_USER, ">" );
            }
        }

        if( TimerDownTimerIsExpired(xTimer) == TRUE )
        {
            // restart timer
            xTimer    = TimerDownTimerStartMs( dwDelayDuration_mSec );
        }
        
        // Give other tasks CPU time
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void HardFault_Handler( void )
{
    // Add a breakpoint here to debug the fault
    volatile uint32_t *hardfault_args;
    asm volatile (
        "tst lr, #4                                                \n"
        "ite eq                                                    \n"
        "mrseq r0, msp                                             \n"
        "mrsne r0, psp                                             \n"
        "ldr r1, [r0, #24]                                        \n"
        "mov %0, r0"
        : "=r" (hardfault_args) : : "r0", "r1"
    );

    // hardfault_args[0] = R0, hardfault_args[1] = R1, etc.
    // hardfault_args[6] = PC (program counter where fault occurred)
    // hardfault_args[7] = xPSR

    while(1)
    {
        // Set breakpoint here and examine hardfault_args[6] for PC
        // This will show exactly where the fault occurred
    }
}

// PendSV_Handler and SVC_Handler moved to FreeRTOSHandlers.c to avoid conflicts
// These handlers are now managed by FreeRTOS for proper task switching
/*
void PendSV_Handler( void )
{
    while(1)
    {}
}

void SVC_Handler( void )
{
    while(1)
    {}
}
*/

void UsageFault_Handler( void )
{
    while(1)
    {}
}

void MemManage_Handler( void )
{
    while(1)
    {}
}

