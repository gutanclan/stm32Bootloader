/*
 * FreeRTOS Interrupt Handlers
 * 
 * This file contains the FreeRTOS-specific interrupt handlers that need to be
 * separated from the main FreeRTOS port.c file to avoid conflicts with
 * existing project handlers.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

/* External variables from sysTick.c - need to be declared here since sysTick.c handler is commented out */
volatile unsigned long long gqdwSysTickCounter = 0;
volatile unsigned long gdwSysTickCounter = 0;

/*
 * Application Tick Hook - Called by FreeRTOS on each tick
 * This allows us to increment system tick counters without conflicting
 * with the FreeRTOS SysTick_Handler
 */
void vApplicationTickHook(void)
{
    /* Maintain existing system tick counters for timer module */
    gqdwSysTickCounter++;
    gdwSysTickCounter++;
}

/* FreeRTOS port layer functions - these are called by the handlers above */
extern void xPortPendSVHandler(void);
extern void vPortSVCHandler(void);

/*
 * PendSV Handler - FreeRTOS context switching
 * Commented out - let FreeRTOS port.c handle these directly
 */
// void PendSV_Handler(void)
// {
//     xPortPendSVHandler();
// }

/*
 * SVC Handler - FreeRTOS system calls
 * Commented out - let FreeRTOS port.c handle these directly
 */
// void SVC_Handler(void)
// {
//     vPortSVCHandler();
// }
