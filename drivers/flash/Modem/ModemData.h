//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		Command.h
//!	\brief		Command Library for the system
//!
//!	\author		Joel Minski [jminski@puracom.com], Puracom Inc.
//! \author     Craig Stickel [cstickel@puracom.com], Puracom Inc.
//! \date		March 5, 2010
//!
//! \details	Provides a command library for the system
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MODEM_DATA_H_
#define _MODEM_DATA_H_

#include "Types.h"
#include "..\Utils\StateMachine.h"
#include "..\PCOM\Console.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// for all strings add +1 to include the '\0'
#define MODEM_DATA_STRING_LEN_MANUFACTURER_NAME	(15)
#define	MODEM_DATA_STRING_LEN_MODEM				(15)
#define	MODEM_DATA_STRING_LEN_FIRMWARE_VER		(15)
#define	MODEM_DATA_STRING_LEN_IMEI				(15)
#define	MODEM_DATA_STRING_LEN_CGSN				(15)
#define	MODEM_DATA_STRING_LEN_IMSI				(15)

#define	MODEM_DATA_STRING_LEN_APN				(31)

#define	MODEM_DATA_STRING_LEN_FTP_ADDRESS		(31)
#define	MODEM_DATA_STRING_LEN_FTP_USER_NAME		(31)
#define	MODEM_DATA_STRING_LEN_FTP_PASSWORD		(31)

#define	MODEM_DATA_STRING_LEN_SIM_ICCID			(31)
#define	MODEM_DATA_STRING_LEN_SIM_CIMI          (15)

#define MODEM_DATA_STRING_LEN_ERROR_MESSAGE     (40)
//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global variables
//////////////////////////////////////////////////////////////////////////////////////////////////
	

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

typedef enum
{
    MODEM_CONNECT_APN_ADDRESS_INVALID     = 0,
    MODEM_CONNECT_APN_ADDRESS_BELL_4G,
    MODEM_CONNECT_APN_ADDRESS_BELL_M2M,    
    MODEM_CONNECT_APN_ADDRESS_JASPER,
    MODEM_CONNECT_APN_ADDRESS_KORE,
    MODEM_CONNECT_APN_ADDRESS_ROGERS,
    MODEM_CONNECT_APN_ADDRESS_SASKTEL,    
    MODEM_CONNECT_APN_ADDRESS_TELUS_DEFAULT,
    MODEM_CONNECT_APN_ADDRESS_TELUS_ALTERNATE,    

    MODEM_CONNECT_APN_ADDRESS_MAX
}ModemConnectApnEnum;

typedef struct
{
    UINT8   bPdpCntxtId;
    CHAR    szPdpProtocol       [ 4 + 1 ];
    CHAR	szApn				[ 31 + 1 ];
    CHAR	szPdpAddress        [ 25 + 1 ];
    UINT8   bPdpDataCompression;
    UINT8   bPdpHeaderCompression;
}ModemConnectPdpConfigType;

typedef struct
{
    UINT8   bSocketConnId;
    UINT8   bPdpCntxtId;

    /* packet size to be used by the TCP/UDP/IP stack for data sending.
    0 - select automatically default value(300).
    1..1500 - packet size in bytes.*/
    UINT16  wDataProtocolPacketSize;
     
    /*exchange timeout (or socket inactivity timeout); if there’s no data
    exchange within this timeout period the connection is closed.
    0 - no timeout
    1..65535 - timeout value in seconds (default 90 s.)*/
    UINT16  wDataExchangeTimeOut_Sec; 
    
    /*connection timeout; if we can’t establish a connection to the remote
    within this timeout period, an error is raised.
    10..1200 - timeout value in hundreds of milliseconds (default 600)*/
    UINT16  wConnTimeOut_x100_mSec; 

    /*
    data sending timeout; after this period data are sent also if they’re less
    than max packet size.
    0 - no timeout
    1..255 - timeout value in hundreds of milliseconds (default 50)*/
    UINT16  wDataSendTimeOut_x100_mSec; 
}ModemConnectSocketConfigType;

typedef struct
{
    BOOL                                fIsPdpSet;
    ModemConnectPdpConfigType           tConfig;
}ModemConnectPdpConfigStatType;

typedef struct
{
    BOOL                                fIsSocketSet;
    ModemConnectSocketConfigType        tConfig;
}ModemConnectSocketConfigStatType;

typedef struct
{       
    ModemConnectPdpConfigStatType       tPdpConfig;
    ModemConnectSocketConfigStatType    tSocketConnConfig;
}ModemConnectStatType;

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

typedef struct
{
    UINT32  dwPort;
    CHAR    szAddress           [ 31 + 1 ];
    CHAR    szUserName          [ 31 + 1 ];
    CHAR    szPassword          [ 31 + 1 ];
}ModemFtpConnType;

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

typedef enum
{
    MODEM_COMMAND_SEMAPHORE_FREE        = 0,
    MODEM_COMMAND_SEMAPHORE_RESERVED,

    MODEM_COMMAND_SEMAPHORE_MAX
}ModemCommandSemaphoreEnum;

typedef struct
{
	CHAR	szManufacturerName	[ MODEM_DATA_STRING_LEN_MANUFACTURER_NAME + 1 ];
	CHAR	szModem				[ MODEM_DATA_STRING_LEN_MODEM + 1 ];
	CHAR	szFirmwareVer		[ MODEM_DATA_STRING_LEN_FIRMWARE_VER + 1 ];
	CHAR	szImei				[ MODEM_DATA_STRING_LEN_IMEI + 1 ];
	CHAR	szCgsn				[ MODEM_DATA_STRING_LEN_CGSN + 1 ];
	CHAR	szImsi				[ MODEM_DATA_STRING_LEN_IMSI + 1 ];
}ModemDataModemInfoType;

typedef struct
{	
    // socket identification credentials
    UINT8   bSocketConnId;
    UINT8   bPdpCntxtId;
    // socket stat
    UINT8   bSocketStat;    
    UINT8   bSocketCntxtStat;
    CHAR	szIp				[ 25 + 1 ];    

    UINT8   bCregStat;
}ModemDataNetworkType;

typedef struct
{    
    BOOL    fIsInfoReady;
    CHAR	szOperatorName      [ 25 + 1 ];
    CHAR	szCellId            [ 25 + 1 ];    
    CHAR	szLac               [ 25 + 1 ];
}ModemDataCellInfoType;

typedef struct
{    
    BOOL 	fIsTimeUpdated;
	UINT8 	bDeltaSecondsConfigured;
}ModemDataSysClockType;

typedef struct
{    
    SINGLE	sgBer_Percent;
	INT32	idwRssi_dBm;
}ModemDataSignalInfoType;

typedef struct
{    
	UINT32	dwFileSize;    
    UINT32	dwBytesToReceive;
    BOOL	fEofFound;
}ModemDataFtpGetType;

typedef struct
{
	CHAR	szAddress			[ MODEM_DATA_STRING_LEN_FTP_ADDRESS + 1 ];
	UINT32	dwPort;	
	CHAR	szUserName			[ MODEM_DATA_STRING_LEN_FTP_USER_NAME + 1 ];
	CHAR	szPassword			[ MODEM_DATA_STRING_LEN_FTP_PASSWORD + 1 ];
	UINT32  dwFtpResponseTimeOut_mSec;
}ModemDataFtpType;

typedef struct
{
    BOOL    fIsSimReady;
	CHAR	szCimei				[ MODEM_DATA_STRING_LEN_SIM_CIMI + 1 ];	
    CHAR	szIccId				[ MODEM_DATA_STRING_LEN_SIM_ICCID + 1 ];
    CHAR	szLastIccId         [ MODEM_DATA_STRING_LEN_SIM_ICCID + 1 ];
}ModemDataSimCardType;

typedef struct
{
    BOOL    fIsOkExpected;
    BOOL    fIsOkReceived;    
}ModemDataResponseOkType;

typedef struct
{
    BOOL    fIsKeyWordExpected;
    BOOL    fIsKeyWordReceived;  
    UINT8   dwKeyWordExpectedCounter;
    UINT8   dwKeyWordReceivedCounter;
    CHAR    cKeywordExpectedBuffer[ 50 ];
}ModemDataResponseKeyWordType;

typedef struct
{
    BOOL                            fIsCompleteResponseReceived;
    BOOL                            fIsError;
    ModemDataResponseOkType         tOk;
    ModemDataResponseKeyWordType    tKeyWord;
}ModemDataResponseType;

typedef struct
{
    BOOL                            fIsProcessingCommandResponse;
    ModemDataResponseType           tResponse;
}ModemDataResponseProcessingType;

// used to hold the data result from/to commands
typedef struct
{
    // used to process command response from modem
    ModemDataResponseProcessingType tCommandResponse;
    // initial configurations
	ModemConnectStatType            tModemConnStat;
    ModemFtpConnType                tFtpServerConn;

    // data gathered from commands
	ModemDataModemInfoType	tModemInfo;
	ModemDataNetworkType	tNetwork;
    ModemDataSysClockType   tSysClock;
    ModemDataCellInfoType   tCellInfo;
    ModemDataSignalInfoType tSignal;
	ModemDataFtpType		tFtp;
    ModemDataFtpGetType     tFtpGet;
	ModemDataSimCardType    tSimCard;
}ModemDataType;

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// for modem state machine
typedef struct
{
    StateMachineType            tState;
}ModemStateMachineType;







///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// for modem command response handler

typedef struct
{
	ConsolePortEnum		eConsolePort;
	UINT8				bNumArgs;
	const CHAR			**pcArgArray;    
}ModemResponseHandlerArgsType;

typedef struct
{
	//! Name of the command
	const char*	pszCommandString;

	//! Pointer to the handler routine for this command
	void (*pfnResponseHandler)( ModemResponseHandlerArgsType *ptResponseArgs );
}ModemResponseHandlerType;

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _MODEM_DATA_H_

