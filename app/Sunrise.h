//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SUNRISE_H_
#define SUNRISE_H_

typedef struct
{
    DOUBLE dbYear;      // YYYY
    DOUBLE dbMonth;     // 1-12
    DOUBLE dbDay;       // 1-31
    SINGLE sgLatDecimal;// decimal format
    SINGLE sgLonDecimal;// decimal format
    DOUBLE dbTimeZone;
}SunriseInputStruct;

typedef enum
{
    SUNRISE_SUN_ALT_SOUTH = 0,
    SUNRISE_SUN_ALT_NORTH,
    
    SUNRISE_SUN_ALT_MAX
}SunAltitudeResultEnum;

typedef struct
{
    DOUBLE                  dbAltitude_deg;
    SunAltitudeResultEnum   eOrientation;
    DOUBLE                  dbNoonTime_decimalHours;
}SunAltitudeResultStruct;

typedef struct
{
    DOUBLE dbDaysSinceY2K;
    DOUBLE dbDeclination;
    DOUBLE dbDaylength_decimalHours;
    DOUBLE dbSunriseCivilTwilight_decimalHours;
    DOUBLE dbSunrise_decimalHours;
    DOUBLE dbSunset_decimalHours;
    DOUBLE dbSunsetCivilTwilight_decimalHours;
    SunAltitudeResultStruct tSunAltitude;
}SunriseResultStruct;

BOOL SunriseProcessInput    ( SunriseInputStruct * ptInput, SunriseResultStruct * ptResult );

#endif /* SUNRISE_H_ */

//////////////////////////////////////////////////////////////////////////////////////////////////
