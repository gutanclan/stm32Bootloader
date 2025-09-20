////////////////////////////////////////////////////////////////////////////////////////////////////
//! Main Application Task Implementation
//! Handles console operations, serial communication, and system boot updates
////////////////////////////////////////////////////////////////////////////////////////////////////

// Task header
#include "main_app_task.h"

// General Library includes
#include "../../drivers/common/Types.h"
#include "../../drivers/common/timer.h"

// Subsystem includes
#include "../../subsys/serial/serial.h"
#include "../../subsys/iostream/ioStream.h"
#include "../../subsys/console/print.h"
#include "../../subsys/console/console.h"
#include "../../subsys/console/Command/command.h"

// Services includes
#include "../../services/SysBoot.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Task Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

void mainApplicationTask(void *pvParameters)
{
    PrintDebugfEnable(TRUE);

    ConsoleInputProcessingAutoUpdateEnable(CONSOLE_PORT_MAIN, TRUE);

    ConsoleEnable(CONSOLE_PORT_MAIN, TRUE);
    //ConsolePrintfEnable(CONSOLE_PORT_MAIN, TRUE);

    ConsoleCommandDictionarySet(CONSOLE_PORT_MAIN, CommandDictionaryGetPointer(COMMAND_DICTIONARY_1));

    //UINT32  dwDelayDuration_mSec= 50;
    UINT32  dwDelayDuration_mSec = 5000;
    TIMER   xTimer = TimerDownTimerStartMs(dwDelayDuration_mSec);

    BOOL fIsClientConnectedPreviousState = FALSE;

    while(1)
    {
        SysBootUpdate();
        ConsoleInputProcessingAutoUpdate();
        SerialPortIsClientConnectedUpdate();

        if(fIsClientConnectedPreviousState != SerialPortIsClientConnected(SERIAL_PORT_MAIN))
        {
            fIsClientConnectedPreviousState = SerialPortIsClientConnected(SERIAL_PORT_MAIN);

            // if new state is connected then print prompt
            if(fIsClientConnectedPreviousState)
            {
                Printf(IO_STREAM_USER, ">");
            }
        }

        if(TimerDownTimerIsExpired(xTimer) == TRUE)
        {
            // restart timer
            xTimer = TimerDownTimerStartMs(dwDelayDuration_mSec);
        }
        
        // Give other tasks CPU time
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
