/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_CELL_INFO_H
#define MODEM_CELL_INFO_H

#include "../ModemData.h"

void                ModemCellInfoInit                  ( void );
void                ModemCellInfoStateMachine          ( void );

// operation
void                ModemCellInfoCheckRun              ( void );
BOOL                ModemCellInfoIsWaitingForCommand   ( void );
BOOL                ModemCellInfoIsReady               ( void );

ModemDataCellInfoType  * ModemCellInfoGetInfoPtr        ( void );

#endif /* MODEM_CELL_INFO_H */ 
