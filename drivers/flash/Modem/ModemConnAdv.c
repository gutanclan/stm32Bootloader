/** C Header ******************************************************************

*******************************************************************************/

#include <assert.h>
#include <string.h>     /* memset() */
#include <stdio.h>      /* printf() */
#include <ctype.h>      /* isprint() */
#include <stdlib.h>

#include <stdarg.h>         // For va_arg support

#include "Types.h"

// PCOM Project Targets
#include "Target.h"             // Hardware target specifications

#include "Asc.h"
#include "Config.h"
#include "Debug.h"
#include "General.h"
#include "Gpio.h"

//#include "Modem.h"
//#include "Module.h"     /* Common task control structure definition */
#include "StringTable.h"
#include "Timer.h"
#include "Usart.h"
#include "Datalog.h"
#include "Control.h"
#include "Console.h"

#include "../Utils/StateMachine.h"
#include "ModemData.h"
#include "ModemCommandResponse/Modem.h"
#include "ModemCommandResponse/ModemCommand.h"
#include "ModemCommandResponse/ModemResponse.h"

#include "ModemOperations/ModemPower.h"
#include "ModemOperations/ModemConfig.h"
#include "ModemOperations/ModemInfo.h"
#include "ModemOperations/ModemSim.h"
#include "ModemOperations/ModemRegStat.h"
#include "ModemOperations/ModemConnect.h"
#include "ModemOperations/ModemCgRegStat.h"
#include "ModemOperations/ModemSetSysClock.h"
#include "ModemOperations/ModemCellInfo.h"
#include "ModemOperations/ModemSignal.h"
#include "ModemOperations/ModemFtp.h"
#include "ModemOperations/ModemFtpOper.h"

#include "ModemConnAdv.h"

#include "Led.h"

typedef enum
{
    MODEM_CONN_ADV_STATE_UNINITIALIZED = 0,
    
    MODEM_CONN_ADV_STATE_INITIALIZING_ALL_MODULES_READY,
    MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_ON,
    MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_MODEM_INFO,
    MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_SIM_INFO,
    MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF,

    MODEM_CONN_ADV_STATE_DISABLED,
    
    MODEM_CONN_ADV_STATE_POWER_OFF_IDLE,
    
    MODEM_CONN_ADV_STATE_CONNECT_START,    
    MODEM_CONN_ADV_STATE_CONNECT_POWER_ON,
    MODEM_CONN_ADV_STATE_CONNECT_GATHERING_MODEM_INFO,
    MODEM_CONN_ADV_STATE_CONNECT_RUN_CONFIGS,
    MODEM_CONN_ADV_STATE_CONNECT_WAITING_TIME,
    MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIM,
    MODEM_CONN_ADV_STATE_CONNECT_RUN_CONNECT,
    MODEM_CONN_ADV_STATE_CONNECT_CHECK_TIME,
    MODEM_CONN_ADV_STATE_CONNECT_CHECK_CELL,    
    MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIGNAL,
    
    MODEM_CONN_ADV_STATE_CONNECT_IDLE,
    
    MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF,
    
    MODEM_CONN_ADV_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemSysStateMachineName[MODEM_CONN_ADV_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "conn adv init all mods ready",
    "conn adv init power on",
    "conn adv init get mdm info",
    "conn adv init get Sim info",
    "conn adv init power off",
    
    "conn adv disabled",
    
    "conn adv power off idle",
    
    "conn adv conn start",    
    "conn adv conn power on",
    "conn adv conn gather modem info",
    "conn adv conn run configs",
    "conn adv conn waiting time",
    "conn adv conn check sim",
    "conn adv conn run connect",
    "conn adv conn check time",
    "conn adv conn check cell",    
    "conn adv conn check signal",
    
    "conn adv conn idle",
    
    "conn adv power off run power off",
};

#if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
#define MODEM_CONN_ADV_FIRMWARE_VERSION_STRING          ("12.00.324")
#else
#define MODEM_CONN_ADV_FIRMWARE_VERSION_STRING          ("12.00.323")
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
static TIMER                    gtRegisteringTimer;
static UINT32                   gdwRegisteringTime_mSec;

static BOOL                     gfIsAutomaticOperationEnabled;
static BOOL                     gfIsConnected;
static UINT32                   gdwOperationTimeOut_mSec;

//
static ModemDataType           *gpModemData;

// operation
static BOOL                     gfIsWaitingForNewCommand;
static ModemStateMachineType    gtStateMachine;
static ModemCommandSemaphoreEnum geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConnAdvInit( void )
{
    
    // ### INIT OTHER MODULES ### 
    
    ModemModuleInit();
    ModemPowerInit();
    ModemRegStatInit();
    ModemConfigInit();
    ModemInfoInit();
    ModemSimInit();
    ModemSetSysClockInit();
    ModemConnectInit();
    ModemCgRegStatInit();
    ModemCellInfoInit();
    ModemSignalInit();
    ModemFtpInit();
    ModemFtpOperationInit();
    

    ////////////////////////////////////////////////////////////////////////////////////////
    // modem sys time configuration
    ////////////////////////////////////////////////////////////////////////////////////////
    ModemSetSysClockSetDeltaTimeCorrection( 10 );

//    ////////////////////////////////////////////////////////////////////////////////////////
//    // modem connection configuration
//    ////////////////////////////////////////////////////////////////////////////////////////
//    ModemConnectPdpConfigType       tPdpConfig;
//    ModemConnectSocketConfigType    tSocketConnConfig;
//    
//    memset( &tPdpConfig, 0, sizeof(tPdpConfig) );
//    memset( &tSocketConnConfig, 0, sizeof(tSocketConnConfig) );
//
//    // set connection configurations
//    tPdpConfig.bPdpCntxtId              = 1;
//    tPdpConfig.bPdpDataCompression      = 0;
//    tPdpConfig.bPdpHeaderCompression    = 0;
//    snprintf( &tPdpConfig.szPdpProtocol[0], sizeof(tPdpConfig.szPdpProtocol),   "IP" );
//    //snprintf( &tPdpConfig.szApn[0],         sizeof(tPdpConfig.szApn),           ModemConnectApnGetString(MODEM_CONNECT_APN_ADDRESS_BELL_M2M) );
//    snprintf( &tPdpConfig.szApn[0],         sizeof(tPdpConfig.szApn),           "hello" );
//    snprintf( &tPdpConfig.szPdpAddress[0],  sizeof(tPdpConfig.szPdpAddress),    "0.0.0.0" );    
//    //
//    tSocketConnConfig.bSocketConnId             = 1;
//    tSocketConnConfig.bPdpCntxtId               = tPdpConfig.bPdpCntxtId;
//    tSocketConnConfig.wDataProtocolPacketSize   = 1500;
//    tSocketConnConfig.wDataExchangeTimeOut_Sec  = 900;
//    tSocketConnConfig.wConnTimeOut_x100_mSec    = 600;  // 600*100 = 60,000 mSec
//    tSocketConnConfig.wDataSendTimeOut_x100_mSec= 50;   // 50*100 = 5,000 mSec
//
//    ModemConnectConfigPdp( &tPdpConfig );
//    ModemConnectConfigSocket( &tSocketConnConfig );
//
//    ////////////////////////////////////////////////////////////////////////////////////////
//    // modem ftp server configuration
//    ////////////////////////////////////////////////////////////////////////////////////////
//    ModemFtpConnType tFtpConn;
//
//    memset( &tFtpConn, 0, sizeof(tFtpConn) );
//    //#define MODEM_FTP_SERVER_TEST (1)
//    #ifdef MODEM_FTP_SERVER_TEST
//    tFtpConn.dwPort     = 21;
//    snprintf( &tFtpConn.szAddress[0],  sizeof(tFtpConn.szAddress),  "216.251.32.98" );    
//    snprintf( &tFtpConn.szUserName[0], sizeof(tFtpConn.szUserName), "puracom.afti.ca" );    
//    snprintf( &tFtpConn.szPassword[0], sizeof(tFtpConn.szPassword), "puracompw" );        
//    #else
//    tFtpConn.dwPort     = 21;
//    snprintf( &tFtpConn.szAddress[0],  sizeof(tFtpConn.szAddress),  "184.69.196.210" );    
//    snprintf( &tFtpConn.szUserName[0], sizeof(tFtpConn.szUserName), "WD3" );    
//    snprintf( &tFtpConn.szPassword[0], sizeof(tFtpConn.szPassword), "puracompw" );    
//    #endif
//
//    ModemFtpConnectSetConfig( &tFtpConn );
    
    ////////////////////////////////////////////////////////////////////////////////////////
    // modem printing enable
    ////////////////////////////////////////////////////////////////////////////////////////
    ModemEventLogEnable( TRUE );

    ModemConsolePrintRxEnable( TRUE );
    ModemConsolePrintTxEnable( TRUE );
    //ModemConsolePrintDbgEnable( TRUE );
    ModemConsolePrintfEnable( TRUE );

    // enable automatic processing of incoming chars
    ModemRxProcessKeyAutoUpdateEnable( TRUE );

    
    
    // ### INIT ITS OWN MODULE ### 
    
    // set state machine to a initial state.
    StateMachineInit( &gtStateMachine.tState );

    // get main data structure pointer
    gpModemData = ModemResponseModemDataGetPtr();    
    
    if( gpModemData == NULL )
    {
        // catch this bug on development time
        while(1);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConnAdvEnable( BOOL fAutoEnable )
{
    gfIsAutomaticOperationEnabled = fAutoEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConnAdvIsEnabled( void )
{
    return gfIsAutomaticOperationEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConnAdvConnectSetTimeOut( UINT32 dwConnectTimeOut_mSec )
{
    gdwOperationTimeOut_mSec = dwConnectTimeOut_mSec;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ModemConnAdvConnectGetTimeOut( void )
{
    return gdwOperationTimeOut_mSec;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConnAdvConnectRun( BOOL fModemEnable )
{    
    if( gfIsAutomaticOperationEnabled )
    {
        // force shut down or stop of automatic operations
        if( fModemEnable == FALSE )
        {            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            // indicate command will start running so more commands are not accepted
            gfIsWaitingForNewCommand = FALSE;
        }
        else // if a run auto operations is requested, check if is not already running
        if( gfIsWaitingForNewCommand )       
        {
            if( gtStateMachine.tState.bStateCurrent == MODEM_CONN_ADV_STATE_POWER_OFF_IDLE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_START );
                // indicate command will start running so more commands are not accepted
                gfIsWaitingForNewCommand = FALSE;
            }            
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConnAdvConnectIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemConnAdvConnectIsConnected( void )
{
    return gfIsConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemConnAdvStateMachine( void )
{
    // OTHER MODULES 
    ModemRxProcessKeyAutoUpdate();
    
    ModemPowerStateMachine();   // all modules that depends on POWER module to operate should be placed below
    ModemRegStatStateMachine();
    ModemConfigStateMachine();
    ModemInfoStateMachine();        
    ModemSimStateMachine();    

    ModemConnectStateMachine(); // all modules that depends on CONNECT module to operate should be placed below
    ModemCgRegStatStateMachine();
    ModemSetSysClockStateMachine();
    ModemCellInfoStateMachine();
    ModemSignalStateMachine();
    
    ModemFtpStateMachine();     // all modules that depends on FTP module to operate should be placed below
    ModemFtpOperationStateMachine();


    // OWN MODULE
    StateMachineUpdate( &gtStateMachine.tState );

    
    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    
    switch( gtStateMachine.tState.bStateCurrent )
    {        
        case MODEM_CONN_ADV_STATE_UNINITIALIZED:
        {
            // go to waiting for operation to run
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_ALL_MODULES_READY );
            break;
        }
        
        case MODEM_CONN_ADV_STATE_INITIALIZING_ALL_MODULES_READY:
        {
            if(  ModemPowerIsWaitingForCommand() == TRUE )
            {
               StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_ON );
            }
            break;
        }
        
        case MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_ON:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemPowerEnablePower( TRUE );
                StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_MODEM_INFO );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,                    
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF );
            }
            break;
        }
        
        case MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_MODEM_INFO:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemInfoRun();
                StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
            }
            
            if( ModemInfoIsWaitingForCommand() == TRUE )
            {
                if( ModemInfoIsExtracted() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_SIM_INFO );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_INITIALIZING_GATHERING_SIM_INFO:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemSimCheckRun();
                StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                break;
            }
            
            if( ModemSimIsWaitingForCommand() == TRUE )
            {
                if( ModemSimIsReady() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF );
            }
            break;
        }
        
        case MODEM_CONN_ADV_STATE_INITIALIZING_POWERING_OFF:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemPowerEnablePower( FALSE );
                StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() == FALSE )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_DISABLED );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_DISABLED );
            }
            break;
        }
        
        
        case MODEM_CONN_ADV_STATE_DISABLED:
        {
            //////////////////////////////////////////////////
            // DISABLED
            //////////////////////////////////////////////////
        
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemConsolePrintf( "Advance Connection DISABLED\r\n" );
            }

            gfIsConnected = FALSE;
            gfIsWaitingForNewCommand    = FALSE;
        
            if( gfIsAutomaticOperationEnabled )
            {
                ModemConsolePrintf( "Advance Connection ENABLED\r\n" );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_IDLE );
            }
            break;
        }
        
        
        case MODEM_CONN_ADV_STATE_POWER_OFF_IDLE:
        {
            //////////////////////////////////////////////////
            // POWER OFF
            //////////////////////////////////////////////////
        
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemConsolePrintf( "Mdm Sys Network Disconnected\r\n" );
                
                // set LED pattern
                LED_setLedBlinkPattern( LED1_POWER, LED_PATTERN_ON );
            }

            gfIsConnected = FALSE;
            gfIsWaitingForNewCommand    = TRUE;
        
            if( ModemPowerIsPowerEnabled() ) // in case somebody turn on modem manually, start running automatic operations
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_START );
            }
            
            
        
            if( gfIsAutomaticOperationEnabled == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_DISABLED );
            }
            break;
        }
        

        case MODEM_CONN_ADV_STATE_CONNECT_START:
        {
            // start timer 
            gdwRegisteringTime_mSec = 0;
            gtRegisteringTimer = TimerUpTimerStartMs();

            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_POWER_ON );
            break;
        }
                        
        case MODEM_CONN_ADV_STATE_CONNECT_POWER_ON:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // set LED pattern
                LED_setLedBlinkPattern( LED1_POWER, LED_PATTERN_SLOW );

                if( ModemPowerIsWaitingForCommand() )
                {
                    if( ModemPowerIsPowerEnabled() == FALSE )
                    {
                        ModemPowerEnablePower( TRUE );
                        StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
                    }
                    else
                    {
                        // go to next
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_GATHERING_MODEM_INFO );
                        break;
                    }
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "pwr mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_GATHERING_MODEM_INFO );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_GATHERING_MODEM_INFO:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemInfoIsWaitingForCommand() )
                {
                    ModemInfoRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                    (
                        TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "cnf mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );                    
                }
                break;
            }
            
            if( ModemInfoIsWaitingForCommand() == TRUE )
            {
                if( ModemInfoIsExtracted() )
                {
                    // check if firmware version is expected
                    if( strcmp( &gpModemData->tModemInfo.szFirmwareVer[0], MODEM_CONN_ADV_FIRMWARE_VERSION_STRING ) == 0 )
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_RUN_CONFIGS );
                    }
                    else
                    {
                        // Set ERROR
                        ModemEventLog
                        (
                            TRUE,
                            "conn adv[%s] Error='%s'", 
                            cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                            "modem FW mismatch" 
                        );
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    }
                    break;
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                    (
                        TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "operation fail" 
                    );
                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_RUN_CONFIGS:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemConfigIsWaitingForCommand() )
                {                    
                    ModemConfigRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );                    
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "cnf mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemConfigIsWaitingForCommand() == TRUE )
            {
                if( ModemConfigIsSuccess() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_WAITING_TIME );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }
        
        case MODEM_CONN_ADV_STATE_CONNECT_WAITING_TIME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                StateMachineSetTimeOut( &gtStateMachine.tState, 500 );
            }

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIM );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIM:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemSimIsWaitingForCommand() )
                {                    
                    ModemSimCheckRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );                    
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "sim mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemSimIsWaitingForCommand() == TRUE )
            {
                if( ModemSimIsReady() )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_RUN_CONNECT );
                    break;
                }
                else
                {
                    // log error. sim not inserted
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "SIM NOT READY" 
                    );

                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_RUN_CONNECT:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // only if disconnected
                if( ModemConnectIsConnected() == FALSE )
                {
                    if( ModemConnectIsWaitingForCommand() )
                    {                                        
                        ModemConnectRun( TRUE, gdwOperationTimeOut_mSec );   
                        StateMachineSetTimeOut( &gtStateMachine.tState, gdwOperationTimeOut_mSec );                    
                    }
                    else
                    {                    
                        // Set ERROR
                        ModemEventLog
                (
                    TRUE,
                            "conn adv[%s] Error='%s'", 
                            cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                            "conn mod not waiting for command" 
                        );
            
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );                        
                    }                    
                }
                else
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_CHECK_TIME );
                }
                break;
            }
            
            if( ModemConnectIsWaitingForCommand() == TRUE )
            {
                if( ModemConnectIsConnected() )
                {
                    // modem connected, but check signal before going to CONNECTED IDLE
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_CHECK_TIME );
                    break;
                }
                else
                {
                    // log error. sim not inserted
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "connect attempt failed" 
                    );

                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_CHECK_TIME:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemSetSysClockIsWaitingForCommand() )
                {
                    ModemSetSysClockRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );                    
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "sysClk mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemSetSysClockIsWaitingForCommand() == TRUE )
            {                
                // modem connected
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_CHECK_CELL );
                break;                                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_CHECK_CELL:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCellInfoIsWaitingForCommand() )
                {
                    ModemCellInfoCheckRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 7000 );                    
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "cellinf mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemCellInfoIsWaitingForCommand() == TRUE )
            {
                // even if it doesnt get all the cell information, 
                // dont fail on CONNECTION since is not a critical requirement to transmit data
                //if( ModemCellInfoIsReady() )                
                {
                    // modem connected
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIGNAL );
                    break;
                }                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }
        
        case MODEM_CONN_ADV_STATE_CONNECT_CHECK_SIGNAL:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemSignalIsWaitingForCommand() )
                {
                    ModemSignalRun();
                    StateMachineSetTimeOut( &gtStateMachine.tState, 5000 );                    
                }
                else
                {                    
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "conn adv[%s] Error='%s'", 
                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "sig mod not waiting for command" 
                    );
            
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
                    break;
                }
            }
            
            if( ModemSignalIsWaitingForCommand() == TRUE )
            {
                if( ModemSignalIsSignalAcquired() )                
                {
                    // modem connected
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_CONNECT_IDLE );
                    break;
                }                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_CONNECT_IDLE:
        {
            //////////////////////////////////////////////////
            // CONNECTED
            //////////////////////////////////////////////////
        
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                gdwRegisteringTime_mSec = TimerUpTimerGetMs( gtRegisteringTimer );
                // Log msg
                ModemEventLog(FALSE,"Net Registration time=%d(ms)", gdwRegisteringTime_mSec );
                ModemConsolePrintf( "Mdm Sys Network Connected in %d(mSec)\r\n", gdwRegisteringTime_mSec );
            }

            gfIsConnected = TRUE;
            gfIsWaitingForNewCommand    = TRUE;
        
            if( ModemPowerIsPowerEnabled() == FALSE ) // in case somebody turn off modem manually, fall back state MODEM_CONN_ADV_STATE_POWER_OFF_IDLE
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_IDLE );
            }
            
            if( ModemConnectIsConnected() == FALSE ) // in case somebody turn off modem manually, fall back state MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF );
            }
        
            if( gfIsAutomaticOperationEnabled == FALSE )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_DISABLED );
            }
            break;
        }

        case MODEM_CONN_ADV_STATE_POWER_OFF_RUN_POWER_OFF:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                ModemPowerEnablePower( FALSE );
                StateMachineSetTimeOut( &gtStateMachine.tState, 10000 );
            }
            
            if( ModemPowerIsWaitingForCommand() == TRUE )
            {
                if( ModemPowerIsPowerEnabled() == FALSE )
                {
                    // modem is power on
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_IDLE );
                    break;
                }
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "conn adv[%s] Error='%s'", 
                    cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "timeout" 
                );
                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_POWER_OFF_IDLE );
            }
            break;
        }


//        case MODEM_CONN_ADV_STATE_RUN_IS_SIM_READY:
//        {
//            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
//            {
//                if( ModemCommandProcessorReserve( &geSemaphore ) )
//                {
//                    ModemCommandProcessorResetResponse();                    
//                    ModemCommandProcessorSetExpectedResponse( TRUE, "+CPIN:", 1, TRUE );
//                    
//                    // if need to wait for response set time out
//                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
//                    ModemCommandProcessorSendAtCommand( "+CPIN?" );
//                }
//                else
//                {
//                    // Set ERROR
//                    ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "semaphore busy" 
//                    );
//
//                    // before change state always release semaphore
//                    ModemCommandProcessorRelease( &geSemaphore );
//                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//                    break;
//                }                        
//            }
//            
//            if( ModemCommandProcessorIsExpectedResponseReceived() )
//            {
//                // check if response indicate sim is ready
//                if( gpModemData->tSimCard.fIsSimReady )
//                {
//                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_RUN_CIMI );
//                }
//                else
//                {
//                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//                }
//
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );                
//                break;
//            }            
//
//            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
//            {
//                // Set ERROR
//                ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "timeout" 
//                    );
//
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );
//                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//            }
//            break;
//        }
//            
//        case MODEM_CONN_ADV_STATE_RUN_CIMI:
//        {
//            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
//            {
//                if( ModemCommandProcessorReserve( &geSemaphore ) )
//                {
//                    ModemCommandProcessorResetResponse();
//                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CIMI:", 1, TRUE );
//                    
//                    // if need to wait for response set time out
//                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
//                    ModemCommandProcessorSendAtCommand( "#CIMI" );
//                }
//                else
//                {
//                    // Set ERROR
//                    ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "semaphore busy" 
//                    );
//
//                    // before change state always release semaphore
//                    ModemCommandProcessorRelease( &geSemaphore );
//                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//                    break;
//                }                        
//            }
//            
//            if( ModemCommandProcessorIsExpectedResponseReceived() )
//            {
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );
//                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_RUN_CCID );
//                break;
//            }            
//
//            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
//            {
//                // Set ERROR
//                ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "timeout" 
//                    );
//
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );
//                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//            }
//            break;
//        }
//            
//        case MODEM_CONN_ADV_STATE_RUN_CCID:
//        {
//            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
//            {
//                if( ModemCommandProcessorReserve( &geSemaphore ) )
//                {
//                    ModemCommandProcessorResetResponse();
//                    ModemCommandProcessorSetExpectedResponse( TRUE, "#CCID:", 1, TRUE );
//                    
//                    // if need to wait for response set time out
//                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
//                    ModemCommandProcessorSendAtCommand( "#CCID" );
//                }
//                else
//                {
//                    // Set ERROR
//                    ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "semaphore busy" 
//                    );
//
//                    // before change state always release semaphore
//                    ModemCommandProcessorRelease( &geSemaphore );
//                    StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//                    break;
//                }
//            }
//            
//            if( ModemCommandProcessorIsExpectedResponseReceived() )
//            {
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );
//                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_PASS );
//                break;
//            }
//
//            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
//            {
//                // Set ERROR
//                ModemErrorLog
//                    (
//                        "sim[%s] Error='%s'", 
//                        cModemSysStateMachineName[gtStateMachine.tState.bStateCurrent], 
//                        "timeout" 
//                    );
//
//                // before change state always release semaphore
//                ModemCommandProcessorRelease( &geSemaphore );
//                StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_FAIL );
//            }
//            break;
//        }
//        
//        case MODEM_CONN_ADV_STATE_PASS:
//        {
//
//            ModemConsolePrintf( "Modem SIM pass\r\n" );
//
//            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_END );
//            break;
//        }
//        
//        case MODEM_CONN_ADV_STATE_FAIL:
//        {
//            // clear sim data strings
//            gpModemData->tSimCard.fIsSimReady = FALSE;
//            gpModemData->tSimCard.szCimei[0] = '\0';
//            gpModemData->tSimCard.szIccId[0] = '\0';   
//                        
//            ModemConsolePrintf( "Modem SIM fail\r\n" );
//            
//            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_END );
//            break;
//        }
//        
//        case MODEM_CONN_ADV_STATE_END:
//        {
//            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_IDLE );
//            
//            break;
//        }
                   
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_CONN_ADV_STATE_UNINITIALIZED );            
            break;
        }
    }
}

