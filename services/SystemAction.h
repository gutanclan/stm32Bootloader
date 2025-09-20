/** C Header ******************************************************************

*******************************************************************************/

#ifndef SYSTEM_ACTION_H
#define SYSTEM_ACTION_H

typedef enum
{
    // NOTE: this enum is not really an action that can be 
    // triggered since it will be running on every loop of control task.
    // It can only be used to enable/disable the sate machine.
    // #0-9
    SYSTEM_ACTION_STATE_MACHINE,        
    SYSTEM_ACTION_DELAY_WAIT,

    // actions related to sensors and external devices
    // #10-19
    SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_SHORT,
    SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_LONG,
    SYSTEM_ACTION_CALC_HOURLY_DATA,
    SYSTEM_ACTION_CAPTURE_IMAGE,
    SYSTEM_ACTION_P_COUNTER_GET_COUNT,
    SYSTEM_ACTION_P_COUNT_TO_HOURLY,
    SYSTEM_ACTION_MODBUS_READ,
    SYSTEM_ACTION_P_COUNTER_GET_DELTA,
    SYSTEM_ACTION_GET_FLUID_DEPTH,
    SYSTEM_ACTION_UPDATE_SUNRISE_TIME,
    SYSTEM_ACTION_RUN_RELAY,
    
    // actions related to files
    // #20-29
    SYSTEM_ACTION_SIG_FILE_CLOSE_QUEUE,
    SYSTEM_ACTION_SIG_FILE_TO_HOURLY,
    SYSTEM_ACTION_DATA_FILE_CLOSE_QUEUE,
    SYSTEM_ACTION_MIN_FILE_CLOSE_QUEUE,
    SYSTEM_ACTION_EVT_FILE_CLOSE_QUEUE,
    SYSTEM_ACTION_BACKUP_FILE_CLOSE_QUEUE,
    
    // transmissions
    // #30-59
    SYSTEM_ACTION_SEND_QUEUED_FILES,
    SYSTEM_ACTION_SEND_SRVR_MSG,
    SYSTEM_ACTION_DOWNLOAD_SCRIPT,
    SYSTEM_ACTION_DOWNLOAD_MAIN_BOARD_FW,
    SYSTEM_ACTION_DOWNLOAD_CAMERA_FW,
    SYSTEM_ACTION_DOWNLOAD_TOTAL_FW,
    SYSTEM_ACTION_DELETE_SCRIPT_FROM_SRVR,
    SYSTEM_ACTION_DELETE_MAINB_FW_FROM_SRVR,    
    SYSTEM_ACTION_DELETE_CAM_FW_FROM_SRVR, 
    SYSTEM_ACTION_DELETE_TOTAL_FW_FROM_SRVR,
    SYSTEM_ACTION_SYNC_SYS_TIME,
    SYSTEM_ACTION_SEND_PCOUNTER_DELTA_COUNT,

    SYSTEM_ACTION_DOWNLOAD_FDM_FW,
    SYSTEM_ACTION_DELETE_FDM_FW_FROM_SRVR,

    // run script and update firmware
    // #60-69
    SYSTEM_ACTION_RUN_SCRIPT,    
    SYSTEM_ACTION_UPDATE_MAIN_BOARD_FW,
    SYSTEM_ACTION_UPDATE_CAMERA_FW,
    SYSTEM_ACTION_UPDATE_TOTAL_FW,
    SYSTEM_ACTION_UPDATE_FDM_FW,
    SYSTEM_ACTION_UPDATE_RELAY_FW,
    
    // actions related to exit system
    // #70-79
    SYSTEM_ACTION_EXIT_SHUT_DOWN,
    SYSTEM_ACTION_EXIT_RESTART,
    SYSTEM_ACTION_EXIT_SLEEP,

    SYSTEM_ACTION_MAX,
}SystemActionEnum;

BOOL    SystemActionInit                ( void );
void    SystemActionUpdate              ( void );

BOOL    SystemActionEnableAction        ( SystemActionEnum eAction, BOOL fEnable );
BOOL    SystemActionIsActionEnable      ( SystemActionEnum eAction );
void    SystemActionEnableAll           ( BOOL fEnable );
void    SystemActionEnableTransmissions ( BOOL fEnable );

BOOL    SystemActionRequestAction       ( SystemActionEnum eAction );
BOOL    SystemActionIsActionRequested   ( SystemActionEnum eAction );

BOOL    SystemActionTriggerAction       ( SystemActionEnum eAction, BOOL fTrigger );
BOOL    SystemActionIsActionTriggered   ( SystemActionEnum eAction );

UINT16  SystemActionGetActionId         ( SystemActionEnum eAction );
CHAR *  SystemActionGetActionName       ( SystemActionEnum eAction );

#endif /* SYSTEM_ACTION_H */

/* EOF */

