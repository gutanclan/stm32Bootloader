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
 * SysTick Handler - combines FreeRTOS tick with existing system tick functionality
 */
void SysTick_Handler(void)
{
    /* Call FreeRTOS tick handler functionality directly */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        /* Increment the RTOS tick */
        portDISABLE_INTERRUPTS();
        {
            if( xTaskIncrementTick() != pdFALSE )
            {
                /* A context switch is required. Context switching is performed in
                the PendSV interrupt. Pend the PendSV interrupt. */
                portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
            }
        }
        portENABLE_INTERRUPTS();
    }
    
    /* Maintain existing system tick counters */
    gqdwSysTickCounter++;
    gdwSysTickCounter++;
}

/* FreeRTOS port layer functions - these are called by the handlers above */
extern void xPortPendSVHandler(void);
extern void vPortSVCHandler(void);

/*
 * PendSV Handler - FreeRTOS context switching
 */
void PendSV_Handler(void)
{
    xPortPendSVHandler();
}

/*
 * SVC Handler - FreeRTOS system calls
 */
void SVC_Handler(void)
{
    vPortSVCHandler();
}
