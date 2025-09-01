//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		Timer.c
//!	\brief		Up and down timers, retry timers and delays using FreeRTOS calls.
//!
//!	\author		Scot Kornak, Puracom Inc.
//! \date		April 11, 2011
//!
//! \details	Up and down timers and delays.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

//#include "../includesProject.h"
#include "Types.h"
#include "Timer.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Module Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

#define TIMER_DOWN_TIMER_DELAY_MAX_TICKS        (0x40000000)  /* Max ticks for timer calculations */
#define TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS	(0xFFFFFFFF)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Variable Declarations
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Routines
//////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     TIMER    TimerDownTimerStartMs( UINT32 dwDurationMs )
//!
//! \brief  Start a new down timer. The maximum duration is DOWN_TIMER_DELAY_MAX_TICKS in
//!         ticks, which is 1/4 of the maximum value of data type (UINT32 or UINT16).
//!
//!         If the time requested exceeds this value, it is set to DOWN_TIMER_DELAY_MAX_TICKS.
//!
//!         Usage:  tTimer = TimerDownTimerStartMs( 150 );  // Set timer for 150ms
//!                 ...
//!                 if( TimerDownTimerIsExpired(tTimer) )
//!                 {}
//!
//! \param[in]  dwDurationMS        Down timer duration in milliseconds.
//!
//!	\return     A timer instance, which is nothing more than the system tick count when the timer
//!             expires.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
TIMER TimerDownTimerStartMs( UINT32 dwDuration_mSec )
{
    UINT32  dwCurrentTickCount;
    UINT32  dwDuration_ticks;
    UINT32  dwTickRate_mSec;

    dwCurrentTickCount  = SysTickGetTicksSincePowerUpSmallCounter();
    dwTickRate_mSec     = 1000 / SysTickGetTickRate();

    // convert duration in msec to duration in ticks
    dwDuration_ticks    = dwDuration_mSec / dwTickRate_mSec;

    // The maximum duration is limited to 25% of the maximum tick range
    // to allow rollover to be handled easily.
    if( dwDuration_ticks > TIMER_DOWN_TIMER_DELAY_MAX_TICKS )
    {
        dwDuration_ticks = TIMER_DOWN_TIMER_DELAY_MAX_TICKS;
    }

    // Determine if rollover will occur before the timer expires.
    // i.e. The current tick value plus the delay > TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS .
    if( ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - dwCurrentTickCount ) > dwDuration_ticks )
    {
        // The tick counter won't roll over before timer expires
        return ( dwCurrentTickCount + dwDuration_ticks );
    }
    else
    {
        // The tick counter WILL roll over before timer expires
        return ( dwDuration_ticks - ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - dwCurrentTickCount ));
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL TimerDownTimerIsExpired( TIMER dwExpiryTickValue )
//!
//! \brief      Check if a down timer has expired.
//!
//! \param[in]  xDownTimer       Down timer expires at this tick value.
//!
//!	\return		BOOL  True if timer expired.  False if timer not expired.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TimerDownTimerIsExpired( TIMER xDownTimerExpiryTick )
{
    UINT32  dwCurrentTickCount;

    dwCurrentTickCount = SysTickGetTicksSincePowerUpSmallCounter();

    // If the expiry value is in the first quadrant,
    // and the current tick value is in the fouth quadrant,
    // the timer is not expired.  i.e. roll over hasn't occurred yet.
    if
    (
        ( xDownTimerExpiryTick <= TIMER_DOWN_TIMER_DELAY_MAX_TICKS ) &&
        ( dwCurrentTickCount > ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - TIMER_DOWN_TIMER_DELAY_MAX_TICKS ) )
    )
    {
        return FALSE;
    }

    // Check if timer expired
    if( dwCurrentTickCount >= xDownTimerExpiryTick )
    {
        /* Timer expired */
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         UINT32 TimerDownTimerGetMsLeft( TIMER xDownTimerExpiryTick )
//!
//! \brief      Read millisecond ticks remaining on a down timer.
//!
//! \param[in]  xDownTimer       Down timer expires at this tick value.
//!
//!	\return		The number to system ticks remaining until the timer expires. Zero if timer has
//!             already expired.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TimerDownTimerGetMsLeft( TIMER xDownTimerExpiryTick )
{
    UINT32  dwCurrentTickCount;

    dwCurrentTickCount = SysTickGetTicksSincePowerUpSmallCounter();

    // If the expiry value is in the first quadrant,
    // and the current tick value is in the fouth quadrant,
    // the timer is not expired.  i.e. roll over hasn't occurred yet.
    if( ( xDownTimerExpiryTick <= TIMER_DOWN_TIMER_DELAY_MAX_TICKS ) &&
        ( dwCurrentTickCount > ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - TIMER_DOWN_TIMER_DELAY_MAX_TICKS ) ) )
    {
        return ( ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - dwCurrentTickCount ) + xDownTimerExpiryTick );
    }

    /* Check if timer expired */
    if( dwCurrentTickCount >= xDownTimerExpiryTick )
    {
        /* Timer expired */
        return 0;
    }

    /* Return ticks remaining */
    return ( xDownTimerExpiryTick - dwCurrentTickCount );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         void TimerBlockingDelayMs( UINT32 dwDuration_mSec )
//!
//! \brief      Delay for a number of milliseconds blocking the progress of the app.
//!
//! \param[in]  dwDuration_mSec - Delay time in milliseconds.
//!
//!	\return
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void TimerBlockingDelayMs( UINT32 dwDuration_mSec )
{
    TIMER xTimer;

    xTimer = TimerDownTimerStartMs( dwDuration_mSec );
    while( TimerDownTimerIsExpired( xTimer ) == FALSE );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         UINT32 TimerCurrentTicks( void )
//!
//! \brief      get current ticks of the system
//!
//! \param[in]
//!
//!	\return     system current ticks
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TimerGetSysCurrentTicks( void )
{
    return SysTickGetTicksSincePowerUpSmallCounter();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn         UINT32 TimerUpTimerStartMs( void )
////!
////! \brief      Start a new Up timer.
////!             Note: The maximum duration is UP_TIMER_DELAY_MAX_TICKS in ticks.
////!
////! 	USAGE   dwNewUpTimerMs = TimerUpTimerStartMs();
////!             ...
////!             dwUpTimeMs= TIM_fUpTimerReadMs( dwNewUpTimer );
////!
////! \param[in]  None.
////!
////!	\return     Returns UINT32, system tick value at timer start.
////!             Save this value to read the up timer later.
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TimerUpTimerStartMs( void )
{
    return SysTickGetTicksSincePowerUpSmallCounter();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn         UINT32 TimerUpTimerGetMs( UINT32 dwUpTimerMs )
////!
////! \brief      Read an Up timer to get milliseconds since it was started.
////!
////! \param[in]  dwUpTimerMs - Up timer start value in milliseconds.
////!
////!	\return     Returns UINT32, millisecond since up time start.
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 TimerUpTimerGetMs( UINT32 xUpTimerStartTime_mSec )
{
    UINT32 dwCurrentTickCount;
    UINT32 dwDurationTicks;

    dwCurrentTickCount = SysTickGetTicksSincePowerUpSmallCounter();

    if( dwCurrentTickCount >= xUpTimerStartTime_mSec )
    {
        // No rollover has occurred
        dwDurationTicks = dwCurrentTickCount - xUpTimerStartTime_mSec;
    }
    else
    {
        // Rollover has occurred
        dwDurationTicks = ( TIMER_TICKS_SINCE_POWER_UP_MAX_TICKS - xUpTimerStartTime_mSec ) + dwCurrentTickCount;
    }

    return dwDurationTicks;
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn       void vApplicationTickHook( void )
////!
////! \brief    User-space interrupt handler to handle the total number of ticks that have occurred.
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
//void vApplicationTickHook( void );
//void vApplicationTickHook( void )
//{
//    TimerTotalTickInterruptHandler();
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn       UINT64 TimerGetTotalTicksSincePowerup( void )
////!
////! \brief    Get the total number of ticks that have occurred since system power-up.
////!
////! \return   UINT64, Total number of system ticks that have occurred since power-up (in milliseconds).
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
//UINT64 TimerGetTotalTicksSincePowerup( void )
//{
//     UINT64     qwTickCount;
//
//	// Critical section required as we are grabbing a 64-bit value
//	taskENTER_CRITICAL();
//	{
//		qwTickCount = gqwTimerTotalTickCount;
//	}
//	taskEXIT_CRITICAL();
//
//	return qwTickCount;
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn       void TimerTotalTickInterruptHandler( void )
////!
////! \brief    User-space interrupt handler to handle the total number of ticks that have occurred.
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
//void TimerTotalTickInterruptHandler( void )
//{
//    gqwTimerTotalTickCount++;
//}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////

