/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_FTP_OPER_H
#define MODEM_FTP_OPER_H

void                ModemFtpOperationInit                       ( void );
void                ModemFtpOperationStateMachine               ( void );

/////////////////////////////////////////////////////////////////////////////
// FTP PUT
void                ModemFtpOperationPutSetFile                 ( CHAR *pszFileName, UINT32 dwFileSize );
BOOL                ModemFtpOperationPutSetFileIsDone           ( void );
BOOL                ModemFtpOperationPutSetFileIsSucceed        ( void );
// * * * * * * * * * * * * * * * * * * * * * * * * * * * 
// FTP PUT SEND chunks of DATA
void                ModemFtpOperationPutSendBufferGetInfo       ( UINT8 **pbBuffer, UINT32 *pdwBufferSize );
void                ModemFtpOperationPutSendData                ( UINT16 wBytesRead );
BOOL                ModemFtpOperationPutSendDataIsDone          ( void );
BOOL                ModemFtpOperationPutSendDataIsSucceed       ( void );
// * * * * * * * * * * * * * * * * * * * * * * * * * * * 
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// FTP GET
void                ModemFtpOperationGetSetFile                 ( CHAR *pszFileName, UINT32 dwAllocMaxSize );
BOOL                ModemFtpOperationGetSetFileIsDone           ( void );
BOOL                ModemFtpOperationGetSetFileIsSucceed        ( void );
BOOL                ModemFtpOperationGetIsFileExceedAllocSize   ( void );
UINT32              ModemFtpOperationGetFileSize                ( void );
// * * * * * * * * * * * * * * * * * * * * * * * * * * * 
// FTP GET EXTRACT chunks of DATA
void                ModemFtpOperationGetReceiverBufferGetInfo   ( UINT8 **pbBuffer, UINT32 *pdwBufferSize, UINT32 *pdwBytesRead, BOOL *pfEofReached );
void                ModemFtpOperationGetReceiveData             ( void );
BOOL                ModemFtpOperationGetReceiveDataIsDone       ( void );
BOOL                ModemFtpOperationGetReceiveDataIsSucceed    ( void );
// * * * * * * * * * * * * * * * * * * * * * * * * * * * 
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// FTP DELETE
void                ModemFtpOperationDeleteFile                 ( CHAR *pszFileName );
BOOL                ModemFtpOperationDeleteFileIsDone           ( void );
BOOL                ModemFtpOperationDeleteFileIsSucceed        ( void );
// /////////////////////////////////////////////////////////////////////////////

#endif /* MODEM_FTP_OPER_H */ 