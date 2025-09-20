/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_CGREG_STAT_H
#define MODEM_CGREG_STAT_H

#include "ModemRegStat.h"

void                ModemCgRegStatInit                  ( void );
void                ModemCgRegStatStateMachine          ( void );

// operation
void                ModemCgRegStatCheckRun              ( void );
BOOL                ModemCgRegStatIsWaitingForCommand   ( void );
ModemRegStatEnum    ModemCgRegStatGetStatus             ( void );

#endif /* MODEM_CGREG_STAT_H */ 
