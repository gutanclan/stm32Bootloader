/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_SET_SYS_CLOCK_H
#define MODEM_SET_SYS_CLOCK_H

void                ModemSetSysClockInit                  ( void );
void                ModemSetSysClockStateMachine          ( void );

void                ModemSetSysClockSetDeltaTimeCorrection( UINT8 bDeltaSecondsTimeCorrection );

// operation
void                ModemSetSysClockRun                   ( void );
BOOL                ModemSetSysClockIsWaitingForCommand   ( void );
BOOL                ModemSetSysClockIsTimeCorrected       ( void );

ModemDataSysClockType  * ModemSetSysClockGetInfoPtr       ( void );

#endif /* MODEM_SET_SYS_CLOCK_H */ 
