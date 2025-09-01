/** C Source ******************************************************************
*
* NAME
*   led.c
*
* DESCRIPTION
*   Routines for the LED module.
*   This routine flashes the LEDs, disables and enables them, and switches the
*   display modes as needed for the application.
*
*   There are four display modes for the LEDs,
*                 On
*                 Rapid Flashing
*                 Slow Flashing
*                 Off
*
** TARGET
*   ST Micro STM32 series processor (ARM Cortex-M3,M4).
*
* TOOLS
*   Rowley Crossworks for ARM v2.1
*   Editor tab stop = 4. Tabs converted to spaces.
*
*******************************************************************************
* (C) Copyright 2012 Puracom Inc.
* Calgary, Alberta, Canada, www.puracom.com
*******************************************************************************/

/** Module Feature Switches ***************************************************/

/** Include Files *************************************************************/

#include <stdio.h>


// PCOM Project Targets
#include "Target.h"

#include "Types.h"
#include "Gpio.h"
#include "Led.h"

#include "Timer.h"

/** Exportable Constants ******************************************************/

typedef struct
{
    GpioPinNamesEnum                eGpioPin;
    LedBlinkPatternEnum             eLedPatternCurrent;
    LedBlinkPatternEnum             eLedPatternNew;
    UINT32                          dwLedTimerMs;
    BOOL                            fLedIsOn;  /* Current led ON/OFF state */
    BOOL                            fLedIsBlinking;
    BOOL                            fLedIsEnabledCurrent;
    BOOL                            fLedIsEnabledPrevious;
} LedInterfaceLedLookupType;

/** Local Constants and types *************************************************/

#define LED_BLINK_TIME_SLOW_MS                  (500)
#define LED_BLINK_TIME_FAST_MS                  (100)

/** Exportable Variable Declarations ******************************************/

/** Local Variable Declarations ***********************************************/

static LedInterfaceLedLookupType tLedLookupArray[] =
{
    //eGpioPin          eLedPatternCurrent  eLedPatternNew      dwLedTimerMs    fLedIsOn        fLedIsBlinking  fLedIsEnabledCurrent    fLedIsEnabledPrevious
    { GPIO_LED1_OUT,    LED_PATTERN_OFF,    LED_PATTERN_OFF,    0,              FALSE,          FALSE,          TRUE,                   FALSE },
};

//static BOOL                     gfUpdateEnable = FALSE;

#define LED_NUMBER_OF_LEDS  ( sizeof(tLedLookupArray) / sizeof( tLedLookupArray[0] ) )


/** Local Function Prototypes *************************************************/
static void LED_setLed( LedHandleEnum eLedHandle, BOOL fEnable );

/** Functions *****************************************************************/

/******************************************************************************
* NAME          LedTaskInit( void )
*
* DESCRIPTION   Create new FreeRTOS thread for LED control.
*
* INPUTS        None.
*
* OUTPUTS       TRUE if the module was initialized successfully.
*               FALSE if the module could not be initialized.
******************************************************************************/
BOOL Led_Init( void )
{
    BOOL fReturn  = TRUE;

    if( LED_NUMBER_OF_LEDS != LED_ENUM_TOTAL )
    {
        // GPIO pin definition error
        while( 1 );
    }

    return fReturn;
}

//void Led_UpdateEnable( BOOL fEnable )
//{
//    gfUpdateEnable = fEnable;
//}

/******************************************************************************
* NAME          LED_enable( LedHandleEnum eLedIndex )
*
* DESCRIPTION   allow for LED to run a blinking patter
*
* INPUTS        eLedIndex
*
* OUTPUTS       None.
******************************************************************************/
BOOL LED_enable( LedHandleEnum eLedIndex, BOOL fEnabled )
{
    if( eLedIndex < LED_ENUM_TOTAL )
    {
        tLedLookupArray[ eLedIndex ].fLedIsEnabledCurrent = fEnabled;
        return TRUE;
    }

    return FALSE;
}

/******************************************************************************
* NAME          LED_task( void )
*
* DESCRIPTION   Implements the master controller thread
*
* INPUTS        
*
* OUTPUTS       None.
******************************************************************************/
void Led_Update( void )
{
    for( LedHandleEnum i = 0; i < LED_ENUM_TOTAL; i++ )
    {
        // update stat on change
        if( tLedLookupArray[ i ].fLedIsEnabledCurrent != tLedLookupArray[ i ].fLedIsEnabledPrevious )
        {
            tLedLookupArray[ i ].fLedIsEnabledPrevious = tLedLookupArray[ i ].fLedIsEnabledCurrent;

            if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
            {
                LED_setLed( (LedHandleEnum) i ,tLedLookupArray[ i ].fLedIsOn );

                // set timers for blinking option
                if( tLedLookupArray[ i ].fLedIsBlinking )
                {
                    switch( tLedLookupArray[ i ].eLedPatternCurrent )
                    {
                        case LED_PATTERN_SLOW:
                            tLedLookupArray[ i ].dwLedTimerMs = TimerDownTimerStartMs( LED_BLINK_TIME_SLOW_MS );
                            break;
                        case LED_PATTERN_FAST:
                            tLedLookupArray[ i ].dwLedTimerMs = TimerDownTimerStartMs( LED_BLINK_TIME_FAST_MS );
                            break;
                        default:
                            tLedLookupArray[ i ].eLedPatternCurrent = LED_PATTERN_OFF;
                            break;
                    } /* end switch() */
                }
            }
            else
            {
                LED_setLed( (LedHandleEnum) i ,FALSE );
            }
        }

        /* Check if the user has given a new pattern to use */
        if ( tLedLookupArray[ i ].eLedPatternCurrent != tLedLookupArray[ i ].eLedPatternNew )
        {
            /* Remember the pattern */
            tLedLookupArray[ i ].eLedPatternCurrent = tLedLookupArray[ i ].eLedPatternNew;

            /* Update the system to use the new pattern */
            switch( tLedLookupArray[ i ].eLedPatternCurrent )
            {
                case LED_PATTERN_OFF:
                    tLedLookupArray[ i ].fLedIsOn       = FALSE;
                    tLedLookupArray[ i ].fLedIsBlinking = FALSE;
                    if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
                    {
                        LED_setLed( (LedHandleEnum) i ,tLedLookupArray[ i ].fLedIsOn );
                    }
                    break;

                case LED_PATTERN_ON:
                    tLedLookupArray[ i ].fLedIsOn       = TRUE;
                    tLedLookupArray[ i ].fLedIsBlinking = FALSE;
                    if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
                    {
                        LED_setLed( (LedHandleEnum) i ,tLedLookupArray[ i ].fLedIsOn );
                    }
                    break;

                case LED_PATTERN_SLOW:
                    tLedLookupArray[ i ].fLedIsOn       = TRUE;
                    if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
                    {
                        LED_setLed( (LedHandleEnum) i ,tLedLookupArray[ i ].fLedIsOn );
                    }
                    
                    tLedLookupArray[ i ].fLedIsBlinking = TRUE;     /* LED is BLINKING */
                    tLedLookupArray[ i ].dwLedTimerMs   = TimerDownTimerStartMs( LED_BLINK_TIME_SLOW_MS );
                    break;

                case LED_PATTERN_FAST:
                    tLedLookupArray[ i ].fLedIsOn       = TRUE;
                    if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
                    {
                        LED_setLed( (LedHandleEnum) i ,tLedLookupArray[ i ].fLedIsOn );
                    }
                    
                    tLedLookupArray[ i ].fLedIsBlinking = TRUE;     /* LED is BLINKING */
                    tLedLookupArray[ i ].dwLedTimerMs   = TimerDownTimerStartMs( LED_BLINK_TIME_FAST_MS );
                    break;

                default:
                    tLedLookupArray[ i ].eLedPatternCurrent = LED_PATTERN_OFF;
                    tLedLookupArray[ i ].fLedIsOn           = FALSE;
                    tLedLookupArray[ i ].fLedIsBlinking     = FALSE;
                    break;
            }
        }
        else if( tLedLookupArray[ i ].fLedIsBlinking )
        {
            if( TimerDownTimerIsExpired( tLedLookupArray[ i ].dwLedTimerMs ) )
            {
                /* Toggle the LED */
                tLedLookupArray[ i ].fLedIsOn = !tLedLookupArray[ i ].fLedIsOn;

                if( tLedLookupArray[ i ].fLedIsEnabledCurrent )
                {
                    LED_setLed( (LedHandleEnum) i, tLedLookupArray[ i ].fLedIsOn );
                }

                switch( tLedLookupArray[ i ].eLedPatternCurrent )
                {
                    case LED_PATTERN_SLOW:
                        tLedLookupArray[ i ].dwLedTimerMs = TimerDownTimerStartMs( LED_BLINK_TIME_SLOW_MS );
                        break;
                    case LED_PATTERN_FAST:
                        tLedLookupArray[ i ].dwLedTimerMs = TimerDownTimerStartMs( LED_BLINK_TIME_FAST_MS );
                        break;
                    default:
                        tLedLookupArray[ i ].eLedPatternCurrent = LED_PATTERN_OFF;
                        break;
                } /* end switch() */
            }
        }
    }       /* end for() */
}

/******************************************************************************
* NAME          LedInterfaceSetLed( LedInterfaceLedHandleEnum eLedHandle, BOOL fEnable )
*
* DESCRIPTION   Turns the led on and off.
*
* INPUTS        eLedHandle
*               fEnable
*
* OUTPUTS       None.
******************************************************************************/
void LED_setLed( LedHandleEnum eLedHandle, BOOL fEnable )
{
    if( eLedHandle < LED_ENUM_TOTAL )
    {
        GpioWrite( tLedLookupArray[eLedHandle].eGpioPin, fEnable );
    }
}

/******************************************************************************
* NAME          LED_setLedBlinkPattern( LedHandleEnum eLedHandle, LedBlinkPatternEnum ePattern )
*
* DESCRIPTION   Changes the pattern of the Led passed as argument
*
* OUTPUTS       TRUE if the pattern was changed successfully
*               FALSE if there was an error
******************************************************************************/
BOOL LED_setLedBlinkPattern( LedHandleEnum eLedHandle, LedBlinkPatternEnum ePattern )
{
    /* Sanity check the enum argument */
    if( eLedHandle < LED_ENUM_TOTAL )
    {
        tLedLookupArray[eLedHandle].eLedPatternNew = ePattern;
        return TRUE;
    }

    return FALSE;
}

BOOL LED_getCurrentLedBlinkPattern( LedHandleEnum eLedHandle, LedBlinkPatternEnum *pePattern )
{
    /* Sanity check the enum argument */
    if
    (
        ( eLedHandle < LED_ENUM_TOTAL ) &&
        ( NULL != pePattern )
    )
    {
        (*pePattern) = tLedLookupArray[eLedHandle].eLedPatternCurrent;
        return TRUE;
    }
    
    return FALSE;
}

/* EOF */