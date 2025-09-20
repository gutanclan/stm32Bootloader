////////////////////////////////////////////////////////////////////////////////////////////////////
//! Test Task Implementation
//! Simple LED blinking task for FreeRTOS functionality verification
////////////////////////////////////////////////////////////////////////////////////////////////////

// Task header
#include "test_task.h"

// General Library includes - MUST come before driver includes
#include "../../drivers/common/Types.h"

// Driver includes
#include "../../drivers/gpio/gpio.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Task Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

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
