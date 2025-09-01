/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>
#include "Target.h"
#include "Types.h"
#include "Gpio.h"
#include "LedPattern.h"
#include "Timer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

#define LED_PATTERN_MAX_NUM_OF_STATES               (8)
#define LED_PATTERN_END_OF_PATTERN_TIME_INDICATOR   (0)

typedef struct
{
    CHAR * const    pcName;
    INT16           iwOnOffState[LED_PATTERN_MAX_NUM_OF_STATES];
    INT16           iwOnOffTimeMs[LED_PATTERN_MAX_NUM_OF_STATES];
}LedPatternOnOffPatternStruct;

typedef struct
{
    LedPatternPatternListEnum       eLedPatternNew;
    LedPatternPatternListEnum       eLedPatternCurrent;    
    TIMER                           tTimer;
    UINT8                           bPatternIndex;
}LedPatternStateStruct;

typedef struct
{
    CHAR * const                    pcName;
    GpioPinNamesEnum                eGpioPin;
    BOOL                            fLoopEnabled;
    BOOL                            fIsPatternEnabled;
    LedPatternStateStruct           tPatternState;
}LedPatternStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Patterns
////////////////////////////////////////////////////////////////////////////////////////////////////
// if time value for that particular index of the array is zero then the pattern will go back to beginning
static const LedPatternOnOffPatternStruct tLedPatternOnOffArray[] =
{
    // SOLID OFF
    {
        // pcName
        "OFF",
        //State
        {0},
        //TimeMs
        {100}
    },
    // SOLID ON
    {
        // pcName
        "ON",
        //State
        {1},
        //TimeMs
        {100}
    },
    // SLOW_SINGLE
    {
        // pcName
        "SLOW_SINGLE",
        //State
        {0,1},
        //TimeMs
        {900,100}
    },
    
    // SLOW_DOUBLE
    {
        // pcName
        "SLOW_DOUBLE",
        //State
        {0,1,0,1},
        //TimeMs
        {700,110,80,110}
    },
    
    // FAST
    {
        // pcName
        "FAST",
        //State
        {0,1},
        //TimeMs
        {300,100}
    },
    
    // SUPER FAST
    {
        // pcName
        "SUPER FAST",
        //State
        {1,0},
        //TimeMs
        {25,25}
    },
    // LED_PATTERN_BLINK_AKNOWLEDGE_F1
    {
        // pcName
        "Function 1",
        //State
        {1,0,1,0},
        //TimeMs
        {2000,500,100,3000}
    },
    // LED_PATTERN_BLINK_AKNOWLEDGE_F2
    {
        // pcName
        "Function 2",
        //State
        {1,0,1,0,1,0},
        //TimeMs
        {2000,500,100,500,100,3000}
    },
    // LED_PATTERN_BLINK_AKNOWLEDGE_F3
    {
        // pcName
        "Function 3",
        //State
        {1,0,1,0,1,0,1,0},
        //TimeMs
        {2000,500,100,500,100,500,100,3000}
    },
    // LED_PATTERN_BLINK_AKNOWLEDGE_NO_F
    {
        // pcName
        "No Function",
        //State
        {1,0},
        //TimeMs
        {2000,3000}
    },
};

////////////////////////////////////////////////////////////////////////////////////////////////////

static LedPatternStruct gtLedListLookupArray[] =
{
    // LED1_SYSTEM_OPERATIONS
    {
        // pcName
        "Sys Operations",
        // eGpioPin
        GPIO_LED1_OUT,
        // fLoopEnabled
        FALSE,
        // fIsPatternEnabled
        FALSE,
        // tPatternState
        {
            // eLedPatternNew
            LED_PATTERN_SOLID_OFF,
            // eLedPatternCurrent
            LED_PATTERN_SOLID_OFF,
            // ].tPatternState.tTimer
            0,
            // bPatternIndex
            0
        }
    },
    // LED1_REED_SWITCH_OPERATIONS
    {
        // pcName
        "RS Operations",
        // eGpioPin
        GPIO_LED1_OUT,
        // fLoopEnabled
        FALSE,
        // fIsPatternEnabled
        FALSE,
        // tPatternState
        {
            // eLedPatternNew
            LED_PATTERN_SOLID_OFF,
            // eLedPatternCurrent
            LED_PATTERN_SOLID_OFF,
            // ].tPatternState.tTimer
            0,
            // bPatternIndex
            0
        }
    },
};

#define LED_PATTERN_LED_LIST_SIZE  ( sizeof(gtLedListLookupArray) / sizeof( gtLedListLookupArray[0] ) )

////////////////////////////////////////////////////////////////////////////////

static BOOL LedPatternSetLed( LedPatternLedListEnum eLed, BOOL fEnable );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternInit( void )
{
    BOOL fReturn  = TRUE;

    if( LED_ENUM_MAX != LED_PATTERN_LED_LIST_SIZE )
    {
        // GPIO pin definition error
        while( 1 );
    }

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternLedPatternEnable( LedPatternLedListEnum eLed, BOOL fEnabled )
{
    BOOL fSuccess = FALSE;
    
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        gtLedListLookupArray[ eLed ].fIsPatternEnabled = fEnabled;
        
        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternLedIsPatternEnabled( LedPatternLedListEnum eLed )
{
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        return gtLedListLookupArray[ eLed ].fIsPatternEnabled;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternLedPatternLoopEnable( LedPatternLedListEnum eLed, BOOL fEnabled )
{
    BOOL fSuccess = FALSE;
    
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        gtLedListLookupArray[ eLed ].fLoopEnabled = fEnabled;
        
        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternLedIsPatternLoopEnable( LedPatternLedListEnum eLed )
{
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        return gtLedListLookupArray[ eLed ].fLoopEnabled;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternSetBlinkPattern( LedPatternLedListEnum eLed, LedPatternPatternListEnum ePattern )
{
    BOOL fSuccess = FALSE;
    
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        if( ePattern < LED_PATTERN_LIST_MAX )
        {
            gtLedListLookupArray[ eLed ].tPatternState.eLedPatternNew = ePattern;
            
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternGetBlinkPattern( LedPatternLedListEnum eLed, LedPatternPatternListEnum *pePattern )
{
    BOOL fSuccess = FALSE;
    
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {        
        if( pePattern != NULL )
        {
            (*pePattern) = gtLedListLookupArray[ eLed ].tPatternState.eLedPatternCurrent;
            
            fSuccess = TRUE;
        }   
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LedPatternSetLed( LedPatternLedListEnum eLed, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        GpioWrite( gtLedListLookupArray[ eLed ].eGpioPin, fEnable );
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void LedPatternUpdate( void )
{
    for( LedPatternLedListEnum i = 0; i < LED_PATTERN_LED_LIST_SIZE; i++ )
    {
        if( gtLedListLookupArray[ i ].fIsPatternEnabled )
        {
            // check if pattern has changed.
            // if change, start new pattern from index 0 and restart timer
            if( gtLedListLookupArray[ i ].tPatternState.eLedPatternNew != gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent )
            {
                // update pattern
                gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent = gtLedListLookupArray[ i ].tPatternState.eLedPatternNew;
                
                // reset stat vars
                gtLedListLookupArray[ i ].tPatternState.bPatternIndex= 0;
                gtLedListLookupArray[ i ].tPatternState.tTimer = TimerDownTimerStartMs( tLedPatternOnOffArray[gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent].iwOnOffTimeMs[gtLedListLookupArray[ i ].tPatternState.bPatternIndex] );
                // set led to initial state
                LedPatternSetLed( i, tLedPatternOnOffArray[gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent].iwOnOffState[gtLedListLookupArray[ i ].tPatternState.bPatternIndex] );
            }
            
            // check if time expired to change state of pattern
            if( TimerDownTimerIsExpired( gtLedListLookupArray[ i ].tPatternState.tTimer ) )
            {
                // check if next index of pattern is inside range or state time is not zero(means index is invalid so go back to start)
                if( (gtLedListLookupArray[ i ].tPatternState.bPatternIndex+ 1) < LED_PATTERN_MAX_NUM_OF_STATES )
                {
                    if( tLedPatternOnOffArray[gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent].iwOnOffTimeMs[gtLedListLookupArray[ i ].tPatternState.bPatternIndex+ 1] > 0 )
                    {
                        gtLedListLookupArray[ i ].tPatternState.bPatternIndex++;
                    }
                    else
                    {
                        if( gtLedListLookupArray[ i ].fLoopEnabled )
                        {
                            gtLedListLookupArray[ i ].tPatternState.bPatternIndex= 0;
                        }
                        else
                        {
                            // if not looping, disable LED pattern when pattern reaches the end
                            gtLedListLookupArray[ i ].fIsPatternEnabled = FALSE;
                        }
                    }
                }
                else
                {
                    if( gtLedListLookupArray[ i ].fLoopEnabled )
                    {
                        gtLedListLookupArray[ i ].tPatternState.bPatternIndex= 0;
                    }
                    else
                    {
                        // if not looping, disable LED pattern when pattern reaches the end
                        gtLedListLookupArray[ i ].fIsPatternEnabled = FALSE;
                    }
                }
                
                gtLedListLookupArray[ i ].tPatternState.tTimer = TimerDownTimerStartMs( tLedPatternOnOffArray[gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent].iwOnOffTimeMs[gtLedListLookupArray[ i ].tPatternState.bPatternIndex] );
                // set led to initial state
                LedPatternSetLed( i, tLedPatternOnOffArray[gtLedListLookupArray[ i ].tPatternState.eLedPatternCurrent].iwOnOffState[gtLedListLookupArray[ i ].tPatternState.bPatternIndex] );
            }
        }
    }       /* end for() */
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * LedPatternGetLedName( LedPatternLedListEnum eLed )
{
    if( eLed < LED_PATTERN_LED_LIST_SIZE )
    {
        return &gtLedListLookupArray[ eLed ].pcName[0];
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * LedPatternGetPatternName( LedPatternPatternListEnum ePattern )
{
    if( ePattern < LED_PATTERN_LIST_MAX )
    {
        // pcName
        return &tLedPatternOnOffArray[ePattern].pcName[0];
    }

    return NULL;
}

/* EOF */