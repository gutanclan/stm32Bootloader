/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_SIM_H
#define MODEM_SIM_H

#include "../ModemData.h"

void                ModemSimInit                  ( void );
void                ModemSimStateMachine          ( void );

// operation
void                ModemSimCheckRun              ( void );
BOOL                ModemSimIsWaitingForCommand   ( void );
BOOL                ModemSimIsReady               ( void );

ModemDataSimCardType  * ModemSimGetInfoPtr        ( void );

#endif /* MODEM_SIM_H */ 
