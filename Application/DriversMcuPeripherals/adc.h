//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Adc.h
//!    \brief       ADC module header.
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ADC_H_
#define _ADC_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ADC_RESOLUTION_VAL_MAX                      (4096.0)// 12 bit
#define ADC_MILLIVOLT_VAL_MAX                       (3300.0)// max 3.3V...datasheet indicates 3.6 but never use the max limits

#define ADC_INTERNAL_TEMP_MAX                       (125.0) // max 3.3V...datasheet indicates 3.6 but never use the max limits
#define ADC_INTERNAL_TEMP_MIN                       (-40.0)   // max 3.3V...datasheet indicates 3.6 but never use the max limits

typedef enum
{
    ADC_TEMP,
    ADC_VREF,
    ADC_BAT,
	ADC_PIN_POT1,
    ADC_PIN_POT2,

	ADC_PIN_MAX,		// Total Number of ADC inputs defined
}AdcInputEnum;

BOOL    AdcModuleInit           ( void );
BOOL    AdcEnable               ( AdcInputEnum eAdc, BOOL fEnable );
BOOL    AdcIsEnable             ( AdcInputEnum eAdc, BOOL *pfEnable );

BOOL 	AdcReadRaw		        ( AdcInputEnum eAdc, UINT8 bNumberOfSamples, FLOAT *psgAdcReadResult );

CHAR* 	AdcGetName		        ( AdcInputEnum eAdc );
BOOL  	AdcGetGpioPort		    ( AdcInputEnum eAdc, GpioPortEnum *peGpioPort, UINT8 *pbGpioPin );

BOOL	AdcMilliVoltsToToDegC	( AdcInputEnum eAdc, FLOAT sgMilliVolts, FLOAT *psgTempDegCResult );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _ADC_H_




