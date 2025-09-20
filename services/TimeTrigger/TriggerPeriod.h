/** C Header ******************************************************************

*******************************************************************************/

#ifndef TRIGGER_PERIOD_H
#define TRIGGER_PERIOD_H

typedef enum
{
    TRIGGER_PERIOD_TYPE_SECONDS,
    TRIGGER_PERIOD_TYPE_MINUTES,
    TRIGGER_PERIOD_TYPE_HOURS,
    TRIGGER_PERIOD_TYPE_DAYS,
    
    TRIGGER_PERIOD_TYPE_MAX,
}TriggerPeriodTypeEnum;

typedef enum
{
	TRIGGER_PERIOD_ACTION_TEST_SECOND,
    TRIGGER_PERIOD_ACTION_TEST_MINUTE,
	TRIGGER_PERIOD_ACTION_TEST_HOUR,
	TRIGGER_PERIOD_ACTION_TEST_DAY,
    
    TRIGGER_PERIOD_ACTION_MAX,
}TriggerPeriodActionEnum;

BOOL 	TriggerPeriodInit                       ( void );
void 	TriggerPeriodUpdate                     ( TriggerPeriodActionEnum eAction, UINT32 dwCurrentEpochSeconds );

// trigger stat
BOOL 	TriggerPeriodEnableActionTrigger        ( TriggerPeriodActionEnum eAction, BOOL fEnable );
BOOL 	TriggerPeriodIsTriggerEnabled           ( TriggerPeriodActionEnum eAction );
BOOL 	TriggerPeriodIsActionTriggered          ( TriggerPeriodActionEnum eAction );
BOOL 	TriggerPeriodClearActionTriggeredFlag   ( TriggerPeriodActionEnum eAction );

// configure triggering mode values
BOOL  	TriggerPeriodSetPeriodType          	( TriggerPeriodActionEnum eAction, TriggerPeriodTypeEnum ePeriodType );
BOOL  	TriggerPeriodGetPeriodType          	( TriggerPeriodActionEnum eAction, TriggerPeriodTypeEnum *pePeriodType );
BOOL  	TriggerPeriodSetPeriodValue         	( TriggerPeriodActionEnum eAction, UINT16 wValueDependingOnPeriodType );
INT32 	TriggerPeriodGetPeriodValue         	( TriggerPeriodActionEnum eAction );

CHAR   *TriggerPeriodGetActionName              ( TriggerPeriodActionEnum eAction );
CHAR   *TriggerPeriodGetPeriodTypeName          ( TriggerPeriodTypeEnum ePeriodType );

#endif /* TRIGGER_PERIOD_H */

/* EOF */

