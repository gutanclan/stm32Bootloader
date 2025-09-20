#ifndef MAIN_APP_TASK_H
#define MAIN_APP_TASK_H

// FreeRTOS includes
#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Main application task
 * @details Handles console operations, serial communication, and system boot updates
 * @param pvParameters Task parameters (unused)
 */
void mainApplicationTask(void *pvParameters);

#endif // MAIN_APP_TASK_H
