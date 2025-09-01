//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Adc.c
//!    \brief       Functions for ADC pins (sensors).
//!
//!    \author
//!    \date
//!
//!    \notes        *********CONSIDER TO ADD SELF CALIBRATION******
//!                 Read link: http://www.micromouseonline.com/2009/05/26/simple-adc-use-on-the-stm32/#axzz1r6gyjlqh
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "../includesProject.h"
#include "gpioPinCommonConfigs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Module Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ADC_RESOLUTION_MAX                      (4096.0)  // 12 bit
#define ADC_MILLIVOLT_MAX                       (3300.0)  // max 3.3V...datasheet indicates 3.6 but never use the max limits
#define ADC_NO_GPIO_PIN                         (GPIO_INVALID_PIN)

#define ADC_TEMP_VSENSE_25_DEG_C_MILLIVOLT      (760.0)
#define ADC_TEMP_AVG_SLOPE                      (2.5)
#define ADC_TEMP_OFFSET_DEG_C                   (25)

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    ADC_TypeDef                 *ptAdc;
    UINT8                       bChannel;
    UINT32                      dwRcc;
}AdcChannCommonType;

typedef struct
{
    UINT8                       bRank;
    UINT32                      dwADC_SampleTime;
}AdcChannConfigType;

typedef struct
{
    BOOL                        fEnabled;
}AdcChannStatusType;

typedef struct
{
    GpioPinEnum                 eGpioPin;
    GpioAfInfoType              tGpioAf;
}AdcChannPinType;

typedef struct
{
    const GpioPeripheralType   *ptPeriph;
    const AdcChannCommonType    tChannCommon;
    const AdcChannConfigType    tChannConfig;
    AdcChannStatusType          tChannStat;
    const AdcChannPinType       tChannPin;
}AdcType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

const GpioConfigType gtGpioAdcDefaultConfig             = { GPIO_MODE_INPUT,    GPIO_OTYPE_PUSH_PULL,   GPIO_OSPEED_2MHZ,   GPIO_PUPD_NOPUPD };
static const GpioPeripheralType     gtAdcPeriph         = { "Adc", GPIO_OTHER_PERIPHERAL_ADC };
static AdcType                      gtAdcLookupArray[]  =
{
    {
        // ### PERIPH
        &gtAdcPeriph,
        // ### COMMON
        // adc #                            Channel #                           rcc
        { TARGET_MCU_TEMP_ADC,              TARGET_MCU_TEMP_ADC_CHAN,           TARGET_MCU_TEMP_ADC_RCC },
        // ### CONFIG
        // group sequence rank (if apply)   SampleTime
        { 1,                                ADC_SampleTime_480Cycles },
        // ### STAT
        // enabled
        { FALSE },
        // ### PIN
        {
            // ### PIN ENUM
            ADC_NO_GPIO_PIN,
            // ### PIN AF
            // af Name                  af define               eAfDisableGpioLogic         pAfDisabledGpioConfig
            { TARGET_MCU_TEMP_ADC_NAME, GPIO_NO_AF_EXCEPTION,   GPIO_LOGIC_NOT_REQUIRED,    &ggtGpioPinCommonConfigs_In_OPp_O2MHz_RPd }
        },
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    {
        // ### PERIPH
        &gtAdcPeriph,
        // ### COMMON
        // adc #                            Channel #                           rcc
        { TARGET_MCU_VREF_ADC,              TARGET_MCU_VREF_ADC_CHAN,           TARGET_MCU_VREF_ADC_RCC },
        // ### CONFIG
        // group sequence rank (if apply)   SampleTime
        { 1,                                ADC_SampleTime_480Cycles },
        // ### STAT
        // enabled
        { FALSE },
        // ### PIN
        {
            // ### PIN ENUM
            ADC_NO_GPIO_PIN,
            // ### PIN AF
            // af Name                  af define               eAfDisableGpioLogic         pAfDisabledGpioConfig
            { TARGET_MCU_VREF_ADC_NAME, GPIO_NO_AF_EXCEPTION,   GPIO_LOGIC_NOT_REQUIRED,    &ggtGpioPinCommonConfigs_In_OPp_O2MHz_RPd }
        },
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    {
        // ### PERIPH
        &gtAdcPeriph,
        // ### COMMON
        // adc #                            Channel #                           rcc
        { TARGET_MCU_BAT_ADC,               TARGET_MCU_BAT_ADC_CHAN,            TARGET_MCU_BAT_ADC_RCC },
        // ### CONFIG
        // group sequence rank (if apply)   SampleTime
        { 1,                                ADC_SampleTime_480Cycles },
        // ### STAT
        // enabled
        { FALSE },
        // ### PIN
        {
            // ### PIN ENUM
            ADC_NO_GPIO_PIN,
            // ### PIN AF
            // af Name                  af define               eAfDisableGpioLogic         pAfDisabledGpioConfig
            { TARGET_MCU_BAT_ADC_NAME,  GPIO_NO_AF_EXCEPTION,   GPIO_LOGIC_NOT_REQUIRED,    &ggtGpioPinCommonConfigs_In_OPp_O2MHz_RPd }
        },
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    {
        // ### PERIPH
        &gtAdcPeriph,
        // ### COMMON
        // adc #                            Channel #                           rcc
        { TARGET_POT1_ADC,                  TARGET_POT1_ADC_CHAN,               TARGET_POT1_ADC_RCC },
        // ### CONFIG
        // group sequence rank (if apply)   SampleTime
        { 1,                                ADC_SampleTime_480Cycles },
        // ### STAT
        // enabled
        { FALSE },
        // ### PIN
        {
            // ### PIN ENUM
            GPIO_PIN_POT_1,
            // ### PIN AF
            // af Name                  af define               eAfDisableGpioLogic         pAfDisabledGpioConfig
            { TARGET_POT1_ADC_NAME,     GPIO_NO_AF_EXCEPTION,   GPIO_LOGIC_NOT_REQUIRED,    &ggtGpioPinCommonConfigs_In_OPp_O2MHz_RPd }
        },
    },
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    {
        // ### PERIPH
        &gtAdcPeriph,
        // ### COMMON
        // adc #                            Channel #                           rcc
        { TARGET_POT2_ADC,                  TARGET_POT2_ADC_CHAN,               TARGET_POT2_ADC_RCC },
        // ### CONFIG
        // group sequence rank (if apply)   SampleTime
        { 1,                                ADC_SampleTime_480Cycles },
        // ### STAT
        // enabled
        { FALSE },
        // ### PIN
        {
            // ### PIN ENUM
            GPIO_PIN_POT_2,
            // ### PIN AF
            // af Name                  af define               eAfDisableGpioLogic         pAfDisabledGpioConfig
            { TARGET_POT2_ADC_NAME,     GPIO_NO_AF_EXCEPTION,   GPIO_LOGIC_NOT_REQUIRED,    &ggtGpioPinCommonConfigs_In_OPp_O2MHz_RPd }
        },
    }
};

#define ADC_ARRAY_MAX    ( sizeof(gtAdcLookupArray) / sizeof( gtAdcLookupArray[0] ) )

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     BOOL AdcModuleInit( void )
//!
//! \brief  Initialize ADC variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcModuleInit( void )
{
    BOOL    fSuccess = FALSE;

    if( ADC_ARRAY_MAX != ADC_PIN_MAX )
    {
        // ADC definition mismatch error
        while(1);
    }
    else
    {
        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL AdcInitialieSingleAdc( AdcInputEnum eAdc )
//!
//! \brief      Initializes a particular ADC listed in
//!             AdcInputEnum
//!
//! \param[in]  eAdc      enum number of the ADC that is required to be initialized
//!
//! \return     BOOL
//!             - TRUE if the operation successfully
//!             - FALSE otherwise.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcEnableAdcChannel( AdcInputEnum eAdc, BOOL fEnable )
{
    BOOL fSuccess = FALSE;

    if( eAdc < ADC_ARRAY_MAX )
    {
        if( fEnable )
        {
            //////////////////////////////////////////////////
            // NOTE: set gpio IF APPLIES!!.
            // not all adc are gpios!!!
            // make sure gpio is initialized
            GpioSetConfig( gtAdcLookupArray[eAdc].tChannPin.eGpioPin, &gtGpioAdcDefaultConfig );

            // make request so that adc can take over pin control
            GpioDelegateConfigControlTo
            (
                gtAdcLookupArray[eAdc].tChannPin.eGpioPin,
                gtAdcLookupArray[eAdc].ptPeriph,
                &gtAdcLookupArray[eAdc].tChannPin.tGpioAf
            );
            //////////////////////////////////////////////////


            // #### ADC INIT ####
            ADC_InitTypeDef       ADC_InitStructure;
            ADC_CommonInitTypeDef ADC_CommonInitStructure;

            //initialized structures to default values
            ADC_StructInit( &ADC_InitStructure );
            ADC_CommonStructInit( &ADC_CommonInitStructure );

            // enable ADC clock
            RCC_APB2PeriphClockCmd( gtAdcLookupArray[eAdc].tChannCommon.dwRcc, ENABLE );

            // ADC Common Init
            ADC_CommonInitStructure.ADC_Mode                = ADC_Mode_Independent;
            ADC_CommonInitStructure.ADC_Prescaler           = ADC_Prescaler_Div2;
            ADC_CommonInitStructure.ADC_DMAAccessMode       = ADC_DMAAccessMode_Disabled;
            ADC_CommonInitStructure.ADC_TwoSamplingDelay    = ADC_TwoSamplingDelay_10Cycles;    // delay between 2 samples
            ADC_CommonInit(&ADC_CommonInitStructure);

            // ADC Init
            ADC_InitStructure.ADC_Resolution                = ADC_Resolution_12b;
            ADC_InitStructure.ADC_ScanConvMode              = DISABLE;
            ADC_InitStructure.ADC_ContinuousConvMode        = DISABLE;
            ADC_InitStructure.ADC_ExternalTrigConvEdge      = ADC_ExternalTrigConvEdge_None;
            ADC_InitStructure.ADC_DataAlign                 = ADC_DataAlign_Right;
            ADC_InitStructure.ADC_NbrOfConversion           = 1;
            ADC_Init(gtAdcLookupArray[eAdc].tChannCommon.ptAdc, &ADC_InitStructure);

            gtAdcLookupArray[eAdc].tChannStat.fEnabled = TRUE;
        }
        else
        {
            if( eAdc == ADC_BAT )
            {
                ADC_VBATCmd( DISABLE );

                gtAdcLookupArray[eAdc].tChannStat.fEnabled = FALSE;
                fSuccess  = TRUE;
            }
            else if
            (
                ( eAdc == ADC_TEMP ) ||
                ( eAdc == ADC_VREF )
            )
            {
                ADC_TempSensorVrefintCmd( DISABLE );

                gtAdcLookupArray[ADC_TEMP].tChannStat.fEnabled = FALSE;
                gtAdcLookupArray[ADC_VREF].tChannStat.fEnabled = FALSE;
                fSuccess  = TRUE;
            }
            else
            {
                if
                (
                    GpioRecoverConfigControlFrom
                    (
                        gtAdcLookupArray[eAdc].tChannPin.eGpioPin,
                        gtAdcLookupArray[eAdc].ptPeriph
                    ) == TRUE
                )
                {
                    // set the gpio config used when AF is disabled
                    GpioOutputRegWrite( gtAdcLookupArray[eAdc].tChannPin.eGpioPin, gtAdcLookupArray[eAdc].tChannPin.tGpioAf.eAfDisableGpioLogic );
                    GpioSetConfig( gtAdcLookupArray[eAdc].tChannPin.eGpioPin, gtAdcLookupArray[eAdc].tChannPin.tGpioAf.pAfDisabledGpioConfig );

                    gtAdcLookupArray[eAdc].tChannStat.fEnabled = FALSE;
                    fSuccess  = TRUE;
                }
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL AdcReadRaw( AdcInputEnum eAdc, UINT8 bNumberOfSamples, FLOAT *psgAdcReadResult )
//!
//! \brief      Obtains an ADC reading depending on the eAdc selected.
//!             It can also take several readings and calculate the average.
//!
//! \param[in]  eAdc                ADC enum
//!
//! \param[in]  bNumberOfSamples    number of samples to be taken
//!
//! \param[in]  *psgAdcReadResult   pointer to a FLOAT var that will store the result
//!
//! \return     BOOL
//!             - TRUE if the operation successfully
//!             - FALSE otherwise.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcReadRaw( AdcInputEnum eAdc, UINT8 bNumberOfSamples, FLOAT *psgAdcReadResult )
{
    BOOL             fSuccess             = FALSE;
    INT32            idwSum               = 0;
    UINT8            bCount;

    if
    (
        ( eAdc < ADC_ARRAY_MAX ) &&
        ( NULL != psgAdcReadResult )
    )
    {
        (*psgAdcReadResult) = -ADC_RESOLUTION_MAX;

        if( gtAdcLookupArray[eAdc].tChannStat.fEnabled == TRUE )
        {
            // ADC config
            ADC_RegularChannelConfig
            (
                gtAdcLookupArray[eAdc].tChannCommon.ptAdc,
                gtAdcLookupArray[eAdc].tChannCommon.bChannel,
                gtAdcLookupArray[eAdc].tChannConfig.bRank,
                gtAdcLookupArray[eAdc].tChannConfig.dwADC_SampleTime
            );

            ///////////////////////////////////////////////////////////
            // enable/disable the TSVREFE bit if temp or VRef
            if
            (
                ( eAdc == ADC_TEMP ) ||
                ( eAdc == ADC_VREF )
            )
            {
                ADC_TempSensorVrefintCmd( ENABLE );
            }
            else
            // VBATE bit
            if( eAdc == ADC_BAT )
            {
                ADC_VBATCmd( ENABLE );
            }
            ///////////////////////////////////////////////////////////


            // Enable ADCx
            ADC_Cmd( gtAdcLookupArray[eAdc].tChannCommon.ptAdc, ENABLE );

            //take X number of samples
            for( bCount=0 ; bCount < bNumberOfSamples ; bCount++ )                                            // Take X number of samples
            {
                ADC_SoftwareStartConv( gtAdcLookupArray[eAdc].tChannCommon.ptAdc );                                        // Start converting

                while( ADC_GetFlagStatus( gtAdcLookupArray[eAdc].tChannCommon.ptAdc, ADC_FLAG_EOC ) == RESET );            // CHECK end of conversion FLAG

                idwSum = idwSum + ADC_GetConversionValue( gtAdcLookupArray[eAdc].tChannCommon.ptAdc );                    // Get ADC value
            }

            // Disable ADCx
            ADC_Cmd( gtAdcLookupArray[eAdc].tChannCommon.ptAdc, DISABLE );

            ///////////////////////////////////////////////////////////
            // enable/disable the TSVREFE bit if temp or VRef
            if
            (
                ( eAdc == ADC_TEMP ) ||
                ( eAdc == ADC_VREF )
            )
            {
                ADC_TempSensorVrefintCmd( DISABLE );
            }
            else
            // VBATE bit
            if( eAdc == ADC_BAT )
            {
                ADC_VBATCmd( DISABLE );
            }
            ///////////////////////////////////////////////////////////

            //now calculate the average
            (*psgAdcReadResult) = ( (FLOAT)idwSum ) / ( (FLOAT)bNumberOfSamples );                        // Calculate the average

            if( eAdc == ADC_BAT )
            {
                (*psgAdcReadResult) = (*psgAdcReadResult) * 2;
            }

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL AdcRawToMilliVolts( AdcInputEnum eAdc, FLOAT sgRawAdc, FLOAT *psgMillivotsResult )
//!
//! \brief      Convert the ADC raw value into milliVolts.
//!             Fixed for resolution of 12bits and 3.3 Volts Max.
//!
//! \param[in]  eAdc                Adc input
//!
//! \param[in]  sgRawAdc            raw reading value
//!
//! \param[in]  psgMillivotsResult  millivolts result
//!
//! \return     BOOL
//!             - TRUE if the operation successfully
//!             - FALSE otherwise.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcRawToMilliVolts( AdcInputEnum eAdc, FLOAT sgRawAdc, FLOAT *psgMillivotsResult )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( eAdc < ADC_ARRAY_MAX ) &&
        ( NULL != psgMillivotsResult )
    )
    {
        (*psgMillivotsResult) = 0;

        // #### VALIDATING DATA ####
        sgRawAdc = ( sgRawAdc > (ADC_RESOLUTION_MAX-1) )  ? (ADC_RESOLUTION_MAX-1) : sgRawAdc  ;

        // #### PERFORMING OPERATION ####
        (*psgMillivotsResult) = ( sgRawAdc * ADC_MILLIVOLT_MAX ) / ADC_RESOLUTION_MAX;

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL AdcMilliVoltsToToDegC( AdcInputEnum eAdc, FLOAT sgMilliVolts, FLOAT *psgTempDegCResult )
//!
//! \brief      Converst millivolts to deg C
//!
//! \param[in]  eAdc                it only makes the conversion only for ADC_TEMP.
//!
//! \param[in]  sgMilliVolts        value of millivolts
//!
//! \param[in]  psgTempDegCResult   Result of conversion from millivolts to deg Celsius
//!
//! \return     BOOL
//!             - TRUE if the operation successfully
//!             - FALSE otherwise.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcMilliVoltsToToDegC( AdcInputEnum eAdc, FLOAT sgMilliVolts, FLOAT *psgTempDegCResult )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( eAdc == ADC_TEMP ) &&
        ( psgTempDegCResult != NULL )
    )
    {
        FLOAT sgTempDegC    = 0;

        sgTempDegC = sgMilliVolts - ADC_TEMP_VSENSE_25_DEG_C_MILLIVOLT;
        sgTempDegC = sgTempDegC / ADC_TEMP_AVG_SLOPE;
        sgTempDegC = sgTempDegC  + ADC_TEMP_OFFSET_DEG_C;

        (*psgTempDegCResult) = sgTempDegC;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         CHAR * AdcGetInputStringName( AdcInputEnum eAdc )
//!
//! \brief      Returns a pointer to a string that contains the name of the ADC
//!
//! \param[in]  eAdc      enum number of the ADC that is required to get its string name
//!
//! \return     CHAR *    pointer to the string name of the ADC
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//CHAR * AdcGetInputStringName( AdcInputEnum eAdc )
//{
//    CHAR * pcNameString = NULL;
//
//    if( eAdc < ADC_ARRAY_MAX )
//    {
//        pcNameString = gtAdcLookupArray[eAdc].tChannPin.eGpioPin;
//    }
//
//    return pcNameString;
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         void AdcInternalSinglePinPrintStatus( ConsolePortEnum eConsole, AdcSensorEnum eAdcSensor )
//!
//! \brief      Print status of a single adc pin to specified console
//!
//! \param[in]  pFile      pointer to a FILE to print ADC status
//!
//! \return     void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void AdcInternalSinglePinPrintStatus( ConsolePortEnum eConsole, AdcSensorEnum eAdcSensor )
//{
//    if( eAdcSensor < ADC_NUMBER_OF_PINS )
//    {
//        UINT16 wReadingRaw          = 0;
//        SINGLE sgReadingMillVolt    = 0;
//
//        AdcInternalGetReadingRaw( eAdcSensor, 1, &wReadingRaw );
//        sgReadingMillVolt = AdcInternalRawToMilliVolts( eAdcSensor, wReadingRaw );
//
//        ConsolePrintf( eConsole, "----------------------\r\n" );
//        ConsolePrintf( eConsole, "PinNumber[%d]:\t\t%d\r\n",            eAdcSensor, eAdcSensor );
//        ConsolePrintf( eConsole, "PcomName[%d]:\t\t%s\r\n",             eAdcSensor, gtAdcLookupArray[eAdcSensor].pcName );
//        ConsolePrintf( eConsole, "aftiName[%d]:\t\t%s\r\n",             eAdcSensor, gtAdcLookupArray[eAdcSensor].pcTag );
//        ConsolePrintf( eConsole, "IsPinInitialized[%d]:\t%d\r\n",       eAdcSensor, gfIsPinInitArray[eAdcSensor] );
//        ConsolePrintf( eConsole, "RawReading[%d]:\t\t%d LSB\r\n",       eAdcSensor, wReadingRaw );
//        ConsolePrintf( eConsole, "RawToMillV[%d]:\t\t%.3f mV\r\n",      eAdcSensor, sgReadingMillVolt );
//
//        if (gtAdcLookupArray[eAdcSensor].fIsThermSensor)
//        {
//            SINGLE sgTempDegCResult = 0.0f;
//            AdcInternalMilliVoltsToDegC(sgReadingMillVolt, &sgTempDegCResult);
//            ConsolePrintf(eConsole, "Temperature[%d]:\t\t%.3f DegC\r\n", eAdcSensor, sgTempDegCResult);
//        }
//    }
//}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         void AdcInternalAllPinsPrintStatus( ConsolePortEnum eConsole )
//!
//! \brief      Print status of all adc pins to specified console
//!
//! \param[in]  pFile      pointer to a FILE to print ADC status
//!
//! \return     void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//void AdcInternalAllPinsPrintStatus( ConsolePortEnum eConsole )
//{
//    for( AdcSensorEnum eAdcCounter = 0 ; eAdcCounter < ADC_NUMBER_OF_PINS ; eAdcCounter++ )
//    {
//        AdcInternalSinglePinPrintStatus( eConsole, eAdcCounter );
//    }
//}
//

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         void AdcTest( void )
//!
//! \brief      shows module implementation
//!
//! \param[in]  void
//!
//! \return     void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void AdcTest( void )
{
    AdcModuleInit();

    FLOAT sgAdcReadRaw      = 0.0;
    FLOAT sgAdcRead_mV      = 0.0;
    FLOAT sgTempDegC        = 0.0;

    UINT32 dwDelay          = 0;
    UINT32 dwAdcTimeCounter = 0;

    UINT8   bCounter;

    while(1)
    {
        bCounter = 3;

        AdcEnableAdcChannel( ADC_TEMP, TRUE );
        AdcEnableAdcChannel( ADC_VREF, TRUE );
        AdcEnableAdcChannel( ADC_BAT, TRUE );
        AdcEnableAdcChannel( ADC_PIN_POT1, TRUE );
        AdcEnableAdcChannel( ADC_PIN_POT2, TRUE );

        while( bCounter )
        {
            if( dwAdcTimeCounter++ == 10 )
            {
                bCounter--;
                dwAdcTimeCounter = 0;

                AdcReadRaw( ADC_TEMP, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_TEMP, sgAdcReadRaw, &sgAdcRead_mV );
                AdcMilliVoltsToToDegC( ADC_TEMP, sgAdcRead_mV, &sgTempDegC );
                printf("Tmp Adc: raw=%d mVolt=%d DegC=%d\r\n", (int)(sgAdcReadRaw), (int)(sgAdcRead_mV), (int)(sgTempDegC) );

                AdcReadRaw( ADC_VREF, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_VREF, sgAdcReadRaw, &sgAdcRead_mV );
                printf("VRef Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_BAT, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_BAT, sgAdcReadRaw, &sgAdcRead_mV );
                printf("VBat Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_PIN_POT1, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_PIN_POT1, sgAdcReadRaw, &sgAdcRead_mV );
                printf("Pot1 Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_PIN_POT2, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_PIN_POT2, sgAdcReadRaw, &sgAdcRead_mV );
                printf("Pot2 Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                printf("\r\n");
            }

            dwDelay = 1000000;
            while( dwDelay-- ){}
            dwDelay = 1000000;
            while( dwDelay-- ){}
        }

        bCounter = 3;

        AdcEnableAdcChannel( ADC_TEMP, FALSE );
        AdcEnableAdcChannel( ADC_VREF, FALSE );
        AdcEnableAdcChannel( ADC_BAT, FALSE );
        AdcEnableAdcChannel( ADC_PIN_POT1, FALSE );
        AdcEnableAdcChannel( ADC_PIN_POT2, FALSE );

        while( bCounter )
        {
            if( dwAdcTimeCounter++ == 10 )
            {
                bCounter--;
                dwAdcTimeCounter = 0;

                AdcReadRaw( ADC_TEMP, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_TEMP, sgAdcReadRaw, &sgAdcRead_mV );
                AdcMilliVoltsToToDegC( ADC_TEMP, sgAdcRead_mV, &sgTempDegC );
                printf("Tmp Adc: raw=%d mVolt=%d DegC=%d\r\n", (int)(sgAdcReadRaw), (int)(sgAdcRead_mV), (int)(sgTempDegC) );

                AdcReadRaw( ADC_VREF, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_VREF, sgAdcReadRaw, &sgAdcRead_mV );
                printf("VRef Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_BAT, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_BAT, sgAdcReadRaw, &sgAdcRead_mV );
                printf("VBat Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_PIN_POT1, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_PIN_POT1, sgAdcReadRaw, &sgAdcRead_mV );
                printf("Pot1 Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                AdcReadRaw( ADC_PIN_POT2, 1, &sgAdcReadRaw );
                AdcRawToMilliVolts( ADC_PIN_POT2, sgAdcReadRaw, &sgAdcRead_mV );
                printf("Pot2 Adc: raw=%d mVolt=%d\r\n", (int)sgAdcReadRaw, (int)sgAdcRead_mV );

                printf("\r\n");
            }

            dwDelay = 1000000;
            while( dwDelay-- ){}
            dwDelay = 1000000;
            while( dwDelay-- ){}
        }
    }
}

///////////////////////////////////////// END OF CODE //////////////////////////////////////////
