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

typedef enum
{
    ADC_TEMP,
    ADC_VREF,
    ADC_BAT,
	ADC_PIN_POT1,
    ADC_PIN_POT2,

	ADC_PIN_MAX,		// Total Number of ADC inputs defined
}AdcInputEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL 	    AdcModuleInit			            ( void );
BOOL        AdcEnableAdcChannel                 ( AdcInputEnum eAdc, BOOL fEnable );

BOOL 	    AdcReadRaw		                    ( AdcInputEnum eAdc, UINT8 bNumberOfSamples, FLOAT *psgAdcReadResult );
BOOL        AdcRawToMilliVolts                  ( AdcInputEnum eAdc, FLOAT sgRawAdc, FLOAT *psgMillivotsResult );

BOOL		AdcMilliVoltsToToDegC		        ( AdcInputEnum eAdc, FLOAT sgMilliVolts, FLOAT *psgTempDegCResult );

//CHAR* 	    AdcGetInputStringName		        ( AdcInputEnum eAdc );

//void			AdcInternalSinglePinPrintStatus	( ConsolePortEnum eConsole, AdcSensorEnum eAdcSensor );
//void			AdcInternalAllPinsPrintStatus	( ConsolePortEnum eConsole );

void        AdcTest                             ( void );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _ADC_H_




