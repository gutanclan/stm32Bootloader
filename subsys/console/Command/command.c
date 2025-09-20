////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \file       Command.c
////! \brief      Command Library for the system
////!
////! \author     Joel Minski [jminski@puracom.com], Puracom Inc.
////! \author     Craig Stickel [cstickel@puracom.com], Puracom Inc.
////! \date       March 5, 2010
////!
////! \details    Generates and registers the command library for the system, implementing the
////!             various handlers when invoked by the console task.
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//// Header include
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// C Library includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../drivers/common/types.h"
#include "../../../drivers/common/timer.h"
#include "../../../drivers/common/adcUtils.h"
#include "../../../drivers/common/stringUtils.h"

#include "command.h"
#include "../../../drivers/gpio/gpio.h"
#include "../../../drivers/adc/adc.h"
#include "../../../drivers/uart/usart.h"
#include "../../../drivers/spi/spi.h"
#include "../../../drivers/rtc/rtc.h"
#include "../../../drivers/rtc/rtcBackupReg.h"
#include "../../../drivers/common/BkpSram.h"

#include "../../../drivers/flash/FlashMem/NandSimu.h"
#include "../../memory/Flash.h"
#include "../../memory/FlashMemMap.h"
#include "../../filesystem/FileHeader.h"
#include "../../filesystem/FileDir.h"

#include "../../../services/DevInfo.h"
#include "../../serial/serial.h"
#include "../../iostream/ioStream.h"

//#include "../../XModem/xModem.h"
//#include "../../XModem/yModem.h"

#include "../../net/yModem.h"

#include "../../../services/SysBoot.h"
#include "../../../services/SysTime.h"

const CHAR 		* const		pcCommandResultNameLookupArray[] =
{
    "UNKNOWN",
    "OK",
    "COMMAND_INVALID",
    "ARGUMENT_AMOUNT_INVALID",
    "ARGUMENT_VALUE_INVALID",
    "PROCESSING_FAIL"
};

#define COMMAND_RESULT_NAME_ARRAY_MAX	( sizeof(pcCommandResultNameLookupArray) / sizeof( pcCommandResultNameLookupArray[0] ) )

////////////////////////////////////////////////////////////////////////////////////////////////////
//// Local Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

static CommandResultEnum    CommandHandlerHelp          ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerComment       ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerGpio          ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerAdc           ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerUsart         ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerSerial        ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerSpi           ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerSram          ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerFlashNand     ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerFile          ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerRtcBackupReg  ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerYModem        ( CommandArgsType *ptCommand );
static CommandResultEnum    CommandHandlerSystem        ( CommandArgsType *ptCommand );

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------------------
// DICTIONARY TABLES
//----------------------------------------------------------------------------------------
static const CommandDictionaryType   gtCommandTable1LookupArray[] =
{
    // NOTE: HELP command always at the beginning to let HELP ALL command work correctly!!
    {   "HELP",         CommandHandlerHelp,             "Command Help Information"      },
    {   "GPIO",         CommandHandlerGpio,             "GPIO Commands"                 },
    {   "ADC",          CommandHandlerAdc,              "ADC Commands"                  },
    {   "USART",        CommandHandlerUsart,            "Usart Commands"                },
    {   "SERIAL",       CommandHandlerSerial,           "Serial Commands"               },
    {   "SPI",          CommandHandlerSpi,              "Spi Commands"                  },
    {   "SRAM",         CommandHandlerSram,             "Sram Commands"                 },
    {   "NAND",         CommandHandlerFlashNand,        "Flash Nand Commands"           },
    {   "FILE",         CommandHandlerFile,             "File Commands"                 },
    {   "RBR",          CommandHandlerRtcBackupReg,     "RtcBackupReg Commands"         },
    {   "YMODEM",       CommandHandlerYModem,           "YModem Commands"               },
    {   "SYS",          CommandHandlerSystem,           "System Commands"               },
    {   "#",            CommandHandlerComment,          "Comment"                       },

    {   NULL,           NULL,                           NULL                            }
};
//----------------------------------------------------------------------------------------

// DICTIONARY COLLECTION POINTER ARRAY
static CommandDictionaryType * gptDictionaryTablesCollection[] =
{
    &gtCommandTable1LookupArray[0]
};

#define COMMAND_DICTIONARY_ARRAY_MAX  ( sizeof(gptDictionaryTablesCollection) / sizeof(gptDictionaryTablesCollection[0] ) )

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     const ConsoleDictionary * CommandGetDictionaryPointer( CommandDictionaryEnum eDictionary )
//!
//! \brief  returns the pointer of the specified dictionary
//!
//! \return pointer
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CommandDictionaryType * CommandDictionaryGetPointer( CommandDictionaryEnum eDictionary )
{
    CommandDictionaryType *ptDictionary = NULL;

    if( eDictionary < COMMAND_DICTIONARY_ARRAY_MAX )
    {
        ptDictionary = gptDictionaryTablesCollection[eDictionary];
    }

    return ptDictionary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn
//!
//! \brief
//!
//! \return
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * CommandResultGetStringPrt( CommandResultEnum eCommandResult )
{
    CHAR * pcResultString = NULL;

    if( eCommandResult < COMMAND_RESULT_NAME_ARRAY_MAX )
    {
        pcResultString = &pcCommandResultNameLookupArray[eCommandResult][0];
    }

    return pcResultString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn
//!
//! \brief
//!
//! \return
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CommandIsCommandValid( CommandDictionaryType *ptDictionary, CHAR *pszCommandString, UINT32 *pdwDictionaryCommandIndex )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( NULL != ptDictionary ) &&
        ( NULL != pszCommandString ) &&
        ( NULL != pdwDictionaryCommandIndex )
    )
    {
        (*pdwDictionaryCommandIndex) = 0;

        // Search through the command dictionary (skip if no token found)
		while( ptDictionary[(*pdwDictionaryCommandIndex)].pszCommandString != NULL )
		{
			if( StringUtilsStringEqual( (const CHAR *)ptDictionary[(*pdwDictionaryCommandIndex)].pszCommandString, (const CHAR *)pszCommandString ) == TRUE )
			{
				fSuccess = TRUE;
				break;
			}

			// Advance to next dictionary entry
			(*pdwDictionaryCommandIndex)++;
		}// end while loop
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerComment( CommandArgsType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  CommandArgsType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CommandResultEnum CommandHandlerComment( CommandArgsType *ptCommand )
{
    return COMMAND_RESULT_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         CommandResultEnum CommandHandlerHelp( CommandArgsType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  CommandArgsType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     CommandResultEnum result type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CommandResultEnum CommandHandlerHelp( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if( ptCommand->bNumArgs == 1 )
    {
        CommandDictionaryType *psDictionary = NULL;

        Printf( ptCommand->eConsolePort,  "All Commands:\r\n" );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );

        ConsoleCommandDictionaryGet( ptCommand->eConsolePort, &psDictionary );

        if( psDictionary != NULL )
        {
            for( UINT8 c = 0 ; psDictionary[c].pszCommandString != NULL ; c++ )
            {
                Printf( ptCommand->eConsolePort,  "%-20s %-50s\r\n", psDictionary[c].pszCommandString, psDictionary[c].pszDescription );
            }
        }
    }
    else if( ptCommand->bNumArgs == 2 )
    {
        if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) )
        {
            Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
            Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
            Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "LIST",                         "Show all commands" );
            Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "CONSOLE",                      "Simple Instructions to use the console" );
        }
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"LIST" ) )
        {
            CommandDictionaryType *psDictionary = NULL;
            CommandArgsType		   tCommandArgs;
            const CHAR            *pcString[2]  =
            {
                NULL,
                "HELP"
            };

            ConsoleCommandDictionaryGet( ptCommand->eConsolePort, &psDictionary );

            if( psDictionary != NULL )
            {
                for( UINT8 c = 0 ; psDictionary[c].pszCommandString != NULL ; c++ )
                {
                    if( psDictionary[c].pszCommandString[0] != '#' )
                    {
                        tCommandArgs.pcCommand = &psDictionary[c].pszCommandString[0];
                        tCommandArgs.bNumArgs = 1;
                        tCommandArgs.eConsolePort = ptCommand->eConsolePort;
                        tCommandArgs.pcArgArray = pcString;
                        ( psDictionary[c].pfnCommandHandler )( &tCommandArgs );
                        Printf( ptCommand->eConsolePort,  "\r\n\r\n" );
                    }
                }
            }
        }
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"CONSOLE" ) )
        {

        }
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TEST" ) )
        {
            CHAR *pcArr [] =
            {
                "TRUE",
                "false",
                "-1",
                "255",
                "'there 1'",
                "'there 2",
                "there 3'",
                "1.3",
                "-1000",
                "+1000"
            };

            UINT8 bSize = sizeof(pcArr) / sizeof(pcArr[0]);

            CHAR bBuff[20];

            for( UINT8 c = 0; c < bSize ; c++ )
            {
                BOOL fIsBoolErr = FALSE;
                BOOL fIsByteErr = FALSE;
                BOOL fIsU32Err = FALSE;
                BOOL fIsI32Err = FALSE;
                BOOL fIsFloErr = FALSE;
                BOOL fIsStrArgErr = FALSE;

                BOOL fBool = FALSE;
                UINT8 bByte = 0;
                UINT32 dwU32 = 0;
                INT32 idwI32 = 0;
                FLOAT sgFlo = 0.0f;
                CHAR * pcStrPtr = NULL;

                fIsBoolErr  = StringToBool(&pcArr[c][0],&fBool);
                fIsByteErr  = StringToByte(&pcArr[c][0],&bByte);
                fIsU32Err   = StringToU32(&pcArr[c][0],&dwU32);
                fIsI32Err   = StringToI32(&pcArr[c][0],&idwI32);
                fIsFloErr   = StringToFloat(&pcArr[c][0],&sgFlo);
                memset( &bBuff[0], 0, sizeof(bBuff) );

                strncpy( (char *)&bBuff[0],(const char *) &pcArr[c][0], sizeof(bBuff)-1 );

                fIsStrArgErr= StringArgStripSymbol(&bBuff[0],strlen((const char *)&bBuff[0]),'\'',&pcStrPtr);

                Printf( ptCommand->eConsolePort,  "[%-15s]\r\n", &pcArr[c][0] );
                Printf( ptCommand->eConsolePort,  "STR            BOL BYT U32 I32 FLT\r\n" );
                Printf( ptCommand->eConsolePort,  "     %-10d %-3d %-3d %-3d %-3d %-3d\r\n", fIsStrArgErr, fIsBoolErr, fIsByteErr, fIsU32Err, fIsI32Err, fIsFloErr );
                Printf( ptCommand->eConsolePort,  "[%-12s] %-3d %-3d %-3d %-3d %-3.2f\r\n", pcStrPtr, fBool, bByte, dwU32, idwI32, sgFlo );
                Printf( ptCommand->eConsolePort,  "\r\n" );
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn         ConsoleErrorEnum CommandHandlerGpio( CommandArgsType *ptCommand )
////!
////! \brief      Executes console command lines sent to the device.
////!
////! \param[in]  CommandArgsType *ptCommand command structure.
////!                 ptCommand->eConsolePort Port to print the command responses to.
////!                 ptCommand->bNumArgs     Number of tokens present in the command.
////!                 ptCommand->pcArgArray[] Array of strings which each hold a token
////!                                         from the command line.
////!
////! \return     ConsoleErrorEnum error type
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
CommandResultEnum CommandHandlerGpio( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( ptCommand->bNumArgs == 1 ) ||
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                          "Mcu pin status" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "CONT <cPort> <nPin> [nMSec]",   "mSecond Pin status" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "OREG <cPort> <nPin> <1|0>",     "Set output register" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        UINT8           bPin;
        UINT8           bPort;

		BOOL			fPortEnable;
		GpioOTypeEnum 	eOTypeResult;
		GpioPuPdEnum  	ePuPdResult;
		GpioOSpeedEnum  eSpeed;
		GpioLogicEnum 	eLogicRegIn;
		GpioLogicEnum 	eLogicRegOut;
		GpioModeEnum 	eModeResult;
		GpioAlternativeFunctionEnum eAfType;

        if( ptCommand->bNumArgs == 2 )
        {
            for( bPort = 0 ; bPort < GPIO_PORT_MAX ; bPort++ )
            {
				fPortEnable = GpioPortClockIsEnabled( bPort );

				Printf( ptCommand->eConsolePort, "Port %c\r\n", (bPort+65) );
				Printf( ptCommand->eConsolePort, "Clock Enable %d\r\n", fPortEnable );

                Printf
                (
                    ptCommand->eConsolePort, "%-3s   %-10s %-10s %-10s %-10s %-2s %-2s   %-20s\r\n",
                    "Pin",
                    "Otype",
                    "PuPd",
                    "Speed",
                    "Mode",
                    "RI",
                    "RO",
                    "AF"
                );

                for( bPin = 0 ; bPin < GPIO_PORT_PIN_AMOUNT ; bPin++ )
                {
					GpioOTypeGetVal( bPort, bPin, &eOTypeResult );
					GpioPuPdGetVal( bPort, bPin, &ePuPdResult );
					GpioSpeedGetVal( bPort, bPin, &eSpeed );
					GpioModeGetVal( bPort, bPin, &eModeResult );

					GpioInputRegRead( bPort, bPin, &eLogicRegIn );
					GpioOutputRegRead( bPort, bPin, &eLogicRegOut );

                    GpioAfGetType( bPort, bPin, &eAfType );

					Printf
					(
						ptCommand->eConsolePort, " %02d   %-10s %-10s %-10s %-10s %2d %2d   %-20s\r\n",
						bPin,
						GpioOTypeGetName( eOTypeResult ),
						GpioPuPdGetName( ePuPdResult ),
						GpioSpeedGetName( eSpeed ),
						GpioModeGetName( eModeResult ),
						eLogicRegIn,
						eLogicRegOut,
						GpioAfGetName( eAfType )
					);
                }
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    // CONT <cPort> <nPin> [nMSec]
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"CONT" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 5 )
        {

        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn         ConsoleErrorEnum CommandHandlerGpio( CommandArgsType *ptCommand )
////!
////! \brief      Executes console command lines sent to the device.
////!
////! \param[in]  CommandArgsType *ptCommand command structure.
////!                 ptCommand->eConsolePort Port to print the command responses to.
////!                 ptCommand->bNumArgs     Number of tokens present in the command.
////!                 ptCommand->pcArgArray[] Array of strings which each hold a token
////!                                         from the command line.
////!
////! \return     ConsoleErrorEnum error type
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
CommandResultEnum CommandHandlerAdc( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( ptCommand->bNumArgs == 1 ) ||
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                          "Mcu pin status" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ENABLE <ADC_NAME> <0|1>",       "Enable Adc" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        AdcInputEnum    eAdc;
        GpioPortEnum    eGpioPort;
        UINT8           bGpioPin;

        CHAR            cGpioPort;
        INT16           iwGpioPin;

		BOOL			fEnable;
		FLOAT           sgRaw;
		FLOAT           sgmVolt;
		FLOAT           sgtoUnit;
		CHAR            cUnitName[5];

        Printf
        (
            ptCommand->eConsolePort, "%-7s %-8s %-10s %-10s %-10s %-6s %-6s %s\r\n",
            "Name",
            "Enable",
            "Raw",
            "mVolt",
            "OtherU",
            "Unit",
            "Port",
            "Pin"
        );

        for( eAdc = 0 ; eAdc < ADC_PIN_MAX ; eAdc++ )
        {

            memset( &cUnitName[0],0,sizeof(cUnitName));

            fEnable = FALSE;
            sgRaw = 0;
            sgmVolt = 0;
            sgtoUnit = 0;

            cGpioPort = '-';
            iwGpioPin = -1;

            AdcIsEnable( eAdc, &fEnable );
            AdcGetGpioPort( eAdc, &eGpioPort, &bGpioPin );

            if( fEnable )
            {
                AdcReadRaw( eAdc, 10, &sgRaw );
                AdcUtilsRawToMilliVolts( sgRaw, ADC_RESOLUTION_VAL_MAX, ADC_MILLIVOLT_VAL_MAX, &sgmVolt );

                if( eAdc == ADC_TEMP )
                {
                    AdcMilliVoltsToToDegC( eAdc, sgmVolt, &sgtoUnit );
                }
            }

            if( eAdc == ADC_TEMP )
            {
                snprintf( (char *)&cUnitName[0], (sizeof(cUnitName)-1), "degC" );
            }
            else
            {
                snprintf( (char *)&cUnitName[0], (sizeof(cUnitName)-1), "----" );
            }

            if( eGpioPort == GPIO_PORT_INVALID )
            {
                cGpioPort = '-';
                iwGpioPin = 0;
            }
            else
            {
                cGpioPort  = eGpioPort+65;
                iwGpioPin = bGpioPin;
            }

            Printf
            (
                ptCommand->eConsolePort, "%-7s %-8d %07.2f    %07.2f    %07.2f    %-6s %-6c %-02d\r\n",
                AdcGetName( eAdc ),
                fEnable,
                sgRaw,
                sgmVolt,
                sgtoUnit,
                &cUnitName[0],
                cGpioPort,
                iwGpioPin
            );
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ENABLE" ) == TRUE )
    {
        for( AdcInputEnum eAdc = 0 ; eAdc < ADC_PIN_MAX ; eAdc++ )
        {
            AdcEnable( eAdc, TRUE );
        }

        if( ptCommand->bNumArgs == 4 )
        {

        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConsoleErrorEnum CommandHandlerUsart( CommandArgsType *ptCommand )
//!
//! \brief      Executes console command lines sent to the device.
//!
//! \param[in]  CommandArgsType *ptCommand command structure.
//!                 ptCommand->eConsolePort Port to print the command responses to.
//!                 ptCommand->bNumArgs     Number of tokens present in the command.
//!                 ptCommand->pcArgArray[] Array of strings which each hold a token
//!                                         from the command line.
//!
//! \return     ConsoleErrorEnum error type
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CommandResultEnum CommandHandlerUsart( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                        "Usart port status" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        UINT8               bPort;
        FLOAT               sgBaudRate;
        UsartModeEnum       eMode;
        UsartWordLengthEnum eWordLen;
        UsartStopBitEnum    eStopBit;
        UsartParityEnum     eParity;

        Printf
        (
            ptCommand->eConsolePort, "%-10s %-8s %-8s %-8s %-8s %-8s %-8s %-10s\r\n",
            "Name",
            "ClockEn",
            "PortEn",
            "Mode",
            "Data",
            "Stop",
            "Parity",
            "BRate"
        );

        for( bPort = 0 ; bPort < USART_PORT_TOTAL ; bPort++ )
        {
            UsartPortGetBaud( bPort, &sgBaudRate );
            UsartPortGetMode( bPort, &eMode );
            UsartPortGetWordLength( bPort, &eWordLen );
            UsartPortGetStopBit( bPort, &eStopBit );
            UsartPortGetParity( bPort, &eParity );

            Printf
            (
                ptCommand->eConsolePort, "%-10s %-8d %-8d %-8s %-8s %-8s %-8s %9.2f\r\n",
                UsartPortGetName( bPort ),
                UsartPortClockIsEnabled( bPort ),
                UsartPortIsEnabled( bPort ),
                UsartPortGetModeName( eMode ),
                UsartPortGetWordLenName( eWordLen ),
                UsartPortGetStopBitName( eStopBit ),
                UsartPortGetParityName( eParity ),
                sgBaudRate
            );
        }
    }
    else{
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerSerial( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                       "Port status" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        UINT8               bPort;
        UsartPortEnum       eUsart;
        GpioPortEnum        eGpioPortRx;
        UINT8               bGpioPinRx;
        GpioPortEnum        eGpioPortTx;
        UINT8               bGpioPinTx;

        Printf
        (
            ptCommand->eConsolePort, "%-10s %-8s %-8s %-8s %-8s %-8s\r\n",
            "Name",
            "PortEn",
            "Usart",
            "Rx",
            "Tx",
            "ClientConn"
        );

        for( bPort = 0 ; bPort < SERIAL_PORT_MAX ; bPort++ )
        {
            SerialGetUsart( bPort, &eUsart );
            SerialGetGpioRx( bPort, &eGpioPortRx, &bGpioPinRx );
            SerialGetGpioTx( bPort, &eGpioPortTx, &bGpioPinTx );

            Printf
            (
                ptCommand->eConsolePort, "%-10s %-8d %-8s %c%02d      %c%02d %8d\r\n",
                SerialPortGetName( bPort ),
                SerialPortIsEnabled( bPort ),
                UsartPortGetName(eUsart),
                (eGpioPortRx + 65),
                bGpioPinRx,
                (eGpioPortTx + 65),
                bGpioPinTx,
                SerialPortIsClientConnected( bPort )
            );
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerSram( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ENABLE <TRUE|FALSE>",           "sram enable" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "WRITE <OFFSET> <VAL> <LEN>",    "Write test" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "READ  <OFFSET>",                "Reads 50 bytes@offset" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ERASE <OFFSET> <LEN>",          "Erase test" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ENABLE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            BOOL fEnable;

            if( StringToBool( ptCommand->pcArgArray[ 2 ], &fEnable ) )
            {
                if( BkpSramEnable( fEnable ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"WRITE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 5 )
        {
            UINT32  dwOffset;
            UINT32  dwVal;
            UINT32  dwLen;
            BOOL    fIsValid = TRUE;

            fIsValid &= StringToU32( ptCommand->pcArgArray[ 2 ], &dwOffset );
            fIsValid &= StringToU32( ptCommand->pcArgArray[ 3 ], &dwVal );
            fIsValid &= StringToU32( ptCommand->pcArgArray[ 4 ], &dwLen );
            fIsValid &= (dwVal<0xFF);
            fIsValid &= dwLen<=BKP_SRAM_SIZE;

            if( fIsValid )
            {
                for( UINT32 c = 0 ; c < dwLen ; c++ )
                {
                    if( BkpSramWrite( dwOffset+c, &dwVal, 1 ) == FALSE )
                    {
                        eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                        break;
                    }
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"READ" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            UINT32  dwOffset;
            UINT8   bBuffer[50];

            if( StringToU32( ptCommand->pcArgArray[ 2 ], &dwOffset ) )
            {
                if( BkpSramRead( dwOffset, &bBuffer[0], sizeof(bBuffer) ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
                else
                {
                    for( UINT32 c = 0 ; c < sizeof(bBuffer) ; c++ )
                    {
                        if( ( c % 10 ) == 0 )
                        {
                            Printf( ptCommand->eConsolePort,  "\r\n" );
                        }
                        Printf( ptCommand->eConsolePort,  "[0x%02X]", bBuffer[c] );
                    }
                    Printf( ptCommand->eConsolePort,  "\r\n" );
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ERASE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            UINT32  dwOffset;
            UINT32  dwLen;
            BOOL    fIsValid = TRUE;

            fIsValid &= StringToU32( ptCommand->pcArgArray[ 2 ], &dwOffset );
            fIsValid &= StringToU32( ptCommand->pcArgArray[ 3 ], &dwLen );
            fIsValid &= dwLen<=BKP_SRAM_SIZE;

            if( fIsValid )
            {
                if( BkpSramErase( dwOffset, dwLen ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerFile( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "TEST",                          "Test moving of blocks" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TEST" ) == TRUE )
    {
        FileDirTest();
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerFlashNand( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "READPG <PG#> <BYTE_OFFSET> <SIZE>",    "Read bytes" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "WRITEPG <PG#> <PG> <CHAR> <SIZE>",     "Write page" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "WPGOFF <PG#> <PG> <CHAR> <SIZE>",     "Write page" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ERASEPG <PG#> <PG> <CHAR> <SIZE>",     "Write page" );

        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ERASESS <SS#>",                  "Erase sub-sector" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "ERASEALL",                     "Erase all" );

        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "TEST",                          "Test moving of blocks" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TEST" ) == TRUE )
    {
        FlashMemMapBlockTest();
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"READPG" ) == TRUE )
    {
        UINT8 bDataBuffer[FLASH_PAGE_SIZE_BYTES];

        memset(&bDataBuffer[0], 0xff, sizeof(bDataBuffer));

        UINT32 dwPage = 1; // start from 0
        UINT32 dwPageByteAddr = (dwPage) * NAND_SIMU_PAGE_SIZE_BYTES;

        //BOOL FlashWritePage		    ( FlashIdEnum eFlashId, UINT32 dwPageNumber, UINT8 *pbDataBuffer, UINT32 dwDataLength );
        //BOOL fPasss = FlashReadPage( FLASH_ID_NAND_SIMU_1, dwPage, sizeof(bDataBuffer), &bDataBuffer[0], sizeof(bDataBuffer) );
        //BOOL FlashErasePage    		( FlashIdEnum eFlashId, UINT32 dwPage, UINT8 *pbRecreateBlockBuffer, UINT32 dwRecreateBlockBufferSize );
        BOOL fPasss = NandSimuReadAtByteAddress( NAND_SIMU_ARRAY_ID_1, dwPageByteAddr, sizeof(bDataBuffer), &bDataBuffer[0], sizeof(bDataBuffer) );

        bDataBuffer[0] = bDataBuffer[0];
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"WPGOFF" ) == TRUE )
    {
        //UINT8 bDataBuffer[NAND_SIMU_PAGE_SIZE_BYTES];
        UINT8 bDataBuffer[20];

        memset(&bDataBuffer[0], 0xA3, sizeof(bDataBuffer));

        //NandSimuWriteAtPage( NAND_SIMU_ARRAY_ID_1, 4, &bDataBuffer[0], sizeof(bDataBuffer) );
        BOOL fPass = NandSimuWriteAtPageOffset( NAND_SIMU_ARRAY_ID_1, 1, 230, &bDataBuffer[0], sizeof(bDataBuffer) );

        //BOOL fPass = FlashWritePage( FLASH_ID_NAND_SIMU_1, 1, &bDataBuffer[0], sizeof(bDataBuffer) );

        bDataBuffer[0] = bDataBuffer[0];
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"WRITEPG" ) == TRUE )
    {
        //UINT8 bDataBuffer[NAND_SIMU_PAGE_SIZE_BYTES];
        UINT8 bDataBuffer[FLASH_PAGE_SIZE_BYTES];

        memset(&bDataBuffer[0], 0xA3, sizeof(bDataBuffer));

        //NandSimuWriteAtPage( NAND_SIMU_ARRAY_ID_1, 4, &bDataBuffer[0], sizeof(bDataBuffer) );
        BOOL fPass = FlashWriteAtPageOffset( FLASH_ID_NAND_SIMU_1, 1, 0, &bDataBuffer[0], sizeof(bDataBuffer) );

        bDataBuffer[0] = bDataBuffer[0];
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ERASEPG" ) == TRUE )
    {
        //UINT8 bDataBuffer[NAND_SIMU_PAGE_SIZE_BYTES];
        UINT8 bDataBuffer[FLASH_BLOCK_SIZE_BYTES];

        memset(&bDataBuffer[0], 0x11, sizeof(bDataBuffer));

        //NandSimuWriteAtPage( NAND_SIMU_ARRAY_ID_1, 4, &bDataBuffer[0], sizeof(bDataBuffer) );
        BOOL fPass = FlashErasePage( FLASH_ID_NAND_SIMU_1, 0, &bDataBuffer[0], sizeof(bDataBuffer) );

        bDataBuffer[0] = bDataBuffer[0];

    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ERASESS" ) == TRUE )
    {

    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"ERASEALL" ) == TRUE )
    {
        NandSimuEraseAll( NAND_SIMU_ARRAY_ID_1 );
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;

}

CommandResultEnum CommandHandlerRtcBackupReg( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                              "Reg Stat" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "SET <INDEX> <VAL>",                 "Set reg value" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        UINT32  dwVal       = 0;

        Printf( ptCommand->eConsolePort, "RTC BACKUP REGS\r\n" );

        for( UINT8 bRegCounter = 0; bRegCounter < RTC_BACKUP_REGS_AMOUNT ; bRegCounter++ )
        {
            RtcBackupRegGet( bRegCounter, &dwVal );

            Printf( ptCommand->eConsolePort, "[%02d:0x%02X]", bRegCounter, dwVal );

            if( bRegCounter%5 == 4 )
            {
                Printf( ptCommand->eConsolePort, "\r\n" );
            }
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"SET" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 4 )
        {
            BOOL    fArgValid   = TRUE;
            UINT8   bRegIdx     = 0;
            UINT32  dwVal       = 0;

            fArgValid &= StringToByte( ptCommand->pcArgArray[ 2 ], &bRegIdx );
            fArgValid &= StringToU32( ptCommand->pcArgArray[ 3 ], &dwVal );

            if( fArgValid )
            {
                if( RtcBackupRegSet( bRegIdx, dwVal ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerSystem( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                              "Operation Error" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "MODE <NORMAL|DISABLE>",             "Run Operation" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "RESET",                             "Software Hard Reset" );

        // add commands to tests system time
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "TIME <YYYY> <MM> <DD> <hh> <mm> <ss>","Set Date Time" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "TZONE <ZONE_OFFSET>",               "Set Time Zone" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "T2E <YYYY> <MM> <DD> <hh> <mm> <ss>","Time to Epoch" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "E2T <EPOCH>",                       "Epoch To Time" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"E2T" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            SysTimeDateTimeStruct tDateTime;
            BOOL fIsConverted;
            UINT32 dwVal;

            fIsConverted = TRUE;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 2 ], &dwVal );

            if( fIsConverted == FALSE )
            {
                eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
            }
            else
            {
                if( SysTimeEpochSecondsToDateTime( dwVal, &tDateTime ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
                else
                {
                    Printf( ptCommand->eConsolePort,  "%04d-%02d-%02d %02d:%02d:%02d\r\n",
                        tDateTime.tDate.wYear,
                        tDateTime.tDate.bMonth,
                        tDateTime.tDate.bDate,
                        tDateTime.tTime.bHour,
                        tDateTime.tTime.bMinute,
                        tDateTime.tTime.bSecond
                    );
                }
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"T2E" ) == TRUE )
    {
        SysTimeDateTimeStruct tDateTime;
        BOOL    fSuccess = FALSE;

        if( ptCommand->bNumArgs == 8 )
        {
            BOOL fIsConverted;
            UINT32 dwVal;

            fIsConverted = TRUE;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 2 ], &dwVal );
            tDateTime.tDate.wYear       = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 3 ], &dwVal );
            tDateTime.tDate.bMonth      = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 4 ], &dwVal );
            tDateTime.tDate.bDate       = dwVal;

            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 5 ], &dwVal );
            tDateTime.tTime.bHour       = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 6 ], &dwVal );
            tDateTime.tTime.bMinute     = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 7 ], &dwVal );
            tDateTime.tTime.bSecond     = dwVal;

            if( fIsConverted )
            {
                UINT32 dwEpoch = SysTimeDateTimeToEpochSeconds( &tDateTime );
                Printf( ptCommand->eConsolePort,  "EPOCH=%d\r\n", dwEpoch );
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"E2T" ) == TRUE )
    {

    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TZONE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 2 )
        {
            Printf( ptCommand->eConsolePort,  "UTC OFFSET %f\r\n", SysTimeGetUtcTimeZoneOffset() );
        }
        else if( ptCommand->bNumArgs == 3 )
        {
            BOOL fIsConverted;
            FLOAT sgVal;

            fIsConverted = TRUE;
            fIsConverted &= StringToFloat( ptCommand->pcArgArray[ 2 ], &sgVal );

            if( fIsConverted == FALSE )
            {
                eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
            }
            else
            {
                if( SysTimeSetUtcTimeZoneOffset( sgVal ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TIME" ) == TRUE )
    {
        SysTimeDateTimeStruct tDateTimeUtcZero;
        SysTimeDateTimeStruct tDateTimeUtcOffset;
        BOOL    fSuccess = FALSE;

        if( ptCommand->bNumArgs == 2 )
        {
            fSuccess = TRUE;
            fSuccess&= SysTimeGetDateTime( &tDateTimeUtcZero, FALSE );
            fSuccess&= SysTimeGetDateTime( &tDateTimeUtcOffset, TRUE );

            if( fSuccess == FALSE )
            {
                eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
            }
            else
            {
                Printf( ptCommand->eConsolePort,  "%04d-%02d-%02d %02d:%02d:%02d UTC %f\r\n",
                    tDateTimeUtcZero.tDate.wYear,
                    tDateTimeUtcZero.tDate.bMonth,
                    tDateTimeUtcZero.tDate.bDate,
                    tDateTimeUtcZero.tTime.bHour,
                    tDateTimeUtcZero.tTime.bMinute,
                    tDateTimeUtcZero.tTime.bSecond,
                    0.0
                );
                Printf( ptCommand->eConsolePort,  "%04d-%02d-%02d %02d:%02d:%02d UTC %f\r\n",
                    tDateTimeUtcOffset.tDate.wYear,
                    tDateTimeUtcOffset.tDate.bMonth,
                    tDateTimeUtcOffset.tDate.bDate,
                    tDateTimeUtcOffset.tTime.bHour,
                    tDateTimeUtcOffset.tTime.bMinute,
                    tDateTimeUtcOffset.tTime.bSecond,
                    SysTimeGetUtcTimeZoneOffset()
                );
            }
        }
        else if( ptCommand->bNumArgs == 8 )
        {
            BOOL fIsConverted;
            UINT32 dwVal;

            fIsConverted = TRUE;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 2 ], &dwVal );
            tDateTimeUtcZero.tDate.wYear       = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 3 ], &dwVal );
            tDateTimeUtcZero.tDate.bMonth      = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 4 ], &dwVal );
            tDateTimeUtcZero.tDate.bDate       = dwVal;

            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 5 ], &dwVal );
            tDateTimeUtcZero.tTime.bHour       = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 6 ], &dwVal );
            tDateTimeUtcZero.tTime.bMinute     = dwVal;
            fIsConverted &= StringToU32( ptCommand->pcArgArray[ 7 ], &dwVal );
            tDateTimeUtcZero.tTime.bSecond     = dwVal;

            if( fIsConverted )
            {
                if( SysTimeSetDateTimeUtcZero( &tDateTimeUtcZero ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"RESET" ) == TRUE )
    {
        SysBootForceSoftwareReset();
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        UINT8               bPort;
        UsartPortEnum       eUsart;
        GpioPortEnum        eGpioPortRx;
        UINT8               bGpioPinRx;
        GpioPortEnum        eGpioPortTx;
        UINT8               bGpioPinTx;

        Printf
        (
            ptCommand->eConsolePort, "%-10s %-8s\r\n",
            "State",
            "Mode"
        );

        Printf
        (
            ptCommand->eConsolePort, "%-10s %-8s\r\n",
            SysBootGetStateName( SysBootGetState() ),
            SysBootGetModeName( SysBootGetMode() )
        );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"MODE" ) == TRUE )
    {
        SysBootModeEnum eBootMode       = SYS_BOOT_MODE_MAX;
        BOOL            fIsModeValid    = FALSE;

        if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"NORMAL" ) == TRUE )
        {
            eBootMode   = SYS_BOOT_MODE_NORMAL;
            fIsModeValid= TRUE;
        }
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"DISABLED" ) == TRUE )
        {
            eBootMode   = SYS_BOOT_MODE_DISABLE;
            fIsModeValid= TRUE;
        }

        if( fIsModeValid )
        {
            if( SysBootSetMode( eBootMode ) == FALSE )
            {
               eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_INVALID_ARG_VALUE;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerYModem( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "STAT",                               "Operation Error" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "RECEIVE <BIN|ASCII>",                "Run Operation" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"RECEIVE" ) == TRUE )
    {
        if( ptCommand->bNumArgs == 3 )
        {
            UINT8                   bYModemBuffer[1024];
            yModemErrorTypeEnum     eError      = Y_MODEM_ERROR_NON;
            yModemTransferTypeEnum  eTransfType = Y_MODEM_TRANSFER_TYPE_MAX;

            if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"BIN" ) == TRUE )
            {
                eTransfType = Y_MODEM_TRANSFER_TYPE_BIN;
            }
            else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"ASCII" ) == TRUE )
            {
                eTransfType = Y_MODEM_TRANSFER_TYPE_ASCII;
            }

            if( eTransfType < Y_MODEM_TRANSFER_TYPE_MAX )
            {
                // set buffer used for protocol transactions. (minimum should be size 1024)
                yModemConfigBuffer(&bYModemBuffer[0], sizeof(bYModemBuffer));

                if( yModemReceiveFile( eTransfType, 30000, IO_STREAM_USER,IO_STREAM_USER,IO_STREAM_INVALID ) )
                {
                    eResult = COMMAND_RESULT_OK;
                }
                else
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }

                Printf( ptCommand->eConsolePort,  "\r\n" );
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"STAT" ) == TRUE )
    {
        yModemErrorTypeEnum eError = Y_MODEM_ERROR_UNKNOWN;
        yModemGetLastError( &eError );

        Printf( ptCommand->eConsolePort,  "Result: %s\r\n", yModemGetErrorName( eError ) );
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

CommandResultEnum CommandHandlerSpi( CommandArgsType *ptCommand )
{
    CommandResultEnum eResult = COMMAND_RESULT_OK;

    if
    (
        ( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"HELP" ) == TRUE ) ||
        ( ptCommand->bNumArgs == 1 )
    )
    {
        Printf( ptCommand->eConsolePort,  "%s Sub commands:\r\n", ptCommand->pcCommand );
        Printf( ptCommand->eConsolePort,  "---------------------\r\n" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "BUS STAT",                      "bus status" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "BUS ENABLE <BUS> <0|1>",        "bus enable" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "BUS ALLOW <SLAVE> <0|1>",       "bus retained by slave" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "SLAVE STAT",                    "slave status" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "SLAVE ASSERT <SLAVE> <0|1>",    "slave assert cs" );
        Printf( ptCommand->eConsolePort,  "%-40s - %-40s\r\n", "TEST",                          "Adesto DF read ID" );
        //Printf( ptCommand->eConsolePort,  "%-50s - %-50s\r\n", "SPI SLAVE SEND <SLAVE> <BYTE>",     "Spi slave send Byte" );
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"TEST" ) == TRUE )
    {
        TIMER   xTimer;

        if( SpiIsBusEnabled( SPI_BUS_1 ) == FALSE )
        {
            SpiBusEnable( SPI_BUS_1, TRUE );
        }

        SpiSlaveClaimBus( SPI_SLAVE_FLASH_MEM_1 );

        xTimer = TimerDownTimerStartMs( 10 );
        while( TimerDownTimerIsExpired(xTimer) == FALSE );

        SpiSlaveChipSelectAssert( SPI_SLAVE_FLASH_MEM_1, TRUE );

        xTimer = TimerDownTimerStartMs( 10 );
        while( TimerDownTimerIsExpired(xTimer) == FALSE );

        UINT8 bByteArraySend[4];
        UINT8 bByteArrayRecv[4];

        memset( &bByteArraySend[0], 0 , sizeof(bByteArraySend) );
        memset( &bByteArrayRecv[0], 0 , sizeof(bByteArrayRecv) );

        // build command (Device Id read) 0x9F
        bByteArraySend[0] = 0x9F;
        bByteArraySend[1] = 0;
        bByteArraySend[2] = 0;
        bByteArraySend[3] = 0;
//        bByteArraySend[4] = 0;
//        bByteArraySend[5] = 0;
//        bByteArraySend[6] = 0;
//        bByteArraySend[7] = 0;
//        bByteArraySend[8] = 0;
//        bByteArraySend[9] = 0;

        // response
        //[0] manuf ID 0
        //[1] manuf ID 1
        //[2] --
        //[3] ext dev inf (0x00)


        SpiSlaveSendRecvByteArray( SPI_SLAVE_FLASH_MEM_1, &bByteArraySend[0], &bByteArrayRecv[0], sizeof(bByteArrayRecv) );

        SpiSlaveChipSelectAssert( SPI_SLAVE_FLASH_MEM_1, FALSE );

        SpiSlaveReleaseBus( SPI_SLAVE_FLASH_MEM_1 );

        //SpiBusEnable( SPI_BUS_1, FALSE );

        Printf(ptCommand->eConsolePort, "\r\n");

        for( UINT8 c = 0; c < sizeof(bByteArrayRecv) ; c++ )
        {
            Printf(ptCommand->eConsolePort, "%d",bByteArrayRecv[c]);
        }

        Printf(ptCommand->eConsolePort, "\r\n");
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"BUS" ) == TRUE )
    {
        if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"STAT" ) == TRUE )
        {
            UINT8               bBus;
            SpiSlaveEnum        eSpiSlave;

            Printf
            (
                ptCommand->eConsolePort, "%-10s %-8s %-8s %-8s\r\n",
                "Name",
                "ClockEn",
                "Reserved",
                "Client"
            );

            for( bBus = 0 ; bBus < SPI_BUS_TOTAL ; bBus++ )
            {
                SpiGetSlaveHoldingBus( bBus, &eSpiSlave );

                Printf
                (
                    ptCommand->eConsolePort, "%-10s %-8d %-8d %-8s\r\n",
                    SpiBusGetName(bBus),
                    SpiIsBusEnabled( bBus ),
                    SpiIsAnySlaveHoldingBus( bBus ),
                    SpiSlaveGetName( eSpiSlave )
                );
            }
        }
        // SPI BUS ENABLE <BUS> <0|1>
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"ENABLE" ) == TRUE )
        {
            if( ptCommand->bNumArgs == 5 )
            {
                BOOL        fEnable = FALSE;
                SpiBusEnum  eSpiBus = SPI_BUS_INVALID;

                for( eSpiBus = 0 ; eSpiBus < SPI_BUS_TOTAL ; eSpiBus++ )
                {
                    if( StringUtilsStringEqual( SpiBusGetName(eSpiBus), ptCommand->pcArgArray[ 3 ] ) )
                    {
                        break;
                    }
                }

                fEnable = atoi((const char *)ptCommand->pcArgArray[ 4 ]);

                if( SpiBusEnable( eSpiBus, fEnable ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        // SPI BUS ALLOW <SLAVE> <0|1>
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"ALLOW" ) == TRUE )
        {
            if( ptCommand->bNumArgs == 5 )
            {
                BOOL            fEnable     = FALSE;
                SpiSlaveEnum    eSpiSlave   = SPI_SLAVE_INVALID;

                for( eSpiSlave = 0 ; eSpiSlave < SPI_SLAVE_TOTAL ; eSpiSlave++ )
                {
                    if( StringUtilsStringEqual( SpiSlaveGetName(eSpiSlave), ptCommand->pcArgArray[ 3 ] ) )
                    {
                        break;
                    }
                }

                fEnable = atoi((const char *)ptCommand->pcArgArray[ 4 ]);

                if( fEnable )
                {
                    if( SpiSlaveClaimBus( eSpiSlave ) == FALSE )
                    {
                        eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
                else
                {
                    if( SpiSlaveReleaseBus( eSpiSlave ) == FALSE )
                    {
                        eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                    }
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 1 ], (const CHAR *)"SLAVE" ) == TRUE )
    {
        if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"STAT" ) == TRUE )
        {
            SpiSlaveEnum        eSlave;

            Printf
            (
                ptCommand->eConsolePort, "%-10s %-8s\r\n",
                "Name",
                "Asserted"
            );

            for( eSlave = 0 ; eSlave < SPI_SLAVE_TOTAL ; eSlave++ )
            {
                Printf
                (
                    ptCommand->eConsolePort, "%-10s %-8d\r\n",
                    SpiSlaveGetName( eSlave ),
                    SpiSlaveIsChipSelectAsserted( eSlave )
                );
            }
        }
        // SPI SLAVE ASSERT <SLAVE> <0|1>
        else if( StringUtilsStringEqual( ptCommand->pcArgArray[ 2 ], (const CHAR *)"ASSERT" ) == TRUE )
        {
            if( ptCommand->bNumArgs == 5 )
            {
                BOOL            fAssert     = FALSE;
                SpiSlaveEnum    eSpiSlave   = SPI_SLAVE_INVALID;

                for( eSpiSlave = 0 ; eSpiSlave < SPI_SLAVE_TOTAL ; eSpiSlave++ )
                {
                    if( StringUtilsStringEqual( SpiSlaveGetName(eSpiSlave), ptCommand->pcArgArray[ 3 ] ) )
                    {
                        break;
                    }
                }

                fAssert = atoi((const char *)ptCommand->pcArgArray[ 4 ]);

                if( SpiSlaveChipSelectAssert( eSpiSlave, fAssert ) == FALSE )
                {
                    eResult = COMMAND_RESULT_ERROR_PROCESSING_FAIL;
                }
            }
            else
            {
                eResult = COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS;
            }
        }
        else
        {
            eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
        }
    }
    else
    {
        eResult = COMMAND_RESULT_ERROR_INVALID_COMMAND;
    }

    return eResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
