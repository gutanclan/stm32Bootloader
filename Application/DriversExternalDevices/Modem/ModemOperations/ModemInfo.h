/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_INFO_H
#define MODEM_INFO_H

#include "../ModemData.h"

void                ModemInfoInit                     ( void );
void                ModemInfoStateMachine             ( void );

// operation
void                ModemInfoRun                      ( void );
BOOL                ModemInfoIsWaitingForCommand      ( void );
BOOL                ModemInfoIsExtracted              ( void );

ModemDataModemInfoType  * ModemGetModemInfoPtr        ( void );

#endif /* MODEM_INFO_H */ 
