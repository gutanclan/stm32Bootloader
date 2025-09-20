/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_SIGNAL_H
#define MODEM_SIGNAL_H

void                ModemSignalInit                  ( void );
void                ModemSignalStateMachine          ( void );

// operation
void                ModemSignalRun                   ( void );
BOOL                ModemSignalIsWaitingForCommand   ( void );
BOOL                ModemSignalIsSignalAcquired      ( void );

ModemDataSignalInfoType  * ModemSignalGetDataPtr     ( void );

#endif /* MODEM_SIGNAL_H */ 
