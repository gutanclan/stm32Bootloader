/** C Header ******************************************************************

*******************************************************************************/

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

typedef struct
{
    BOOL                        fStateIsFirstEntry;
    UINT8                       bStateCurrent;
    UINT8                       bStatePrevious;
    BOOL                        fTimeOutEnabled;
    TIMER                       xTimeOutTimer;
}StateMachineType;

#define STATE_MACHINE_STRUCT_SIZE_BYTES     sizeof(StateMachineType)

BOOL    StateMachineInit           	( StateMachineType *ptStateMachineState );

BOOL    StateMachineUpdate         	( StateMachineType *ptStateMachineState );
BOOL    StateMachineIsFirtEntry    	( StateMachineType *ptStateMachineState );
BOOL    StateMachineSetTimeOut     	( StateMachineType *ptStateMachineState, UINT32 dwStateTimeOut_mSec );
BOOL    StateMachineIsTimeOut      	( StateMachineType *ptStateMachineState );
BOOL    StateMachineChangeState    	( StateMachineType *ptStateMachineState, UINT8 bNewState );

#endif /* MODEM_POWER_H */
