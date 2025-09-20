//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \file       Command.c
//! \brief      Command Library for the system
//!
//! \author     Joel Minski [jminski@puracom.com], Puracom Inc.
//! \author     Craig Stickel [cstickel@puracom.com], Puracom Inc.
//! \date       March 5, 2010
//!
//! \details    Generates and registers the command library for the system, implementing the
//!             various handlers when invoked by the console task.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// RTOS Library includes
#include "FreeRTOS.h"

#include "Types.h"              // Timer depends on Types
#include "Target.h"

// PCOM General Library includes
#include "Timer.h"              // task depends on Timer for vApplicationTickHook

// PCOM Project Targets
// NOT REQUIRED!!

// PCOM Library includes
// low level drivers
#include "System.h"
#include "Watchdog.h"
#include "Adc.h"
#include "Backup.h"
#include "Bluetooth.h"
#include "General.h"
#include "Gpio.h"
#include "Dataflash.h"
#include "DFFaddress.h"
#include "DFFile.h"
//#include "Modem.h"
#include "ModemFtpq.h"
#include "Rtc.h"
#include "Spi.h"
#include "Usart.h"

#include "TaskDemo.h"

// high level application
#include "Command.h"
#include "Config.h"
#include "Console.h"
#include "Control.h"
#include "Datalog.h"

#include "main.h"
#include "../Bootloader/Bootloader.h"
#include "Loader.h"
#include "BuildInfo.h"

#include "InterfaceBoard.h"
#include "PortEx.h"
#include "AdcExternal.h"
#include "UserIf.h"
//#include "ModCamera.h"
#include "Camera.h"
#include "ControlScriptRun.h"
#include "ControlAdcReading.h"

#include "Modem/ModemData.h"
#include "Modem/ModemCommandResponse/Modem.h"
#include "Modem/ModemCommandResponse/ModemCommand.h"
#include "Modem/ModemCommandResponse/ModemResponse.h"
#include "Modem/ModemOperations/ModemPower.h"
#include "Modem/ModemOperations/ModemConfig.h"
#include "Modem/ModemOperations/ModemInfo.h"
#include "Modem/ModemOperations/ModemSim.h"
#include "Modem/ModemOperations/ModemRegStat.h"
#include "Modem/ModemOperations/ModemConnect.h"
#include "Modem/ModemOperations/ModemCgRegStat.h"
#include "Modem/ModemOperations/ModemSetSysClock.h"
#include "Modem/ModemOperations/ModemCellInfo.h"
#include "Modem/ModemOperations/ModemSignal.h"
#include "Modem/ModemOperations/ModemFtp.h"
#include "Modem/ModemOperations/ModemFtpOper.h"
#include "Modem/ModemConnAdv.h"

#include "DevInfo.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

//#define COMMAND_ESC_KEY     (0x1B)

typedef struct
{
    ConsoleDictionary * const ptDictionary;
}CommandDictionaryColectionType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

static ConsoleResultEnum    CommandHandlerHelp          ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerAdc           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerBackup        ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerBluetooth     ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerBootloader    ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerConfig        ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerDev           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerFile          ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerGpio          ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerFtp           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerRtc           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerSleep         ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerSpi           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerUsart         ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerInterfaceBoard( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerCam           ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerAdcExt        ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerPortExp       ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerUserInterf    ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerSystem        ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerComment       ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerScript        ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerModem         ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerAdcSmpl       ( ConsoleCommandType *ptCommand );
static ConsoleResultEnum    CommandHandlerMenuDelimiter ( ConsoleCommandType *ptCommand );


//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------
// DICTIONARY TABLES
//----------------------------------------------------------------------------------------
// DICTIONARY TABLE 1
static const ConsoleDictionary    gtCommandsUser[] =
{
    // NOTE: HELP command always at the beginning to let HELP ALL command work correctly!!
    {   "HELP",         CommandHandlerHelp,             "Command Help Information"      },
    {   "-",            CommandHandlerMenuDelimiter,    ""           },
    {   "SYS",          CommandHandlerSystem,           "System Commands"               },    
    {   "FILE",         CommandHandlerFile,             "File Commands"                 },
    {   "MDM",          CommandHandlerModem,            "Modem driver"                  },
    {   "CAM",          CommandHandlerCam,              "Camera Commands"               },
    {   "ADCSMPL",      CommandHandlerAdcSmpl,          "Adc sampling Commands"         },
    {   "BLUETOOTH",    CommandHandlerBluetooth,        "Bluetooth Commands"            },
    {   "-",            CommandHandlerMenuDelimiter,    ""           },
    {   "DEV",          CommandHandlerDev,              "Device Commands"               },
    {   "RTC",          CommandHandlerRtc,              "Real Time Clock Commands"      },
    {   "GPIO",         CommandHandlerGpio,             "GPIO Commands"                 },
    {   "ADC",          CommandHandlerAdc,              "ADC Commands"                  },
    {   "USART",        CommandHandlerUsart,            "Usart Commands"                },
    {   "SPI",          CommandHandlerSpi,              "SPI Commands"                  },
    {   "BACKUP",       CommandHandlerBackup,           "Backup SRam Commands"          },
    {   "SLEEP",        CommandHandlerSleep,            "Sleep Commands"                },
    {   "-",            CommandHandlerMenuDelimiter,    ""           },
    {   "CONFIG",       CommandHandlerConfig,           "Configuration Commands"        },
    {   "INTB",         CommandHandlerInterfaceBoard,   "Interface board Commands"      },
    {   "ADCEX",        CommandHandlerAdcExt,           "ADCEX Commands"                },
    {   "PORTEX",       CommandHandlerPortExp,          "PORTEX Commands"               },
    {   "FTP",          CommandHandlerFtp,              "Ftp Commands"                  },
    {   "BOOTLOADER",   CommandHandlerBootloader,       "Bootloader Commands"           },
    //{   "DF",   CommandHandlerDataflash,       "dataflash Commands"           },
    {   "UI",           CommandHandlerUserInterf,       "User Interface Commands"       },
    {   "SCRIPT",       CommandHandlerScript,           "Script Commands"               },
    {   "#",            CommandHandlerComment,          "Simbol for Comments"           },

    {   NULL,           NULL,                           NULL                            }
};
// ADD HERE MORE DICTIONARIES
//----------------------------------------------------------------------------------------

// DICTIONARY COLLECTION POINTER ARRAY
static CommandDictionaryColectionType gtDictionaryCollection[] =
{
    (ConsoleDictionary * const) &gtCommandsUser[0],
    // ADD MORE DICTIONARIES HERE IF YOU WANT
    // TO MAKE THE POINTER ACCESSIBLE WITH THE FUNCTION CommandGetDictionaryPointer
    // CommandGetDictionaryPointer();
};

#define COMMAND_NUMBER_OF_DICTIONARIES  ( sizeof(gtDictionaryCollection) / sizeof(gtDictionaryCollection[0] ) )

// Strings that gets repeated constantly. Set as const so it can be stored in FLASH instead of RAM
static const CHAR *const COMMAND_STRING__STAT   = "STAT";
static const CHAR *const COMMAND_STRING__HELP   = "HELP";

///////////////////////////////// FUNCTION DESCRIPTION /////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     const ConsoleDictionary * CommandGetDictionaryPointer( CommandDictionaryEnum eDictionary )
//!
//! \brief  returns the pointer of the specified dictionary
//!
//! \return pointer
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
const ConsoleDictionary * CommandGetDictionaryPointer( CommandDictionaryEnum eDictionary )
{
    ConsoleDictionary * ptDictionary = NULL;

    if( eDictionary < COMMAND_NUMBER_OF_DICTIONARIES )
    {
        ptDictionary = gtDictionaryCollection[eDictionary].ptDictionary;
    }

    return ptDictionary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerComment( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerComment( ConsoleCommandType *ptCommand )
{    
    return CONSOLE_RESULT_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerMenuDelimiter( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerMenuDelimiter( ConsoleCommandType *ptCommand )
{    
    return CONSOLE_RESULT_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerScript( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerScript( ConsoleCommandType *ptCommand )
{    
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if
    ( 
        ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) 
    )
    {
        ConsolePrintf( ptCommand->eConsolePort, "SCRIPT Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "SCRIPT DELAY <milliseconds>\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SCRIPT LOG <BOOL>\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "DELAY" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            UINT32 dwDelay_mSec = atoi( ptCommand->pcArgArray[ 2 ] );

            ControlScriptRunStartDelay( dwDelay_mSec );
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "LOG" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            BOOL fLogCommandResultToFileEnable = atoi( ptCommand->pcArgArray[ 2 ] );

            ControlScriptRunCommandResultToFileEnable( fLogCommandResultToFileEnable );
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerAdcSmpl( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerAdcSmpl( ConsoleCommandType *ptCommand )
{    
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if
    ( 
        ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) 
    )
    {
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL RUN - starts sampling\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL REQ SHORT - request short sampling\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL REQ LONG - request long sampling\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL REQ HOUR - request Hourly average calc\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL SHORT INFO\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL SHORT CHAN ENABLE <CHANNEL#> <BOOL>\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL LONG INFO\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL LONG CHAN ENABLE <CHANNEL#> <BOOL>\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCSMPL LONG CONFIG <TRIGGER_MINUTES> <ADC_READINGS_P/SAMPLE> <SAMPLE_PERIOD_MSEC> <RECORDING_TIME_LEN>\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        ControlAdcReadingChannelConfigStruct    tChannelConfig;

        for( AdcInputMapEnum eChannel = 0 ; eChannel < ADC_INPUT_EXTERNAL_MAX ; eChannel++ )
        {
            ControlAdcReadingGetChannelConfig( eChannel, &tChannelConfig );

            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Short Sampling Enabled: %d\r\n", eChannel, tChannelConfig.fIsShortSamplingEnabled );
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Long Sampling Enabled: %d\r\n", eChannel, tChannelConfig.fIsLongSamplingEnabled );
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Power mode: %d\r\n",             eChannel, tChannelConfig.tSensorConfig.ePowerMode );
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Delta sampling Enabled: %d\r\n", eChannel, tChannelConfig.tSensorConfig.fIsDeltaSamplingEnabled );            
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Is correction  Enabled: %d\r\n", eChannel, tChannelConfig.tSensorConfig.tSampleCorrection.fIsEnable );
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Offset: %f\r\n", eChannel, tChannelConfig.tSensorConfig.tSampleCorrection.sgOffset );
            ConsolePrintf( ptCommand->eConsolePort, "Chan%d Slope: %f\r\n", eChannel, tChannelConfig.tSensorConfig.tSampleCorrection.sgSlope );
            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "RUN" ) == TRUE )
    {
        ControlAdcReadingSamplingStart();
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "REQ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SHORT" ) == TRUE )
            {
                ControlAdcReadingSamplingShortRequested();                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "LONG" ) == TRUE )
            {
                ControlAdcReadingSamplingLongRequested();                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "HOUR" ) == TRUE )
            {
                ControlAdcReadingCalculateHourly();
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }       
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    // ADCSMPL SHORT INFO
    // ADCSMPL SHORT CHAN ENABLE <CHANNEL#> <BOOL>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SHORT" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 2 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "INFO" ) == TRUE )
            {
                ControlAdcReadingChannelConfigStruct    tChannelConfig;

                for( AdcInputMapEnum eChannel = 0 ; eChannel < ADC_INPUT_EXTERNAL_MAX ; eChannel++ )
                {
                    ControlAdcReadingGetChannelConfig( eChannel, &tChannelConfig );        
                    // check if short sampling channel is enabled
                    ConsolePrintf( ptCommand->eConsolePort, "Chan%d Short Sampling Enabled: %d\r\n", eChannel, tChannelConfig.fIsShortSamplingEnabled );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CHAN" ) == TRUE )
            {
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "ENABLE" ) == TRUE )
                {
                    UINT8   bChannel;
                    UINT8   bChannelBitMask;
                    BOOL    fEnabled;                    

                    bChannel = atoi( ptCommand->pcArgArray[ 4 ] );
                    fEnabled = atoi( ptCommand->pcArgArray[ 5 ] );

                    if( bChannel < ADC_INPUT_EXTERNAL_MAX )
                    {
                        // get the current bit status
                        ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_SHORT_ENABLED_BIT_MASK, &bChannelBitMask, sizeof(bChannelBitMask) );
                        // set or clear the bit
                        if( fEnabled )
                        {
                            // if set...OR  (original | xxx1)
                            bChannelBitMask |= (1<<bChannel);
                        }
                        else
                        {
                            // if clear...AND  (original & xxx0)
                            bChannelBitMask &= ~(1<<bChannel);   
                        }

                        // clear bits that doesn't belong to any channel
                        bChannelBitMask &= 0x3F;

                        // save new value
                        ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_SHORT_ENABLED_BIT_MASK, &bChannelBitMask );

                        ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }    
    // ADCSMPL LONG CHAN ENABLE <CHANNEL#> <BOOL>
    // ADCSMPL LONG CONFIG <ADC_READINGS_P/SAMPLE> <SAMPLE_PERIOD_MSEC> <RECORDING_TIME_LEN>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "LONG" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "INFO" ) == TRUE )
            {
                // print long sampling configuration  
                UINT16 wAdcSamplingLongTriggerMinutes;
                UINT16 wAdcSamplingLongAdcReadingsPerSample;
                UINT16 wAdcSamplingLongSamplePeriodMillisecond;
                UINT16 wAdcSamplingLongRecordingTimeSeconds;
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_TRIGGER_MINUTES, &wAdcSamplingLongTriggerMinutes, sizeof(wAdcSamplingLongTriggerMinutes) );
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_ADC_READINGS_PER_SMPL, &wAdcSamplingLongAdcReadingsPerSample, sizeof(wAdcSamplingLongAdcReadingsPerSample) );
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_SMPL_PERIOD_MILLISEC, &wAdcSamplingLongSamplePeriodMillisecond, sizeof(wAdcSamplingLongSamplePeriodMillisecond) );
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_RECORDING_TIME_SECONDS, &wAdcSamplingLongRecordingTimeSeconds, sizeof(wAdcSamplingLongRecordingTimeSeconds) );
                
                ConsolePrintf( ptCommand->eConsolePort, "Long Sampling Trigger minutes: %d\r\n", wAdcSamplingLongTriggerMinutes );
                ConsolePrintf( ptCommand->eConsolePort, "Long Sampling Adc readings per sample: %d\r\n", wAdcSamplingLongAdcReadingsPerSample );
                ConsolePrintf( ptCommand->eConsolePort, "Long Sampling period milliseconds: %d\r\n", wAdcSamplingLongSamplePeriodMillisecond );
                ConsolePrintf( ptCommand->eConsolePort, "Long Sampling total recording seconds: %d\r\n", wAdcSamplingLongRecordingTimeSeconds );



                ControlAdcReadingChannelConfigStruct    tChannelConfig;

                for( AdcInputMapEnum eChannel = 0 ; eChannel < ADC_INPUT_EXTERNAL_MAX ; eChannel++ )
                {
                    ControlAdcReadingGetChannelConfig( eChannel, &tChannelConfig );        
                    // check if short sampling channel is enabled
                    ConsolePrintf( ptCommand->eConsolePort, "Chan%d Long Sampling Enabled: %d\r\n", eChannel, tChannelConfig.fIsLongSamplingEnabled );
                }                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CHAN" ) == TRUE )
            {
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "ENABLE" ) == TRUE )
                {
                    UINT8   bChannel;
                    UINT8   bChannelBitMask;
                    BOOL    fEnabled;                    

                    bChannel = atoi( ptCommand->pcArgArray[ 4 ] );
                    fEnabled = atoi( ptCommand->pcArgArray[ 5 ] );

                    if( bChannel < ADC_INPUT_EXTERNAL_MAX )
                    {
                        // get the current bit status
                        ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_ENABLED_BIT_MASK, &bChannelBitMask, sizeof(bChannelBitMask) );
                        // set or clear the bit
                        if( fEnabled )
                        {                            
                            // if set...OR  (original | xxx1)
                            bChannelBitMask |= (1<<bChannel);
                        }
                        else
                        {
                            // if clear...AND  (original & xxx0)
                            bChannelBitMask &= ~(1<<bChannel);   
                        }

                        // clear bits that doesn't belong to any channel
                        bChannelBitMask &= 0x3F;

                        // save new value
                        ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_ENABLED_BIT_MASK, &bChannelBitMask );

                        ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CONFIG" ) == TRUE )
            {                
                UINT16 wAdcSamplingLongTriggerMinutes;
                UINT16 wNumberOfAdcReadingsPerRegularSample;
                UINT16 wSamplePeriod_mSec;
                UINT16 wRecordingTimeLenSeconds;

                wAdcSamplingLongTriggerMinutes      = atoi( ptCommand->pcArgArray[ 3 ] );
                wNumberOfAdcReadingsPerRegularSample= atoi( ptCommand->pcArgArray[ 4 ] );
                wSamplePeriod_mSec                  = atoi( ptCommand->pcArgArray[ 5 ] );
                wRecordingTimeLenSeconds            = atoi( ptCommand->pcArgArray[ 6 ] );            

                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_TRIGGER_MINUTES, &wAdcSamplingLongTriggerMinutes );
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_ADC_READINGS_PER_SMPL, &wNumberOfAdcReadingsPerRegularSample );
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_SMPL_PERIOD_MILLISEC, &wSamplePeriod_mSec );
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_SAMPLING_LONG_RECORDING_TIME_SECONDS, &wRecordingTimeLenSeconds );
                ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );                
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

ConsoleResultEnum CommandHandlerModem( ConsoleCommandType *ptCommand )
{    
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if
    ( 
        ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) 
    )
    {
        ConsolePrintf( ptCommand->eConsolePort, "MDM Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "MDM STAT\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM PRNT <STAT|RXTX|CNSL|DBG> - Enable Disable printing\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CASE1 - Only Ok expected (general response)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CASE2 - Ok and Keyword expected \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CASE3 - Only Keyword expected multiple times\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CASE4 - Only OK expected when keyword showing multiple times\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM PWR <STAT|ON|OFF> - \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "MDM CFG <STAT|RUN> - \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "MDM INF <STAT|RUN> - \r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "MDM SIM <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CRE <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CGR <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CON <STAT|ON|OFF> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CON CONFIG APN LIST - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CON CONFIG APN SET <ApnString>- \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CON CONFIG APN SETFROMLIST <ApnListNumber>- \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CEL <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM SIG <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CLK <STAT|RUN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FTP <STAT|FTP_TYPE> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                   Ftp types: - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                   * PUT - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                   * GET - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                   * DEL - \r\n" );
        
        ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT NAME <Name> <fSize> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT DATA <data> <bytes> - \r\n" );
        //ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT SHOW - show data uploaded \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FGET NAME <Name> <alloc> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FGET DATA - request data \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FGET SHOW - show data downloaded \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM FDEL NAME <Name> - \r\n" );        

        ConsolePrintf( ptCommand->eConsolePort, "MDM CONNAUTO <STAT|RUN|EN> - \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "MDM CONSOLE - \r\n" );
    }    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        ConsolePrintf( ptCommand->eConsolePort, "MODEM ADVANCE CONNECT:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Is connect advance enabled = %d\r\n", ModemConnAdvIsEnabled() );        
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "PRINT CONFIG:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Rx En   = %d\r\n", ModemConsolePrintRxIsEnabled() );
        ConsolePrintf( ptCommand->eConsolePort, "Tx En   = %d\r\n", ModemConsolePrintTxIsEnabled() );
        ConsolePrintf( ptCommand->eConsolePort, "Cnsl En = %d\r\n", ModemConsolePrintfIsEnabled() );
        ConsolePrintf( ptCommand->eConsolePort, "Dbg En  = %d\r\n", ModemConsolePrintDbgIsEnabled() );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "NETWORK CONFIG:\r\n" );
        ModemDataType * ptModemData = NULL;
        ptModemData = ModemResponseModemDataGetPtr();
        ConsolePrintf( ptCommand->eConsolePort, "Network Apn        =\"%s\"\r\n", ptModemData->tModemConnStat.tPdpConfig.tConfig.szApn );
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Server Address =\"%s\"\r\n", ptModemData->tFtpServerConn.szAddress );
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Server Port    =%d\r\n", ptModemData->tFtpServerConn.dwPort );
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Server User    =\"%s\"\r\n", ptModemData->tFtpServerConn.szUserName );
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Server Password=\"%s\"\r\n", ptModemData->tFtpServerConn.szPassword );                
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "MODEM INFO:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Is Info Extracted = %d\r\n", ModemInfoIsExtracted() );                
        ModemDataModemInfoType  * ptModemInfo = NULL;
        ptModemInfo = ModemGetModemInfoPtr();
        if( NULL != ptModemInfo )
        {
            ConsolePrintf( ptCommand->eConsolePort, "Manuf  = %s\r\n", ptModemInfo->szManufacturerName );
            ConsolePrintf( ptCommand->eConsolePort, "model  = %s\r\n", ptModemInfo->szModem );
            ConsolePrintf( ptCommand->eConsolePort, "fw     = %s\r\n", ptModemInfo->szFirmwareVer );
            ConsolePrintf( ptCommand->eConsolePort, "Imei   = %s\r\n", ptModemInfo->szImei );
        }
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "MODEM OTHERS:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Pwr Is On = %d\r\n", ModemPowerIsPowerEnabled() );
        ConsolePrintf( ptCommand->eConsolePort, "Cnf Is Ready = %d\r\n", ModemConfigIsSuccess() );
        ConsolePrintf( ptCommand->eConsolePort, "Sim Is Ready = %d\r\n", ModemSimIsReady() );                
        ModemDataSimCardType  * ptModemSim = NULL;
        ptModemSim = ModemSimGetInfoPtr();
        if( NULL != ptModemSim )
        {                    
            ConsolePrintf( ptCommand->eConsolePort, "Sim CIMI = %s\r\n", ptModemSim->szCimei );
            ConsolePrintf( ptCommand->eConsolePort, "Sim CCID = %s\r\n", ptModemSim->szIccId );
            ConsolePrintf( ptCommand->eConsolePort, "Sim Last CCID = %s\r\n", ptModemSim->szLastIccId );
        }        
        ModemDataNetworkType  * ptModemNetwork = NULL;
        ptModemNetwork = ModemConnectGetInfoPtr();
        if( NULL != ptModemNetwork )
        {            
            ConsolePrintf( ptCommand->eConsolePort, "Con Socket ID = %d\r\n", ptModemNetwork->bSocketConnId );
            ConsolePrintf( ptCommand->eConsolePort, "Con Socket cntx ID = %d\r\n", ptModemNetwork->bPdpCntxtId );
            ConsolePrintf( ptCommand->eConsolePort, "Con Socket stat= %d\r\n", ptModemNetwork->bSocketStat );
            ConsolePrintf( ptCommand->eConsolePort, "Con Socket cntx stat= %d\r\n", ptModemNetwork->bSocketCntxtStat );
            ConsolePrintf( ptCommand->eConsolePort, "Con creg = %d\r\n", ptModemNetwork->bCregStat );
            ConsolePrintf( ptCommand->eConsolePort, "Con Ip assigned = %s\r\n", ptModemNetwork->szIp );
            ConsolePrintf( ptCommand->eConsolePort, "Con Is Connected = %d\r\n", ModemConnectIsConnected() );
        }
        ConsolePrintf( ptCommand->eConsolePort, "Cel Is Info Ready = %d\r\n", ModemCellInfoIsReady() );                
        ModemDataCellInfoType  * ptModemCellInfo = NULL;
        ptModemCellInfo = ModemCellInfoGetInfoPtr();
        if( NULL != ptModemCellInfo )
        {
            ConsolePrintf( ptCommand->eConsolePort, "Cel Operator = %s\r\n", ptModemCellInfo->szOperatorName );            
            ConsolePrintf( ptCommand->eConsolePort, "Cel Lac = %s\r\n", ptModemCellInfo->szLac );
            ConsolePrintf( ptCommand->eConsolePort, "Cel Id = %s\r\n", ptModemCellInfo->szCellId );
        }

        ConsolePrintf( ptCommand->eConsolePort, "Sig Is Ready = %d\r\n", ModemSignalIsSignalAcquired() );                
        ModemDataSignalInfoType  * ptModemSignal = NULL;
        ptModemSignal = ModemSignalGetDataPtr();
        if( NULL != ptModemSignal )
        {
            ConsolePrintf( ptCommand->eConsolePort, "Sig Rssi=%d(dBm)\r\n", ptModemSignal->idwRssi_dBm);
            ConsolePrintf( ptCommand->eConsolePort, "Sig Ber=%6.3f(Percent)\r\n", ptModemSignal->sgBer_Percent );
        }

        ConsolePrintf( ptCommand->eConsolePort, "Ftp Is Connected = %d\r\n", ModemFtpConnectIsConnected() );
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Type =" );               
        switch( ModemFtpConnectAllowedOperation() )
        {
            case MODEM_FTP_OPERATION_TYPE_NONE:
                ConsolePrintf( ptCommand->eConsolePort, "None\r\n" );
                break;
            case MODEM_FTP_OPERATION_TYPE_PUT:
                ConsolePrintf( ptCommand->eConsolePort, "Put\r\n" );
                break;
            case MODEM_FTP_OPERATION_TYPE_GET:
                ConsolePrintf( ptCommand->eConsolePort, "Get\r\n" );
                break;
            case MODEM_FTP_OPERATION_TYPE_DELETE:
                ConsolePrintf( ptCommand->eConsolePort, "Delete\r\n" );
                break;
            default:
                ConsolePrintf( ptCommand->eConsolePort, "Unknown\r\n" );
                break;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PRNT" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Rx En = %d\r\n", ModemConsolePrintRxIsEnabled() );
                ConsolePrintf( ptCommand->eConsolePort, "Tx En = %d\r\n", ModemConsolePrintTxIsEnabled() );
                ConsolePrintf( ptCommand->eConsolePort, "Cnsl En = %d\r\n", ModemConsolePrintfIsEnabled() );                
                ConsolePrintf( ptCommand->eConsolePort, "Dbg En = %d\r\n", ModemConsolePrintDbgIsEnabled() );                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RXTX" ) )
            {
                // toggles
                if( ModemConsolePrintRxIsEnabled() )
                {
                    ModemConsolePrintRxEnable( FALSE );
                    ModemConsolePrintTxEnable( FALSE );
                }
                else
                {
                    ModemConsolePrintRxEnable( TRUE );
                    ModemConsolePrintTxEnable( TRUE );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CNSL" ) )
            {
                // toggles
                if( ModemConsolePrintfIsEnabled() )
                {
                    ModemConsolePrintfEnable( FALSE );                    
                }
                else
                {
                    ModemConsolePrintfEnable( TRUE );                    
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "DBG" ) )
            {
                // toggles
                if( ModemConsolePrintDbgIsEnabled() )
                {
                    ModemConsolePrintDbgEnable( FALSE );                    
                }
                else
                {
                    ModemConsolePrintDbgEnable( TRUE );                    
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CASE1" ) == TRUE )
    {
        ModemStateMachineType       tStateMachine;
        ModemCommandSemaphoreEnum   eSemaphore;
        CHAR                        cChar;
        BOOL                        fTerminateTest = FALSE;
        
        StateMachineInit( &tStateMachine.tState );        

        while(1)
        {
            StateMachineUpdate( &tStateMachine.tState );

            switch( tStateMachine.tState.bStateCurrent )
            {
                case 0: // start                    
                    ModemConsolePrintRxEnable( TRUE );
                    ModemConsolePrintTxEnable( TRUE );
                    StateMachineChangeState( &tStateMachine.tState, 1 ); // always at the end of case statement when possible
                    break;
                
                case 1: // running
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    if( StateMachineIsFirtEntry( &tStateMachine.tState ) )
                    {                        
                        if( ModemCommandProcessorReserve( &eSemaphore ) )
                        {
                            ModemCommandProcessorResetResponse();
                            ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                            
                            // if need to wait for response set time out
                            StateMachineSetTimeOut( &tStateMachine.tState, 1000 );
                            
                            ModemRxProcessKeyAutoUpdateEnable(FALSE); // indicate UsartGetNB() will be used here
                            ModemCommandProcessorSendAtCommand( "+GMI" );
                        }
                        else
                        {
                            // Set ERROR
                            ConsolePrintf( ptCommand->eConsolePort, "Error\r\n" );

                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        }                        
                    }

                    if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
                    {
                        ModemRxProcessKeyManualUpdate( cChar );
                        if( ModemCommandProcessorIsResponseComplete() )
                        {
                            if( ModemCommandProcessorIsError() )
                            {                                
                                StateMachineChangeState( &tStateMachine.tState, 3 ); 
                            }
                            else
                            {
                                StateMachineChangeState( &tStateMachine.tState, 2 ); 
                            }
                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            ModemRxProcessKeyAutoUpdateEnable(TRUE);
                            break;
                        }
                    }

                    if( StateMachineIsTimeOut( &tStateMachine.tState ) ) 
                    {
                        // Set ERROR
                        ConsolePrintf( ptCommand->eConsolePort, "timeout\r\n" );

                        // before change state always release semaphore
                        ModemCommandProcessorRelease( &eSemaphore );                        
                        StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        ModemRxProcessKeyAutoUpdateEnable(TRUE);
                    }
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    break;

                case 2: // pass
                    ConsolePrintf( ptCommand->eConsolePort, "PASS\r\n" );
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;
                case 3: // fail
                    ConsolePrintf( ptCommand->eConsolePort, "FAIL\r\n" );                    
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;

                case 4: // end
                    fTerminateTest = TRUE;
                    break;
                                        
                default:
                    StateMachineChangeState( &tStateMachine.tState, 4 ); // goto fail
                    break;
            }

            if( fTerminateTest )
            {
                break; // for while loop
            }

            WatchdogKickSoftwareWatchdog( 10000 );
        }
    }
    // ok and keyword expected
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CASE2" ) == TRUE )
    {
        ModemStateMachineType       tStateMachine;
        ModemCommandSemaphoreEnum   eSemaphore;
        CHAR                        cChar;
        BOOL                        fTerminateTest = FALSE;
        
        StateMachineInit( &tStateMachine.tState );        

        while(1)
        {        
            StateMachineUpdate( &tStateMachine.tState );

            switch( tStateMachine.tState.bStateCurrent )
            {
                case 0: // start                    
                    ModemConsolePrintRxEnable( TRUE );
                    ModemConsolePrintTxEnable( TRUE );
                    StateMachineChangeState( &tStateMachine.tState, 1 ); // always at the end of case statement when possible
                    break;
                
                case 1: // running
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    if( StateMachineIsFirtEntry( &tStateMachine.tState ) )
                    {                        
                        if( ModemCommandProcessorReserve( &eSemaphore ) )
                        {
                            ModemCommandProcessorResetResponse();
                            ModemCommandProcessorSetExpectedResponse( TRUE, "#CGMI:", 1, TRUE );
                            
                            ModemRxProcessKeyAutoUpdateEnable(FALSE); // indicate UsartGetNB() will be used here
                            // if need to wait for response set time out
                            StateMachineSetTimeOut( &tStateMachine.tState, 1000 );
                            ModemCommandProcessorSendAtCommand( "#CGMI" );
                        }
                        else
                        {
                            // Set ERROR
                            ConsolePrintf( ptCommand->eConsolePort, "Error\r\n" );

                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        }                        
                    }

                    if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
                    {
                        ModemRxProcessKeyManualUpdate( cChar );
                        if( ModemCommandProcessorIsResponseComplete() )
                        {
                            if( ModemCommandProcessorIsError() )
                            {
                                StateMachineChangeState( &tStateMachine.tState, 3 ); 
                            }
                            else
                            {
                                // print response                            
                                ConsolePrintf( ptCommand->eConsolePort, "Response:[%s]\r\n", ModemResponseModemDataGetPtr()->tModemInfo.szManufacturerName );                            
                                StateMachineChangeState( &tStateMachine.tState, 2 ); // goto print result
                            }
                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            ModemRxProcessKeyAutoUpdateEnable(TRUE);
                            break;
                        }
                    }

                    if( StateMachineIsTimeOut( &tStateMachine.tState ) ) 
                    {
                        // Set ERROR
                        ConsolePrintf( ptCommand->eConsolePort, "timeout\r\n" );

                        // before change state always release semaphore
                        ModemCommandProcessorRelease( &eSemaphore );
                        StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        ModemRxProcessKeyAutoUpdateEnable(TRUE);
                    }
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    break;

                case 2: // pass
                    ConsolePrintf( ptCommand->eConsolePort, "PASS\r\n" );
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;
                case 3: // fail
                    ConsolePrintf( ptCommand->eConsolePort, "FAIL\r\n" );                    
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;

                case 4: // end
                    fTerminateTest = TRUE;
                    break;
                                        
                default:
                    StateMachineChangeState( &tStateMachine.tState, 4 ); // goto fail
                    break;
            }

            if( fTerminateTest )
            {
                break; // for while loop
            }

            WatchdogKickSoftwareWatchdog( 10000 );
        }
    }
    // only keyword expected multiple times
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CASE3" ) == TRUE )
    {
        ModemStateMachineType       tStateMachine;
        ModemCommandSemaphoreEnum   eSemaphore;
        CHAR                        cChar;
        BOOL                        fTerminateTest = FALSE;
        
        StateMachineInit( &tStateMachine.tState );        

        while(1)
        {        
            StateMachineUpdate( &tStateMachine.tState );

            switch( tStateMachine.tState.bStateCurrent )
            {
                case 0: // start                    
                    ModemConsolePrintRxEnable( TRUE );
                    ModemConsolePrintTxEnable( TRUE );
                    StateMachineChangeState( &tStateMachine.tState, 1 ); // always at the end of case statement when possible
                    break;
                
                case 1: // running
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    if( StateMachineIsFirtEntry( &tStateMachine.tState ) )
                    {
                        if( ModemCommandProcessorReserve( &eSemaphore ) )
                        {
                            ModemCommandProcessorResetResponse();
                            ModemCommandProcessorSetExpectedResponse( TRUE, "#V24CFG:", 6, TRUE );                            
                            
                            ModemRxProcessKeyAutoUpdateEnable(FALSE); // indicate UsartGetNB() will be used here
                            // if need to wait for response set time out
                            StateMachineSetTimeOut( &tStateMachine.tState, 1000 );
                            ModemCommandProcessorSendAtCommand( "#V24CFG?" );
                        }
                        else
                        {
                            // Set ERROR
                            ConsolePrintf( ptCommand->eConsolePort, "Error\r\n" );

                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        }                        
                    }

                    if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
                    {
                        ModemRxProcessKeyManualUpdate( cChar );
                        if( ModemCommandProcessorIsResponseComplete() )
                        {
                            if( ModemCommandProcessorIsError() )
                            {
                                StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                            }
                            else
                            {
                                // print response
                                //ConsolePrintf( ptCommand->eConsolePort, "Response:[%s]\r\n", ModemCommandGetResponseBufferPtr() );                            
                                StateMachineChangeState( &tStateMachine.tState, 2 ); // goto print result
                            }
                            // before change state always release semaphore
                            ModemRxProcessKeyAutoUpdateEnable(TRUE); // indicate UsartGetNB() will be used here
                            ModemCommandProcessorRelease( &eSemaphore );
                            break;
                        }
                    }

                    if( StateMachineIsTimeOut( &tStateMachine.tState ) ) 
                    {
                        // Set ERROR
                        ConsolePrintf( ptCommand->eConsolePort, "timeout\r\n" );

                        // before change state always release semaphore
                        ModemCommandProcessorRelease( &eSemaphore );
                        StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        ModemRxProcessKeyAutoUpdateEnable(TRUE); // indicate UsartGetNB() will be used here
                    }
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    break;

                case 2: // pass
                    ConsolePrintf( ptCommand->eConsolePort, "PASS\r\n" );
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;
                case 3: // fail
                    ConsolePrintf( ptCommand->eConsolePort, "FAIL\r\n" );                    
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;

                case 4: // end
                    fTerminateTest = TRUE;
                    break;
                                        
                default:
                    StateMachineChangeState( &tStateMachine.tState, 4 ); // goto fail
                    break;
            }

            if( fTerminateTest )
            {
                break; // for while loop
            }

            WatchdogKickSoftwareWatchdog( 10000 );
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CASE4" ) == TRUE )
    {
        ModemStateMachineType       tStateMachine;
        ModemCommandSemaphoreEnum   eSemaphore;
        CHAR                        cChar;
        BOOL                        fTerminateTest = FALSE;
        
        StateMachineInit( &tStateMachine.tState );        

        while(1)
        {        
            StateMachineUpdate( &tStateMachine.tState );

            switch( tStateMachine.tState.bStateCurrent )
            {
                case 0: // start                    
                    ModemConsolePrintRxEnable( TRUE );
                    ModemConsolePrintTxEnable( TRUE );
                    StateMachineChangeState( &tStateMachine.tState, 1 ); // always at the end of case statement when possible
                    break;
                
                case 1: // running
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    if( StateMachineIsFirtEntry( &tStateMachine.tState ) )
                    {
                        if( ModemCommandProcessorReserve( &eSemaphore ) )
                        {
                            ModemCommandProcessorResetResponse();
                            ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );                            
                            
                            ModemRxProcessKeyAutoUpdateEnable(FALSE); // indicate UsartGetNB() will be used here
                            // if need to wait for response set time out
                            StateMachineSetTimeOut( &tStateMachine.tState, 3000 );
                            ModemCommandProcessorSendAtCommand( "#V24CFG?" );
                        }
                        else
                        {
                            // Set ERROR
                            ConsolePrintf( ptCommand->eConsolePort, "Error\r\n" );

                            // before change state always release semaphore
                            ModemCommandProcessorRelease( &eSemaphore );
                            StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        }                        
                    }

                    if( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &cChar ) )
                    {
                        ModemRxProcessKeyManualUpdate( cChar );

                        if( ModemCommandProcessorIsResponseComplete() )
                        {
                            if( ModemCommandProcessorIsError() )
                            {
                                StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                            }
                            else
                            {                                
                                StateMachineChangeState( &tStateMachine.tState, 2 ); // goto print result
                            }
                            // before change state always release semaphore
                            ModemRxProcessKeyAutoUpdateEnable(TRUE); // indicate UsartGetNB() will be used here
                            ModemCommandProcessorRelease( &eSemaphore );
                            break;
                        }
                    }

                    if( StateMachineIsTimeOut( &tStateMachine.tState ) ) 
                    {
                        // Set ERROR
                        ConsolePrintf( ptCommand->eConsolePort, "timeout\r\n" );

                        // before change state always release semaphore
                        ModemCommandProcessorRelease( &eSemaphore );
                        StateMachineChangeState( &tStateMachine.tState, 3 ); // goto print result
                        ModemRxProcessKeyAutoUpdateEnable(TRUE); // indicate UsartGetNB() will be used here
                    }
                    //////////////////////////////////////////////////////////////////
                    //////////////////////////////////////////////////////////////////
                    break;

                case 2: // pass
                    ConsolePrintf( ptCommand->eConsolePort, "PASS\r\n" );
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;
                case 3: // fail
                    ConsolePrintf( ptCommand->eConsolePort, "FAIL\r\n" );                    
                    StateMachineChangeState( &tStateMachine.tState, 4 );
                    break;

                case 4: // end
                    fTerminateTest = TRUE;
                    break;
                                        
                default:
                    StateMachineChangeState( &tStateMachine.tState, 4 ); // goto fail
                    break;
            }

            if( fTerminateTest )
            {
                break; // for while loop
            }

            WatchdogKickSoftwareWatchdog( 10000 );
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PWR" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Power = %d", ModemPowerIsPowerEnabled() );
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ON" ) )
            {
                ModemPowerEnablePower( TRUE );
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "OFF" ) )
            {
                ModemPowerEnablePower( FALSE );
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CFG" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Config = %d", ModemConfigIsSuccess() );
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {
                ModemConfigRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INF" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Info Extracted = %d\r\n", ModemInfoIsExtracted() );
                
                ModemDataModemInfoType  * ptModemInfo = NULL;
                ptModemInfo = ModemGetModemInfoPtr();

                if( NULL != ptModemInfo )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Manuf  = %s\r\n", ptModemInfo->szManufacturerName );
                    ConsolePrintf( ptCommand->eConsolePort, "model  = %s\r\n", ptModemInfo->szModem );
                    ConsolePrintf( ptCommand->eConsolePort, "fw     = %s\r\n", ptModemInfo->szFirmwareVer );
                    ConsolePrintf( ptCommand->eConsolePort, "Imei   = %s\r\n", ptModemInfo->szImei );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {
                ModemInfoRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SIM" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Sim Ready = %d\r\n", ModemSimIsReady() );
                
                ModemDataSimCardType  * ptModemSim = NULL;
                ptModemSim = ModemSimGetInfoPtr();

                if( NULL != ptModemSim )
                {                    
                    ConsolePrintf( ptCommand->eConsolePort, "CIMI = %s\r\n", ptModemSim->szCimei );
                    ConsolePrintf( ptCommand->eConsolePort, "CCID = %s\r\n", ptModemSim->szIccId );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {
                ModemSimCheckRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CRE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Creg = %s\r\n", ModemRegStatGetStatusString( ModemRegStatGetStatus() ) );                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {
                ModemRegStatCheckRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CGR" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Creg/Cgreg = %s\r\n", ModemRegStatGetStatusString( ModemRegStatGetStatus() ) );                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {
                ModemCgRegStatCheckRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CON" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Registered = %d\r\n", ModemConnectIsConnected() );
                
                ModemDataNetworkType  * ptModemNetwork = NULL;
                ptModemNetwork = ModemConnectGetInfoPtr();

                if( NULL != ptModemNetwork )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Ip assigned = %s\r\n", ptModemNetwork->szIp );
                    ConsolePrintf( ptCommand->eConsolePort, "Socket ID = %d\r\n", ptModemNetwork->bSocketConnId );
                    ConsolePrintf( ptCommand->eConsolePort, "Socket cntx ID = %d\r\n", ptModemNetwork->bPdpCntxtId );

                    ConsolePrintf( ptCommand->eConsolePort, "Socket stat= %d\r\n", ptModemNetwork->bSocketStat );
                    ConsolePrintf( ptCommand->eConsolePort, "Socket cntx stat= %d\r\n", ptModemNetwork->bSocketCntxtStat );

                    ConsolePrintf( ptCommand->eConsolePort, "creg= %d\r\n", ptModemNetwork->bCregStat );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ON" ) )
            {                
                ModemConnectRun( TRUE, (1000*60*1) );                
            } 
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "OFF" ) )
            {
                ModemConnectRun( FALSE, (1000*60*1) );
            } 
            else
            // MDM CON CONFIG APN LIST
            // MDM CON CONFIG APN SET <ApnString>
            // MDM CON CONFIG APN SETFROMLIST <ApnListNumber>
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CONFIG" ) )
            {
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "APN" ) )
                {
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 4 ], "LIST" ) )
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Apn list\r\n" );

                        for( UINT8 c = 0 ; c < MODEM_CONNECT_APN_ADDRESS_MAX ; c++ )
                        {
                            ConsolePrintf( ptCommand->eConsolePort, "Apn %d = \"%s\"\r\n", c, ModemConnectApnGetString(c) );
                        }
                    }
                    else
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 4 ], "SET" ) )
                    {
                        if( ptCommand->bNumArgs == 6 )
                        {
                            ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_MODEM_APN_ADDRESS_STRING, ptCommand->pcArgArray[ 5 ] );                        
                        
                            ConfigParametersSaveByConfigType( CONFIG_TYPE_COMMUNICATIONS );
                        }
                        else
                        {
                            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                        }
                    }
                    else
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 4 ], "SETFROMLIST" ) )
                    {
                        if( ptCommand->bNumArgs == 6 )
                        {
                            UINT8 bApnSelection = atoi( ptCommand->pcArgArray[ 5 ] );

                            if( bApnSelection < MODEM_CONNECT_APN_ADDRESS_MAX )
                            {
                                ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_MODEM_APN_ADDRESS_STRING, ModemConnectApnGetString(bApnSelection) );
                        
                                ConfigParametersSaveByConfigType( CONFIG_TYPE_COMMUNICATIONS );
                            }
                            else
                            {
                                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                            }
                        }
                        else
                        {
                            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                        }
                    }
                    else                
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
                    }
                }                
                else                
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CEL" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Cell Info Ready = %d\r\n", ModemCellInfoIsReady() );
                
                ModemDataCellInfoType  * ptModemCellInfo = NULL;
                ptModemCellInfo = ModemCellInfoGetInfoPtr();

                if( NULL != ptModemCellInfo )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Operator = %s\r\n", ptModemCellInfo->szOperatorName );                    
                    ConsolePrintf( ptCommand->eConsolePort, "Lac = %s\r\n", ptModemCellInfo->szLac );
                    ConsolePrintf( ptCommand->eConsolePort, "Cel Id = %s\r\n", ptModemCellInfo->szCellId );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {                
                ModemCellInfoCheckRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SIG" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Signal Quality Ready = %d\r\n", ModemSignalIsSignalAcquired() );
                
                ModemDataSignalInfoType  * ptModemSignal = NULL;
                ptModemSignal = ModemSignalGetDataPtr();

                if( NULL != ptModemSignal )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Rssi=%d(dBm)  Ber=%6.3f(Percent)\r\n", ptModemSignal->idwRssi_dBm, ptModemSignal->sgBer_Percent );                    
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {                
                ModemSignalRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CLK" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is time corrected = %d\r\n", ModemSetSysClockIsTimeCorrected() );
                
                ModemDataSysClockType  * ptModemSysClock = NULL;
                ptModemSysClock = ModemSetSysClockGetInfoPtr();

                if( NULL != ptModemSysClock )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Delta seconds=%d\r\n", ptModemSysClock->bDeltaSecondsConfigured );                    
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {                
                ModemSetSysClockRun();
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FTP" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Ftp Connected = %d\r\n", ModemFtpConnectIsConnected() );

                //if( gtFtpOperationSemaphore.ptOperationPermit != NULL )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Transfer Type =" );
                                        
                    switch( ModemFtpConnectAllowedOperation() )
                    {
                        case MODEM_FTP_OPERATION_TYPE_NONE:
                            ConsolePrintf( ptCommand->eConsolePort, "None\r\n" );
                            break;
                        case MODEM_FTP_OPERATION_TYPE_PUT:
                            ConsolePrintf( ptCommand->eConsolePort, "Put\r\n" );
                            break;
                        case MODEM_FTP_OPERATION_TYPE_GET:
                            ConsolePrintf( ptCommand->eConsolePort, "Get\r\n" );
                            break;
                        case MODEM_FTP_OPERATION_TYPE_DELETE:
                            ConsolePrintf( ptCommand->eConsolePort, "Delete\r\n" );
                            break;
                        default:
                            ConsolePrintf( ptCommand->eConsolePort, "Unknown\r\n" );
                            break;
                    }
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PUT" ) )
            {               
                BOOL    fConnectToServer = FALSE;
                
                if( ModemFtpConnectIsConnected() == FALSE )
                {
                    fConnectToServer = TRUE;
                }
                
                ModemFtpConnectRun( fConnectToServer, MODEM_FTP_OPERATION_TYPE_PUT );
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "GET" ) )
            {               
                BOOL    fConnectToServer = FALSE;
                
                if( ModemFtpConnectIsConnected() == FALSE )
                {
                    fConnectToServer = TRUE;
                }

                ModemFtpConnectRun( fConnectToServer, MODEM_FTP_OPERATION_TYPE_GET );
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "DEL" ) )
            {
                BOOL    fConnectToServer = FALSE;
                
                if( ModemFtpConnectIsConnected() == FALSE )
                {
                    fConnectToServer = TRUE;
                }

                ModemFtpConnectRun( fConnectToServer, MODEM_FTP_OPERATION_TYPE_DELETE );
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }

//    ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT NAME <Name> <fSize> - \r\n" );
//    ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT DATA <data> <bytes> - \r\n" );
//    ConsolePrintf( ptCommand->eConsolePort, "MDM FPUT SHOW - show data uploaded \r\n" );    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FPUT" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "NAME" ) )
            {
                if( ptCommand->bNumArgs == 5 )
                {
                    UINT16 wFileSize = atoi( ptCommand->pcArgArray[ 4 ] );
                    ModemFtpOperationPutSetFile( (CHAR *)ptCommand->pcArgArray[ 3 ], wFileSize );
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "DATA" ) )
            {
                if( ptCommand->bNumArgs == 5 )
                {
                    UINT16 wBytes = atoi( ptCommand->pcArgArray[ 4 ] );
                    
                    UINT8   *pbBuffer = NULL;
                    UINT32  dwBufferSize;
                    UINT32  dwBytesRead;                    

                    ModemFtpOperationPutSendBufferGetInfo( &pbBuffer, &dwBufferSize );

                    // validate amount of bytes to copy
                    UINT16 strLen = strlen( (CHAR *)ptCommand->pcArgArray[ 3 ] );

                    if( strLen <= dwBufferSize )
                    {
                        dwBytesRead = strLen;
                    }
                    else
                    {
                        dwBytesRead = dwBufferSize;                        
                    }

                    // Buffer to send gets updated by writing to pbBuffer
                    memcpy( pbBuffer, ptCommand->pcArgArray[ 3 ], dwBytesRead );

                    // send bytes writen to pbBuffer
                    ModemFtpOperationPutSendData( dwBytesRead );
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }            
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
//    ConsolePrintf( ptCommand->eConsolePort, "MDM FGET NAME <Name> <alloc> - \r\n" );
//    ConsolePrintf( ptCommand->eConsolePort, "MDM FGET DATA - request data \r\n" );
//    ConsolePrintf( ptCommand->eConsolePort, "MDM FGET SHOW - show data downloaded \r\n" );
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FGET" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "NAME" ) )
            {
                if( ptCommand->bNumArgs == 5 )
                {
                    UINT16 wAllocMaxSize = atoi( ptCommand->pcArgArray[ 4 ] );
                    ModemFtpOperationGetSetFile( (CHAR *)ptCommand->pcArgArray[ 3 ], wAllocMaxSize );
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "DATA" ) )
            {
                ModemFtpOperationGetReceiveData();                            
            }            
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SHOW" ) )
            {
                UINT8   *pbBuffer = NULL;
                UINT32  dwBufferSize;
                UINT32  dwBytesRead;
                BOOL    fEofReached;
                
                ModemFtpOperationGetReceiverBufferGetInfo( &pbBuffer, &dwBufferSize, &dwBytesRead, &fEofReached );
                
                GeneralHexDump
                ( 
                    ptCommand->eConsolePort, 
                    0,
                    &pbBuffer[0], 
                    dwBytesRead
                );
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    //ConsolePrintf( ptCommand->eConsolePort, "MDM FDEL NAME <Name> - \r\n" );        
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FDEL" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "NAME" ) )
            {
                ModemFtpOperationDeleteFile( (CHAR *)ptCommand->pcArgArray[ 3 ] );
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONNAUTO" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], COMMAND_STRING__STAT ) )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Is Auto enabled = %d\r\n", ModemConnAdvIsEnabled() );
                ConsolePrintf( ptCommand->eConsolePort, "Is waiting for cmd = %d\r\n", ModemConnAdvConnectIsWaitingForCommand() );
                ConsolePrintf( ptCommand->eConsolePort, "Is connected = %d\r\n", ModemConnAdvConnectIsConnected() );
//                
//                ModemDataSignalInfoType  * ptModemSignal = NULL;
//                ptModemSignal = ModemSignalGetDataPtr();
//
//                if( NULL != ptModemSignal )
//                {
//                    ConsolePrintf( ptCommand->eConsolePort, "Rssi=%d(dBm)  Ber=%6.3f(Percent)\r\n", ptModemSignal->idwRssi_dBm, ptModemSignal->sgBer_Percent );                    
//                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "EN" ) )
            {
                if( ModemConnAdvIsEnabled() )
                {
                    ModemConnAdvEnable( FALSE );
                }
                else
                {
                    ModemConnAdvEnable( TRUE );
                }
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RUN" ) )
            {                
                if( ModemConnAdvIsEnabled() )
                {
                    if( ModemConnAdvConnectIsWaitingForCommand() )
                    {
                        if( ModemConnAdvConnectIsConnected() == FALSE )
                        {
                            ModemConnAdvConnectRun( TRUE );
                        }
                        else
                        {
                            ModemConnAdvConnectRun( FALSE );
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
     // MODEM CONSOLE command --------------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONSOLE" ) == TRUE )
    {
        if( ModemPowerIsPowerEnabled() )
        {
            TIMER tTimerNoActivity = TimerDownTimerStartMs( 20000 );
            TIMER tTimerEsc;
            BOOL fEscapeReceived = FALSE;
            ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO TERMINATE MODEM DIRECT COMM!!!\r\n\r\n" );
        
            UINT16		wBytesReturned = 0;
            UINT8 c;
            // Wait for the response to be returned
        
            ModemCommandSemaphoreEnum eSemaphoreResult = 0;
            ModemCommandProcessorReserve( &eSemaphoreResult );

            while( 1 )
            {
                WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                UINT8 c;
                if ( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &c ) == TRUE )
                {
                    if
                    (
                        ( c < 127 ) && 
                        ( c >= 32 )
                    )
                    {                    
                        ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
                    }
                    else
                    {
                        //ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
                        ConsolePrintf( ptCommand->eConsolePort, "[0x%02X]", c );                       
                    }
                }

                if ( ConsoleGetChar(ptCommand->eConsolePort, &c) == TRUE )
                {
                    tTimerNoActivity = TimerDownTimerStartMs( 20000 );
                    tTimerEsc = TimerDownTimerStartMs( 1000 );
                    fEscapeReceived = FALSE;
                    if ( c == COMMAND_ESC_KEY )
                    {
                        fEscapeReceived = TRUE;
                    }                
                    else if ((c == '\r') || ((c >= ' ') && (c <= '~')))
                    {
                        UsartPutNB( TARGET_USART_PORT_TO_MODEM, c );

                        // Echo the character out of the serial port
                        ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
                        if (c == '\r')
                        {
                            ConsolePutChar( ptCommand->eConsolePort, '\n', TRUE );
                        }
                    }
                }
                if  (TimerDownTimerIsExpired( tTimerEsc ) == TRUE )
                {
                    if (fEscapeReceived)
                    {
                        break;
                    }
                }
                if(TimerDownTimerIsExpired( tTimerNoActivity ) == TRUE )
                {
                    break;
                }
            }
            ModemCommandProcessorRelease( &eSemaphoreResult );
            ConsolePrintf( ptCommand->eConsolePort, "\r\nDirect Modem connection terminated.\r\n" );
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Modem if OFF\r\n" );
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerAdc( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerAdc( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "ADC Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADC STAT -  Shows the list of all active ADCs\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADC CONT ALL [PERIOD] - Print continuously all active ADCs every PERIOD milliseconds\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = 1000 ms{Min = 10 ms}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADC CONT SINGLE <ADC_NUM> [PERIOD] - Print data from given ADC sensor every PERIOD milliseconds\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = 1000 ms{Min = 10 ms}\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        AdcInternalAllPinsPrintStatus( ptCommand->eConsolePort );
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONT" ) == TRUE )
    {
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
        {
            UINT32          dwPeriod_ms = 1000;
            TIMER           tTimer      = 0;
            CHAR            cIsEscKey   = 0;
            UINT16          wReadingRaw         = 0;
            SINGLE          sgReadingMillVolt   = 0;

            if( ptCommand->pcArgArray[ 3 ] != NULL )
            {//// [PERIOD] ////
                dwPeriod_ms = atoi( ptCommand->pcArgArray[ 3 ] );
                // validate period
                dwPeriod_ms = dwPeriod_ms > 10 ? dwPeriod_ms : 10;
            }

            tTimer = TimerDownTimerStartMs( 1000 );
            ConsolePrintf( ptCommand->eConsolePort, "Units in millivolts\r\n" );
            ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
            while( TimerDownTimerIsExpired( tTimer ) == FALSE );

            // print column descriptor
            ConsolePrintf( ptCommand->eConsolePort, "Order of pins by column:" );
            for( AdcSensorEnum eAdcCounter = 0 ; eAdcCounter < ADC_SENSOR_ENUM_TOTAL ; eAdcCounter++ )
            {
                ConsolePrintf( ptCommand->eConsolePort, "[%d]:%s, ", eAdcCounter, AdcInternalGetStringName( eAdcCounter ) );
            }
            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

            // Set timer to zero to start so initial reading is immediate
            tTimer = TimerDownTimerStartMs( 0 );

            while(1)
            {
                WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                {
                    tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                    // print pin status
                    for( AdcSensorEnum eAdcCounter = 0 ; eAdcCounter < ADC_SENSOR_ENUM_TOTAL ; eAdcCounter++ )
                    {
                        AdcInternalGetReadingRaw( eAdcCounter, 1, &wReadingRaw );
                        sgReadingMillVolt = AdcInternalRawToMilliVolts( eAdcCounter, wReadingRaw );
                        ConsolePrintf( ptCommand->eConsolePort, "[%d]:%5.2f, ", eAdcCounter, sgReadingMillVolt );
                    }
                    ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
                }

                // break loop if Escape Key received
                if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                {
                    if( cIsEscKey == COMMAND_ESC_KEY )
                    {
                        break;
                    }
                }

            }// end while loop

        }
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SINGLE" ) == TRUE )
        {
            UINT32          dwPeriod_ms = 1000;
            TIMER           tTimer      = 0;
            CHAR            cIsEscKey   = 0;
            LOGIC           eLogic      = LOW;
            AdcSensorEnum   ePinNumber          = 0;
            UINT16          wReadingRaw         = 0;
            SINGLE          sgReadingMillVolt   = 0;

            if( ptCommand->pcArgArray[ 3 ] != NULL )
            {//// <PIN_NUM> ////
                ePinNumber = atoi( ptCommand->pcArgArray[ 3 ] );

                // validate pin number
                if( ePinNumber < ADC_SENSOR_ENUM_TOTAL )
                {

                    if( ptCommand->pcArgArray[ 4 ] != NULL )
                    {//// [PERIOD] ////
                        dwPeriod_ms = atoi( ptCommand->pcArgArray[ 4 ] );
                        // validate period
                        dwPeriod_ms = dwPeriod_ms > 10 ? dwPeriod_ms : 10;
                    }

                    tTimer = TimerDownTimerStartMs( 1000 );
                    ConsolePrintf( ptCommand->eConsolePort, "Units in millivolts\r\n" );
                    ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
                    while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                    // print column descriptor
                    ConsolePrintf( ptCommand->eConsolePort, "Adc Pin Name:%s\r\n", AdcInternalGetStringName( ePinNumber ) );

                    // Set timer to zero to start so initial reading is immediate
                    tTimer = TimerDownTimerStartMs( 0 );

                    while(1)
                    {
                        WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                        if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                        {
                            tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                            AdcInternalGetReadingRaw( ePinNumber, 1, &wReadingRaw );
                            sgReadingMillVolt = AdcInternalRawToMilliVolts( ePinNumber, wReadingRaw );

                            if (AdcInternalIsPinThermSensor(ePinNumber))
                            {
                                SINGLE      sgTempDegCResult = 0.0;
                                AdcInternalMilliVoltsToDegC     (sgReadingMillVolt, &sgTempDegCResult );

                                // print pin status
                                ConsolePrintf( ptCommand->eConsolePort, "%5.2f (%5.2f DegC)\r\n", sgReadingMillVolt, sgTempDegCResult );
                            }
                            else
                            {
                                // print pin status
                                ConsolePrintf( ptCommand->eConsolePort, "%5.2f\r\n", sgReadingMillVolt );
                            }
                        }

                        // break loop if Escape Key received
                        if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                        {
                            if( cIsEscKey == COMMAND_ESC_KEY )
                            {
                                break;
                            }
                        }

                    }// end while loop

                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerBackup( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerBackup( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "BACKUP Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BACKUP ERASE <OFFSET> <SIZE> -  Erase a block of SIZE number of bytes\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BACKUP READ  <OFFSET> <SIZE> -  Read a block of SIZE number of bytes\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BACKUP WRITE <OFFSET> <SIZE> <INT8> -  Write a block of SIZE number of bytes with the value INT8\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ERASE" ) == TRUE )
    {
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) )
        {//// <OFFSET> <SIZE> ////
            UINT32  dwOffset    = 0;
            UINT32  dwSize      = 0;
            BOOL    fSuccess    = FALSE;

            dwOffset= atoi( ptCommand->pcArgArray[ 2 ] );
            dwSize  = atoi( ptCommand->pcArgArray[ 3 ] );

            fSuccess = BackupSramErase( dwOffset, dwSize );

            if( fSuccess == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL; // Pin is not output
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "READ" ) == TRUE )
    {
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) )
        {//// <OFFSET> <SIZE> ////
            UINT32  dwOffset    = 0;
            UINT32  dwSize      = 0;
            UINT8   bVal        = 0;
            UINT32  dwJumpLine  = 0;
            BOOL    fSuccess    = TRUE;

            dwOffset= atoi( ptCommand->pcArgArray[ 2 ] );
            dwSize  = atoi( ptCommand->pcArgArray[ 3 ] );

            for( UINT32 dwDynamicOffset = dwOffset ; dwDynamicOffset < (dwOffset+dwSize) ; dwDynamicOffset++ )
            {
                fSuccess &= BackupSramRead( dwDynamicOffset, &bVal, 1 );
                // print value
                ConsolePrintf( ptCommand->eConsolePort, "0x%02X,", bVal );

                if( (dwJumpLine % 16) == 15 )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
                }
                dwJumpLine++;
            }

            if( fSuccess == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL; // Pin is not output
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "WRITE" ) == TRUE )
    {
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL )  && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
        {//// <OFFSET> <SIZE> <CHAR> ////
            UINT32  dwOffset    = 0;
            UINT32  dwSize      = 0;
            CHAR    cChar       = 0;
            BOOL    fSuccess    = TRUE;

            dwOffset= atoi( ptCommand->pcArgArray[ 2 ] );
            dwSize  = atoi( ptCommand->pcArgArray[ 3 ] );
            cChar   = atoi( ptCommand->pcArgArray[ 4 ] );

            for( UINT32 dwDynamicOffset = dwOffset ; dwDynamicOffset < (dwOffset+dwSize) ; dwDynamicOffset++ )
            {
                fSuccess &= BackupSramWrite( dwDynamicOffset, &cChar, 1 );
            }

            if( fSuccess == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL; // Pin is not output
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerBluetooth( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerBluetooth( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH STAT - Indicates if Bluetooth is Enabled/Disabled\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH ENABLE <BOOL> - Enable/Disable the Bluetooth Module\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH TEST\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH BCONSOLE\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BLUETOOTH STATLED <BOOL>\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {            
        ConsolePrintf( ptCommand->eConsolePort, "Bluetooth Enabled: %d\r\n", BluetoothIsPowerEnabled() );        
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ENABLE" ) == TRUE )
    {
        BOOL    fEnable = atoi( ptCommand->pcArgArray[ 2 ] );

        BluetoothPowerEnable(fEnable);
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TEST" ) == TRUE )
    {
        BOOL        fResult;
        fResult = BluetoothTestModule();

        ConsolePrintf( ptCommand->eConsolePort, "Bluetooth Test Result: %u\r\n", fResult );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "BCONSOLE" ) == TRUE )
    {
        TIMER tTimerNoActivity = TimerDownTimerStartMs( 20000 );
        TIMER tTimerEsc;
        BOOL fEscapeReceived = FALSE;
        ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO TERMINATE BLUETOOTH DIRECT COMM!!!\r\n\r\n" );
        
        UINT8           c;
        // Wait for the response to be returned

        while( 1 )
        {
            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

            UINT8 c;
            if ( UsartGetNB(TARGET_USART_PORT_TO_BLUETOOTH, &c) == TRUE )
            {
                ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
            }

            if ( ConsoleGetChar(ptCommand->eConsolePort, &c) == TRUE )
            {
                tTimerNoActivity = TimerDownTimerStartMs( 20000 );
                tTimerEsc = TimerDownTimerStartMs( 1000 );
                UsartPutNB( TARGET_USART_PORT_TO_BLUETOOTH, c );
                fEscapeReceived = FALSE;
                if ( c == COMMAND_ESC_KEY )
                {
                    fEscapeReceived = TRUE;
                }                
            }
            if  (TimerDownTimerIsExpired( tTimerEsc ) == TRUE )
            {
              if (fEscapeReceived)
              {
                break;
              }
            }
            if  (TimerDownTimerIsExpired( tTimerNoActivity ) == TRUE )
            {
              break;
            }
        }
        ConsolePrintf( ptCommand->eConsolePort, "\r\nDirect Bluetooth connection terminated.\r\n" );
    }     
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "STATLED" ) == TRUE )
    {        
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {
            BOOL fEnable = atoi( ptCommand->pcArgArray[ 2 ] );
            BluetoothStatusLedEnable( fEnable );
            
            eResult = CONSOLE_RESULT_OK;
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerBootloader( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerBootloader( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "BOOTLOADER Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BOOTLOADER EXEC\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "BOOTLOADER STAT\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "EXEC" ) == TRUE )
    {
        if ((BuildInfo_PidMatches_DFFile()) && (LoaderCRCValidates() ))
//        if (1)
//dsa
        //if (GeneralCalcCrc32DataFlash(DFF_FIRMWARE_SUBSECTOR_START * DATAFLASH_SUBSECTOR_SIZE_BYTES, DFF_FIRMWARE_SUBSECTORS_TOTAL * DATAFLASH_SUBSECTOR_SIZE_BYTES) == 0xFFFFFFFF)
//        if (1)
        {
          BootloaderExecuteBootloader();
        }
        else
        {
          eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "STAT" ) == TRUE )
    {
        ConsolePrintf( ptCommand->eConsolePort, "BootLoader Status Information:\r\n" );
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerConfig( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerConfig( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum   eResult     = CONSOLE_RESULT_OK;
    BOOL                fSuccess    = FALSE;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG HELP - This help information\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "        replace <CONFIG_TYPE_STRING> for the following options:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                < " );
        for( ConfigTypeEnum eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
        {            
            ConsolePrintf( ptCommand->eConsolePort, "%s", ConfigGetConfigTypeName( eConfigType ) );
            if( eConfigType < (CONFIG_TYPE_MAX-1) )
            {
                ConsolePrintf( ptCommand->eConsolePort, " | " );
            }
        }
        ConsolePrintf( ptCommand->eConsolePort, " >\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG LIST <ALL | <CONFIG_TYPE_STRING> > - Displays a list of selected configuration.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG SHOW <CONFIG_TYPE_STRING> <PARAM #> - Get the details of a single parameter.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG TO <ALL | <CONFIG_TYPE_STRING Bit Mask> > < CONSOLE | BACKUP | SCRIPT >- Send commands with current configuration to the specified output(console or backup file).\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "   Bit Mask               CONFIG_TYPE_STRING\r\n" );
        UINT32 dwBitMask = 0;
        for( ConfigTypeEnum eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
        {              
            dwBitMask = (UINT32)pow( 10.0 ,eConfigType );
            ConsolePrintf( ptCommand->eConsolePort, "   %-6d                ", dwBitMask );
            ConsolePrintf( ptCommand->eConsolePort, "%s\r\n", ConfigGetConfigTypeName( eConfigType ) );
        }        
        ConsolePrintf( ptCommand->eConsolePort, "    Examples:\r\n\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG TO ALL CONSOLE  \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG TO 101 BACKUP   (dev_info and adc_cal will be saved in backup)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG TO 1010 SCRIPT  (setting and comm will be saved in script file)\r\n" );
        
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG FROM < BACKUP | SCRIPT > - Read the file and run it as script.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                    File is expected to contain commands to set config params.\r\n" );        

        ConsolePrintf( ptCommand->eConsolePort, "CONFIG LOAD - Loads configuration from non-volatile memory\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG SAVE - Saves current configuration to non-volatile memory\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG REVERT <ALL | <TYPE <CONFIG_TYPE_STRING> | PARAM <CONFIG_TYPE_STRING> <PARAM#> > > \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "    Reverts a parameter from a configuration type.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "    Examples:\r\n\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG REVERT ALL\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG REVERT TYPE SETTING\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        CONFIG REVERT PARAM  SETTING 10\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG SET <CONFIG_TYPE_STRING> <PARAM#> <VALUE>- Sets the value of a single parameter from a configuration type\r\n" );                                       
        ConsolePrintf( ptCommand->eConsolePort, "    <PARAM> Specifies the parameter to change. May be either the parameter name or ID number\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "    <VALUE> The value to change the parameter to\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "            If <PARAM#> is of type String(7), wrap arg <VALUE> with quotation [`]simbol\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "    Examples:\r\n\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        config set SETTING 10 123456\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "        config set SETTING 10 `123456`\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        config set SETTING DEVSN 12345\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "CONFIG TEST - \r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "LIST" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 2 ) // if no other arguments, show all configuration parameters.
		{
			fSuccess = ConfigShowListAll( ptCommand->eConsolePort );

			if( fSuccess == FALSE )
			{
				eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
			}
		}
        else if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
            {
                fSuccess = ConfigShowListAll( ptCommand->eConsolePort );

                if( fSuccess == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else 
            {                
                ConfigTypeEnum  eConfigType         = 0;
                BOOL            fIsConfigTypeValid  = FALSE;

                // identify the config type
                for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
                {
                    // check if config is valid
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], ConfigGetConfigTypeName( eConfigType ) ) )
                    {                        
                        fIsConfigTypeValid  = TRUE;
                        break;
                    }                    
                }          

                if(fIsConfigTypeValid)
                {
                    if( ConfigShowListByConfigType( ptCommand->eConsolePort, eConfigType ) == FALSE )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else                
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }             
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    //FROM < BACKUP_FILE | SCRIPT >
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FROM" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            BOOL                fIsValid = FALSE;
            DatalogFilesType    eDatalogFileOption = 0;

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "BACKUP" ) == TRUE )
            {
                fIsValid = TRUE;
                eDatalogFileOption = DATALOG_FILE_BACKUP_SCRIPT;                
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SCRIPT" ) == TRUE )
            {
                fIsValid = TRUE;
                eDatalogFileOption = DATALOG_FILE_SCRIPT;                
            }            

            if( fIsValid )
            {
                UINT8   bBufferDataFlashPage          [DATAFLASH_PAGE_SIZE_BYTES];            
                BOOL    fEofReached;
                BOOL    fPass;
                UINT32  dwLengthRead;

                fEofReached     = FALSE;
                fPass           = TRUE;
                dwLengthRead    = 0;

                DatalogOpenForRead( eDatalogFileOption, 0 );
                
                while( ( fPass == TRUE ) && ( fEofReached == FALSE ) )
                {
                    fPass = DatalogReadfileData( eDatalogFileOption, &bBufferDataFlashPage[0], sizeof(bBufferDataFlashPage), &dwLengthRead, &fEofReached );
                    for( UINT16 u = 0; u < dwLengthRead; u++ )
                    {
                        ConsoleProcessSingleCommandString( CONSOLE_PORT_SCRIPT, &bBufferDataFlashPage[ u ], 1 );                                                    
                    }
                    WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );
                }                
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    // CONFIG TOCOMMAND <ALL | <CONFIG_TYPE_STRING> > < CONSOLE | BACKUP_FILE >
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TO" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            UINT8 bConfigTypeBitMask = 0;
            ConfigTypeEnum  eConfigType     = 0;                

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
            {
                bConfigTypeBitMask = 0xFF;
            }
            else
            {                
                /////////////////
                // extract the config bit mask
                // convert char bit mask into bit bit mask
                UINT32 dwBitMask = 0;                       
                //dwBitMask = (UINT32)pow( 10.0 ,eConfigType );

                UINT16  wStringLenBitMask = 0;
                CHAR    cChar;
                wStringLenBitMask = strlen( ptCommand->pcArgArray[ 2 ] );
                
                // identify the config type
                for( eConfigType = 0 ; eConfigType < wStringLenBitMask ; eConfigType++ )
                {
                    cChar = ptCommand->pcArgArray[ 2 ][eConfigType];

                    if
                    (
                        ( cChar == '1' ) &&
                        ( eConfigType < CONFIG_TYPE_MAX )
                    )
                    {
                        bConfigTypeBitMask = bConfigTypeBitMask | (1<<eConfigType);
                    }
                }                
                ///////////////
            }
                
            if( bConfigTypeBitMask > 0 )
            {
                CHAR            cBuffer[250];
                UINT16          wNumOfBytesWritenInBuffer;
                UINT16          wNumOfParams    = 0;                
                INT8            ibOutputType    = -1; // -1 = invalid, 0 to console, 1 to file

                // validate output type
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "CONSOLE" ) == TRUE )
                {
                    ibOutputType = 0;
                }
                else if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "BACKUP" ) == TRUE )
                {
                    ibOutputType = 1;
                }
                else if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "SCRIPT" ) == TRUE )
                {
                    ibOutputType = 2;
                }

                if( ibOutputType != -1 )
                {
                    // write firmware version as a comment in the script
                    // in order to be aware of changes between versions                        
                    Main_SystemInfo     sSystemInfo;        
                    // get current system Fw version
                    Main_GetSystemInfo( &sSystemInfo );
                    cBuffer[0] = '\0';
                    wNumOfBytesWritenInBuffer = snprintf( &cBuffer[0], sizeof(cBuffer),
                        "# %s\r\n # FW rev:(%d)\r\n",
                        &gcCONFIG_BackupScriptIndicatorString[0],
                        sSystemInfo.dwSvnRevision
                    );

                    // if directory file selected (BACKUP_FILE or SCRIPT) clear subsector so is possible to start writing.
                    if( ibOutputType == 1 )// BACKUP
                    {
                        DatalogOpenForWrite( DATALOG_FILE_BACKUP_SCRIPT );
                    	// Erase the first record so that the new file will be placed at the beginning        	
                        //Dataflash_EraseSubsector( DATAFLASH0_ID, DFF_BACKUP_SCRIPT_SUBSECTOR_START );
                        
                        //If the file format is binary, a flat file is opened.
                        // last arg = maximum file size required, for binary files only.
                        //DFF_writefileOpen( DFF_FILE_TYPE_BACKUP_SCRIPT, "TXT", DFF_FILE_STORAGE_TYPE_CIRCULAR, (DFF_BACKUP_SCRIPT_SUBSECTORS_TOTAL*DATAFLASH_SUBSECTOR_SIZE_BYTES) );

                        // write firmware version as a comment in the script
                        // in order to be aware of changes between versions                        
                        DatalogWriteBuffer( DATALOG_FILE_BACKUP_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                    }
                    else if( ibOutputType == 2 )// SCRIPT
                    {
                        DatalogOpenForWrite( DATALOG_FILE_SCRIPT );
                    	// Erase the first record so that the new file will be placed at the beginning        	
                        //Dataflash_EraseSubsector( DATAFLASH0_ID, DFF_SCRIPT_SUBSECTOR_START );    

                        //If the file format is binary, a flat file is opened.
                        // last arg = maximum file size required, for binary files only.
                        //DFF_writefileOpen( DFF_FILE_TYPE_SCRIPT, "TXT", DFF_FILE_STORAGE_TYPE_CIRCULAR, (DFF_SCRIPT_SUBSECTORS_TOTAL*DATAFLASH_SUBSECTOR_SIZE_BYTES) );

                        // write firmware version as a comment in the script
                        // in order to be aware of changes between versions                        
                        DatalogWriteBuffer( DATALOG_FILE_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                    }
                    else
                    {              
                        // send to console
                        ConsolePrintf( ptCommand->eConsolePort, "%s", &cBuffer[0] );
                    } 

                    for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
                    {
                        if( ( bConfigTypeBitMask & (1<<eConfigType) ) > 0 )
                        {
                            ConfigGetNumberOfParamsByConfigType( eConfigType, &wNumOfParams );        
                
                            // print header indicating the config type
                            // send stream to console
                            wNumOfBytesWritenInBuffer = snprintf( &cBuffer[0], sizeof(cBuffer),
                                "# Config Type:(%s)\r\n",
                                ConfigGetConfigTypeName(eConfigType)
                            );

                            // send 
                            if( ibOutputType == 1 )// BACKUP
                            {
                                // send to backup file                                    
                                //DFF_writeBuffer( DFF_FILE_TYPE_BACKUP_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                                DatalogWriteBuffer( DATALOG_FILE_BACKUP_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                            }
                            else
                            if( ibOutputType == 2 )// SCRIPT
                            {
                                // send to script
                                //DFF_writeBuffer( DFF_FILE_TYPE_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                                DatalogWriteBuffer( DATALOG_FILE_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                            }
                            else
                            {              
                                // send to console
                                ConsolePrintf( ptCommand->eConsolePort, "%s", &cBuffer[0] );
                            } 
                            
                            // iterate through all the params of the config type
                            for( UINT16 i = 0 ; i < wNumOfParams ; i++ )
                            {
                                ConfigGetCurrentParamConfigString( eConfigType, i, &cBuffer[0], sizeof(cBuffer), &wNumOfBytesWritenInBuffer );

                                // send 
                                if( ibOutputType == 1 )// BACKUP
                                {
                                    // send to backup file                                
                                    //DFF_writeBuffer( DFF_FILE_TYPE_BACKUP_SCRIPT, &cBuffer[0], wNumOfBytesWritenInBuffer );
                                    DatalogWriteBuffer( DATALOG_FILE_BACKUP_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                                }
                                else
                                if( ibOutputType == 2 )// SCRIPT
                                {
                                    // send to script
                                    //DFF_writeBuffer( DFF_FILE_TYPE_SCRIPT, &cBuffer[0], wNumOfBytesWritenInBuffer );                          
                                    DatalogWriteBuffer( DATALOG_FILE_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                                }
                                else
                                {              
                                    // send to console
                                    ConsolePrintf( ptCommand->eConsolePort, "%s", &cBuffer[0] );
                                }                             
                            }
                                                
                        }// if bit mask for config type
                    }

                    // write a [config save] command at the end                    
                    cBuffer[0] = '\0';
                    wNumOfBytesWritenInBuffer = snprintf( &cBuffer[0], sizeof(cBuffer),
                        "config save\r\n"                 
                    );
                    
                    if( ibOutputType == 1 )// BACKUP
                    {
                        DatalogWriteBuffer( DATALOG_FILE_BACKUP_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                    }
                    else if( ibOutputType == 2 )// SCRIPT
                    {
                        DatalogWriteBuffer( DATALOG_FILE_SCRIPT, &cBuffer[0], (UINT32)wNumOfBytesWritenInBuffer );
                    }
                    else
                    {              
                        // send to console
                        ConsolePrintf( ptCommand->eConsolePort, "%s", &cBuffer[0] );
                    }
                    

                    // close file if file used
                    if( ibOutputType == 1 )// BACKUP
                    {                    
                        //DFF_writefileClose( DFF_FILE_TYPE_BACKUP_SCRIPT );        
                        DatalogCloseForWrite( DATALOG_FILE_BACKUP_SCRIPT );
                    }
                    else
                    if( ibOutputType == 2 )// SCRIPT
                    {                            
                        //DFF_writefileClose( DFF_FILE_TYPE_SCRIPT );        
                        DatalogCloseForWrite( DATALOG_FILE_SCRIPT );
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else 
            {                
//                ConfigTypeEnum  eConfigType         = 0;
//                BOOL            fIsConfigTypeValid  = FALSE;
//
//                // identify the config type
//                for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
//                {
//                    // check if config is valid
//                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], ConfigGetConfigTypeName( eConfigType ) ) )
//                    {                        
//                        fIsConfigTypeValid  = TRUE;
//                        break;
//                    }                    
//                }          
//
//                if(fIsConfigTypeValid)
//                {
//                    if( ConfigShowListByConfigType( ptCommand->eConsolePort, eConfigType ) == FALSE )
//                    {
//                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
//                    }
//                }
//                else                
//                {
//                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
//                }
            }             
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "LOAD" ) == TRUE )
    {
        // Loads configuration from non-volatile memory
        fSuccess = ConfigParametersLoadAll();

        if( fSuccess == FALSE )
        {
            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SAVE" ) == TRUE )
    {
        // Saves the current configuration to non-volatile memory
        fSuccess = ConfigParametersSaveAll();

        if( fSuccess == FALSE )
        {
            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "REVERT" ) == TRUE )
    {
        //CONFIG REVERT ALL                
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
            {
                if( ConfigParametersRevertAll() == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        //CONFIG REVERT TYPE DEVICE
        else if( ptCommand->bNumArgs == 4 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "TYPE" ) == TRUE )
            {
                ConfigTypeEnum  eConfigType         = 0;
                BOOL            fIsConfigTypeValid  = FALSE;

                // identify the config type
                for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
                {
                    // check if config is valid
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], ConfigGetConfigTypeName( eConfigType ) ) )
                    {                        
                        fIsConfigTypeValid  = TRUE;
                        break;
                    }                    
                }                   

                if(fIsConfigTypeValid)
                {
                    if( ConfigParametersRevertByConfigType( eConfigType ) == FALSE )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else                
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        //CONFIG REVERT PARAM  DEVICE 10
        else if( ptCommand->bNumArgs == 5 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PARAM" ) == TRUE )
            {
                ConfigTypeEnum  eConfigType;
                BOOL            fIsConfigTypeValid  = FALSE;                               

                // identify the config type
                for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
                {
                    // check if config is valid
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], ConfigGetConfigTypeName( eConfigType ) ) )
                    {                        
                        fIsConfigTypeValid  = TRUE;
                        break;
                    }                    
                }
                
                if( fIsConfigTypeValid )
                {
                    UINT16 wParamId;

                    // Check to see if the next token is a number representing the parameter ID
                    if( GeneralStringContainsNumber( ptCommand->pcArgArray[ 4 ], 16 ) )
                    {
                        wParamId = strtoul( ptCommand->pcArgArray[ 4 ], NULL, 16 );
                    }
                    // Token is not a number, so it must be a string
                    else
                    {                        
                        ConfigGetIdFromNameByConfigType( eConfigType, ptCommand->pcArgArray[ 4 ], &wParamId );                        
                    }

                    // Revert the parameter
                    if( ConfigRevertValueByConfigType( eConfigType, wParamId ) == FALSE )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    // CONFIG SHOW <DEVICE | SETTING> <PARAM #>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SHOW" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            ConfigTypeEnum  eConfigType = 0;
            BOOL            fIsConfigTypeValid  = FALSE;

            // identify the config type
            for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
            {
                // check if config is valid
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], ConfigGetConfigTypeName( eConfigType ) ) )
                {                        
                    fIsConfigTypeValid  = TRUE;
                    break;
                }                    
            }            

            // CONFIG GET <DEVICE | SETTING> <PARAM#>
            if( fIsConfigTypeValid )
            {
                UINT16  wParamId;                

                // PARAM
                // Check to see if the next token is a number representing the parameter ID
                if( GeneralStringContainsNumber( ptCommand->pcArgArray[ 3 ], 16 ) )
                {
                    wParamId = strtoul( ptCommand->pcArgArray[ 3 ], NULL, 16 );                    
                }
                // Token is not a number, so it must be a string
                else
                {                        
                    ConfigGetIdFromNameByConfigType( eConfigType, ptCommand->pcArgArray[ 3 ], &wParamId );                        
                }                             

                if( ConfigShowParamFromParamId( ptCommand->eConsolePort, TRUE, eConfigType, wParamId ) == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }                
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    // CONFIG SET <DEVICE | SETTING> <PARAM #> <VALUE>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SET" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 5 )
        {
            ConfigTypeEnum  eConfigType;
            BOOL            fIsConfigTypeValid  = FALSE;

            // identify the config type
            for( eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
            {
                // check if config is valid
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], ConfigGetConfigTypeName( eConfigType ) ) )
                {                        
                    fIsConfigTypeValid  = TRUE;
                    break;
                }                    
            }
            
            // CONFIG SET <DEVICE | SETTING> <PARAM#> <VALUE>
            if( fIsConfigTypeValid )
            {
                UINT16                  wParamId;       
                ConfigParameterTypeEnum eParamType;
                INT8                    ibSignedVal;
                INT16                   iwSignedVal;
                INT32                   idwSignedVal;
                UINT32                  dwUnsignedVal;
                SINGLE                  sgFloatVal;
                INT32                   idwVal;

                // PARAM ID
                // Check to see if the next token is a number representing the parameter ID
                if( GeneralStringContainsNumber( ptCommand->pcArgArray[ 3 ], 16 ) )
                {
                    wParamId = strtoul( ptCommand->pcArgArray[ 3 ], NULL, 16 );                    
                }
                // Token is not a number, so it must be a string
                else
                {                        
                    ConfigGetIdFromNameByConfigType( eConfigType, ptCommand->pcArgArray[ 3 ], &wParamId );                        
                }                             
                
                // VALUE
                //////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////
                if( ConfigGetParamType( eConfigType, wParamId, &eParamType ) )
                {
                    switch( eParamType )
                    {
                        case CONFIG_PARAM_TYPE_UINT8:
                        case CONFIG_PARAM_TYPE_UINT16:
                        case CONFIG_PARAM_TYPE_UINT32:
                        case CONFIG_PARAM_TYPE_BOOL:
                        case CONFIG_PARAM_TYPE_CHAR:
                            dwUnsignedVal = strtoul( ptCommand->pcArgArray[ 4 ], NULL, 0 );
                            ConfigSetValueByConfigType( eConfigType, wParamId, &dwUnsignedVal );
                            break;

                        case CONFIG_PARAM_TYPE_INT8:
                            ibSignedVal = atoi( ptCommand->pcArgArray[ 4 ] ); 
                            ConfigSetValueByConfigType( eConfigType, wParamId, &ibSignedVal );
                            break;

                        case CONFIG_PARAM_TYPE_INT16:
                            iwSignedVal = atoi( ptCommand->pcArgArray[ 4 ] ); 
                            ConfigSetValueByConfigType( eConfigType, wParamId, &iwSignedVal );
                            break;

                        case CONFIG_PARAM_TYPE_INT32:
                            idwSignedVal = atoi( ptCommand->pcArgArray[ 4 ] ); 
                            ConfigSetValueByConfigType( eConfigType, wParamId, &idwSignedVal );
                            break;

                        case CONFIG_PARAM_TYPE_FLOAT:                                                                    
                            if( sscanf( ptCommand->pcArgArray[ 4 ], "%f", &sgFloatVal ) == 1 ) 
                            {
                                // it is a float.
                                if( ConfigSetValueByConfigType( eConfigType, wParamId, &sgFloatVal ) == FALSE )
                                {
                                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                                }                        
                            }
                            else
                            {
                                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                            }
                            break;                            
                        case CONFIG_PARAM_TYPE_STRING: 
                            // Extract string from `` symbols
                            if( ptCommand->pcArgArray[ 4 ][0]  == '`' )
                            {   
                                // should be a string
                                // check that the last char is also quotation mark
                                UINT32 dwStrLen = strlen( ptCommand->pcArgArray[ 4 ] );
                            
                                if( ptCommand->pcArgArray[ 4 ][dwStrLen-1] == '`' )
                                {
                                    // extract the quotation marks
                                    CHAR  *pcStringPointer;
                                    // 1.- first quotation removed by pointing the string index pointer to [1]
                                    pcStringPointer = (CHAR  *)&ptCommand->pcArgArray[ 4 ][1];
                                    dwStrLen = dwStrLen - 1;

                                    // 2.- second quotation removed by setting the char [`] to [0] or end of string
                                    pcStringPointer[dwStrLen-1] = 0;
                                    dwStrLen = dwStrLen - 1;
                                    
                                    if( ConfigSetValueByConfigType( eConfigType, wParamId, &pcStringPointer[0] ) == FALSE )
                                    {
                                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                                    }                                
                                }
                                else
                                {
                                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                                }  
                            }
                            else
                            {
                                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                            }
                            break; 
                    
                    }// end switch
                }// end if param type valid                
            }// edn if config type valid
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TEST" ) == TRUE )
    {
        //eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        CHAR            cBuffer[250];
        UINT16          wNumOfParams    = 0;
        ConfigTypeEnum  eConfigType     = CONFIG_TYPE_DEVICE_INFO;

        ConfigGetNumberOfParamsByConfigType( eConfigType, &wNumOfParams );        
        
        for( UINT16 i = 0 ; i < wNumOfParams ; i++ )
        {
            ConfigGetCurrentParamConfigString( eConfigType, i, &cBuffer[0], sizeof(cBuffer), NULL );
            ConsolePrintf( ptCommand->eConsolePort, "%s\r\n", &cBuffer[0] );
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerDev( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerDev( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "DEV Device Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV CLOCK    -  Show the MCU clock information\r\n" );
        
        ConsolePrintf( ptCommand->eConsolePort, "DEV CLOCK HSITRIMUPDATE [ON | OFF]\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV CLOCK HSITRIM [0..23]\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                    -  HSI TRIM Auto Uptade / Manual TRIM Setting\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "DEV ERASEALL -  Erase entire dataflash, including configuration\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV ERASESS  -  Erase dataflash subsector s\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV ERRORS   -  Display initialization status and errors\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV MCU      -  Show the MCU ID information\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV RESET    -  Reset the device\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV RTOS     -  Show the RTOS information\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV SECURITY -  Show the MCU flash security information\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV MCUFW2DFLASH - Copies the firmware running into Dataflash Fimrware File.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "DEV FIRMWARE_UPLOAD -  Upload a firmware image to DataFlash\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV REQ NEWFW - Request the system to download a new FW.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV REQ NEWCAMFW - Request the system to download a new Camera FW.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DEV SENDFW2CAM - Send Firmware in dataflash to Camera.\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "MCUFW2DFLASH" ) == TRUE )
    {
        ControlCopyMcuFlashFwToDataflashFwFile();
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "REQ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "NEWFW" ) == TRUE )
            {
                ControlSystemRequest( CONTROL_ACTION_DOWNLOAD_MAIN_BOARD_FW );
            }
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "NEWCAMFW" ) == TRUE )
            {
                ControlSystemRequest( CONTROL_ACTION_DOWNLOAD_CAMERA_FW );
            }
        }        
    }
    //// DEV CLOCK command //////////////////////////////////////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CLOCK" ) == TRUE )
    {
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "HSITRIM" ) == TRUE )
        {
           if (ptCommand->pcArgArray[ 3 ] == NULL)
           {
              UINT32 tmpreg = RCC->CR;

              /* Clear HSITRIM[4:0] bits */
              tmpreg &= RCC_CR_HSITRIM;
              tmpreg >>= 3;

              ConsolePrintf( ptCommand->eConsolePort, "HSITRIM=%d \r\n", tmpreg);            
           }
           else
           {
              UINT16 wHsitrim     = strtoul(ptCommand->pcArgArray[ 3 ], NULL, 0);
              if ((wHsitrim >=0 ) && (wHsitrim <=31 ))
              {
                RCC_AdjustHSICalibrationValue( wHsitrim );              
              }
              else
              {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
              }
           }
        }
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "HSITRIMUPDATE" ) == TRUE )
        {
           if (ptCommand->pcArgArray[ 3 ] == NULL)
           {
              BOOL fTemp = ControlGetHSITRIMUpdate();
              ConsolePrintf( ptCommand->eConsolePort, "HSITRIMUPDATE %d \r\n", fTemp);
           }
           else
           {
              if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "ON" ) == TRUE )
              {
                ControlHSITRIMUpdateEnable( TRUE );
              }
              else if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "OFF" ) == TRUE )
              {
                ControlHSITRIMUpdateEnable( FALSE );
                }
           }
        }
        else
        {
            RCC_ClocksTypeDef RCC_clockFreq;
            UINT8 c;

            /* Print clock source
             * Internal High Speed Clock (HSI)
             * External High Speed Clock (HSE)
             * Phase locked Loop (PLL)
             */
            c = RCC_GetSYSCLKSource();
            switch( c )
            {
                case 0x00:
                    ConsolePrintf( ptCommand->eConsolePort, "HSI" );
                    break;
                case 0x04:
                    ConsolePrintf( ptCommand->eConsolePort, "HSE" );
                    break;
                case 0x08:
                    ConsolePrintf( ptCommand->eConsolePort, "PLL" );
                    break;
                default:
                    ConsolePrintf( ptCommand->eConsolePort, "Unknown source" );
                    break;
            }
            ConsolePrintf( ptCommand->eConsolePort, " used as system clock\r\n" );

            RCC_GetClocksFreq( &RCC_clockFreq );
            ConsolePrintf( ptCommand->eConsolePort, "SYSCLK=%ld, HCLK=%ld, PCLK1=%ld, PCLK2=%ld\r\n",
                    RCC_clockFreq.SYSCLK_Frequency, RCC_clockFreq.HCLK_Frequency,
                    RCC_clockFreq.PCLK1_Frequency, RCC_clockFreq.PCLK2_Frequency );
        }
    }
    //// DEV ERASEALL to erase entire dataflash ////////////////////////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ERASEALL" ) == TRUE )
    {
        ConsolePrintf( ptCommand->eConsolePort, "Erasing entire dataflash (will take 160 to 250 seconds)." );
        ConsolePrintf( ptCommand->eConsolePort, "Please be patient. Erasing dataflash..." );
        Dataflash_EraseAll( DATAFLASH0_ID );
        ConsolePrintf( ptCommand->eConsolePort, "done." );
    }
    //// DEV ERASESS to erase a dataflash subsector ////////////////////////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ERASESS" ) == TRUE )
    {
        DATAFLASH_ERROR_ENUM eError;
        UINT16 wSubSector;

        wSubSector = atoi( ptCommand->pcArgArray[ 2 ] );

        ConsolePrintf( ptCommand->eConsolePort, "Erasing dataflash subsector..." );
        eError = Dataflash_EraseSubsector( DATAFLASH0_ID, wSubSector );
        ConsolePrintf( ptCommand->eConsolePort, "done." );
        if( eError != DATAFLASH_OK )
        {
            return FALSE;
        }
    }
    //// DEV ERRORS to display initialization status and errors //////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ERRORS" ) == TRUE )
    {
        Main_PrintSystemInitStatus( ptCommand->eConsolePort, TRUE );
    }    
    //// DEV MCU command to show microcontroller info //////////////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "MCU" ) == TRUE )
    {
        Main_PrintSystemInfo( ptCommand->eConsolePort );
    }
    //// DEV RESET command  ////////////////////////////////////////////////////////////////////////////
    else if( ConsoleStringEqual(ptCommand->pcArgArray[ 1 ], "RESET") == TRUE )
    {
        SystemSoftwareReset();
    }
    //// DEV RTOS command to print RTOS status /////////////////////////////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "RTOS" ) == TRUE )
    {
#define CMD_NUM_TASKS                       (4)    // Number of tasks in the system
#define CMD_TASK_INFO_BUFFER_SIZE           ( ( CMD_NUM_TASKS + 1 ) * ( 41 + 2 ) )

        UINT8 cTaskInfoBuffer[ CMD_TASK_INFO_BUFFER_SIZE ];

        memset( &cTaskInfoBuffer[0], 0x44, sizeof( cTaskInfoBuffer ) );

        unsigned portBASE_TYPE dwNumTasks = uxTaskGetNumberOfTasks();

        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Number of Tasks:  %d\r\n", dwNumTasks );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        vTaskList(  &cTaskInfoBuffer[ 0 ] );
        ConsolePrintf( ptCommand->eConsolePort, "Task List:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Name       Status  Priority  StackAvail TCB\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "                             (Words)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "%s", &cTaskInfoBuffer[ 0 ] );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n\r\n" );

//      vTaskGetRunTimeStats(  &cTaskInfoBuffer[ 0 ] );
//      ConsolePrintf( ptCommand->eConsolePort, "FreeRTOS Runtime Statistics:\r\n" );
//      ConsolePrintf( ptCommand->eConsolePort, "Name         RuntimeUsage    Percentage\r\n" );
//      ConsolePrintf( ptCommand->eConsolePort, "%s", &cTaskInfoBuffer[ 0 ] );
//      ConsolePrintf( ptCommand->eConsolePort, "\r\n\r\n" );

        /* Note: configTOTAL_HEAP_SIZE defined in FreeRTOSConfig.h */
        ConsolePrintf( ptCommand->eConsolePort, "Heap used / total heap = %ld / %ld\r\n",
                       configTOTAL_HEAP_SIZE - (INT32)xPortGetFreeHeapSize(),
                       configTOTAL_HEAP_SIZE );

        ConsolePrintf( ptCommand->eConsolePort, "CONTROL_STACK_SIZE  = %lu\r\n", ControlGetStackSize() );
        //ConsolePrintf( ptCommand->eConsolePort, "LED_TASK_STACK_SIZE = %lu\r\n", gdwLedTaskStackSize );

    }

    //// DEV SECURITY command to print Flash EEPROM sucurity settings //////////////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SECURITY" ) == TRUE )
    {
        ConsolePrintf( ptCommand->eConsolePort, "FLASH User Option Bytes = %u\r\n", FLASH_OB_GetUser() );
        ConsolePrintf( ptCommand->eConsolePort, "FLASH Write Protection Option Bytes = 0x%08lX\r\n", FLASH_OB_GetWRP() );
        ConsolePrintf( ptCommand->eConsolePort, "FLASH Read out Protection Status = %u\r\n", FLASH_OB_GetRDP() );
        ConsolePrintf( ptCommand->eConsolePort, "FLASH BOR level = %u\r\n", FLASH_OB_GetBOR() );
    }
    //// DEV FIRMWARE_UPLOAD command to write a 1MB firmware image to DataFlash ///////////////////////////
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FIRMWARE_UPLOAD" ) == TRUE )
    {
      if ((ptCommand->pcArgArray[ 2 ] == NULL) ||
          (ptCommand->pcArgArray[ 3 ] == NULL) ||
          (ptCommand->pcArgArray[ 4 ] == NULL))
      {
        eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
      }
      else
      {
        UINT16 wNumTotalPackets     = strtoul(ptCommand->pcArgArray[ 2 ], NULL, 16);
        UINT16 wPacketSizeBytes     = strtoul(ptCommand->pcArgArray[ 3 ], NULL, 16);
        UINT32 dwTransferSizeBytes  = strtoul(ptCommand->pcArgArray[ 4 ], NULL, 16);
        // Initialize the loader module
        LoaderInit();
        UINT32 dwProgramFlashSizeBytes = (*((UINT16*)(BOOTLOADER_PROGRAM_FLASH_SIZE_REGISTER))) * 1024;
//        // Place the Transfer Size in bytes into the PSRAM immediately after the firmware image
//        UINT32* pdwParams = (UINT32*)(FSMC_SRAM_ADDRESS + dwProgramFlashSizeBytes);
//        *(pdwParams++) = dwTransferSizeBytes;
//        // Clear the contents of the Firmware Image Buffer.
//        memset((UINT8*)FSMC_SRAM_ADDRESS,0xFF,dwProgramFlashSizeBytes);
        if (!LoaderTransferRequestToDataFlash(wNumTotalPackets, wPacketSizeBytes, dwTransferSizeBytes, DFF_FILE_TYPE_FIRMWARE, ptCommand->eConsolePort))
        {
          eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
        }
        else if (!LoaderTransferRecv(ptCommand->eConsolePort))
        {
          eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
        }
      }
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SENDFW2CAM" ) == TRUE )
    {
      CAMERA_commandUpgradeFirmware(ptCommand->eConsolePort);
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL CommandHandlerFile( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes "FILE" commands.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     BOOLEAN
//!             - TRUE if the command was executed successfully and the console will report "OK"
//!               upon completion.
//!             - FALSE if not and the console will report "ERROR" upon completion.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerFile( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        /* Empty command. Display a command list. */
        ConsolePrintf( ptCommand->eConsolePort, "\r\nFILE Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "        replace <FILE_TYPE_STRING> for the following options:\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "                < " );
        
        for( DffFileTypeEnum eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
        {            
            ConsolePrintf( ptCommand->eConsolePort, "%s", DFF_getFileTypeNameString( eFileType ) );
            if( eFileType < (DFF_NUM_FILE_TYPES-1) )
            {
                ConsolePrintf( ptCommand->eConsolePort, " | " );
            }
        }
        ConsolePrintf( ptCommand->eConsolePort, " >\r\n" );               
        //ConsolePrintf( ptCommand->eConsolePort, "FILE DIR < ALL | <FILE_TYPE_STRING> > - Displays file dir information\r\n" );
//        FILE DIR PRINT ALL/TYPE
//        FILE DIR CLEAR TYPE        
//        
//        FILE INFO <>
//        FILE READ
//        FILE READB
//        FILE PAGE READ
//        FILE OPEN
//        FILE CURRENTSS
//        FILE WRITE
//        FILE CLOSE


        //ConsolePrintf( ptCommand->eConsolePort, "FILE DIR PRINT - Displays the directory List\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DIR < ALL | <FILE_TYPE_STRING> > CLEAR - Clear All files from selected directory\r\n" );
        //ConsolePrintf( ptCommand->eConsolePort, "FILE DIR FILES - Print the List of file in a Directory\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "FILE DIR < ALL | <FILE_TYPE_STRING> > - Displays file dir information\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "FILE SUB <" );        
//        ConsolePrintf( ptCommand->eConsolePort, "%s", DFF_getFileTypeNameString( DFF_FILE_TYPE_SENSOR_DATA ) );
//        ConsolePrintf( ptCommand->eConsolePort, " | " );
//        ConsolePrintf( ptCommand->eConsolePort, "%s", DFF_getFileTypeNameString( DFF_FILE_TYPE_EVENT ) );                
//        ConsolePrintf( ptCommand->eConsolePort, "> - Get the subsector under use of the current open file.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE READ <FILE_TYPE_STRING> <subsector#> - Read a txt file at subsector ss, printing to the console\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE READB <FILE_TYPE_STRING> <subsector#> - Read a binary (closed)file at subsector ss, printing hex to the console\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "FILE MAKE size   - Create a file in dataflash, size kBytes\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "FILE WRITE string- Write a string to the data log file\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        //ConsolePrintf( ptCommand->eConsolePort, "FILE FTP PUT ss  - Queue a file for FTP upload (starting at subsector ss via FTP\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DF INFO     - Show dataflash size and ID\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DF READ addr        - Read dataflash at address, 256 bytes \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DF READPAGE page    - Read dataflash page\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DF READSS subsector - Read dataflash subsector\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DF TEST     - Test dataflash ICs\r\n" );
        
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "DATALOG (DL)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DL INFO <FILE_TYPE_STRING> - Info of a File\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DL OPEN <FILE_TYPE_STRING> - Opens a File\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DL GETOSS <FILE_TYPE_STRING> - Returns the subsector used by an Open File\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DL CLOSE <FILE_TYPE_STRING> - Close a File\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FILE DL WRITE <FILE_TYPE_STRING> <TYPED TIMEOUT(ms)>- Writes to a File and stops if no chars entered after Time out\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "DL" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 4 )
        {
            // identify the file type
            DffFileTypeEnum     eFileType     = 0;
            DatalogFilesType    eDatalogType  = 0;
            BOOL                fIsTypeValid  = FALSE;
            
            // identify the file type
            for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
            {
                // check if file is valid
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], DFF_getFileTypeNameString( eFileType ) ) )
                {                        
                    fIsTypeValid  = TRUE;
                    break;
                }                    
            }          

            if(fIsTypeValid)
            {                
                // convert file type to datalog type
                // WARNING!! since all file types are mappen in datalog, the enum translation between them has the same value.
                eDatalogType = eFileType;

                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "INFO" ) == TRUE )
                {
                    UINT16                  wFileSubsectorAddress   = 0;
                    UINT32                  dwFileBytes             = 0;
                    CHAR                    cStorageFormat          = '?';
                    DatalogDataFormatEnum   eDataFormat;
                    CHAR                    cDataFormat;
 
                    UINT16                  wFileStartSubsector     = 0;
                    UINT16                  wFileEndSubsector       = 0;

                    DatalogGetLatestFileSubsector( eDatalogType, &wFileSubsectorAddress );
                    DatalogGetLatestFileBytesWritten( eDatalogType, &dwFileBytes );
                    DatalogGetFileDataFormat( eDatalogType, &eDataFormat );
                    DatalogGetFileStartSubsector( eDatalogType, &wFileStartSubsector );
                    DatalogGetFileEndSubsector( eDatalogType, &wFileEndSubsector );
                    DatalogGetFileStorageMode( eDatalogType, &cStorageFormat );

                    if( eDataFormat == DATALOG_DATA_FORMAT_BIN )
                    {
                        cDataFormat = 'B';
                    }
                    else
                    {
                        cDataFormat = 'T';
                    }                                                           

                    ConsolePrintf( ptCommand->eConsolePort, " Type              = %s\r\n", DFF_getFileTypeNameString( eFileType ) );
                    ConsolePrintf( ptCommand->eConsolePort, " Current Subsector = %d\r\n", wFileSubsectorAddress );  
                    ConsolePrintf( ptCommand->eConsolePort, " Start Subsector   = %d\r\n", wFileStartSubsector );  
                    ConsolePrintf( ptCommand->eConsolePort, " End Subsector     = %d\r\n", wFileEndSubsector );  
                    ConsolePrintf( ptCommand->eConsolePort, " Stat              = %s\r\n", DatalogGetFileStatNameString( eDatalogType ) );
                    ConsolePrintf( ptCommand->eConsolePort, " Data Format       = %c ( B=bin  | T=txt )\r\n", cDataFormat );                                      
                    ConsolePrintf( ptCommand->eConsolePort, " Bytes Written     = %d\r\n", dwFileBytes );                                      
                    ConsolePrintf( ptCommand->eConsolePort, " Open Mode         = %c ( F=flat | C=circular )\r\n", cStorageFormat );                                      
                }
                else
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "OPEN" ) == TRUE )
                {
                    if( DatalogOpenForWrite( eDatalogType ) == FALSE )
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Make sure file is init or closed.\r\n" );
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }                    
                }
                else
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "GETOSS" ) == TRUE )
                {
                    UINT16 wNewFileAddress;
                                        
                    if( DatalogGetLatestFileSubsector( eDatalogType, &wNewFileAddress ) == FALSE )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                    else
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Current Open File SS=%d\r\n", wNewFileAddress );
                    }
                }
                else
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CLOSE" ) == TRUE )
                {                    
                    if( DatalogCloseForWrite( eDatalogType ) == FALSE )
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Make sure file is open.\r\n" );
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }                    
                }
                else
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "WRITE" ) == TRUE )
                {
                    UINT32  dwTimeOut_ms    = 0;
                    TIMER   tTimer;
                    CHAR    cChar;
                    BOOL    fTerminateLoop  = FALSE;

                    if( ptCommand->bNumArgs == 5 )
                    {
                        dwTimeOut_ms = atoi( ptCommand->pcArgArray[ 4 ] );

                        tTimer = TimerDownTimerStartMs( dwTimeOut_ms );

                        while( fTerminateLoop == FALSE )
                        {
                            if( TimerDownTimerIsExpired( tTimer ) )
                            {
                                fTerminateLoop = TRUE;
                            }

                            while( UsartGetChar( TARGET_USART_PORT_TO_CONSOLE, &cChar, 0 ) )
                            {
                                // restart time out
                                tTimer = TimerDownTimerStartMs( dwTimeOut_ms );
                                
                                UsartPut( TARGET_USART_PORT_TO_CONSOLE, cChar );

                                // put char into DF file type
                                if( DatalogWriteBuffer( eDatalogType, &cChar, sizeof(cChar) ) == FALSE )
                                {
                                    fTerminateLoop = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
        }                 
    }    
    // "FILE SUB <LOG | EVT>" command ---------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SUB" ) == TRUE )
//    {
//        if( ptCommand->bNumArgs == 3 )
//        {
//            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "LOG" ) == TRUE )                 
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "Current write Subsector = %d\r\n", DFF_writeStartSubsector( DFF_FILE_TYPE_SENSOR_DATA ) );                    
//            }
//            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "EVT" ) == TRUE )                 
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "Current write Subsector = %d\r\n", DFF_writeStartSubsector( DFF_FILE_TYPE_EVENT ) );
//            }
//            else
//            {
//                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
//            }
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }
    // "FILE DF x" DataFlash commands ---------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "DF" ) == TRUE )
    {
        // "FILE DF INFO" command ---------------------------------------------------------
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "INFO" ) == TRUE )
        {
            /* Read dataflash info/ID command */
            UINT8 c;
            DATAFLASH_INFO_STRUCT tDataflashInfo;

            ConsolePrintf( ptCommand->eConsolePort, "DataFlash Info\r\n" );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_IC_SIZE_BYTES        = %lu\r\n\n", DATAFLASH_IC_SIZE_BYTES        );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_NUM_SECTORS          = %u\r\n",    DATAFLASH_NUM_SECTORS          );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_SECTOR_SIZE_PAGES    = %u\r\n",    DATAFLASH_SECTOR_SIZE_PAGES    );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_SECTOR_SIZE_BYTES    = %lu\r\n\n", DATAFLASH_SECTOR_SIZE_BYTES    );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_NUM_SUBSECTORS       = %u\r\n",    DATAFLASH_NUM_SUBSECTORS       );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_SUBSECTOR_SIZE_PAGES = %u\r\n",    DATAFLASH_SUBSECTOR_SIZE_PAGES );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_SUBSECTOR_SIZE_BYTES = %u\r\n\n",  DATAFLASH_SUBSECTOR_SIZE_BYTES );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_NUM_PAGES            = %lu\r\n",   DATAFLASH_NUM_PAGES            );
            ConsolePrintf( ptCommand->eConsolePort, "FLASH_PAGE_SIZE_BYTES      = %u\r\n\n",  DATAFLASH_PAGE_SIZE_BYTES      );

            c = Dataflash_ReadDeviceID( DATAFLASH0_ID, &tDataflashInfo );
            ConsolePrintf( ptCommand->eConsolePort, "Manufacturer ID = 0x%02X\r\n", tDataflashInfo.bManufacturer );
            ConsolePrintf( ptCommand->eConsolePort, "Memory type     = 0x%02X\r\n", tDataflashInfo.bMemoryType );
            ConsolePrintf( ptCommand->eConsolePort, "Capacity        = 0x%02X\r\n", tDataflashInfo.bCapacity );
            ConsolePrintf( ptCommand->eConsolePort, "ID/Factory Len  = %u\r\n", tDataflashInfo.bUIDLen );
            ConsolePrintf( ptCommand->eConsolePort, "Ext device ID   = 0x%02X%02X\r\n",  tDataflashInfo.bEDID[ 0 ],  tDataflashInfo.bEDID[ 1 ] );

        }

        // "FILE DF READPAGE" command ---------------------------------------------------------
        else if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "READPAGE" ) == TRUE ) ||
                 ( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RP" ) == TRUE ) )
        {
            UINT8  pbBuffer[ DATAFLASH_PAGE_SIZE_BYTES ];
            UINT8  bResult;
            UINT16 wPage;

            wPage = atoi( ptCommand->pcArgArray[ 3 ] );

            /* Read memory page */
            bResult = Dataflash_ReadPage( DATAFLASH0_ID, wPage, pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
            if( bResult != DATAFLASH_OK )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Dataflash Error %u\r\n", bResult );
            }
            else
            {
                GeneralHexDump( ptCommand->eConsolePort, (UINT32)( wPage * DATAFLASH_PAGE_SIZE_BYTES ),
                                pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
            }
        }
        // "FILE DF READSUBSECTOR" command ---------------------------------------------------------
        else if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "READSS" ) == TRUE ) ||
                 ( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "RSS" ) == TRUE ) )
        {
            UINT8  pbBuffer[ DATAFLASH_PAGE_SIZE_BYTES ];
            UINT8  bResult;
            UINT16 wPage;
            UINT16 wSubSector;
            INT16  i;

            wSubSector = atoi( ptCommand->pcArgArray[ 3 ] );
            wPage = wSubSector * DATAFLASH_SUBSECTOR_SIZE_PAGES;

            for( i = 0; i < DATAFLASH_SUBSECTOR_SIZE_PAGES; i++ )
            {
                /* Read memory page */
                bResult = Dataflash_ReadPage( DATAFLASH0_ID, wPage, pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
                if( bResult != DATAFLASH_OK )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Dataflash Error %u\r\n", bResult );
                }
                else
                {
                    GeneralHexDump( ptCommand->eConsolePort, (UINT32)( wPage * DATAFLASH_PAGE_SIZE_BYTES ),
                                     pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
                }

                wPage++;
            }
        }
        // "FILE DF READ" command ---------------------------------------------------------
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "READ" ) == TRUE )
        {
            UINT8  pbBuffer[ DATAFLASH_PAGE_SIZE_BYTES ];
            UINT8  bResult;
            UINT32 dwAddress;

            bResult = GeneralHexStringToUINT32( ptCommand->pcArgArray[ 3 ], &dwAddress );
            if( bResult != TRUE )
            {
                return FALSE;
            }

            /* Read memory page */
            bResult = Dataflash_Read( DATAFLASH0_ID, dwAddress, pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
            if( bResult != DATAFLASH_OK )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Dataflash Error %u\r\n", bResult );
            }
            else
            {
                GeneralHexDump( ptCommand->eConsolePort, dwAddress, pbBuffer, DATAFLASH_PAGE_SIZE_BYTES );
            }
        }

        // "FILE DF TEST" command ---------------------------------------------------------
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "TEST" ) == TRUE )
        {
            UINT8 c;
            c = Dataflash_TestICs();
            ConsolePrintf( ptCommand->eConsolePort, "Dataflash_TestICs()=%u\r\n", c );
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Unknown DF command!\r\n" );
            return FALSE;
        }
    }
    // end of FILE DF commands --------------------------------------------------------------

    // "FILE CLOSE" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CLOSE" ) == TRUE )
//    {
//        return DFF_writefileClose( DFF_FILE_TYPE_SENSOR_DATA );
//    }
    // "FILE DIR" command --------------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "DIR" ) == TRUE )
    {
        //< ALL | LOG | IMG | EVT | BACKUP | SCRIPT | SCRESULT | FW >
        if( ptCommand->bNumArgs == 3 )
        {
            // identify the file type
            DffFileTypeEnum eFileType         = 0;            
            UINT8           bFileTypeBitMask = 0;

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
            {
                bFileTypeBitMask = 0xFF;                
            }
            else
            {
                // identify the file type
                for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
                {
                    // check if file is valid
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], DFF_getFileTypeNameString( eFileType ) ) )
                    {                                                
                        bFileTypeBitMask = (1<<eFileType);   
                        break;
                    }                    
                }
            }

            if( bFileTypeBitMask > 0 )
            {
                for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
                {                    
                    if( ( bFileTypeBitMask & (1<<eFileType) ) > 0 )
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "DirName[%d]:(%s)\r\n", eFileType, DFF_getFileTypeNameString( eFileType ) );                    

                        DFF_fileDirPrint( ptCommand->eConsolePort, eFileType );
                    }
                }
            }            
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        //FILE DIR < ALL | <FILE_TYPE_STRING> > CLEAR
        else if( ptCommand->bNumArgs == 4 )
        {
            DffFileTypeEnum eFileType           = 0;            
            UINT8           bFileTypeBitMask    = 0;
            UINT16          wFileStartSubsector = 0;
            UINT16          wFileEndSubsector   = 0;
            UINT16          wSubSectorTotal     = 0;
            UINT8           bBuffer[100];

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ALL" ) == TRUE )
            {
                bFileTypeBitMask = 0xFF;                
            }
            else
            {
                // identify the file type
                for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
                {
                    // check if file is valid
                    if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], DFF_getFileTypeNameString( eFileType ) ) )
                    {                                                
                        bFileTypeBitMask = (1<<eFileType);   
                        break;
                    }                    
                }
            }

            if( bFileTypeBitMask > 0 )
            {
                for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
                {                    
                    if( ( bFileTypeBitMask & (1<<eFileType) ) > 0 )
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "This operation might take several seconds.(Depending on Dir Size)\r\n" );
                        ConsolePrintf( ptCommand->eConsolePort, "Dir: %s \r\n", DFF_getFileTypeNameString( eFileType ) );
                        DatalogGetFileStartSubsector( eFileType, &wFileStartSubsector );
                        DatalogGetFileEndSubsector( eFileType, &wFileEndSubsector );
                        wSubSectorTotal = wFileEndSubsector - wFileStartSubsector;
                        // for this file type, instantiate start subsector and end susbsector
                        for( UINT16 wCurrentSubsector = wFileStartSubsector ; wCurrentSubsector <= wFileEndSubsector ; wCurrentSubsector++ )
                        {                           
                            ConsolePrintf( ptCommand->eConsolePort, "(%d/%d)\r\n", (wCurrentSubsector - wFileStartSubsector), wSubSectorTotal );
                            // clear the flash mem. for the directories selected
                            Dataflash_EraseSubsector( DATAFLASH0_ID, wCurrentSubsector );

                            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );
                        }
                    }
                }
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
        }
    }
    // "FILE INFO" command --------------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INFO" ) == TRUE )
    {
        ConsolePrintf( ptCommand->eConsolePort, "File Info\r\n" );
        DFF_fileInfoPrint( DFF_FILE_TYPE_ADC_HOUR );        
        DFF_fileInfoPrint( DFF_FILE_TYPE_IMG );
        FTPQ_statusPrint( ptCommand->eConsolePort );
    }
    // "FILE MAKE" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "MAKE" ) == TRUE )
//    {
//        UINT16 u, v, w, x;
//        UINT8 pszExtension[ 5 ];
//        UINT8 pszString[ 80 ];
//        BOOL fPass;
//
//        /* Make a binary file for testing */
//        ConsolePrintf( ptCommand->eConsolePort, "File Make\r\n" );
//
//        if( ptCommand->bNumArgs != 3 )
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Invalid # parameters\r\n" );
//            return FALSE;
//        }
//
//        u = (UINT16)atol( ptCommand->pcArgArray[ 2 ] );
//        ConsolePrintf( ptCommand->eConsolePort, "Writing a %uk file\r\n", u );
//
//        sprintf( pszExtension, "IMG" );     // Set extension type
//        DFF_writefileOpen( DFF_FILE_TYPE_IMG, pszExtension, DFF_FILE_STORAGE_TYPE_FLAT, ( u * 1024 ) );
//
//        if( DFF_fileStatus( DFF_FILE_TYPE_IMG ) != DFF_FILE_OPEN )
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Dataflash File Open Error %u\r\n", DFF_fileError( DFF_FILE_TYPE_IMG ) );
//            return FALSE;
//        }
//
//        /* Create string, 64 characters long */
//        for( x = 0; x < 62; x++ )
//        {
//            pszString[ x ] = GENERAL_HEX2ASC( x % 16 );
//        }
//        pszString[ 62 ] = '\r'; /* End string with a CRLF */
//        pszString[ 63 ] = '\n';
//        pszString[ 64 ] = '\0'; /* Terminate with a NULL */
//
//        /* Write file. 16 strings make 1k of data (64 bytes each). */
//        for( v = 0; v < ( u * 16 ); v++ )
//        {
//            /* Put offset at start of string */
//            sprintf( pszString, "%08lX", ( (UINT32)v ) * 64 );
//            pszString[ 8 ] = ':';   /* Replace NULL with a colon, string is still 64 bytes long */
//
//            DFF_writeBuffer( DFF_FILE_TYPE_IMG, pszString, 64 );  /* Write 64 bytes */
//
//            if( DFF_fileStatus( DFF_FILE_TYPE_IMG ) != DFF_FILE_OPEN )
//            {
//                u = DFF_fileStatus( DFF_FILE_TYPE_IMG );
//                w = DFF_fileError( DFF_FILE_TYPE_IMG );
//                ConsolePrintf( ptCommand->eConsolePort, "File Write Error (%u,%u=\"%s\")\r\n", u, w,
//                        DataflashGetErrorAsString( w ) );
//                return FALSE;
//            }
//
//            if( ( v % 16 ) == 0 )
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "%u,", v / 16 );
//            }
//
//            SystemWatchdogRestart();     // Reset watchdog in case read is long
//        }
//
//        DFF_writefileClose( DFF_FILE_TYPE_IMG );
//        ConsolePrintf( ptCommand->eConsolePort, "Done\r\n", u );
//
//    }
    // "FILE READ FILE_TYPE SS" command --------------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "READ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            #define READ_STR_SIZE (160)
            UINT8  bBuffer[ READ_STR_SIZE ];
            BOOL   fEofReached, fPass;
            UINT32 dwLengthRead;
            UINT16 wSubsector;
            UINT16 u;
            DatalogFilesType eDatalogFileType;
            DatalogDataFormatEnum eDataFormat;


            // identify the file type
            DffFileTypeEnum eFileType         = 0;
            BOOL            fIsTypeValid  = FALSE;

            // identify the file type
            for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
            {
                // check if file is valid
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], DFF_getFileTypeNameString( eFileType ) ) )
                {                        
                    fIsTypeValid  = TRUE;
                    break;
                }                    
            }          

            if(fIsTypeValid)
            {                
                // extract subsector                
                /* File subsector specified */
                wSubsector = (UINT16)atol( ptCommand->pcArgArray[ 3 ] );
                
                // change DFF_FileType for DatalogFilesType                
                eDatalogFileType    = eFileType;                
                fEofReached         = FALSE;
                fPass               = TRUE;

                DatalogGetFileDataFormat( eDatalogFileType, &eDataFormat );

                if( eDataFormat == DATALOG_DATA_FORMAT_TEXT )
                {
                    if( DatalogOpenForRead( eDatalogFileType, wSubsector ) )
                    {                        
                        while( ( fPass == TRUE ) && ( fEofReached == FALSE ) )
                        {
                            fPass = DatalogReadfileData( eDatalogFileType, &bBuffer[0], sizeof(bBuffer), &dwLengthRead, &fEofReached );

                            for( u = 0; u < dwLengthRead; u++ )
                            {
                                ConsolePutChar( ptCommand->eConsolePort,  bBuffer[ u ], TRUE );    // Send char with console lock requested
                            }

                            SystemWatchdogRestart();     // Reset watchdog in case read is long
                        }
                    }
                    else
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Error Opening File\r\n" );
                    }
                }
                else
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Invalid Data format.Not a text file\r\n" );
                }
            }
            else                
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }  
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;   
        }
    }

    // "FILE READB" binary file read command ------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "READB" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            #define READ_STR_SIZE (160)
            UINT8  bBuffer[ READ_STR_SIZE ];
            BOOL   fEofReached, fPass;
            UINT32 dwLengthRead;
            UINT16 wSubsector;
            UINT16 u;
            DatalogFilesType eDatalogFileType;
            DatalogDataFormatEnum eDataFormat;


            // identify the file type
            DffFileTypeEnum eFileType         = 0;
            BOOL            fIsTypeValid  = FALSE;

            // identify the file type
            for( eFileType = 0 ; eFileType < DFF_NUM_FILE_TYPES ; eFileType++ )
            {
                // check if file is valid
                if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], DFF_getFileTypeNameString( eFileType ) ) )
                {                        
                    fIsTypeValid  = TRUE;
                    break;
                }                    
            }          

            if(fIsTypeValid)
            {                
                // extract subsector                
                /* File subsector specified */
                wSubsector = (UINT16)atol( ptCommand->pcArgArray[ 3 ] );
                
                // change DFF_FileType for DatalogFilesType                
                eDatalogFileType    = eFileType;                
                fEofReached         = FALSE;
                fPass               = TRUE;
                UINT32 dwReadOffset = 0;            
                UINT8  cIsEscKey;                
                
                if( DatalogOpenForRead( eDatalogFileType, wSubsector ) )
                {                        
                    while( ( fPass == TRUE ) && ( fEofReached == FALSE ) )
                    {
                        fPass = DatalogReadfileData( eDatalogFileType, &bBuffer[0], sizeof(bBuffer), &dwLengthRead, &fEofReached );

                        GeneralHexDump( ptCommand->eConsolePort, dwReadOffset, &bBuffer[0], dwLengthRead );

                        dwReadOffset = dwReadOffset + dwLengthRead;  
                        
                        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

                        SystemWatchdogRestart();     // Reset watchdog in case read is long
                        
                        // break loop if Escape Key received
                        if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                        {
                            if( cIsEscKey == COMMAND_ESC_KEY )
                            {
                                ConsolePrintf( ptCommand->eConsolePort, "[ESC] key pressed\r\n" );
                                break;
                            }
                        }
                    }
                }
                else
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Error Opening File\r\n" );
                }                
            }
            else                
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }  
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;   
        }
    }
    // "FILE WRITE" -------------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "WRITE" ) == TRUE )
//    {
//        DatalogWriteString( DATALOG_FILE_SENSOR_DATA, ptCommand->pcArgArray[ 2 ], TRUE );
//    }

//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "FTP" ) == TRUE )
//    {
//        // "FILE FTP PUT" command --------------------------------------------------------------
//        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PUT" ) == TRUE )
//        {
//            /* Send log file to the FTP server using FTP PUT */
//            UINT16 bFileType;
//            UINT16 wFileSubsector;
//            BOOL fPass;
//
//            /* Send a file via FTP */
//            if( ptCommand->bNumArgs != 4 )
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "Invalid # parameters\r\n" );
//                return FALSE;
//            }
//
//            wFileSubsector = (UINT16)atol( ptCommand->pcArgArray[ 3 ] );
//            ConsolePrintf( ptCommand->eConsolePort, "Sending file %wFileSubsector via FTP\r\n", wFileSubsector );
//
//            /* wFileSubsector is the file start subsector. bFileType is the file type */
//
//            if( wFileSubsector >= DATAFLASH_NUM_SUBSECTORS )
//            {
//                /* Illegal value */
//                return FALSE;
//            }
//
//            if( ( wFileSubsector >= DFF_DATALOG_SUBSECTOR_START ) && ( wFileSubsector <= DFF_DATALOG_SUBSECTOR_END ) )
//            {
//                bFileType = DFF_FILE_TYPE_LOG;
//            }
////            else if( wFileSubsector < DFF_GPS_SUBSECTOR_END )
////            {
////                bFileType = DFF_FILE_TYPE_GPS;
////            }
//            else if( wFileSubsector < DFF_IMG_SUBSECTOR_END )
//            {
//                bFileType = DFF_FILE_TYPE_BINARY;
//            }
//            else
//            {
//                return FALSE;
//            }
//
//            /* Future - check if subsector wFileSubsector is a valid start of file */
//
//            fPass = FTPQ_ftpFileQueueForPut( bFileType, wFileSubsector );
//            if( fPass == TRUE )
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "File %wFileSubsector queued to send\r\n", wFileSubsector );
//            }
//            else
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "File queueing error\r\n", wFileSubsector );
//            }
//        }
#if 0   // FTP GET Command Not Implemented Yet ///////////////////////////////////////////////
        // "FILE FTP GET" command --------------------------------------------------------------
//        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "GET" ) == TRUE )
//        {
//            UINT8  pFileNameString[ 32 ];
//            UINT16 u;
//            BOOL   fPass;
//
//            u = 0;
//            if( ptCommand->bNumArgs == 4 )
//            {
//                u = (UINT16)atol( ptCommand->pcArgArray[ 3 ] );  /* Get option number, >= 1 to get 64k file */
//            }
//
//            /* Set file name to GET */
//            if( ( u == 1 ) || ( u == 0 ) )
//            {
//                sConsolePrintf( ptCommand->eConsolePort, pFileNameString, "binary1k.txt" );
//            }
//            else if( u == 8 )
//            {
//                sConsolePrintf( ptCommand->eConsolePort, pFileNameString, "binary8k.txt" );
//            }
//            else if( u == 64 )
//            {
//                sConsolePrintf( ptCommand->eConsolePort, pFileNameString, "binary64k.txt" );
//            }
//            else if( u == 1024 )
//            {
//                sConsolePrintf( ptCommand->eConsolePort, pFileNameString, "binary1024k.txt" );
//            }
//            else
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "FTP filename unknown for that size\r\n" );
//                return FALSE;
//            }
//            ConsolePrintf( ptCommand->eConsolePort, "Requesting file %s via FTP\r\n", pFileNameString );
//
//            /* Request file from FTP server */
//            fPass = FTPQ_ftpFileQueueForGet( DFF_FILE_TYPE_BINARY, pFileNameString, 0, 128*1024UL, TRUE );
//            if( fPass != TRUE )
//            {
//                ConsolePrintf( ptCommand->eConsolePort, "FTP queue error\r\n" );
//            }
//        }
#endif  // FTP Command Not Implemented Yet ///////////////////////////////////////////////

    //}

    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerGpio( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerGpio( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "GPIO Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO STAT                     - Shows the status of all digital inputs and outputs\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "GPIO STAT <PortLetter> <Pin#> - Shows the status of a single pin\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO PORT <PortLetter>        - Shows the status of the pins mapped in the Gpio module and that belogn to the defined port<LETTER> (e.g. PORT A)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO CONT PORT <PortLetter> [PERIOD] - Print Logic Level (0=LOW 1=HIGH) continuously at portLetter every PERIOD milliseconds\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = 1000 ms{Min = 10 ms}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO CONT SINGLE <PortLetter> <Pin#> [PERIOD] - Print the Logic Level (0=LOW 1=HIGH) of a single GPIO pin every PERIOD milliseconds\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = 1000 ms{Min = 10 ms}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO GET  <PortLetter> <Pin#>  - Get the pin Logic level (only if Mode is INPUT, OUTPUT or AF)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO SET  <PortLetter> <Pin#> <1 | 0>  - Set an OUTPUT pin HIGH or LOW (1=HIGH, 0=LOW)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "GPIO MODE <PortLetter> <Pin#> <MODE#> - Set pin as INPUT, OUTPUT, ALTERNATIVE FUNCTION(AF) OR ANALOG \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "          MODE #0 = INPUT\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "          MODE #1 = OUTPUT\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "          MODE #2 = AF     (Not supported by all pins. Check Datasheet)\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "          MODE #3 = ANALOG (Not supported by all pins. Check Datasheet)\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] == NULL )
        {            
            GpioPrintAll( ptCommand->eConsolePort );
        }
        else
        {
            if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) )
            {
                // <PortLetter> <Pin#>
                CHAR    cPortLetter = 0 ;
                UINT8   bPinNumber  = 0 ;
                GpioPortEnum    eGpioPort;                

                cPortLetter  = *ptCommand->pcArgArray[ 2 ];
                bPinNumber   = atoi( ptCommand->pcArgArray[ 3 ] );                

                if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
                {                    
                    ConsolePrintf( ptCommand->eConsolePort, "\r\nPIN   MODE     OTYPE      PUpPDown     IN   OUT  MbName                      InterfName                Id\r\n");
                    GpioPrintSinglePin( ptCommand->eConsolePort, eGpioPort, bPinNumber );                    
                }
                else
                {                
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONT" ) == TRUE )
    {
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PORT" ) == TRUE )
        {
            if( ptCommand->pcArgArray[ 3 ] != NULL )
            {
                UINT32          dwPeriod_ms = 1000;
                TIMER           tTimer      = 0;
                CHAR            cIsEscKey   = 0;
                BOOL            fVoltLevel  = FALSE;
                CHAR            cPortLetter = 0 ;
                GpioPortEnum    eGpioPort;

                cPortLetter = *ptCommand->pcArgArray[ 3 ];
                
                if( ptCommand->pcArgArray[ 4 ] != NULL )
                {//// [PERIOD] ////
                    dwPeriod_ms = atoi( ptCommand->pcArgArray[ 4 ] );
                    // validate period
                    dwPeriod_ms = dwPeriod_ms > 10 ? dwPeriod_ms : 10;
                }

                if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
                {
                    tTimer = TimerDownTimerStartMs( 1000 );
                    ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
                    while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                    tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                    while(1)
                    {
                        WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                        if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                        {
                            tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                            // print pin status  
                            for( UINT8 i = 0 ; i < 16 ; i++ )
                            {                            
                                if( GpioInputRead( eGpioPort, i , &fVoltLevel ) )
                                {
                                    ConsolePrintf( ptCommand->eConsolePort, "P%c%02u=[%d]\r\n",cPortLetter, i, fVoltLevel );
                                }
                            }                        
                            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
                        }

                        // break loop if Escape Key received
                        if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                        {
                            if( cIsEscKey == COMMAND_ESC_KEY )
                            {
                                break;
                            }
                        }

                    }// end while loop
                }
                
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
        }
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SINGLE" ) == TRUE )
        {
            UINT32          dwPeriod_ms = 1000;
            TIMER           tTimer      = 0;
            CHAR            cIsEscKey   = 0;
            BOOL            fLogicLevel = FALSE;
            UINT8           bPinNumber  = 0 ;
            CHAR            cPortLetter = 0 ;
            GpioPortEnum    eGpioPort;

            // <PortLetter> <Pin#>
            if( ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
            {
                cPortLetter = *ptCommand->pcArgArray[ 3 ];
                bPinNumber  = atoi( ptCommand->pcArgArray[ 4 ] );                

                if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
                {
                    if( ptCommand->pcArgArray[ 5 ] != NULL )
                    {//// [PERIOD] ////
                        dwPeriod_ms = atoi( ptCommand->pcArgArray[ 5 ] );
                        // validate period
                        dwPeriod_ms = dwPeriod_ms > 10 ? dwPeriod_ms : 10;
                    }

                    tTimer = TimerDownTimerStartMs( 1000 );
                    ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
                    while( TimerDownTimerIsExpired( tTimer ) == FALSE );                    

                    tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                    while(1)
                    {
                        WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                        if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                        {
                            tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                            // print pin status
                            if( GpioInputRead( eGpioPort, bPinNumber , &fLogicLevel ) )
                            {
                                ConsolePrintf( ptCommand->eConsolePort, "P%c%02u=[%d]\r\n",cPortLetter, bPinNumber, fLogicLevel );
                            }
                            else
                            {
                                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                                break;
                            }

                            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
                        }

                        // break loop if Escape Key received
                        if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                        {
                            if( cIsEscKey == COMMAND_ESC_KEY )
                            {
                                break;
                            }
                        }

                    }// end while loop                                
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "GET" ) == TRUE )
    {
        // <PortLetter> <Pin#>
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) )
        {   //// <PIN_NUM> <1 | 0> ////                        
            BOOL            fLogicLevel = FALSE;
            UINT8           bPinNumber  = 0 ;
            CHAR            cPortLetter = 0 ;
            GpioPortEnum    eGpioPort;
            BOOL            fSuccess    = TRUE;            

            cPortLetter = *ptCommand->pcArgArray[ 2 ];
            bPinNumber  = atoi( ptCommand->pcArgArray[ 3 ] );                

            if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
            {
                if( GpioInputRead( eGpioPort, bPinNumber , &fLogicLevel ) )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "P%c%02u=[%d]\r\n",cPortLetter, bPinNumber, fLogicLevel );
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }    
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "MODE" ) == TRUE )
    {
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
        {
            //// <PIN_NUM> <1 | 0> ////
            // <PortLetter> <Pin#> <MODE#>            
            UINT8            bPinNumber = 0 ;
            CHAR             cPortLetter = 0 ;            
            GpioPortEnum     eGpioPort;
            UINT8            bGpioMode;
            BOOL             fSuccess   = TRUE;

            cPortLetter = *ptCommand->pcArgArray[ 2 ];
            bPinNumber  = atoi( ptCommand->pcArgArray[ 3 ] );                
            bGpioMode   = atoi( ptCommand->pcArgArray[ 4 ] );            

            if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
            {            
                if( GpioChangeMode( eGpioPort, bPinNumber, bGpioMode ) == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PORT" ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {
            UINT8           cPortLetter;
            GpioPortEnum    eGpioPort;
            
            cPortLetter = *ptCommand->pcArgArray[ 2 ];

            if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
            {            
                if( GpioPrintSinglePort( ptCommand->eConsolePort, eGpioPort ) == FALSE )
                {                    
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SET" ) == TRUE )
    {
        if( ( ptCommand->pcArgArray[ 2 ] != NULL ) && ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
        {
            // <PortLetter> <Pin#>
            CHAR    cPortLetter = 0 ;
            GpioPortEnum   eGpioPort;            
            UINT8   bPinNumber  = 0 ;
            BOOL    fLogicLevel = FALSE;
            BOOL    fSuccess    = TRUE;

            cPortLetter  = *ptCommand->pcArgArray[ 2 ];
            bPinNumber   = atoi( ptCommand->pcArgArray[ 3 ] ); 
            fLogicLevel  = atoi( ptCommand->pcArgArray[ 4 ] ); 

            if( GpioConvertPortLetterToPortNumber( cPortLetter, &eGpioPort ) == TRUE )
            {
                if( GpioOutputWrite( eGpioPort, bPinNumber, fLogicLevel ) == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleResultEnum CommandHandlerHelp( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleResultEnum result type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerHelp( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if ( ptCommand->bNumArgs == 1 )
    {
        DevInfoProductInfoType * ptProductInfo = NULL;

        ptProductInfo = DevInfoGetProductInfo();

        // print header
        ConsolePrintf( ptCommand->eConsolePort, "******************************************************************\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Product:    %s (c)   %s\r\n", ptProductInfo->pcProductName, ptProductInfo->pcCompanyName );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "Firmware:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\tVersion:         %d.%d.%d.%d\r\n",   ptProductInfo->tFwInfo.bVersionMajor,ptProductInfo->tFwInfo.bVersionMinor,ptProductInfo->tFwInfo.bVersionPoint, ptProductInfo->tFwInfo.bVersionLast );
        ConsolePrintf( ptCommand->eConsolePort, "\tRepo Revision:   r%d\r\n",           ptProductInfo->tFwInfo.dwSvnRevision );
        ConsolePrintf( ptCommand->eConsolePort, "\tCompile Time:    %s %s\r\n",         ptProductInfo->tFwInfo.pcCompileDate, ptProductInfo->tFwInfo.pcCompileTime );
        ConsolePrintf( ptCommand->eConsolePort, "\tFile Size:       %d\r\n",            ptProductInfo->tFwInfo.dwFwSize );
        ConsolePrintf( ptCommand->eConsolePort, "\tcrc32:           0x%08lX\r\n",       ptProductInfo->tFwInfo.dwCrc32Embed );

        ConsolePrintf( ptCommand->eConsolePort, "Hardware:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\tMCU Type:        %s\r\n",            ptProductInfo->tMcuInfo.pcMcuType );
        ConsolePrintf( ptCommand->eConsolePort, "\tMCU Silicon ver: %d\r\n",            ptProductInfo->tMcuInfo.dwSiliconVersion );
        ConsolePrintf( ptCommand->eConsolePort, "\tMCU Dev Id:      %d\r\n",            ptProductInfo->tMcuInfo.dwDeviceId );
        ConsolePrintf( ptCommand->eConsolePort, "\tMCU Sig Bytes:   0x%02lX 0x%02lX 0x%02lX\r\n", ptProductInfo->tMcuInfo.dwUniqueId[0],ptProductInfo->tMcuInfo.dwUniqueId[1], ptProductInfo->tMcuInfo.dwUniqueId[2] );
        ConsolePrintf( ptCommand->eConsolePort, "\tMCU Flash Size:  %d\r\n",            ptProductInfo->tMcuInfo.dwFlashMemSizeBytes );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "******************************************************************\r\n" );
        
        // print all the command list
        ConsolePrintCommandList ( ptCommand->eConsolePort );
    }
    else
    if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE )
    {
        Main_PrintSystemInfo( ptCommand->eConsolePort );
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "HELP Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "HELP ALL       -  Show help information for all commands\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "HELP HELP      -  This help information\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "<COMMAND> HELP -  Show command specific help information\r\n" );
    }
    else if( (ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ALL" ) == TRUE) || (ptCommand->bNumArgs == 2) )
    {
        BOOL fShowAll                               = FALSE;
        BOOL fCommandFound                          = FALSE;
        const ConsoleDictionary *ptDictionary       = NULL;
        const CHAR              *pcString[2] =
        {
            NULL,
            COMMAND_STRING__HELP
        };
        ConsoleCommandType      tConsoleCommand;

        if (ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ALL" ) == TRUE)
        {
            fShowAll = TRUE;
        }

        // set variables to be able to call HELP sub-command on every Command
        tConsoleCommand.eConsolePort    = ptCommand->eConsolePort;
        tConsoleCommand.bNumArgs        = 1;
        tConsoleCommand.pcArgArray      = pcString;
        ConsoleGetDictionaryPointer( ptCommand->eConsolePort, &ptDictionary );

        ///////////////////////////////////////////////////////
        // Print all commands with their respective subcommands of the current console!!
        // start printing after help command handler
        for( UINT32 dwCommandCounter = 1; ptDictionary[dwCommandCounter].pszCommandString != NULL ; dwCommandCounter++ )
        {
            if (fShowAll || (ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], ptDictionary[dwCommandCounter].pszCommandString)))
            {
                fCommandFound = TRUE;

                ConsolePrintf( ptCommand->eConsolePort, "=========================================================\r\n" );

                // call function pointer to sub command HELP
                ( ptDictionary[dwCommandCounter].pfnCommandHandler )( &tConsoleCommand );
            }
        }
        ///////////////////////////////////////////////////////

        if (!fCommandFound)
        {
            ConsolePrintf( ptCommand->eConsolePort, "Command not found!\r\n" );
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn         ConsoleErrorEnum CommandHandlerModem( ConsoleCommandType *ptCommand )
////!
////! \brief      Executes Modem commands.
////!
////! \param[in]  ConsoleCommandType *ptCommand command structure.
////!                 ptCommand->eConsolePort Port to print the command responses to.
////!                 ptCommand->bNumArgs     Number of tokens present in the command.
////!                 ptCommand->pcArgArray[] Array of strings which each hold a token
////!                                         from the command line.
////!
////! \return     ConsoleErrorEnum error type
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
//ConsoleResultEnum CommandHandlerModem( ConsoleCommandType *ptCommand )
//{
//    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;
//
//    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
//        ( ptCommand->bNumArgs == 1 ) )
//    {
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM Commands:\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM CONSOLE - Connect the modem to the console for direct control\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM STAT    - Display modem diagnostic info\r\n" );        
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM ON      - Force modem on\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM OFF     - Force modem to shut down\r\n" );
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM RSSI    - Request modem receive signal strength indicator\r\n" );        
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM CONNTEST <NUM OF CONN ATTEMPS> - Attempt to connect to network # amount of times.\r\n" );        
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM CONFIG LIST          - Show all the apn addresses to configure\r\n" );        
//        ConsolePrintf( ptCommand->eConsolePort, "MODEM CONFIG SET <CONFIG#> - Set a particular apn address in config for the modem to connect.\r\n" );        
//    }
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
//    {
//        // Modem status
//        MODEM_statusPrint( ptCommand->eConsolePort );
//    }           
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONNTEST" ) == TRUE )
//    {
//        if( ptCommand->bNumArgs == 3 )
//        {
//            UINT8 bNumOfAttempts = 0;
//
//            bNumOfAttempts = atoi( ptCommand->pcArgArray[ 2 ] );
//
//            BOOL    fTestPass = FALSE;
//            UINT8   bConnTestState = 0;
//            BOOL    fBreakLoop = FALSE;
//            UINT8   bCurrentConnectionAttemp = 1;
//
//            WatchdogEnable( FALSE, 0 );
//
//            while( bCurrentConnectionAttemp <= bNumOfAttempts )
//            {
//                MODEM_stateMachine();                               
//
//                switch( bConnTestState )
//                {
//                    case 0: // start powering on modem
//                        MODEM_commandPowerOn();
//                        bConnTestState++;
//                        break;
//                    case 1: // wait until it fails or get connected
//                        if( MODEM_isModemConnected() == TRUE )
//                        {
//                            fTestPass   = TRUE;                            
//                            eResult     = CONSOLE_RESULT_OK;
//                            bConnTestState = 2;
//                            MODEM_commandPowerOff();
//                        }
//                        if( MODEM_isModemOff() == TRUE )
//                        {                                                       
//                            bConnTestState = 0;
//                            bCurrentConnectionAttemp++;
//                        }                                               
//                        break;                    
//                    case 2: 
//                        if( MODEM_isModemOff()  )
//                        {                            
//                            fBreakLoop = TRUE;
//                        }                        
//                        break;
//
//                    default:
//                        fBreakLoop = TRUE;
//                        break;
//                }
//
//                if( fBreakLoop ) 
//                {
//                    break;
//                }
//            }
//
//            WatchdogEnable( TRUE, 10000 );
//
//            if( !fTestPass )
//            {
//                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
//            }
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }     
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONFIG" ) == TRUE )
//    {
//        if( ptCommand->bNumArgs >= 3 )
//        {
//            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "LIST" ) == TRUE )
//            {
//                // operations
//                for( ModemTelitApnAddressEnum i = 0 ; i < MODEM_TELIT_APN_ADDRESS_MAX ; i++ )
//                {
//                    ConsolePrintf( ptCommand->eConsolePort, "%02d %s\r\n", i, MODEM_getApnAddressString( i ) );
//                }
//            }
//            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SET" ) == TRUE )
//            {
//                if( ptCommand->bNumArgs == 4 )
//                {
//                    UINT8 bApnAddress = 0;
//
//                    bApnAddress = atoi( ptCommand->pcArgArray[ 3 ] );
//
//                    if( bApnAddress < MODEM_TELIT_APN_ADDRESS_MAX )
//                    {
//                        // operations
//                        ConfigParametersLoadByConfigType( CONFIG_TYPE_COMMUNICATIONS );
//                        ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_MODEM_APN_ADDRESS_STRING,  MODEM_getApnAddressString( bApnAddress ) );   
//                        ConfigParametersSaveByConfigType( CONFIG_TYPE_COMMUNICATIONS );
//                    }                    
//                    else
//                    {
//                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
//                    }
//                }
//                else
//                {
//                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//                }
//            }
//            else
//            {
//                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
//            }
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }
//    // "MODEM INFO" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INFO" ) == TRUE )
//    {
//        MODEM_statusPrint( ptCommand->eConsolePort );
//    }
//    // "MODEM OFF" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "OFF" ) == TRUE )
//    {
//        MODEM_commandPowerOff();
//        ConsolePrintf( ptCommand->eConsolePort, "Turning MODEM off\r\n" );
//    }
//    // "MODEM ON" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "ON" ) == TRUE )
//    {        
//        MODEM_commandPowerOn();
//        ConsolePrintf( ptCommand->eConsolePort, "Turning MODEM on\r\n" );
//    }
//    // "MODEM OPEN" command ------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "OPEN" ) == TRUE )
//    {
//        MODEM_commandSocketOpen();
//        ConsolePrintf( ptCommand->eConsolePort, "Opening modem sockets\r\n" );
//    }
//    // "MODEM CLOSE" command -----------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CLOSE" ) == TRUE )
//    {
//        MODEM_commandSocketClose();
//        ConsolePrintf( ptCommand->eConsolePort, "Closing modem sockets\r\n" );
//    }        
//    // MODEM CONSOLE command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONSOLE" ) == TRUE )
//    {
//        TIMER tTimerNoActivity = TimerDownTimerStartMs( 20000 );
//        TIMER tTimerEsc;
//        BOOL fEscapeReceived = FALSE;
//        ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO TERMINATE MODEM DIRECT COMM!!!\r\n\r\n" );
//        
//	UINT16		wBytesReturned = 0;
//        UINT8 c;
//        // Wait for the response to be returned
//
//        while( 1 )
//        {
//            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );
//
//            UINT8 c;
//            if ( UsartGetNB( TARGET_USART_PORT_TO_MODEM, &c ) == TRUE )
//            {
//                ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
//            }
//
//            if ( ConsoleGetChar(ptCommand->eConsolePort, &c) == TRUE )
//            {
//                tTimerNoActivity = TimerDownTimerStartMs( 20000 );
//                tTimerEsc = TimerDownTimerStartMs( 1000 );
//                fEscapeReceived = FALSE;
//                if ( c == COMMAND_ESC_KEY )
//                {
//                    fEscapeReceived = TRUE;
//                }                
//                else if ((c == '\r') || ((c >= ' ') && (c <= '~')))
//                {
//                    UsartPutNB( TARGET_USART_PORT_TO_MODEM, c );
//
//                    // Echo the character out of the serial port
//                    ConsolePutChar( ptCommand->eConsolePort, c, TRUE );
//                    if (c == '\r')
//                    {
//                        ConsolePutChar( ptCommand->eConsolePort, '\n', TRUE );
//                    }
//                }
//            }
//            if  (TimerDownTimerIsExpired( tTimerEsc ) == TRUE )
//            {
//              if (fEscapeReceived)
//              {
//                break;
//              }
//            }
//            if  (TimerDownTimerIsExpired( tTimerNoActivity ) == TRUE )
//            {
//              break;
//            }
//        }
//        ConsolePrintf( ptCommand->eConsolePort, "\r\nDirect Modem connection terminated.\r\n" );
//    }     
//    // "MODEM RSSI" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "RSSI" ) == TRUE )
//    {
//        MODEM_commandRssiReadRequest();
//    }
//    // "MODEM TEST1" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TEST1" ) == TRUE )
//    {
//        MODEM_commandTest1();
//    }
//    // "MODEM TEST2" command --------------------------------------------------------------
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TEST2" ) == TRUE )
//    {
//        MODEM_commandTest2();
//    }
////     // "MODEM SEND" command ---------------------------------------------------------------
////     // Send position report and monitor report.
////     else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SEND" ) == TRUE )
////     {
////         MODEM_gpsReportRequest();
////         MODEM_monitorReportRequest();
////     }
//    else
//    {
//        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
//    }
//
//    return eResult;
//}
//
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerFtp( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes Ftp commands.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerFtp( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "FTP Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "FTP INFO - Print Queue status and server configuration\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "FTP SERVER SET <ADDRS> <PORT#> <LOGNAME> <PASSWORD>\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "FTP CLEARQS - Remove all the items from all Queues.\r\n" );
                
        ConsolePrintf( ptCommand->eConsolePort, "FTP PUT ADD2Q <FILE_TYPE> <SUBSECTOR> - Add file to Ftp put Queue.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "FTP PUT CLEARQ    - Clear all files in queue.\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "FTP PUT QMAXSIZE  - Show the lenght of the queue.\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "FTP PUT QNITEMS   - Show the number of items in queue.\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "FTP MSG ADD2Q <MessageType> <BOOL>- Add msg to ftp msg Queue.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "             MessageType:  \r\n" );
        for( UINT8 c = 0 ; c < CONTROL_FILE_TRANSFER_MSG_MAX ; c++ )
        {
            ConsolePrintf( ptCommand->eConsolePort, "            %s\r\n", ControlFileTransfMessageGetString( c ) );            
        }        
        ConsolePrintf( ptCommand->eConsolePort, "FTP MSG CLEARQ    - Clear all files in queue.\r\n" );             
        ConsolePrintf( ptCommand->eConsolePort, "FTP MSG QMAXSIZE  - Show the lenght of the queue.\r\n" );         
        ConsolePrintf( ptCommand->eConsolePort, "FTP MSG QNITEMS   - Show the number of items in queue.\r\n" );  
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INFO" ) == TRUE )
    {
        UINT16 wMaxSize;
        UINT16 wNumberOfElements;

        // ftp server
        ConsolePrintf( ptCommand->eConsolePort, "Ftp Server\r\n" );
        ModemFtpConnType    tFtpConn;
        UINT16              wPort = 0;
        
        ConfigGetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_ADDRESS_STRING,       &tFtpConn.szAddress[ 0 ], (sizeof(tFtpConn.szAddress)-1) );
        ConfigGetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_PORT_UINT16,          &wPort, sizeof(wPort) );
        ConfigGetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_NAME_STRING,          &tFtpConn.szUserName[ 0 ], (sizeof(tFtpConn.szUserName)-1) );
        ConfigGetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_PASSWORD_STRING,      &tFtpConn.szPassword[ 0 ], (sizeof(tFtpConn.szPassword)-1) );
        ConsolePrintf( ptCommand->eConsolePort, "Address = \"%s\"\r\n", tFtpConn.szAddress );
        ConsolePrintf( ptCommand->eConsolePort, "Port    = \"%d\"\r\n", wPort );
        ConsolePrintf( ptCommand->eConsolePort, "UsrName = \"%s\"\r\n", tFtpConn.szUserName );
        ConsolePrintf( ptCommand->eConsolePort, "Psswrd  = \"%s\"\r\n", tFtpConn.szPassword );

        // queue info
        ConsolePrintf( ptCommand->eConsolePort, "Queue Info\r\n" );                        
        ConsolePrintf( ptCommand->eConsolePort, "Queue Name      Queue Max      Queue items \r\n" );                        
        
        FTPQ_ftpFileQueueSizeForPut ( &wMaxSize );
        FTPQ_ftpFilePutQueueIsEmpty( &wNumberOfElements );
        ConsolePrintf( ptCommand->eConsolePort, "Queue Put       %2d            %2d\r\n", wMaxSize, wNumberOfElements );
        
        FTPQ_ftpQueueSizeForSrvrMsg ( &wMaxSize );
        FTPQ_ftpSrvrMsgQueueIsEmpty( &wNumberOfElements );
        ConsolePrintf( ptCommand->eConsolePort, "Queue Msg       %2d            %2d\r\n", wMaxSize, wNumberOfElements );
    }
    // FTP SERVER SET <ADDRS> <PORT#> <LOGNAME> <PASSWORD>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SERVER" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 7 )
        {              
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "SET" ) == TRUE )
            {
                UINT16              wPort = 0;
                wPort = atoi( ptCommand->pcArgArray[ 4 ] );

                ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_ADDRESS_STRING,       ptCommand->pcArgArray[ 3 ] );
                ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_PORT_UINT16,          &wPort );
                ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_NAME_STRING,          ptCommand->pcArgArray[ 5 ] );
                ConfigSetValueByConfigType( CONFIG_TYPE_COMMUNICATIONS, CONFIG_DI_FTP_PASSWORD_STRING,      ptCommand->pcArgArray[ 6 ] );
                ConfigParametersSaveByConfigType( CONFIG_TYPE_COMMUNICATIONS );
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CLEARQS" ) == TRUE )
    {
        FTPQ_ftpFileQueueForPutClear();        
        FTPQ_ftpFileQueueForSrvrMsgClear();        
    }        
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //                              FTP PUT FILE
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    // FTP PUT command --------------------------------------------------------------
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PUT" ) == TRUE )
    {
        if( ptCommand->bNumArgs > 2 )
        {              
            // FTP PUT ADD2Q <FILE_TYPE> <SUBSECTOR> command --------------------------------------------------------------
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ADD2Q" ) == TRUE )
            {
                // WARNING: arg X not same as index in the pcArgArray[]. index= ( arg X - 1 )
                // arg 4         arg 5
                // <FILE_TYPE> <SUBSECTOR> 
                if ( ptCommand->bNumArgs >= 4 )
                {
                    // <FILE_TYPE> <SUBSECTOR>
                    UINT8 bFileType= 0;
                    UINT16 wFileSS  = 0;            

                    // get command args
                    // get file name                    
                    bFileType   = atoi( ptCommand->pcArgArray[ 3 ] );
                    // get directory storage type            
                    wFileSS     = atoi( ptCommand->pcArgArray[ 4 ] );                                
                    
                    if( FTPQ_ftpFileQueueForPut( bFileType, wFileSS ) == FALSE )
                    {              
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }                         
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CLEARQ" ) == TRUE )
            {
                FTPQ_ftpFileQueueForPutClear();
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "QMAXSIZE" ) == TRUE )
            {
                UINT16 wQueueSize = 0;

                if( FTPQ_ftpFileQueueSizeForPut( &wQueueSize ) )
                {
                    ConsolePrintf( CONSOLE_PORT_USART, "Queue Size=%d\r\n", wQueueSize );                    
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;                    
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "QNITEMS" ) == TRUE )
            {
                UINT16 wFileCount = 0;

                FTPQ_ftpFilePutQueueIsEmpty( &wFileCount );

                ConsolePrintf( CONSOLE_PORT_USART, "Items in Queue=%d\r\n", wFileCount );                                    
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "INFORM" ) == TRUE )
            {
                ConsolePrintf( CONSOLE_PORT_USART, "Not implemented yet.\r\n" );                    
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }   
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //                              FTP MSG
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////    
    // FTP MSG RUNQUEUE command --------------------------------------------------------------
    // FTP MSG ADD2Q <MessageType> <BOOL>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "MSG" ) == TRUE )
    {
        if( ptCommand->bNumArgs > 2 )
        {        
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "ADD2Q" ) == TRUE )
            {                
                // WARNING: arg X not same as index in the pcArgArray[]. index= ( arg X - 1 )
                // arg 4       
                // <MessageText>
                if ( ptCommand->bNumArgs == 5 )
                {
                    ControlFileTransferMsgType tFileTransferMsg;
                    BOOL   fIsMessageValid = FALSE;
                    BOOL   fIsPass = FALSE;
                    
                    fIsPass   = atoi( ptCommand->pcArgArray[ 4 ] );                    

                    // time stamp message
                    RtcDateTimeGet( &tFileTransferMsg.tDateTimeStamp );
                    
                    // check if message type is valid
                    for( UINT8 c = 0 ; c < CONTROL_FILE_TRANSFER_MSG_MAX ; c++ )
                    {
                        if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], ControlFileTransfMessageGetString(c) ) == TRUE )
                        {
                            tFileTransferMsg.eMsgActionType = c;
                            tFileTransferMsg.fIsActionPass  = fIsPass;
                            fIsMessageValid = TRUE;
                            break;
                        }
                    }

                    if( fIsMessageValid )
                    {                        
                        if( FTPQ_ftpQueueForSrvrMsg( &tFileTransferMsg ) == FALSE )
                        {              
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }  
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }                 
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CLEARQ" ) == TRUE )
            {
                FTPQ_ftpFileQueueForSrvrMsgClear();
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "QMAXSIZE" ) == TRUE )
            {
                UINT16 wQueueSize = 0;

                if( FTPQ_ftpQueueSizeForSrvrMsg( &wQueueSize ) )
                {
                    ConsolePrintf( CONSOLE_PORT_USART, "Queue Size=%d\r\n", wQueueSize );                    
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;                    
                }            
            }            
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "QNITEMS" ) == TRUE )
            {                                    
                UINT16 wFileCount = 0;

                FTPQ_ftpSrvrMsgQueueIsEmpty( &wFileCount );

                ConsolePrintf( CONSOLE_PORT_USART, "Items in Queue=%d\r\n", wFileCount );                                                
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }   
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerRtc( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerRtc( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "RTC Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "RTC      -  Show the RTC date & time\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "RTC STAT -  Show the RTC status\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "RTC SET <YYYY> <MM> <DD> <hh> <mm> <ss> [WEEK_DAY]  - Set the date and time on the RTC\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tWEEK_DAY [Monday=1..Sunday=7]\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "RTC INT PA0 - Monitors continuously RTC interruptions on PA0\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "RTC INT TIME <MATCH_TYPE> <TIME_NUMBER> - Monitors continuously Time Match interruption\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tMATCH_TYPE options:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t1 = Second Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t2 = Minute Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t3 = Hour Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tExample: SLEEP TIME STOP 1 45\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tSystem will wakeup when the seconds of the RTC clock match the number 45\r\n" );

        if( ptCommand->bNumArgs == 1 )    // RTC command only shows the date & time too
        {
            RtcPrintClockStatus( ptCommand->eConsolePort );
        }
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        RtcPrintClockStatus( ptCommand->eConsolePort );
        RtcPrintBckupStatus( ptCommand->eConsolePort );
        RtcPrintInterruptStatus( ptCommand->eConsolePort );
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SET" ) == TRUE )
    {
        if(
            ( ptCommand->pcArgArray[ 2 ] != NULL ) &&
            ( ptCommand->pcArgArray[ 3 ] != NULL ) &&
            ( ptCommand->pcArgArray[ 4 ] != NULL ) &&
            ( ptCommand->pcArgArray[ 5 ] != NULL ) &&
            ( ptCommand->pcArgArray[ 6 ] != NULL ) &&
            ( ptCommand->pcArgArray[ 7 ] != NULL )
        )//// <YYYY> <MM> <DD> <hh> <mm> <ss> ////
        {
            RtcDateTimeStruct   tDateTime;
            BOOL                fSuccess = FALSE;

            RtcDateTimeStructInit( &tDateTime );

            tDateTime.bYear        = ( atoi( ptCommand->pcArgArray[ 2 ] ) - RTC_CENTURY );
            tDateTime.eMonth       = atoi( ptCommand->pcArgArray[ 3 ] );
            tDateTime.bDate        = atoi( ptCommand->pcArgArray[ 4 ] );
            tDateTime.bHour        = atoi( ptCommand->pcArgArray[ 5 ] );
            tDateTime.bMinute      = atoi( ptCommand->pcArgArray[ 6 ] );
            tDateTime.bSecond      = atoi( ptCommand->pcArgArray[ 7 ] );

            if( ptCommand->pcArgArray[8] != NULL )
            {
                tDateTime.eDayOfWeek = atoi( ptCommand->pcArgArray[8] );
            }

            fSuccess = RtcDateTimeSet( &tDateTime );

            if( fSuccess == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INT" ) == TRUE )
    {
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PA0" ) == TRUE )
        {
            CHAR    cIsEscKey                       = 0;
            TIMER   tTimer                          = 0;
            UINT32  dwInterruptCounterCurrentVal    = 0;
            UINT32  dwInterruptCounterPreviousVal   = 0;

            tTimer = TimerDownTimerStartMs( 1000 );
            ConsolePrintf( ptCommand->eConsolePort, "If an Interruption ocurres, a message will be printed\r\n" );
            ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
            while( TimerDownTimerIsExpired( tTimer ) == FALSE );

            RtcInterruptPinA0Enable( TRUE );

            while(1)
            {
                WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                RtcGetInterruptCounter( &dwInterruptCounterCurrentVal, NULL );

                if( dwInterruptCounterCurrentVal != dwInterruptCounterPreviousVal )
                {
                    ConsolePrintf( ptCommand->eConsolePort, "PA0 Interrupt Ocurred!! Counter val = %d\r\n", dwInterruptCounterCurrentVal );

                    dwInterruptCounterPreviousVal = dwInterruptCounterCurrentVal;
                }

                // break loop if Escape Key received
                if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                {
                    if( cIsEscKey == COMMAND_ESC_KEY )
                    {
                        break;
                    }
                }

            }// end while loop

            RtcInterruptPinA0Enable( FALSE );
        }
        else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "TIME" ) == TRUE )
        {
            if( ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
            {//// <MATCH_TYPE> <TIME_NUMBER> ////
                RtcInterruptOnTimeMatchEnum eMatchType  = 0;
                UINT8                       bTimeNum    = 0;
                BOOL                        fSuccess    = FALSE;
                CHAR    cIsEscKey                       = 0;
                TIMER   tTimer                          = 0;
                UINT32  dwInterruptCounterCurrentVal    = 0;
                UINT32  dwInterruptCounterPreviousVal   = 0;

                eMatchType  = atoi( ptCommand->pcArgArray[ 3 ] );
                bTimeNum    = atoi( ptCommand->pcArgArray[ 4 ] );

                if( eMatchType < RTC_ON_TIME_MATCH_MAX )
                {
                    fSuccess = RtcInterruptOnTimeMatch( eMatchType, bTimeNum );

                    if( fSuccess )
                    {
                        tTimer = TimerDownTimerStartMs( 1000 );
                        ConsolePrintf( ptCommand->eConsolePort, "If an Interruption ocurres, a message will be printed\r\n" );
                        ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
                        while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                        RtcInterruptOnTimeMatch( eMatchType, bTimeNum );

                        while(1)
                        {
                            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                            RtcGetInterruptCounter( NULL, &dwInterruptCounterCurrentVal );

                            if( dwInterruptCounterCurrentVal != dwInterruptCounterPreviousVal )
                            {
                                RtcPrintClockStatus( ptCommand->eConsolePort );
                                ConsolePrintf( ptCommand->eConsolePort, "TIME Match Interrupt Ocurred!! Counter val = %d\r\n", dwInterruptCounterCurrentVal );

                                dwInterruptCounterPreviousVal = dwInterruptCounterCurrentVal;
                            }

                            // break loop if Escape Key received
                            if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                            {
                                if( cIsEscKey == COMMAND_ESC_KEY )
                                {
                                    break;
                                }
                            }

                        }// end while loop

                        RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerSleep( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerSleep( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "SLEEP Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SLEEP NOW - Control loop enters sleep immediately without waiting for console timeout\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SLEEP PA0 <STOP|STANDBY> - Send system to Sleep and waits for PA0 interruption\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SLEEP TIME <STOP|STANDBY> <MATCH_TYPE> <TIME_NUMBER> - Send system to Sleep and waits for Time Match interruption\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tMATCH_TYPE options:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t1 = Second Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t2 = Minute Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\t3 = Hour Match type\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tExample: SLEEP TIME STOP 1 45\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tSystem will wakeup when the seconds of the RTC clock match the number 45\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "NOW" ) == TRUE )
    {
        ControlSystemEnable( CONTROL_ACTION_SYSTEM_SLEEP, TRUE );
        ControlSystemRequest( CONTROL_ACTION_SYSTEM_SLEEP );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PA0" ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {//// <STOP|STANDBY> ////
            TIMER tTimer = 0;

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "STOP" ) == TRUE )
            {
                tTimer = TimerDownTimerStartMs( 1000 );
                ConsolePrintf( ptCommand->eConsolePort, "System in STOP MODE...\r\n" );
                while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                RtcInterruptPinA0Enable( TRUE );
                //////////////////////////////////////
                //////////////////////////////////////
                SystemSleep(SYSTEM_SLEEP_MODE_STOP);
                //////////////////////////////////////
                //////////////////////////////////////
                RtcInterruptPinA0Enable( FALSE );

                ConsolePrintf( ptCommand->eConsolePort, "System WAKE UP!!!\r\n" );
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "STANDBY" ) == TRUE )
            {
                tTimer = TimerDownTimerStartMs( 1000 );
                ConsolePrintf( ptCommand->eConsolePort, "System in STANDBY MODE...\r\n" );
                while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                RtcInterruptPinA0Enable( TRUE );
                //////////////////////////////////////
                //////////////////////////////////////
                SystemSleep(SYSTEM_SLEEP_MODE_STAND_BY);
                //////////////////////////////////////
                // NO RETURN HERE. SYSTEM is going to restart
                //////////////////////////////////////
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TIME" ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {//// <STOP|STANDBY> ////

            TIMER tTimer = 0;

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "STOP" ) == TRUE )
            {//// <MATCH_TYPE> <TIME_NUMBER> /////
                if( ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
                {
                    RtcInterruptOnTimeMatchEnum eMatchType  = 0;
                    UINT8                       bTimeNum    = 0;
                    BOOL                        fSuccess    = FALSE;

                    eMatchType  = atoi( ptCommand->pcArgArray[ 3 ] );
                    bTimeNum    = atoi( ptCommand->pcArgArray[ 4 ] );

                    if( eMatchType < RTC_ON_TIME_MATCH_MAX )
                    {
                        fSuccess = RtcInterruptOnTimeMatch( eMatchType, bTimeNum );

                        if( fSuccess )
                        {
                            tTimer = TimerDownTimerStartMs( 1000 );
                            ConsolePrintf( ptCommand->eConsolePort, "System in STOP MODE...\r\n" );
                            while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                            //////////////////////////////////////
                            //////////////////////////////////////
                            SystemSleep(SYSTEM_SLEEP_MODE_STOP);
                            //////////////////////////////////////
                            //////////////////////////////////////
                            RtcInterruptOnTimeMatch( RTC_INT_WKUP_ON_TIME_MATCH_NONE, 0 );

                            RtcPrintClockStatus( ptCommand->eConsolePort );
                            ConsolePrintf( ptCommand->eConsolePort, "----------------------\r\n" );
                            ConsolePrintf( ptCommand->eConsolePort, "System WAKE UP!!!\r\n" );
                        }
                        else
                        {
                            eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "STANDBY" ) == TRUE )
            {//// <MATCH_TYPE> <TIME_AMOUNT> /////
                if( ( ptCommand->pcArgArray[ 3 ] != NULL ) && ( ptCommand->pcArgArray[ 4 ] != NULL ) )
                {
                    RtcInterruptOnTimeMatchEnum eMatchType  = 0;
                    UINT8                       bTimeNum    = 0;
                    BOOL                        fSuccess    = FALSE;

                    eMatchType  = atoi( ptCommand->pcArgArray[ 3 ] );
                    bTimeNum    = atoi( ptCommand->pcArgArray[ 4 ] );

                    if( eMatchType < RTC_ON_TIME_MATCH_MAX )
                    {
                        fSuccess = RtcInterruptOnTimeMatch( eMatchType, bTimeNum );

                        if( fSuccess )
                        {
                            tTimer = TimerDownTimerStartMs( 1000 );
                            ConsolePrintf( ptCommand->eConsolePort, "System in STANDBY MODE...\r\n" );
                            while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                            //////////////////////////////////////
                            //////////////////////////////////////
                            SystemSleep(SYSTEM_SLEEP_MODE_STAND_BY);
                            //////////////////////////////////////
                            // NO RETURN HERE. SYSTEM is going to restart
                            //////////////////////////////////////

                        }
                        else
                        {
                            eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerSpi( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerSpi( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "\r\nSPI Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SPI STAT - Check the current status of the SPI.  \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SPI PWR <BUS#> < 1 | 0 > - Powers ON/OFF a particular Spi bus\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SPI PWR < 1 | 0 > - Powers ON/OFF All Spi peripherals <1=ON|0=OFF>\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SPI SEND <SLAVE#> [BYTE] [PERIOD] - Write continuously a byte to the Spi bus used by the SLAVE every PERIOD milliseconds\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = 10 ms{Min = 10 ms}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault byte = 0xA5\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        SpiPrintStatus( ptCommand->eConsolePort );
    }

    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PWR" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            if( ptCommand->pcArgArray[ 2 ] != NULL )
            {//// < 1 | 0 > ////
                BOOL                fPwrOn      = FALSE;
                BOOL                fSuccess    = FALSE;

                fPwrOn = atoi( ptCommand->pcArgArray[ 2 ] );

                fSuccess = SpiBusPowerOnAll( fPwrOn );

                if( fSuccess == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        if( ptCommand->bNumArgs == 4 )
        {
            if( ptCommand->pcArgArray[ 2 ] != NULL )
            {
                UINT8               bBusNumber  = 0;
                BOOL                fPwrOn      = FALSE;
                BOOL                fSuccess    = FALSE;

                bBusNumber  = atoi( ptCommand->pcArgArray[ 2 ] );
                fPwrOn      = atoi( ptCommand->pcArgArray[ 3 ] );

                fSuccess = SpiBusPowerOn( bBusNumber, fPwrOn );

                if( fSuccess == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SEND" ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {
            //// Get <SLAVE#> ////
            SpiSlaveEnum        eSpiSlave   = SPI_SLAVE_UNKNOWN;
            UINT8               bByte       = 0x5A;
            UINT32              dwPeriod_ms = 10;
            CHAR                cIsEscKey   = 0;
            BOOL                fSuccess    = FALSE;
            TIMER               tTimer      = 0;
            UINT32              u;

            eSpiSlave = atoi( ptCommand->pcArgArray[ 2 ] );

            if( eSpiSlave < SPI_SLAVE_TOTAL )
            {
                if( ptCommand->pcArgArray[ 3 ] != NULL )
                {
                    //// Get [BYTE] in hex (prefixed by "0x") or decimal ////
                    if( ( ptCommand->pcArgArray[ 3 ][0] == '0' ) &&
                        ( ( ptCommand->pcArgArray[ 3 ][1] == 'X' ) || ( ptCommand->pcArgArray[ 3 ][1] == 'x' ) ) )
                    {
                        fSuccess = GeneralHexStringToUINT32( ptCommand->pcArgArray[ 3 ], &u );

                        if( ( fSuccess == TRUE ) && ( u <= 255 ) )
                        {
                            bByte = (UINT8)u;
                        }
                        else
                        {
                            eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                            return eResult;
                        }
                    }
                    else
                    {
                        bByte = (UINT8)atoi( ptCommand->pcArgArray[ 3 ] );
                    }
                }
                if( ptCommand->pcArgArray[ 4 ] != NULL )
                {
                    //// Get [PERIOD] ////
                    dwPeriod_ms = atoi( ptCommand->pcArgArray[ 4 ] );
                    // validate period
                    dwPeriod_ms = dwPeriod_ms > 10 ? dwPeriod_ms : 10;
                }

                ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );

                tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                // LOCK SPI BUS for an specific SLAVE
                fSuccess = SpiSlaveBusOpen( eSpiSlave, 0 );

                if( fSuccess == TRUE )
                {
                    while(1)
                    {
                        WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );  

                        if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                        {
                            tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                            // Assert chip select
                            if( SpiSlaveChipSelectAssert( eSpiSlave, TRUE ) == FALSE )
                            {
                                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                                break;
                            }
                            // send a byte
                            if( SpiSlaveSendRecvByte( eSpiSlave, bByte , NULL ) == FALSE )
                            {
                                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                                break;
                            }
                            // Deassert chip select
                            if( SpiSlaveChipSelectAssert( eSpiSlave, FALSE ) == FALSE )
                            {
                                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                                break;
                            }
                        }

                        // break loop if Escape Key received
                        if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                        {
                            if( cIsEscKey == COMMAND_ESC_KEY )
                            {
                                break;
                            }
                        }
                    }// end while loop

                    // UNLOCK SPI BUS ( Only the SLAVE that was using it can UNLOCK IT )
                    SpiSlaveBusClose( eSpiSlave );
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerUsart( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerUsart( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "USART Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "USART STAT - Shows the status of all Usart Ports\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "USART SEND <PORT_NUMBER> [CHARACTER] [PERIOD] - Send a char to the Usart Port specified every PERIOD milliseconds \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault PERIOD = {Min Value = 11ms}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tdefault CHARACTER = 0xA5 {Ascii Min=0x33,Ascii Max=0x7E}\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "USART BAUD <BAUD_RATE> - Sets The baud rate for the current usart port\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "           <PORT_NUMBER> <BAUD_RATE> - Sets The baud rate for a specific usart port\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tWARNING! this command only return if error.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tTo verify the operation succeed, Adjust baud rate of your\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "\t\tconsole after executing the comand and test for promt return.\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {
        UsartAllPortsPrintStatus( ptCommand->eConsolePort );
    }
    // USART BAUD <PORT_NUMBER> <BAUD_RATE>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "BAUD" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {//// <PORT_NUMBER> ////
            BOOL            fSuccess    = FALSE;
            TIMER           tTimer      = 0;            
            UINT32          dwPeriod_ms = 11;
            UINT32          dwBaud      = atoi( ptCommand->pcArgArray[ 2 ] );

            // validate ARG
            if( ptCommand->eConsolePort < USART_PORT_TOTAL )
            {
                if( UsartPortChangeBaud( ptCommand->eConsolePort, dwBaud ) == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else if( ptCommand->bNumArgs == 4 )
        {//// <PORT_NUMBER> ////
            BOOL            fSuccess    = FALSE;
            TIMER           tTimer      = 0;            
            UINT32          dwPeriod_ms = 11;
            UsartPortsEnum  ePort       = atoi( ptCommand->pcArgArray[ 2 ] );
            UINT32          dwBaud      = atoi( ptCommand->pcArgArray[ 3 ] );

            // validate ARG
            if( ePort < USART_PORT_TOTAL )
            {
                if( UsartPortChangeBaud( ePort, dwBaud ) == FALSE )
                {
                    eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "SEND" ) == TRUE )
    {
        if( ptCommand->pcArgArray[ 2 ] != NULL )
        {//// <PORT_NUMBER> ////
            BOOL            fSuccess    = FALSE;
            TIMER           tTimer      = 0;
            CHAR            cChar       = 0xA5;
            CHAR            cIsEscKey   = 0;
            UINT32          dwPeriod_ms = 11;
            UsartPortsEnum  ePort       = atoi( ptCommand->pcArgArray[ 2 ] );

            // validate ARG
            if( ePort < USART_PORT_TOTAL )
            {

                if( ptCommand->pcArgArray[ 3 ] != NULL )
                {//// [CHARACTER] ////
                    cChar = *ptCommand->pcArgArray[ 3 ];
                }

                if( ptCommand->pcArgArray[ 4 ] != NULL )
                {//// [PERIOD] ////
                    dwPeriod_ms = atoi( ptCommand->pcArgArray[ 4 ] );
                    // validate period
                    dwPeriod_ms = dwPeriod_ms > 11 ? dwPeriod_ms : 11;
                }

                tTimer = TimerDownTimerStartMs( 1000 );
                ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO STOP OPERATION!!!\r\n\r\n" );
                while( TimerDownTimerIsExpired( tTimer ) == FALSE );

                tTimer = TimerDownTimerStartMs( dwPeriod_ms );

                while(1)
                {
                    WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

                    if( TimerDownTimerIsExpired( tTimer ) == TRUE )
                    {
                        tTimer = TimerDownTimerStartMs( dwPeriod_ms );
                        fSuccess = UsartPutChar( ePort , cChar, 10 );

                        if( fSuccess == FALSE )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;

                            break;
                        }
                    }

                    // break loop if Escape Key received
                    if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
                    {
                        if( cIsEscKey == COMMAND_ESC_KEY )
                        {
                            break;
                        }
                    }

                }// end while loop

            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerInterfaceBoard( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerInterfaceBoard( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;
    
	if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "INTB Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "INTB STAT                - Shows the status of the interface board pins\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "INTB ENABLE <BOOL>       - Enable/Disable Interface Board Power lines(2V5, 3V3).\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "INTB TEST1 <times to loop> <ON delay ms> <OFF delay ms> - Truns on off int board.\r\n" );        
    }
    // Console Command: PORTEX STAT
	else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  COMMAND_STRING__STAT))
    {
        BOOL fLogicLevel = FALSE;

        if( IntBoardIsEnabled() )               
        {                                           
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board Enabled\r\n" );                
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board Disabled\r\n" );    
        }
    }
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "ENABLE"))
	{		        
        BOOL fEnable = FALSE;		        
        
        if( ptCommand->bNumArgs == 3 )
        {
            fEnable = atoi( ptCommand->pcArgArray[2] );

            IntBoardEnable( fEnable );            
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    //ConsolePrintf( ptCommand->eConsolePort, "INTB TEST1 <times to loop> <ON delay ms> <OFF delay ms> - Truns on off int board.\r\n" );        
//    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "TEST1"))
//	{		        
//        BOOL fEnable = FALSE;
//        
//        if( ptCommand->bNumArgs == 5 )
//        {
//            UINT16 wNumOfLoops  = 0;
//            UINT32 dwTimeOn_ms  = 0;
//            UINT32 dwTimeOff_ms = 0;
//
//            TIMER tTimer;
//            CHAR cIsEscKey;
//
//            wNumOfLoops  = atoi(ptCommand->pcArgArray[2]);
//            dwTimeOn_ms  = atoi(ptCommand->pcArgArray[3]);
//            dwTimeOff_ms = atoi(ptCommand->pcArgArray[4]);
//           
//            // set default state
//            //IntBoardEnable( TRUE );
//            IntBoardEnable( FALSE );
//            TIMER tTimerModem;
//            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );
//
//            for( UINT32 dwLoopCounter = 0 ; dwLoopCounter < wNumOfLoops ; dwLoopCounter++ )
//            {                     
//                if
//                (
//                    ( wNumOfLoops == 255 ) &&
//                    ( dwLoopCounter == 254 )
//                )
//                {
//                    dwLoopCounter = 0;
//                }
//
//                MODEM_commandPowerOn();
//
//                tTimerModem = TimerDownTimerStartMs( 2000 );   // allow 1000 ms to reach this state
//    
//                while(1)
//                {   
//                    MODEM_stateMachine();
//
//                    if( MODEM_stateRead() >= MODEM_STATE_INIT_START )
//                    {
//                        MODEM_stateMachine();
//                        break;
//                    }
//                    else
//                    {
//                        // check if time out
//                        if( TimerDownTimerIsExpired( tTimerModem ) == TRUE )
//                        {                            
//                            break;
//                        }
//                    }
//                }
//
//                // int b on time
//                IntBoardEnable( TRUE ); 
//                  
//                IntBoardCamEnable( TRUE );
//                // enable 12v power line
//                IntBoardGpioSetPinMode( INT_B_12V_POwER_ENABLE, GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//                //IntBoardGpioSetPinMode( INT_B_3V3_PWR_ENABLE, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
//                // delay to wait for the power line to get stable
//
//                TimerTaskDelayMs( 100 );                 
//
//                ConsolePrintf( ptCommand->eConsolePort, "enabled\r\n" );    
//
//                tTimer = TimerDownTimerStartMs( dwTimeOn_ms );
//                while( TimerDownTimerIsExpired( tTimer ) == FALSE ){}	
//
//                WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );                
//
//                // break loop if Escape Key received
//                if( ConsoleGetChar( ptCommand->eConsolePort, &cIsEscKey ) )
//                {
//                    if( cIsEscKey == COMMAND_ESC_KEY )
//                    {
//                        break;
//                    }
//                }
//
//                // int b off time
//                IntBoardEnable( FALSE );
//                // disable 12v power line
//                MODEM_commandPowerOff();
//
//                tTimerModem = TimerDownTimerStartMs( 2000 );   // allow 1000 ms to reach this state
//    
//                while(1)
//                {   
//                    MODEM_stateMachine();
//
//                    if( MODEM_isModemOff() )
//                    {                        
//                        break;
//                    }
//                    else
//                    {
//                        // check if time out
//                        if( TimerDownTimerIsExpired( tTimerModem ) == TRUE )
//                        {                            
//                            break;
//                        }
//                    }
//                }
//
//                //IntBoardGpioSetPinMode( INT_B_12V_POwER_ENABLE, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
//                //IntBoardGpioSetPinMode( INT_B_3V3_PWR_ENABLE, GPIO_MODE_OUTPUT, HIGH, GPIO_OUTPUT_TYPE_PUSH_PULL );
//                // make sure 12 volt power line is out of power
//                TimerTaskDelayMs( 100 ); 
//
//                ConsolePrintf( ptCommand->eConsolePort, "disabled\r\n" );    
//
//                tTimer = TimerDownTimerStartMs( dwTimeOff_ms );
//                while( TimerDownTimerIsExpired( tTimer ) == FALSE ){}	
//
//                WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );
//
//                ConsolePrintf( ptCommand->eConsolePort, "Round %d\r\n", (dwLoopCounter+1) );    
//            }  
//              
//            MODEM_commandPowerOff();
//            IntBoardEnable( FALSE );
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }
	else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }
    
    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerCam( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerCam( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;
    
	if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "CAM Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CAM STAT                - Shows the status of the camera driver\r\n" ); 
        ConsolePrintf( ptCommand->eConsolePort, "CAM CONFIG <A|B> <ENABLE|DISABLE> <QUALITY#> <RESOL#> - Sets the configuration of a camera\r\n" ); 
        ConsolePrintf( ptCommand->eConsolePort, "CAM PWR <A|B> <ON|OFF>  - Powers on/off the camera.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CAM CONSOLE             - Connect the serial port of the Camera to the Console port.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CAM CAPTURE             - Triggers an Image Capture.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "CAM INFO                - Reads the Camera Startup Info.\r\n" );                        
        //ConsolePrintf( ptCommand->eConsolePort, "CAM ENABLE <A/B> <BOOL>       - Enable/Disable Mux/Demux.\r\n" );
        //ConsolePrintf( ptCommand->eConsolePort, "CAM SELECT <A/B>        - Select Camera A or B(Mux/Demux channel select).\r\n" );
        //ConsolePrintf( ptCommand->eConsolePort, "CAM LINE <RX|TX|RXTX> <BOOL> - Enable/Disable Rx and Tx transmission lines.\r\n" );
    }
    // Console Command: PORTEX STAT
	else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  COMMAND_STRING__STAT))
    {
        CAMERA_NUMBER_ENUM_TYPE eCamNumber;
        BOOL fLogicLevel = FALSE;

        // show current cam configurations
        CAMERA_CONFIG_STRUCT_TYPE tCamConfig;
        ConsolePrintf( ptCommand->eConsolePort, "Cam configurations\r\n" );
        for( CAMERA_NUMBER_ENUM_TYPE eCamNumber = CAMERA_NUMBER_1 ; eCamNumber < CAMERA_NUMBER_MAX ; eCamNumber++ )
        {
            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
            CAMERA_GetConfig( eCamNumber, &tCamConfig );            
            ConsolePrintf( ptCommand->eConsolePort, "Cam%d(%c)\r\n", (eCamNumber+1), (eCamNumber+65) );
            ConsolePrintf( ptCommand->eConsolePort, "Is enabled = %d\r\n", tCamConfig.fIsEnabled );
            ConsolePrintf( ptCommand->eConsolePort, "Resolution = %d\r\n", tCamConfig.bCamResolution );
            ConsolePrintf( ptCommand->eConsolePort, "Quality    = %d\r\n", tCamConfig.bCamQuality );            
        }

        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "Current Cam Status\r\n" );
        if( IntBoardIsEnabled() )               
        {                           
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board Enabled\r\n" );            
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board disabled\r\n" );
        }

        // indicate the channel selected
        eCamNumber = CAMERA_GetCurrentCamSelected();
        if( eCamNumber == CAMERA_NUMBER_1 )
        {
            ConsolePrintf( ptCommand->eConsolePort, "Cam1(A) Selected\r\n" );    
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Cam2(B) Selected\r\n" );
        }

//        fLogicLevel = FALSE;
//        IntBoardGpioPinRead( INT_B_CAMERA_SEL_A_B, &fLogicLevel );
//        if( fLogicLevel == FALSE )
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Cam1(A) Selected\r\n" );    
//            eCamNumber = CAMERA_NUMBER_1;
//        }
//        else
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Cam2(B) Selected\r\n" );    
//            eCamNumber = CAMERA_NUMBER_2;
//        }

        if( CAMERA_IsPowerEnabled( eCamNumber ) )
        {                           
            ConsolePrintf( ptCommand->eConsolePort, "Cam Powered On\r\n" );            
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Cam Powered Off\r\n" );
        }
        
//        fLogicLevel = FALSE;
//        IntBoardGpioPinRead( INT_B_CAMERA_ENABLE, &fLogicLevel );
//        if( fLogicLevel == TRUE )
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Cam Mux/Demux Disabled\r\n" );    
//        }
//        else
//        {
//            ConsolePrintf( ptCommand->eConsolePort, "Cam Mux/Demux Enabled\r\n" );                    
//        }
//
//
//        // indicate rx or tx enabled       
//        // WARNING!! RX logic inverted!!          
//        IntBoardGpioPinRead( INT_B_CAM_RS422_RECEIVER_OUT_ENABLE, &fLogicLevel );            
//        if( fLogicLevel == TRUE )
//        {
//            fLogicLevel = FALSE;
//        }
//        else
//        {
//            fLogicLevel = TRUE;
//        }
//        ConsolePrintf( ptCommand->eConsolePort, "Rx enable = %d\r\n", fLogicLevel );
//        IntBoardGpioPinRead( INT_B_CAM_RS422_DRIVER_OUT_ENABLE, &fLogicLevel );            
//        ConsolePrintf( ptCommand->eConsolePort, "Tx enable = %d\r\n", fLogicLevel );                
                        
        ConsolePrintf( ptCommand->eConsolePort, "\r\n" );
	}
    // CAM CONFIG <A|B> <ENABLE|DISABLE> <QUALITY#> <RESOL#>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONFIG" ) == TRUE )
    {
        BOOL                        fIsCamValid             = FALSE;
        BOOL                        fIsEnableDisableValid   = FALSE;
        CAMERA_NUMBER_ENUM_TYPE     eCamNumber;
        BOOL                        fEnable;
        UINT8                       bQuality;
        UINT8                       bResolution;

        if( ptCommand->bNumArgs == 6 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "A" ) == TRUE )
            {
                eCamNumber = CAMERA_NUMBER_1;
                fIsCamValid = TRUE;
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "B" ) == TRUE )
            {
                eCamNumber = CAMERA_NUMBER_2;
                fIsCamValid = TRUE;
            }

            if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "ENABLE" ) == TRUE )
            {
                fEnable = TRUE;
                fIsEnableDisableValid = TRUE;
            }
            else
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "DISABLE" ) == TRUE )
            {
                fEnable = FALSE;
                fIsEnableDisableValid = TRUE;
            }

            bQuality        = atoi( ptCommand->pcArgArray[ 4 ] );
            bResolution     = atoi( ptCommand->pcArgArray[ 5 ] );

            if( fIsEnableDisableValid && fIsCamValid )
            {
                // set new vals in config
                UINT8 bCamEnableBitMask = 0;
                ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM_ENABLE, &bCamEnableBitMask, sizeof(bCamEnableBitMask) );
                // set or clear the bit
                if( fEnable )
                {
                    // if set...OR  (original | xxx1)
                    bCamEnableBitMask |= (1<<eCamNumber);
                }
                else
                {
                    // if clear...AND  (original & xxx0)
                    bCamEnableBitMask &= ~(1<<eCamNumber);   
                }
                // clear bits that doesn't belong to any camera
                bCamEnableBitMask &= 0x03;
                ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM_ENABLE, &bCamEnableBitMask );

                if( eCamNumber == CAMERA_NUMBER_1 )
                {
                    ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM1_QUALITY, &bQuality );
                    ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM1_RESOLUTION, &bResolution );
                }
                else
                {
                    ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM2_QUALITY, &bQuality );
                    ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_CAM2_RESOLUTION, &bResolution );
                }

                ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }        
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    //CAM PWR <A|B> <ON|OFF>
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "PWR" ) == TRUE )
    {
        BOOL                        fIsCamValid     = FALSE;
        BOOL                        fIsOnOffValid   = FALSE;
        CAMERA_NUMBER_ENUM_TYPE     eCamNumber;
        BOOL                        fEnable;

        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "A" ) == TRUE )
        {
            eCamNumber = CAMERA_NUMBER_1;
            fIsCamValid = TRUE;
        }
        else
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "B" ) == TRUE )
        {
            eCamNumber = CAMERA_NUMBER_2;
            fIsCamValid = TRUE;
        }
        
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "ON" ) == TRUE )
        {
            fEnable = TRUE;
            fIsOnOffValid = TRUE;
        }
        else
        if( ConsoleStringEqual( ptCommand->pcArgArray[ 3 ], "OFF" ) == TRUE )
        {
            fEnable = FALSE;
            fIsOnOffValid = TRUE;
        }

        if( fIsCamValid && fIsOnOffValid )
        {
            CAMERA_SetCurrentCamSelected( eCamNumber );

            if( fEnable )
            {
                ConsolePrintf( ptCommand->eConsolePort, "Camera Powering On\r\n\r\n" );                
                CAMERA_commandPowerOn( ptCommand->eConsolePort );
            }
            else
            {
                if( CAMERA_isCameraOff( eCamNumber ) == FALSE ) 
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Camera Powering Off\r\n\r\n" );
                    CAMERA_commandPowerOff( ptCommand->eConsolePort );
                }
                else
                {
                    ConsolePrintf( ptCommand->eConsolePort, "Error: Current Camera Is not On\r\n\r\n" );
                }
            }
        }
    }
//    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "OFF" ) == TRUE )
//    {
//      ConsolePrintf( ptCommand->eConsolePort, "Camera Powering Off\r\n\r\n" );
//      CAMERA_commandPowerOff(ptCommand->eConsolePort);
//    }
//    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "ENABLE"))
//	{		        
//        BOOL fEnable = FALSE;		
//        TIMER tTimer;        
//        
//        if( ptCommand->bNumArgs == 3 )
//        {
//            fEnable = atoi( ptCommand->pcArgArray[2] );
//            
//            CAMERA_EnablePower( CAMERA_NUMBER_1, fEnable );
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }    
//    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "SELECT"))
//	{		                                       
//        if( ptCommand->bNumArgs == 3 )
//        {
//            CHAR cCamChannel = 0;		                
//
//            cCamChannel = *ptCommand->pcArgArray[2];
//            
//            switch( cCamChannel )
//            {
//                case 'A':
//                case 'a':
//                    IntBoardGpioOutputWrite ( INT_B_CAMERA_SEL_A_B, FALSE );
//                    break;
//                case 'B':
//                case 'b':
//                    IntBoardGpioOutputWrite ( INT_B_CAMERA_SEL_A_B, TRUE );
//                    break;
//                default:
//                    eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
//                    break;
//            }
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//    }
//	// CAM LINE <RX|TX|RXTX> <BOOL>
//	else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "LINE"))
//	{		
//        if( ptCommand->bNumArgs == 4 )
//        {
//            BOOL fEnable = atoi( ptCommand->pcArgArray[3] );
//
//            if( ConsoleStringEqual(ptCommand->pcArgArray[2],  "RX") )
//            {   
//                // WARNING!! RX logic inverted!!
//                if( fEnable == TRUE )
//                {
//                    fEnable = FALSE;
//                }
//                else
//                {
//                    fEnable = TRUE;
//                }
//                IntBoardGpioOutputWrite ( INT_B_CAM_RS422_RECEIVER_OUT_ENABLE, fEnable );
//            }
//            else
//            if( ConsoleStringEqual(ptCommand->pcArgArray[2],  "TX") )
//            {
//                IntBoardGpioOutputWrite ( INT_B_CAM_RS422_DRIVER_OUT_ENABLE, fEnable );
//            }
//            else
//            if( ConsoleStringEqual(ptCommand->pcArgArray[2],  "RXTX") )
//            {
//                // enable / disable both lines
//                IntBoardGpioOutputWrite ( INT_B_CAM_RS422_DRIVER_OUT_ENABLE, fEnable );
//                
//                // WARNING!! RX logic inverted!!
//                if( fEnable == TRUE )
//                {
//                    fEnable = FALSE;
//                }
//                else
//                {
//                    fEnable = TRUE;
//                }
//                IntBoardGpioOutputWrite ( INT_B_CAM_RS422_RECEIVER_OUT_ENABLE, fEnable );                
//            }
//            else
//            {
//                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
//            }
//        }
//        else
//        {
//            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
//        }
//	}	
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONSOLE" ) == TRUE )
    {
        TIMER tTimerNoActivity = TimerDownTimerStartMs( 20000 );
        TIMER tTimerEsc;
        BOOL fEscapeReceived = FALSE;
        ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO TERMINATE CAM DIRECT COMM!!!%s", CONSOLE_STRING_NEW_PROMPT );
        
	UINT16		wBytesReturned = 0;
        UINT8           bBuffer[1024];
        // Wait for the response to be returned

        while( 1 )
        {
            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

            UINT8 c;
            if ( UsartGetBuffer(TARGET_USART_PORT_CAMERA, &bBuffer[0], sizeof(bBuffer), &wBytesReturned, 0) == TRUE )
            {
                ConsolePutBuffer( ptCommand->eConsolePort, &bBuffer[0], wBytesReturned );
            }

            if ( ConsoleGetBuffer(ptCommand->eConsolePort, &bBuffer[0], sizeof(bBuffer), &wBytesReturned) == TRUE )
            {
                tTimerNoActivity = TimerDownTimerStartMs( 20000 );
                tTimerEsc = TimerDownTimerStartMs( 1000 );
                UsartPutBuffer( TARGET_USART_PORT_CAMERA, &bBuffer[0], wBytesReturned );
                fEscapeReceived = FALSE;
                if ( bBuffer[wBytesReturned-1] == COMMAND_ESC_KEY )
                {
                    fEscapeReceived = TRUE;
                }                
            }
            if  (TimerDownTimerIsExpired( tTimerEsc ) == TRUE )
            {
              if (fEscapeReceived)
              {
                // finish sending the line clear sequence on the camera <ESC>[2K
                UsartPutString( TARGET_USART_PORT_CAMERA, "[2K" );
                break;
              }
            }
            if  (TimerDownTimerIsExpired( tTimerNoActivity ) == TRUE )
            {
              break;
            }
        }
        ConsolePrintf( ptCommand->eConsolePort, "\r\nDirect Camera connection terminated.\r\n" );
    }     
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CAPTURE" ) == TRUE )
    {
      //ModCameraTriggerCapture();
      CAMERA_commandCaptureImage(ptCommand->eConsolePort);
      ConsolePrintf( ptCommand->eConsolePort, "Image Capture In Progress\r\n\r\n" );
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "INFO" ) == TRUE )
    {
      //ModCameraTriggerCapture();
      CAMERA_commandReadInfo(ptCommand->eConsolePort);
      ConsolePrintf( ptCommand->eConsolePort, "Camera Read Info In Progress\r\n\r\n" );
    }
	else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }
    
    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerPortExp( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerPortExp( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    // Console Command: PORTEX HELP
	if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX STAT                - Shows the status of all Usart Ports\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX DEFAULT             - set all pins in port ex to default output low.\r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX READ <REG#>         - Read a Register in portex\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX WRITE <REG#> <VAL>  - Write a value in a portex register.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "PORTEX SET <PIN#> <IN/OUT> <LEVEL# 0=LOW/1=HIGH> - Set portex pin as input or output, low or high \r\n" );        
        // Console Command: PORTEX SET <PIN#> <IN/OUT> <LEVEL# 0=LOW/1=HIGH>
    }
    // Console Command: PORTEX STAT
	else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  COMMAND_STRING__STAT))
    {
        BOOL fLogicLevel = FALSE;

        if( IntBoardIsEnabled() )               
        {               
            PortExShowStatus( ptCommand->eConsolePort );
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board disabled\r\n" );            
        }
	}    
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "DEFAULT"))
	{		                       
        PortExSetInitialPinState();        
    }
	// Console Command: PORTEX SET <PIN#> <IN/OUT> <LEVEL# 0=LOW/1=HIGH>
	else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "SET"))
	{		
        UINT8		 	bPinNumber;
        BOOL			fDirectionOut;		
		BOOL			fDriveHigh;

        bPinNumber 		= atoi(ptCommand->pcArgArray[2]);		
        fDriveHigh		= atoi(ptCommand->pcArgArray[4]);

		if( ConsoleStringEqual(ptCommand->pcArgArray[3],  "OUT") )
		{
			fDirectionOut = TRUE;
		}
        else if( ConsoleStringEqual(ptCommand->pcArgArray[3],  "IN") )
        {
            fDirectionOut = FALSE;
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
        }
    
		if( eResult == CONSOLE_RESULT_OK )
		{
			PortExPinEnum ePinNumber = (PortExPinEnum) bPinNumber;

			if ((ePinNumber >= PORTEX_PIN_P4) && (ePinNumber <= PORTEX_PIN_P31))
			{
				if (fDirectionOut)
				{
					PortExSetPinAsOutput(ePinNumber, fDriveHigh);
				}
				else
				{
					PortExSetPinAsInput(ePinNumber, fDriveHigh);
				}
			}
			else
			{
				eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
			}
		}		
		else
		{
			eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
		}
	}	
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "READ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 ) 
        {
            UINT8 bRegNum = 0 ;
            UINT8 bRegVal = 0 ;

            bRegNum = atoi( ptCommand->pcArgArray[ 2 ] );

            bRegVal = PortExRegisterRead( bRegNum );

            ConsolePrintf( ptCommand->eConsolePort, "Val returned %d\r\n", bRegVal );
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }     
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "WRITE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 ) 
        {
            UINT8 bRegNum = 0 ;
            UINT8 bRegVal = 0 ;

            bRegNum = atoi( ptCommand->pcArgArray[ 2 ] );

            bRegVal = atoi( ptCommand->pcArgArray[ 3 ] );

            PortExRegisterWrite( bRegNum, bRegVal );            

            ConsolePrintf( ptCommand->eConsolePort, "Reg written\r\n" );
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }     
	else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }
    
    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerAdcExt( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerAdcExt( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;

    if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX STAT                - Shows the status of all Adcex\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONT                - Print every second the status of the external adc\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX READ <REG#>         - Read a Register\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX WRITE <REG#> <VAL>  - Write a value in a register.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG 2POINT      <CHANNEL#> <PWR_TYPE#> <POINT# 0|1 > <EXPECTED mV>- Measures voltage and save it in config as a calibration point.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG 1POINT      <CHANNEL#> <PWR_TYPE#> <EXPECTED mV>- Measures voltage and save it in config as a calibration point.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG CALIBRATION <CHANNEL#> <PWR_TYPE#> - Calculate slope and offset of ADC channel according to config calibration points.\r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG CORRECTION  <CHANNEL#> <BOOL> - Enables/Disables slope and offset correction factors for ADC channel.\r\n" );              
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG DELTA       <CHANNEL#> <BOOL> - Enables/Disables delta measurement for ADC channel.\r\n" );              
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CONFIG MODE        <CHANNEL#> <PWR_TYPE#>        - set the Voltage type in config.\r\n" );                      
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CHAN READ          <CHANNEL#> <PWR_TYPE#> <CORRECTION_ENABLE_BOOL> - Read a channel. Enables and Disables Pwr switch selected and apply slope/offset when enabling corrections.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CHAN MINMAXDEL     <CHANNEL#> <PWR_TYPE#> <CORRECTION_ENABLE_BOOL> - Read a channel. Enables and Disables Pwr switch selected and apply slope/offset when enabling corrections.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX CHAN PWREN         <CHANNEL#> <PWR_TYPE#> <BOOL> - Enables an adc Pwr switch for channels 0-5. For debugging.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "             PWR_TYPES: \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "             0) RTD \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "             1) SCALING A \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "             2) SCALING B \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "             3) 4-20mA loop \r\n" );                
        ConsolePrintf( ptCommand->eConsolePort, "ADCEX TEST                - Test functions\r\n" );  
    }    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CHAN" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 3 )
        {
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "READ" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 6 )
                {
                    UINT8                                   bChannel;
                    ControlAdcReadingChannelConfigStruct    tChannelConfig;

                    bChannel                                    = atoi( ptCommand->pcArgArray[ 3 ] );
                    ControlAdcReadingGetChannelConfig( bChannel, &tChannelConfig );

                    // overwrite config
                    tChannelConfig.tSensorConfig.ePowerMode                   = atoi( ptCommand->pcArgArray[ 4 ] );
                    tChannelConfig.tSensorConfig.tSampleCorrection.fIsEnable  = atoi( ptCommand->pcArgArray[ 5 ] );
                    SINGLE                  sgReadingRaw;
                    SINGLE                  sgMillivolts;
                                        
                    if
                    (
                        ( bChannel < ADC_INPUT_EXTERNAL_MAX ) &&
                        ( tChannelConfig.tSensorConfig.ePowerMode < ADC_POWER_SWITCH_MAX )
                    )
                    {
                        if( ControlAdcReadingSamplingIsWaitingForCommand() )
                        {
                            ControlAdcReadingShortSampleGetSample( bChannel, tChannelConfig.tSensorConfig.ePowerMode, FALSE, tChannelConfig.tSensorConfig.fIsDeltaSamplingEnabled, &sgReadingRaw );
                            ConsolePrintf( ptCommand->eConsolePort, " raw = %f\r\n", sgReadingRaw );

                            sgMillivolts = ControlAdcReadingConvertRawToScaledUnit( bChannel, sgReadingRaw, &tChannelConfig );

                            ConsolePrintf( ptCommand->eConsolePort, " millivolt = %f\r\n", sgMillivolts );
                        }
                        else
                        {
                            ConsolePrintf( ptCommand->eConsolePort, "Adc sampling busy." );
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;            
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
                }
            }
            // ADCEX CHAN MINMAXDEL <CHANNEL#> <PWR_TYPE#> <CORRECTION_ENABLE_BOOL>
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "MINMAXDEL" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 6 )
                {                    
                    UINT8                                   bChannel;
                    ControlAdcReadingChannelConfigStruct    tChannelConfig;

                    bChannel                                    = atoi( ptCommand->pcArgArray[ 3 ] );
                    ControlAdcReadingGetChannelConfig( bChannel, &tChannelConfig );

                    // overwrite config
                    tChannelConfig.tSensorConfig.ePowerMode                   = atoi( ptCommand->pcArgArray[ 4 ] );
                    tChannelConfig.tSensorConfig.tSampleCorrection.fIsEnable  = atoi( ptCommand->pcArgArray[ 5 ] );
                    SINGLE                  sgReadingRaw;
                    SINGLE                  sgMillivolts;

                    if
                    (
                        ( bChannel < ADC_INPUT_EXTERNAL_MAX ) &&
                        ( tChannelConfig.tSensorConfig.ePowerMode < ADC_POWER_SWITCH_MAX )
                    )
                    {
                        if( ControlAdcReadingSamplingIsWaitingForCommand() )
                        {
                            ControlAdcReadingShortSampleGetSample( bChannel, tChannelConfig.tSensorConfig.ePowerMode, FALSE, TRUE, &sgReadingRaw );
                            ConsolePrintf( ptCommand->eConsolePort, " Delta raw = %f\r\n", sgReadingRaw );

                            sgMillivolts = ControlAdcReadingConvertRawToScaledUnit( bChannel, sgReadingRaw, &tChannelConfig );

                            ConsolePrintf( ptCommand->eConsolePort, " Delta millivolt = %f\r\n", sgMillivolts );
                        }
                        else
                        {
                            ConsolePrintf( ptCommand->eConsolePort, "Adc sampling busy." );
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;            
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
                }
            }            
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "PWREN" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 6 )
                {
                    UINT8 bChannel          = atoi( ptCommand->pcArgArray[ 3 ] );
                    UINT8 bPowerLineType    = atoi( ptCommand->pcArgArray[ 4 ] );
                    BOOL  fEnable           = atoi( ptCommand->pcArgArray[ 5 ] );
                               
                    if( AdcEnableChannPowerMode( bChannel, bPowerLineType, fEnable ) == FALSE )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }            
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;            
        }              
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__STAT ) == TRUE )
    {              
        AdcPrintStat( ptCommand->eConsolePort, FALSE );    //prints out External ADC readings and data.
    }       
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "CONT"))
	{ 
        AdcPrintStat( ptCommand->eConsolePort, TRUE );    //prints out calibrated External ADC readings continuously.  
    }    
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "READ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 ) 
        {
            UINT8 bRegNum = 0 ;
            UINT16 wRegVal = 0 ;
            BOOL  fBit = 0;

            bRegNum = atoi( ptCommand->pcArgArray[ 2 ] );
            
            wRegVal = AdcReadRegister( bRegNum );

            ConsolePrintf( ptCommand->eConsolePort, "Val returned dec=%d\r\n", wRegVal );
            ConsolePrintf( ptCommand->eConsolePort, "Val returned hex=0x%04X\r\n", wRegVal );            
            ConsolePrintf( ptCommand->eConsolePort, "MSB               LSB\r\n");            
            for(int i = 0 ; i < 16 ; i++ )
            {
                if( i % 4 == 0 )
                {
                    ConsolePrintf( ptCommand->eConsolePort, " "); 
                }

                fBit = ( (wRegVal & 0x8000) >> 15 );
                wRegVal = wRegVal << 1;
                ConsolePrintf( ptCommand->eConsolePort, "%d", fBit );                 
            }
            ConsolePrintf( ptCommand->eConsolePort, "\r\n" );            
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }     
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "WRITE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 ) 
        {
            UINT8 bRegNum = 0 ;
            UINT16 wRegVal = 0 ;

            bRegNum = atoi( ptCommand->pcArgArray[ 2 ] );

            wRegVal = atoi( ptCommand->pcArgArray[ 3 ] );
            
            AdcWriteRegister( bRegNum, wRegVal );

            ConsolePrintf( ptCommand->eConsolePort, "Reg written\r\n" );
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }        
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "CONFIG" ) == TRUE )
    {
        if( ptCommand->bNumArgs >= 4 ) 
        {                        
            if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "2POINT" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 7 ) 
                {
                    UINT8 bChannel                  = atoi(ptCommand->pcArgArray[3]);
                    UINT8 bPwrType                  = atoi(ptCommand->pcArgArray[4]);
                    UINT8 bPoint                    = atoi(ptCommand->pcArgArray[5]);
                    SINGLE sgExpectedValue_mVolts	= atof(ptCommand->pcArgArray[6]);
                    
                    // make sure adc reading short or long is not currently running.
                    if( ControlAdcReadingSamplingIsWaitingForCommand() )
                    {
                        if( FALSE == AdcConfigSetChannCalibrationPoint( bChannel, bPwrType, bPoint, ADC_NUM_AVERAGES_ANALOG_INPUTS, sgExpectedValue_mVolts) )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                    }
                    else
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Currently running adc reading. This operation is not allowed\r\n" );

                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }                        
            // ADCEX CONFIG 1POINT      <CHANNEL#> <PWR_TYPE#> <EXPECTED mV>
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "1POINT" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 6 ) 
                {
                    UINT8 bChannel                  = atoi(ptCommand->pcArgArray[3]);
                    UINT8 bPwrType                  = atoi(ptCommand->pcArgArray[4]);
                    SINGLE sgExpectedValue_mVolts	= atof(ptCommand->pcArgArray[5]);
                           
                    // make sure adc reading short or long is not currently running.
                    if( ControlAdcReadingSamplingIsWaitingForCommand() )
                    {
                        if( FALSE == AdcConfigSetChannCalibrationPoint( bChannel, bPwrType, ADC_CALIBRATION_POINT_1, ADC_NUM_AVERAGES_ANALOG_INPUTS, sgExpectedValue_mVolts) )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                        else
                        {
                            AdcConfigIndexesType    tAdcConfigIndexes;        
                            AdcConfigGetChannConfigIndexes( bChannel, bPwrType, &tAdcConfigIndexes );

                            SINGLE sgAdcCalibrationExpX1_mV;
                            SINGLE sgAdcCalibrationObtX1_mV;
                            SINGLE sgAdcCalibrationExpX2_mV;
                            SINGLE sgAdcCalibrationObtX2_mV;
                            // get expected pt1
                            // get read  pt1
                            ConfigGetValueByConfigType( CONFIG_TYPE_ADC_CALIBRATION, tAdcConfigIndexes.wIdxCalibObtX1, &sgAdcCalibrationObtX1_mV, sizeof(sgAdcCalibrationObtX1_mV) );
                            ConfigGetValueByConfigType( CONFIG_TYPE_ADC_CALIBRATION, tAdcConfigIndexes.wIdxCalibExpX1, &sgAdcCalibrationExpX1_mV, sizeof(sgAdcCalibrationExpX1_mV) );
                            // set expected to pt 1 and 2(pt 2 should add + 1)
                            // set read     to pt 1 and 2(pt 2 should add + 1)
                            sgAdcCalibrationExpX2_mV  = sgAdcCalibrationExpX1_mV + 1;
                            sgAdcCalibrationObtX2_mV  = sgAdcCalibrationObtX1_mV + 1;

                            ConfigSetValueByConfigType( CONFIG_TYPE_ADC_CALIBRATION, tAdcConfigIndexes.wIdxCalibExpX2, &sgAdcCalibrationExpX2_mV );
                            ConfigSetValueByConfigType( CONFIG_TYPE_ADC_CALIBRATION, tAdcConfigIndexes.wIdxCalibObtX2, &sgAdcCalibrationObtX2_mV );
                            ConfigParametersSaveByConfigType( CONFIG_TYPE_ADC_CALIBRATION );                                               
                        }
                    }
                    else
                    {
                        ConsolePrintf( ptCommand->eConsolePort, "Currently running adc reading. This operation is not allowed\r\n" );

                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CALIBRATION" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 5 ) 
                {
                    UINT8 bChannel 		= atoi(ptCommand->pcArgArray[3]);
                    UINT8 bPwrType 		= atoi(ptCommand->pcArgArray[4]);

                    if( FALSE == AdcConfigSetChannCalibrationOffsetSlope( bChannel, bPwrType ) )
                    {
                        eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            //ADCEX CONFIG CORRECTION  <CHANNEL#> <BOOL>
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "CORRECTION" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 5 ) 
                {
                    UINT8   bChannel 		= atoi(ptCommand->pcArgArray[3]);                    
                    BOOL    fEnable 		= atoi(ptCommand->pcArgArray[4]);
                    BOOL    fSuccess;                                         
                    UINT8   bCorrectionEnabledBitMask = 0;

                    if( bChannel < ADC_INPUT_EXTERNAL_MAX )
                    {
                        // get current bit mask and set the bit for the requested channel
                        ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_CORRECTION_ENABLED, &bCorrectionEnabledBitMask, sizeof(bCorrectionEnabledBitMask) );

                        if( fEnable == 0 )
                        {
                            UINT8 bMask = (1<<bChannel);    // 0001 0000
                            // invert the mask so that all other channels will keep the same value
                            bMask = ~bMask;
                            // clear the last 2 bits since config only accept a mask for 6 channels( max 6th bit set)
                            bMask = bMask & 0x3F;
                            // AND with 0 to clear bit
                            bCorrectionEnabledBitMask = bCorrectionEnabledBitMask & bMask; // 0010 1111
                        }
                        else
                        {                            
                            UINT8 bMask = (1<<bChannel);    // 0001 0000                            
                            // OR with 1 to enable bit
                            bCorrectionEnabledBitMask = bCorrectionEnabledBitMask | bMask;
                        }

                        fSuccess = TRUE;
                        fSuccess &= ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_CORRECTION_ENABLED, &bCorrectionEnabledBitMask );
                        fSuccess &= ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                        if( fSuccess == FALSE )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "DELTA" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 5 ) 
                {
                    UINT8   bChannel 		= atoi(ptCommand->pcArgArray[3]);                    
                    BOOL    fEnable 		= atoi(ptCommand->pcArgArray[4]);
                    BOOL    fSuccess;                                         
                    UINT8   bDeltaEnabledBitMask = 0;

                    if( bChannel < ADC_INPUT_EXTERNAL_MAX )
                    {
                        // get current bit mask and set the bit for the requested channel
                        ConfigGetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_MEASURE_DELTA_ENABLED, &bDeltaEnabledBitMask, sizeof(bDeltaEnabledBitMask) );

                        if( fEnable == 0 )
                        {
                            UINT8 bMask = (1<<bChannel);    // 0001 0000
                            // invert the mask so that all other channels will keep the same value
                            bMask = ~bMask;
                            // clear the last 2 bits since config only accept a mask for 6 channels( max 6th bit set)
                            bMask = bMask & 0x3F;
                            // AND with 0 to clear bit
                            bDeltaEnabledBitMask = bDeltaEnabledBitMask & bMask; // 0010 1111
                        }
                        else
                        {                            
                            UINT8 bMask = (1<<bChannel);    // 0001 0000                            
                            // OR with 1 to enable bit
                            bDeltaEnabledBitMask = bDeltaEnabledBitMask | bMask;
                        }

                        fSuccess = TRUE;
                        fSuccess &= ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, CONFIG_ADC_CHAN_MEASURE_DELTA_ENABLED, &bDeltaEnabledBitMask );
                        fSuccess &= ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                        if( fSuccess == FALSE )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else if( ConsoleStringEqual( ptCommand->pcArgArray[ 2 ], "MODE" ) == TRUE )
            {
                if( ptCommand->bNumArgs == 5 ) 
                {
                    UINT8 bChannel 		= atoi(ptCommand->pcArgArray[3]);
                    UINT8 bPwrType 		= atoi(ptCommand->pcArgArray[4]);                    

                    BOOL fSuccess;                    
                    AdcConfigIndexesType tAdcConfigIndexes;                                                            
                    if
                    (
                        ( bChannel < ADC_INPUT_EXTERNAL_MAX ) &&
                        ( bPwrType < ADC_POWER_SWITCH_MAX )
                    )
                    {
                        AdcConfigGetChannConfigIndexes( bChannel, 0, &tAdcConfigIndexes );        
                        fSuccess = TRUE;
                        fSuccess &= ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, tAdcConfigIndexes.wIdxMode, &bPwrType );
                        fSuccess &= ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                        if( fSuccess == FALSE )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }            
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], "TEST" ) == TRUE )
    {                   
    }
    else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerUserInterf( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerUserInterf( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;
    
	if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "UI Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "UI STAT                - Shows the status of the interface board pins\r\n" );                        
        ConsolePrintf( ptCommand->eConsolePort, "UI INTERRUPT <REED_SWITCH_#> <BOOL> - Enable/Disable Reedswitch Interrupt.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "     REED_SWITCH_#: \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "     1) Reed switch 1 \r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "     2) Reed switch 2 \r\n" );        
        ConsolePrintf( ptCommand->eConsolePort, "UI RSTEST              - Test for reed switches.\r\n" );        
    }
	else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  COMMAND_STRING__STAT))
    {
        BOOL fLogicLevel = FALSE;
        
        UserIfPrintInterruptStatus( ptCommand->eConsolePort );

        if( IntBoardIsEnabled() )               
        {                                           
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board Enabled\r\n" );                
        }
        else
        {
            ConsolePrintf( ptCommand->eConsolePort, "Interface Board Disabled\r\n" );    
        }
	}    
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "RSTEST"))
	{		        
        // Set 2 minute timeout in case this gets left on
        TIMER tTimer = TimerDownTimerStartMs( 120000 );
        ConsolePrintf( ptCommand->eConsolePort, "PRESS [ESC] KEY TO TERMINATE REED SWITCH TEST!!!\r\n\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "pin    stateMachine    Level       intCounter\r\n" );
                
        while( 1 )
        {
            WatchdogKickSoftwareWatchdog( CONTROL_WATCHDOG_TIMER_TIME_OUT_MS );

            UINT8 c;
                        
            UserIfReedSwitchFunctionTriggerStateMachine();

            if( UsartGetNB( TARGET_USART_PORT_TO_CONSOLE, &c ) == TRUE )
            {                
                // ESC = 27
                if( c  == 27 )
                {
                    break;
                }                
            }
            if  (TimerDownTimerIsExpired( tTimer ) == TRUE )
            {
              break;
            }
        }
    }    
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "INTERRUPT"))
	{		                        
        if( ptCommand->bNumArgs == 4 )
        {
            BOOL    fEnable             = FALSE;	
            UINT8   bReedSwitchNumber   = 0;
            BOOL    fSuccess            = FALSE;

            bReedSwitchNumber   = atoi( ptCommand->pcArgArray[2] );
            fEnable             = atoi( ptCommand->pcArgArray[3] );

            // validate data
            fSuccess = TRUE;
            fSuccess&= ( bReedSwitchNumber > 0 );
            fSuccess&= ( bReedSwitchNumber <= 2 );
            bReedSwitchNumber--;
            fSuccess&= ( fEnable > -1 );
            fSuccess&= ( fEnable < 2 );

            if( fSuccess )
            {                
                UsertIfInterruptPinReedSwtchEnable( bReedSwitchNumber, fEnable );                
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }             
	else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }
    
    return eResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerSystem( ConsoleCommandType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  ConsoleCommandType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConsoleResultEnum CommandHandlerSystem( ConsoleCommandType *ptCommand )
{
    ConsoleResultEnum eResult = CONSOLE_RESULT_OK;
    
	if( ( ConsoleStringEqual( ptCommand->pcArgArray[ 1 ], COMMAND_STRING__HELP ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 ) )
    {
        ConsolePrintf( ptCommand->eConsolePort, "SYS Commands:\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "---------------------\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SYS STAT                - Shows the status of the system application\r\n" );                        
        ConsolePrintf( ptCommand->eConsolePort, "SYS ENABLE <operation#> <BOOL> - Enable/Disable system app operations.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "    List of Operations:\r\n" );
        for( ControlActionType eAction = 0; eAction < CONTROL_ACTION_MAX ; eAction++ )
        {
            // the [15] is just an offset into the string returned by the ControlSystemActionGetStringName() function
            ConsolePrintf( ptCommand->eConsolePort, "%2d %-20s\r\n", eAction, (&((CHAR *)ControlSystemActionGetStringName( eAction ))[15]) );
        }
        ConsolePrintf( ptCommand->eConsolePort, "100 ALL\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "99 ALL except Main state machine\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "98 FTP TRANSMISSIONS\r\n" );        

        ConsolePrintf( ptCommand->eConsolePort, "SYS REQ <action#> - Request the system to perform an Action.\r\n" );     
        ConsolePrintf( ptCommand->eConsolePort, "    List of Actions:\r\n" );
        for( ControlActionType eAction = 1; eAction < CONTROL_ACTION_MAX ; eAction++ )
        {
            // the [15] is just an offset into the string returned by the ControlSystemActionGetStringName() function
            ConsolePrintf( ptCommand->eConsolePort, "%2d %-20s\r\n", eAction, (&((CHAR *)ControlSystemActionGetStringName( eAction ))[15]) );
        }        
        ConsolePrintf( ptCommand->eConsolePort, "SYS SCHEDULE SHOW - Print modifiable action schedules.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SYS SCHEDULE SET <ACTION_TYPE> <SCHEDULE_PATTERN> - Set the type of schedule pattern for the action selected.\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SYS SCHEDULE NOTE! To change transmission minute offset into the day, it has to be done from config module.\r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "SYS WDEN <1/0>  - Enable disable Watchdog\r\n" );                        
        ConsolePrintf( ptCommand->eConsolePort, "SYS WDSTAT  - Indicates if Watchdog Enable disable \r\n" );                        
        ConsolePrintf( ptCommand->eConsolePort, "SYS TEST ENABLE - Dont run this tests unless you know what you are doing \r\n" );

        ConsolePrintf( ptCommand->eConsolePort, "SYS SUPER <1/0>  - Enable disable Supervisory task\r\n" );
        ConsolePrintf( ptCommand->eConsolePort, "SYS SUPERSTAT  - Indicates if supervisory task is Enable disable \r\n" );        
    }            
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "TEST"))
    {
        if( ptCommand->bNumArgs == 3 )
        {
            UINT8 bEnable = atoi( ptCommand->pcArgArray[2] );
            
            ControlAllOperationsEnable( bEnable );
            ControlMainBoardSup3v3Enable( bEnable );
        }
    }
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "WDSTAT"))
    {
        ConsolePrintf( ptCommand->eConsolePort, "Enable = %d\r\n",  WatchdogIsEnabled() );        
    }
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "WDEN"))
    {                
        if( ptCommand->bNumArgs >= 3 )
        {
            UINT8 bEnable = atoi( ptCommand->pcArgArray[2] );

            if( bEnable == 1 )
            {
                WatchdogEnable( TRUE, 10000 );
            }
            else
            {
                WatchdogEnable( FALSE, 0 );
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "SUPER"))
    {                
        if( ptCommand->bNumArgs >= 3 )
        {
            UINT8 bEnable = atoi( ptCommand->pcArgArray[2] );

            if( bEnable == 1 )
            {
                TaskDemoSystemCycleSupervisionEnable( TRUE );
            }
            else
            {
                TaskDemoSystemCycleSupervisionEnable( FALSE );
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "SUPERSTAT"))
    {                
        ConsolePrintf( ptCommand->eConsolePort, "Enable = %d\r\n",  TaskDemoSystemCycleSupervisionIsEnabled() );
    }
    else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  "SCHEDULE"))
    {                
        if( ptCommand->bNumArgs >= 3 )
        {
            if ( ConsoleStringEqual(ptCommand->pcArgArray[2],  "SHOW"))
            {       
                // print control array
                ConsolePrintf( ptCommand->eConsolePort, "Transmit minute offset in the day: \r\n" );
                ControlPrintSchedule( ptCommand->eConsolePort, CONTROL_SCHEDULE_TYPE_TRANSMIT_EVENT_LOG );

//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Action Type: %d\r\n", CONTROL_SCHEDULE_TYPE_TRANSMIT_SENSOR_LOG );
//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Name: Transmit Sensor Log\r\n" );
//                ControlPrintSchedule( ptCommand->eConsolePort, CONTROL_SCHEDULE_TYPE_TRANSMIT_SENSOR_LOG );

//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Action Type: %d\r\n", CONTROL_SCHEDULE_TYPE_TRANSMIT_EVENT_LOG );
//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Name: Transmit Event Log\r\n" );
//                ControlPrintSchedule( ptCommand->eConsolePort, CONTROL_SCHEDULE_TYPE_TRANSMIT_EVENT_LOG );

//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Action Type: %d\r\n", CONTROL_SCHEDULE_TYPE_CAM_CAPTURE_TRANSMIT_IMG );
//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Name: Transmit-Capture image\r\n" );
//                ControlPrintSchedule( ptCommand->eConsolePort, CONTROL_SCHEDULE_TYPE_CAM_CAPTURE_TRANSMIT_IMG );

//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Action Type: %d\r\n", CONTROL_SCHEDULE_TYPE_DOWNLOAD_SCRIPT );
//                ConsolePrintf( ptCommand->eConsolePort, "Schedule Name: Download script\r\n" );
//                ControlPrintSchedule( ptCommand->eConsolePort, CONTROL_SCHEDULE_TYPE_DOWNLOAD_SCRIPT );
            }
            else if ( ConsoleStringEqual(ptCommand->pcArgArray[2],  "SET"))
            {
                if( ptCommand->bNumArgs == 5 )
                {
                    BOOL    fSuccess = FALSE;
                    UINT8   bActionType;
                    UINT8   bSchedulePattern;
                    UINT16  wConfigParamId;

                    bActionType     = atoi( ptCommand->pcArgArray[3] );
                    bSchedulePattern= atoi( ptCommand->pcArgArray[4] );

                    // validate data
                    switch( bActionType )
                    {        
//                        case CONTROL_SCHEDULE_TYPE_TRANSMIT_SENSOR_LOG:
//                            if( bSchedulePattern < CONTROL_SEND_SENSOR_DATA_DAY_HOUR_PATTERN_MAX )
//                            {
//                                wConfigParamId = CONFIG_SYS_SCHEDULE_TRANSMIT_DATA_LOG;
//                                fSuccess = TRUE;
//                            }
//                            break;
//                        case CONTROL_SCHEDULE_TYPE_TRANSMIT_EVENT_LOG:
//                            if( bSchedulePattern < CONTROL_SEND_EVENT_LOG_DAY_HOUR_PATTERN_MAX )
//                            {
//                                wConfigParamId = CONFIG_SYS_SCHEDULE_TRANSMIT_EVENT_LOG;
//                                fSuccess = TRUE;
//                            }                            
//                            break;
//                        case CONTROL_SCHEDULE_TYPE_CAM_CAPTURE_TRANSMIT_IMG:
//                            if( bSchedulePattern < CONTROL_CAPTURE_IMAGE_DAY_HOUR_PATTERN_MAX )
//                            {
//                                wConfigParamId = CONFIG_SYS_SCHEDULE_CAM_CAPTURE_IMAGE;
//                                fSuccess = TRUE;
//                            }                            
//                            break;
//                        case CONTROL_SCHEDULE_TYPE_DOWNLOAD_SCRIPT:
//                            if( bSchedulePattern < CONTROL_DOWNLOAD_SCRIPT_DAY_HOUR_PATTERN_MAX )
//                            {
//                                wConfigParamId = CONFIG_SYS_SCHEDULE_DOWNLOAD_SCRIPT;
//                                fSuccess = TRUE;
//                            }                            
                            break;
                    }
                    
                    if( fSuccess )
                    {
                        if( ConfigSetValueByConfigType( CONFIG_TYPE_SETTINGS, wConfigParamId, &bSchedulePattern ) == FALSE )
                        {
                            eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;
                        }
                        else
                        {
                            // save changes in config
                            ConfigParametersSaveByConfigType( CONFIG_TYPE_SETTINGS );
                        }
                    }
                    else
                    {
                        eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
                    }
                }
                else
                {
                    eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
                }
            }
            else
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
	}    
	else if ( ConsoleStringEqual(ptCommand->pcArgArray[1],  COMMAND_STRING__STAT))
    {                
        ControlPrintStat( ptCommand->eConsolePort );
	}    
    // SYS ENABLE <operation#> <BOOL>
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "ENABLE"))
	{		   
        if( ptCommand->bNumArgs == 4 )
        {
            BOOL    fEnable            = FALSE;	
            UINT8   bOperationNumber   = 0;            

            bOperationNumber    = atoi( ptCommand->pcArgArray[2] );
            fEnable             = atoi( ptCommand->pcArgArray[3] );

            if( bOperationNumber == 100 )
            {
                ControlAllOperationsEnable( fEnable );
            }
            else if( bOperationNumber == 99 )
            {
                for( ControlActionType eAction = 1; eAction < CONTROL_ACTION_MAX ; eAction++ )
                {
                    ControlSystemEnable( eAction, fEnable );
                }   
            }
            else if( bOperationNumber == 98 )
            {
                ControlTransmissionEnable( fEnable );
            }
            else if( ControlSystemEnable( bOperationNumber, fEnable ) == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_INVALID_ARG_VALUE;
            }                                       
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }    
    // SYS REQ <action#>
    else if (ConsoleStringEqual(ptCommand->pcArgArray[1],  "REQ"))
	{		   
        if( ptCommand->bNumArgs == 3 )
        {            
            UINT8   bActionNumber   = 0;            

            bActionNumber    = atoi( ptCommand->pcArgArray[2] );                        

            if( ControlSystemRequest( ( ControlActionType )bActionNumber ) == FALSE )
            {
                eResult = CONSOLE_RESULT_ERROR_PROCESSING_FAIL;                    
            }
        }
        else
        {
            eResult = CONSOLE_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }    
	else
    {
        eResult = CONSOLE_RESULT_ERROR_INVALID_COMMAND;
    }
    
    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

