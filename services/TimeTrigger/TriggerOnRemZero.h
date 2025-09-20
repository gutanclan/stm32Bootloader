/** C Header ******************************************************************

*******************************************************************************/

#ifndef TRIGGER_ON_REM_ZERO_H
#define TRIGGER_ON_REM_ZERO_H

typedef enum
{
    TRIGGER_ON_REM_ZERO_TIME_TYPE_SECOND = 0,
    TRIGGER_ON_REM_ZERO_TIME_TYPE_MINUTE,
    TRIGGER_ON_REM_ZERO_TIME_TYPE_HOUR,
    TRIGGER_ON_REM_ZERO_TIME_TYPE_DAY,
    
    TRIGGER_ON_REM_ZERO_TIME_TYPE_MAX,
}TriggerOnRemZeroTimeTypeEnum;

typedef enum
{
    TRIGGER_ON_REM_ZERO_ACTION_TEST_1,
    
    TRIGGER_ON_REM_ZERO_ACTION_MAX,
}TriggerOnRemZeroActionEnum;

BOOL            TriggerOnRemZeroInit                       ( void );
void            TriggerOnRemZeroUpdate                     ( TriggerOnRemZeroActionEnum eAction, UINT32 dwCurrentEpochSeconds );

// trigger stat
BOOL            TriggerOnRemZeroEnableActionTrigger        ( TriggerOnRemZeroActionEnum eAction, BOOL fEnable );
BOOL            TriggerOnRemZeroIsTriggerEnabled           ( TriggerOnRemZeroActionEnum eAction );
BOOL            TriggerOnRemZeroIsActionTriggered          ( TriggerOnRemZeroActionEnum eAction );
BOOL            TriggerOnRemZeroClearActionTriggeredFlag   ( TriggerOnRemZeroActionEnum eAction );

// configure triggering mode values
BOOL            TriggerOnRemZeroSetTimeType                ( TriggerOnRemZeroActionEnum eAction, TriggerOnRemZeroTimeTypeEnum eChangeType );
BOOL            TriggerOnRemZeroGetTimeType                ( TriggerOnRemZeroActionEnum eAction, TriggerOnRemZeroTimeTypeEnum *peChangeType );
BOOL            TriggerOnRemZeroSetTimeValue               ( TriggerOnRemZeroActionEnum eAction, UINT16 wValueType );
INT32           TriggerOnRemZeroGetTimeValue               ( TriggerOnRemZeroActionEnum eAction );

CHAR           *TriggerOnRemZeroGetActionName              ( TriggerOnRemZeroActionEnum eAction );
CHAR           *TriggerOnRemZeroGetTimeTypeName            ( TriggerOnRemZeroTimeTypeEnum eChangeType );

#endif /* TRIGGER_ON_REM_ZERO_H */

/* EOF */

