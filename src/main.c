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
#include "../inc/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../Application/Utils/Types.h"
#include "../Application/Utils/Queue.h"
#include "../Application/Targets/Target_CurrentTarget.h"

//// Low level drivers
#include "../Application/DriversMcuPeripherals/sysTick.h"
#include "../Application/Utils/timer.h"

#include "../Application/DriversMcuPeripherals/gpio.h"
#include "../Application/DriversMcuPeripherals/usart.h"
#include "../Application/DriversMcuPeripherals/rtc.h"

#include "../Application/DriversExternalDevices/FlashMem/NandSimu.h"
#include "../Application/Memory/Flash.h"

#include "../Application/Serial/serial.h"

#include "../Application/IoStream/ioStream.h"
//
#include "../Application/Console/strLineEditor.h"
#include "../Application/Console/print.h"
#include "../Application/Console/console.h"
#include "../Application/Console/Command/command.h"

#include "../Application/System/SysBoot.h"
#include "../Application/System/SysTime.h"

// FreeRTOS includes
#include "../FreeRTOS/include/FreeRTOS.h"
#include "../FreeRTOS/include/task.h"
#include "../Application/Apps/FreeRTOSHandlers.h"

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

int main( void )
{
    // initializes hardware peripherals
    mainHardwareSetup();

    // initializes application modules
    mainApplicationInit();

    // Initialize FreeRTOS (tasks will be created here in the future)
    // For now, create a simple main task to preserve existing functionality
    xTaskCreate(mainApplicationTask, "MainTask", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL);
    
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    /////////////////////////////////////////////
    // it should never reach this point!!
    // If we get here, there was insufficient memory to create the idle task
    /////////////////////////////////////////////
    while(1)
    {
        // Error: FreeRTOS scheduler failed to start
    }
}

void mainHardwareSetup( void )
{
    SysTickModuleInit();    // required for timer to work

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
    }

//#if(0)
//    CHAR cChar;
//
//    while(1)
//    {
//        if( UsartGetChar( USART_PORT_3, &cChar ) )
//        {
//            UsartPutChar( USART_PORT_3, cChar );
//        }
//    }
//#else
//    CHAR cCharArray[50];
//    UINT16 wBytesReceived;
//    UINT32  dwDelayDuration_mSec    = 500;
//    TIMER xDownTimer    = TimerDownTimerStartMs( dwDelayDuration_mSec );
//
//
//    while(1)
//    {
//        if( TimerDownTimerIsExpired(xDownTimer) == TRUE )
//        {
//            xDownTimer = TimerDownTimerStartMs( dwDelayDuration_mSec );
//
//            if( UsartGetBuffer( USART_PORT_3, &cCharArray[0], sizeof(cCharArray), &wBytesReceived ) )
//            {
//                UsartPutBuffer( USART_PORT_3, &cCharArray[0], wBytesReceived );
//            }
//        }
//    }
//#endif



//    UINT32  dwDelayDuration_mSec    = 500;
//    UINT64  qdwLargeTickCounter     = 0;
//
//    CHAR    szString[] = "hellow worls 123 test\r\n";
//
//    UINT8   cStartChar  = 33;
//    UINT8   cEndChar    = 126;
//    UINT8   cCurrentChar= cStartChar;
//
//    UINT8 bIndex = 0;
//    UINT8 bLen = strlen((const char *)szString);
//
//    ConsoleEnable( CONSOLE_PORT_MAIN, TRUE );
//    // test put char
//    for( bIndex = 0 ; bIndex < bLen ; bIndex++ )
//    {
//        ConsolePutChar( CONSOLE_PORT_MAIN, szString[bIndex] );
//    }
//    // test ConsolePutBuffer();
//    ConsolePutBuffer( CONSOLE_PORT_MAIN, szString, bLen );
//    // test console printf()
//    ConsolePrintf( CONSOLE_PORT_MAIN, "%s", szString );
//
//    ConsolePrintf( CONSOLE_PORT_MAIN, "hellow world. Delay= %d\r\n", dwDelayDuration_mSec );
//    ConsolePrintf( CONSOLE_PORT_MAIN, "//***********************************\r\n" );
//    ConsolePrintf( CONSOLE_PORT_MAIN, "        Jorge's application\r\n" );
//    ConsolePrintf( CONSOLE_PORT_MAIN, "         Version 10.0.0\r\n" );
//    ConsolePrintf( CONSOLE_PORT_MAIN, "//***********************************\r\n" );
//
//
//    RtcDateTimeStruct tDateTime;
//    RtcDateTimeGet( &tDateTime );
//    ConsolePrintf
//    (
//        CONSOLE_PORT_MAIN, "%04d-%02d-%02d %02d:%02d:%02d\r\n",
//        tDateTime.bYear+RTC_CENTURY_OFFSET,
//        tDateTime.eMonth,
//        tDateTime.bDate,
//        tDateTime.bHour,
//        tDateTime.bMinute,
//        tDateTime.bSecond
//    );
//
//
//    ConsolePrintf( CONSOLE_PORT_MAIN, "System ready start typing:" );
//
//
//    GpioConfigType gGpioConf_Out_OPp_O2MHz_RNoP = { GPIO_MODE_OUTPUT,    GPIO_OTYPE_PUSH_PULL,   GPIO_OSPEED_2MHZ,   GPIO_PUPD_NOPUPD };
//    GpioSetConfig( GPIO_PIN_LED_1, &gGpioConf_Out_OPp_O2MHz_RNoP );
//
//#if(0)
//#else
//    BOOL fIsLedOn       = FALSE;
//    TIMER xDownTimer    = TimerDownTimerStartMs( dwDelayDuration_mSec );
//
//    CHAR cChar              = 0;
//    BOOL fIsDeviceAttachedCurrent   = FALSE;
//    BOOL fIsDeviceAttachedPrevious  = FALSE;
//
//    while(1)
//    {
//        ConsoleInputProcessingAutoUpdate();
//
//        if( TimerDownTimerIsExpired(xDownTimer) == TRUE )
//        {
//            // restart timer
//            xDownTimer  = TimerDownTimerStartMs( dwDelayDuration_mSec );
//
//            if( fIsLedOn )
//            {
//                fIsLedOn = FALSE;
//                // set led off
//                GpioOutputRegWrite( GPIO_PIN_LED_1, GPIO_LOGIC_LOW );
//            }
//            else
//            {
//                fIsLedOn = TRUE;
//                // set led on
//                GpioOutputRegWrite( GPIO_PIN_LED_1, GPIO_LOGIC_HIGH );
//            }
//        }
//
////        if( UsartGetChar( USART3_PORT, &cChar ) )
////        {
////            UsartPutChar( USART3_PORT, cChar );
////        }
//    }
//    #endif
}


void HardFault_Handler( void )
{
    while(1)
    {}
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

