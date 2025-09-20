/** C Header ******************************************************************

*******************************************************************************/

#ifndef _MODEM_RESPONSE_H_
#define _MODEM_RESPONSE_H_

////////////////////////////////////////////////

// init functions
BOOL                        ModemResponseModuleInit                         ( void );
ModemDataType             * ModemResponseModemDataGetPtr                    ( void );

// functions for modem response processing
BOOL                        ModemResponseIsResponseListed                   ( ModemResponseHandlerType *ptResponseList, CHAR *pszResponseString, UINT32 *pdwResponseIndex );
ModemResponseHandlerType  * ModemResponseHandlerGetResponseListPtr          ( void );

// result functions


////////////////////////////////////////////////

#endif /* _MODEM_RESPONSE_LIST_HANDLER_H_ */ 
