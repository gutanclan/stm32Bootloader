//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        System.h
//!    \brief       Functions for the main System.
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "Console.h"

typedef enum
{
    SYSTEM_SLEEP_MODE_STAND_BY = 0,
    SYSTEM_SLEEP_MODE_STOP,

    SYSTEM_SLEEP_MODE_MAX,
}SystemSleepModeEnum;

#define SYSTEM_INTERRUPT_MASK_CONSOLE_RX        (1<<0)
#define SYSTEM_INTERRUPT_MASK_RTC_TIME_MATCH    (1<<1)
#define SYSTEM_INTERRUPT_MASK_RTC_PA0_WKUP      (1<<2)
#define SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_1 (1<<3)
#define SYSTEM_INTERRUPT_MASK_RTC_REED_SWITCH_2 (1<<4)

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SystemClockInitialize		( void );
BOOL SystemTickInitialize		( void );
void SystemGetClockFreq			( RCC_ClocksTypeDef *ptSystemClocks );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// WARNING! it is recommended to shut down peripherals 
// before going to any sleep mode and to start up after coming back from sleep mode
void SystemShutDownPeripherals  ( UINT32 dwWkUpInterruptMaskEnable );
BOOL SystemSleep				( SystemSleepModeEnum eSleepMode );
void SystemSoftwareReset		( void );
void SystemStartUpPeripherals   ( UINT32 dwWkUpInterruptMaskEnable );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void SystemWatchdogRestart      ( void );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif 	//_SYSTEM_H_




