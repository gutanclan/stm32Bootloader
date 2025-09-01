/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_FTP_H
#define MODEM_FTP_H

typedef enum
{
    MODEM_FTP_OPERATION_TYPE_NONE = 0,
    MODEM_FTP_OPERATION_TYPE_PUT,
    MODEM_FTP_OPERATION_TYPE_GET,
    MODEM_FTP_OPERATION_TYPE_DELETE,
    
    MODEM_FTP_OPERATION_MAX,
}ModemFtpOperationTypeEnum;

typedef enum
{
    MODEM_FTP_CONN_ACTION_CLOSE = 0,
    MODEM_FTP_CONN_ACTION_OPEN,
    
    MODEM_FTP_CONN_ACTION_MAX,
}ModemFtpConnActionEnum;

void                ModemFtpInit                            ( void );
void                ModemFtpStateMachine                    ( void );

BOOL                ModemFtpConnectSetConfig                ( ModemFtpConnType *ptFtpConn );

void                ModemFtpConnectRun                      ( BOOL fConnectToServer, ModemFtpOperationTypeEnum eTransferType );
BOOL                ModemFtpConnectIsWaitingForCommand      ( void );
BOOL                ModemFtpConnectIsConnected              ( void );

ModemFtpOperationTypeEnum ModemFtpConnectAllowedOperation   ( void );

#endif /* MODEM_FTP_H */ 


