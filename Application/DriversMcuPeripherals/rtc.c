//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Rtc.c
//!    \brief       Functions for Real Time Clock (RTC).
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <stdio.h>
#include <string.h>
#include <time.h>

//#include "../includesProject.h"
#include "../../inc/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.
#include "../Utils/Types.h"

#include "rtc.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local DEFINES
//////////////////////////////////////////////////////////////////////////////////////////////////

#define RTC_BACKUP_DATA_REGS_AMOUNT             (20)

#define RTC_INTERRUPT_PREEMPTION_PRIORITY       (0)
#define RTC_INTERRUPT_TIME_MATCH_SUB_PRIORITY   (0x0F)

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variable
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    RTC_CONFIG_OSC_PLL,
    RTC_CONFIG_TIME_SYNC,

    RTC_CONFIG_MAX
}RtcConfigEnum;

static const UINT32         dwBakupDataRegisters[RTC_BACKUP_DATA_REGS_AMOUNT] =
{
    RTC_BKP_DR0,    RTC_BKP_DR1,    RTC_BKP_DR2,
    RTC_BKP_DR3,    RTC_BKP_DR4,    RTC_BKP_DR5,
    RTC_BKP_DR6,    RTC_BKP_DR7,    RTC_BKP_DR8,
    RTC_BKP_DR9,    RTC_BKP_DR10,   RTC_BKP_DR11,
    RTC_BKP_DR12,   RTC_BKP_DR13,   RTC_BKP_DR14,
    RTC_BKP_DR15,   RTC_BKP_DR16,   RTC_BKP_DR17,
    RTC_BKP_DR18,   RTC_BKP_DR19
};

static RtcClockSourceEnum   geCurrentSourceClock            = RTC_SOURCE_CLOCK_UNKNOWN;
volatile static UINT32      gdwInterruptWKUPRolloverCounter = 0;
static BOOL                 gfIsInterruptTimeMatchEnabled   = FALSE;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL RtcSetBackupRegister( UINT8 bBkpRegIndex, UINT32 dwValue );
static BOOL RtcGetBackupRegister( UINT8 bBkpRegIndex, UINT32 *pdwValue );
static BOOL RtcConfig           ( RtcClockSourceEnum eSourceClock, RtcConfigEnum eConfigType );

// Interruptions should not be static
void        RTC_Alarm_IRQHandler( void );
void        RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_IRQ_HNDLR    ( void );

///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcInitialize( RtcClockSourceEnum eSourceClock )
//!
//! \brief  Initialize Rtc peripheral
//!
//! \param[in]  eSourceClock   Indicate the source clock for rtc (LSI or LSE)
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcInitialize( RtcClockSourceEnum eSourceClock )
{
    BOOL        fSuccess            = FALSE;
    BOOL        fIsRtcConfigured    = FALSE;

    if( eSourceClock < RTC_SOURCE_CLOCK_MAX )
    {
        //////////////////////////////////////////////////
        // check if registers are configured correctly
        //////////////////////////////////////////////////
        // NOTE: Last time this library was tested,
        // LSI was not holding its configured values
        // after any reset, even with power ON all the time.
        // (LSE working fine!!)
        fIsRtcConfigured = TRUE;
        fIsRtcConfigured &= RtcIsRtcConfigured();
        fIsRtcConfigured &=                                                     // check Any of the oscillators is ready
        (
            ( RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET ) ||
            ( RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == SET )
        );
        fIsRtcConfigured &= ( ( ( RCC->BDCR & 0x300 ) >> 8 ) == eSourceClock ) ;// check the OSC selected matches with the one config in register
        //////////////////////////////////////////////////

        if( fIsRtcConfigured )
        {
            // Rtc already configured
            fSuccess = RtcConfig( eSourceClock, RTC_CONFIG_TIME_SYNC );
        }
        else
        {
            fSuccess = RtcConfig( eSourceClock, RTC_CONFIG_OSC_PLL );

            // SET time for first time
            RtcSetBackupRegister( 0, 0 );              // Clear RTC configured BCKUP REG

            if( fSuccess )
            {
                RtcDateTimeStruct tDateTime;

                RtcDateTimeStructInit( &tDateTime );

                // Set preferred default start time for demos
                RtcDateTimeStructInit( &tDateTime );
                fSuccess = RtcDateTimeSet( &tDateTime );

                RtcSetBackupRegister( 0, 1 );              // Indicate RTC has been configured
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcSourceClockSet( RtcClockSourceEnum eSourceClock )
//!
//! \brief  set the clock source for RTC
//!
//! \param[in]  eSourceClock   source clock for rtc (LSI or LSE)
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcSourceClockSet( RtcClockSourceEnum eSourceClock )
{
    BOOL fSuccess = FALSE;

    fSuccess = RtcConfig( eSourceClock, RTC_CONFIG_OSC_PLL );

    if( fSuccess )
    {
        // set global source clock variable
        geCurrentSourceClock = eSourceClock;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcSourceClockGet( RtcClockSourceEnum *peSourceClock )
//!
//! \brief  returns the clock source of the RTC
//!
//! \param[in]  *peSourceClock   source clock variable
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RtcSourceClockGet( RtcClockSourceEnum *peSourceClock )
{
    if( peSourceClock != NULL )
    {
        *peSourceClock = geCurrentSourceClock;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcIsRtcConfigured( void )
//!
//! \brief  indicates if the RTC peripheralhas been configured
//!
//! \param[in]  void
//!
//! \return BOOL
//!             TRUE if RTC configured already
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcIsRtcConfigured( void )
{
    BOOL    fIsRtcConfigured    = FALSE;
    UINT32  dwValue             = 0;

    RtcGetBackupRegister( 0, &dwValue );

    if( dwValue == 0x01 )
    {
        fIsRtcConfigured = TRUE;
    }

    #warning find another way to indicate rtc is configured. check which register can be read
    //return fIsRtcConfigured;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcIsRtcConfigured( void )
//!
//! \brief  configures the RTC peripheral
//!
//! \param[in]  eSourceClock   source clock for rtc (LSI or LSE)
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcConfig( RtcClockSourceEnum eSourceClock, RtcConfigEnum eConfigType )
{
    BOOL    fSuccess        = FALSE;
    UINT32  dwWaitingTime   = 0;
    BOOL    fErrorFlag      = FALSE; // for debugging errors on waiting times.

    if( eSourceClock < RTC_SOURCE_CLOCK_MAX )
    {
        RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );           // Enable the PWR clock (Peripheral Clock)

        if( eConfigType == RTC_CONFIG_OSC_PLL )
        {
            // reset the whole Backup domain (RTCSEL,RTCEN,LSE LSI ENABLED)
            // Once the RTC clock source has been selected, it cannot be changed
            // anymore unless the Backup domain is reset.
            RCC_BackupResetCmd( ENABLE );
            RCC_BackupResetCmd( DISABLE );
            //after Reset, backup domain is write-protected, therefore remove the lock on BCKUP domain: PWR_BackupAccessCmd()
        }

        PWR_BackupAccessCmd( ENABLE );                                  // Allow access to RTC Domain

        if( eConfigType == RTC_CONFIG_OSC_PLL )
        {
            /////////////////////////////////////////////////
            // CONFIGURE CLOCK SOURCES
            /////////////////////////////////////////////////
            if( eSourceClock == RTC_SOURCE_CLOCK_LSE )
            {
                RCC_LSEConfig( RCC_LSE_ON );                                // Enable the LSE OSC

                dwWaitingTime = 100000;
                while( (RCC_GetFlagStatus( RCC_FLAG_LSERDY ) == RESET) && (dwWaitingTime--) );     // Wait till LSE is ready

                if( dwWaitingTime == 0 )
                {
                    // error, waiting time finished
                    fErrorFlag = TRUE;
                }

                RCC_RTCCLKConfig( RCC_RTCCLKSource_LSE );                   // Select the RTC Clock Source
            }
            else
            {
                RCC_LSICmd( ENABLE );                                       // Enable the LSI OSC

                dwWaitingTime = 100000;
                while( ( RCC_GetFlagStatus( RCC_FLAG_LSIRDY ) == RESET ) && (dwWaitingTime--) );     // Wait till LSI is ready

                if( dwWaitingTime == 0 )
                {
                    // error, waiting time finished
                    fErrorFlag = TRUE;
                }

                RCC_RTCCLKConfig( RCC_RTCCLKSource_LSI );
            }

            RCC_RTCCLKCmd( ENABLE );                                        // Enable the RTC Clock

            /////////////////////////////////////////////////
            /////////////////////////////////////////////////
        }

        dwWaitingTime = 1000;
        while( RTC_WaitForSynchro() == FALSE && (dwWaitingTime--) );        // Wait for RTC APB registers synchronisation

        if( dwWaitingTime == 0 )
        {
            // error, waiting time finished
            fErrorFlag = TRUE;
        }
        else
        {
            fSuccess = TRUE;
        }

        if( eConfigType == RTC_CONFIG_OSC_PLL )
        {
            if( fSuccess )
            {
                /////////////////////////////////////////////////
                // CONFIGURE RTC REGS AND PRESCALLER
                /////////////////////////////////////////////////
                RTC_InitTypeDef RTC_InitStructure;

                RTC_StructInit( &RTC_InitStructure );                           //RTC structure initialized to default values

                // Set the RTC time base to 1s
                RTC_InitStructure.RTC_AsynchPrediv  = 0x7F;                     // Specifies the RTC Asynchronous Predivider value.
                RTC_InitStructure.RTC_SynchPrediv   = 0xFF;                     // Specifies the RTC Synchronous Predivider value.
                RTC_InitStructure.RTC_HourFormat    = RTC_HourFormat_24;

                if( RTC_Init( &RTC_InitStructure ) != ERROR )                   // Check on RTC init
                {
                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcDateTimeStructInit( RtcDateTimeStruct * ptDateTime )
//!
//! \brief  Initialize RtcDateTimeStruct to default values
//!
//! \param[in]  ptDateTime      Rtc time structure
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RtcDateTimeStructInit( RtcDateTimeStruct * ptDateTime )
{
    if( ptDateTime != NULL )
    {
        ptDateTime->bSecond     = 0;
        ptDateTime->bMinute     = 0;
        ptDateTime->bHour       = 0;
        ptDateTime->eDayOfWeek  = RTC_DAY_MONDAY;
        ptDateTime->bDate       = 1;
        ptDateTime->eMonth      = RTC_MONTH_JANUARY;
        ptDateTime->bYear       = 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcIsDateTimeValid( RtcDateTimeStruct * ptDateTime )
//!
//! \brief  returns if the Date Time are valid
//!
//! \param[in]  ptDateTime   structure pointer that contains date and time
//!
//! \return BOOL
//!             TRUE if valid date time
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcIsDateTimeValid( RtcDateTimeStruct * ptDateTime )
{
    BOOL fAreParamsValid = FALSE;

    if( ptDateTime != NULL )
    {
        fAreParamsValid = TRUE;
        fAreParamsValid &= ( ( ptDateTime->bSecond >= 0 )   && ( ptDateTime->bSecond <= 59 ) );
        fAreParamsValid &= ( ( ptDateTime->bMinute >= 0 )   && ( ptDateTime->bMinute <= 59 ) );
        fAreParamsValid &= ( ( ptDateTime->bHour >= 0 )     && ( ptDateTime->bHour <= 23 ) );
        fAreParamsValid &= ( ( ptDateTime->bDate >= 1 )     && ( ptDateTime->bDate <= 31 ) );
        fAreParamsValid &= ( ( ptDateTime->bYear >= 0 )     && ( ptDateTime->bYear <= 99 ) );
        fAreParamsValid &= ( ( ptDateTime->eMonth >= RTC_MONTH_JANUARY )    && ( ptDateTime->eMonth <= RTC_MONTH_DECEMBER ) );
        fAreParamsValid &= ( ( ptDateTime->eDayOfWeek >= RTC_DAY_MONDAY )   && ( ptDateTime->eDayOfWeek <= RTC_DAY_SUNDAY ) );
    }

    return fAreParamsValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcDateTimeGet( RtcDateTimeStruct *ptDateTime )
//!
//! \brief  returns the Date Time of the RTC
//!
//! \param[in]  ptDateTime   structure pointer that contains date and time
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcDateTimeGet( RtcDateTimeStruct *ptDateTime )
{
    BOOL fSuccess = FALSE;

    if( ptDateTime != NULL )
    {
        if( ( RtcIsRtcConfigured() == TRUE ) && ( RTC_GetFlagStatus( RTC_FLAG_RSF ) == SET ) )
        {
            RTC_TimeTypeDef RTC_TimeStructure;
            RTC_DateTypeDef RTC_DateStructure;

            RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );          // Get the current Time and lock the Date
            RTC_GetDate( RTC_Format_BIN, &RTC_DateStructure );          // Get the current Date

            RtcDateTimeStructInit( ptDateTime );

            ptDateTime->bYear       = RTC_DateStructure.RTC_Year;
            ptDateTime->eMonth      = RTC_DateStructure.RTC_Month;
            ptDateTime->bDate       = RTC_DateStructure.RTC_Date;
            ptDateTime->eDayOfWeek  = RTC_DateStructure.RTC_WeekDay;
            ptDateTime->bHour       = RTC_TimeStructure.RTC_Hours;
            ptDateTime->bMinute     = RTC_TimeStructure.RTC_Minutes;
            ptDateTime->bSecond     = RTC_TimeStructure.RTC_Seconds;

            fSuccess = RtcIsDateTimeValid( ptDateTime );
        }
    }

    if( fSuccess != TRUE )
    {
        // Time invalid. Set safe values.
        RtcDateTimeStructInit( ptDateTime );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcDateTimeSet( RtcDateTimeStruct *ptDateTime )
//!
//! \brief  sets the Date Time of the RTC
//!
//! \param[in]  ptDateTime   structure pointer that contains date and time
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcDateTimeSet( RtcDateTimeStruct *ptDateTime )
{
    BOOL fSuccess = FALSE;

    if ( ptDateTime != NULL )
    {
        RTC_TimeTypeDef RTC_TimeStructure;
        RTC_DateTypeDef RTC_DateStructure;

        if( RtcIsDateTimeValid( ptDateTime ) == TRUE )
        {
            // Populate RTC original structure
            RTC_TimeStructure.RTC_Seconds   = ptDateTime->bSecond;
            RTC_TimeStructure.RTC_Minutes   = ptDateTime->bMinute;
            RTC_TimeStructure.RTC_Hours     = ptDateTime->bHour;
            RTC_DateStructure.RTC_WeekDay   = ptDateTime->eDayOfWeek;
            RTC_DateStructure.RTC_Date      = ptDateTime->bDate;
            RTC_DateStructure.RTC_Month     = ptDateTime->eMonth;
            RTC_DateStructure.RTC_Year      = ptDateTime->bYear;

            // Allow access to RTC Domain (RTC registers)
            PWR_BackupAccessCmd( ENABLE );

            fSuccess = RTC_SetTime( RTC_Format_BIN, &RTC_TimeStructure );
            fSuccess &= RTC_SetDate( RTC_Format_BIN, &RTC_DateStructure );
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     UINT32 RtcDateTimeToEpochSeconds( RtcDateTimeStruct *ptDateTime )
//!
//! \brief  Convert a given date time struct to seconds
//!
//! \param[in]  ptDataRecord      Pointer to the structure date time
//!
//! \return UINT32, Number of seconds since Jan. 1st 1970
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 RtcDateTimeToEpochSeconds( RtcDateTimeStruct *ptDateTime )
{
    UINT32          dwSeconds       = 0;
    struct tm       tTimeStructure;
    time_t          tTimeSeconds;

    if( ptDateTime != NULL )
    {
        tTimeStructure.tm_sec       = ptDateTime->bSecond;
        tTimeStructure.tm_min       = ptDateTime->bMinute;
        tTimeStructure.tm_hour      = ptDateTime->bHour;
        tTimeStructure.tm_mday      = ptDateTime->bDate;                // Day of Month is 1-based
        tTimeStructure.tm_mon       = ptDateTime->eMonth - 1;           // Month must be 0-based
        tTimeStructure.tm_year      = ((UINT16) ptDateTime->bYear) + (RTC_CENTURY_OFFSET - 1900); // tm_year must be years since 1900
        tTimeStructure.tm_wday      = ptDateTime->eDayOfWeek;
        tTimeStructure.tm_yday      = 0;                                // mktime() function doesn't require this parameter to be set
        tTimeStructure.tm_isdst     = 0;                                // mktime() function doesn't require this parameter to be set

        // convert date time to EPOCH seconds
        tTimeSeconds                = mktime( &tTimeStructure );

        if( tTimeSeconds == -1 )
        {
            dwSeconds = 0;
        }
        else
        {
            dwSeconds = tTimeSeconds;
        }
    }

    return dwSeconds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcEpochSecondsToDateTime( UINT32 dwEpochSecs, RtcDateTimeStruct *ptDateTime )
//!
//! \brief  Convert a given Epoch seconds into a date time struct
//!
//! \param[in]  dwEpochSecs       Epoch seconds
//!
//! \param[in]  ptDataRecord      Pointer to the structure date time
//!
//! \return UINT32, Number of seconds since Jan. 1st 1970
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcEpochSecondsToDateTime( UINT32 dwEpochSecs, RtcDateTimeStruct *ptDateTime )
{
    BOOL        fSuccess    = FALSE;
    time_t      tTimeSeconds= 0;
    struct tm   tTimeStruct;

    // set time in seconds
    tTimeSeconds = dwEpochSecs;

    // convert to time structure
    tTimeStruct =* localtime( &tTimeSeconds );

    ptDateTime->bYear       = tTimeStruct.tm_year - (RTC_CENTURY_OFFSET-1900); // tm_year must be years since 1900
    ptDateTime->eMonth      = tTimeStruct.tm_mon + 1;
    ptDateTime->bDate       = tTimeStruct.tm_mday;
    ptDateTime->eDayOfWeek  = tTimeStruct.tm_wday;
    ptDateTime->bHour       = tTimeStruct.tm_hour;
    ptDateTime->bMinute     = tTimeStruct.tm_min;
    ptDateTime->bSecond     = tTimeStruct.tm_sec;

    fSuccess = RtcIsDateTimeValid( ptDateTime );

    return fSuccess;
}

/* //////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcInterruptPinA0Enable( BOOL fInputPinEnable )
//!
//! \brief  Enables/ Disable the pin interruption PA0
//!
//! \param[in]  fInputPinEnable   TRUE=ENABLE / FALSE=DISABLE
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RtcInterruptPinA0Enable( BOOL fInputPinEnable )
{
    GPIO_InitTypeDef    GPIO_InitStructure;
    EXTI_InitTypeDef    EXTI_InitStructure;
    NVIC_InitTypeDef    NVIC_InitStructure;
    /////////////////////////////////////////////////
    // CONFIGURE GPIO
    /////////////////////////////////////////////////
    // Initialized to defualt values
    GPIO_StructInit( &GPIO_InitStructure );
    // Enable clock
    RCC_AHB1PeriphClockCmd( RTC_INT_WAKEUP_REED_WITCH_1_RCC, ENABLE );

    // #### SET PIN MODE ####
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Pin     = RTC_INT_WAKEUP_REED_WITCH_1_PIN;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;   // dont care for input
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init( RTC_INT_WAKEUP_REED_WITCH_1_PORT, &GPIO_InitStructure );

    /////////////////////////////////////////////////
    // CONFIGURE GPIO EXTI
    /////////////////////////////////////////////////
    // Connect Button EXTI Line to GPIO Pin
    //SYSCFG_EXTILineConfig( RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_PORT, GPIO_PinSource0 );
    SYSCFG_EXTILineConfig( RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_PORT, RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_PIN );
    // Configure Button EXTI line
    RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_RCC_INIT();

    EXTI_InitStructure.EXTI_Line    = RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /////////////////////////////////////////////////
    // CONFIGURE NVIC INTERRUPT VECTOR
    /////////////////////////////////////////////////
    // Enable the RTC Alarm Interrupt
    NVIC_InitStructure.NVIC_IRQChannel                      = RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = RTC_INTERRUPT_PREEMPTION_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority           = RTC_INTERRUPT_PA0_SUB_PRIORITY;
    if( fInputPinEnable == TRUE )
    {
        NVIC_InitStructure.NVIC_IRQChannelCmd   = ENABLE;

        gfIsInterruptPA0Enabled                 = TRUE;
    }
    else
    {
        NVIC_InitStructure.NVIC_IRQChannelCmd   = DISABLE;

        gfIsInterruptPA0Enabled                 = FALSE;
    }

    NVIC_Init( &NVIC_InitStructure );
} */

#if(0)
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcInterruptOnTimeMatch( RtcInterruptOnTimeMatchEnum eTimeMatchType, UINT8 bMatchValue )
//!
//! \brief  Enables/ Disable the time match interruption
//!
//! \param[in]  eTimeMatchType   type of time match from RtcInterruptWkupOnTimeMatchEnum;
//!
//! \param[in]  bMatchValue      time when interruption will occur. At XX:XX:ValSecond or XX:ValMinute:XX or ValHour:XX:XX
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcAlarmTimeMatchSetTime( RtcAlarmTypeEnum eAlarmType, BOOL fIsAlarmEnabled, RtcAlarmTimeMatchType *ptAlarmConfig )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( RtcAlarmTypeEnum < RTC_ALARM_TYPE_MAX ) &&
        ( NULL != ptAlarmConfig )
    )
    {
        EXTI_InitTypeDef    EXTI_InitStructure;
        NVIC_InitTypeDef    NVIC_InitStructure;
        RTC_AlarmTypeDef    RTC_AlarmStructure;

        // Config Commons
        NVIC_InitStructure.NVIC_IRQChannel                      = RTC_Alarm_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = RTC_INTERRUPT_PREEMPTION_PRIORITY;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority           = RTC_INTERRUPT_TIME_MATCH_SUB_PRIORITY;

        RTC_AlarmStructInit( &RTC_AlarmStructure );

        if( fIsAlarmEnabled )
        {
            if( eTimeMatchType != RTC_INT_WKUP_ON_TIME_MATCH_NONE )
            {
                RTC_AlarmCmd( RTC_Alarm_A, DISABLE );   // Disable the Alarm A
                RTC_ITConfig( RTC_IT_ALRA, DISABLE );   // Enable the RTC Wakeup Interrupt
                RTC_ClearITPendingBit( RTC_IT_ALRA );


            }
        }
        else
        {

        }
    }

    return fSuccess;
}

BOOL RtcInterruptOnTimeMatch( RtcInterruptOnTimeMatchEnum eTimeMatchType, UINT8 bMatchValue )
{
    BOOL fSuccess = FALSE;

    if( eTimeMatchType < RTC_ON_TIME_MATCH_MAX )
    {
        EXTI_InitTypeDef    EXTI_InitStructure;
        NVIC_InitTypeDef    NVIC_InitStructure;
        RTC_AlarmTypeDef    RTC_AlarmStructure;

        // Config Commons
        NVIC_InitStructure.NVIC_IRQChannel                      = RTC_Alarm_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = RTC_INTERRUPT_PREEMPTION_PRIORITY;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority           = RTC_INTERRUPT_TIME_MATCH_SUB_PRIORITY;

        if( eTimeMatchType != RTC_INT_WKUP_ON_TIME_MATCH_NONE )
        {
            /////////////////////////////////////////////////
            // CONFIGURE ALARM CLOCK
            /////////////////////////////////////////////////
            RTC_AlarmCmd( RTC_Alarm_A, DISABLE );   // Disable the Alarm A
            RTC_ITConfig( RTC_IT_ALRA, DISABLE );   // Enable the RTC Wakeup Interrupt
            RTC_ClearITPendingBit( RTC_IT_ALRA );

            /////////////////////////////////////////////////
            // CALCULATE ALARM TIMES
            /////////////////////////////////////////////////
            RTC_AlarmStructInit( &RTC_AlarmStructure );

            switch( eTimeMatchType )
            {
                case RTC_INT_WKUP_ON_TIME_MATCH_SECOND:
                    // sanity check
                    if( bMatchValue < 60 )
                    {
                        RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds    = bMatchValue;
                        // clear the mask bit that wants to be used to trigger interruption
                        RTC_AlarmStructure.RTC_AlarmMask                = (RTC_AlarmMask_All & (~RTC_AlarmMask_Seconds));
                        fSuccess = TRUE;
                    }
                    break;
                case RTC_INT_WKUP_ON_TIME_MATCH_MINUTE:
                    if( bMatchValue < 60 )
                    {
                        RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes    = bMatchValue;
                        // clear the mask bit that wants to be used to trigger interruption
                        RTC_AlarmStructure.RTC_AlarmMask                = (RTC_AlarmMask_All & (~RTC_AlarmMask_Minutes));
                        fSuccess = TRUE;
                    }
                    break;
                case RTC_INT_WKUP_ON_TIME_MATCH_HOUR:
                    if( bMatchValue < 24 )
                    {
                        RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours      = bMatchValue;
                        // clear the mask bit that wants to be used to trigger interruption
                        RTC_AlarmStructure.RTC_AlarmMask                = (RTC_AlarmMask_All & (~RTC_AlarmMask_Hours));
                        fSuccess = TRUE;
                    }
                    break;
                default:
                    // Error:option not recognized
                    break;
            }

            if( fSuccess == TRUE )
            {
                // Configure the RTC Alarm A register
                RTC_SetAlarm( RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure );

                RTC_ITConfig( RTC_IT_ALRA, ENABLE );                // Enable the RTC Wakeup Interrupt
                fSuccess = RTC_AlarmCmd( RTC_Alarm_A, ENABLE );     // Enable the alarm  A

                if( fSuccess == TRUE )
                {
                    /////////////////////////////////////////////////
                    // CONFIGURE WKUP EXTI LINE 17 (from manual)
                    /////////////////////////////////////////////////
                    // Configure EXTI line
                    EXTI_ClearITPendingBit( EXTI_Line17 );
                    EXTI_InitStructure.EXTI_Line    = EXTI_Line17;      // Line 17 connected to RTC ALARMA event
                    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
                    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
                    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                    EXTI_Init( &EXTI_InitStructure );

                    /////////////////////////////////////////////////
                    // CONFIGURE NVIC INTERRUPT VECTOR
                    /////////////////////////////////////////////////
                    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
                    NVIC_Init( &NVIC_InitStructure );

                    // Indicate this interrupt has been enabled
                    gfIsInterruptTimeMatchEnabled = TRUE;
                }
            }
        }
        else
        {
            /////////////////////////////////////////////////
            // CONFIGURE NVIC INTERRUPT VECTOR
            /////////////////////////////////////////////////
            NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
            NVIC_Init( &NVIC_InitStructure );

            // Indicate this interrupt has been disabled
            gfIsInterruptTimeMatchEnabled = FALSE;

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcGetInterruptCounter( UINT32 *pdwInterruptPinA0Counter, UINT32 *pdwInterruptOnTimeMatchCounter )
//!
//! \brief  Return the value of the amount of interruptions
//!
//! \param[in]  pdwInterruptPinA0Counter        interruption counter for event: PA0 on rising
//!
//! \param[in]  pdwInterruptOnTimeMatchCounter  interruption counter for event: Time Match (hours/minutes/seconds)
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcGetInterruptCounter( UINT32 *pdwInterruptPinA0Counter, UINT32 *pdwInterruptOnTimeMatchCounter )
{
    BOOL fSuccess = FALSE;

    if( pdwInterruptOnTimeMatchCounter != NULL )
    {
        *pdwInterruptOnTimeMatchCounter     = gdwInterruptWKUPRolloverCounter;

        fSuccess = TRUE;
    }


    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_IRQ_HNDLR( void )
//!
//! \brief  interruption handler
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_IRQ_HNDLR( void )
{
    if( EXTI_GetITStatus( RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_LINE ) == SET )
    {
        // Clear the Wakeup Button EXTI line pending bit
        EXTI_ClearITPendingBit( RTC_INT_WAKEUP_REED_WITCH_1_EXT_INT_LINE );

        gdwInterruptPinA0Counter++;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RTC_Alarm_IRQHandler( void )
//!
//! \brief  interruption handler
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RTC_Alarm_IRQHandler( void )
{
    if( RTC_GetITStatus( RTC_IT_ALRA ) == SET )
    {
        // Clear the WKUP pending bit
        RTC_ClearITPendingBit( RTC_IT_ALRA );
        // Clear the Line 17 pending bit (Line 17 connected to RTC ALARMA event)
        EXTI_ClearITPendingBit( EXTI_Line17 );

        gdwInterruptWKUPRolloverCounter++;
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcSetBackupRegister( UINT8 bBkpRegIndex, UINT32 dwValue )
//!
//! \brief  write a uint32 into a RTC backup register
//!
//! \param[in]  bBkpRegIndex        index:0...19
//!
//! \param[in]  dwValue             val max size 4 bytes
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcSetBackupRegister( UINT8 bBkpRegIndex, UINT32 dwValue )
{
    BOOL fSuccess = FALSE;

    if( bBkpRegIndex < RTC_BACKUP_DATA_REGS_AMOUNT )
    {
        RTC_WriteBackupRegister( dwBakupDataRegisters[bBkpRegIndex], dwValue );

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL RtcGetBackupRegister( UINT8 bBkpRegIndex, UINT32 *pdwValue )
//!
//! \brief  read a uint32 RTC backup register
//!
//! \param[in]  bBkpRegIndex        index:0...19
//!
//! \param[in]  dwValue             val max size 4 bytes
//!
//! \return BOOL
//!             TRUE if no error on the operation.
//!             FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RtcGetBackupRegister( UINT8 bBkpRegIndex, UINT32 *pdwValue )
{
    BOOL fSuccess = FALSE;

    if( bBkpRegIndex < RTC_BACKUP_DATA_REGS_AMOUNT )
    {
        if( pdwValue != NULL )
        {
            *pdwValue = RTC_ReadBackupRegister( dwBakupDataRegisters[bBkpRegIndex] );

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcPrintClockStatus( ConsolePortEnum eConsole )
//!
//! \brief  prints the status of Rtc clock
//!
//! \param[in]  eConsole   Console Id to print message
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void RtcPrintClockStatus( ConsolePortEnum eConsole )
//{
//    RtcDateTimeStruct   tDateTime;
//    UINT32              dwEpoch  = 0;
//    BOOL                fSuccess = FALSE;
//
//    fSuccess = RtcIsRtcConfigured();
//    RtcDateTimeGet( &tDateTime );
//    dwEpoch = RtcDateTimeToEpochSeconds( &tDateTime );
//
//    ConsolePrintf( eConsole, "----------------------\r\n" );
//    ConsolePrintf( eConsole, "RTC DATE TIME\r\n" );
//    ConsolePrintf( eConsole, "IsRtcConfigured:\t%d\r\n", fSuccess );
//    ConsolePrintf(
//        eConsole, "DateTime:\t\t%04d-%02d-%02d %02d:%02d:%02d\r\n",
//        ( tDateTime.bYear + RTC_CENTURY ),
//        tDateTime.eMonth,
//        tDateTime.bDate,
//        tDateTime.bHour,
//        tDateTime.bMinute,
//        tDateTime.bSecond );
//    ConsolePrintf( eConsole, "EpochTime:\t\t%d\r\n", dwEpoch );
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcPrintInterruptStatus( ConsolePortEnum eConsole )
//!
//! \brief  prints the status of Rtc interruptions
//!
//! \param[in]  eConsole   Console Id to print message
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void RtcPrintInterruptStatus( ConsolePortEnum eConsole )
//{
//    UINT32 dwInterruptPinA0Counter      = 0;
//    UINT32 dwInterruptOnTimeMatchCounter= 0;
//
//    RtcGetInterruptCounter( &dwInterruptPinA0Counter, &dwInterruptOnTimeMatchCounter );
//
//    ConsolePrintf( eConsole, "----------------------\r\n" );
//    ConsolePrintf( eConsole, "RTC INTERRUPTIONS\r\n" );
//    ConsolePrintf( eConsole, "IsInterruptTimeMatchEnabled:\t\t%d\r\n", gfIsInterruptTimeMatchEnabled );
//    ConsolePrintf( eConsole, "\tInterruptTimeMatchCounterVal:\t%d\r\n", dwInterruptOnTimeMatchCounter );
//    ConsolePrintf( eConsole, "IsInterruptPA0Enabled:\t\t\t%d\r\n", gfIsInterruptPA0Enabled );
//    ConsolePrintf( eConsole, "\tIsInterruptPA0CounterVal:\t%d\r\n", dwInterruptPinA0Counter );
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcPrintBckupStatus( ConsolePortEnum eConsole )
//!
//! \brief  prints the status of Backup registers
//!
//! \param[in]  eConsole   Console Id to print message
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void RtcPrintBckupStatus( ConsolePortEnum eConsole )
//{
//    UINT32 dwRegValue = 0;
//
//    ConsolePrintf( eConsole, "----------------------\r\n" );
//    ConsolePrintf( eConsole, "RTC BACKUP REGS\r\n" );
//
//    for( UINT8 bRegCounter = 0; bRegCounter < RTC_BACKUP_DATA_REGS_AMOUNT ; bRegCounter++ )
//    {
//        RtcGetBackupRegister( bRegCounter, &dwRegValue );
//
//        ConsolePrintf( eConsole, "[%02d:0x%02X]", bRegCounter, dwRegValue );
//
//        if( bRegCounter%5 == 4 )
//        {
//            ConsolePrintf( eConsole, "\r\n" );
//        }
//    }
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void RtcPrintDateTime( ConsolePortEnum eConsole )
//!
//! \brief  Print the RTC Date Time
//!
//! \param[in]  eConsole   Console Id to print message
//!
//! \return void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void RtcPrintDateTime( ConsolePortEnum eConsole )
//{
//    RtcDateTimeStruct       tDateTime;
//    CHAR                    cTimestampBuffer[32];
//
//    RtcDateTimeGet(&tDateTime);
//
//    snprintf( &cTimestampBuffer[0], sizeof(cTimestampBuffer), "%04u-%02u-%02u,%02u:%02u:%02u",
//                ( tDateTime.bYear + RTC_CENTURY ),
//                tDateTime.eMonth,
//                tDateTime.bDate,
//                tDateTime.bHour,
//                tDateTime.bMinute,
//                tDateTime.bSecond
//                );
//
//    ConsolePrintf(eConsole, &cTimestampBuffer[0]);
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         void RtcTest( void )
//!
//! \brief      Test function for Debugging purposes.
//!
//! \return     void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void RtcTest( void )
{
    // #### TEST CODE ####
    BOOL                fSuccess                        = FALSE;
    BOOL                fReturn                         = FALSE;
    UINT32              dwInterruptPinA0Counter         = 0;
    UINT32              dwInterruptWKUPRolloverCounter  = 0;
    UINT32              dwDelay                         = 1;
    RtcDateTimeStruct   tDateTime;

    fSuccess = RtcInitialize( RTC_SOURCE_CLOCK_LSI );

    if( fSuccess )
    {
        fSuccess &= RtcIsRtcConfigured();

        if( fSuccess )
        {
            tDateTime.bSecond       = 0;
            tDateTime.bMinute       = 30;
            tDateTime.bHour         = 17;
            tDateTime.eDayOfWeek    = RTC_DAY_WEDNESDAY;
            tDateTime.bDate         = 1;
            tDateTime.eMonth        = RTC_MONTH_MAY;
            tDateTime.bYear         = 13;
            //////////////////////////////////////
            // test the epoch functions are working
            //////////////////////////////////////
            UINT32 dwEpoch = RtcDateTimeToEpochSeconds( &tDateTime );
            RtcDateTimeStructInit( &tDateTime );
            RtcEpochSecondsToDateTime( dwEpoch, &tDateTime );
            //////////////////////////////////////

            fSuccess = RtcDateTimeSet( &tDateTime );

            RtcDateTimeStructInit( &tDateTime );
            //get time
            fSuccess = RtcDateTimeGet( &tDateTime );
            //print time

            //////////////////////////////////////
            // test PA0 interruptions
            //////////////////////////////////////
            //#define TEST_INT_PA0
            #if defined(TEST_INT_PA0)
            RtcInterruptPinA0Enable( TRUE );

            dwDelay = 1;
            while( dwDelay );           // use debugger break point to set delay val to zero

            RtcGetInterruptCounter( &dwInterruptPinA0Counter, &dwInterruptWKUPRolloverCounter );

            RtcInterruptPinA0Enable( FALSE );

            dwDelay = 1;
            while( dwDelay );           // use debugger break point to set delay val to zero

            RtcGetInterruptCounter( &dwInterruptPinA0Counter, &dwInterruptWKUPRolloverCounter );
            #endif
            //////////////////////////////////////

            //////////////////////////////////////
            // test TimeRollover interruptions
            //////////////////////////////////////
            //#define TEST_INT_ROLLOVER
            #if defined(TEST_INT_ROLLOVER)
            UINT8 bTimeMatchVal = 1;
            RtcInterruptWakeupOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_SECOND, bTimeMatchVal );

            dwDelay = 1;
            while( dwDelay );           // use debugger break point to set delay val to zero

            RtcInterruptWakeupOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );
            dwDelay = 1;
            while( dwDelay );           // use debugger break point to set delay val to zero
            #endif
            //////////////////////////////////////


            // Test RTC backup regs
            UINT32 dwRegVal = 0;
            RtcGetBackupRegister( 0 , &dwRegVal );
            RtcSetBackupRegister( 0 , 0x1234 );
            RtcGetBackupRegister( 0 , &dwRegVal );
            RtcGetBackupRegister( 0 , &dwRegVal );
        }
        else
        {
            //drop error
        }
    }
    else
    {
        // Error. Rtc not initialized
    }

    while(1);
    // #### --------- ####
}
