/*
 * FreeRTOS Interrupt Handlers Header
 * 
 * This file declares the FreeRTOS-specific interrupt handlers.
 */

#ifndef FREERTOS_HANDLERS_H
#define FREERTOS_HANDLERS_H

/* FreeRTOS interrupt handlers */
void SysTick_Handler(void);
void PendSV_Handler(void);
void SVC_Handler(void);

#endif /* FREERTOS_HANDLERS_H */
