/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_CONNECT_H
#define MODEM_CONNECT_H

#include "../ModemData.h"

void                ModemConnectInit                  ( void );
void                ModemConnectStateMachine          ( void );

// operation
// NOTE: Pdp and Socket MUST be SET/CONFIGURED before attempting the first connection
void                ModemConnectRun                   ( BOOL fConnectToNetwork, UINT32 dwAttemptConnectionTimeOut_mSec );
BOOL                ModemConnectIsWaitingForCommand   ( void );
BOOL                ModemConnectIsConnected           ( void );

// connection configuration
void                ModemConnectConfigPdp             ( ModemConnectPdpConfigType *ptPdpConfig );
void                ModemConnectConfigSocket          ( ModemConnectSocketConfigType *ptSocketConnConfig );

ModemDataNetworkType  * ModemConnectGetInfoPtr        ( void );

// Apn Predefined List
CHAR *              ModemConnectApnGetString          ( ModemConnectApnEnum eModemApn );

#endif /* MODEM_CONNECT_H */ 
