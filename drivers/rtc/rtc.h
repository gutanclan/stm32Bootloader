//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Rtc.h
//!    \brief       RTC module header.
//!
//!    \author      Puracom Inc.
//!    \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _RTC_H_
#define _RTC_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	RTC_SOURCE_CLOCK_UNKNOWN	= 0,
    RTC_SOURCE_CLOCK_LSE		= 1,	//LOW SPEED INTERNAL (32.000 KHz)
	RTC_SOURCE_CLOCK_LSI		= 2,	//LOW SPEED EXTERNAL (32.768 KHz)

    RTC_SOURCE_CLOCK_MAX
}RtcClockSourceEnum;

#define RTC_CENTURY_OFFSET 			(2000)

typedef enum
{
    RTC_DAY_INVALID = 0,
    RTC_DAY_MONDAY,
    RTC_DAY_TUESDAY,
    RTC_DAY_WEDNESDAY,
    RTC_DAY_THURSDAY,
    RTC_DAY_FRIDAY,
    RTC_DAY_SATURDAY,
    RTC_DAY_SUNDAY,

    RTC_DAY_MAX,
}RtcWeekDayEnum;

typedef enum
{
    RTC_MONTH_INVALID = 0,
    RTC_MONTH_JANUARY,
    RTC_MONTH_FEBRUARY,
    RTC_MONTH_MARCH,
    RTC_MONTH_APRIL,
    RTC_MONTH_MAY,
    RTC_MONTH_JUNE,
    RTC_MONTH_JULY,
    RTC_MONTH_AUGUST,
    RTC_MONTH_SEPTEMBER,
    RTC_MONTH_OCTOBER,
    RTC_MONTH_NOVEMBER,
    RTC_MONTH_DECEMBER,

    RTC_MONTH_MAX,
}RtcMonthEnum;

typedef struct
{
    // Time
    UINT8 			bSecond;        // 0..59, Second
	UINT8 			bMinute;        // 0..59, Minute
	UINT8 			bHour;          // 0..23, Hour in 24-hour format
	// Date
	RtcWeekDayEnum  eDayOfWeek;     // 1..7
    UINT8           bDate;          // 1..31, Day of Month
	RtcMonthEnum 	eMonth;         // 1..12
	UINT8 			bYear;          // 00..99
} RtcDateTimeStruct;

typedef enum
{
	RTC_ALARM_TYPE_A	= 0,
    RTC_ALARM_TYPE_B	= 1,

    RTC_ALARM_TYPE_MAX
}RtcAlarmTypeEnum;

typedef struct
{
	BOOL fIsHourMatchEnabled;
	BOOL fIsMinMatchEnabled;
	BOOL fIsSecMatchEnabled;
	UINT8 bHourMatchVal;
	UINT8 bMinMatchVal;
    UINT8 bSecMatchVal;
}RtcAlarmTimeMatchType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL 	RtcInitialize                   ( RtcClockSourceEnum eSourceClock );
BOOL    RtcSourceClockSet               ( RtcClockSourceEnum eSourceClock );
void    RtcSourceClockGet               ( RtcClockSourceEnum *peSourceClock );
BOOL    RtcIsRtcConfigured              ( void );

void    RtcDateTimeStructInit           ( RtcDateTimeStruct *ptDateTime );
BOOL    RtcIsDateTimeValid              ( RtcDateTimeStruct *ptDateTime );
BOOL 	RtcDateTimeGet					( RtcDateTimeStruct *ptDateTime );
BOOL 	RtcDateTimeSet					( RtcDateTimeStruct *ptDateTime );

UINT32  RtcDateTimeToEpochSeconds       ( RtcDateTimeStruct *ptDateTime );
BOOL    RtcEpochSecondsToDateTime       ( UINT32 dwEpochSecs, RtcDateTimeStruct *ptDateTime );

// not used for now since time stamp functionality is more important than alarms for now.
#if(0)
BOOL    RtcAlarmTimeMatchSetTime        ( RtcAlarmTypeEnum eAlarmType, BOOL fIsAlarmEnabled, RtcAlarmTimeMatchType *ptAlarmConfig );
UINT32  RtcAlarmTimeMatchGetEventCounter( RtcAlarmTypeEnum eAlarmType );
#endif

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _RTC_H_

