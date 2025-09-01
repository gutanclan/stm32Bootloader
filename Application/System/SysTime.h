/** C Header ******************************************************************

*******************************************************************************/

#ifndef SYS_TIME_H
#define SYS_TIME_H

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	UINT16	wYear;          // 0000..9999
	UINT8 	bMonth;         // 1..12
    UINT8 	bDate;          // 1..31, Day of Month
}__attribute__((__packed__)) SysTimeDateStruct;

typedef struct
{
	UINT8 	bHour;          // 0..23, Hour in 24-hour format
	UINT8 	bMinute;        // 0..59, Minute
    UINT8 	bSecond;        // 0..59, Second
}__attribute__((__packed__)) SysTimeTimeStruct;

typedef struct
{
	SysTimeDateStruct tDate;
	SysTimeTimeStruct tTime;
}__attribute__((__packed__)) SysTimeDateTimeStruct;

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL   	SysTimeInit	( void );

BOOL 	SysTimeSetUtcTimeZoneOffset 	( FLOAT sgTimeZoneOffset );
FLOAT 	SysTimeGetUtcTimeZoneOffset 	( void );

BOOL 	SysTimeSetDateTimeUtcZero       ( SysTimeDateTimeStruct *ptDateTimeUtcZero );
BOOL 	SysTimeGetDateTime              ( SysTimeDateTimeStruct *ptDateTimeUtc, BOOL fApplyTimeZoneOffset );

UINT32  SysTimeDateTimeToEpochSeconds	( SysTimeDateTimeStruct *ptDateTime );
BOOL    SysTimeEpochSecondsToDateTime   ( UINT32 dwEpochSecs, SysTimeDateTimeStruct *ptDateTime );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // SYS_TIME_H
