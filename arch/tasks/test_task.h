#ifndef TEST_TASK_H
#define TEST_TASK_H

// FreeRTOS includes
#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Simple test task for FreeRTOS isolation
 * @details Blinks LED on pin D12 every 1 second to verify FreeRTOS functionality
 * @param pvParameters Task parameters (unused)
 */
void vTestTask(void *pvParameters);

#endif // TEST_TASK_H
