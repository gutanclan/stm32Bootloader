/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../Utils/types.h"

#include "Rtc.h"

#include "SysTime.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

static FLOAT gsgTimeZoneOffsetSeconds = 0.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL SysTimeIsDateValid( SysTimeDateStruct *ptDate );
static BOOL SysTimeIsTimeValid( SysTimeTimeStruct *ptTime );

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeInit( void )
{
	RtcInitialize( RTC_SOURCE_CLOCK_LSI );
	return RtcIsRtcConfigured();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeSetUtcTimeZoneOffset( FLOAT sgTimeZoneOffset )
{
	BOOL    fSuccess        = FALSE;
    BOOL    fIsNegative     = FALSE;
    UINT8   bDecimalHours   = 0;
    FLOAT   sgFractional    = 0.0;
    FLOAT   sgTimeZoneOffsetTemp;

    sgTimeZoneOffsetTemp = sgTimeZoneOffset;

    if( sgTimeZoneOffsetTemp < 0 )
    {
        fIsNegative = TRUE;
        sgTimeZoneOffsetTemp = sgTimeZoneOffsetTemp * (-1.0);
    }

    bDecimalHours   = (UINT8)sgTimeZoneOffsetTemp;

    sgFractional= sgTimeZoneOffsetTemp - bDecimalHours;

    // convert fractional part of the hour to minutes
    sgFractional= sgFractional * 60.0;

    // 15 = 60/4 one quarter of the hour
    if( ((UINT8)sgFractional % 15) == 0 )
    {
        // max number of offset hours starting from offset 0
        // (time zones max mins)
        if( bDecimalHours < 15 )
        {
            gsgTimeZoneOffsetSeconds = (bDecimalHours * 60.0 * 60.0);

            gsgTimeZoneOffsetSeconds = gsgTimeZoneOffsetSeconds + (sgFractional * 60.0);

            if( fIsNegative )
            {
                gsgTimeZoneOffsetSeconds = gsgTimeZoneOffsetSeconds * (-1.0);
            }

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT SysTimeGetUtcTimeZoneOffset( void )
{
	BOOL    fIsNegative     = FALSE;
    FLOAT  sgTimeZoneOffsetTemp;

    sgTimeZoneOffsetTemp = gsgTimeZoneOffsetSeconds;

    if( sgTimeZoneOffsetTemp < 0 )
    {
        fIsNegative = TRUE;

        sgTimeZoneOffsetTemp = sgTimeZoneOffsetTemp * (-1.0);
    }

    // to mins
    sgTimeZoneOffsetTemp = sgTimeZoneOffsetTemp / 60;
    // to hours
    sgTimeZoneOffsetTemp = sgTimeZoneOffsetTemp / 60;

    if( fIsNegative )
    {
        sgTimeZoneOffsetTemp = sgTimeZoneOffsetTemp * -1.0;
    }

    return sgTimeZoneOffsetTemp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!lknfgklbgd
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeSetDateTimeUtcZero( SysTimeDateTimeStruct *ptDateTimeUtcZero )
{
	BOOL fSuccess = FALSE;

    if ( ptDateTimeUtcZero != NULL )
    {
		RtcDateTimeStruct   tDateTime;

        RtcDateTimeStructInit( &tDateTime );

		tDateTime.bSecond       = ptDateTimeUtcZero->tTime.bSecond;
		tDateTime.bMinute       = ptDateTimeUtcZero->tTime.bMinute;
		tDateTime.bHour         = ptDateTimeUtcZero->tTime.bHour;

		tDateTime.bDate         = ptDateTimeUtcZero->tDate.bDate;
		tDateTime.eMonth        = ptDateTimeUtcZero->tDate.bMonth;
		tDateTime.bYear         = ptDateTimeUtcZero->tDate.wYear - RTC_CENTURY_OFFSET;

		fSuccess = RtcDateTimeSet( &tDateTime );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeGetDateTime( SysTimeDateTimeStruct *ptDateTimeUtc, BOOL fApplyTimeZoneOffset )
{
	BOOL fSuccess = FALSE;
    UINT32 dwEpochSeconds;
    UINT32 dwEpochSecondsOffset;

    if( ptDateTimeUtc != NULL )
    {
        if( RtcIsRtcConfigured() == TRUE )
        {
			RtcDateTimeStruct   tDateTime;

			if( RtcDateTimeGet( &tDateTime ) )
			{
				ptDateTimeUtc->tTime.bSecond= tDateTime.bSecond;
				ptDateTimeUtc->tTime.bMinute= tDateTime.bMinute;
				ptDateTimeUtc->tTime.bHour 	= tDateTime.bHour;

				ptDateTimeUtc->tDate.wYear 	= tDateTime.bYear + RTC_CENTURY_OFFSET;
				ptDateTimeUtc->tDate.bMonth = tDateTime.eMonth;
				ptDateTimeUtc->tDate.bDate 	= tDateTime.bDate;

				if( fApplyTimeZoneOffset )
				{
					dwEpochSeconds = SysTimeDateTimeToEpochSeconds( ptDateTimeUtc );

					if( gsgTimeZoneOffsetSeconds < 0 )
					{
						dwEpochSecondsOffset = (UINT32)( gsgTimeZoneOffsetSeconds * (-1.0) );
						dwEpochSeconds = dwEpochSeconds - dwEpochSecondsOffset;
					}
					else
					{
						dwEpochSecondsOffset = (UINT32)( gsgTimeZoneOffsetSeconds );
						dwEpochSeconds = dwEpochSeconds + dwEpochSecondsOffset;
					}

					SysTimeEpochSecondsToDateTime( dwEpochSeconds, ptDateTimeUtc );
				}

				if
				(
					( SysTimeIsDateValid( &ptDateTimeUtc->tDate ) ) &&
					( SysTimeIsTimeValid( &ptDateTimeUtc->tTime ) )
				)
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
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeIsDateValid( SysTimeDateStruct *ptDate )
{
	BOOL fIsValid = FALSE;

	if( ptDate != NULL )
	{
		fIsValid = TRUE;
		fIsValid&= ((ptDate->wYear >= 0) && (ptDate->wYear <= 9999));
		fIsValid&= ((ptDate->bMonth >= 1) && (ptDate->bMonth <= 12));
		fIsValid&= ((ptDate->bDate >= 1) && (ptDate->bDate <= 31));
	}

	return fIsValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeIsTimeValid( SysTimeTimeStruct *ptTime )
{
	BOOL fIsValid = FALSE;

	if( ptTime != NULL )
	{
		fIsValid = TRUE;
		fIsValid&= ((ptTime->bHour >= 0) && (ptTime->bHour <= 23));
		fIsValid&= ((ptTime->bMinute >= 0) && (ptTime->bMinute <= 59));
		fIsValid&= ((ptTime->bSecond >= 0) && (ptTime->bSecond <= 59));
	}

	return fIsValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 SysTimeDateTimeToEpochSeconds( SysTimeDateTimeStruct *ptDateTime )
{
	UINT32          dwSeconds       = 0;
    struct tm       tTimeStructure;
    time_t          tTimeSeconds;

    if( ptDateTime != NULL )
    {
		if
		(
			( SysTimeIsDateValid( &ptDateTime->tDate ) ) &&
			( SysTimeIsTimeValid( &ptDateTime->tTime ) )
		)
		{
			/*
			struct tm
			int    tm_sec   seconds [0,61]
			int    tm_min   minutes [0,59]
			int    tm_hour  hour [0,23]
			int    tm_mday  day of month [1,31]
			int    tm_mon   month of year [0,11]
			int    tm_year  years since 1900
			int    tm_wday  day of week [0,6] (Sunday = 0)
			int    tm_yday  day of year [0,365]
			int    tm_isdst daylight savings flag
			*/

			tTimeStructure.tm_sec       = ptDateTime->tTime.bSecond;
			tTimeStructure.tm_min       = ptDateTime->tTime.bMinute;
			tTimeStructure.tm_hour      = ptDateTime->tTime.bHour;

			tTimeStructure.tm_mday      = ptDateTime->tDate.bDate;         // Day of Month is 1-based
			tTimeStructure.tm_mon       = ptDateTime->tDate.bMonth - 1;    // Month must be 0-based
			tTimeStructure.tm_year      =(ptDateTime->tDate.wYear - 1900); // tm_year must be years since 1900

			tTimeStructure.tm_wday      = 0;
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
    }

    return dwSeconds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SysTimeEpochSecondsToDateTime( UINT32 dwEpochSecs, SysTimeDateTimeStruct *ptDateTime )
{
	BOOL        fSuccess    = FALSE;
    time_t      tTimeSeconds= 0;
    struct tm   tTimeStruct;

	if( ptDateTime != NULL )
	{
		// set time in seconds
		tTimeSeconds = dwEpochSecs;

		// convert to time structure
		tTimeStruct =* localtime( &tTimeSeconds );

		ptDateTime->tDate.wYear       = tTimeStruct.tm_year + 1900; // tm_year must be years since 1900
		ptDateTime->tDate.bMonth      = tTimeStruct.tm_mon + 1;   /* localtime() month, range 0 to 11 */
		ptDateTime->tDate.bDate       = tTimeStruct.tm_mday;

		ptDateTime->tTime.bHour       = tTimeStruct.tm_hour;
		ptDateTime->tTime.bMinute     = tTimeStruct.tm_min;
		ptDateTime->tTime.bSecond     = tTimeStruct.tm_sec;

		if
		(
			( SysTimeIsDateValid( &ptDateTime->tDate ) ) &&
			( SysTimeIsTimeValid( &ptDateTime->tTime ) )
		)
		{
			fSuccess = TRUE;
		}
	}

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
