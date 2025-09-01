/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>

#include "Target.h"
#include "Types.h"
#include "Gpio.h"
#include "InterfaceBoard.h"
#include "..\Utils\StateMachine.h"
#include "Power.h"
#include "Timer.h"
#include "Adc.h"
#include "Rtc.h"
#include "Control.h"

#include "ControlAdcReading.h"

#include "SystemAction.h"

#include "Scheduler/TriggerMinDay.h"
#include "Scheduler/TriggerPeriod.h"
#include "Scheduler/TriggerOnChange.h"
#include "Scheduler/TriggerOnEpochRemZero.h"

#include "ControlFileTransferRetry.h"

#include "PCountDeltaQueue.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    const CHAR *     const      pcName;
    const UINT16                wActionId;
    BOOL                        fIsEnabled;
    BOOL                        fIsRequested;
    BOOL                        fIsTriggered;
}SystemActionStatStruct;

//////////////////////////////////////////////////////////////////////////////////////////////////

static SystemActionStatStruct gtActionArray[SYSTEM_ACTION_MAX] = 
{
    // pcName                       Id                  fIsFlagSet
    {   "STATE_MACHINE",            0,                  0,0,0    },
    {   "DELAY_WAIT",               1,                  0,0,0    },
    
    {   "SENSOR_SAMPLE_SHORT",      10,                 0,0,0    },
    {   "SENSOR_SAMPLE_LONG",       11,                 0,0,0    },
    {   "CALC_HOURLY_AVERAGE",      12,                 0,0,0    },
    {   "CAPTURE_IMAGE",            13,                 0,0,0    },
    {   "P_COUNTER_GET_COUNT",      14,                 0,0,0    },
    {   "P_COUNT_TO_HOURLY",        15,                 0,0,0    },
    {   "MODBUS_REGISTER_COMMANDS", 16,                 0,0,0    },
    {   "P_COUNTER_GET_DELTA",      17,                 0,0,0    },
    {   "GET_FLUID_DEPTH",          18,                 0,0,0    },
    {   "UPDATE_SUNRISE",           19,                 0,0,0    },
    {   "RUN_RELAY",                110,                0,0,0    },
    
    {   "CLOSE_Q_SIG_FILE",         20,                 0,0,0    },
    {   "SIG_FILE_TO_HOURLY",       21,                 0,0,0    },
    {   "CLOSE_Q_DATA_FILE",        22,                 0,0,0    },
    {   "CLOSE_Q_MIN_FILE",         23,                 0,0,0    },
    {   "CLOSE_Q_EVENT_FILE",       28,                 0,0,0    },
    {   "CLOSE_Q_CONFIG_FILE",      29,                 0,0,0    },
    
    {   "SEND_QUEUED_FILES",        30,                 0,0,0    },
    {   "SEND_SERVER_MSG",          31,                 0,0,0    },
    {   "DOWNLOAD_SCRIPT",          32,                 0,0,0    },
    {   "DOWNLOAD_FW_MAINB",        33,                 0,0,0    },
    {   "DOWNLOAD_FW_CAM",          34,                 0,0,0    },
    {   "DOWNLOAD_FW_PCOUNTR",      35,                 0,0,0    },
    {   "DELETE_SCRIPT",            36,                 0,0,0    },
    {   "DELETE_FW_MAINB",          37,                 0,0,0    },
    {   "DELETE_FW_CAM",            38,                 0,0,0    },
    {   "DELETE_FW_PCOUNTR",        39,                 0,0,0    },
    {   "SYNC_SYS_TIME",            40,                 0,0,0    },
    {   "SEND_PCOUNT_DELTA",        41,                 0,0,0    },
    {   "DOWNLOAD_FW_FDM",          42,                 0,0,0    },
    {   "DELETE_FW_FDM",            43,                 0,0,0    },
    
    {   "RUN_SCRIPT",               60,                 0,0,0    },
    {   "UPDATE_FW_MAINB",          61,                 0,0,0    },
    {   "UPDATE_FW_CAM",            62,                 0,0,0    },
    {   "UPDATE_FW_PCOUNT",         63,                 0,0,0    },
    {   "UPDATE_FW_FDM",            64,                 0,0,0    },
    {   "UPDATE_FW_RELAY",          65,                 0,0,0    },
    
    {   "SYSTEM_EXIT_SHUT_DHOWN",   70,                 0,0,0    },
    {   "SYSTEM_EXIT_RESTART",      71,                 0,0,0    },
    {   "SYSTEM_EXIT_SLEEP",        72,                 0,0,0    },
};

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SystemActionInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionEnableAction        ( SystemActionEnum eAction, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        gtActionArray[eAction].fIsEnabled = fEnable;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionIsActionEnable      ( SystemActionEnum eAction )
{
    BOOL fIsEnabled = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        fIsEnabled = gtActionArray[eAction].fIsEnabled;
    }
    
    return fIsEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void    SystemActionEnableAll           ( BOOL fEnable )
{
    for( UINT8 bAction = 0 ; bAction < SYSTEM_ACTION_MAX ; bAction++ )
    {
        SystemActionEnableAction( bAction, fEnable );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void    SystemActionEnableTransmissions ( BOOL fEnable )
{
    SystemActionEnableAction( SYSTEM_ACTION_SEND_QUEUED_FILES, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_SEND_SRVR_MSG, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DOWNLOAD_SCRIPT, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DOWNLOAD_MAIN_BOARD_FW, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DOWNLOAD_CAMERA_FW, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DOWNLOAD_TOTAL_FW, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DOWNLOAD_FDM_FW, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DELETE_SCRIPT_FROM_SRVR, fEnable ); 
    SystemActionEnableAction( SYSTEM_ACTION_DELETE_MAINB_FW_FROM_SRVR,  fEnable );   
    SystemActionEnableAction( SYSTEM_ACTION_DELETE_CAM_FW_FROM_SRVR,  fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DELETE_TOTAL_FW_FROM_SRVR, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_DELETE_FDM_FW_FROM_SRVR, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_SYNC_SYS_TIME, fEnable );
    SystemActionEnableAction( SYSTEM_ACTION_SEND_PCOUNTER_DELTA_COUNT, fEnable );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionRequestAction       ( SystemActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        if( gtActionArray[eAction].fIsEnabled )
        {
            gtActionArray[eAction].fIsRequested = TRUE;
        }
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionIsActionRequested   ( SystemActionEnum eAction )
{
    BOOL fIsRequested = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        fIsRequested = gtActionArray[eAction].fIsRequested;
    }
    
    return fIsRequested;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionTriggerAction       ( SystemActionEnum eAction, BOOL fTrigger )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        gtActionArray[eAction].fIsTriggered = fTrigger;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    SystemActionIsActionTriggered   ( SystemActionEnum eAction )
{
    BOOL fIsTriggered = FALSE;
    
    if( eAction < SYSTEM_ACTION_MAX )
    {
        fIsTriggered = gtActionArray[eAction].fIsTriggered;
    }
    
    return fIsTriggered;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SystemActionUpdate( void )
{
    UINT32              dwEpochSeconds = 0;
    UINT16              wCurrentMinuteInTheDay;
    RtcDateTimeStruct   tDateTime;
    
    RtcDateTimeGet( &tDateTime, TRUE );         
    dwEpochSeconds = RtcDateTimeToEpochSeconds( &tDateTime );

    wCurrentMinuteInTheDay = ( tDateTime.bHour * 60 ) + tDateTime.bMinute;        

    /////////////////////////////////////////////////////////////////////////////////
    // update advance scheduled actions
    /////////////////////////////////////////////////////////////////////////////////        
    ControlFileTransferRetryUpdate( dwEpochSeconds );

    for( UINT8 bAction = 0 ; bAction < TRIGGER_ON_EPOCH_REM_ZERO_ACTION_MAX ; bAction++ )
    {
        TriggerOnEpochRemZeroUpdate( bAction, dwEpochSeconds );
    }

    for( UINT8 bAction = 0 ; bAction < TRIGGER_ON_CHANGE_ACTION_MAX ; bAction++ )
    {
        TriggerOnChangeUpdate( bAction, &tDateTime );
    }

    for( UINT8 bAction = 0 ; bAction < TRIGGER_PERIOD_ACTION_MAX ; bAction++ )
    {
        TriggerPeriodUpdate( bAction, dwEpochSeconds );
    }

    for( UINT8 bAction = 0 ; bAction < TRIGGER_MIN_DAY_ACTION_MAX ; bAction++ )
    {
        if
        (
            ( bAction == TRIGGER_MIN_DAY_ACTION_UPLOAD_DATA_TO_SERVER ) ||
            ( bAction == TRIGGER_MIN_DAY_ACTION_DOWNLOAD_SCRIPT_FROM_SERVER ) ||
            ( bAction == TRIGGER_MIN_DAY_ACTION_SYNC_SYS_TIME )
        )
        {
            if( ControlIsTransmissionMinuteOffsetEnabled() )
            {
                // this particular case requires to add a minute offset to triggering of transmissions
                TriggerMinDayUpdate( bAction, (UINT16)TriggerMinDayAddOffsetToMinOfDay( (INT16)wCurrentMinuteInTheDay, (UINT16)ControlGetTransmissionMinuteOffset(), FALSE ) );
            }
            else
            {
                TriggerMinDayUpdate( bAction, wCurrentMinuteInTheDay );
            }
        }
        else
        {
            TriggerMinDayUpdate( bAction, wCurrentMinuteInTheDay );
        }
    }

    
    /////////////////////////////////////////////////////////////////////////////////
    // trigger actions related to advance schedulers
    /////////////////////////////////////////////////////////////////////////////////        
    if( TriggerOnEpochRemZeroIsActionTriggered( TRIGGER_ON_EPOCH_REM_ZERO_ACTION_DOWNLOAD_SCRIPT ) )
    {
        TriggerOnEpochRemZeroClearActionTriggeredFlag( TRIGGER_ON_EPOCH_REM_ZERO_ACTION_DOWNLOAD_SCRIPT );

        SystemActionRequestAction( SYSTEM_ACTION_DOWNLOAD_SCRIPT );
    }

    if( TriggerOnEpochRemZeroIsActionTriggered( TRIGGER_ON_EPOCH_REM_ZERO_ACTION_PCOUNTER_DELTA_COUNT_T1 ) )
    {
        TriggerOnEpochRemZeroClearActionTriggeredFlag( TRIGGER_ON_EPOCH_REM_ZERO_ACTION_PCOUNTER_DELTA_COUNT_T1 );
        
        // start action 
        SystemActionRequestAction( SYSTEM_ACTION_P_COUNTER_GET_DELTA );

        TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0, TRUE );
        // TriggerPeriod when enabled it triggers immediately the first time
        // make sure it doesn't trigger the first time but the second time
        TriggerPeriodUpdate( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0, dwEpochSeconds );
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0 );

        // also trigger transmission of delta count
        SystemActionRequestAction( SYSTEM_ACTION_SEND_PCOUNTER_DELTA_COUNT );
    }

    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_MODBUS_READ_FIRST_TRIGG ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_MODBUS_READ_FIRST_TRIGG );

        // enable to start triggering modbus reads every configured period
        TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_MODBUS_READ, TRUE );
    }

    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_HOURLY_AVERAGE ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_HOURLY_AVERAGE );

        SystemActionRequestAction( SYSTEM_ACTION_CALC_HOURLY_DATA );
    }
    
    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_EVT_FILE_CLOSE_QUEUE ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_EVT_FILE_CLOSE_QUEUE );

        SystemActionRequestAction( SYSTEM_ACTION_EVT_FILE_CLOSE_QUEUE );
    }

    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_MIN_FILE_CLOSE_QUEUE ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_MIN_FILE_CLOSE_QUEUE );

        if( ControlIsTransferMinutelyFilesEnabled() == FALSE )
        {
            SystemActionRequestAction( SYSTEM_ACTION_MIN_FILE_CLOSE_QUEUE );
        }
    }

    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_SIG_FILE_CLOSE_QUEUE ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_SIG_FILE_CLOSE_QUEUE );

        if( ControlIsTransferSignalFilesEnabled() == FALSE )
        {
            SystemActionRequestAction( SYSTEM_ACTION_SIG_FILE_CLOSE_QUEUE );
        }
    }
    
    if( TriggerOnChangeIsActionTriggered( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK ) )
    {
        TriggerOnChangeClearActionTriggeredFlag( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK );

        SystemActionRequestAction( SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_SHORT );
    }
    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_SHORT_SENSOR_SAMPLING_LOW_POWER ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_SHORT_SENSOR_SAMPLING_LOW_POWER );

        SystemActionRequestAction( SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_SHORT );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_LONG_SENSOR_SAMPLING ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_LONG_SENSOR_SAMPLING );

        SystemActionRequestAction( SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_LONG );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0 ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0 );
        // disable triggering
        TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_PCOUNTER_DELTA_COUNT_T0, FALSE );

        // start action 
        SystemActionRequestAction( SYSTEM_ACTION_P_COUNTER_GET_DELTA );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_MODBUS_READ ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_MODBUS_READ );

        SystemActionRequestAction( SYSTEM_ACTION_MODBUS_READ );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_UPDATE_SYS_TIME_INTENSIVE ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_UPDATE_SYS_TIME_INTENSIVE );

        SystemActionRequestAction( SYSTEM_ACTION_SYNC_SYS_TIME );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_ADD_CONFIG_TO_QUEUE ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_ADD_CONFIG_TO_QUEUE );

        // only trigger 1 time
        TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_ADD_CONFIG_TO_QUEUE, FALSE );

        SystemActionRequestAction( SYSTEM_ACTION_BACKUP_FILE_CLOSE_QUEUE );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_SUNRISE_UPDATE ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_SUNRISE_UPDATE );

        SystemActionRequestAction( SYSTEM_ACTION_UPDATE_SUNRISE_TIME );
    }

    if( TriggerPeriodIsActionTriggered( TRIGGER_PERIOD_ACTION_RELAY_RETRY ) )
    {
        TriggerPeriodClearActionTriggeredFlag( TRIGGER_PERIOD_ACTION_RELAY_RETRY );

        SystemActionRequestAction( SYSTEM_ACTION_RUN_RELAY );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_SYNC_SYS_TIME ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_SYNC_SYS_TIME );
        
        SystemActionRequestAction( SYSTEM_ACTION_SYNC_SYS_TIME );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_UPLOAD_DATA_TO_SERVER ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_UPLOAD_DATA_TO_SERVER );
        
        SystemActionRequestAction( SYSTEM_ACTION_DATA_FILE_CLOSE_QUEUE );
        SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_DOWNLOAD_SCRIPT_FROM_SERVER ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_DOWNLOAD_SCRIPT_FROM_SERVER );
        
        SystemActionRequestAction( SYSTEM_ACTION_DOWNLOAD_SCRIPT );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_CAPTURE_IMAGE ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_CAPTURE_IMAGE );
        
        SystemActionRequestAction( SYSTEM_ACTION_CAPTURE_IMAGE );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_P_COUNTER ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_P_COUNTER );
        
        SystemActionRequestAction( SYSTEM_ACTION_P_COUNTER_GET_COUNT );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_FLUID_DEPTH ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_FLUID_DEPTH );
        
        // trigger this action just so that control enters CONTROL_STATE_RUN_MON_APP state
        SystemActionRequestAction( SYSTEM_ACTION_GET_FLUID_DEPTH );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_CAPTURE_IMAGE_SUNRISE ) )
    {
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_CAPTURE_IMAGE_SUNRISE );
        
        SystemActionRequestAction( SYSTEM_ACTION_CAPTURE_IMAGE );
    }

    if( TriggerMinDayIsActionTriggered( TRIGGER_MIN_DAY_ACTION_SENSOR_LONG_SAMPLE_START ) )
    {
        TriggerMinDayEnableActionTrigger( TRIGGER_MIN_DAY_ACTION_SENSOR_LONG_SAMPLE_START, FALSE );
        TriggerMinDayClearActionTriggeredFlag( TRIGGER_MIN_DAY_ACTION_SENSOR_LONG_SAMPLE_START );

        // chained triggering schedulers!!
        TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_LONG_SENSOR_SAMPLING, TRUE );
    }

    if( ControlFileTransferRetryIsTriggered() )
    {
        ControlFileTransferRetryClearTriggeredFlag();

        ConsoleDebugfln( CONSOLE_PORT_USART, "Transfer Retry" );
        SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES );
    }

    /////////////////////////////////////////////////////////////////////////////////
    // trigger actions if requested
    /////////////////////////////////////////////////////////////////////////////////
    // if action requested set triggered flag
    for( SystemActionEnum eAction = 0; eAction < SYSTEM_ACTION_MAX ; eAction++ )
    {
        if( gtActionArray[eAction].fIsTriggered == FALSE )
        {
            if( gtActionArray[eAction].fIsRequested ) 
            {
                // clear requested flag
                gtActionArray[eAction].fIsRequested = FALSE;

                if( gtActionArray[eAction].fIsEnabled )
                {
                    ConsoleDebugfln( CONSOLE_PORT_USART, "Action triggered: %s", SystemActionGetActionName(eAction) );
                    
                    gtActionArray[eAction].fIsTriggered = TRUE;

                    /////////////////////////////////////////////////////////////////////////////////
                    // call functions that start operations (functions that doesn't take long time to execute)
                    /////////////////////////////////////////////////////////////////////////////////        
                    switch( eAction )
                    {
                        case SYSTEM_ACTION_CALC_HOURLY_DATA:
                        {
                            // since operation is performed here and very fast, triggered flag can be cleared
                            ControlAdcReadingCalculateHourly();
                            gtActionArray[eAction].fIsTriggered = FALSE;
                            break;
                        }

                        case SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_SHORT:
                        {
                            ControlAdcReadingSamplingShortRequested();
                            ControlAdcReadingSamplingStart();
                            break;
                        }

                        case SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_LONG:
                        {
                            ControlAdcReadingSamplingLongRequested();
                            ControlAdcReadingSamplingStart();
                            break;
                        }
                        
                        case SYSTEM_ACTION_DATA_FILE_CLOSE_QUEUE:
                        {
                            SystemActionRequestAction( SYSTEM_ACTION_SIG_FILE_CLOSE_QUEUE );
                            break;
                        }

                        case SYSTEM_ACTION_UPDATE_MAIN_BOARD_FW:
                        {
                            // trigger transmissions before running bootloader so the latest data is in the server.
                            SystemActionRequestAction( SYSTEM_ACTION_DATA_FILE_CLOSE_QUEUE );
                            SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES );
                            break;
                        }

                        default:
                        {
                            break;
                        }
                    }
                }
                else
                {
                    ConsoleDebugfln( CONSOLE_PORT_USART, "Action Disabled: %s", SystemActionGetActionName(eAction) );
                }
            }
        }
        /////////////////////////////////////////////////////////////////////////////////
        // clear "requested" actions that are already "triggered"(running)
        /////////////////////////////////////////////////////////////////////////////////        
        else // else if triggered true
        {
            if( gtActionArray[eAction].fIsRequested == TRUE ) 
            {
                gtActionArray[eAction].fIsRequested = FALSE;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * SystemActionGetActionName( SystemActionEnum eAction )
{
    if( eAction < SYSTEM_ACTION_MAX )
    {
        return (CHAR *)&gtActionArray[eAction].pcName[0];
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT16   SystemActionGetActionId         ( SystemActionEnum eAction )
{
    if( eAction < SYSTEM_ACTION_MAX )
    {
        return gtActionArray[eAction].wActionId;
    }
    
    return 255;
}

/* EOF */