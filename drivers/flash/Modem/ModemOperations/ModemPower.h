/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_POWER_H
#define MODEM_POWER_H

void                ModemPowerInit                      ( void );
void                ModemPowerStateMachine              ( void );

// operation
BOOL                ModemPowerEnablePower               ( BOOL fEnable );
BOOL                ModemPowerIsWaitingForCommand       ( void );
BOOL                ModemPowerIsPowerEnabled            ( void );

#endif /* MODEM_POWER_H */ 
