/** C Header ******************************************************************

*******************************************************************************/

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

// TODO: convert this into an array/Enum that is READ ONLY(similar as the enum of PowerManagerSystemModeEnum)
typedef struct
{
    UINT32                      dwCsrRegsAtSysInit;
    BOOL                        fSoftwareReset;
    BOOL                        fPowerOnReset;
    BOOL                        fExternalPinReset;
    BOOL                        fWatchDogTimerReset;
    BOOL                        fWindowWatchdogTimerReset;
    BOOL                        fLowPowerReset;
    BOOL                        fBrownOutResetReset;
}PowerManagerPowerResetStruct;

typedef enum
{
    POWER_NAMAGER_SYS_MODE_UNKNOWN = 0,
    POWER_NAMAGER_SYS_MODE_MANUFACTURING,
    POWER_NAMAGER_SYS_MODE_NORMAL,
    
    POWER_NAMAGER_SYS_MODE_MAX
}PowerManagerSystemModeEnum;

typedef enum
{
    POWER_NAMAGER_SYS_EXIT_MODE_SLEEP = 0,
    POWER_NAMAGER_SYS_EXIT_MODE_SHUT_DOWN,
    POWER_NAMAGER_SYS_EXIT_MODE_RESET,
    
    POWER_NAMAGER_SYS_EXIT_MODE_MAX,
}PowerManagerSysExitModeEnum;

BOOL    PowerManagerInit                ( void );
void    PowerManagerUpdate              ( void );

BOOL    PowerManagerSysModeIsInitDone   ( void );
BOOL    PowerManagerSysModeSetMode      ( PowerManagerSystemModeEnum eSysMode );
CHAR *  PowerManagerSysModeGetModeName  ( PowerManagerSystemModeEnum eSysMode );
PowerManagerSystemModeEnum    PowerManagerSysModeGetMode      ( void );

void    PowerManagerSysDefaultSet       ( void );
BOOL    PowerManagerSysExitForceMode    ( PowerManagerSysExitModeEnum eExitMode );
CHAR *  PowerManagerSysExitGetModeName  ( PowerManagerSysExitModeEnum eExitMode );

// TODO: convert this into an array/Enum that is READ ONLY(similar as the enum of PowerManagerSystemModeEnum)
PowerManagerPowerResetStruct * PowerManagerGetResetFlags( void );

void    PowerManagerDebugEnable         ( BOOL fEnable );
BOOL    PowerManagerIsDebugEnabled      ( void );

#endif /* POWER_MANAGER_H */

/* EOF */

