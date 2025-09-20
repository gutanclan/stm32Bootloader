/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_CONFIG_H
#define MODEM_CONFIG_H

void                ModemConfigInit                     ( void );
void                ModemConfigStateMachine             ( void );

// operation
void                ModemConfigRun                      ( void );
BOOL                ModemConfigIsWaitingForCommand      ( void );
BOOL                ModemConfigIsSuccess                ( void );

#endif /* MODEM_CONFIG_H */ 
