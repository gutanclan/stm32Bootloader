/** C Header ******************************************************************

*******************************************************************************/

//#include <stdio.h>
//#include "Target.h"
//#include "Types.h"
//#include "Gpio.h"
//#include "InterfaceBoard.h"
//#include "..\Utils\StateMachine.h"
//#include "Power.h"
//#include "Timer.h"
//
//#include "PowerManager.h"


// C Library includes
#include <stdio.h>
#include <string.h>

// Rowley includes
#include "Assert.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Project include files
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// RTOS Library includes
#include "FreeRTOS.h"
#include "Types.h"              // Timer depends on Types
#include "Timer.h"              // task depends on Timer for vApplicationTickHook
#include "task.h"
#include "queue.h"

// PCOM Project Targets
#include "Target.h"             // Hardware target specifications

// PCOM Library includes
// low level drivers
//#include "System.h"
#include "Adc.h"
#include "Gpio.h"
#include "Watchdog.h"
#include "Dataflash.h"
#include "Rtc.h"
#include "Spi.h"
#include "Usart.h"
#include "Backup.h"
#include "Bluetooth.h"

// Mid level application
#include "TaskDemo.h"
#include "AdcExternal.h"
#include "Data.h"
#include "Main.h"
#include "Console.h"
#include "Command.h"
#include "Config.h"
#include "DFFile.h"
#include "Datalog.h"
//#include "Modem.h"
#include "ModemFtpq.h"
//#include "StateMachine.h"
#include "System.h"

#include "InterfaceBoard.h"
#include "PortEx.h"

#include "Camera.h"
#include "ModemFtpq.h"

// High level application
//#include "Target.h"
#include "TaskCommons.h"
#include "Control.h"

#include "UserIf.h"
//#include "ControlStateDesc.c"   // State description strings tControlStateDescTable[], inline C code
#include "./Utils/StateMachine.h"

#include "../Bootloader/Bootloader.h" // Bootloader interface specification


#include "Modem/ModemData.h"
#include "Modem/ModemCommandResponse/Modem.h"
#include "Modem/ModemCommandResponse/ModemCommand.h"
#include "Modem/ModemCommandResponse/ModemResponse.h"
#include "Modem/ModemOperations/ModemPower.h"
#include "Modem/ModemOperations/ModemInfo.h"
#include "Modem/ModemOperations/ModemSim.h"
#include "Modem/ModemOperations/ModemConfig.h"
#include "Modem/ModemOperations/ModemConnect.h"
#include "Modem/ModemOperations/ModemSetSysClock.h"
#include "Modem/ModemOperations/ModemCellInfo.h"
#include "Modem/ModemOperations/ModemSignal.h"
#include "Modem/ModemOperations/ModemFtp.h"
#include "Modem/ModemOperations/ModemFtpOper.h"

#include "Modem/ModemConnAdv.h"

#include "ControlFileTransfer.h"
#include "ControlScriptRun.h"
#include "ControlAdcReading.h"

//#include "Led.h"
#include "DevInfo.h"

#include "ControlExtDev.h"
#include "Totalizer.h"
#include "Modbus.h"
#include "ConsolePortCheck.h"
#include "LedPattern.h"

#include "LogFormat.h"
#include "Power.h"
#include "PowerManager.h"

#include "SystemInitError.h"

#include "Scheduler/TriggerMinDay.h"
#include "Scheduler/TriggerPeriod.h"
#include "Scheduler/TriggerOnChange.h"
#include "Scheduler/TriggerOnEpochRemZero.h"

#include "SystemAction.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

#define POWER_MANAGER_RESTART_IN_MANUFACTURE_MODE_RTC_BACKUP_REG_INDEX  (1)
#define POWER_MANAGER_DEBUG_ENABLE_RTC_BACKUP_REG_INDEX                 (2)
#define POWER_MANAGER_WATCHDOG_TIMER_TIME_OUT_MS                        (90000)
#define POWER_MANAGER_SUPERVISORY_TASK_UNHEALTHY_TIME_OUT_MS            (1000*60*60*2) // 2 hours

typedef struct
{
    // power reset flags
    PowerManagerPowerResetStruct    tPowerReset;

    // permission to turn on high power usage modules
    BOOL                            fIsModemAllowedToOperate;
    BOOL                            fIsBluetoothAllowedToOperate;
    BOOL                            fIsExternalDeviceAllowedOperate;
    BOOL                            fIsSensorSamplingAllowed;

    // booting up initialization mode
    BOOL                            fIsAnyPowerModeSelected;
    BOOL                            fIsManufacturingModeSelected;
    BOOL                            fIsInitializationDone;
    
    BOOL                            fInterfaceBoardConnected;
    StateMachineType                tState;
    PowerModeEnum                   ePowerModeCurrent;
    PowerModeEnum                   ePowerModeNew;
}PowerManagerStateStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////
// get a copy of all the array actions
// that way depending to voltage this module enables disables some of those actions
// 
typedef enum
{
    POWER_MANAGER_STATE_UNINITIALIZED = 0,
    
    POWER_MANAGER_STATE_BASIC_SYSTEM_INTI,  // init basic peripherals
    
    POWER_MANAGER_STATE_MIN_VOLTAGE_REACHED,// this is protection state for solar pannel first plug. Initialize other modules that require low voltage. make sure all state machines init as well
    
    POWER_MANAGER_STATE_ENABLE_CONSOLE,     // allow continuous monitoring of console
    
    POWER_MANAGER_ARE_STATE_MACHINES_READY,
    
    POWER_MANAGER_STATE_SELECT_MODE, // allow user to select manufacturing or normal mode. 5 seconds window
    
    // at any moment from init starting, user should be able to change to the Normal mode
    // disable all automatic actions, disable watchdog timer, disable supervisory task
    POWER_MANAGER_STATE_INIT_MFG_START,
    POWER_MANAGER_STATE_INIT_MFG_STEP_1,
    POWER_MANAGER_STATE_INIT_MFG_STEP_2,
    POWER_MANAGER_STATE_INIT_MFG_STEP_3,
    POWER_MANAGER_STATE_INIT_MFG_END,
    
    POWER_MANAGER_STATE_MFG_IDLE,
    
    // at any moment from init starting, user should be able to change to the manufacturing mode
    // every minute check voltage, if moved to different level, change action array to disable some stuff.
    POWER_MANAGER_STATE_INIT_NORMAL_START,
    POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_ON,
    POWER_MANAGER_STATE_INIT_NORMAL_MODEM_GET_INFO,
    POWER_MANAGER_STATE_INIT_NORMAL_MODEM_SIM_GET_INFO,
    POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF,
    POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_POWER_TO_DFLASH,
    POWER_MANAGER_STATE_INIT_NORMAL_OPEN_EVENT_LOG_FILE,
    POWER_MANAGER_STATE_INIT_NORMAL_CHECK_CONFIG,
    POWER_MANAGER_STATE_INIT_NORMAL_WAIT_CONFIG_REBUILD,
    POWER_MANAGER_STATE_INIT_NORMAL_CHECK_RTC_SYSTEM_RESET_FLAGS,
    POWER_MANAGER_STATE_INIT_NORMAL_CHECK_PENDING_TRANSMIT_FILE,
    POWER_MANAGER_STATE_INIT_NORMAL_CHECK_VALID_FW_CRC,
    POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_AUTOMATIC_OPERATIONS,
    POWER_MANAGER_STATE_INIT_NORMAL_END,
    
    POWER_MANAGER_STATE_IS_CONNECT_TRANSMIT_REQUIRED,
    POWER_MANAGER_STATE_MONITORING_BATTERY_HEALTHY,
    POWER_MANAGER_STATE_MONITORING_BATTERY_LOW,
    POWER_MANAGER_STATE_MONITORING_BATTERY_DYING,
    POWER_MANAGER_STATE_SEND_LOW_BATTERY_ALERT,
    POWER_MANAGER_STATE_SEND_HEALTHY_BATTERY_ALERT,
    
    POWER_MANAGER_STATE_MAX,
}PowerManagerStateEnum;

static const CHAR * const gtSysModeStringTable[POWER_NAMAGER_SYS_MODE_MAX] =
{
    "UNKNOWN",
    "TEST",
    "NORMAL"
};

static const CHAR * const gtSysExitModeStringTable[POWER_NAMAGER_SYS_EXIT_MODE_MAX] =
{
    "SLEEP",
    "SHUT_DOWN",
    "RESET"
};

static  PowerManagerStateStruct gtPowerManagerState;

static BOOL                     gfIsEnableDebugAtSysInitBitRead                 = FALSE;
static BOOL                     gfIsEnableDebugAtSysInitSet                     = FALSE;
static BOOL                     gfIsEnableDebugAtSysInitConfigurationImplemented= FALSE;

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

static void PowerManagerInitializePeripherals       ( void );
static void PowerManagerInitializeApplicationModules( void );

static BOOL PowerManagerLogEventMessage             ( BOOL fIsError, UINT8 * pbStringBuffer );
static void PowerManagerSetDefaults                 ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PowerManagerInit( void )
{
    PowerManagerInitializePeripherals();
    PowerManagerInitializeApplicationModules();
    
    PowerManagerSetDefaults();

    SystemInitErrorInitialize();
    
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PowerManagerSetDefaults( void )
{
    gtPowerManagerState.fIsModemAllowedToOperate        = FALSE;
    gtPowerManagerState.fIsBluetoothAllowedToOperate    = FALSE;
    gtPowerManagerState.fIsExternalDeviceAllowedOperate = FALSE;
    gtPowerManagerState.fIsSensorSamplingAllowed        = FALSE;
    
    gtPowerManagerState.fIsAnyPowerModeSelected         = FALSE;
    gtPowerManagerState.fIsManufacturingModeSelected    = FALSE;
    gtPowerManagerState.fIsInitializationDone           = FALSE;
    
    gtPowerManagerState.fInterfaceBoardConnected        = FALSE;
    StateMachineInit( &gtPowerManagerState.tState );
    gtPowerManagerState.ePowerModeCurrent               = POWER_MODE_NOT_READY;
    gtPowerManagerState.ePowerModeNew                   = POWER_MODE_NOT_READY;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PowerManagerUpdate( void )
{   
    UINT32 dwValue;


    StateMachineUpdate( &gtPowerManagerState.tState );
    
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // check current power stat
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    PowerUpdate();
    gtPowerManagerState.ePowerModeNew = PowerGetMode();
    
    if( gtPowerManagerState.ePowerModeCurrent != gtPowerManagerState.ePowerModeNew )
    {
        gtPowerManagerState.ePowerModeCurrent = gtPowerManagerState.ePowerModeNew;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    

    // make sure initialization at least console has been enabled
    if( gtPowerManagerState.tState.bStateCurrent > POWER_MANAGER_STATE_ENABLE_CONSOLE )
    {
        if( gtPowerManagerState.fIsManufacturingModeSelected == FALSE ) // normal mode selected
        {
            if( gfIsEnableDebugAtSysInitBitRead )
            {
                if( gfIsEnableDebugAtSysInitConfigurationImplemented == FALSE )
                {
                    gfIsEnableDebugAtSysInitConfigurationImplemented = TRUE;

                    if( gfIsEnableDebugAtSysInitSet )
                    {
                        // this affect system protections
                        TaskDemoSystemCycleSupervisionEnable( FALSE );
                        WatchdogEnable( FALSE, 10000 );

                        // and also system printing information (only to the main console [USB console])
                        ConsolePrintEnable( CONSOLE_PORT_USART, TRUE );
                        ConsolePrintDebugfEnable( CONSOLE_PORT_USART, TRUE );
                    }
                    else
                    {
                        // this affect system protections
                        TaskDemoSystemCycleSupervisionEnable( TRUE );
                        WatchdogEnable( TRUE, 10000 );

                        // and also system printing information (only to the main console [USB console])
                        ConsolePrintEnable( CONSOLE_PORT_USART, FALSE );
                        ConsolePrintDebugfEnable( CONSOLE_PORT_USART, FALSE );
                    }
                }
            }
        }
    }


    
    switch( gtPowerManagerState.tState.bStateCurrent )
    {
        case POWER_MANAGER_STATE_UNINITIALIZED:
        {
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_BASIC_SYSTEM_INTI );
            break;
        }
        
        case POWER_MANAGER_STATE_BASIC_SYSTEM_INTI:
        {   
            gtPowerManagerState.fInterfaceBoardConnected = PowerIsInterfaceBoardConnected();
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MIN_VOLTAGE_REACHED );
            break;
        }
        
        case POWER_MANAGER_STATE_MIN_VOLTAGE_REACHED:
        {
            if( gtPowerManagerState.ePowerModeCurrent >= POWER_MODE_USB_LEVEL )
            {
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_ENABLE_CONSOLE );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_ENABLE_CONSOLE:
        {
            ConsoleInitialize();
            
            // besides several operations for different peripherals, it sets usart to default baud rates
            PowerManagerSysDefaultSet();

            // initialize serial port connected monitoring module
            ConsolePortCheckInit(); 
            ConsolePortCheckEnableCheking( CONSOLE_PORT_CHECK_MAIN, TRUE );
            ConsolePortCheckEnableCheking( CONSOLE_PORT_CHECK_BLUETOOTH, TRUE );
            ConsolePortCheckEnableCheking( CONSOLE_PORT_CHECK_INTERFACE_BOARD, TRUE );
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_ARE_STATE_MACHINES_READY );
            break;
        }
        
        case POWER_MANAGER_ARE_STATE_MACHINES_READY:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                // enable this temporarily until all the other modem state machines are initialized
                // disable it when is done. 
                ModemConnAdvEnable( TRUE );

                // extract the boolean value that indicates if debugging is enabled
                dwValue = 0;
                RtcGetBackupRegister( POWER_MANAGER_DEBUG_ENABLE_RTC_BACKUP_REG_INDEX, &dwValue );
                gfIsEnableDebugAtSysInitBitRead = TRUE;
                if( dwValue == 0 )
                {
                    gfIsEnableDebugAtSysInitSet = FALSE;
                }
                else
                {
                    gfIsEnableDebugAtSysInitSet = TRUE;
                }
            }

            if
            (
                ( ModemConnAdvConnectIsWaitingForCommand()          == TRUE ) &&
                ( ControlAdcReadingSamplingIsWaitingForCommand()    == TRUE ) &&                
                ( ControlScriptRunIsWaitingForNewCommand()          == TRUE ) &&
                ( ControlFileTransfIsTransferDone()                 == TRUE ) &&
                ( CameraIsCommandRunning()                          == FALSE )
            )
            {
                ModemConnAdvEnable( FALSE );
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_SELECT_MODE );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_SELECT_MODE:
        {
            ////////////////////////////////////////////////////////////////////////
            // WARNIGN: be carefull with voltage at this stage
            // only allow to go to normal mode if voltage higher than usb only
            ////////////////////////////////////////////////////////////////////////
            
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                #ifdef  MFG_TEST_FIRMWARE_BUILD
                gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                gtPowerManagerState.fIsManufacturingModeSelected    = TRUE;
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_START );    
                break;
                #endif

                // if no interface board, go to mfg mode automatically
                if( gtPowerManagerState.fInterfaceBoardConnected == FALSE )
                {
                    gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                    gtPowerManagerState.fIsManufacturingModeSelected    = TRUE;
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_START );    
                    break;
                }
                else // int b connected.
                {
                    // check if restart system in manufacturing mode has been requested
                    RtcGetBackupRegister( POWER_MANAGER_RESTART_IN_MANUFACTURE_MODE_RTC_BACKUP_REG_INDEX, &dwValue );
                    if( dwValue == 1 )
                    {
                        // clear register so the system wont start again in manufacturing mode next time.
                        RtcSetBackupRegister( POWER_MANAGER_RESTART_IN_MANUFACTURE_MODE_RTC_BACKUP_REG_INDEX, 0 );
                        
                        gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                        gtPowerManagerState.fIsManufacturingModeSelected    = TRUE;
                        StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_START );        
                        break;
                    }
                    else// else wait 5 seconds to see if user selects a particular mode
                    {
                        // if voltage is enough to run hight power modules then allow for user to select that option
                        // if at least there is usb connected, allow user to select mode. if user select normal is his/her 
                        // responsibility to disable manually big power consumers
                        if( PowerGetMode() > POWER_MODE_LOW_POWER_SOURCE )
                        {
                            // since int b is connected, is possible that power modules is processing voltage level 
                            // and might increase
                            // therefore allow for 5 seconds waiting
                            StateMachineSetTimeOut( &gtPowerManagerState.tState, 5000 );
                        }
                        else
                        {
                            PowerManagerSetDefaults();
                            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_UNINITIALIZED );        
                            break;
                        }
                    }
                }
            }
            
            // if user select a particular mode
            if( gtPowerManagerState.fIsAnyPowerModeSelected == TRUE )
            {
                if( gtPowerManagerState.fIsManufacturingModeSelected )
                {
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_START );
                }
                else
                {
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_START );
                }
                break;
            }
            
            // time out only allowed when interface board is attached
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {
                // only allow to go to normal operation mode if voltage is enough
                if( PowerGetMode() >= POWER_MODE_BATTERY_DYING )
                {
                    // go to normal mode
                    gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                    gtPowerManagerState.fIsManufacturingModeSelected    = FALSE;
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_START );
                }
                else if( PowerGetMode() == POWER_MODE_USB_LEVEL )
                {
                    // go to manufacturing mode
                    gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                    gtPowerManagerState.fIsManufacturingModeSelected    = TRUE;
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_START );
                }
                else
                {
                    PowerManagerSetDefaults();
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_UNINITIALIZED );
                }
                break;
            }
            break;
        }
        
        // ###################################################################################################
        // MANUFACTURING
        // ###################################################################################################
        case POWER_MANAGER_STATE_INIT_MFG_START:
        {
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_STEP_1 );
            break;
        }
        case POWER_MANAGER_STATE_INIT_MFG_STEP_1:
        {
            ///////////////////////////////////////////////////////////
            // assign name to error flag
            // dataflash is located in the support 3v3 power line. Enable it before trying to read it    
            ControlMainBoardSup3v3Enable( TRUE );
            
            SystemInitErrorSetError( SYSTEM_INIT_ERROR_DATAFLASH, (!Dataflash_TestICs( CONSOLE_PORT_USART, FALSE )) );
            ///////////////////////////////////////////////////////////
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_STEP_2 );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_MFG_STEP_2:
        {

            // load config best effort
            SystemInitErrorSetError( SYSTEM_INIT_ERROR_CONFIG, (!ConfigLoad()) );
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_STEP_3 );            
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_MFG_STEP_3:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 500 ); // give 500 msec to INT_B_12V_POwER_ENABLE to be stable

                // interface board enabled and 12vline enabled by default in case adc reading are required
                if( PowerIsInterfaceBoardConnected() == TRUE )
                {
                    TimerTaskDelayMs(50);
                    IntBoardEnable( TRUE ); // protected with delay
                    TimerTaskDelayMs(50);

                    IntBoardGpioOutputWrite( INT_B_12V_POwER_ENABLE, HIGH );
                }

                // disable supervisory task and watch dog timer
                TaskDemoSystemCycleSupervisionEnable( FALSE );
                WatchdogEnable( FALSE, 0 );
            }
             
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {
                // only allow stat machine and sleep operations to run
//                ControlSystemEnable( CONTROL_ACTION_RUN_STATE_MACHINE, TRUE );
//                ControlSystemEnable( CONTROL_ACTION_SYSTEM_SHUT_DOWN, TRUE );
//                ControlSystemEnable( CONTROL_ACTION_SYSTEM_RESTART, TRUE );
                SystemActionEnableAction( SYSTEM_ACTION_STATE_MACHINE, TRUE );
                SystemActionEnableAction( SYSTEM_ACTION_EXIT_SHUT_DOWN, TRUE );
                SystemActionEnableAction( SYSTEM_ACTION_EXIT_RESTART, TRUE );
                
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_MFG_END );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_MFG_END:
        {
            // provide full list of commands to all the consoles
            ConsoleMountNewDictionary( CONSOLE_PORT_USART, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            ConsoleMountNewDictionary( CONSOLE_PORT_SCRIPT, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            ConsoleMountNewDictionary( CONSOLE_PORT_BLUETOOTH, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
            ConsoleMountNewDictionary( CONSOLE_PORT_INTERFACE_BOARD, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            #endif

            // indicate initialization finished
            gtPowerManagerState.fIsInitializationDone = TRUE;
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MFG_IDLE );
            break;
        }

        // ###################################################################################################
        // ###################################################################################################
        case POWER_MANAGER_STATE_MFG_IDLE:
        {
            // end of initialization. loops here until infinite
            break;
        }
        
        // ###################################################################################################
        // NORMAL
        // ###################################################################################################
        case POWER_MANAGER_STATE_INIT_NORMAL_START:
        {
            // configure supervisory task
            //BOOL    fSupervisoryTaskEnabled = TRUE;
            TaskDemoSystemCycleSupervisionSetTimeout( POWER_MANAGER_SUPERVISORY_TASK_UNHEALTHY_TIME_OUT_MS ); 
//            if( gfIsDebugEnabledCurrent )
//            {
//                WatchdogEnable( FALSE, 10000 );
//                TaskDemoSystemCycleSupervisionEnable( FALSE );
//            }
//            else
//            {
//                WatchdogEnable( TRUE, 10000 );
//                TaskDemoSystemCycleSupervisionEnable( TRUE );
//            }
            

            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_ON );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_ON:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                ModemPowerEnablePower( TRUE );
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 40000 );
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_GET_INFO );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_MODEM_GET_INFO:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                ModemInfoRun();
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 5000 );
            }
            
            if( ModemInfoIsWaitingForCommand() == TRUE )
            {
                if( ModemInfoIsExtracted() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_SIM_GET_INFO );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_MODEM_SIM_GET_INFO:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                ModemSimCheckRun();
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 5000 );
                break;
            }
            
            if( ModemSimIsWaitingForCommand() == TRUE )
            {
                if( ModemSimIsReady() )
                {
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_MODEM_PWR_OFF:
        {
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                ModemPowerEnablePower( FALSE );
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 10000 );
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() == FALSE )
                {
                    // modem is power on
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_POWER_TO_DFLASH );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_POWER_TO_DFLASH );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_POWER_TO_DFLASH:
        {
            // assign name to error flag

            // dataflash is located in the support 3v3 power line. Enable it before trying to read it    
            ControlMainBoardSup3v3Enable( TRUE );
            SystemInitErrorSetError( SYSTEM_INIT_ERROR_DATAFLASH, (!Dataflash_TestICs( CONSOLE_PORT_USART, FALSE )) );
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_OPEN_EVENT_LOG_FILE );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_OPEN_EVENT_LOG_FILE:
        {
            DatalogFilesType    eDatalogFileType;
            // BOOL                fIsOpen;

            ///////////////////////////////////////////////////////////////////////
            // log minutely and events from the beginning since these 2 are used for debugging 
            //  we are not worry about them having the correct time stamp in case the system
            //  hasn't update the time from he network
            ///////////////////////////////////////////////////////////////////////
            eDatalogFileType = DATALOG_FILE_EVENT;
            // fIsOpen = 
            DatalogOpenForWrite( eDatalogFileType );
            // log system information
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_SYSTEM_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_SYSTEM_DATA );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_MODEM_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_MODEM_DATA );
            // log closing time headers
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_CLOSING_TIME_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_CLOSING_TIME_FORMAT ); 
            // log event headers
            LogFormatLogEventHeader( eDatalogFileType, LOG_FORMAT_EVENT_HEADER_TYPE_HEADER );
            LogFormatLogEventHeader( eDatalogFileType, LOG_FORMAT_EVENT_HEADER_TYPE_FORMAT ); 


            eDatalogFileType = DATALOG_FILE_ADC_MINUTELY;
            //fIsOpen = 
            DatalogOpenForWrite( eDatalogFileType );
                
            // log system information
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_SYSTEM_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_SYSTEM_DATA );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_MODEM_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_MODEM_DATA );
            // log closing time headers
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_CLOSING_TIME_HEADER );
            LogFormatLogDeviceInfo( eDatalogFileType, LOG_FORMAT_DEVINFO_TYPE_CLOSING_TIME_FORMAT ); 
            // log data headers
            LogFormatLogSampleShortHeader( eDatalogFileType, LOG_FORMAT_SAMPLE_SHORT_HEADER_TYPE_HEADER );
            LogFormatLogSampleShortHeader( eDatalogFileType, LOG_FORMAT_SAMPLE_SHORT_HEADER_TYPE_VAR_ID );
            LogFormatLogSampleShortHeader( eDatalogFileType, LOG_FORMAT_SAMPLE_SHORT_HEADER_TYPE_UNITS );
            // log Modbus headers
            LogFormatLogModbusHeader( eDatalogFileType, LOG_FORMAT_MODBUS_HEADER_TYPE_HEADER );
            LogFormatLogModbusHeader( eDatalogFileType, LOG_FORMAT_MODBUS_HEADER_TYPE_FORMAT );


            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_CHECK_CONFIG );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_CHECK_CONFIG:
        {
#ifdef SUPPORT_OLD_BACKUP_SCRIPT_LOCATION
            // Check if the Backup configuration exists in the New location 
            // .. If not then copy from the old location before it is overwritten by default values
            // Does the old location contain script data?
            UINT8 bPageBuffer[DATAFLASH_PAGE_SIZE_BYTES];
            BOOL fOldConfig = FALSE;
            CHAR            cCompareString[250];
            UINT16          wCompareSize;
            wCompareSize = snprintf(&cCompareString[0], sizeof(cCompareString),"# %s\r\n",gcCONFIG_BackupScriptIndicatorString);

            if (Dataflash_ReadPage( DATAFLASH0_ID, DFF_BACKUP_SCRIPT_SUBSECTOR_START * DATAFLASH_SUBSECTOR_SIZE_PAGES, &bPageBuffer[0], DATAFLASH_PAGE_SIZE_BYTES ) == DATAFLASH_OK)
            {
              if (strncmp(bPageBuffer+DFF_FILENAME_LEN,&cCompareString[0],wCompareSize) == 0)
              {
                fOldConfig = FALSE;
              }
              else
              {
                if (Dataflash_ReadPage( DATAFLASH0_ID, DFF_BACKUP_SCRIPT_OLD_SUBSECTOR_START * DATAFLASH_SUBSECTOR_SIZE_PAGES, &bPageBuffer[0], DATAFLASH_PAGE_SIZE_BYTES ) == DATAFLASH_OK)
                  {
                    if (strncmp(bPageBuffer+DFF_FILENAME_LEN,&cCompareString[0],wCompareSize) == 0)
                    {
                      fOldConfig = TRUE;
                    }
                  }
              }
            }

            // If the backup script is in the old location - Copy it to the new location.
            if (fOldConfig)
            {
              if (Dataflash_EraseSubSectorRange( DATAFLASH0_ID, DFF_BACKUP_SCRIPT_SUBSECTOR_START, DFF_BACKUP_SCRIPT_SUBSECTOR_END ) == DATAFLASH_OK)
              {
                UINT32 wCurrentPageOld = DFF_BACKUP_SCRIPT_OLD_SUBSECTOR_START * DATAFLASH_SUBSECTOR_SIZE_PAGES;
                UINT32 wCurrentPageNew = DFF_BACKUP_SCRIPT_SUBSECTOR_START * DATAFLASH_SUBSECTOR_SIZE_PAGES;
                while ((wCurrentPageNew <= ((DFF_BACKUP_SCRIPT_SUBSECTOR_END + 1) * DATAFLASH_SUBSECTOR_SIZE_PAGES) - 1) &&
                      (wCurrentPageOld <= ((DFF_BACKUP_SCRIPT_OLD_SUBSECTOR_END_R + 1) * DATAFLASH_SUBSECTOR_SIZE_PAGES) - 1))
                {
                  if (Dataflash_ReadPage( DATAFLASH0_ID, wCurrentPageOld, &bPageBuffer[0], DATAFLASH_PAGE_SIZE_BYTES ) == DATAFLASH_OK)
                  {
                    Dataflash_WritePage( DATAFLASH0_ID, wCurrentPageNew, &bPageBuffer[0], DATAFLASH_PAGE_SIZE_BYTES );
                  }
                  wCurrentPageOld++;
                  wCurrentPageNew++;                    
                }
              }
            }
#endif

            // load config 1 by 1 and log the section that is corrupted.
            // if 1 section is corrupted then recreate all the config sections
            BOOL fIsErrorAny = FALSE;
            for( ConfigTypeEnum eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX; eConfigType++ ) 
            {
                if( ConfigParametersLoadByConfigType( eConfigType ) == FALSE )
                {
                    // ConsoleDebugfln( CONSOLE_PORT_USART, "Config corrupted = %s", ConfigGetConfigTypeName(eConfigType) );
                    PowerManagerLogEventMessage( TRUE,"Config Corrupted");
                    PowerManagerLogEventMessage( TRUE,ConfigGetConfigTypeName(eConfigType) );
                    fIsErrorAny = TRUE;
                }
            }

            if( fIsErrorAny )
            {
                ConsoleDebugfln( CONSOLE_PORT_USART, "At least 1 Config corrupted!!" );
                ConsoleDebugfln( CONSOLE_PORT_USART, "Running Config Back Up" );
                // log an event        
                PowerManagerLogEventMessage( TRUE,"All Config Rebuilding...");
                
                //run backup script to reconfigure settings in case it gets reset
                // configure script run
                ControlScriptRunSetFile( DATALOG_FILE_BACKUP_SCRIPT, TRUE );
                ControlScriptRunTransmitResultWhenFinish( TRUE );
                ControlScriptRunCommandResultToFileEnable( TRUE );
                ControlScriptRunStart();
            }

//            BOOL fPass = TRUE;
//            fPass = ConfigLoad();
//            
//            // assign name to error flag
//            // SystemInitErrorSetError( SYSTEM_INIT_ERROR_CONFIG, (!fPass) );
//
//            if( fPass == FALSE )
//            {
//                ConsoleDebugfln( CONSOLE_PORT_USART, "Config corrupted!!" );
//                ConsoleDebugfln( CONSOLE_PORT_USART, "Running Config Back Up" );
//                // log an event        
//                PowerManagerLogEventMessage( TRUE,"Config Corrupted. Rebuilding...");
//                
//                //run backup script to reconfigure settings in case it gets reset
//                // configure script run
//                ControlScriptRunSetFile( DATALOG_FILE_BACKUP_SCRIPT, TRUE );
//                ControlScriptRunTransmitResultWhenFinish( TRUE );
//                ControlScriptRunCommandResultToFileEnable( TRUE );
//                ControlScriptRunStart();
//            }

            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_WAIT_CONFIG_REBUILD );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_WAIT_CONFIG_REBUILD:
        {
            if( ControlScriptRunIsWaitingForNewCommand() )
            {
                SystemInitErrorSetError( SYSTEM_INIT_ERROR_CONFIG, FALSE );
                StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_CHECK_RTC_SYSTEM_RESET_FLAGS );
            }
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_CHECK_RTC_SYSTEM_RESET_FLAGS:
        {
            // LOG ANY RESET
            if( gtPowerManagerState.tPowerReset.dwCsrRegsAtSysInit != 0 )
            {
                if( gtPowerManagerState.tPowerReset.fSoftwareReset )
                {
                    PowerManagerLogEventMessage( TRUE,"Software Reset");
                }
                if( gtPowerManagerState.tPowerReset.fPowerOnReset )
                {
                    PowerManagerLogEventMessage( TRUE,"Power On Reset");
                }
                if( gtPowerManagerState.tPowerReset.fExternalPinReset )
                {
                    PowerManagerLogEventMessage( TRUE,"External Pin Reset");
                }
                if( gtPowerManagerState.tPowerReset.fWatchDogTimerReset )
                {
                    PowerManagerLogEventMessage( TRUE,"Watchdog reset");
                }
                if( gtPowerManagerState.tPowerReset.fWindowWatchdogTimerReset )
                {
                    PowerManagerLogEventMessage( TRUE,"Window Watchdog reset");
                }
                if( gtPowerManagerState.tPowerReset.fLowPowerReset )
                {
                    PowerManagerLogEventMessage( TRUE,"Low Power Reset");
                }
                if( gtPowerManagerState.tPowerReset.fBrownOutResetReset )
                {
                    PowerManagerLogEventMessage( TRUE,"BrownOut Reset");
                }
                
                ConsoleDebugfln( CONSOLE_PORT_USART, "WARNING: System coming back from Reset.." );
                ControlSendCriticalError( LOG_FORMAT_ALERT_SYS_SYSTEM_RESET );
            }
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_CHECK_PENDING_TRANSMIT_FILE );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_CHECK_PENDING_TRANSMIT_FILE:
        {
            BOOL    fIsFilePendingToBeQueued    = FALSE;
            UINT8   bFileType                   = 0;
            UINT32  dwFileSS                    = 0;
            
            ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_SETTING_FILE_PENDING_QUEUE_FLAG, &fIsFilePendingToBeQueued, sizeof(fIsFilePendingToBeQueued) );
            
            if( fIsFilePendingToBeQueued == TRUE )
            {
                // clear flag
                fIsFilePendingToBeQueued = FALSE;
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_SETTING_FILE_PENDING_QUEUE_FLAG, &fIsFilePendingToBeQueued );
                ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );

                // extract:
                // file type 
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_TYPE, &bFileType, sizeof(bFileType) );
                // subsector index
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_SUBSECTOR, &dwFileSS, sizeof(dwFileSS) );
                // and add it to the queue of transmissions    
                ConsoleDebugfln( CONSOLE_PORT_USART, "Queued Pending File %s subsector %d" , DatalogGetFileInternalExtension(bFileType), dwFileSS );
                
                if( bFileType == DATALOG_FILE_ALERT )
                {
                    FTPQ_ftpFileQueueForAlert( bFileType, dwFileSS );
                }
                else
                {
                    FTPQ_ftpFileQueueForPut( bFileType, dwFileSS );
                }
            }
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_CHECK_VALID_FW_CRC );
            break;
        }
        case POWER_MANAGER_STATE_INIT_NORMAL_CHECK_VALID_FW_CRC:
        {
            DevInfoProductInfoType  *ptDevInfo              = NULL;
            UINT32                  dwCrc32StoredInConfig   = 0;
            BOOL                    fIsCrcError             = FALSE;
            
            ptDevInfo = DevInfoGetProductInfo();
            
            // check the crc calc is same as the one embedded. if not, log event
            if( ptDevInfo->tFwInfo.dwCrc32Embed != ptDevInfo->tFwInfo.dwCrc32Calc )
            {
                PowerManagerLogEventMessage(TRUE, "FW CRC32 (calc != embed)!!" );
                ConsoleDebugfln( CONSOLE_PORT_USART, "Fw CRC32 Error!!" );

                fIsCrcError = TRUE;
            }

            // assign name to error flag
            SystemInitErrorSetError( SYSTEM_INIT_ERROR_FW_CRC232, fIsCrcError );

            // get the firmware crc stored in config. we still need to check if it matches with the embedded crc variable
            ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_DI_FIRMWARE_CRC32_UINT32, &dwCrc32StoredInConfig, sizeof(dwCrc32StoredInConfig) );

            if( ptDevInfo->tFwInfo.dwCrc32Embed != dwCrc32StoredInConfig )
            {
                //gfIsNewFirmware = TRUE;
                // update the firmware crc stored in config
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_DI_FIRMWARE_CRC32_UINT32, &ptDevInfo->tFwInfo.dwCrc32Embed );        
                ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );

                CHAR cBuffer[100];
                memset( &cBuffer[0], 0, sizeof(cBuffer) );

                snprintf
                (   
                    &cBuffer[0], (sizeof(cBuffer)-1), "New Firmware!! V=%d.%d.%d.%d rev=%d crc32=0x%08X", 
                    ptDevInfo->tFwInfo.bVersionMajor,
                    ptDevInfo->tFwInfo.bVersionMinor,
                    ptDevInfo->tFwInfo.bVersionPoint,
                    ptDevInfo->tFwInfo.bVersionLast,
                    ptDevInfo->tFwInfo.dwSvnRevision,
                    ptDevInfo->tFwInfo.dwCrc32Embed
                );
                // log the event to file
                PowerManagerLogEventMessage( FALSE, &cBuffer[0] );
                ConsoleDebugfln( CONSOLE_PORT_USART, "%s", &cBuffer[0] );

                // queue msg and request to send it to server
                ControlFileTransfSendMessage( CONTROL_FILE_TRANSFER_MSG_INSTALL_MAIN_B_FW, CONTROL_FILE_TRANSFER_MSG_SUFFIX_PASS, TRUE );
                //ControlSystemRequest( CONTROL_ACTION_FILE_CLOSE_QUEUE_AND_SEND );
            }
            
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_AUTOMATIC_OPERATIONS );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_ENABLE_AUTOMATIC_OPERATIONS:
        {
            // enable triggering of ACTIONS
            TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK, TRUE );
            TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_HOURLY_AVERAGE, TRUE );
            TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_EVT_FILE_CLOSE_QUEUE, TRUE );
            TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_MIN_FILE_CLOSE_QUEUE, FALSE );
            

            for( UINT8 bAction = 0 ; bAction < TRIGGER_MIN_DAY_ACTION_MAX ; bAction++ )
            {
                // enable triggering of ACTIONS
                TriggerMinDayEnableActionTrigger( bAction, TRUE );
                // set schedule to point to the config scheduler by default
                TriggerMinDaySetCurrentSchedule( bAction, TRIGGER_MIN_DAY_SCHEDULE_CONFIGURABLE_1 );
            }

            // enable modem advance connection. (this mode, if modem gets powered on, it will attempt to get connection to network 
            // instead of just being a simple power on)
            ModemConnAdvEnable(TRUE);

            // enable all system automatic operations.
            //ControlAllOperationsEnable( TRUE );
            SystemActionEnableAll( TRUE );
    
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_INIT_NORMAL_END );
            break;
        }
        
        case POWER_MANAGER_STATE_INIT_NORMAL_END:
        {
            // provide full list of commands to all the consoles
            ConsoleMountNewDictionary( CONSOLE_PORT_USART, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            ConsoleMountNewDictionary( CONSOLE_PORT_SCRIPT, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            ConsoleMountNewDictionary( CONSOLE_PORT_BLUETOOTH, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
            ConsoleMountNewDictionary( CONSOLE_PORT_INTERFACE_BOARD, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
            #endif

            // indicate initialization finished
            gtPowerManagerState.fIsInitializationDone = TRUE;

            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_IS_CONNECT_TRANSMIT_REQUIRED );
            break;
        }

        
        // ###################################################################################################
        // ###################################################################################################
        case POWER_MANAGER_STATE_IS_CONNECT_TRANSMIT_REQUIRED:
        {
            // -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
            // 1.- if time not set, attempt to connect
            // 2.- if files in queue(in case an alert got generated) attempt to transmit it
            // -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

            // attempt to connect to set the correct time in the system (only 1 time)
            RtcDateTimeStruct       tDateTime;
            
            RtcDateTimeGet( &tDateTime, FALSE );
    
            // if time has been set already start logging hourly data
//            if( tDateTime.bYear <= 1 )  
//            {
//                SystemActionRequestAction( SYSTEM_ACTION_DATA_FILE_CLOSE_QUEUE );
//            }

            // year 2001 =>  invalid, try to connect to network and extract time
            if( tDateTime.bYear <= 01 )  
            {
                // attempt to connect so that system time can be updated.
                // ControlSystemRequest( CONTROL_ACTION_DOWNLOAD_SCRIPT );
                SystemActionRequestAction( SYSTEM_ACTION_SYNC_SYS_TIME ); 
            }
            
            if
            (
                ( FTPQ_ftpFilePutQueueIsEmpty( NULL ) == FALSE ) ||
                ( FTPQ_ftpSrvrMsgQueueIsEmpty( NULL ) == FALSE ) ||
                ( FTPQ_ftpFileAlertQueueIsEmpty( NULL ) == FALSE )
            )
            {
                // ControlSystemRequest( CONTROL_ACTION_SEND_QUEUED_DATA );
                SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES ); 
            }

            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MONITORING_BATTERY_HEALTHY );
            break;
        }
        
        case POWER_MANAGER_STATE_MONITORING_BATTERY_HEALTHY:
        {
            // end of initialization. loops here until infinite
            // Here there is a continuous monitoring of power.
            // if voltage changes, change system behaviour.
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                // check voltage
                if( PowerGetMode() < POWER_MODE_BATTERY_HEALTHY )
                {
//                    /////////////////////////////////////
//                    // LIST OF BEHAVIOUR CHANGES
//                    /////////////////////////////////////
//                    // * Reduce transmissions?
//                    // * Reduce cam captures 
//                    // * Reduce pulse count get count
//                    // * Reduce sensor sampling?
//                    // * Send Critical message
//                    /////////////////////////////////////
//                    for( UINT8 bAction = 0 ; bAction < TRIGGER_MIN_DAY_ACTION_MAX ; bAction++ )
//                    {
//                        TriggerMinDaySetCurrentSchedule( bAction, TRIGGER_MIN_DAY_SCHEDULE_LOW_POWER );
//                    }
//                    
//                    // * Reduce sensor sampling?
//                    // sensor sampling changes scheduler type (from "OnChange" to "Period" low power profile)
//                    TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK, FALSE );
//                    
//                    TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_SHORT_SENSOR_SAMPLING_LOW_POWER, TRUE );
//                    TriggerPeriodSetPeriodMode( TRIGGER_PERIOD_ACTION_SHORT_SENSOR_SAMPLING_LOW_POWER, TRIGGER_PERIOD_MODE_LOW_POWER );
                    
                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    // log critical error and trigger transmission
                    ControlSendCriticalError( LOG_FORMAT_ALERT_POWER_SYS_VOLTAGE_BATTERY_LOW );
                    SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES );
                    LogFormatLogEventData(DATALOG_FILE_EVENT,LOG_FORMAT_EVENT_DATA_TYPE_EVENT,"PowerManager","SysV Battery Low");
                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MONITORING_BATTERY_LOW );
                    break;
                }
            
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            break;
        }
        
        case POWER_MANAGER_STATE_MONITORING_BATTERY_LOW:
        {
            // end of initialization. loops here until infinite
            // here there is a continuous monitoring of power.
            // if voltage changes, change system behaviour.
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                // check voltage
                if( PowerGetMode() > POWER_MODE_BATTERY_LOW )
                {
//                    /////////////////////////////////////
//                    // LIST OF BEHAVIOUR CHANGES
//                    /////////////////////////////////////
//                    // * Original transmissions?
//                    // * Original cam captures 
//                    // * Original pulse count get count
//                    // * Original sensor sampling?
//                    // * Send Critical message
//                    /////////////////////////////////////
//                    for( UINT8 bAction = 0 ; bAction < TRIGGER_MIN_DAY_ACTION_MAX ; bAction++ )
//                    {
//                        TriggerMinDaySetCurrentSchedule( bAction, TRIGGER_MIN_DAY_SCHEDULE_CONFIGURABLE_1 );
//                    }
//                    // * Original sensor sampling?
//                    TriggerPeriodEnableActionTrigger( TRIGGER_PERIOD_ACTION_SHORT_SENSOR_SAMPLING_LOW_POWER, FALSE );
//                    TriggerOnChangeEnableActionTrigger( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK, TRUE );
//                    TriggerOnChangeSetChangeType( TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK, TRIGGER_ON_CHANGE_TYPE_MINUTE );
//
//                    SystemActionEnableAll( TRUE );

                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    // log critical error and trigger transmission
                    ControlSendCriticalError( LOG_FORMAT_ALERT_POWER_SYS_VOLTAGE_BATTERY_HEALTHY );
                    SystemActionRequestAction( SYSTEM_ACTION_SEND_QUEUED_FILES );
                    LogFormatLogEventData(DATALOG_FILE_EVENT,LOG_FORMAT_EVENT_DATA_TYPE_EVENT,"PowerManager","SysV Battery Healthy");
                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MONITORING_BATTERY_HEALTHY );
                    break;
                }
                else if( PowerGetMode() == POWER_MODE_BATTERY_LOW )
                {
                    // do nothing
                }
                else if( PowerGetMode() < POWER_MODE_BATTERY_LOW )
                {
//                    /////////////////////////////////////
//                    // LIST OF BEHAVIOUR CHANGES
//                    /////////////////////////////////////
//                    // * Send Critical message
//                    // * only allow state machine to download scripts at noon (half implemented. schedule change is the missing part.)
//                    /////////////////////////////////////
//
//                    for( UINT8 bAction = 0 ; bAction < TRIGGER_MIN_DAY_ACTION_MAX ; bAction++ )
//                    {
//                        TriggerMinDaySetCurrentSchedule( bAction, TRIGGER_MIN_DAY_SCHEDULE_DEFAULT );
//                    }
//                    
//                    SystemActionEnableAll( TRUE );
//                    SystemActionEnableAction( SYSTEM_ACTION_CALC_HOURLY_DATA, TRUE );
//                    SystemActionEnableAction( SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_SHORT, TRUE );
//                    SystemActionEnableAction( SYSTEM_ACTION_TAKE_SENSOR_SAMPLE_LONG, FALSE );
//                    SystemActionEnableAction( SYSTEM_ACTION_CAPTURE_IMAGE, FALSE );
//                    SystemActionEnableAction( SYSTEM_ACTION_P_COUNTER_GET_COUNT, FALSE );

                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    // log critical error
                    ControlSendCriticalError( LOG_FORMAT_ALERT_POWER_SYS_VOLTAGE_HEALTHY_DYING );
                    LogFormatLogEventData( DATALOG_FILE_EVENT,LOG_FORMAT_EVENT_DATA_TYPE_EVENT,"PowerManager","SysV Battery Dying");
                    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    
                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MONITORING_BATTERY_DYING );
                    break;
                }
            
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            break;
        }
        
        case POWER_MANAGER_STATE_MONITORING_BATTERY_DYING:
        {
            // there is no way back from here.
            // this mode only logs events.
            // at this stage, unit is waiting for someone to rescue it and change battery.
            if( StateMachineIsFirtEntry( &gtPowerManagerState.tState ) )
            {
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            
            if( StateMachineIsTimeOut( &gtPowerManagerState.tState ) ) 
            {   
                // check voltage
//                if( PowerGetMode() > POWER_MODE_BATTERY_DYING )
//                {
//                    ControlAllOperationsEnable( TRUE );
//                    
//                    StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_MONITORING_BATTERY_LOW );
//                    break;
//                }
//                else 
                if( PowerGetMode() > POWER_MODE_BATTERY_DYING )
                {
                    // do nothing
                }
                else if( PowerGetMode() == POWER_MODE_BATTERY_DYING )
                {
                    // do nothing
                }
                else if( PowerGetMode() < POWER_MODE_BATTERY_DYING )
                {
                    // go to lowest power mode possible. 
                    // (system is continuously sleeping.)
//                    PowerManagerSysExitForceMode( POWER_NAMAGER_SYS_EXIT_MODE_SHUT_DOWN );
//                    break;
                }
            
                StateMachineSetTimeOut( &gtPowerManagerState.tState, 60000 ); // check every minute
            }
            break;
        }
        
        case POWER_MANAGER_STATE_SEND_LOW_BATTERY_ALERT:
        {
        
        }
        
        case POWER_MANAGER_STATE_SEND_HEALTHY_BATTERY_ALERT:
        {
        
        }
        
        default:
        {
            PowerManagerSetDefaults();
            StateMachineChangeState( &gtPowerManagerState.tState, POWER_MANAGER_STATE_UNINITIALIZED );        
            break;
        }
        
        // TODO: need to add the other cases to init mfg and normal mode
        // * double check console port check is still working since it was modified
        // * pass the array of aumatic actions and disable/enable them depending on the system mode or voltage level
        // * make sure system behavies the same as before making all this modifications to not affect warrent or yousra software
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PowerManagerSysModeIsInitDone( void )
{
    return gtPowerManagerState.fIsInitializationDone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PowerManagerSysModeSetMode( PowerManagerSystemModeEnum eSysMode )
{
    BOOL fSuccess = FALSE;
    
    if( eSysMode < POWER_NAMAGER_SYS_MODE_MAX )
    {
        // if already a power mode selected, then everything has 
        // to be interrupted and after than change power mode
        if( gtPowerManagerState.fIsAnyPowerModeSelected )   
        {    
            if( eSysMode == POWER_NAMAGER_SYS_MODE_MANUFACTURING )
            {
                if( gtPowerManagerState.fIsManufacturingModeSelected )
                {
                    // if same mode already selected is requested don't change anything   
                    fSuccess = TRUE;
                }
                else
                {
                    // set register byte to indicate on next restart go immediately to manufacturing mode
                    RtcSetBackupRegister( POWER_MANAGER_RESTART_IN_MANUFACTURE_MODE_RTC_BACKUP_REG_INDEX, 1 );
                    // if different mode selected, then force a change of mode
                    PowerManagerSysExitForceMode( POWER_NAMAGER_SYS_EXIT_MODE_RESET );
                }
            }
            else if( eSysMode == POWER_NAMAGER_SYS_MODE_NORMAL )
            {
                if( gtPowerManagerState.fIsManufacturingModeSelected == FALSE )
                {
                    // if same mode already selected is requested don't change anything   
                    fSuccess = TRUE;
                }
                else
                {
                    // clear register byte to not force manufacturing mode on next restart
                    RtcSetBackupRegister( POWER_MANAGER_RESTART_IN_MANUFACTURE_MODE_RTC_BACKUP_REG_INDEX, 0 );
                    // if different mode selected, then force a change of mode
                    PowerManagerSysExitForceMode( POWER_NAMAGER_SYS_EXIT_MODE_RESET );
                }
            }
        }
        else
        {
            // if no power mode selected yet, then is straight forward, just set the mode
            if( eSysMode == POWER_NAMAGER_SYS_MODE_MANUFACTURING )
            {
                gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                gtPowerManagerState.fIsManufacturingModeSelected    = TRUE;
                fSuccess = TRUE;    
            }
            else if( eSysMode == POWER_NAMAGER_SYS_MODE_NORMAL )
            {
                gtPowerManagerState.fIsAnyPowerModeSelected         = TRUE;
                gtPowerManagerState.fIsManufacturingModeSelected    = FALSE;
                fSuccess = TRUE;    
            }
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR *  PowerManagerSysModeGetModeName( PowerManagerSystemModeEnum eSysMode )
{
    if( eSysMode < POWER_NAMAGER_SYS_MODE_MAX )
    {
        return (CHAR *)&gtSysModeStringTable[eSysMode][0];
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
PowerManagerSystemModeEnum PowerManagerSysModeGetMode( void )
{
    if( gtPowerManagerState.fIsAnyPowerModeSelected )
    {
        if( gtPowerManagerState.fIsManufacturingModeSelected )
        {
            return POWER_NAMAGER_SYS_MODE_MANUFACTURING;
        }
        else
        {
            return POWER_NAMAGER_SYS_MODE_NORMAL;
        }
    }
    else
    {
        return POWER_NAMAGER_SYS_MODE_UNKNOWN;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PowerManagerSysDefaultSet( void )
{
    // used: 
    // * before and after going to sleep
    // * before running boot loader
    // * when booting up
    
    // ------------------------------
    // operations performed here:
    // ------------------------------
    // * turn off internal adc.
    
    // * sensor sampling: stop sampling and disable voltage lines.
    
    // * stop external device operations
    // * turn off interface board
    // * turn off modem
    // * turn off BT
    // * usarts to default baud rate
    
    
    
    
    // * turn off internal adc.
    // not implemented
    
    // * sensor sampling: stop sampling and disable voltage lines.
    TIMER tTimerModemOff;
    tTimerModemOff = TimerDownTimerStartMs( 10000 );    

    ControlAdcReadingSamplingLongInterrupt();
    // make sure is not running
    while( TRUE )
    {
        TimerTaskDelayMs(5);

        ControlAdcReadingStateMachine();
        
        if( TimerDownTimerIsExpired( tTimerModemOff ) )
        {
            break;
        }

        if( ControlAdcReadingSamplingIsWaitingForCommand() )
        {
            break;
        }
    }
    
    // * stop external device operations
    // not implemented, external device module doesn't have a FORCE STOP. (disabling interface board could help)
    
    // * turn off interface board
    if( IntBoardIsEnabled() )
    {
        IntBoardEnable( FALSE );
    }
    
    // * turn off modem
    if( ModemPowerIsPowerEnabled() )
    {
        ModemForcePowerOff();
    }
    
    // * turn off BT
    if( BluetoothIsPowerEnabled() == TRUE )
    {
        BluetoothPowerEnable( FALSE );
    }
    
    // * usarts to default baud rate
    UsartPortChangeBaud( TARGET_USART_PORT_TO_CONSOLE,      TARGET_USART_BAUD_RATE_CONSOLE );
    UsartPortChangeBaud( TARGET_USART_PORT_TO_MODEM,        TARGET_USART_BAUD_RATE_MODEM );
    UsartPortChangeBaud( TARGET_USART_PORT_TO_BLUETOOTH,    TARGET_USART_BAUD_RATE_BLUETOOTH );
    UsartPortChangeBaud( TARGET_USART_PORT_CAMERA,          TARGET_USART_BAUD_RATE_CAMERA );
    #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)    
    UsartPortChangeBaud( TARGET_USART_PORT_TO_INT_B_CONSOLE,TARGET_USART_BAUD_RATE_INT_B_CONSOLE );
    #else
    UsartPortChangeBaud( TARGET_USART_PORT_TO_GPS,          TARGET_USART_BAUD_RATE_GPS );
    #endif
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PowerManagerSysExitForceMode( PowerManagerSysExitModeEnum eExitMode )
{
    BOOL fSuccess = FALSE;
    
    if( eExitMode < POWER_NAMAGER_SYS_EXIT_MODE_MAX )
    {
        ConsoleDebugfln( CONSOLE_PORT_USART, "Exit System" );
        PowerManagerLogEventMessage( FALSE, "Exit System" );
        
        
        // turn off everything.(peripheral wise)
        PowerManagerSysDefaultSet();

        // if in normal mode and 
        if( PowerManagerSysModeGetMode() == POWER_NAMAGER_SYS_MODE_NORMAL )
        {            
            // if console ports are disconnected,
            if( ConsolePortCheckIsAnyPortConnected() == FALSE )
            {
                // if debugging is disabled
                if( PowerManagerIsDebugEnabled() == FALSE )
                {
                    // then disable printing information from all ports
                    for( ConsolePortCheckEnum ePort = 0; ePort < CONSOLE_PORT_CHECK_MAX ; ePort++ )
                    {
                        ConsolePortEnum eConsolePort;
                    
                        eConsolePort = ConsolePortCheckGetPrintingConsoleEnum( ePort );

                        if( ConsolePrintIsEnabled( eConsolePort ) )
                        {
                            ConsolePrintEnable( eConsolePort, FALSE );
                        }

                        if( ConsolePrintDebugfIsEnabled( eConsolePort ) )
                        {
                            ConsolePrintDebugfEnable( eConsolePort, FALSE );
                        }
                    }
                }
            }
        }
        
        // turn off "console port checking" since it enables usarts if cable is connected
        for( ConsolePortCheckEnum ePort = 0; ePort < CONSOLE_PORT_CHECK_MAX ; ePort++ )
        {
            ConsolePortCheckEnableCheking( ePort, FALSE );
        }
        // update status. Run state machine 5 times
        for( UINT8 c = 0; c < 5 ; c++ )
        {
            ConsolePortCheckMonitoring();
        }
        
        // disable all usart ports.
        // if debugging is disabled
        if( PowerManagerIsDebugEnabled() == FALSE )
        {
            for( UsartPortsEnum ePort = 0; ePort < USART_PORT_TOTAL ; ePort++ )
            {
                UsartPortClose( ePort );
            }
        }
        
        LedPatternSetBlinkPattern( LED1_SYSTEM_OPERATIONS, LED_PATTERN_SOLID_OFF );

        GpioWrite( GPIO_LED1_OUT, LOW );
        
        ControlMainBoardSup3v3Enable( FALSE );
        
        // wait 1 seconds to make sure power lines and pins are stable low
        TimerTaskDelayMs( 1000 );
        

        BOOL fDebugFlagEnabledIsWatchDogEnabled = WatchdogIsEnabled();

        if( PowerManagerSysModeGetMode() == POWER_NAMAGER_SYS_MODE_NORMAL )
        {
            // always turn off watchdog timer when going to sleep
            // but when wake up, only enable it if debug mode is disabled
            WatchdogEnable( FALSE, 0 );
        }
        
        if( eExitMode == POWER_NAMAGER_SYS_EXIT_MODE_SLEEP )
        {
            SystemSleep( SYSTEM_SLEEP_MODE_STOP );
        }
        else if( eExitMode == POWER_NAMAGER_SYS_EXIT_MODE_SHUT_DOWN )
        {
            SystemSleep( SYSTEM_SLEEP_MODE_STAND_BY );
        }
        else
        {
            // this function never goes back
            SystemSoftwareReset();
        }
        
        if( PowerManagerSysModeGetMode() == POWER_NAMAGER_SYS_MODE_NORMAL )
        {
            if( PowerManagerIsDebugEnabled() )
            {
                WatchdogEnable( fDebugFlagEnabledIsWatchDogEnabled, 10000 );
            }
            else
            {
                // enable watchdog timer if in normal mode
                WatchdogEnable( TRUE, 10000 );
            }
        }
        
        ControlMainBoardSup3v3Enable( TRUE );
        
        LedPatternSetBlinkPattern( LED1_SYSTEM_OPERATIONS, LED_PATTERN_SOLID_ON );
        
        // turn on "console port checking"
        for( ConsolePortCheckEnum ePort = 0; ePort < CONSOLE_PORT_CHECK_MAX ; ePort++ )
        {
            ConsolePortCheckEnableCheking( ePort, TRUE );
        }

        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * PowerManagerSysExitGetModeName( PowerManagerSysExitModeEnum eExitMode )
{
    if( eExitMode < POWER_NAMAGER_SYS_EXIT_MODE_MAX )
    {
        return (CHAR *)&gtSysExitModeStringTable[eExitMode][0];
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void PowerManagerInitializePeripherals( void )
{
    BOOL fComponentReady;

    // kick watch dog timer before starting to make 
    // sure nothing gets stuck during initialization of peripherals.
    WatchdogKickSoftwareWatchdog( POWER_MANAGER_WATCHDOG_TIMER_TIME_OUT_MS );

    
    
    ///////////////////////////////////////////////////////////
    // copy register that keeps track of the reason for system reset
    ///////////////////////////////////////////////////////////
    memset( &gtPowerManagerState.tPowerReset, 0, sizeof(gtPowerManagerState.tPowerReset) );
    
    gtPowerManagerState.tPowerReset.dwCsrRegsAtSysInit = RCC->CSR;
    
    // if (PWR_GetFlagStatus(PWR_FLAG_SB))
    // {
        // puts("System resumed from STANDBY mode");
    // }
 
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST))
    {
        // puts("Software Reset");
        gtPowerManagerState.tPowerReset.fSoftwareReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_PORRST))
    {
        // puts("Power-On-Reset");
        gtPowerManagerState.tPowerReset.fPowerOnReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_PINRST)) // Always set, test other cases first
    {
        // puts("External Pin Reset");
        gtPowerManagerState.tPowerReset.fExternalPinReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
    {    
        // puts("Watchdog Reset");
        gtPowerManagerState.tPowerReset.fWatchDogTimerReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
    {
        // puts("Window Watchdog Reset");
        gtPowerManagerState.tPowerReset.fWindowWatchdogTimerReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET)
    {
        // puts("Low Power Reset");
        gtPowerManagerState.tPowerReset.fLowPowerReset = TRUE;
    }

    if (RCC_GetFlagStatus(RCC_FLAG_BORRST) != RESET) // F4 Usually set with POR
    {
        // puts("Brown-Out Reset");
        gtPowerManagerState.tPowerReset.fBrownOutResetReset = TRUE;
    }

    // don't forget to clear the flags once they have been used
    // WARNING!!: further in the code this is being used so don't clear it yet.
    // NOTE: The farthest in the code where this is being called is inside RtcInitialize()
    // RCC_ClearFlag();
    ///////////////////////////////////////////////////////////
    
    
    ///////////////////////////////////////////////////////////
    // Check if HSE oscillator is stable and ready to use
    ///////////////////////////////////////////////////////////
    fComponentReady = FALSE;
    // TODO:  The RCC_WaitForHSEStartUp() routine has an internal delay, it should not be used to just check if the HSE is stable
    if( RCC_WaitForHSEStartUp() == SUCCESS )
    {
        fComponentReady = TRUE;
    }
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_HSE, (!fComponentReady) );
    ///////////////////////////////////////////////////////////

    
    
    ///////////////////////////////////////////////////////////
    // check if PLL is ready
    ///////////////////////////////////////////////////////////
    fComponentReady = FALSE;
    if((RCC->CR & RCC_CR_PLLRDY) != 0)
    {
        fComponentReady = TRUE;
    }
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_PLL, (!fComponentReady) );
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    // check RTC 
    ///////////////////////////////////////////////////////////
    fComponentReady = RtcInitialize( RTC_SOURCE_CLOCK_LSE );
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_RTC, (!fComponentReady) );
    ///////////////////////////////////////////////////////////

    
    
    ///////////////////////////////////////////////////////////
    // GPIOs
    ///////////////////////////////////////////////////////////
    // initialize unused pins set them to input pull down.
    fComponentReady = TRUE;
    fComponentReady&= GpioInitUnUsedPin( GPIO_BLUETOOTH_RTS );
    fComponentReady&= GpioInitUnUsedPin( GPIO_BLUETOOTH_CTS );

    fComponentReady&= GpioInitUnUsedPin( GPIO_MODEM_DSR_IN );
    fComponentReady&= GpioInitUnUsedPin( GPIO_MODEM_DCD_IN );
    fComponentReady&= GpioInitUnUsedPin( GPIO_MODEM_RING_IN );

    fComponentReady&= GpioInitUnUsedPin( GPIO_PWR_CTRL_ON_OFF );
    fComponentReady&= GpioInitUnUsedPin( GPIO_SAT_PWR_EN );
    fComponentReady&= GpioInitUnUsedPin( GPIO_SAT_GPIO_1 );
//    fComponentReady&= GpioInitUnUsedPin( GPIO_EXT_GPIO_PD14 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_EXT_GPIO_PE9 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_EXT_GPIO_PE12 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_EXT_GPIO_PE15 );

    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PB13 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PB14 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PB15 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PC8 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PC9 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PC13 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PD13 );
    #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)    
    #else
    fComponentReady&= GpioInitUnUsedPin( GPIO_UNUSED_PB12 );
    fComponentReady&= GpioInitUnUsedPin( GPIO_GPS_PWR_EN );
    #endif

    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_GPIO, (!fComponentReady) );
    /////////////////////////////////////////////////////////// 


    
    ///////////////////////////////////////////////////////////
    // USART
    ///////////////////////////////////////////////////////////
    fComponentReady = TRUE;
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_TO_CONSOLE,      USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_CONSOLE, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_TO_CONSOLE );
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_TO_MODEM,        USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_MODEM, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_TO_MODEM );
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_TO_BLUETOOTH,    USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_BLUETOOTH, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_TO_BLUETOOTH );
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_CAMERA,          USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_CAMERA, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_CAMERA );
    #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_TO_INT_B_CONSOLE,USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_INT_B_CONSOLE, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_TO_INT_B_CONSOLE );
    #else
    fComponentReady&= UsartPortInit( TARGET_USART_PORT_TO_GPS,          USART_MODE_RXTX_ON, TARGET_USART_BAUD_RATE_GPS, USART_StopBits_1, USART_Parity_No, USART_PROTOCOL_NORMAL_FULL );
    UsartPortClose( TARGET_USART_PORT_TO_GPS );
    #endif
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_USART, (!fComponentReady) );
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    // backup register reader/writer
    ///////////////////////////////////////////////////////////
    fComponentReady = BackupInitialize();
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_BACKUP, (!fComponentReady) );
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    // SPI
    ///////////////////////////////////////////////////////////
    fComponentReady = SpiInitialize();
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_SPI, (!fComponentReady) );
    ///////////////////////////////////////////////////////////

    
    
    ///////////////////////////////////////////////////////////
    // ADC internal
    ///////////////////////////////////////////////////////////
    fComponentReady = TRUE;
    fComponentReady&= AdcInternalInitialize();
    #if defined(BUILD_TARGET_STM32F207_EVAL)
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_POT);
    #elif defined(BUILD_TARGET_AFTI_MAINBOARD)
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_EXT_PRIMARY_BATTERY_VOLTAGE);
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_EXT_SECONDARY);
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_EXT_TERTIARY);
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_THERM_SENSOR);
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_INT_THERM_SENSOR);
    fComponentReady&= AdcInternalInitSinglePin(ADC_SENSOR_VREFINT);
    #endif 
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_ADC_INTERNAL, (!fComponentReady) );
    ///////////////////////////////////////////////////////////
}

// initialize modules without touching any voltage levels since at this point device is not guaranteed to have enough voltage.
void PowerManagerInitializeApplicationModules( void )
{
    BOOL    fComponentReady;
    UINT32  dwDefaultTimeoutConnection;
    UINT16  wSubsectorStart;
    UINT16  wSubsectorEnd;
        
    // kick watch dog timer before starting to make 
    // sure nothing gets stuck during initialization of peripherals.
    WatchdogKickSoftwareWatchdog( POWER_MANAGER_WATCHDOG_TIMER_TIME_OUT_MS );

    ///////////////////////////////////////////////////////////
    // FILE system
    ///////////////////////////////////////////////////////////
    fComponentReady = DFF_init();

    // Warning!!:DFFile module should be initialized by its own functions however it doesn't have functions to return subsector.
    // therefore Datalog module is being used instead. 
    // Both modules should have the same amount of enums for file types
    for( DatalogFilesType eDatalogFileType = 0; eDatalogFileType < DATALOG_FILE_MAX; eDatalogFileType++ )
    {
        DatalogGetFileStartSubsector( eDatalogFileType, (UINT16 *)&wSubsectorStart );
        DatalogGetFileEndSubsector( eDatalogFileType, (UINT16 *)&wSubsectorEnd );
        fComponentReady   &= DFF_fileTypeInit( eDatalogFileType, wSubsectorStart, wSubsectorEnd );    
    }
    
    if( (UINT8)DATALOG_FILE_MAX != (UINT8)DFF_NUM_FILE_TYPES )
    {
        while(1)
        {
            // these enums have to match
        }
    }

    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_FILES, (!fComponentReady) );
    
   
    ///////////////////////////////////////////////////////////
    // file logging and formatting
    LogFormatInit();
    ///////////////////////////////////////////////////////////
    
    
    ///////////////////////////////////////////////////////////
    // MODEM 
    ///////////////////////////////////////////////////////////    
    dwDefaultTimeoutConnection = (1000*60*5);// 5 minutes to time out in case it doesn't achieve connection
    ModemConnAdvInit();
    ModemConnAdvEnable( FALSE );
    ModemConnAdvConnectSetTimeOut( dwDefaultTimeoutConnection );
    ModemConsolePrintDbgEnable( FALSE );
    ModemConsolePrintfEnable( FALSE );

    
    ///////////////////////////////////////////////////////////
    // BLUETOOTH
    ///////////////////////////////////////////////////////////    
    fComponentReady = BluetoothInitialize();
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_BLUETOOTH, (!fComponentReady) );
    ///////////////////////////////////////////////////////////
    
    
    ///////////////////////////////////////////////////////////
    // default state
    IntBoardEnable( FALSE );
    ///////////////////////////////////////////////////////////
    
    
    ///////////////////////////////////////////////////////////
    // REED SWITCH
    ///////////////////////////////////////////////////////////    
    fComponentReady = TRUE;
    fComponentReady &= UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_1, TRUE );
    fComponentReady &= UsertIfInterruptPinReedSwtchEnable( USER_IF_REED_SWITCH_2, TRUE );

    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_REEDSWITCH, (!fComponentReady) );
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    // adc sampling module
    ///////////////////////////////////////////////////////////    
    ControlAdcReadingInit();
    ControlAdcReadingShortSamplingConfig( ADC_NUM_AVERAGES_ANALOG_INPUTS, ADC_VIBRATION_NUM_SAMPLES_LONG );
    ControlAdcReadingLongSamplingConfig( 5, 3, 30 );
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    // CAM
    ///////////////////////////////////////////////////////////    
//    fComponentReady = CAMERA_init();
//    
//    // assign name to error flag
//    SystemInitErrorSetError( SYSTEM_INIT_ERROR_CAM, (!fComponentReady) );
    ///////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////////
    // TOTALIZER
    ///////////////////////////////////////////////////////////    
    // fComponentReady = TOTALIZER_init();
    
    // assign name to error flag
//    SystemInitErrorSetError( SYSTEM_INIT_ERROR_PCOUNTER, (!fComponentReady) );
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    // MODBUS
    ///////////////////////////////////////////////////////////    
    fComponentReady = MODBUS_init();
    
    // assign name to error flag
    SystemInitErrorSetError( SYSTEM_INIT_ERROR_MODBUS, (!fComponentReady) );
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    ControlExtDeviceInit();
    ///////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////////
    ControlScriptRunInit();
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    //ControlFileTransfInit( ControlActionTriggeredArrayGetPointer(), ControlActionEnabledArrayGetPointer() );
    ControlFileTransfInit();
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    TriggerMinDayInit();
    TriggerPeriodInit();
    TriggerOnChangeInit();
    TriggerOnEpochRemZeroInit();
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    SystemActionInit();
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    // mounting dictionaries into consoles
    ///////////////////////////////////////////////////////////    
    fComponentReady = TRUE;
    fComponentReady &= ConsoleMountNewDictionary( CONSOLE_PORT_USART, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BOOTING_UP ) );
    fComponentReady &= ConsoleMountNewDictionary( CONSOLE_PORT_SCRIPT, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );
    fComponentReady &= ConsoleMountNewDictionary( CONSOLE_PORT_BLUETOOTH, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BOOTING_UP ) );
    #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
    fComponentReady &= ConsoleMountNewDictionary( CONSOLE_PORT_INTERFACE_BOARD, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BOOTING_UP ) );
    #endif

    SystemInitErrorSetError( SYSTEM_INIT_ERROR_CONSOLE_DICTIONARY, (!fComponentReady) );
}

BOOL PowerManagerLogEventMessage( BOOL fIsError, UINT8 * pbStringBuffer )
{
    BOOL fSuccess = FALSE;

    if( NULL != pbStringBuffer )
    {        
        LogFormatEventDataType eMsgType;

        if( fIsError )
        {            
            eMsgType = LOG_FORMAT_EVENT_DATA_TYPE_ERROR;
        }
        else
        {            
            eMsgType = LOG_FORMAT_EVENT_DATA_TYPE_EVENT;
        }
        
        fSuccess = LogFormatLogEventData
        ( 
            DATALOG_FILE_EVENT, 
            eMsgType, 
            "PowerManager.c", 
            pbStringBuffer
        );
    }
    return fSuccess;
}

PowerManagerPowerResetStruct * PowerManagerGetResetFlags( void )
{
    return &gtPowerManagerState.tPowerReset;
}

void PowerManagerDebugEnable( BOOL fEnable )
{
    gfIsEnableDebugAtSysInitSet = fEnable;

    RtcSetBackupRegister( POWER_MANAGER_DEBUG_ENABLE_RTC_BACKUP_REG_INDEX, fEnable );
}

BOOL PowerManagerIsDebugEnabled( void )
{
    return gfIsEnableDebugAtSysInitSet;
}

/* EOF */