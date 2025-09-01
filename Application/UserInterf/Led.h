/** C Header *******************************************************************
*
* NAME
*   led.h
*
* DESCRIPTION
*   Header file for led.c module.
*
*  NOTES:
*     NECESARY CHANGES IN THE FILES TO USE THIS LIBRARY:
*     1. Change the TARGET_PIN and TARGET_PORT from the tLedLookupArray[]
*        at the .c file to match the target.
*     2. Change the following defines to meet the needs of the project.
*
*         #define LED_TASK_PRIORITY
*         #define LED_TASK_SLEEP_TIME
*         #define LED_BLINK_TIME_SLOW_MS
*         #define LED_BLINK_TIME_FAST_MS
*
*  USAGE:
*     1. Initialize your GPIOs (pins and ports)
*     2. Initialize the Led Thread by calling LED_initialize();
*     3. Change a LED pattern by calling LED_setLedBlinkPattern();
*
********************************************************************************
* (C) Copyright 2012 Puracom Inc.
* Calgary, Alberta, Canada, www.puracom.com
*******************************************************************************/

#ifndef LED_H
#define LED_H

/** Exportable Constants and Types ********************************************/

/* LED index names, matching the order of tLedLookupArray[] */
typedef enum
{
    LED1_POWER      = 0,
    
    /* ---- Add new LED's above this line! ---- */
    LED_ENUM_TOTAL,    /* Number of LEDs */
} LedHandleEnum;

/* Available LED patterns */
typedef enum
{
    LED_PATTERN_OFF = 0,
    LED_PATTERN_ON,
    LED_PATTERN_SLOW,
    LED_PATTERN_FAST,

    /* ---- Always add new LED patterns above this line! ---- */
    LED_NUM_LED_PATTERNS, /* Number of patterns */

} LedBlinkPatternEnum;

/** Exportable Function Prototypes ********************************************/

BOOL Led_Init                        ( void );
void Led_Update                      ( void );

BOOL LED_enable                      ( LedHandleEnum eLedIndex, BOOL fEnabled );
BOOL LED_setLedBlinkPattern          ( LedHandleEnum eLedIndex, LedBlinkPatternEnum ePattern );
BOOL LED_getCurrentLedBlinkPattern   ( LedHandleEnum eLedIndex, LedBlinkPatternEnum *pePattern );

#endif /* LED_H */

/* EOF */

