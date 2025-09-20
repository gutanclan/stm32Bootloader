/** C Header ******************************************************************

*******************************************************************************/

#ifndef TRIGGER_ON_CHANGE_H
#define TRIGGER_ON_CHANGE_H

typedef enum
{
    TRIGGER_ON_CHANGE_TYPE_SECOND,
    TRIGGER_ON_CHANGE_TYPE_MINUTE,
    TRIGGER_ON_CHANGE_TYPE_HOUR,
    TRIGGER_ON_CHANGE_TYPE_DAY,
    
    TRIGGER_ON_CHANGE_TYPE_MAX,
}TriggerOnChangeTypeEnum;

typedef enum
{
    TRIGGER_ON_CHANGE_ACTION_TEST_1
    
    TRIGGER_ON_CHANGE_ACTION_MAX,
}TriggerOnChangeActionEnum;

BOOL            TriggerOnChangeInit                       ( void );
void            TriggerOnChangeUpdate                     ( TriggerOnChangeActionEnum eAction, RtcDateTimeStruct *ptDateTime );

// trigger stat
BOOL            TriggerOnChangeEnableActionTrigger        ( TriggerOnChangeActionEnum eAction, BOOL fEnable );
BOOL            TriggerOnChangeIsTriggerEnabled           ( TriggerOnChangeActionEnum eAction );
BOOL            TriggerOnChangeIsActionTriggered          ( TriggerOnChangeActionEnum eAction );
BOOL            TriggerOnChangeClearActionTriggeredFlag   ( TriggerOnChangeActionEnum eAction );

// configure triggering mode values
BOOL            TriggerOnChangeSetChangeType              ( TriggerOnChangeActionEnum eAction, TriggerOnChangeTypeEnum eChangeType );
BOOL            TriggerOnChangeGetChangeType              ( TriggerOnChangeActionEnum eAction, TriggerOnChangeTypeEnum *peChangeType );

CHAR           *TriggerOnChangeGetActionName              ( TriggerOnChangeActionEnum eAction );
CHAR           *TriggerOnChangeGetChangeTypeName          ( TriggerOnChangeTypeEnum eChangeType );

#endif /* TRIGGER_ON_CHANGE_H */

/* EOF */

