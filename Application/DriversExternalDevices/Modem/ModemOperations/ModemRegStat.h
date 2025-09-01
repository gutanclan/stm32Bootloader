/** C Header ******************************************************************

*******************************************************************************/

#ifndef MODEM_REG_STAT_H
#define MODEM_REG_STAT_H

typedef enum
{
    MODEM_REG_STAT_NO_REG       = 0,
    MODEM_REG_STAT_REG_HOME     = 1,
    MODEM_REG_STAT_REG_SEARCHING= 2,
    MODEM_REG_STAT_REG_DENIED   = 3,
    MODEM_REG_STAT_REG_UNKNOWN  = 4,
    MODEM_REG_STAT_REG_ROAMING  = 5,
    
    MODEM_REG_STAT_MAX,
}ModemRegStatEnum;

void                ModemRegStatInit                  ( void );
void                ModemRegStatStateMachine          ( void );

// operation
void                ModemRegStatCheckRun              ( void );
BOOL                ModemRegStatIsWaitingForCommand   ( void );
ModemRegStatEnum    ModemRegStatGetStatus             ( void );

CHAR *              ModemRegStatGetStatusString       ( ModemRegStatEnum eRegStat );

#endif /* MODEM_REG_STAT_H */ 
