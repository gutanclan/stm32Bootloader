/********************************************

File:  			UserIf.c

Description:	User Interface Module. 
NOTE:           The user interface elements that this module interact with
                are located in the interface board.

Author: 		

Date:			

*********************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _USERIF_H_
#define _USERIF_H_
///////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    USER_IF_REED_SWITCH_1,
    USER_IF_REED_SWITCH_2,

    USER_IF_REED_SWITCH_MAX
} UserIfReedSwitchEnum;

BOOL            UsertIfInterruptPinReedSwtchEnable          ( UserIfReedSwitchEnum eReedSwitch, BOOL fInterruptEnable );
BOOL            UserIfIsReedSwitchInterruptEnabled          ( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsEnabled );
void            UserIfPrintInterruptStatus                  ( ConsolePortEnum eConsole );
void            UserIfReedSwitchFunctionTriggerStateMachine ( void );
BOOL            UserIfIsReedSwitchBussy                     ( UserIfReedSwitchEnum eReedSwitch );
BOOL            UserIfIsAnyReedSwitchBussy                  ( void );

///////////////////////////////////////////////////////////////////////////////////////////////
#endif // _USERIF_H_
///////////////////////////////////////////////////////////////////////////////////////////////
