/** C Header ******************************************************************

*******************************************************************************/

#ifndef LED_PATTERN_H
#define LED_PATTERN_H

typedef enum
{
    LED1_SYSTEM_OPERATIONS = 0,
    LED1_REED_SWITCH_OPERATIONS,
    
    LED_ENUM_MAX,
}LedPatternLedListEnum;

/* Available LED patterns */
typedef enum
{
    LED_PATTERN_SOLID_OFF, 
    LED_PATTERN_SOLID_ON, 
    LED_PATTERN_BLINK_SLOW_SINGLE,// connecting
    LED_PATTERN_BLINK_SLOW_DOUBLE,  // magnet swipe
    LED_PATTERN_BLINK_FAST, // connected
    LED_PATTERN_BLINK_SUPER_FAST, // data transfer
    
    LED_PATTERN_BLINK_AKNOWLEDGE_F1,// magnet swipe F1
    LED_PATTERN_BLINK_AKNOWLEDGE_F2,// magnet swipe F2
    LED_PATTERN_BLINK_AKNOWLEDGE_F3,// magnet swipe F3
    LED_PATTERN_BLINK_AKNOWLEDGE_NO_F,// magnet swipe no function
    
    LED_PATTERN_LIST_MAX,
}LedPatternPatternListEnum;

BOOL LedPatternInit                     ( void );
void LedPatternUpdate                   ( void );

// TODO: add option to enable/disable loop pattern.

BOOL LedPatternLedPatternEnable         ( LedPatternLedListEnum eLed, BOOL fEnabled );
BOOL LedPatternLedIsPatternEnabled      ( LedPatternLedListEnum eLed );
BOOL LedPatternLedPatternLoopEnable     ( LedPatternLedListEnum eLed, BOOL fEnabled );
BOOL LedPatternLedIsPatternLoopEnable   ( LedPatternLedListEnum eLed );
BOOL LedPatternSetBlinkPattern          ( LedPatternLedListEnum eLed, LedPatternPatternListEnum ePattern );
BOOL LedPatternGetBlinkPattern          ( LedPatternLedListEnum eLed, LedPatternPatternListEnum *pePattern );

CHAR *LedPatternGetLedName              ( LedPatternLedListEnum eLed );
CHAR *LedPatternGetPatternName          ( LedPatternPatternListEnum ePattern );

#endif /* LED_PATTERN_H */

/* EOF */

