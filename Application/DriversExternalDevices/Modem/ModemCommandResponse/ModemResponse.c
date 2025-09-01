#include <stdio.h>          // Standard I/O library
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>         // For va_arg support
#include <__vfprintf.h>     // For printf-esque support
#include "Types.h"
#include "Rtc.h"
#include "../Utils/StringUtils.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../ModemOperations/ModemRegStat.h"
#include "../PCOM/Console.h"
#include "../PCOM/Usart.h"


static void    Response_Ok             ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Error          ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_NoCarrier      ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Connect        ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Busy           ( ModemResponseHandlerArgsType *ptResponse );

static void    Response_Plus_Cme       ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Plus_Cops      ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Plus_Cpin      ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Plus_Creg      ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Plus_Csq       ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_Plus_Pacsp1    ( ModemResponseHandlerArgsType *ptResponse );

static void    Response_POUND_Selint   ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Ccid     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Scfg     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Sgact    ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Moni     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Servinfo ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cclk     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Ceer     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Ceernet  ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Ss       ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Ccid     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cimi     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cgmi     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cgmm     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cgmr     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Cgsn     ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_V24cfg   ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_Snt      ( ModemResponseHandlerArgsType *ptResponse );

static void    Response_POUND_FtpClose ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpOpen  ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpType  ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpDele  ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpPut   ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpAppext( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpFSize ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpRest  ( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpGetPkt( ModemResponseHandlerArgsType *ptResponse );
static void    Response_POUND_FtpRecv  ( ModemResponseHandlerArgsType *ptResponse );

static const ModemResponseHandlerType    gtModemResponseList[] =
{        
    //{   NULL,           Response_General            },

    {   "OK",           Response_Ok                 },
    {   "ERROR",        Response_Error              },
    {   "NO CARRIER",   Response_NoCarrier          },
    {   "CONNECT",      Response_Connect            },
    {   "BUSY",         Response_Busy               },
    
    {   "+CME",         Response_Plus_Cme           },
    {   "+COPS:",       Response_Plus_Cops          },
    {   "+CPIN:",       Response_Plus_Cpin          },
    {   "+CREG:",       Response_Plus_Creg          },
    {   "+CGREG:",      Response_Plus_Creg          },
    {   "+CSQ:",        Response_Plus_Csq           },    
    {   "+PACSP1",      Response_Plus_Pacsp1        },    
    
    {   "#SELINT",      Response_POUND_Selint       },
    {   "#CCID",        Response_POUND_Ccid         },
    {   "#SCFG:",       Response_POUND_Scfg         },
    {   "#SGACT:",      Response_POUND_Sgact        },
    {   "#MONI:",       Response_POUND_Moni         },
    {   "#SERVINFO:",   Response_POUND_Servinfo     },
    {   "#CCLK:",       Response_POUND_Cclk         },
    {   "#CEER",        Response_POUND_Ceer         },
    {   "#CEERNET",     Response_POUND_Ceernet      },
    {   "#SS:",         Response_POUND_Ss           },   

    {   "#CCID:",       Response_POUND_Ccid         },
    {   "#CIMI:",       Response_POUND_Cimi         },    
    {   "#CGMI:",       Response_POUND_Cgmi         },
    {   "#CGMM:",       Response_POUND_Cgmm         },    
    {   "#CGMR:",       Response_POUND_Cgmr         },        
    {   "#CGSN:",       Response_POUND_Cgsn         },        
    {   "#V24CFG:",     Response_POUND_V24cfg       },    
    {   "#STN:",        Response_POUND_Snt          },        
    
    {   "#FTPCLOSE",    Response_POUND_FtpClose     },
    {   "#FTPOPEN",     Response_POUND_FtpOpen      },
    {   "#FTPTYPE",     Response_POUND_FtpType      },
    {   "#FTPDELE",     Response_POUND_FtpDele      },
    {   "#FTPAPPEXT",   Response_POUND_FtpAppext    },
    {   "#FTPPUT",      Response_POUND_FtpPut       },
    {   "#FTPFSIZE:",   Response_POUND_FtpFSize     },
    {   "#FTPRES",      Response_POUND_FtpRest      },
    {   "#FTPGETPKT:",  Response_POUND_FtpGetPkt    },
    {   "#FTPRECV:",    Response_POUND_FtpRecv      },
    
    {   NULL,           NULL                        },
};

static ModemDataType gtModemData;

/////////////////////////////////// BODY OF THE LIBRARY /////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemResponseModuleInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemResponseIsResponseListed( ModemResponseHandlerType *ptResponseList, CHAR *pszResponseString, UINT32 *pdwResponseIndex )
{
    BOOL fSuccess = FALSE;

    if
    (
        ( NULL != ptResponseList ) &&
        ( NULL != pszResponseString ) &&
        ( NULL != pdwResponseIndex )
    )
    {
        (*pdwResponseIndex) = 0;

        // Search through the command dictionary (skip if no token found)
		while( ptResponseList[(*pdwResponseIndex)].pfnResponseHandler != NULL )
		{
			if( StringUtilsStringEqual( (const CHAR *)ptResponseList[(*pdwResponseIndex)].pszCommandString, (const CHAR *)pszResponseString ) == TRUE )
			{
				fSuccess = TRUE;
				break;
			}

			// Advance to next dictionary entry
			(*pdwResponseIndex)++;
		}// end while loop
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ModemResponseHandlerType * ModemResponseHandlerGetResponseListPtr ( void )
{
    return (ModemResponseHandlerType *) (&gtModemResponseList[0]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ModemDataType * ModemResponseModemDataGetPtr( void )
{    
    return &gtModemData;    
}

//################################################################################################

///////////////////////////////////////////////////////////////////////////
void Response_Ok( ModemResponseHandlerArgsType *ptResponse )
{   
    // set flags only if waiting for response, otherwise don't care
    if( gtModemData.tCommandResponse.fIsProcessingCommandResponse )
    {        
        if( gtModemData.tCommandResponse.tResponse.tOk.fIsOkExpected ) 
        {
            gtModemData.tCommandResponse.tResponse.tOk.fIsOkReceived = TRUE;            
        }
    }
}

void Response_Error( ModemResponseHandlerArgsType *ptResponse )
{    
    // set flags only if waiting for response, otherwise don't care    
    if( gtModemData.tCommandResponse.fIsProcessingCommandResponse )
    {
        gtModemData.tCommandResponse.tResponse.fIsError             = TRUE;
    }
}

void Response_NoCarrier( ModemResponseHandlerArgsType *ptResponse )
{    
    
}

void Response_Connect( ModemResponseHandlerArgsType *ptResponse )
{    
    
}

void Response_Busy( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
/////////////////////////////////////////////////////////////////////

void Response_Plus_Cme( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs >= 2 )
    {        
        if( StringUtilsStringEqual( "ERROR:", ptResponse->pcArgArray[1] ) == TRUE )
        {
            if( gtModemData.tCommandResponse.fIsProcessingCommandResponse )
            {
                gtModemData.tCommandResponse.tResponse.fIsError             = TRUE;
            }
        }
    }
}

void Response_Plus_Cops( ModemResponseHandlerArgsType *ptResponse )
{    
    
}

void Response_Plus_Cpin( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        if( StringUtilsStringEqual( "READY", ptResponse->pcArgArray[1] ) == TRUE )
        {
            gtModemData.tSimCard.fIsSimReady = TRUE;
        }
        else
        {
            gtModemData.tSimCard.fIsSimReady = FALSE;
        }
    }
}

// WARNING!! this function handles two commands. creg and cgreg!!
void Response_Plus_Creg( ModemResponseHandlerArgsType *ptResponse )
{
    // make a copy of arguments into general buffer to process the result
    if( ptResponse->bNumArgs == 2 )
    {
        ///////////////////////////////////////////
        // process response data
        ///////////////////////////////////////////        
        CHAR   *pcResponseTokens[ 2 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)ptResponse->pcArgArray[1],
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );
        
        gtModemData.tNetwork.bCregStat = atoi( pcResponseTokens[1] );

        ModemConsolePrintf( "%s\r\n", ModemRegStatGetStatusString( gtModemData.tNetwork.bCregStat ) );        
    }
}

void Response_Plus_Csq( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        ///////////////////////////////////////////
        // process response data
        ///////////////////////////////////////////
        UINT8   bRssi   = 0;
        UINT8   bBer    = 0;
        SINGLE  sgPreRssi;
        CHAR   *pcResponseTokens[ 2 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)(ptResponse->pcArgArray[1]),
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );

        bRssi   = atoi( pcResponseTokens[0] );
        bBer    = atoi( pcResponseTokens[1] );

        ///////////////////////////////////////////////////////////////////////
        // RSSI processing
        ///////////////////////////////////////////////////////////////////////
        if( bRssi == 0 )
        {
            gtModemData.tSignal.idwRssi_dBm = -113;
        }
        else
        if( bRssi == 1 )
        {
            gtModemData.tSignal.idwRssi_dBm = -111;
        }
        else
        if
        ( 
            ( bRssi >= 2 ) && 
            ( bRssi <= 30 )
        )
        {
            // this formula converts from 2 to 30 into dBm -109 to -53
            sgPreRssi = 30 - bRssi; 
            sgPreRssi = -53-(sgPreRssi*2); 
            gtModemData.tSignal.idwRssi_dBm = sgPreRssi;        
        }
        else
        if( bRssi == 31 )
        {
            gtModemData.tSignal.idwRssi_dBm = -51;
        }
        else
        if( bRssi == 99 )
        {
            gtModemData.tSignal.idwRssi_dBm = 99;
        }
        else
        {
            gtModemData.tSignal.idwRssi_dBm = 99;
        }

        ///////////////////////////////////////////////////////////////////////
        // BER processing
        ///////////////////////////////////////////////////////////////////////
        switch( bBer )
        {
            case 0: /* Less than 0.2% */
                gtModemData.tSignal.sgBer_Percent = 0.0;
                break;
            case 1: /*  0.2% to 0.4% */
                gtModemData.tSignal.sgBer_Percent = 0.4;
                break;
            case 2: /*  0.4% to 0.8% */
                gtModemData.tSignal.sgBer_Percent = 0.8;
                break;
            case 3: /*  0.8% to 1.6% */
                gtModemData.tSignal.sgBer_Percent = 1.6;
                break;
            case 4: /*  1.6% to 3.2% */
                gtModemData.tSignal.sgBer_Percent = 3.2;
                break;
            case 5: /*  3.2% to 6.4% */
                gtModemData.tSignal.sgBer_Percent = 6.4;
                break;
            case 6: /*  6.4% to 12.8% */
                gtModemData.tSignal.sgBer_Percent = 12.8;
                break;
            case 7: /*  more than 12.8% */
                gtModemData.tSignal.sgBer_Percent = 15.0;
                break;
            case 99: /*  Not known or not detectable */
            default:
                gtModemData.tSignal.sgBer_Percent = 99.0;
                break;
        }

        ModemConsolePrintf( "Rssi=%d(dBm) Ber=%6.1f(Percent)\r\n", gtModemData.tSignal.idwRssi_dBm,gtModemData.tSignal.sgBer_Percent );
        
        // log msg
        //ModemEventLog(FALSE,"Rssi=%d(dBm) Ber=%6.1f(Percent)", gtModemData.tSignal.idwRssi_dBm,gtModemData.tSignal.sgBer_Percent );
    }
}

void Response_Plus_Pacsp1( ModemResponseHandlerArgsType *ptResponse )
{

}
///////////////////////////////////////////////////////////////////
void Response_POUND_Selint( ModemResponseHandlerArgsType *ptResponse )
{    
    
}

void Response_POUND_Scfg( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_Sgact( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        // check if response have commands or dots
        BOOL    fIsResponseContainComma = FALSE;
        UINT16  wStrLen = strlen(ptResponse->pcArgArray[1]);
        
        for( UINT16 w = 0 ; w < wStrLen ; w++ )
        {   
            if( ptResponse->pcArgArray[1][w] == ',' )
            {
                fIsResponseContainComma = TRUE;
                break;
            }
            if( ptResponse->pcArgArray[1][w] == '.' )
            {
                fIsResponseContainComma = FALSE;
                break;
            }
        }

        if( fIsResponseContainComma == FALSE )         
        // response is an IP
        {
            // update modem data structure
            strncpy
            ( 
                &gtModemData.tNetwork.szIp[0], 
                ptResponse->pcArgArray[1], 
                sizeof(gtModemData.tNetwork.szIp)
            );
        }
        else    
        // response is context stat
        {               
            UINT8 bContext = 0;
            UINT8 bStat = 0;            
            CHAR   *pcResponseTokens[ 2 ];

            StringUtilsStringToTokenArray
            (
                (CHAR *)(ptResponse->pcArgArray[1]),
                strlen(ptResponse->pcArgArray[1]),
                ',',
                FALSE,
                0,
                &pcResponseTokens[0],
                (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
            );

            bContext    = atoi( pcResponseTokens[0] );
            bStat       = atoi( pcResponseTokens[1] );
            
            if( gtModemData.tNetwork.bPdpCntxtId == bContext )
            {
                gtModemData.tNetwork.bSocketCntxtStat = bStat; 
            }
            else
            {
                ModemConsolePrintf( "Socket Context not matching!!\r\n" );
            }

            ModemConsolePrintf( "Socket context(%d) Stat:", bContext );
            
            switch( bStat )
            {
                case 0:
                ModemConsolePrintf( "Deactivated\r\n" );                
                break;

                case 1:
                ModemConsolePrintf( "Activated\r\n" );                
                break;

                default:                
                    break;
            }
        }
    }
}
void Response_POUND_Moni( ModemResponseHandlerArgsType *ptResponse )
{    
    ///////////////////////////////////////////
    // process response data
    ///////////////////////////////////////////    
    // since moni is separated by ' 'space, previous tokenizer will SPLIT THE MAIN RESPONSE INTO SERVERAL SUBSTRINGS
    // (UMTS network)
    // #MONI: <cc> <nc> PSC:<psc> RSCP:<rscp> LAC:<lac> Id:<id> EcIo:<ecio> UARFCN:<uarfcn> PWR:<dBm>dBm DRX:<drx>SCR:<scr>
    // at least there should be 12 tokens (interested to extract only Id)
    if( ptResponse->bNumArgs >= 2 )
    {    
        // iterate throught the tokens until find Id
        for( UINT8 c = 1 ; c < ptResponse->bNumArgs ; c++ )
        {
            if( strncmp( ptResponse->pcArgArray[c], "Id:", 3 ) == 0 )
            {
                CHAR   *pcResponseTokens[ 2 ];

                StringUtilsStringToTokenArray
                (
                    (CHAR *)ptResponse->pcArgArray[c],
                    strlen(ptResponse->pcArgArray[c]),
                    ':',
                    FALSE,
                    0,
                    &pcResponseTokens[0],
                    (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
                );

                if( pcResponseTokens[1] != NULL )
                {
                    strncpy
                    ( 
                        &gtModemData.tCellInfo.szCellId[0], 
                        pcResponseTokens[1],
                        (sizeof(gtModemData.tCellInfo.szCellId)-1)
                    );
                }
                
                // terminate loop
                break;
            }
        }// end for loop
    }
}
void Response_POUND_Servinfo( ModemResponseHandlerArgsType *ptResponse )
{    
    ///////////////////////////////////////////
    // process response data
    ///////////////////////////////////////////    
    if( ptResponse->bNumArgs == 2 )
    {           
        // (UMTS network)
        // #SERVINFO: <UARFCN>, <dBM>, <NetNameAsc>,<NetCode>,<PSC>,<LAC>,<DRX>,<SD>,<RSCP>, <NOM>,<RAC>
        // token offset   0       1        2            3       4      5    6 
        CHAR   *pcResponseTokens[ 7 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)ptResponse->pcArgArray[1],
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );

        // get token offset 2: <NetNameAsc> - operator name, quoted string type
        if( pcResponseTokens[2] != NULL )
        {
            // remove quotes '"'            
            CHAR *pszDelimiterStrippedString = NULL;
            // if string contains open and close delimiter, remove them and continue processing data
            // WARNING!! StringUtilsStringParamStripDelim() modifies the string
            if( StringUtilsStringParamStripDelim( (CHAR *)pcResponseTokens[2] , '"', &pszDelimiterStrippedString ) )
            {
                strncpy
                ( 
                    &gtModemData.tCellInfo.szOperatorName[0], 
                    pszDelimiterStrippedString,
                    (sizeof(gtModemData.tCellInfo.szOperatorName)-1)
                );
            }            
        }

        // get token offset 5: <LAC> - Localization Area Code
        if( pcResponseTokens[5] != NULL )
        {
            strncpy
            ( 
                &gtModemData.tCellInfo.szLac[0], 
                pcResponseTokens[5],
                (sizeof(gtModemData.tCellInfo.szLac)-1)
            );
        }
    }
}
void Response_POUND_Cclk( ModemResponseHandlerArgsType *ptResponse )
{    
    // string format "yy/MM/dd,hh:mm:ss±zz,d"
    //               "14/01/18,23:15:56+89,1"
    //               "14/01/18,23:15:56-28,1"
    if( ptResponse->bNumArgs == 2 )
    {
        CHAR *pszDelimiterStrippedString = NULL;
        // if string contains open and close delimiter, remove them and continue processing data
        // WARNING!! StringUtilsStringParamStripDelim() modifies the string
        if( StringUtilsStringParamStripDelim( (CHAR *)ptResponse->pcArgArray[1] , '"', &pszDelimiterStrippedString ) )
        {
            ModemEventLog(FALSE,"net DateTime extracted [%s]", pszDelimiterStrippedString );

            // process string
            CHAR   *pcResponseTokens[ 3 ];
            CHAR   *pcDateTokens[ 3 ];
            CHAR   *pcTimeTokens[ 3 ];
            RtcDateTimeStruct tSysDateTime;
            RtcDateTimeStruct tNetworkDateTime;
            INT16   iwTimeZone              = 0;
            UINT8   bDayLightSavingHours    = 0;
            UINT32  dwDeltaTimeSecs         = 0;
            UINT32  dwEpochNetworkSecs      = 0;
            UINT32  dwEpochSystemSecs       = 0;
            UINT16  wStrLen                 = 0;

            // split by comma            
            /*
            expected result: 
            token 1[yy/MM/dd]
            token 2[hh:mm:ss±zz]
            token 3[d]
            */

            StringUtilsStringToTokenArray
            (
                pszDelimiterStrippedString,
                strlen(pszDelimiterStrippedString),
                ',',
                FALSE,
                0,
                &pcResponseTokens[0],
                (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
            );            
            
            // token 2[hh:mm:ss±zz], extract time zone and terminate string ('\0') at index where char='+-'
            wStrLen = strlen( pcResponseTokens[1] );            
            for( UINT8 c = 0 ; c < wStrLen ; c++ )
            {
                if
                (
                    ( pcResponseTokens[1][c] == '+' ) ||
                    ( pcResponseTokens[1][c] == '-' )
                )
                {
                    iwTimeZone = atoi( &pcResponseTokens[1][c] );
                    pcResponseTokens[1][c] = '\0';
                    break;
                }
            }

            bDayLightSavingHours = atoi( &pcResponseTokens[2][0] );

            // split date
            StringUtilsStringToTokenArray
            (
                &pcResponseTokens[0][0],
                strlen(pcResponseTokens[0]),
                '/',
                FALSE,
                0,
                &pcDateTokens[0],
                (sizeof(pcDateTokens)/sizeof(pcDateTokens[0]))
            );
            // split time
            StringUtilsStringToTokenArray
            (
                &pcResponseTokens[1][0],
                strlen(pcResponseTokens[1]),
                ':',
                FALSE,
                0,
                &pcTimeTokens[0],
                (sizeof(pcTimeTokens)/sizeof(pcTimeTokens[0]))
            );
            // finish converting strings into numeric vals
            tNetworkDateTime.bYear  = atoi( pcDateTokens[0] ); //0-99 
            tNetworkDateTime.eMonth = atoi( pcDateTokens[1] ); //1-12 
            tNetworkDateTime.bDate  = atoi( pcDateTokens[2] ); //1-31 

            tNetworkDateTime.bHour  = atoi( pcTimeTokens[0] ); //0-99 
            tNetworkDateTime.bMinute= atoi( pcTimeTokens[1] ); //1-12 
            tNetworkDateTime.bSecond= atoi( pcTimeTokens[2] ); //1-31 

            RtcDateTimeGet( &tSysDateTime );

            // at this point the two times( system and network) have been extracted
            // sometime this default value is returned by modem = 00/01/01,00:00:XX-24,1        
            if( tNetworkDateTime.bYear != 0 )
            {
                // perform arithmetic operations over EPOCH seconds
                dwEpochNetworkSecs  =  RtcDateTimeToEpochSeconds( &tNetworkDateTime );
                dwEpochSystemSecs   =  RtcDateTimeToEpochSeconds( &tSysDateTime );

                // NETWORK TIME CORRECTION
                // token 3[d] = number of hours added to the local TZ because 
                // of Daylight Saving Time (summertime) adjustment; range is 0-2.
                dwEpochNetworkSecs = dwEpochNetworkSecs - ( bDayLightSavingHours * 60 * 60 );

                
                // calculate the difference
                if( dwEpochSystemSecs >= dwEpochNetworkSecs )
                {
                    dwDeltaTimeSecs = dwEpochSystemSecs - dwEpochNetworkSecs;
                }
                else
                {
                    dwDeltaTimeSecs = dwEpochNetworkSecs - dwEpochSystemSecs;
                }
                
                if( dwDeltaTimeSecs > gtModemData.tSysClock.bDeltaSecondsConfigured )
                {
                    // update sys time
                    // covert EPOCH back to standard time format.
                    if( RtcEpochSecondsToDateTime( dwEpochNetworkSecs, &tSysDateTime ) )
                    {
                        if( RtcDateTimeSet( &tSysDateTime ) )
                        {            
                            gtModemData.tSysClock.fIsTimeUpdated = TRUE;
                            ModemConsolePrintf( "sys Date time Updated\r\n" );
                            ModemEventLog(FALSE,"sys Date time Updated" );
                        }
                    }                    
                }
            }
            else
            {
                ModemConsolePrintf( "network Date time invalid\r\n" );
            }
        }
    }
}
void Response_POUND_Ceer( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_Ceernet( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_Ss( ModemResponseHandlerArgsType *ptResponse )
{    
    // process response    
    if( ptResponse->bNumArgs == 2 )
    {
        // process result
        CHAR   *pcResponseTokens[ 2 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)ptResponse->pcArgArray[1],
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );
        
        gtModemData.tNetwork.bSocketStat = atoi( pcResponseTokens[1] );

        ModemConsolePrintf( "Socket Stat:" );
        switch( gtModemData.tNetwork.bSocketStat )
        {
            case 0:
                ModemConsolePrintf( "Closed\r\n" );                
                break;
            case 1:
                ModemConsolePrintf( "Active data tranfer\r\n" );                
                break;
            case 2:
                ModemConsolePrintf( "Suspended\r\n" );                
                break;
            case 3:
                ModemConsolePrintf( "Suspended with pending data\r\n" );                
                break;                                            
            case 4:
                ModemConsolePrintf( "Listening\r\n" );                
                break;                                            
            case 5:
                ModemConsolePrintf( "Incomming connection\r\n" ); //res dns                
                break;
            
            default:
                ModemConsolePrintf( "Unknown\r\n" );                
                break;
        }
    }
}

void Response_POUND_Ccid( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem sim
        gtModemData.tSimCard.fIsSimReady = TRUE;
    
        strncpy
        ( 
            &gtModemData.tSimCard.szIccId[0], 
            ptResponse->pcArgArray[1], 
            (sizeof(gtModemData.tSimCard.szIccId)-1)
        );
        
        strncpy
        ( 
            &gtModemData.tSimCard.szLastIccId[0], 
            ptResponse->pcArgArray[1], 
            (sizeof(gtModemData.tSimCard.szLastIccId)-1)
        );
    }
}

void Response_POUND_Cimi( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        strncpy
        ( 
            &gtModemData.tSimCard.szCimei[0], 
            ptResponse->pcArgArray[1], 
            sizeof(gtModemData.tSimCard.szCimei)
        );
    }
}

void Response_POUND_Cgmi( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        strncpy
        ( 
            &gtModemData.tModemInfo.szManufacturerName[0], 
            ptResponse->pcArgArray[1], 
            sizeof(gtModemData.tModemInfo.szManufacturerName)
        );
    }
}

void Response_POUND_Cgmm( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        strncpy
        ( 
            &gtModemData.tModemInfo.szModem[0], 
            ptResponse->pcArgArray[1], 
            sizeof(gtModemData.tModemInfo.szModem)
        );
    }
}

void Response_POUND_Cgmr( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        strncpy
        ( 
            &gtModemData.tModemInfo.szFirmwareVer[0], 
            ptResponse->pcArgArray[1], 
            sizeof(gtModemData.tModemInfo.szFirmwareVer)
        );
    }
}

void Response_POUND_Cgsn( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        // update modem data structure
        strncpy
        ( 
            &gtModemData.tModemInfo.szImei[0], 
            ptResponse->pcArgArray[1], 
            sizeof(gtModemData.tModemInfo.szImei)
        );
    }
}

void Response_POUND_V24cfg( ModemResponseHandlerArgsType *ptResponse )
{
    if( ptResponse->bNumArgs == 2 )
    {
        ///////////////////////////////////////////
        // process response data
        ///////////////////////////////////////////
        UINT8   bPin    = 0;
        UINT8   bStat   = 0;
        CHAR   *pcResponseTokens[ 2 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)(ptResponse->pcArgArray[1]),
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );

        bPin  = atoi( pcResponseTokens[0] );
        bStat = atoi( pcResponseTokens[1] );
        ModemConsolePrintf( "Pin%d = %s\r\n", (bPin+1), (bStat==0 ? "LOW" : "HIGH") );
    }
}

void Response_POUND_Snt( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        UINT16 wSimAlert = atoi( ptResponse->pcArgArray[1] );

        switch( wSimAlert )
        {
            case 237:
                ModemConsolePrintf( "SIM CARD REMOVED\r\n" );
            
                gtModemData.tSimCard.fIsSimReady = FALSE;
                gtModemData.tSimCard.szIccId[0] = '\0';
                gtModemData.tSimCard.szCimei[0] = '\0';
            
                break;

            default:
                break;
        }
    }
}
///////////////////////////////////////////////////////////////
void Response_POUND_FtpClose( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpOpen( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpType( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpDele( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpAppext( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpPut( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpFSize( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        gtModemData.tFtpGet.dwFileSize = atoi(ptResponse->pcArgArray[1]);
    }
}
void Response_POUND_FtpRest( ModemResponseHandlerArgsType *ptResponse )
{    
    
}
void Response_POUND_FtpGetPkt( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        // filename,viewMode,EOF found  
        CHAR   *pcResponseTokens[ 3 ];

        StringUtilsStringToTokenArray
        (
            (CHAR *)(ptResponse->pcArgArray[1]),
            strlen(ptResponse->pcArgArray[1]),
            ',',
            FALSE,
            0,
            &pcResponseTokens[0],
            (sizeof(pcResponseTokens)/sizeof(pcResponseTokens[0]))
        );

        gtModemData.tFtpGet.fEofFound  = atoi( pcResponseTokens[2] );    

        //ModemConsolePrintDbg( "Eof%d\r\n", gtModemData.tFtpGet.fEofFound );
    }
}
void Response_POUND_FtpRecv( ModemResponseHandlerArgsType *ptResponse )
{    
    if( ptResponse->bNumArgs == 2 )
    {
        gtModemData.tFtpGet.dwBytesToReceive = atoi(ptResponse->pcArgArray[1]);
    }
}

