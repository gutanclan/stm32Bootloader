/** C Header ******************************************************************

*******************************************************************************/

#ifndef _MODEM_H_
#define _MODEM_H_

////////////////////////////////////////////////
BOOL    ModemModuleInit                     ( void );

// functions to process incoming chars from modem
// if auto Update is disabled, manual update can be used to process incoming chars.
void    ModemRxProcessKeyAutoUpdateEnable   ( BOOL fAutoUpdateEnable );
void    ModemRxProcessKeyAutoUpdate         ( void );
void    ModemRxProcessKeyManualUpdate       ( CHAR cChar );
// to send chars to modem(semaphore protected)
BOOL    ModemTxPutBuffer                    ( void *pvBuffer, UINT16 wBufferSize );
INT32   ModemTxSendAtCommand                ( const char *format, ... );


// printing functions
void    ModemConsolePrintRxEnable           ( BOOL fPrintRxEnable );
BOOL    ModemConsolePrintRxIsEnabled        ( void );
void    ModemConsolePrintTxEnable           ( BOOL fPrintTxEnable );
BOOL    ModemConsolePrintTxIsEnabled        ( void );

void    ModemConsolePrintfEnable            ( BOOL fPrintfEnable );
BOOL    ModemConsolePrintfIsEnabled         ( void );
#define ModemConsolePrintf(...)             if( ModemConsolePrintfIsEnabled() == TRUE ) { ConsolePrintf( CONSOLE_PORT_USART, __VA_ARGS__ ); }

void    ModemConsolePrintDbgEnable          ( BOOL fPrintfEnable );
BOOL    ModemConsolePrintDbgIsEnabled       ( void );
#define ModemConsolePrintDbg(...)           if( ModemConsolePrintDbgIsEnabled() == TRUE ) { ConsoleDebugfln( CONSOLE_PORT_USART, __VA_ARGS__ ); }

void    ModemEventLogEnable                 ( BOOL fEventLogEnable );
BOOL    ModemEventLogIsEnabled              ( void );
INT32   ModemEventLog                       ( BOOL fIsError, const char *format, ... );

////////////////////////////////////////////////

#endif /* _MODEM__H_ */ 
