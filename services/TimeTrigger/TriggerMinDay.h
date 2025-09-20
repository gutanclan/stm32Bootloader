/** C Header ******************************************************************

*******************************************************************************/

#ifndef TRIGGER_MIN_DAY_H
#define TRIGGER_MIN_DAY_H

#define TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MIN     (0)
#define TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MAX     (1439)

typedef enum
{
    TRIGGER_MIN_DAY_SCHEDULE_DEFAULT,
    TRIGGER_MIN_DAY_SCHEDULE_LOW_POWER,
    TRIGGER_MIN_DAY_SCHEDULE_CONFIGURABLE_1,
    
    TRIGGER_MIN_DAY_SCHEDULE_MAX
}TriggerMinDayScheduleEnum;

typedef enum
{
    TRIGGER_MIN_DAY_ACTION_TEST_1
    
    TRIGGER_MIN_DAY_ACTION_MAX
}TriggerMinDayActionEnum;

typedef struct
{
    UINT8 			bHour;          // 0..23, Hour in 24-hour format
	UINT8 			bMinute;        // 0..59, Minute	
}TriggerMinDayTimeStruct;

BOOL            TriggerMinDayInit                       ( void );
void            TriggerMinDayUpdate                     ( TriggerMinDayActionEnum eAction, UINT16 wCurrentMinuteInTheDay );

// configure triggering
BOOL            TriggerMinDayEnableActionTrigger        ( TriggerMinDayActionEnum eAction, BOOL fEnable );
BOOL            TriggerMinDayIsTriggerEnabled           ( TriggerMinDayActionEnum eAction );
BOOL            TriggerMinDayIsActionTriggered          ( TriggerMinDayActionEnum eAction );
BOOL            TriggerMinDayClearActionTriggeredFlag   ( TriggerMinDayActionEnum eAction );

BOOL            TriggerMinDaySetCurrentSchedule         ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule );
BOOL            TriggerMinDayGetCurrentSchedule         ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum *peSchedule );
BOOL            TriggerMinDayIsScheduleConfigurable     ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule );

INT16           TriggerMinDayGetScheduleMinutesAmount   ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule );
INT16           TriggerMinDayGetScheduleMinuteAtIndex   ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule, INT16 iwIndex );
BOOL            TriggerMinDaySetScheduleMinuteAtIndex   ( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule, INT16 iwIndex, INT16 iwMinOffsetOfDay );

// operations
BOOL            TriggerMinDayConvertMinOfDayToDayTime   ( INT16 iwMinOffsetOfDay, TriggerMinDayTimeStruct *ptTime );
INT16           TriggerMinDayConvertDayTimeToMinOfDay   ( TriggerMinDayTimeStruct *ptTime );
INT16           TriggerMinDayAddOffsetToMinOfDay        ( INT16 iwMinOffsetOfDay, UINT16 wOffset, BOOL fIsPossitiveOffset );

CHAR           *TriggerMinDayGetActionName              ( TriggerMinDayActionEnum eAction );
CHAR           *TriggerMinDayGetScheduleName            ( TriggerMinDayScheduleEnum eSchedule );

#endif /* TRIGGER_MIN_DAY_H */

/* EOF */

