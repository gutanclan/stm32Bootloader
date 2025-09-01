/** C Header ******************************************************************
// http://www.sci.fi/~benefon/rscalc.c

// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
*******************************************************************************/

#include "Types.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <Sunrise.h>

//////////////////////////////////////////////////////////////////////////////////////////////////

#define     PI              (3.14159)
#define     SUNDIA          (0.53)      // Sunradius degrees
#define     AIR_REFR        (34.0/60.0) // Athmospheric refraction degrees

//////////////////////////////////////////////////////////////////////////////////////////////////

static double degs  = 180.0/PI;
static double rads  = PI/180.0;
static double L     = 0;
static double g     = 0;
static double daylen= 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

static double  FNday    ( int y, int m, int d, float h );
static double  FNrange  ( double x );
static double  f0       ( double lat, double declin );
static double  f1       ( double lat, double declin );
static double  FNsun    ( double d );
//static void    showhrmn ( double dhr );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SunriseProcessInput( SunriseInputStruct * ptInput, SunriseResultStruct * ptResult )
{
    BOOL fSuccess = FALSE;
    
    double d,lambda;
    double obliq,alpha,delta,LL,equation,ha,hb,twx;
    double twam,altmax,noont,settm,riset,twpm;
    
    if( (ptInput != NULL) && (ptResult != NULL) )
    {
        //   Get the days to J2000
        // 12 param indicates 12 hours or noon
        d = FNday(ptInput->dbYear, ptInput->dbMonth, ptInput->dbDay, 12);
        
        //   Use FNsun to find the ecliptic longitude of the
        //   Sun
        lambda = FNsun(d);

        //   Obliquity of the ecliptic
        obliq = 23.439 * rads - .0000004 * rads * d;

        //   Find the RA and DEC of the Sun
        alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
        delta = asin(sin(obliq) * sin(lambda));

        // Find the Equation of Time
        // in minutes
        // Correction suggested by David Smith
        LL = L - alpha;
        if (L < PI)
        {
            LL += 2.0*PI;
        }
        equation = 1440.0 * (1.0 - LL / PI/2.0);
        ha = f0(ptInput->sgLatDecimal,delta);
        hb = f1(ptInput->sgLatDecimal,delta);
        twx = hb - ha;	// length of twilight in radians
        twx = 12.0*twx/PI;		// length of twilight in hours
        
        // Conversion of angle to hours and minutes //
        daylen = degs*ha/7.5;
        if (daylen<0.0001)
        {
            daylen = 0.0;
        }

        // arctic winter     //
        riset = 12.0 - 12.0 * ha/PI + ptInput->dbTimeZone - ptInput->sgLonDecimal/15.0 + equation/60.0;
        settm = 12.0 + 12.0 * ha/PI + ptInput->dbTimeZone - ptInput->sgLonDecimal/15.0 + equation/60.0;
        noont = riset + 12.0 * ha/PI;
        altmax = 90.0 + delta * degs - ptInput->sgLatDecimal;

        // Correction for S HS suggested by David Smith
        // to express altitude as degrees from the N horizon
        if (ptInput->sgLatDecimal < delta * degs)
        {
            altmax = 180.0 - altmax;
        }

        twam = riset - twx;	// morning twilight begin
        twpm = settm + twx;	// evening twilight end
        if (riset > 24.0)
        {
            riset-= 24.0;
        }
        if (settm > 24.0)
        {
            settm-= 24.0;
        }
        
        ptResult->dbDaysSinceY2K = d;
        ptResult->dbDeclination = delta * degs;
        ptResult->dbDaylength_decimalHours = daylen;
        ptResult->dbSunriseCivilTwilight_decimalHours = twam;
        ptResult->dbSunrise_decimalHours = riset;
        ptResult->dbSunset_decimalHours = settm;
        ptResult->dbSunsetCivilTwilight_decimalHours = twpm;
        
        ptResult->tSunAltitude.dbAltitude_deg = altmax;
        ptResult->tSunAltitude.eOrientation = ( ptInput->sgLatDecimal >= 0.0 ) ? SUNRISE_SUN_ALT_SOUTH : SUNRISE_SUN_ALT_NORTH;
        ptResult->tSunAltitude.dbNoonTime_decimalHours = noont;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   Get the days to J2000
//   h is UT in decimal hours
//   FNday only works between 1901 to 2099 - see Meeus chapter 7
//////////////////////////////////////////////////////////////////////////////////////////////////
double FNday (int y, int m, int d, float h)
{
    long int luku = - 7 * (y + (m + 9)/12)/4 + 275*m/9 + d;

    // type casting necessary on PC DOS and TClite to avoid overflow
    luku+= (long int)y*367;

    return (double)luku - 730531.5 + h/24.0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   the function below returns an angle in the range
//   0 to 2*pi
//////////////////////////////////////////////////////////////////////////////////////////////////
double FNrange (double x)
{
    double b = 0.5*x / PI;
    double a = 2.0*PI * (b - (long)(b));

    if (a < 0)
    {
        a = 2.0*PI + a;
    }

    return a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Calculating the hourangle
//////////////////////////////////////////////////////////////////////////////////////////////////
double f0(double lat, double declin)
{
    double fo,dfo;

    // Correction: different sign at S HS
    dfo = rads*(0.5*SUNDIA + AIR_REFR);

    if (lat < 0.0)
    {
        dfo = -dfo;
    }

    fo = tan(declin + dfo) * tan(lat*rads);

    if (fo>0.99999)
    {
        fo=1.0; // to avoid overflow //
    }

    fo = asin(fo) + PI/2.0;

    return fo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Calculating the hourangle for twilight times
//////////////////////////////////////////////////////////////////////////////////////////////////
double f1( double lat, double declin )
{
    double fi,df1;

    // Correction: different sign at S HS
    df1 = rads * 6.0;

    if (lat < 0.0)
    {
        df1 = -df1;
    }

    fi = tan(declin + df1) * tan(lat*rads);

    if (fi>0.99999)
    {
        fi=1.0; // to avoid overflow //
    }

    fi = asin(fi) + PI/2.0;

    return fi;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Find the ecliptic longitude of the Sun
//////////////////////////////////////////////////////////////////////////////////////////////////
double FNsun( double d )
{
    //   mean longitude of the Sun

    L = FNrange(280.461 * rads + .9856474 * rads * d);

    //   mean anomaly of the Sun

    g = FNrange(357.528 * rads + .9856003 * rads * d);

    //   Ecliptic longitude of the Sun

    return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//// Display decimal hours in hours and minutes
////////////////////////////////////////////////////////////////////////////////////////////////////
//void showhrmn( double dhr )
//{
//    int hr,mn;
//
//    hr = (int) dhr;
//
//    mn = (dhr - (double) hr)*60;
//
//    printf("%0d:%0d",hr,mn);
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
