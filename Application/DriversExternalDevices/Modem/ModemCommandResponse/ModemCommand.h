/** C Header ******************************************************************

*******************************************************************************/

#ifndef _MODEM_COMMAND_H_
#define _MODEM_COMMAND_H_

BOOL    ModemCommandModuleInit                          ( void );

// functions used for response processing
void    ModemCommandUpdateResponseReceived              ( void );
void    ModemCommandProcessorCountKeywordMatch          ( CHAR *pszReceivedKeyWord );

// for public use functions to send commands and wait for response from modem. 
BOOL    ModemCommandProcessorReserve                    ( ModemCommandSemaphoreEnum *peSemaphoreResult );
BOOL    ModemCommandProcessorRelease                    ( ModemCommandSemaphoreEnum *peSemaphoreResult );
void    ModemCommandProcessorResetResponse              ( void );
BOOL    ModemCommandProcessorSetExpectedResponse        ( BOOL fIsResponseKeywordExpected, CHAR *pcExpectedResponseKeyword, UINT8 bExpectedResponseKeywordAmount, BOOL fIsOkExpected );
#define ModemCommandProcessorSendAtCommand(...)         ModemTxSendAtCommand(__VA_ARGS__);
BOOL    ModemCommandProcessorIsResponseComplete         ( void );   
BOOL    ModemCommandProcessorIsError                    ( void );
BOOL    ModemCommandIsProcessorBusy                     ( void );   

////////////////////////////////////////////////

#endif /* _MODEM_COMMAND_H_ */ 
