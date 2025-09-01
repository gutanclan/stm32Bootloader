//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ADC_2_H_
#define _ADC_2_H_

#define ADC_RESOLUTION_VAL_MAX                  (4096.0)  // 12 bit
#define ADC_MILLIVOLT_MAX                       (3300.0)  // max 3.3V...datasheet indicates 3.6 but never use the max limits

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

BOOL        AdcChannelEnable                 	( AdcInputEnum eAdc, BOOL fEnable );
BOOL        AdcChannelIsEnabled                 ( AdcInputEnum eAdc );

BOOL 	    AdcGpioGetPortPin		            ( AdcInputEnum eAdc, GpioPortEnum *pePort, UINT8 *pbPinNumber );

BOOL 	    AdcReadRaw		                    ( AdcInputEnum eAdc, UINT8 bNumberOfSamples, FLOAT *psgAdcReadResult );
BOOL        AdcRawToMilliVolts                  ( AdcInputEnum eAdc, FLOAT sgRawAdc, FLOAT *psgMillivotsResult );

BOOL		AdcMilliVoltsToToDegC		        ( AdcInputEnum eAdc, FLOAT sgMilliVolts, FLOAT *psgTempDegCResult );

//CHAR* 	    AdcGetInputStringName		        ( AdcInputEnum eAdc );

//void			AdcInternalSinglePinPrintStatus	( ConsolePortEnum eConsole, AdcSensorEnum eAdcSensor );
//void			AdcInternalAllPinsPrintStatus	( ConsolePortEnum eConsole );

void        AdcTest                             ( void );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _ADC_2_H_




