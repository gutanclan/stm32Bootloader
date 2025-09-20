/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_CONNECTION_ADVANCE_H
#define MODEM_CONNECTION_ADVANCE_H

void                ModemConnAdvInit                        ( void );
void                ModemConnAdvStateMachine                ( void );

void                ModemConnAdvEnable                      ( BOOL fAutoEnable );
BOOL                ModemConnAdvIsEnabled                   ( void );

// set time out for the whole transfer
void                ModemConnAdvConnectSetTimeOut           ( UINT32 dwConnectTimeOut_mSec );
UINT32              ModemConnAdvConnectGetTimeOut           ( void );

// functions to power on off the modem
void                ModemConnAdvConnectRun                  ( BOOL fModemEnable );
BOOL                ModemConnAdvConnectIsWaitingForCommand  ( void );
BOOL                ModemConnAdvConnectIsConnected          ( void );

#endif /* MODEM_CONNECTION_ADVANCE_H */ 
