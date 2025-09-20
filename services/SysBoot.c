//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../hal/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

#include "../drivers/common/types.h"
#include "../drivers/common/timer.h"
#include "../drivers/common/stateMachine.h"
#include "../drivers/rtc/rtcBackupReg.h"
#include "SysBoot.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

// clearly indicate rtc index + offset
#define SYS_BOOT_MODE_RTC_BACKUP_REG 	(RTC_BACKUP_REG_INDX_START+1)

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SYS_BOOT_SM_UNINITIALIZED = 0,

	///////////////////////////////////////
    SYS_BOOT_SM_UDEF_NOT_READY_BASIC_SYSTEM_INIT,  	// init basic peripherals and state machines
    SYS_BOOT_SM_UDEF_NOT_READY_MIN_VOLTAGE,			// this is protection state for solar pannel first plug. Initialize other modules that require low voltage. make sure all state machines init as well
    SYS_BOOT_SM_UDEF_NOT_READY_CONSOLE_ENABLE,     	// allow continuous monitoring of console

    SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE, 			// allow user to select manufacturing or normal mode. 5 seconds window
	SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE_DONE,
	///////////////////////////////////////


	///////////////////////////////////////
	// MODE DISABLE
	///////////////////////////////////////
    SYS_BOOT_SM_DISABLE_BOOTING_START,
    SYS_BOOT_SM_DISABLE_BOOTING_END,

    SYS_BOOT_SM_DISABLE_READY,
	///////////////////////////////////////


	///////////////////////////////////////
	// MODE NORMAL
	///////////////////////////////////////
    SYS_BOOT_SM_NORMAL_BOOTING_START,
    SYS_BOOT_SM_NORMAL_BOOTING_END,

    SYS_BOOT_SM_NORMAL_READY,
	///////////////////////////////////////

    SYS_BOOT_SM_MAX
}SysBootStateMachineEnum;

static const CHAR * const gtSysBootModeNameTable[] =
{
    "UNDEFINED",
    "DISABLE",
    "NORMAL"
};

#define SYS_BOOT_MODE_NAME_TABLE_MAX		( sizeof(gtSysBootModeNameTable)/sizeof(gtSysBootModeNameTable[0]) )

static const CHAR * const gtSysBootStatNameTable[] =
{
    "NOT_READY",
    "WAITING",
    "BOOTING",
	"READY"
};

#define SYS_BOOT_STAT_NAME_TABLE_MAX		( sizeof(gtSysBootStatNameTable)/sizeof(gtSysBootStatNameTable[0]) )

static StateMachineType			gtState;
static SysBootModeEnum 			geMode 	= SYS_BOOT_MODE_UNDEFINED;
static SysBootStateEnum 		geState = SYS_BOOT_STATE_NOT_READY;
static BOOL 					gfDebugEnable = FALSE;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void SysBootDisableExternalDevices	( void );
static void SysBootDisablePeripherals		( void );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysBootInit( void )
{
	// init stuff here
	StateMachineInit( &gtState );

	if( SYS_BOOT_MODE_NAME_TABLE_MAX != SYS_BOOT_MODE_MAX )
	{
		while(1);
	}

	if( SYS_BOOT_STAT_NAME_TABLE_MAX != SYS_BOOT_STATE_MAX )
	{
		while(1);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SysBootDebugEnable( BOOL fEnable )
{
	gfDebugEnable = fEnable;
}

BOOL SysBootIsDebugEnabled( void )
{
	return gfDebugEnable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SysBootUpdate( void )
{

	StateMachineUpdate( &gtState );

	switch( gtState.bStateCurrent )
    {
        case SYS_BOOT_SM_UNINITIALIZED:
        {
			StateMachineInit( &gtState );
			geMode 	= SYS_BOOT_MODE_UNDEFINED;
			geState = SYS_BOOT_STATE_NOT_READY;

            StateMachineChangeState( &gtState, SYS_BOOT_SM_UDEF_NOT_READY_BASIC_SYSTEM_INIT );
            break;
        }

        case SYS_BOOT_SM_UDEF_NOT_READY_BASIC_SYSTEM_INIT:
        {
			// initialize state machines from other modules
			// initialize at least adc so that is possible to measure power line voltage in the next state
            StateMachineChangeState( &gtState, SYS_BOOT_SM_UDEF_NOT_READY_MIN_VOLTAGE );
            break;
        }

        case SYS_BOOT_SM_UDEF_NOT_READY_MIN_VOLTAGE:
        {
            StateMachineChangeState( &gtState, SYS_BOOT_SM_UDEF_NOT_READY_CONSOLE_ENABLE );
            break;
        }

        case SYS_BOOT_SM_UDEF_NOT_READY_CONSOLE_ENABLE:
        {
			if( StateMachineIsFirtEntry( &gtState ) )
            {
				// simulate time
				StateMachineSetTimeOut( &gtState, 5000 );
			}
			
			if( StateMachineIsTimeOut( &gtState ) )
            {
				// enable usart
				// enable main console only
				// set console commands
				StateMachineChangeState( &gtState, SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE );
			}
            break;
        }

		case SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE:
		{
			if( StateMachineIsFirtEntry( &gtState ) )
            {
				geState = SYS_BOOT_STATE_WAITING;
				// give 5 seconds for the user to select a booting option
				StateMachineSetTimeOut( &gtState, 5000 );
			}

			if( geMode != SYS_BOOT_MODE_UNDEFINED )
			{
				StateMachineChangeState( &gtState, SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE_DONE );
				break;
			}

            if( StateMachineIsTimeOut( &gtState ) )
            {
				// if nothing slected default goes to normal mode
				geMode 	= SYS_BOOT_MODE_NORMAL;
			}
            break;
        }
		case SYS_BOOT_SM_UDEF_WAITING_SELECT_MODE_DONE:
		{
			// indicate booting up process is going to start
			geState = SYS_BOOT_STATE_BOOTING;
			
			if( geMode == SYS_BOOT_MODE_NORMAL )
			{
				StateMachineChangeState( &gtState, SYS_BOOT_SM_NORMAL_BOOTING_START );
			}
			else
			{
				StateMachineChangeState( &gtState, SYS_BOOT_SM_DISABLE_BOOTING_START );
			}
			break;
		}


		///////////////////////////////////////
		// MODE DISABLE
		///////////////////////////////////////
		case SYS_BOOT_SM_DISABLE_BOOTING_START:
		{
            StateMachineChangeState( &gtState, SYS_BOOT_SM_DISABLE_BOOTING_END );
            break;
        }
		case SYS_BOOT_SM_DISABLE_BOOTING_END:
		{
            StateMachineChangeState( &gtState, SYS_BOOT_SM_DISABLE_READY );
            break;
        }
		case SYS_BOOT_SM_DISABLE_READY:
		{
            geState = SYS_BOOT_STATE_READY;
            break;
        }
		///////////////////////////////////////


		///////////////////////////////////////
		// MODE NORMAL
		///////////////////////////////////////
		case SYS_BOOT_SM_NORMAL_BOOTING_START:
		{
            StateMachineChangeState( &gtState, SYS_BOOT_SM_NORMAL_BOOTING_END );
            break;
        }
		case SYS_BOOT_SM_NORMAL_BOOTING_END:
		{
			if( StateMachineIsFirtEntry( &gtState ) )
            {
				// simulate time
				StateMachineSetTimeOut( &gtState, 10000 );
			}
			
			if( StateMachineIsTimeOut( &gtState ) )
            {
				StateMachineChangeState( &gtState, SYS_BOOT_SM_NORMAL_READY );
			}
            break;
        }
		case SYS_BOOT_SM_NORMAL_READY:
		{
            geState = SYS_BOOT_STATE_READY;
            break;
        }
		///////////////////////////////////////


		default:
		{
            StateMachineChangeState( &gtState, SYS_BOOT_SM_UNINITIALIZED );
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
SysBootModeEnum SysBootGetMode( void )
{
	return geMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysBootSetMode( SysBootModeEnum eSysBootMode )
{
	BOOL fSuccess = FALSE;

	if( eSysBootMode < SYS_BOOT_MODE_MAX )
	{
		if( eSysBootMode != SYS_BOOT_MODE_UNDEFINED )
		{
			if( geMode == SYS_BOOT_MODE_UNDEFINED )
			{
				geMode = eSysBootMode;
				fSuccess = TRUE;
			}
			else
			{
				////////////////////////////////////
				if( eSysBootMode != geMode )
				{
					SysBootDisableExternalDevices();
					SysBootDisablePeripherals();
					RtcBackupRegSet( SYS_BOOT_MODE_RTC_BACKUP_REG, eSysBootMode );
					SysBootForceSoftwareReset();
				}
				////////////////////////////////////
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
CHAR * SysBootGetModeName( SysBootModeEnum eSysBootMode )
{
	if( eSysBootMode < SYS_BOOT_MODE_MAX )
	{
		return &gtSysBootModeNameTable[eSysBootMode][0];
	}
	else
	{
		return NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
SysBootStateEnum SysBootGetState( void )
{
	return geState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * SysBootGetStateName( SysBootStateEnum eSysBootState )
{
	if( eSysBootState < SYS_BOOT_STATE_MAX )
	{
		return &gtSysBootStatNameTable[eSysBootState][0];
	}
	else
	{
		return NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SysBootDisablePeripherals( void )
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SysBootDisableExternalDevices( void )
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void SysBootForceSoftwareReset( void )
{
	// The function initiates a system reset request to reset the MCU.
	NVIC_SystemReset();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

